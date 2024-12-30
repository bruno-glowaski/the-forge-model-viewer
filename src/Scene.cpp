#include "Scene.hpp"

#include "Utilities/Math/ShaderUtilities.h"

#include "ofbx.h"

#include "SceneRenderSystem.hpp"

struct SceneVertex {
  float3 mPosition;
  uint32_t mNormal;
  uint32_t mUv;
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

  uint32_t maxVertexCount = 0;
  uint32_t maxIndexPerPolygonCount = 0;
  for (uint32_t geomIdx = 0; geomIdx < scene->getGeometryCount(); geomIdx++) {
    auto geometry = scene->getGeometry(geomIdx);
    auto &geomData = geometry->getGeometryData();
    for (uint32_t partIdx = 0; partIdx < geomData.getPartitionCount();
         partIdx++) {
      auto partition = geomData.getPartition(partIdx);
      maxVertexCount += partition.triangles_count * 3;
      maxIndexPerPolygonCount =
          max(maxIndexPerPolygonCount,
              (uint32_t)partition.max_polygon_triangles * 3);
    }
  }
  uint32_t maxIndexCount = maxVertexCount;
  auto vertices = reinterpret_cast<SceneVertex *>(
      tf_calloc(maxVertexCount, sizeof(SceneVertex)));
  auto indices =
      reinterpret_cast<int16_t *>(tf_calloc(maxIndexCount, sizeof(int16_t)));

  auto indexTmp = reinterpret_cast<int32_t *>(
      tf_calloc(maxIndexPerPolygonCount, sizeof(int32_t)));
  mIndexCount = 0;
  auto write = [&](SceneVertex vertex) {
    vertices[mIndexCount] = vertex;
    indices[mIndexCount] = mIndexCount;
    mIndexCount++;
  };

  for (uint32_t geomIdx = 0; geomIdx < scene->getGeometryCount(); geomIdx++) {
    auto geometry = scene->getGeometry(geomIdx);
    auto &geomData = geometry->getGeometryData();
    auto positions = geomData.getPositions();
    auto normals = geomData.getNormals();
    auto uvs = geomData.getUVs();
    for (uint32_t partIdx = 0; partIdx < geomData.getPartitionCount();
         partIdx++) {
      auto partition = geomData.getPartition(partIdx);
      for (size_t polyIdx = 0; polyIdx < partition.polygon_count; polyIdx++) {
        auto polygon = partition.polygons[polyIdx];
        uint32_t vertexCount = ofbx::triangulate(geomData, polygon, indexTmp);
        for (size_t vtxIdx = 0; vtxIdx < vertexCount; vtxIdx++) {
          int32_t geomVIdx = indexTmp[vtxIdx];
          auto rawPosition = positions.get(geomVIdx);
          auto rawNormal = normals.get(geomVIdx);
          auto rawUv = uvs.get(geomVIdx);
          float3 position = {(float)rawPosition.x, (float)rawPosition.y,
                             (float)rawPosition.z};
          uint32_t normal =
              packUnorm2x16(encodeDir({rawNormal.x, rawNormal.y, rawNormal.z}));
          uint32_t uv = packFloat2ToHalf2({rawUv.x, rawUv.y});
          write({position, normal, uv});
        }
      }
    }
  }
  tf_free(indexTmp);

  BufferLoadDesc vbDesc = {};
  vbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
  vbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
  vbDesc.mDesc.pName = "VertexBuffer";
  vbDesc.mDesc.mSize = maxVertexCount * sizeof(SceneVertex);
  vbDesc.pData = vertices;
  vbDesc.ppBuffer = &pVertexBuffer;
  addResource(&vbDesc, nullptr);

  BufferLoadDesc ibDesc = {};
  ibDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_INDEX_BUFFER;
  ibDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
  ibDesc.mDesc.pName = "IndexBuffer";
  ibDesc.mDesc.mSize = maxIndexCount * sizeof(uint16_t);
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
