#pragma once

#include "Application/Interfaces/ICameraController.h"
#include "Application/Interfaces/IProfiler.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "OS/Interfaces/IOperatingSystem.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "Utilities/Math/MathTypes.h"
#include "Utilities/RingBuffer.h"

#include "RenderContext.hpp"
#include "Scene.hpp"
#include "SkyBox.hpp"

static const VertexLayout kSceneVertexLayout = {
    {
        {sizeof(float3) + sizeof(uint32_t) + sizeof(float),
         VERTEX_BINDING_RATE_VERTEX},
    },
    {
        {SEMANTIC_POSITION, 0, "vPosition", TinyImageFormat_R32G32B32_SFLOAT, 0,
         0, 0},
        {SEMANTIC_NORMAL, 0, "vNormal", TinyImageFormat_R32_UINT, 0, 1,
         sizeof(float3)},
        {SEMANTIC_TEXCOORD0, 0, "vUV", TinyImageFormat_R16G16_SFLOAT, 0, 2,
         sizeof(float3) + sizeof(uint32_t)},
    },
    1,
    3,
};

class SceneRenderSystem {
public:
  void Init(RenderContext &renderContext);
  void Exit(RenderContext &renderContext);

  void Load(RenderContext &renderContext, const SkyBox &skyBox,
            ReloadDesc *pReloadDesc);
  void Unload(RenderContext &renderContext, ReloadDesc *pReloadDesc);

  void UpdateSceneViewProj(mat4 sceneMat, mat4 viewMat, CameraMatrix projMat);

  void Draw(RenderContext::Frame &frame, const Scene &scene,
            const SkyBox &skyBox, ProfileToken gpuProfileToken);

private:
  RootSignature *pRootSignature = NULL;

  Shader *pSceneShader = NULL;
  Pipeline *pScenePipeline = NULL;

  Shader *pSkyBoxDrawShader = NULL;
  Pipeline *pSkyBoxDrawPipeline = NULL;

  DescriptorSet *pDescriptorSetTexture = {NULL};
  DescriptorSet *pDescriptorSetUniforms = {NULL};

  struct SceneUniformBlock {
    CameraMatrix mModelProjectView;

    vec4 mLightPosition;
    vec4 mLightColor;
  };

  struct SkyBoxUniformBlock {
    CameraMatrix mProjectView;
  };

  SceneUniformBlock mSceneUniformData;
  SkyBoxUniformBlock mSkyBoxUniformData;
  Buffer *pSceneUniformBuffer[RenderContext::kDataBufferCount] = {NULL};
  Buffer *pSkyboxUniformBuffer[RenderContext::kDataBufferCount] = {NULL};

  void UpdateUniformBuffers(RenderContext::Frame &frame);
  void DrawSkyBox(RenderContext::Frame &frame, const SkyBox &skyBox);
  void DrawScene(RenderContext::Frame &frame, const Scene &scene);

  void AddDescriptorSets(RenderContext &renderContext);
  void RemoveDescriptorSets(RenderContext &renderContext);
  void AddRootSignatures(RenderContext &renderContext);
  void RemoveRootSignatures(RenderContext &renderContext);
  void AddShaders(RenderContext &renderContext);
  void RemoveShaders(RenderContext &renderContext);
  void AddPipelines(RenderContext &renderContext);
  void RemovePipelines(RenderContext &renderContext);

  void PrepareDescriptorSets(RenderContext &renderContext,
                             const SkyBox &skyBox);
};