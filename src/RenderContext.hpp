#pragma once

#include "Application/Interfaces/IProfiler.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "Utilities/RingBuffer.h"

class RenderContext {
public:
  const static uint32_t kDataBufferCount = 2;

  bool Init(const char *appName);
  void Exit();

  bool Load(WindowHandle hWindow, uint32_t width, uint32_t height,
            bool vSyncEnabled, ReloadDesc *pReloadDesc);
  void Unload(ReloadDesc *pReloadDesc);

  bool IsVSyncEnabled() const;
  inline TinyImageFormat GetSwapChainFormat() const {
    return pSwapChain->ppRenderTargets[0]->mFormat;
  }
  inline SampleCount GetSwapChainSampleCount() const {
    return pSwapChain->ppRenderTargets[0]->mSampleCount;
  }
  inline uint32_t GetSwapChainSampleQuality() const {
    return pSwapChain->ppRenderTargets[0]->mSampleQuality;
  }
  inline TinyImageFormat GetDepthFormat() const {
    return pDepthBuffer->mFormat;
  }

  void WaitIdle();
  void ToggleVSync();

  ProfileToken CreateGpuProfiler(const char *pProfilerName);

  Sampler *CreateSampler(SamplerDesc *pDesc);
  void DestroySampler(Sampler *pSampler);

  RootSignature *CreateRootSignature(RootSignatureDesc *pDesc);
  void DestroyRootSignature(RootSignature *pRootSignature);

  DescriptorSet *CreateDescriptorSet(DescriptorSetDesc *pDesc);
  void UpdateDescriptorSet(DescriptorSet *pDescriptorSet, uint32_t index,
                           uint32_t count, DescriptorData *pParams);
  void DestroyDescriptorSet(DescriptorSet *pDescriptorSet);

  Shader *LoadShader(ShaderLoadDesc *pDesc);
  void DestroyShader(Shader *pShader);

  Pipeline *CreatePipeline(PipelineDesc *pDesc);
  void DestroyPipeline(Pipeline *pPipeline);

  struct Frame {
    uint32_t index;
    uint32_t imageIndex;
    RenderTarget *pImage;
    RenderTarget *pDepthBuffer;
    GpuCmdRingElement mCmdRingElement;
  };

  Frame BeginFrame();
  void EndFrame(Frame &&frame);

private:
  Renderer *pRenderer = NULL;

  Queue *pGraphicsQueue = NULL;
  GpuCmdRing mGraphicsCmdRing = {};

  SwapChain *pSwapChain = NULL;
  RenderTarget *pDepthBuffer = NULL;
  Semaphore *pImageAcquiredSemaphore = NULL;

  uint8_t mFrameIndex = 0;
};