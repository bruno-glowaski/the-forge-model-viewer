// Interfaces
#include "Application/Interfaces/IApp.h"
#include "Application/Interfaces/ICameraController.h"
#include "Application/Interfaces/IFont.h"
#include "Application/Interfaces/IProfiler.h"
#include "Application/Interfaces/IScreenshot.h"
#include "Application/Interfaces/IUI.h"
#include "Game/Interfaces/IScripting.h"
#include "Utilities/Interfaces/IFileSystem.h"
#include "Utilities/Interfaces/ILog.h"
#include "Utilities/Interfaces/ITime.h"
#include "Utilities/RingBuffer.h"

// Renderer
#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

// Math
#include "Utilities/Interfaces/IMemory.h"
#include "Utilities/Math/MathTypes.h"

#include "OrbitCameraController.hpp"

struct UniformBlock {
  CameraMatrix mModelProjectView;

  // Point Light Information
  vec4 mLightPosition;
  vec4 mLightColor;
};

struct UniformBlockSky {
  CameraMatrix mProjectView;
};

const uint32_t gDataBufferCount = 2;

Renderer *pRenderer = NULL;

Queue *pGraphicsQueue = NULL;
GpuCmdRing gGraphicsCmdRing = {};

SwapChain *pSwapChain = NULL;
RenderTarget *pDepthBuffer = NULL;
Semaphore *pImageAcquiredSemaphore = NULL;

const char *const kSceneMeshPath = "castle.bin";
Geometry *pSceneGeometry;
GeometryData *pSceneGeometryData;
Shader *pSceneShader = NULL;
Pipeline *pScenePipeline = NULL;
VertexLayout gSceneVertexLayout = {};
uint32_t gSceneLayoutType = 0;

Shader *pSkyBoxDrawShader = NULL;
Buffer *pSkyBoxVertexBuffer = NULL;
Pipeline *pSkyBoxDrawPipeline = NULL;
RootSignature *pRootSignature = NULL;
Sampler *pSamplerSkyBox = NULL;
Texture *pSkyBoxTextures[6];
DescriptorSet *pDescriptorSetTexture = {NULL};
DescriptorSet *pDescriptorSetUniforms = {NULL};

Buffer *pSceneUniformBuffer[gDataBufferCount] = {NULL};
Buffer *pSkyboxUniformBuffer[gDataBufferCount] = {NULL};

uint32_t gFrameIndex = 0;
ProfileToken gGpuProfileToken = PROFILE_INVALID_TOKEN;

UniformBlock gUniformData;
UniformBlockSky gUniformDataSky;

ICameraController *pCameraController = NULL;

uint32_t gFontID = 0;

const char *const kPSkyBoxImageFileNames[] = {
    "Skybox_right1.tex",  "Skybox_left2.tex",  "Skybox_top3.tex",
    "Skybox_bottom4.tex", "Skybox_front5.tex", "Skybox_back6.tex"};

FontDrawDesc gFrameTimeDraw;

// Generate sky box vertex buffer
const float gSkyBoxPoints[] = {
    10.0f,  -10.0f, -10.0f, 6.0f, // -z
    -10.0f, -10.0f, -10.0f, 6.0f,   -10.0f, 10.0f,  -10.0f,
    6.0f,   -10.0f, 10.0f,  -10.0f, 6.0f,   10.0f,  10.0f,
    -10.0f, 6.0f,   10.0f,  -10.0f, -10.0f, 6.0f,

    -10.0f, -10.0f, 10.0f,  2.0f, //-x
    -10.0f, -10.0f, -10.0f, 2.0f,   -10.0f, 10.0f,  -10.0f,
    2.0f,   -10.0f, 10.0f,  -10.0f, 2.0f,   -10.0f, 10.0f,
    10.0f,  2.0f,   -10.0f, -10.0f, 10.0f,  2.0f,

    10.0f,  -10.0f, -10.0f, 1.0f, //+x
    10.0f,  -10.0f, 10.0f,  1.0f,   10.0f,  10.0f,  10.0f,
    1.0f,   10.0f,  10.0f,  10.0f,  1.0f,   10.0f,  10.0f,
    -10.0f, 1.0f,   10.0f,  -10.0f, -10.0f, 1.0f,

    -10.0f, -10.0f, 10.0f,  5.0f, // +z
    -10.0f, 10.0f,  10.0f,  5.0f,   10.0f,  10.0f,  10.0f,
    5.0f,   10.0f,  10.0f,  10.0f,  5.0f,   10.0f,  -10.0f,
    10.0f,  5.0f,   -10.0f, -10.0f, 10.0f,  5.0f,

    -10.0f, 10.0f,  -10.0f, 3.0f, //+y
    10.0f,  10.0f,  -10.0f, 3.0f,   10.0f,  10.0f,  10.0f,
    3.0f,   10.0f,  10.0f,  10.0f,  3.0f,   -10.0f, 10.0f,
    10.0f,  3.0f,   -10.0f, 10.0f,  -10.0f, 3.0f,

    10.0f,  -10.0f, 10.0f,  4.0f, //-y
    10.0f,  -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f, -10.0f,
    4.0f,   -10.0f, -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f,
    10.0f,  4.0f,   10.0f,  -10.0f, 10.0f,  4.0f,
};

