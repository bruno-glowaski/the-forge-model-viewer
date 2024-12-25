# Model Viewer over The Forge Framework

A 3D model viewer developed over the [The Forge Framework](https://github.com/ConfettiFX/The-Forge). Primarily designed to display a single specific FBX model file. Using [tomwjerry's ForgeCMake](https://github.com/tomwjerry/ForgeCMake), a set of CMake files, as a base.

## Requirements

### Essential

- [x] User can see a 3D model;
- [x] User can rotate the camera around the model;
- [x] User can run this on Windows 10;

### Extra

- [x] User can control the camera parameters;
- [ ] User can scale the model as they please;
- [ ] User can load any FBX model directly;
- [ ] User can toggle multisampling;
- [ ] User can configure lights;
- [ ] User can toggle ambient occlusion;
- [ ] User can toggle PBR shading.

## Compile instructions

1. Clone this repository and its submodules:
```sh
git clone --recurse-submodules https://github.com/bruno-glowaski/the-forge-model-viewer
```
2. Run `Vendor/TheForge/PRE_BUILD.bat`, or `Vendor/TheForge/PRE_BUILD.command` (on Linux or Mac) to download the assets;
3. Create a `Assets` folder and copy some assets into it:
    - The FBX model, renamed `castle.fbx` into `Assets/Meshes`;
    - Skybox textures, from `Art/Textures` folder from the previous script, into `Assets/Textures`:
        - Obs.: On Windows, use the textures inside `dds`; on Linux, use the textures `ktx`;
        - "Skybox_right1.tex";
        - "Skybox_left2.tex";
        - "Skybox_top3.tex";
        - "Skybox_bottom4.tex";
        - "Skybox_front5.tex";
        - "Skybox_back6.tex";
    - The GUI font, `TitilliumText`, from `Art/Fonts`, into `Assets/Fonts`;
        - Obs.: copy the whole folder;
5. Compile and configure the project.

On Windows, either use Visual Studios 2019 or comment out line 239-243 in `Vendor/TheForge/Common_3/Application/Config.h` to disable the MSVC whitelist.

