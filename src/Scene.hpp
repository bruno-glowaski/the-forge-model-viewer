#pragma once

#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "RenderContext.hpp"

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