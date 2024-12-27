#include "RenderContext.hpp"

#include "Application/Interfaces/IFont.h"
#include "Application/Interfaces/IScreenshot.h"
#include "Application/Interfaces/IUI.h"

bool RenderContext::Init(const char *appName) {
  RendererDesc settings;
  memset(&settings, 0, sizeof(settings));
  initGPUConfiguration(settings.pExtendedSettings);
  initRenderer(appName, &settings, &pRenderer);
  // check for init success
  if (!pRenderer) {
    return false;
  }
  setupGPUConfigurationPlatformParameters(pRenderer,
                                          settings.pExtendedSettings);

  QueueDesc queueDesc = {};
  queueDesc.mType = QUEUE_TYPE_GRAPHICS;
  queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
  initQueue(pRenderer, &queueDesc, &pGraphicsQueue);

  GpuCmdRingDesc cmdRingDesc = {};
  cmdRingDesc.pQueue = pGraphicsQueue;
  cmdRingDesc.mPoolCount = kDataBufferCount;
  cmdRingDesc.mCmdPerPoolCount = 1;
  cmdRingDesc.mAddSyncPrimitives = true;
  initGpuCmdRing(pRenderer, &cmdRingDesc, &mGraphicsCmdRing);

  initSemaphore(pRenderer, &pImageAcquiredSemaphore);

  initResourceLoaderInterface(pRenderer);

  FontSystemDesc fontRenderDesc = {};
  fontRenderDesc.pRenderer = pRenderer;
  if (!initFontSystem(&fontRenderDesc))
    return false; // report?

  UserInterfaceDesc uiRenderDesc = {};
  uiRenderDesc.pRenderer = pRenderer;
  initUserInterface(&uiRenderDesc);

  // Initialize micro profiler and its UI.
  ProfilerDesc profiler = {};
  profiler.pRenderer = pRenderer;
  initProfiler(&profiler);

  initScreenshotInterface(pRenderer, pGraphicsQueue);

  return true;
}

void RenderContext::Exit() {
  exitScreenshotInterface();

  exitUserInterface();

  exitFontSystem();

  exitProfiler();

  exitGpuCmdRing(pRenderer, &mGraphicsCmdRing);
  exitSemaphore(pRenderer, pImageAcquiredSemaphore);

  exitResourceLoaderInterface(pRenderer);

  exitQueue(pRenderer, pGraphicsQueue);

  exitRenderer(pRenderer);
  exitGPUConfiguration();
  pRenderer = NULL;
}

bool RenderContext::Load(WindowHandle hWindow, uint32_t width, uint32_t height,
                         bool vSyncEnabled, ReloadDesc *pReloadDesc) {
  if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
    loadProfilerUI(width, height);

    // Add swapchain
    SwapChainDesc swapChainDesc = {};
    swapChainDesc.mWindowHandle = hWindow;
    swapChainDesc.mPresentQueueCount = 1;
    swapChainDesc.ppPresentQueues = &pGraphicsQueue;
    swapChainDesc.mWidth = width;
    swapChainDesc.mHeight = height;
    swapChainDesc.mImageCount =
        getRecommendedSwapchainImageCount(pRenderer, &hWindow);
    swapChainDesc.mColorFormat = getSupportedSwapchainFormat(
        pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
    swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
    swapChainDesc.mEnableVsync = vSyncEnabled;
    swapChainDesc.mFlags =
        SWAP_CHAIN_CREATION_FLAG_ENABLE_FOVEATED_RENDERING_VR;
    ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);
    if (pSwapChain == NULL)
      return false;

    // Add depth buffer
    RenderTargetDesc depthRT = {};
    depthRT.mArraySize = 1;
    depthRT.mClearValue.depth = 0.0f;
    depthRT.mClearValue.stencil = 0;
    depthRT.mDepth = 1;
    depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
    depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
    depthRT.mWidth = width;
    depthRT.mHeight = height;
    depthRT.mSampleCount = SAMPLE_COUNT_1;
    depthRT.mSampleQuality = 0;
    depthRT.mFlags =
        TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
    addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);
    if (pDepthBuffer == NULL)
      return false;
  }

  UserInterfaceLoadDesc uiLoad = {};
  uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
  uiLoad.mWidth = width;
  uiLoad.mHeight = height;
  uiLoad.mLoadType = pReloadDesc->mType;
  loadUserInterface(&uiLoad);

  FontSystemLoadDesc fontLoad = {};
  fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
  fontLoad.mWidth = width;
  fontLoad.mHeight = height;
  fontLoad.mLoadType = pReloadDesc->mType;
  loadFontSystem(&fontLoad);
}

void RenderContext::Unload(ReloadDesc *pReloadDesc) {

  unloadFontSystem(pReloadDesc->mType);
  unloadUserInterface(pReloadDesc->mType);

  if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
    removeSwapChain(pRenderer, pSwapChain);
    removeRenderTarget(pRenderer, pDepthBuffer);
    unloadProfilerUI();
  }
}

