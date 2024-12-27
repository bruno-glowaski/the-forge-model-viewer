#include "Scene.hpp"

#include "SceneRenderSystem.hpp"

void Scene::LoadMeshResource(RenderContext &renderContext,
                             const char *pResourceFileName) {
  GeometryLoadDesc sceneGDesc = {};
  sceneGDesc.ppGeometry = &pGeometry;
  sceneGDesc.ppGeometryData = &pGeometryData;
  sceneGDesc.pFileName = pResourceFileName;
  sceneGDesc.pVertexLayout = &kSceneVertexLayout;
  addResource(&sceneGDesc, NULL);
}
void Scene::Destroy(RenderContext &renderContext) {
  removeResource(pGeometry);
  removeResource(pGeometryData);
}