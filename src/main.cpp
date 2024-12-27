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
#include "RenderContext.hpp"
#include "SkyBox.hpp"

struct UniformBlock {
  CameraMatrix mModelProjectView;

  // Point Light Information
  vec4 mLightPosition;
  vec4 mLightColor;
};

struct UniformBlockSky {
  CameraMatrix mProjectView;
};

class ModelViewer : public IApp {
public:
  bool Init() {
    // window and renderer setup
    if (!mRenderContext.Init(GetName())) {
      ShowUnsupportedMessage("Failed To Initialize renderer!");
      return false;
    }

    // Load scene
    mSceneVertexLayout.mAttribCount = 3;
    mSceneVertexLayout.mBindingCount = 1;
    mSceneVertexLayout.mBindings[0].mStride =
        sizeof(float3) + sizeof(uint32_t) + sizeof(float);
    mSceneVertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
    mSceneVertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
    mSceneVertexLayout.mAttribs[0].mLocation = 0;
    mSceneVertexLayout.mAttribs[0].mOffset = 0;
    mSceneVertexLayout.mAttribs[0].mBinding = 0;
    mSceneVertexLayout.mAttribs[1].mSemantic = SEMANTIC_NORMAL;
    mSceneVertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32_UINT;
    mSceneVertexLayout.mAttribs[1].mLocation = 1;
    mSceneVertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);
    mSceneVertexLayout.mAttribs[1].mBinding = 0;
    mSceneVertexLayout.mAttribs[2].mSemantic = SEMANTIC_TEXCOORD0;
    mSceneVertexLayout.mAttribs[2].mFormat = TinyImageFormat_R16G16_SFLOAT;
    mSceneVertexLayout.mAttribs[2].mLocation = 2;
    mSceneVertexLayout.mAttribs[2].mOffset = sizeof(float3) + sizeof(uint32_t);
    mSceneVertexLayout.mAttribs[2].mBinding = 0;
    GeometryLoadDesc sceneGDesc = {};
    sceneGDesc.ppGeometry = &pSceneGeometry;
    sceneGDesc.ppGeometryData = &pSceneGeometryData;
    sceneGDesc.pFileName = kSceneMeshPath;
    sceneGDesc.pVertexLayout = &mSceneVertexLayout;
    addResource(&sceneGDesc, NULL);

    skyBox.LoadDefault(mRenderContext);

    BufferLoadDesc ubDesc = {};
    ubDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_TO_GPU;
    ubDesc.mDesc.mFlags = BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT;
    ubDesc.pData = NULL;
    for (uint32_t i = 0; i < RenderContext::kDataBufferCount; ++i) {
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

    // Gpu profiler can only be added after initProfile.
    mGpuProfileToken = mRenderContext.CreateGpuProfiler("Graphics");

    waitForAllResourceLoads();

    vec3 camPos{0.0f, 0.0f, 10.0f};
    vec3 lookAt{vec3(0)};

    pCameraController = initOrbitCameraController(camPos, lookAt);

    AddCustomInputBindings();

    return true;
  }

  void Exit() {
    exitCameraController(pCameraController);

    for (uint32_t i = 0; i < RenderContext::kDataBufferCount; ++i) {
      removeResource(pSceneUniformBuffer[i]);
      removeResource(pSkyboxUniformBuffer[i]);
    }

    removeResource(pSceneGeometry);
    removeResource(pSceneGeometryData);

    skyBox.Destroy(mRenderContext);

    mRenderContext.Exit();
  }

