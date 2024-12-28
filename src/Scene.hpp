#pragma once

#include "Graphics/Interfaces/IGraphics.h"
#include "Resources/ResourceLoader/Interfaces/IResourceLoader.h"

#include "RenderContext.hpp"

enum class SceneKind {
  Raw,
  Preprocessed,
};

struct Scene {
public:
  void LoadMeshResource(RenderContext &renderContext,
                        const char *pResourceFileName);
  /// Ideally, this would have been integrated inside The Forge's Resource
  /// Loader systems as to leverage its multithreading, but I'd like to keep
  /// Forge as vanilla as I can.
  /// For simplicity, this loads all meshes and models into a single unified
  /// geometry.
  void LoadRawFBX(RenderContext &renderContext, const char *pFilePath);
  void Destroy(RenderContext &renderContext);

  inline uint32_t GetIndexCount() const {
    switch (mKind) {
    case SceneKind::Preprocessed:
      return pGeometry->mIndexCount;
    case SceneKind::Raw:
      return mIndexCount;
    }
  }
  inline uint32_t GetVertexBufferCount() const {
    switch (mKind) {
    case SceneKind::Preprocessed:
      return pGeometry->mVertexBufferCount;
    case SceneKind::Raw:
      return 1;
    }
  }
  inline Buffer *const *GetVertexBuffers() const {
    switch (mKind) {
    case SceneKind::Preprocessed:
      return pGeometry->pVertexBuffers;
    case SceneKind::Raw:
      return &pVertexBuffer;
    }
  }
  inline Buffer *GetIndexBuffer() const {
    switch (mKind) {
    case SceneKind::Preprocessed:
      return pGeometry->pIndexBuffer;
    case SceneKind::Raw:
      return pIndexBuffer;
    }
  }

private:
  // std::variant doesn't exist on C++14 ¯\_(ツ)_/¯
  SceneKind mKind;
  union {
    struct {
      Geometry *pGeometry;
      GeometryData *pGeometryData;
    };
    struct {
      void *pVertices, *pIndices;
      Buffer *pVertexBuffer;
      Buffer *pIndexBuffer;
      uint32_t mIndexCount;
    };
  };
};
