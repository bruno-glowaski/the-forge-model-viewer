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
#include "Scene.hpp"
#include "SceneRenderSystem.hpp"
#include "SkyBox.hpp"

class ModelViewer : public IApp {
public:
  bool Init() {
    if (!mRenderContext.Init(GetName())) {
      ShowUnsupportedMessage("Failed To Initialize renderer!");
      return false;
    }
    mRenderSystem.Init(mRenderContext);

    mScene.LoadMeshResource(mRenderContext, kSceneMeshPath);
    mSkyBox.LoadDefault(mRenderContext);

    FontDesc font = {};
    font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
    fntDefineFonts(&font, 1, &gFontID);

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

    mRenderSystem.Exit(mRenderContext);

    mScene.Destroy(mRenderContext);
    mSkyBox.Destroy(mRenderContext);

    mRenderContext.Exit();
  }

  bool Load(ReloadDesc *pReloadDesc) {
    if (!mRenderContext.Load(pWindow->handle, mSettings.mWidth,
                             mSettings.mHeight, mSettings.mVSyncEnabled,
                             pReloadDesc)) {
      return false;
    }
    mRenderSystem.Load(mRenderContext, mSkyBox, pReloadDesc);

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

    return true;
  }

  void Unload(ReloadDesc *pReloadDesc) {
    mRenderContext.WaitIdle();

    mRenderContext.Unload(pReloadDesc);

    mRenderSystem.Unload(mRenderContext, pReloadDesc);

    if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
      uiRemoveComponent(pSceneGui);
      uiRemoveComponent(pControlsGui);
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

    mat4 sceneMat = mat4::scale(vec3(gSceneScale));
    mat4 viewMat = pCameraController->getViewMatrix();
    const float horizontal_fov = PI / 2.0f;
    const float aspectInverse =
        (float)mSettings.mHeight / (float)mSettings.mWidth;
    CameraMatrix projMat = CameraMatrix::perspectiveReverseZ(
        horizontal_fov, aspectInverse, 0.1f, 1000.0f);
    mRenderSystem.UpdateSceneViewProj(sceneMat, viewMat, projMat);
  }

  void Draw() {
    if (mRenderContext.IsVSyncEnabled() != mSettings.mVSyncEnabled) {
      mRenderContext.WaitIdle();
      mRenderContext.ToggleVSync();
    }

    RenderContext::Frame frame = mRenderContext.BeginFrame();
    Cmd *cmd = frame.mCmdRingElement.pCmds[0];

    beginCmd(cmd);
    cmdBeginGpuFrameProfile(cmd, mGpuProfileToken);
    RenderTargetBarrier barriers[] = {
        {frame.pImage, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET},
    };
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
    ClearScreen(frame);

    cmdBeginGpuTimestampQuery(cmd, mGpuProfileToken, "Draw Canvas");
    mRenderSystem.Draw(frame, mScene, mSkyBox, mGpuProfileToken);
    cmdEndGpuTimestampQuery(cmd, mGpuProfileToken);

    cmdBindRenderTargets(cmd, NULL);

    cmdBeginGpuTimestampQuery(cmd, mGpuProfileToken, "Draw UI");
    DrawUI(frame);
    cmdEndGpuTimestampQuery(cmd, mGpuProfileToken);

    cmdBindRenderTargets(cmd, NULL);

    barriers[0] = {frame.pImage, RESOURCE_STATE_RENDER_TARGET,
                   RESOURCE_STATE_PRESENT};
    cmdResourceBarrier(cmd, 0, NULL, 0, NULL, 1, barriers);
    cmdEndGpuFrameProfile(cmd, mGpuProfileToken);
    endCmd(cmd);

    mRenderContext.EndFrame(std::move(frame));
  }

  void ClearScreen(RenderContext::Frame &frame) {
    Cmd *cmd = frame.mCmdRingElement.pCmds[0];

    BindRenderTargetsDesc bindRenderTargets = {};
    bindRenderTargets.mRenderTargetCount = 1;
    bindRenderTargets.mRenderTargets[0] = {frame.pImage, LOAD_ACTION_CLEAR};
    bindRenderTargets.mDepthStencil = {frame.pDepthBuffer, LOAD_ACTION_CLEAR};
    cmdBindRenderTargets(cmd, &bindRenderTargets);
    cmdSetViewport(cmd, 0.0f, 0.0f, (float)frame.pImage->mWidth,
                   (float)frame.pImage->mHeight, 0.0f, 1.0f);
    cmdSetScissor(cmd, 0, 0, frame.pImage->mWidth, frame.pImage->mHeight);
  }

  void DrawUI(RenderContext::Frame &frame) {
    Cmd *cmd = frame.mCmdRingElement.pCmds[0];

    BindRenderTargetsDesc bindRenderTargets = {};
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
  }

  const char *GetName() { return "ModelViewer"; }

  static void RequestShadersReload(void *) {
    ReloadDesc reload{RELOAD_TYPE_SHADER};
    requestReload(&reload);
  }

private:
  RenderContext mRenderContext;
  SceneRenderSystem mRenderSystem;

  const char *const kSceneMeshPath = "castle.bin";
  SkyBox mSkyBox;
  Scene mScene;

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

  ProfileToken mGpuProfileToken = PROFILE_INVALID_TOKEN;
};

DEFINE_APPLICATION_MAIN(ModelViewer)