  bool Load(ReloadDesc *pReloadDesc) {
    mRenderContext.Load(pWindow->handle, mSettings.mWidth, mSettings.mHeight,
                        mSettings.mVSyncEnabled, pReloadDesc);

    if (pReloadDesc->mType & RELOAD_TYPE_SHADER) {
      addShaders();
      addRootSignatures();
      addDescriptorSets();
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
      UIComponentDesc constrolsGuiDesc{};
      constrolsGuiDesc.mStartPosition =
          vec2(mSettings.mWidth * 0.01f, mSettings.mHeight * 0.01f);
      uiAddComponent("Controls", &constrolsGuiDesc, &pControlsGui);

      static float4 color = {1.0f, 1.0f, 1.0f, 0.75f};
      DynamicTextWidget controlsManualWidget;
      controlsManualWidget.pText = &gControlsText;
      controlsManualWidget.pColor = &color;
      uiAddComponentWidget(pControlsGui, "Manual", &controlsManualWidget,
                           WIDGET_TYPE_DYNAMIC_TEXT);

      SliderFloatWidget cameraAccelerationWidget;
      cameraAccelerationWidget.mMin = 0.0f;
      cameraAccelerationWidget.mMax = 1000.0f;
      cameraAccelerationWidget.mStep = 1.0f;
      cameraAccelerationWidget.pData = &mCameraAcceleration;
      uiAddComponentWidget(pControlsGui, "Camera Acceleration",
                           &cameraAccelerationWidget, WIDGET_TYPE_SLIDER_FLOAT);

      SliderFloatWidget cameraBrakingWidget;
      cameraBrakingWidget.mMin = 0.0f;
      cameraBrakingWidget.mMax = 1000.0f;
      cameraBrakingWidget.mStep = 1.0f;
      cameraBrakingWidget.pData = &mCameraBraking;
      uiAddComponentWidget(pControlsGui, "Camera Braking", &cameraBrakingWidget,
                           WIDGET_TYPE_SLIDER_FLOAT);

      SliderFloatWidget cameraZoomSpeedWidget;
      cameraZoomSpeedWidget.mMin = 0.0f;
      cameraZoomSpeedWidget.mMax = 1000.0f;
      cameraZoomSpeedWidget.mStep = 1.0f;
      cameraZoomSpeedWidget.pData = &mCameraZoomSpeed;
      uiAddComponentWidget(pControlsGui, "Zoom Speed", &cameraZoomSpeedWidget,
                           WIDGET_TYPE_SLIDER_FLOAT);

      SliderFloatWidget cameraOrbitSpeedWidget;
      cameraOrbitSpeedWidget.mMin = 0.0f;
      cameraOrbitSpeedWidget.mMax = 1000.0f;
      cameraOrbitSpeedWidget.mStep = 1.0f;
      cameraOrbitSpeedWidget.pData = &mCameraOrbitSpeed;
      uiAddComponentWidget(pControlsGui, "Orbit Speed", &cameraOrbitSpeedWidget,
                           WIDGET_TYPE_SLIDER_FLOAT);

      UIComponentDesc sceneGuiDesc{};
      sceneGuiDesc.mStartPosition =
          vec2(mSettings.mWidth * 0.01f, mSettings.mHeight * 0.87f);
      uiAddComponent("Scene", &sceneGuiDesc, &pSceneGui);

      SliderFloatWidget sceneScaleWidget;
      sceneScaleWidget.mMin = 0.0f;
      sceneScaleWidget.mMax = 100.0f;
      sceneScaleWidget.mStep = 0.1f;
      sceneScaleWidget.pData = &gSceneScale;
      uiAddComponentWidget(pSceneGui, "Scale", &sceneScaleWidget,
                           WIDGET_TYPE_SLIDER_FLOAT);
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET)) {
      addPipelines();
    }

    prepareDescriptorSets();