static unsigned char gPipelineStatsCharArray[2048] = {};
static bstring gPipelineStats = bfromarr(gPipelineStatsCharArray);

class ModelViewer : public IApp {
public:
  bool Init() {
    // window and renderer setup
    RendererDesc settings;
    memset(&settings, 0, sizeof(settings));
    initGPUConfiguration(settings.pExtendedSettings);
    initRenderer(GetName(), &settings, &pRenderer);
    // check for init success
    if (!pRenderer) {
      ShowUnsupportedMessage("Failed To Initialize renderer!");
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
    cmdRingDesc.mPoolCount = gDataBufferCount;
    cmdRingDesc.mCmdPerPoolCount = 1;
    cmdRingDesc.mAddSyncPrimitives = true;
    initGpuCmdRing(pRenderer, &cmdRingDesc, &gGraphicsCmdRing);

    initSemaphore(pRenderer, &pImageAcquiredSemaphore);

    initResourceLoaderInterface(pRenderer);

    // Load scene
    gSceneVertexLayout.mAttribCount = 3;
    gSceneVertexLayout.mBindingCount = 1;
    gSceneVertexLayout.mBindings[0].mStride = sizeof(float3) + sizeof(uint32_t) + sizeof(float);
    gSceneVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    gSceneVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    gSceneVertexLayout.mAttribs[0].mLocation = 0;
    gSceneVertexLayout.mAttribs[0].mOffset = 0;
    gSceneVertexLayout.mAttribs[0].mBinding = 0;
    gSceneVertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
    gSceneVertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32_UINT;
    gSceneVertexLayout.mAttribs[1].mLocation = 1;
    gSceneVertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
    gSceneVertexLayout.mAttribs[1].mBinding = 0;
    gSceneVertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
    gSceneVertexLayout.mAttribs[2].mFormat = TinyImageFormat_R16G16_SFLOAT;
    gSceneVertexLayout.mAttribs[2].mLocation = 2;
    gSceneVertexLayout.mAttribs[2].mOffset = sizeof(float3) + sizeof(uint32_t);
    gSceneVertexLayout.mAttribs[2].mBinding = 0;
    GeometryLoadDesc sceneGDesc = {};
    sceneGDesc.ppGeometry = &pSceneGeometry;
    sceneGDesc.ppGeometryData = &pSceneGeometryData;
    sceneGDesc.pFileName = kSceneMeshPath;
    sceneGDesc.pVertexLayout = &gSceneVertexLayout;
    addResource(&sceneGDesc, NULL);

    for (int i = 0; i < 6; ++i) {
      TextureLoadDesc textureDesc = {};
      textureDesc.pFileName = kPSkyBoxImageFileNames[i];
      textureDesc.ppTexture = &pSkyBoxTextures[i];
      textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
      addResource(&textureDesc, NULL);
    }

    SamplerDesc samplerDesc = {FILTER_LINEAR,
                               FILTER_LINEAR,
                               MIPMAP_MODE_NEAREST,
                               ADDRESS_MODE_CLAMP_TO_EDGE,
                               ADDRESS_MODE_CLAMP_TO_EDGE,
                               ADDRESS_MODE_CLAMP_TO_EDGE};
    addSampler(pRenderer, &samplerDesc, &pSamplerSkyBox);

    uint64_t skyBoxDataSize = 4 * 6 * 6 * sizeof(float);
    BufferLoadDesc skyboxVbDesc = {};
    skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
    skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
    skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
    skyboxVbDesc.pData = gSkyBoxPoints;
    skyboxVbDesc.ppBuffer = &pSkyBoxVertexBuffer;
    addResource(&skyboxVbDesc, NULL);

    BufferLoadDesc ubDesc = {};
    ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    ubDesc.pData = NULL;
    for (uint32_t i = 0; i < gDataBufferCount; ++i) {
      ubDesc.mDesc.pName = "ProjViewUniformBuffer";
      ubDesc.mDesc.mSize = sizeof(UniformBlock);
      ubDesc.ppBuffer = &pSceneUniformBuffer[i];
      addResource(&ubDesc, NULL);
      ubDesc.mDesc.pName = "SkyboxUniformBuffer";
      ubDesc.mDesc.mSize = sizeof(UniformBlockSky);
      ubDesc.ppBuffer = &pSkyboxUniformBuffer[i];
      addResource(&ubDesc, NULL);
    }

    FontDesc font = {};
    font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
    fntDefineFonts(&font, 1, &gFontID);

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

    // Gpu profiler can only be added after initProfile.
    gGpuProfileToken = initGpuProfiler(pRenderer, pGraphicsQueue, "Graphics");

    waitForAllResourceLoads();

    CameraMotionParameters cmp{{}, 600.0f, 200.0f, 10.0f, PI};
    vec3 camPos{0.0f, 0.0f, 10.0f};
    vec3 lookAt{vec3(0)};

    pCameraController = initOrbitCameraController(camPos, lookAt);

    pCameraController->setMotionParameters(cmp);

    AddCustomInputBindings();
    initScreenshotInterface(pRenderer, pGraphicsQueue);
    gFrameIndex = 0;

    return true;
  }

  void Exit() {
    exitScreenshotInterface();

    exitCameraController(pCameraController);

    exitUserInterface();

    exitFontSystem();

    // Exit profile
    exitProfiler();

    for (uint32_t i = 0; i < gDataBufferCount; ++i) {
      removeResource(pSceneUniformBuffer[i]);
      removeResource(pSkyboxUniformBuffer[i]);
    }

    removeResource(pSceneGeometry);
    removeResource(pSceneGeometryData);

    removeResource(pSkyBoxVertexBuffer);

    for (uint i = 0; i < 6; ++i)
      removeResource(pSkyBoxTextures[i]);

    removeSampler(pRenderer, pSamplerSkyBox);

    exitGpuCmdRing(pRenderer, &gGraphicsCmdRing);
    exitSemaphore(pRenderer, pImageAcquiredSemaphore);

    exitResourceLoaderInterface(pRenderer);

    exitQueue(pRenderer, pGraphicsQueue);

    exitRenderer(pRenderer);
    exitGPUConfiguration();
    pRenderer = NULL;
  }

  bool Load(ReloadDesc *pReloadDesc) {
    if (pReloadDesc->mType & RELOAD_TYPE_SHADER) {
      addShaders();
      addRootSignatures();
      addDescriptorSets();
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
      // we only need to reload gui when the size of window changed
      loadProfilerUI(mSettings.mWidth, mSettings.mHeight);

      if (!addSwapChain())
        return false;

      if (!addDepthBuffer())
        return false;
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET)) {
      addPipelines();
    }

    prepareDescriptorSets();

    UserInterfaceLoadDesc uiLoad = {};
    uiLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
    uiLoad.mHeight = mSettings.mHeight;
    uiLoad.mWidth = mSettings.mWidth;
    uiLoad.mLoadType = pReloadDesc->mType;
    loadUserInterface(&uiLoad);

    FontSystemLoadDesc fontLoad = {};
    fontLoad.mColorFormat = pSwapChain->ppRenderTargets[0]->mFormat;
    fontLoad.mHeight = mSettings.mHeight;
    fontLoad.mWidth = mSettings.mWidth;
    fontLoad.mLoadType = pReloadDesc->mType;
    loadFontSystem(&fontLoad);

    return true;
  }