void RenderContext::WaitIdle() { waitQueueIdle(pGraphicsQueue); }

void RenderContext::ToggleVSync() { ::toggleVSync(pRenderer, &pSwapChain); }

bool RenderContext::IsVSyncEnabled() const {
  return (bool)pSwapChain->mEnableVsync;
}

ProfileToken RenderContext::CreateGpuProfiler(const char *pProfilerName) {
  return initGpuProfiler(pRenderer, pGraphicsQueue, pProfilerName);
}

Sampler *RenderContext::CreateSampler(SamplerDesc *pDesc) {
  Sampler *pSampler;
  addSampler(pRenderer, pDesc, &pSampler);
  return pSampler;
}
void RenderContext::DestroySampler(Sampler *pSampler) {
  removeSampler(pRenderer, pSampler);
}

RootSignature *RenderContext::CreateRootSignature(RootSignatureDesc *pDesc) {
  RootSignature *pRootSignature;
  addRootSignature(pRenderer, pDesc, &pRootSignature);
  return pRootSignature;
}
void RenderContext::DestroyRootSignature(RootSignature *pRootSignature) {
  removeRootSignature(pRenderer, pRootSignature);
}

DescriptorSet *RenderContext::CreateDescriptorSet(DescriptorSetDesc *pDesc) {
  DescriptorSet *pDescriptorSet;
  addDescriptorSet(pRenderer, pDesc, &pDescriptorSet);
  return pDescriptorSet;
}
void RenderContext::UpdateDescriptorSet(DescriptorSet *pDescriptorSet,
                                        uint32_t index, uint32_t count,
                                        DescriptorData *pParams) {
  updateDescriptorSet(pRenderer, index, pDescriptorSet, count, pParams);
}
void RenderContext::DestroyDescriptorSet(DescriptorSet *pDescriptorSet) {
  removeDescriptorSet(pRenderer, pDescriptorSet);
}

Shader *RenderContext::LoadShader(ShaderLoadDesc *pDesc) {
  Shader *pShader;
  addShader(pRenderer, pDesc, &pShader);
  return pShader;
}
void RenderContext::DestroyShader(Shader *pShader) {
  removeShader(pRenderer, pShader);
}

Pipeline *RenderContext::CreatePipeline(PipelineDesc *pDesc) {
  Pipeline *pPipeline;
  addPipeline(pRenderer, pDesc, &pPipeline);
  return pPipeline;
}
void RenderContext::DestroyPipeline(Pipeline *pPipeline) {
  removePipeline(pRenderer, pPipeline);
}

RenderContext::Frame RenderContext::BeginFrame() {
  uint32_t imageIndex;
  acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL,
                   &imageIndex);

  RenderTarget *pRenderTarget = pSwapChain->ppRenderTargets[imageIndex];
  GpuCmdRingElement elem = getNextGpuCmdRingElement(&mGraphicsCmdRing, true, 1);

  // Stall if CPU is running `kDataBufferCount` frames ahead of GPU
  FenceStatus fenceStatus;
  getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
  if (fenceStatus == FENCE_STATUS_INCOMPLETE)
    waitForFences(pRenderer, 1, &elem.pFence);

  // Reset cmd pool for this frame
  resetCmdPool(pRenderer, elem.pCmdPool);

  beginCmd(elem.pCmds[0]);

  return RenderContext::Frame{mFrameIndex, imageIndex, pRenderTarget,
                              pDepthBuffer, elem};
}

void RenderContext::EndFrame(RenderContext::Frame &&frame) {
  endCmd(frame.mCmdRingElement.pCmds[0]);

  FlushResourceUpdateDesc flushUpdateDesc = {};
  flushUpdateDesc.mNodeIndex = 0;
  flushResourceUpdates(&flushUpdateDesc);
  Semaphore *waitSemaphores[2] = {flushUpdateDesc.pOutSubmittedSemaphore,
                                  pImageAcquiredSemaphore};

  QueueSubmitDesc submitDesc = {};
  submitDesc.mCmdCount = 1;
  submitDesc.mSignalSemaphoreCount = 1;
  submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
  submitDesc.ppCmds = &frame.mCmdRingElement.pCmds[0];
  submitDesc.ppSignalSemaphores = &frame.mCmdRingElement.pSemaphore;
  submitDesc.ppWaitSemaphores = waitSemaphores;
  submitDesc.pSignalFence = frame.mCmdRingElement.pFence;
  queueSubmit(pGraphicsQueue, &submitDesc);

  QueuePresentDesc presentDesc = {};
  presentDesc.mIndex = (uint8_t)frame.imageIndex;
  presentDesc.mWaitSemaphoreCount = 1;
  presentDesc.pSwapChain = pSwapChain;
  presentDesc.ppWaitSemaphores = &frame.mCmdRingElement.pSemaphore;
  presentDesc.mSubmitDone = true;

  queuePresent(pGraphicsQueue, &presentDesc);
  flipProfiler();

  mFrameIndex = (mFrameIndex + 1) % RenderContext::kDataBufferCount;
}