    return true;
  }

  void Unload(ReloadDesc *pReloadDesc) {
    mRenderContext.WaitIdle();

    mRenderContext.Unload(pReloadDesc);

    if (pReloadDesc->mType & (RELOAD_TYPE_SHADER | RELOAD_TYPE_RENDERTARGET)) {
      removePipelines();
    }

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
      uiRemoveComponent(pSceneGui);
      uiRemoveComponent(pControlsGui);
    }

    if (pReloadDesc->mType & RELOAD_TYPE_SHADER) {
      removeDescriptorSets();
      removeRootSignatures();
      removeShaders();
    }
  }

  void Update(float deltaTime) {
    if (!uiIsFocused()) {
      CameraMotionParameters cmp{{},
                                 mCameraAcceleration,
                                 mCameraBraking,
                                 mCameraZoomSpeed,
                                 mCameraOrbitSpeed};
      pCameraController->setMotionParameters(cmp);

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
  }

  void Draw() {
    if (mRenderContext.IsVSyncEnabled() != mSettings.mVSyncEnabled) {
      mRenderContext.WaitIdle();
      mRenderContext.ToggleVSync();
    }

    mat4 viewMat = pCameraController->getViewMatrix();

    const float aspectInverse =
        (float)mSettings.mHeight / (float)mSettings.mWidth;
    const float horizontal_fov = PI / 2.0f;
    CameraMatrix projMat = CameraMatrix::perspectiveReverseZ(
        horizontal_fov, aspectInverse, 0.1f, 1000.0f);
    mat4 modelMat = mat4::scale(vec3(gSceneScale));
    mUniformData.mModelProjectView = projMat * viewMat * modelMat;

    mUniformData.mLightPosition = vec4(0, 0, 0, 0);
    mUniformData.mLightColor = vec4(0.9f, 0.9f, 0.7f, 1.0f); // Pale Yellow

    viewMat.setTranslation(vec3(0));
    mUniformDataSky = {};
    mUniformDataSky.mProjectView = projMat * viewMat;

    RenderContext::Frame frame = mRenderContext.BeginFrame();

    // Update uniform buffers
    BufferUpdateDesc viewProjCbv = {pSceneUniformBuffer[frame.index]};
    beginUpdateResource(&viewProjCbv);
    memcpy(viewProjCbv.pMappedData, &mUniformData, sizeof(mUniformData));
    endUpdateResource(&viewProjCbv);

    BufferUpdateDesc skyboxViewProjCbv = {pSkyboxUniformBuffer[frame.index]};
    beginUpdateResource(&skyboxViewProjCbv);
    memcpy(skyboxViewProjCbv.pMappedData, &mUniformDataSky,
           sizeof(mUniformDataSky));
    endUpdateResource(&skyboxViewProjCbv);

    Cmd *cmd = frame.mCmdRingElement.pCmds[0];
    cmdBeginGpuFrameProfile(cmd, mGpuProfileToken);

    RenderTargetBarrier barriers[] = {
        {frame.pImage, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdBeginGpuTimestampQuery(cmd, mGpuProfileToken, "Draw Canvas");

    // simply record the screen cleaning command
    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = {frame.pImage, LOAD_ACTION_CLEAR};
    bindRenderTargets.mDepthStencil = {frame.pDepthBuffer, LOAD_ACTION_CLEAR};
    cmdBindRenderTargets(cmd, &bindRenderTargets);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                   (float)frame.pImage->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, frame.pImage->mWidth, frame.pImage->mHeight);

    const uint32_t skyboxVbStride = sizeof(float) * 4;
    // draw skybox
    cmdBeginGpuTimestampQuery(cmd, mGpuProfileToken, "Draw Skybox");
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                   (float)frame.pImage->mHeight, 1.0f, 1.0f);
    cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, frame.index * 2 + 0, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1,
                        const_cast<Buffer **>(&skyBox.GetVertexBuffer()),
                        &skyboxVbStride, NULL);
    cmdDraw(cmd, 36, 0);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                   (float)frame.pImage->mHeight, 0.0f, 1.0f);
    cmdEndGpuTimestampQuery(cmd, mGpuProfileToken);

    cmdBeginGpuTimestampQuery(cmd, mGpuProfileToken, "Draw Scene");
    cmdBindPipeline(cmd, pScenePipeline);
    cmdBindDescriptorSet(cmd, frame.index * 2 + 1, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, pSceneGeometry->mVertexBufferCount,
                        pSceneGeometry->pVertexBuffers,
                        &mSceneVertexLayout.mBindings[0].mStride, nullptr);
    cmdBindIndexBuffer(cmd, pSceneGeometry->pIndexBuffer, INDEX_TYPE_UINT16, 0);
    cmdDrawIndexed(cmd, pSceneGeometry->mIndexCount, 0, 0);
    cmdEndGpuTimestampQuery(cmd, mGpuProfileToken);

    cmdEndGpuTimestampQuery(cmd, mGpuProfileToken); // Draw Canvas
    cmdBindRenderTargets(cmd, NULL);

    cmdBeginGpuTimestampQuery(cmd, mGpuProfileToken, "Draw UI");

    bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = {frame.pImage, LOAD_ACTION_LOAD};
    bindRenderTargets.mDepthStencil = {NULL, LOAD_ACTION_DONTCARE};
    cmdBindRenderTargets(cmd, &bindRenderTargets);

    gFrameTimeDraw.mFontColor = 0xff00ffff;
    gFrameTimeDraw.mFontSize = 18.0f;
    gFrameTimeDraw.mFontID = gFontID;
    float2 txtSizePx =
        cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
    cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), mGpuProfileToken,
                      &gFrameTimeDraw);

    cmdDrawUserInterface(cmd);

    cmdEndGpuTimestampQuery(cmd, mGpuProfileToken);
    cmdBindRenderTargets(cmd, NULL);

    barriers[0] = {frame.pImage, RESOURCE_STATE_RENDER_TARGET,
                   RESOURCE_STATE_PRESENT};
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdEndGpuFrameProfile(cmd, mGpuProfileToken);

    mRenderContext.EndFrame(std::move(frame));
  }

  const char *GetName() { return "ModelViewer"; }

  static void RequestShadersReload(void *) {
    ReloadDesc reload{RELOAD_TYPE_SHADER};
    requestReload(&reload);
  }

