#include "SceneRenderSystem.hpp"

void SceneRenderSystem::Init(RenderContext &renderContext) {
  BufferLoadDesc ubDesc = {};
  ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
  ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
  ubDesc.pData = NULL;
  for (uint32_t i = 0; i < RenderContext::kDataBufferCount; ++i) {
    ubDesc.mDesc.pName = "SceneUniformBuffer";
    ubDesc.mDesc.mSize = sizeof(SceneUniformBlock);
    ubDesc.ppBuffer = &pSceneUniformBuffer[i];
    addResource(&ubDesc, NULL);
    ubDesc.mDesc.pName = "SkyBoxUniformBuffer";
    ubDesc.mDesc.mSize = sizeof(SkyBoxUniformBlock);
    ubDesc.ppBuffer = &pSkyboxUniformBuffer[i];
    addResource(&ubDesc, NULL);
  }
}
void SceneRenderSystem::Exit(RenderContext &renderContext) {
  for (uint32_t i = 0; i < RenderContext::kDataBufferCount; ++i) {
    removeResource(pSceneUniformBuffer[i]);
    removeResource(pSkyboxUniformBuffer[i]);
  }
}

void SceneRenderSystem::Load(RenderContext &renderContext, const SkyBox &skyBox,
                             ReloadDesc *pReloadDesc) {
  if (pReloadDesc->mType & RELOAD_TYPE_SHADER) {
    AddShaders(renderContext);
    AddRootSignatures(renderContext);
    AddDescriptorSets(renderContext);
  }

  if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET)) {
    AddPipelines(renderContext);
  }

  PrepareDescriptorSets(renderContext, skyBox);
}
void SceneRenderSystem::Unload(RenderContext &renderContext,
                               ReloadDesc *pReloadDesc) {
  if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET)) {
    RemovePipelines(renderContext);
  }
  if (pReloadDesc->mType & RELOAD_TYPE_SHADER) {
    RemoveDescriptorSets(renderContext);
    RemoveRootSignatures(renderContext);
    RemoveShaders(renderContext);
  }
}

void SceneRenderSystem::UpdateSceneViewProj(mat4 sceneMat, mat4 viewMat,
                                            CameraMatrix projMat) {
  mSceneUniformData.mModelProjectView = projMat * viewMat * sceneMat;
  mSceneUniformData.mLightPosition = vec4(0, 0, 0, 0);
  mSceneUniformData.mLightColor = vec4(0.9f, 0.9f, 0.7f, 1.0f); // Pale Yellow

  viewMat.setTranslation(vec3(0));
  mSkyBoxUniformData = {};
  mSkyBoxUniformData.mProjectView = projMat * viewMat;
}

void SceneRenderSystem::Draw(RenderContext::Frame &frame, const Scene &scene,
                             const SkyBox &skyBox,
                             ProfileToken gpuProfileToken) {
  Cmd *cmd = frame.mCmdRingElement.pCmds[0];

  UpdateUniformBuffers(frame);

  cmdBeginGpuTimestampQuery(cmd, gpuProfileToken, "Draw Skybox");
  DrawSkyBox(frame, skyBox);
  cmdEndGpuTimestampQuery(cmd, gpuProfileToken);

  cmdBeginGpuTimestampQuery(cmd, gpuProfileToken, "Draw Scene");
  DrawScene(frame, scene);
  cmdEndGpuTimestampQuery(cmd, gpuProfileToken);
}

void SceneRenderSystem::UpdateUniformBuffers(RenderContext::Frame &frame) {
  BufferUpdateDesc viewProjCbv = {pSceneUniformBuffer[frame.index]};
  beginUpdateResource(&viewProjCbv);
  memcpy(viewProjCbv.pMappedData, &mSceneUniformData,
         sizeof(mSceneUniformData));
  endUpdateResource(&viewProjCbv);

  BufferUpdateDesc skyboxViewProjCbv = {pSkyboxUniformBuffer[frame.index]};
  beginUpdateResource(&skyboxViewProjCbv);
  memcpy(skyboxViewProjCbv.pMappedData, &mSkyBoxUniformData,
         sizeof(mSkyBoxUniformData));
  endUpdateResource(&skyboxViewProjCbv);
}

void SceneRenderSystem::DrawScene(RenderContext::Frame &frame,
                                  const Scene &scene) {
  Cmd *cmd = frame.mCmdRingElement.pCmds[0];
  cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                 (float)frame.pImage->mHeight, 0.0f, 1.0f);
  cmdBindPipeline(cmd, pScenePipeline);
  cmdBindDescriptorSet(cmd, frame.index * 2 + 1, pDescriptorSetUniforms);
  cmdBindVertexBuffer(cmd, scene.GetVertexBufferCount(),
                      scene.GetVertexBuffers(),
                      &kSceneVertexLayout.mBindings[0].mStride, nullptr);
  cmdBindIndexBuffer(cmd, scene.GetIndexBuffer(), INDEX_TYPE_UINT16, 0);
  cmdDrawIndexed(cmd, scene.GetIndexCount(), 0, 0);
}

void SceneRenderSystem::DrawSkyBox(RenderContext::Frame &frame,
                                   const SkyBox &skyBox) {
  Cmd *cmd = frame.mCmdRingElement.pCmds[0];
  const uint32_t skyboxVbStride = sizeof(float) * 4;

  cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                 (float)frame.pImage->mHeight, 1.0f, 1.0f);
  cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
  cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
  cmdBindDescriptorSet(cmd, frame.index * 2 + 0, pDescriptorSetUniforms);
  cmdBindVertexBuffer(cmd, 1, const_cast<Buffer **>(&skyBox.GetVertexBuffer()),
                      &skyboxVbStride, NULL);
  cmdDraw(cmd, 36, 0);
}

