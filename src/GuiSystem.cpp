#include "GuiSystem.hpp"

void GuiSystem::Init() {
  FontDesc font = {};
  font.pFontPath = "TitilliumText/TitilliumText-Bold.otf";
  fntDefineFonts(&font, 1, &gFontID);
}
void GuiSystem::Exit() {}

void GuiSystem::Load(GuiModelView modelView, int32_t appWidth,
                     int32_t appHeight, ReloadDesc *pReloadDesc) {
  if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
    UIComponentDesc constrolsGuiDesc{};
    constrolsGuiDesc.mStartPosition = vec2(appWidth * 0.01f, appHeight * 0.01f);
    uiAddComponent("Controls", &constrolsGuiDesc, &pControlsWindow);

    static float4 color = {1.0f, 1.0f, 1.0f, 0.75f};
    DynamicTextWidget controlsManualWidget;
    controlsManualWidget.pText = &gControlsText;
    controlsManualWidget.pColor = &color;
    uiAddComponentWidget(pControlsWindow, "Manual", &controlsManualWidget,
                         WIDGET_TYPE_DYNAMIC_TEXT);

    SliderFloatWidget cameraAccelerationWidget;
    cameraAccelerationWidget.mMin = 0.0f;
    cameraAccelerationWidget.mMax = 1000.0f;
    cameraAccelerationWidget.mStep = 1.0f;
    cameraAccelerationWidget.pData = modelView.pCameraAcceleration;
    uiAddComponentWidget(pControlsWindow, "Camera Acceleration",
                         &cameraAccelerationWidget, WIDGET_TYPE_SLIDER_FLOAT);

    SliderFloatWidget cameraBrakingWidget;
    cameraBrakingWidget.mMin = 0.0f;
    cameraBrakingWidget.mMax = 1000.0f;
    cameraBrakingWidget.mStep = 1.0f;
    cameraBrakingWidget.pData = modelView.pCameraBraking;
    uiAddComponentWidget(pControlsWindow, "Camera Braking",
                         &cameraBrakingWidget, WIDGET_TYPE_SLIDER_FLOAT);

    SliderFloatWidget cameraZoomSpeedWidget;
    cameraZoomSpeedWidget.mMin = 0.0f;
    cameraZoomSpeedWidget.mMax = 1000.0f;
    cameraZoomSpeedWidget.mStep = 1.0f;
    cameraZoomSpeedWidget.pData = modelView.pCameraZoomSpeed;
    uiAddComponentWidget(pControlsWindow, "Zoom Speed", &cameraZoomSpeedWidget,
                         WIDGET_TYPE_SLIDER_FLOAT);

    SliderFloatWidget cameraOrbitSpeedWidget;
    cameraOrbitSpeedWidget.mMin = 0.0f;
    cameraOrbitSpeedWidget.mMax = 1000.0f;
    cameraOrbitSpeedWidget.mStep = 1.0f;
    cameraOrbitSpeedWidget.pData = modelView.pCameraOrbitSpeed;
    uiAddComponentWidget(pControlsWindow, "Orbit Speed",
                         &cameraOrbitSpeedWidget, WIDGET_TYPE_SLIDER_FLOAT);

    UIComponentDesc sceneGuiDesc{};
    sceneGuiDesc.mStartPosition = vec2(appWidth * 0.01f, appHeight * 0.87f);
    uiAddComponent("Scene", &sceneGuiDesc, &pSceneOptionsWindow);

    SliderFloatWidget sceneScaleWidget;
    sceneScaleWidget.mMin = 0.0f;
    sceneScaleWidget.mMax = 100.0f;
    sceneScaleWidget.mStep = 0.1f;
    sceneScaleWidget.pData = modelView.pSceneScale;
    uiAddComponentWidget(pSceneOptionsWindow, "Scale", &sceneScaleWidget,
                         WIDGET_TYPE_SLIDER_FLOAT);
  }
}
void GuiSystem::Unload(ReloadDesc *pReloadDesc) {
  if (pReloadDesc->mType & (RELOAD_TYPE_RESIZE | RELOAD_TYPE_RENDERTARGET)) {
    uiRemoveComponent(pSceneOptionsWindow);
    uiRemoveComponent(pControlsWindow);
  }
}

void GuiSystem::Draw(RenderContext::Frame frame, ProfileToken gpuProfileToken) {
  Cmd *cmd = frame.mCmdRingElement.pCmds[0];

  BindRenderTargetsDesc bindRenderTargets = {};
  bindRenderTargets.mRenderTargetCount = 1;
  bindRenderTargets.mRenderTargets[0] = {frame.pImage, LOAD_ACTION_LOAD};
  bindRenderTargets.mDepthStencil = {NULL, LOAD_ACTION_DONTCARE};
  cmdBindRenderTargets(cmd, &bindRenderTargets);

  gFrameTimeDraw.mFontColor = 0xff00ffff;
  gFrameTimeDraw.mFontSize = 18.0f;
  gFrameTimeDraw.mFontID = gFontID;
  float2 txtSizePx = cmdDrawCpuProfile(cmd, float2(8.f, 15.f), &gFrameTimeDraw);
  cmdDrawGpuProfile(cmd, float2(8.f, txtSizePx.y + 75.f), gpuProfileToken,
                    &gFrameTimeDraw);

  cmdDrawUserInterface(cmd);
}