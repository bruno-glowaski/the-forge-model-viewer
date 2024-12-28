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
#include "Utilities/Math/MathTypes.h"

// Systems
#include "GuiSystem.hpp"
#include "OrbitCameraController.hpp"
#include "RenderContext.hpp"
#include "SceneRenderSystem.hpp"

// Resources
#include "Scene.hpp"
#include "SkyBox.hpp"

class ModelViewer : public IApp {
public:
  bool Init() {
    if (!mRenderContext.Init(GetName())) {
      ShowUnsupportedMessage("Failed To Initialize renderer!");
      return false;
    }
    mRenderSystem.Init(mRenderContext);
    mGuiSystem.Init();

    mGpuProfileToken = mRenderContext.CreateGpuProfiler("Graphics");

    waitForAllResourceLoads();

    vec3 camPos{0.0f, 0.0f, 10.0f};
    vec3 lookAt{vec3(0)};
    pCameraController = initOrbitCameraController(camPos, lookAt);

    AddCustomInputBindings();

    mScene.LoadRawFBX(mRenderContext, "castle.fbx");
    mSkyBox.LoadDefault(mRenderContext);
    waitForAllResourceLoads();

    return true;
  }

  void Exit() {
    exitCameraController(pCameraController);

    mScene.Destroy(mRenderContext);
    mSkyBox.Destroy(mRenderContext);

    mGuiSystem.Exit();
    mRenderSystem.Exit(mRenderContext);

    mRenderContext.Exit();
  }

  bool Load(ReloadDesc *pReloadDesc) {
    if (!mRenderContext.Load(pWindow->handle, mSettings.mWidth,
                             mSettings.mHeight, mSettings.mVSyncEnabled,
                             pReloadDesc)) {
      return false;
    }
    mRenderSystem.Load(mRenderContext, mSkyBox, pReloadDesc);
    mGuiSystem.Load({&mSceneScale, &mCameraAcceleration, &mCameraBraking,
                     &mCameraZoomSpeed, &mCameraOrbitSpeed},
                    mSettings.mWidth, mSettings.mHeight, pReloadDesc);

    return true;
  }

  void Unload(ReloadDesc *pReloadDesc) {
    mRenderContext.WaitIdle();
    mRenderContext.Unload(pReloadDesc);
    mRenderSystem.Unload(mRenderContext, pReloadDesc);
    mGuiSystem.Unload(pReloadDesc);
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

    mat4 sceneMat = mat4::scale(vec3(mSceneScale));
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
    mGuiSystem.Draw(frame, mGpuProfileToken);
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

  const char *GetName() { return "ModelViewer"; }

  static void RequestShadersReload(void *) {
    ReloadDesc reload{RELOAD_TYPE_SHADER};
    requestReload(&reload);
  }

private:
  RenderContext mRenderContext;
  SceneRenderSystem mRenderSystem;
  GuiSystem mGuiSystem;

  SkyBox mSkyBox;

  float mSceneScale = 1.0f;
  Scene mScene;

  float mCameraAcceleration = 600.0f;
  float mCameraBraking = 200.0f;
  float mCameraZoomSpeed = 1.0f;
  float mCameraOrbitSpeed = 1.0f;
  ICameraController *pCameraController = NULL;

  ProfileToken mGpuProfileToken = PROFILE_INVALID_TOKEN;
};

DEFINE_APPLICATION_MAIN(ModelViewer)
