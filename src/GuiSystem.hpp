#pragma once

#include "Application/Interfaces/IFont.h"
#include "Application/Interfaces/IProfiler.h"
#include "Application/Interfaces/IUI.h"

#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "OrbitCameraController.hpp"
#include "RenderContext.hpp"
#include "Scene.hpp"
#include "SceneRenderSystem.hpp"
#include "SkyBox.hpp"

struct GuiModelView {
  float *pSceneScale;
  float *pCameraAcceleration;
  float *pCameraBraking;
  float *pCameraZoomSpeed;
  float *pCameraOrbitSpeed;
};

class GuiSystem {
public:
  void Init();
  void Exit();

  void Load(GuiModelView guiModelView, int32_t appWidth, int32_t appHeight,
            ReloadDesc *pReloadDesc);
  void Unload(ReloadDesc *pReloadDesc);

  void Draw(RenderContext::Frame frame, ProfileToken gpuProfileToken);

private:
  uint32_t gFontID = 0;
  FontDrawDesc gFrameTimeDraw;

  const char *const kControlsTextCharArray = "Manual:\n"
                                             "W: Zoom in\n"
                                             "S: Zoom out\n"
                                             "A: Orbit right\n"
                                             "D: Orbit left\n"
                                             "Q: Orbit down\n"
                                             "E: Orbit up\n"
                                             "Mouse drag: Orbit around\n";
  bstring gControlsText = bfromarr(kControlsTextCharArray);

  UIComponent *pControlsWindow = NULL;
  UIComponent *pSceneOptionsWindow = NULL;
};