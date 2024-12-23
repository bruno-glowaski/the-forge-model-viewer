# Model Viewer over The Forge Framework

A 3D model viewer developed over the [The Forge Framework](https://github.com/ConfettiFX/The-Forge). Primarily designed to display a single specific FBX model file. Using [tomwjerry's ForgeCMake](https://github.com/tomwjerry/ForgeCMake), a set of CMake files, as a base.

## Requirements

### Essential

- [x] User can see a 3D model;
- [x] User can rotate the camera around the model;
- [ ] User can run this on Windows 11;

### Extra

- [ ] User can control the camera parameters;
- [ ] User can scale the model as they please;
- [ ] User can load any FBX model directly;
- [ ] User can toggle multisampling;
- [ ] User can configure lights;
- [ ] User can toggle ambient occlusion;
- [ ] User can toggle PBR shading.

## Instructions

1. If you have a FBX model, convert it to GLTF (you can use [this tool](https://github.com/facebookincubator/FBX2glTF));
2. Compile The Forge's *AssetPipeline* CLI tool (on Linux, this happens while compiling this application);
    - On Linux, you might need to run `make -C Vendor/Common_3/Tools/ThirdParty/OpenSource/ISPCTextureCompressor -f Makefile.linux ISPC_ARCH=x86-64 ISPC_TARGETS=sse2,avx ISPC_OBJS="sse2 avx" ARCH_CXXFLAGS=-msse2` beforehand. You may also change the `ISPC_` flags according to the features of your processor;
3. Process the .GLTF file into a .BIN file using the *AssetPipeline*;
4. Place the processed .BIN file in the `Assets/Meshes` directory, and rename it to `castle.bin`;
5. Compile the project using `cmake -B build`;
6. Navigate into the `build` directory;
7. Run the application using `./ModelViewer`.

