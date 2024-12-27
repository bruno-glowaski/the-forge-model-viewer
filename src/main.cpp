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

struct UniformBlock {
  CameraMatrix mModelProjectView;

  // Point Light Information
  vec4 mLightPosition;
  vec4 mLightColor;
};

struct UniformBlockSky {
  CameraMatrix mProjectView;
};

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

Buffer *pSceneUniformBuffer[RenderContext::kDataBufferCount] = {NULL};
Buffer *pSkyboxUniformBuffer[RenderContext::kDataBufferCount] = {NULL};

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

static float gCameraAcceleration = 600.0f;
float gCameraBraking = 200.0f;
float gCameraZoomSpeed = 1.0f;
float gCameraOrbitSpeed = 1.0f;

UIComponent *pSceneGui = NULL;
float gSceneScale = 1.0f;

class ModelViewer : public IApp {
public:
  bool Init() {
    // window and renderer setup
    if (!mRenderContext.Init(GetName())) {
      ShowUnsupportedMessage("Failed To Initialize renderer!");
      return false;
    }

    // Load scene
    gSceneVertexLayout.mAttribCount = 3;
    gSceneVertexLayout.mBindingCount = 1;
    gSceneVertexLayout.mBindings[0].mStride =
        sizeof(float3) + sizeof(uint32_t) + sizeof(float);
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
    pSamplerSkyBox = mRenderContext.CreateSampler(&samplerDesc);

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
    gGpuProfileToken = mRenderContext.CreateGpuProfiler("Graphics");

    waitForAllResourceLoads();

    vec3 camPos{0.0f, 0.0f, 10.0f};
    vec3 lookAt{vec3(0)};

    pCameraController = initOrbitCameraController(camPos, lookAt);

    AddCustomInputBindings();
    gFrameIndex = 0;

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

    removeResource(pSkyBoxVertexBuffer);

    mRenderContext.DestroySampler(pSamplerSkyBox);

    for (uint i = 0; i < 6; ++i)
      removeResource(pSkyBoxTextures[i]);

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
      cameraAccelerationWidget.pData = &gCameraAcceleration;
      uiAddComponentWidget(pControlsGui, "Camera Acceleration",
                           &cameraAccelerationWidget, WIDGET_TYPE_SLIDER_FLOAT);

      SliderFloatWidget cameraBrakingWidget;
      cameraBrakingWidget.mMin = 0.0f;
      cameraBrakingWidget.mMax = 1000.0f;
      cameraBrakingWidget.mStep = 1.0f;
      cameraBrakingWidget.pData = &gCameraBraking;
      uiAddComponentWidget(pControlsGui, "Camera Braking", &cameraBrakingWidget,
                           WIDGET_TYPE_SLIDER_FLOAT);

      SliderFloatWidget cameraZoomSpeedWidget;
      cameraZoomSpeedWidget.mMin = 0.0f;
      cameraZoomSpeedWidget.mMax = 1000.0f;
      cameraZoomSpeedWidget.mStep = 1.0f;
      cameraZoomSpeedWidget.pData = &gCameraZoomSpeed;
      uiAddComponentWidget(pControlsGui, "Zoom Speed", &cameraZoomSpeedWidget,
                           WIDGET_TYPE_SLIDER_FLOAT);

      SliderFloatWidget cameraOrbitSpeedWidget;
      cameraOrbitSpeedWidget.mMin = 0.0f;
      cameraOrbitSpeedWidget.mMax = 1000.0f;
      cameraOrbitSpeedWidget.mStep = 1.0f;
      cameraOrbitSpeedWidget.pData = &gCameraOrbitSpeed;
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
                                 gCameraAcceleration,
                                 gCameraBraking,
                                 gCameraZoomSpeed,
                                 gCameraOrbitSpeed};
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
    mat4 modelMat = mat4::scale(vec3(gSceneScale));
    gUniformData.mModelProjectView = projMat * viewMat * modelMat;

    // point light parameters
    gUniformData.mLightPosition = vec4(0, 0, 0, 0);
    gUniformData.mLightColor = vec4(0.9f, 0.9f, 0.7f, 1.0f); // Pale Yellow

    viewMat.setTranslation(vec3(0));
    gUniformDataSky = {};
    gUniformDataSky.mProjectView = projMat * viewMat;
  }

  void Draw() {
    if (mRenderContext.IsVSyncEnabled() != mSettings.mVSyncEnabled) {
      mRenderContext.WaitIdle();
      mRenderContext.ToggleVSync();
    }

    RenderContext::Frame frame = mRenderContext.BeginFrame();

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

    Cmd *cmd = frame.mCmdRingElement.pCmds[0];
    cmdBeginGpuFrameProfile(cmd, gGpuProfileToken);

    RenderTargetBarrier barriers[] = {
        {frame.pImage, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Canvas");

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
    cmdBeginGpuTimestampQuery(cmd, gGpuProfileToken, "Draw Skybox");
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                   (float)frame.pImage->mHeight, 1.0f, 1.0f);
    cmdBindPipeline(cmd, pSkyBoxDrawPipeline);
    cmdBindDescriptorSet(cmd, 0, pDescriptorSetTexture);
    cmdBindDescriptorSet(cmd, gFrameIndex * 2 + 0, pDescriptorSetUniforms);
    cmdBindVertexBuffer(cmd, 1, &pSkyBoxVertexBuffer, &skyboxVbStride, NULL);
    cmdDraw(cmd, 36, 0);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                   (float)frame.pImage->mHeight, 0.0f, 1.0f);
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
    bindRenderTargets.mRenderTargets[0] = {frame.pImage, LOAD_ACTION_LOAD};
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

    barriers[0] = {frame.pImage, RESOURCE_STATE_RENDER_TARGET,
                   RESOURCE_STATE_PRESENT};
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);

    cmdEndGpuFrameProfile(cmd, gGpuProfileToken);

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
    pipelineSettings.pVertexLayout = &gSceneVertexLayout;
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
};

DEFINE_APPLICATION_MAIN(ModelViewer)