private:
  RenderContext mRenderContext;

  void addDescriptorSets() {
    DescriptorSetDesc desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_NONE, 1};
    pDescriptorSetTexture = mRenderContext.CreateDescriptorSet(&desc);
    desc = {pRootSignature, DESCRIPTOR_UPDATE_FREQ_PER_FRAME,
            RenderContext::kDataBufferCount * 2};
    pDescriptorSetUniforms = mRenderContext.CreateDescriptorSet(&desc);
  }

  void removeDescriptorSets() {
    mRenderContext.DestroyDescriptorSet(pDescriptorSetTexture);
    mRenderContext.DestroyDescriptorSet(pDescriptorSetUniforms);
  }

  void addRootSignatures() {
    Shader *shaders[2];
    uint32_t shadersCount = 0;
    shaders[shadersCount++] = pSceneShader;
    shaders[shadersCount++] = pSkyBoxDrawShader;

    RootSignatureDesc rootDesc = {};
    rootDesc.mShaderCount = shadersCount;
    rootDesc.ppShaders = shaders;
    pRootSignature = mRenderContext.CreateRootSignature(&rootDesc);
  }

  void removeRootSignatures() {
    mRenderContext.DestroyRootSignature(pRootSignature);
  }

  void addShaders() {
    ShaderLoadDesc skyShader = {};
    skyShader.mVert.pFileName = "skybox.vert";
    skyShader.mFrag.pFileName = "skybox.frag";

    ShaderLoadDesc basicShader = {};
    basicShader.mVert.pFileName = "basic.vert";
    basicShader.mFrag.pFileName = "basic.frag";

    pSkyBoxDrawShader = mRenderContext.LoadShader(&skyShader);
    pSceneShader = mRenderContext.LoadShader(&basicShader);
  }

  void removeShaders() {
    mRenderContext.DestroyShader(pSceneShader);
    mRenderContext.DestroyShader(pSkyBoxDrawShader);
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

    TinyImageFormat swapChainFormat = mRenderContext.GetSwapChainFormat();
    PipelineDesc desc = {};
    desc.mType = PIPELINE_TYPE_GRAPHICS;
    GraphicsPipelineDesc &pipelineSettings = desc.mGraphicsDesc;
    pipelineSettings.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
    pipelineSettings.mRenderTargetCount = 1;
    pipelineSettings.pDepthState = &depthStateDesc;
    pipelineSettings.pColorFormats = &swapChainFormat;
    pipelineSettings.mSampleCount = mRenderContext.GetSwapChainSampleCount();
    pipelineSettings.mSampleQuality =
        mRenderContext.GetSwapChainSampleQuality();
    pipelineSettings.mDepthStencilFormat = mRenderContext.GetDepthFormat();
    pipelineSettings.pRootSignature = pRootSignature;
    pipelineSettings.pShaderProgram = pSceneShader;
    pipelineSettings.pVertexLayout = &mSceneVertexLayout;
    pipelineSettings.pRasterizerState = &sceneRasterizerStateDesc;
    pipelineSettings.mVRFoveatedRendering = true;
    pScenePipeline = mRenderContext.CreatePipeline(&desc);

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
    pSkyBoxDrawPipeline = mRenderContext.CreatePipeline(&desc);
  }

  void removePipelines() {
    mRenderContext.DestroyPipeline(pSkyBoxDrawPipeline);
    mRenderContext.DestroyPipeline(pScenePipeline);
  }

  void prepareDescriptorSets() {
    // Prepare descriptor sets
    DescriptorData params[7] = {};
    params[0].pName = "RightText";
    params[0].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(0));
    params[1].pName = "LeftText";
    params[1].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(1));
    params[2].pName = "TopText";
    params[2].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(2));
    params[3].pName = "BotText";
    params[3].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(3));
    params[4].pName = "FrontText";
    params[4].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(4));
    params[5].pName = "BackText";
    params[5].ppTextures = const_cast<Texture **>(&skyBox.GetTexture(5));
    params[6].pName = "uSampler0";
    params[6].ppSamplers = const_cast<Sampler **>(&skyBox.GetSampler());
    mRenderContext.UpdateDescriptorSet(pDescriptorSetTexture, 0, 7, params);

    for (uint32_t i = 0; i < RenderContext::kDataBufferCount; ++i) {
      DescriptorData uParams[1] = {};
      uParams[0].pName = "uniformBlock";
      uParams[0].ppBuffers = &pSkyboxUniformBuffer[i];
      mRenderContext.UpdateDescriptorSet(pDescriptorSetUniforms, i * 2 + 0, 1,
                                         uParams);

      uParams[0].pName = "uniformBlock";
      uParams[0].ppBuffers = &pSceneUniformBuffer[i];
      mRenderContext.UpdateDescriptorSet(pDescriptorSetUniforms, i * 2 + 1, 1,
                                         uParams);
    }
  }

  const char *const kSceneMeshPath = "castle.bin";
  Geometry *pSceneGeometry;
  GeometryData *pSceneGeometryData;
  Shader *pSceneShader = NULL;
  Pipeline *pScenePipeline = NULL;
  VertexLayout mSceneVertexLayout = {};
  uint32_t mSceneLayoutType = 0;

  SkyBox skyBox;
  Shader *pSkyBoxDrawShader = NULL;
  Pipeline *pSkyBoxDrawPipeline = NULL;
  RootSignature *pRootSignature = NULL;
  DescriptorSet *pDescriptorSetTexture = {NULL};
  DescriptorSet *pDescriptorSetUniforms = {NULL};

  Buffer *pSceneUniformBuffer[RenderContext::kDataBufferCount] = {NULL};
  Buffer *pSkyboxUniformBuffer[RenderContext::kDataBufferCount] = {NULL};

  ProfileToken mGpuProfileToken = PROFILE_INVALID_TOKEN;

  UniformBlock mUniformData;
  UniformBlockSky mUniformDataSky;

  ICameraController *pCameraController = NULL;

  uint32_t gFontID = 0;

  FontDrawDesc gFrameTimeDraw;

  UIComponent *pControlsGui = NULL;
  const char *const kControlsTextCharArray = "Manual:\n"
                                             "W: Zoom in\n"
                                             "S: Zoom out\n"
                                             "A: Orbit right\n"
                                             "D: Orbit left\n"
                                             "Q: Orbit down\n"
                                             "E: Orbit up\n"
                                             "Mouse drag: Orbit around\n";
  bstring gControlsText = bfromarr(kControlsTextCharArray);

  float mCameraAcceleration = 600.0f;
  float mCameraBraking = 200.0f;
  float mCameraZoomSpeed = 1.0f;
  float mCameraOrbitSpeed = 1.0f;

  UIComponent *pSceneGui = NULL;
  float gSceneScale = 1.0f;
};

DEFINE_APPLICATION_MAIN(ModelViewer)
