#include "Scene.hpp"

#include "Utilities/Math/ShaderUtilities.h"

// FBX Loader
#include "ofbx.h"

#include "SceneRenderSystem.hpp"

struct SceneVertex {
  float3 mPosition;
  uint32_t mNormal;
  float2 mUv;
};

void Scene::LoadMeshResource(RenderContext &renderContext,
                             const char *pResourceFileName) {
  GeometryLoadDesc sceneGDesc = {};
  sceneGDesc.ppGeometry = &pGeometry;
  sceneGDesc.ppGeometryData = &pGeometryData;
  sceneGDesc.pFileName = pResourceFileName;
  sceneGDesc.pVertexLayout = &kSceneVertexLayout;
  addResource(&sceneGDesc, NULL);
  mKind = SceneKind::Preprocessed;
}
void Scene::LoadRawFBX(RenderContext &renderContext,
                       const char *pResourceFileName) {
  FileStream file = {};
  if (!fsOpenStreamFromPath(RD_MESHES, pResourceFileName, FileMode::FM_READ,
                            &file)) {
    LOGF(eERROR, "Failed to open file %s", pResourceFileName);
    ASSERT(false);
  }

  size_t fileSize = fsGetStreamFileSize(&file);
  ofbx::u8 *data = reinterpret_cast<ofbx::u8 *>(tf_calloc(1, fileSize));
  fsReadFromStream(&file, data, fileSize);

  ofbx::LoadFlags flags =
      //		ofbx::LoadFlags::IGNORE_MODELS |
      ofbx::LoadFlags::IGNORE_BLEND_SHAPES | ofbx::LoadFlags::IGNORE_CAMERAS |
      ofbx::LoadFlags::IGNORE_LIGHTS | ofbx::LoadFlags::IGNORE_TEXTURES |
      ofbx::LoadFlags::IGNORE_SKIN | ofbx::LoadFlags::IGNORE_BONES |
      ofbx::LoadFlags::IGNORE_PIVOTS | ofbx::LoadFlags::IGNORE_MATERIALS |
      ofbx::LoadFlags::IGNORE_POSES | ofbx::LoadFlags::IGNORE_VIDEOS |
      ofbx::LoadFlags::IGNORE_LIMBS |
      //		ofbx::LoadFlags::IGNORE_MESHES |
      ofbx::LoadFlags::IGNORE_ANIMATIONS;

  // There is a leak here. OpenFBX allocates a buffer with unique_ptr, but we
  // can't free it here because the "delete" keyword has been redifined. There
  // are two options (aside from ignoring this): 1) Modify OpenFBX to allow
  // custom allocators; 2) Prevent the redefining of the "delete" keyword for
  // this section only.
  auto scene = ofbx::load(data, fileSize, (ofbx::u16)flags);
  if (scene == nullptr) {
    LOGF(LogLevel::eERROR, "Failed to load FBX: %s", ofbx::getError());
  }

  size_t maxTriangleCount = 0;
  for (size_t i = 0; i < scene->getGeometryCount(); i++) {
    auto geometry = scene->getGeometry(i);
    auto &data = geometry->getGeometryData();

    for (size_t j = 0; j < data.getPartitionCount(); j++) {
      auto partition = data.getPartition(j);
      maxTriangleCount += partition.triangles_count * 3;
    }
  }
  auto maxVertexCount = maxTriangleCount * 3;

  auto vertices = reinterpret_cast<SceneVertex *>(
      tf_calloc(maxVertexCount, sizeof(SceneVertex)));
  auto indices =
      reinterpret_cast<int32_t *>(tf_calloc(maxVertexCount, sizeof(int32_t)));

  auto indicesCursor = indices;
  for (size_t i = 0; i < scene->getGeometryCount(); i++) {
    auto geometry = scene->getGeometry(i);
    auto &data = geometry->getGeometryData();
    auto positions = data.getPositions();
    auto normals = data.getNormals();
    auto uvs = data.getUVs();

    for (size_t j = 0; j < data.getPartitionCount(); j++) {
      auto partition = data.getPartition(j);
      for (size_t k = 0; k < partition.polygon_count; k++) {
        auto &polygon = partition.polygons[k];
        uint32_t triCount = ofbx::triangulate(data, polygon, indicesCursor);
        for (size_t l = 0; l < triCount; l++) {
          size_t vertexIndex = indicesCursor[l];
          auto position = positions.get(vertexIndex);
          auto normal = normals.get(vertexIndex);
          auto uv = uvs.get(vertexIndex);
          vertices[vertexIndex] = {
              {position.x, position.y, position.z},
              packUnorm2x16(encodeDir({normal.x, normal.y, normal.z})),
              {uv.x, uv.y},
          };
        }
        indicesCursor += triCount;
      }
    }
  }

  BufferLoadDesc vbDesc = {};
  vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
  vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
  vbDesc.mDesc.pName = "VertexBuffer";
  vbDesc.mDesc.mSize = maxVertexCount;
  vbDesc.pData = vertices;
  vbDesc.ppBuffer = &pVertexBuffer;
  addResource(&vbDesc, nullptr);

  BufferLoadDesc ibDesc = {};
  ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
  ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
  ibDesc.mDesc.pName = "IndexBuffer";
  ibDesc.mDesc.mSize = maxVertexCount;
  ibDesc.pData = indices;
  ibDesc.ppBuffer = &pIndexBuffer;
  addResource(&ibDesc, nullptr);

  tf_free(data);

  mKind = SceneKind::Raw;
  pVertices = vertices;
  pIndices = indices;
}
void Scene::Destroy(RenderContext &renderContext) {
  switch (mKind) {
  case SceneKind::Raw:
    removeResource(pVertexBuffer);
    removeResource(pIndexBuffer);
    tf_free(pVertices);
    tf_free(pIndices);
    return;
  case SceneKind::Preprocessed:
    removeResource(pGeometry);
    removeResource(pGeometryData);
    return;
  }
}