void SceneRenderSystem::AddDescriptorSets(RenderContext &renderContext) {
  DescriptorSetDesc desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1};
  pDescriptorSetTexture = renderContext.CreateDescriptorSet(&desc);
  desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME,
          RenderContext::kDataBufferCount * 2};
  pDescriptorSetUniforms = renderContext.CreateDescriptorSet(&desc);
}

void SceneRenderSystem::RemoveDescriptorSets(RenderContext &renderContext) {
  renderContext.DestroyDescriptorSet(pDescriptorSetTexture);
  renderContext.DestroyDescriptorSet(pDescriptorSetUniforms);
}

void SceneRenderSystem::AddRootSignatures(RenderContext &renderContext) {
  Shader *shaders[2];
  uint32_t shadersCount = 0;
  shaders[shadersCount++] = pSceneShader;
  shaders[shadersCount++] = pSkyBoxDrawShader;

  RootSignatureDesc rootDesc = {};
  rootDesc.mShaderCount = shadersCount;
  rootDesc.ppShaders = shaders;
  pRootSignature = renderContext.CreateRootSignature(&rootDesc);
}

void SceneRenderSystem::RemoveRootSignatures(RenderContext &renderContext) {
  renderContext.DestroyRootSignature(pRootSignature);
}

void SceneRenderSystem::AddShaders(RenderContext &renderContext) {
  ShaderLoadDesc skyShader = {};
  skyShader.mVert.pFileName = "skybox.vert";
  skyShader.mFrag.pFileName = "skybox.frag";

  ShaderLoadDesc basicShader = {};
  basicShader.mVert.pFileName = "basic.vert";
  basicShader.mFrag.pFileName = "basic.frag";

  pSkyBoxDrawShader = renderContext.LoadShader(&skyShader);
  pSceneShader = renderContext.LoadShader(&basicShader);
}

void SceneRenderSystem::RemoveShaders(RenderContext &renderContext) {
  renderContext.DestroyShader(pSceneShader);
  renderContext.DestroyShader(pSkyBoxDrawShader);
}

void SceneRenderSystem::AddPipelines(RenderContext &renderContext) {
  RasterizerStateDesc rasterizerStateDesc = {};
  rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

  RasterizerStateDesc sceneRasterizerStateDesc = {};
  sceneRasterizerStateDesc.mCullMode = CULL_MODE_NONE;

  DepthStateDesc depthStateDesc = {};
  depthStateDesc.mDepthTest = true;
  depthStateDesc.mDepthWrite = true;
  depthStateDesc.mDepthFunc = CMP_GEQUAL;

  TinyImageFormat swapChainFormat = renderContext.GetSwapChainFormat();
  PipelineDesc desc = {};
  desc.mType = PIPELINE_TYPE_GRAPHICS;
  GraphicsPipelineDesc &pipelineSettings = desc.mGraphicsDesc;
  pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
  pipelineSettings.mRenderTargetCount = 1;
  pipelineSettings.pDepthState = &depthStateDesc;
  pipelineSettings.pColorFormats = &swapChainFormat;
  pipelineSettings.mSampleCount = renderContext.GetSwapChainSampleCount();
  pipelineSettings.mSampleQuality = renderContext.GetSwapChainSampleQuality();
  pipelineSettings.mDepthStencilFormat = renderContext.GetDepthFormat();
  pipelineSettings.pRootSignature = pRootSignature;
  pipelineSettings.pShaderProgram = pSceneShader;
  pipelineSettings.pVertexLayout =
      const_cast<VertexLayout *>(&kSceneVertexLayout);
  pipelineSettings.pRasterizerState = &sceneRasterizerStateDesc;
  pipelineSettings.mVRFoveatedRendering = true;
  pScenePipeline = renderContext.CreatePipeline(&desc);

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
  pSkyBoxDrawPipeline = renderContext.CreatePipeline(&desc);
}

void SceneRenderSystem::RemovePipelines(RenderContext &renderContext) {
  renderContext.DestroyPipeline(pSkyBoxDrawPipeline);
  renderContext.DestroyPipeline(pScenePipeline);
}

void SceneRenderSystem::PrepareDescriptorSets(RenderContext &renderContext,
                                              const SkyBox &skyBox) {
  DescriptorData params[7] = {};

  params[0].pName = "RightText";
  params[1].pName = "LeftText";
  params[2].pName = "TopText";
  params[3].pName = "BotText";
  params[4].pName = "FrontText";
  params[5].pName = "BackText";
  for (auto i = 0; i < SkyBox::kSideCount; i++) {
    params[i].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(i));
  }

  params[6].pName = "uSampler0";
  params[6].ppSamplers = const_cast<Sampler **>(&skyBox.GetSampler());
  renderContext.UpdateDescriptorSet(pDescriptorSetTexture, 0, 7, params);

  for (uint32_t i = 0; i < RenderContext::kDataBufferCount; ++i) {
    DescriptorData uParams[1] = {};
    uParams[0].pName = "uniformBlock";
    uParams[0].ppBuffers = &pSkyboxUniformBuffer[i];
    renderContext.UpdateDescriptorSet(pDescriptorSetUniforms, i * 2 + 0, 1,
                                      uParams);

    uParams[0].pName = "uniformBlock";
    uParams[0].ppBuffers = &pSceneUniformBuffer[i];
    renderContext.UpdateDescriptorSet(pDescriptorSetUniforms, i * 2 + 1, 1,
                                      uParams);
  }
}