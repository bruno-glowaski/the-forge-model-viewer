#pragma once

#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "RenderContext.hpp"

class SkyBox {
public:
  static const size_t kSideCount = 6;

  void Load(RenderContext &renderContext,
            const char *const pTextureFilenames[SkyBox::kSideCount]);
  void LoadDefault(RenderContext &renderContext);
  void Destroy(RenderContext &renderContext);

  inline Sampler *const &GetSampler() const { return pSampler; }
  inline Texture *const &GetTexture(size_t i) const { return pTextures[i]; }
  inline Buffer *const &GetVertexBuffer() const { return pVertexBuffer; };

private:
  Buffer *pVertexBuffer = NULL;

  Texture *pTextures[kSideCount] = {};
  Sampler *pSampler = NULL;
};