  void Unload(ReloadDesc *pReloadDesc) {
    waitQueueIdle(pGraphicsQueue);

    unloadFontSystem(pReloadDesc->mType);
    unloadUserInterface(pReloadDesc->mType);

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET)) {
      removePipelines();
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
      removeSwapChain(pRenderer, pSwapChain);
      removeRenderTarget(pRenderer, pDepthBuffer);
      unloadProfilerUI();
    }

    if (pReloadDesc->mType & RELOAD_TYPE_SHADER) {
      removeDescriptorSets();
      removeRootSignatures();
      removeShaders();
    }
  }

  void Update(float deltaTime) {
    if (!uiIsFocused()) {
      pCameraController->onMove(
          {inputGetValue(0, CUSTOM_MOVE_X), inputGetValue(0, CUSTOM_MOVE_Y)});
      pCameraController->onRotate(
          {inputGetValue(0, CUSTOM_LOOK_X), inputGetValue(0, CUSTOM_LOOK_Y)});
      pCameraController->onMoveY(inputGetValue(0, CUSTOM_MOVE_UP));
      if (inputGetValue(0, CUSTOM_RESET_VIEW)) {
        pCameraController->resetView();
      }
      if (inputGetValue(0, CUSTOM_TOGGLE_FULLSCREEN)) {
        toggleFullscreen(pWindow);
      }
      if (inputGetValue(0, CUSTOM_TOGGLE_UI)) {
        uiToggleActive();
      }
      if (inputGetValue(0, CUSTOM_DUMP_PROFILE)) {
        dumpProfileData(GetName());
      }
      if (inputGetValue(0, CUSTOM_EXIT)) {
        requestShutdown();
      }
    }

    pCameraController->update(deltaTime);
    /************************************************************************/
    // Scene Update
    /************************************************************************/
    static float currentTime = 0.0f;
    currentTime += deltaTime * 1000.0f;

    // update camera with time
    mat4 viewMat = pCameraController->getViewMatrix();

    const float aspectInverse =
        (float)mSettings.mHeight / (float)mSettings.mWidth;
    const float horizontal_fov = PI / 2.0f;
    CameraMatrix projMat = CameraMatrix::perspectiveReverseZ(
        horizontal_fov, aspectInverse, 0.1f, 1000.0f);
    gUniformData.mModelProjectView = projMat * viewMat;

    // point light parameters
    gUniformData.mLightPosition = vec4(0, 0, 0, 0);
    gUniformData.mLightColor = vec4(0.9f, 0.9f, 0.7f, 1.0f); // Pale Yellow

    viewMat.setTranslation(vec3(0));
    gUniformDataSky = {};
    gUniformDataSky.mProjectView = projMat * viewMat;
  }

  void Draw() {
    if ((bool)pSwapChain->mEnableVsync != mSettings.mVSyncEnabled) {
      waitQueueIdle(pGraphicsQueue);
      ::toggleVSync(pRenderer, &pSwapChain);
    }

    uint32_t swapchainImageIndex;
    acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, NULL,
                     &swapchainImageIndex);

    RenderTarget *pRenderTarget =
        pSwapChain->ppRenderTargets[swapchainImageIndex];
    GpuCmdRingElement elem =
        getNextGpuCmdRingElement(&gGraphicsCmdRing, true, 1);

    // Stall if CPU is running "gDataBufferCount" frames ahead of GPU
    FenceStatus fenceStatus;
    getFenceStatus(pRenderer, elem.pFence, &fenceStatus);
    if (fenceStatus == FENCE_STATUS_INCOMPLETE)
      waitForFences(pRenderer, 1, &elem.pFence);

    // Update uniform buffers
    BufferUpdateDesc viewProjCbv = {pSceneUniformBuffer[gFrameIndex]};
    beginUpdateResource(&viewProjCbv);
    memcpy(viewProjCbv.pMappedData, &gUniformData, sizeof(gUniformData));
    endUpdateResource(&viewProjCbv);

    BufferUpdateDesc skyboxViewProjCbv = {pSkyboxUniformBuffer[gFrameIndex]};
    beginUpdateResource(&skyboxViewProjCbv);
    memcpy(skyboxViewProjCbv.pMappedData, &gUniformDataSky,
           sizeof(gUniformDataSky));
    endUpdateResource(&skyboxViewProjCbv);

    // Reset cmd pool for this frame
    resetCmdPool(pRenderer, elem.pCmdPool);

    Cmd *cmd = elem.pCmds[0];
    beginCmd(cmd);

    cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);

    RenderTargetBarrier barriers[] = {
        {pRenderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Canvas");

    // simply record the screen cleaning command
    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = {pRenderTarget, LOAD_ACTION_CLEAR};
    bindRenderTargets.mDepthStencil = {pDepthBuffer, LOAD_ACTION_CLEAR};
    cmdBindRenderTargets(cmd, &bindRenderTargets);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth,
                   (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, pRenderTarget->mWidth, pRenderTarget->mHeight);

    const uint32_t skyboxVbStride = sizeof(float) * 4;
    // draw skybox
    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth,
                   (float)pRenderTarget->mHeight, 1.0f, 1.0f);
    cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &skyboxVbStride, NULL);
    cmdDraw(cmd, 36, 0);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)pRenderTarget->mWidth,
                   (float)pRenderTarget->mHeight, 0.0f, 1.0f);
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Scene");
    cmdBindPipeline(cmd, pScenePipeline);
    cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 1, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, pSceneGeometry->mVertexBufferCount,
                        pSceneGeometry->pVertexBuffers,
                        &gSceneVertexLayout.mBindings[0].mStride, nullptr);
    cmdBindIndexBuffer(cmd, pSceneGeometry->pIndexBuffer, INDEX_TYPE_UINT16, 0);
    cmdDrawIndexed(cmd, pSceneGeometry->mIndexCount, 0, 0);
    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);

    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken); // Draw Canvas
    cmdBindRenderTargets(cmd, NULL);

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw UI");

    bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = {pRenderTarget, LOAD_ACTION_LOAD};
    bindRenderTargets.mDepthStencil = {NULL, LOAD_ACTION_DONTCARE};
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    gFrameTimeDraw.mFontColor = 0xff00ffff;
    gFrameTimeDraw.mFontSize = 18.0f;
    gFrameTimeDraw.mFontID = gFontID;
    float2 txtSizePx =
        cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
    cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), gGpuProfileToken,
                      &gFrameTimeDraw);

    cmdDrawUserInterface(cmd);

    cmdEndGpuTimestampQuery(cmd, gGpuProfileToken);
    cmdBindRenderTargets(cmd, NULL);

    barriers[0] = {pRenderTarget, RESOURCE_STATE_RENDER_TARGET,
                   RESOURCE_STATE_PRESENT};
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdEndGpuFrameProfile(cmd, gGpuProfileToken);

    endCmd(cmd);

    FlushResourceUpdateDesc flushUpdateDesc = {};
    flushUpdateDesc.mNodeIndex = 0;
    flushResourceUpdates(&flushUpdateDesc);
    Semaphore *waitSemaphores[2] = {flushUpdateDesc.pOutSubmittedSemaphore,
                                    pImageAcquiredSemaphore};

    QueueSubmitDesc submitDesc = {};
    submitDesc.mCmdCount = 1;
    submitDesc.mSignalSemaphoreCount = 1;
    submitDesc.mWaitSemaphoreCount = TF_ARRAY_COUNT(waitSemaphores);
    submitDesc.ppCmds = &cmd;
    submitDesc.ppSignalSemaphores = &elem.pSemaphore;
    submitDesc.ppWaitSemaphores = waitSemaphores;
    submitDesc.pSignalFence = elem.pFence;
    queueSubmit(pGraphicsQueue, &submitDesc);

    QueuePresentDesc presentDesc = {};
    presentDesc.mIndex = (uint8_t)swapchainImageIndex;
    presentDesc.mWaitSemaphoreCount = 1;
    presentDesc.pSwapChain = pSwapChain;
    presentDesc.ppWaitSemaphores = &elem.pSemaphore;
    presentDesc.mSubmitDone = true;

    queuePresent(pGraphicsQueue, &presentDesc);
    flipProfiler();

    gFrameIndex = (gFrameIndex + 1) % gDataBufferCount;
  }

  const char *GetName() { return "ModelViewer"; }

  static void RequestShadersReload(void *) {
    ReloadDesc reload{RELOAD_TYPE_SHADER};
    requestReload(&reload);
  }

