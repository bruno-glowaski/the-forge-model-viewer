#include "SkyBox.hpp"

const float kSkyBoxVertices[4 * 6 * SkyBox::kSideCount] = {
    10.0f,  -10.0f, -10.0f, 6.0f, // -z
    -10.0f, -10.0f, -10.0f, 6.0f,   -10.0f, 10.0f,  -10.0f,
    6.0f,   -10.0f, 10.0f,  -10.0f, 6.0f,   10.0f,  10.0f,
    -10.0f, 6.0f,   10.0f,  -10.0f, -10.0f, 6.0f,

    -10.0f, -10.0f, 10.0f,  2.0f, //-x
    -10.0f, -10.0f, -10.0f, 2.0f,   -10.0f, 10.0f,  -10.0f,
    2.0f,   -10.0f, 10.0f,  -10.0f, 2.0f,   -10.0f, 10.0f,
    10.0f,  2.0f,   -10.0f, -10.0f, 10.0f,  2.0f,

    10.0f,  -10.0f, -10.0f, 1.0f, //+x
    10.0f,  -10.0f, 10.0f,  1.0f,   10.0f,  10.0f,  10.0f,
    1.0f,   10.0f,  10.0f,  10.0f,  1.0f,   10.0f,  10.0f,
    -10.0f, 1.0f,   10.0f,  -10.0f, -10.0f, 1.0f,

    -10.0f, -10.0f, 10.0f,  5.0f, // +z
    -10.0f, 10.0f,  10.0f,  5.0f,   10.0f,  10.0f,  10.0f,
    5.0f,   10.0f,  10.0f,  10.0f,  5.0f,   10.0f,  -10.0f,
    10.0f,  5.0f,   -10.0f, -10.0f, 10.0f,  5.0f,

    -10.0f, 10.0f,  -10.0f, 3.0f, //+y
    10.0f,  10.0f,  -10.0f, 3.0f,   10.0f,  10.0f,  10.0f,
    3.0f,   10.0f,  10.0f,  10.0f,  3.0f,   -10.0f, 10.0f,
    10.0f,  3.0f,   -10.0f, 10.0f,  -10.0f, 3.0f,

    10.0f,  -10.0f, 10.0f,  4.0f, //-y
    10.0f,  -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f, -10.0f,
    4.0f,   -10.0f, -10.0f, -10.0f, 4.0f,   -10.0f, -10.0f,
    10.0f,  4.0f,   10.0f,  -10.0f, 10.0f,  4.0f,
};

const char *const kDefaultSkyBoxImageFileNames[SkyBox::kSideCount] = {
    "Skybox_right1.tex",  "Skybox_left2.tex",  "Skybox_top3.tex",
    "Skybox_bottom4.tex", "Skybox_front5.tex", "Skybox_back6.tex"};

void SkyBox::Load(RenderContext &renderContext,
                  const char *const pTextureFilenames[kSideCount]) {
  for (int i = 0; i < 6; ++i) {
    TextureLoadDesc textureDesc = {};
    textureDesc.pFileName = pTextureFilenames[i];
    textureDesc.ppTexture = &pTextures[i];
    textureDesc.mCreationFlag = TEXTURE_CREATION_FLAG_SRGB;
    addResource(&textureDesc, NULL);
  }

  SamplerDesc samplerDesc = {FILTER_LINEAR,
                             FILTER_LINEAR,
                             MIPMAP_MODE_NEAREST,
                             ADDRESS_MODE_CLAMP_TO_EDGE,
                             ADDRESS_MODE_CLAMP_TO_EDGE,
                             ADDRESS_MODE_CLAMP_TO_EDGE};
  pSampler = renderContext.CreateSampler(&samplerDesc);

  uint64_t skyBoxDataSize = 4 * 6 * kSideCount * sizeof(float);
  BufferLoadDesc skyboxVbDesc = {};
  skyboxVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
  skyboxVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_GPU_ONLY;
  skyboxVbDesc.mDesc.mSize = skyBoxDataSize;
  skyboxVbDesc.pData = kSkyBoxVertices;
  skyboxVbDesc.ppBuffer = &pVertexBuffer;
  addResource(&skyboxVbDesc, NULL);
}

void SkyBox::LoadDefault(RenderContext &renderContext) {
  Load(renderContext, kDefaultSkyBoxImageFileNames);
}

void SkyBox::Destroy(RenderContext &renderContext) {
  removeResource(pVertexBuffer);

  renderContext.DestroySampler(pSampler);

  for (uint i = 0; i < kSideCount; ++i)
    removeResource(pTextures[i]);
}