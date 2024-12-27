#pragma once

#include "Application/Interfaces/IProfiler.h"
#include "OS/Interfaces/IOperatingSystem.h"
#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"
#include "Utilities/RingBuffer.h"

#include "RenderContext.hpp"

class SceneRenderSystem {
  bool Init();
  void Exit();

  bool Load(ReloadDesc *pReloadDesc);
  void Unload(ReloadDesc *pReloadDesc);

  void Draw(RenderContext::Frame &frame);
};