private:
  bool addSwapChain() {
    SwapChainDesc swapChainDesc = {};
    swapChainDesc.mWindowHandle = pWindow->handle;
    swapChainDesc.mPresentQueueCount = 1;
    swapChainDesc.ppPresentQueues = &pGraphicsQueue;
    swapChainDesc.mWidth = mSettings.mWidth;
    swapChainDesc.mHeight = mSettings.mHeight;
    swapChainDesc.mImageCount =
        getRecommendedSwapchainImageCount(pRenderer, &pWindow->handle);
    swapChainDesc.mColorFormat = getSupportedSwapchainFormat(
        pRenderer, &swapChainDesc, COLOR_SPACE_SDR_SRGB);
    swapChainDesc.mColorSpace = COLOR_SPACE_SDR_SRGB;
    swapChainDesc.mEnableVsync = mSettings.mVSyncEnabled;
    swapChainDesc.mFlags =
        SWAP_CHAIN_CREATION_FLAG_ENABLE_FOVEATED_RENDERING_VR;
    ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

    return pSwapChain != NULL;
  }

  bool addDepthBuffer() {
    // Add depth buffer
    RenderTargetDesc depthRT = {};
    depthRT.mArraySize = 1;
    depthRT.mClearValue.depth = 0.0f;
    depthRT.mClearValue.stencil = 0;
    depthRT.mDepth = 1;
    depthRT.mFormat = TinyImageFormat_D32_SFLOAT;
    depthRT.mStartState = RESOURCE_STATE_DEPTH_WRITE;
    depthRT.mHeight = mSettings.mHeight;
    depthRT.mSampleCount = SAMPLE_COUNT_1;
    depthRT.mSampleQuality = 0;
    depthRT.mWidth = mSettings.mWidth;
    depthRT.mFlags =
        TEXTURE_CREATION_FLAG_ON_TILE | TEXTURE_CREATION_FLAG_VR_MULTIVIEW;
    addRenderTarget(pRenderer, &depthRT, &pDepthBuffer);

    return pDepthBuffer != NULL;
  }

  void addDescriptorSets() {
    DescriptorSetDesc desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1};
    addDescriptorSet(pRenderer, &desc, &pDescriptorSetTexture);
    desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME,
            gDataBufferCount * 2};
    addDescriptorSet(pRenderer, &desc, &pDescriptorSetUniforms);
  }

  void removeDescriptorSets() {
    removeDescriptorSet(pRenderer, pDescriptorSetTexture);
    removeDescriptorSet(pRenderer, pDescriptorSetUniforms);
  }

  void addRootSignatures() {
    Shader *shaders[2];
    uint32_t shadersCount = 0;
    shaders[shadersCount++] = pSceneShader;
    shaders[shadersCount++] = pSkyBoxDrawShader;

    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = shadersCount;
    rootDesc.ppShaders = shaders;
    addRootSignature(pRenderer, &rootDesc, &pRootSignature);
  }

  void removeRootSignatures() {
    removeRootSignature(pRenderer, pRootSignature);
  }

  void addShaders() {
    ShaderLoadDesc skyShader = {};
    skyShader.mVert.pFileName = "skybox.vert";
    skyShader.mFrag.pFileName = "skybox.frag";

    ShaderLoadDesc basicShader = {};
    basicShader.mVert.pFileName = "basic.vert";
    basicShader.mFrag.pFileName = "basic.frag";

    addShader(pRenderer, &skyShader, &pSkyBoxDrawShader);
    addShader(pRenderer, &basicShader, &pSceneShader);
  }

  void removeShaders() {
    removeShader(pRenderer, pSceneShader);
    removeShader(pRenderer, pSkyBoxDrawShader);
  }

  void addPipelines() {
    RasterizerStateDesc rasterizerStateDesc = {};
    rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    RasterizerStateDesc sceneRasterizerStateDesc = {};
    sceneRasterizerStateDesc.mCullMode = CULL_MODE_NONE;

    DepthStateDesc depthStateDesc = {};
    depthStateDesc.mDepthTest = true;
    depthStateDesc.mDepthWrite = true;
    depthStateDesc.mDepthFunc = CMP_GEQUAL;

    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc &pipelineSettings = desc.mGraphicsDesc;
    pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineSettings.mRenderTargetCount = 1;
    pipelineSettings.pDepthState = &depthStateDesc;
    pipelineSettings.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
    pipelineSettings.mSampleCount =
        pSwapChain->ppRenderTargets[0]->mSampleCount;
    pipelineSettings.mSampleQuality =
        pSwapChain->ppRenderTargets[0]->mSampleQuality;
    pipelineSettings.mDepthStencilFormat = pDepthBuffer->mFormat;
    pipelineSettings.pRootSignature = pRootSignature;
    pipelineSettings.pShaderProgram = pSceneShader;
    pipelineSettings.pVertexLayout = &gSceneVertexLayout;
    pipelineSettings.pRasterizerState = &sceneRasterizerStateDesc;
    pipelineSettings.mVRFoveatedRendering = true;
    addPipeline(pRenderer, &desc, &pScenePipeline);

    // layout and pipeline for skybox draw
    VertexLayout vertexLayout = {};
    vertexLayout.mBindingCount = 1;
    vertexLayout.mBindings[0].mStride = sizeof(float4);
    vertexLayout.mAttribCount = 1;
    vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.mAttribs[0].mBinding = 0;
    vertexLayout.mAttribs[0].mLocation = 0;
    vertexLayout.mAttribs[0].mOffset = 0;
    pipelineSettings.pVertexLayout = &vertexLayout;

    pipelineSettings.pDepthState = NULL;
    pipelineSettings.pRasterizerState = &rasterizerStateDesc;
    pipelineSettings.pShaderProgram = pSkyBoxDrawShader; //-V519
    addPipeline(pRenderer, &desc, &pSkyBoxDrawPipeline);
  }

  void removePipelines() {
    removePipeline(pRenderer, pSkyBoxDrawPipeline);
    removePipeline(pRenderer, pScenePipeline);
  }

  void prepareDescriptorSets() {
    // Prepare descriptor sets
    DescriptorData params[7] = {};
    params[0].pName = "RightText";
    params[0].ppTextures = &pSkyBoxTextures[0];
    params[1].pName = "LeftText";
    params[1].ppTextures = &pSkyBoxTextures[1];
    params[2].pName = "TopText";
    params[2].ppTextures = &pSkyBoxTextures[2];
    params[3].pName = "BotText";
    params[3].ppTextures = &pSkyBoxTextures[3];
    params[4].pName = "FrontText";
    params[4].ppTextures = &pSkyBoxTextures[4];
    params[5].pName = "BackText";
    params[5].ppTextures = &pSkyBoxTextures[5];
    params[6].pName = "uSampler0";
    params[6].ppSamplers = &pSamplerSkyBox;
    updateDescriptorSet(pRenderer, 0, pDescriptorSetTexture, 7, params);

    for (uint32_t i = 0; i < gDataBufferCount; ++i) {
      DescriptorData uParams[1] = {};
      uParams[0].pName = "uniformBlock";
      uParams[0].ppBuffers = &pSkyboxUniformBuffer[i];
      updateDescriptorSet(pRenderer, i * 2 + 0, pDescriptorSetUniforms, 1,
                          uParams);

      uParams[0].pName = "uniformBlock";
      uParams[0].ppBuffers = &pSceneUniformBuffer[i];
      updateDescriptorSet(pRenderer, i * 2 + 1, pDescriptorSetUniforms, 1,
                          uParams);
    }
  }
};

DEFINE_APPLICATION_MAIN(ModelViewer)
