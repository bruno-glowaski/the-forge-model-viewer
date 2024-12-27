#pragma once

#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "RenderContext.hpp"

struct Scene {
public:
  void LoadMeshResource(RenderContext &renderContext,
                        const char *pResourceFileName);
  void Destroy(RenderContext &renderContext);

  inline uint32_t GetIndexCount() const { return pGeometry->mIndexCount; }
  inline uint32_t GetVertexBufferCount() const {
    return pGeometry->mVertexBufferCount;
  }
  inline Buffer **GetVertexBuffers() const { return pGeometry->pVertexBuffers; }
  inline Buffer *GetIndexBuffer() const { return pGeometry->pIndexBuffer; }

private:
  Geometry *pGeometry;
  GeometryData *pGeometryData;
};