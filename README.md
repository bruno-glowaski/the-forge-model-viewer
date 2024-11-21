# ForgeCMake
CMake files working for ConfettiFX/The-Forge 1.60

How to use:
1. Pull TheForge submodule, sadly I dont know the exact command but should be something like `git submodule init` and `git submodule update`.
2. Open Vendor/TheForge, run PRE_BUILD.bat or the PRE_BUILD script for your OS. Refer to https://github.com/ConfettiFX/The-Forge for the exact instructions (scroll down FAR).
3. For Windows, open `x64 Native Tools Command Prompt for VS 2022` or whatever Visual Studio version you have, cd to the folder of what you downloaded this repo.
4. Run `cmake . -B build` or similiar.
5. Then you can open the solution in the created build folder, compile in Visual Studio
6. Sadly you need to adjust the Working Directory for the Project manually for it to run, else it the Debugger wont find the files. I am working on this! Likewise change the active Project, I haven't manage to make it work with CMake.

Refer to CMakeLists.txt how to set up an project, modify the lines after `
// Example of how to set up forge project`. `tf_add_shader` will compile shaders in supplied ShaderList.fsl, `tf_add_forge_utils` is useful to automatically copy stuff needed to run The Forge.

Many thanks to https://github.com/Caellian for providing the intial CMake files!
