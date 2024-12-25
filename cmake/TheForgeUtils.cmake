set(TFORGE_DIR ${CMAKE_SOURCE_DIR}/Vendor/TheForge)

if(LINUX OR ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
    # Set executable permissions
    execute_process(COMMAND chmod +x ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/VulkanSDK/bin/Linux/glslangValidator)
    execute_process(COMMAND chmod +x ${TFORGE_DIR}/Examples_3/forge_utilss/src/06_MaterialPlayground/compile_materials.sh)
endif()

# Determine Python command
if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
  set(TFORGE_PYTHON_COMMAND "python")
else()
  set(TFORGE_PYTHON_COMMAND  "${TFORGE_DIR}/Tools/python-3.6.0-embed-amd64/python.exe")
endif()

# Determines the unit test's resource dir
function(determine_resources_dir forge_utils_name result_var)
    if(LINUX OR ANDROID)
        set(${result_var}
            ${CMAKE_CURRENT_BINARY_DIR}
            PARENT_SCOPE
        )
    elseif(WIN32)
        set(${result_var}
            ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>
            PARENT_SCOPE
        )
    elseif(APPLE)
        set(${result_var}
            ${CMAKE_CURRENT_BINARY_DIR}/${forge_utils_name}.app/Contents/Resources
            PARENT_SCOPE
        )
    else()
        message(FATAL_ERROR "Unsupported platform.")
    endif()
endfunction()

# Determines The Forge's resource dir
function(determine_forge_resources_dir result_var)
    if(LINUX OR ANDROID)
        set(${result_var}
            ${TFORGE_DIR}/Common_3
            PARENT_SCOPE
        )
    elseif(WIN32)
        set(${result_var}
            ${TFORGE_DIR}/Common_3/$<CONFIG>
            PARENT_SCOPE
        )
    elseif(APPLE)
        set(${result_var}
            ${TFORGE_DIR}/Common_3/Renderer.app/Contents/Resources
            PARENT_SCOPE
        )
    else()
        message(FATAL_ERROR "Unsupported platform.")
    endif()
endfunction()

# Runs The Forge's FSL utility against a given shader list file
function(tf_add_shader target_name shader_list_file)
    # Step 1: Read the shader list file and extract .fsl file paths
    file(READ ${shader_list_file} shaders_content)
    string(REGEX MATCHALL "#include \"([^\"]*.fsl)\"" matched_fsl_files ${shaders_content})

    # Step 2: Create a list to hold the full paths of the .fsl files
    set(full_path_fsl_files "")
    get_filename_component(shaders_path "${shader_list_file}" DIRECTORY)
    get_filename_component(shader_list_filename "${shader_list_file}" NAME_WE)
    foreach(match ${matched_fsl_files})
        string(REGEX REPLACE "#include \"([^\"]*.fsl)\"" "\\1" fsl_file ${match})
        list(APPEND full_path_fsl_files "${shaders_path}/${fsl_file}")
    endforeach()

    determine_resources_dir(${target_name} resources_dir)

    # Step 3: set FSL language
    if(LINUX)
        set(fsl_language "VULKAN")
    elseif(WIN32)
        set(fsl_language "DIRECT3D12 VULKAN")
    elseif(APPLE)
        if(IOS)
            set(fsl_language "IOS")
        else()
            set(fsl_language "MACOS")
        endif()
    elseif(ANDROID)
        set(fsl_language "ANDROID_VULKAN")
    endif()

    # Step 4: Define the custom command with dependencies on the .fsl files
    add_custom_command(
        OUTPUT ${resources_dir}/Shaders/${shader_list_filename}.fsl.deps
        COMMAND
            ${TFORGE_PYTHON_COMMAND} ${TFORGE_DIR}/Common_3/Tools/ForgeShadingLanguage/fsl.py -l ${fsl_language} -d
            ${resources_dir}/Shaders --verbose -b ${resources_dir}/CompiledShaders -i ${resources_dir}/ --cache-args --incremental --compile
            ${shader_list_file}
        DEPENDS ${full_path_fsl_files} ${shader_list_file}
        COMMENT "Compiling shaders ${shader_list_file}..."
    )

    # Step 5: Create a custom target that depends on the custom command's output
    add_custom_target(
        ${forge_utils_name}-${shader_list_filename} ALL
        DEPENDS ${resources_dir}/Shaders/${shader_list_filename}.fsl.deps
        COMMENT "Compiling shaders ${shader_list_file}..."
    )

    # Step 6: Ensure your main target depends on the Shaders target
    add_dependencies(${target_name} ${forge_utils_name}-${shader_list_filename})
endfunction()

# Adds a new unit test
function(tf_add_forge_utils forge_utils_name)
    # Use ARGN to capture all additional arguments, which are your source files
    set(forge_utils_sources ${ARGN})

    determine_resources_dir(${forge_utils_name} resources_dir)
    determine_forge_resources_dir(forge_resources_dir)

    if(APPLE)
        # Link against the application's delegate class since it's not visible if built inside Forge libraries
        if(IOS)
            list(APPEND forge_utils_sources ${TFORGE_DIR}/Common_3/OS/Darwin/iOSAppDelegate.m)
        else()
            list(APPEND forge_utils_sources ${TFORGE_DIR}/Common_3/OS/Darwin/macOSAppDelegate.m)
        endif()
    endif()

    # COPY ASSETS COMMON TO ALL UNIT TESTS
    if(LINUX)
        # Copy Vulkan validation assets
        add_custom_command(
            TARGET ${forge_utils_name}
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy
                ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/VulkanSDK/bin/Linux/libVkLayer_khronos_validation.so
                ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND
                ${CMAKE_COMMAND} -E copy
                ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/VulkanSDK/bin/Linux/VkLayer_khronos_validation.json
                ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Copying Vulkan's validation assets"
        )
    endif()

    if(WIN32)
        set_target_properties(${forge_utils_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${resources_dir})
        add_custom_command(
            TARGET ${forge_utils_name}
            POST_BUILD
            COMMAND
                ${CMAKE_COMMAND} -E copy ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/ags/ags_lib/lib/amd_ags_x64.dll
                ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/Direct3d12Agility/bin/x64/D3D12Core.dll
                ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/Direct3d12Agility/bin/x64/d3d12SDKLayers.dll
                ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/DirectXShaderCompiler/bin/x64/dxcompiler.dll
                ${TFORGE_DIR}/Common_3/Graphics/ThirdParty/OpenSource/winpixeventruntime/bin/WinPixEventRuntime.dll ${resources_dir}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/PathStatement.txt ${resources_dir}/PathStatement.txt
            COMMENT "Copying Windows dependencies"
        )
    endif()

    if(APPLE)
        set_source_files_properties(${forge_utils_sources} PROPERTIES MACOSX_BUNDLE TRUE COMPILE_FLAGS "-x objective-c++ -fobjc-arc")
        if(IOS)
            set_source_files_properties(
                ${TFORGE_DIR}/Common_3/OS/Darwin/iOSAppDelegate.m PROPERTIES COMPILE_FLAGS "-x objective-c -fobjc-arc"
            )
            # add_custom_command(
            #     TARGET ${forge_utils_name}
            #     POST_BUILD
            #     COMMAND ${CMAKE_COMMAND} -E copy ${TFORGE_DIR}/Art/PathStatement.iOS.txt ${resources_dir}/PathStatement.txt
            #     COMMENT "Copying IOS dependencies"
            # )
        else()
            set_source_files_properties(
                ${TFORGE_DIR}/Common_3/OS/Darwin/macOSAppDelegate.m PROPERTIES COMPILE_FLAGS "-x objective-c -fobjc-arc"
            )
            # add_custom_command(
            #     TARGET ${forge_utils_name}
            #     POST_BUILD
            #     COMMAND ${CMAKE_COMMAND} -E copy ${TFORGE_DIR}/Art/PathStatement.MacOS.txt ${resources_dir}/PathStatement.txt
            #     COMMENT "Copying MacOS dependencies"
            # )
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../macOS Xcode/${forge_utils_name}/${forge_utils_name}/Base.lproj/MainMenu.xib")
            add_custom_command(
                TARGET ${forge_utils_name}
                POST_BUILD
                COMMAND xcrun ibtool --compile "${resources_dir}/Base.lproj/MainMenu.nib"
                        "${CMAKE_CURRENT_SOURCE_DIR}/../../macOS Xcode/${forge_utils_name}/${forge_utils_name}/Base.lproj/MainMenu.xib"
                COMMENT "Compiling unit test xib file"
            )
        endif()
        if(IOS)
            set_target_properties(
                ${forge_utils_name}
                PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION 1.58
                           MACOSX_BUNDLE_SHORT_VERSION_STRING "1.58"
                           MACOSX_BUNDLE_GUI_IDENTIFIER "The-Forge.${forge_utils_name}"
            )
        endif()
    endif()

    # if(ANDROID)
    #     add_custom_command(
    #         TARGET ${forge_utils_name}
    #         POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E copy ${TFORGE_DIR}/Art/PathStatement.Android.txt ${resources_dir}/PathStatement.txt
    #         COMMENT "Copying Android dependencies"
    #     )
    # endif()
    #
    # if(LINUX)
    #     add_custom_command(
    #         TARGET ${forge_utils_name}
    #         POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E copy ${TFORGE_DIR}/Art/PathStatement.SteamOS.txt ${resources_dir}/PathStatement.txt
    #         COMMENT "Copying Linux/SteamOS dependencies"
    #     )
    # endif()

    # Copy common shaders
    # add_custom_command(
    #     TARGET ${forge_utils_name}
    #     POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_directory ${forge_resources_dir}/Shaders ${resources_dir}/Shaders
    #     COMMAND ${CMAKE_COMMAND} -E copy_directory ${forge_resources_dir}/CompiledShaders ${resources_dir}/CompiledShaders
    #     COMMENT "Copying common shaders..."
    # )

    # Copy GPU config
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/GPUCfg")
        add_custom_command(
            TARGET ${forge_utils_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/GPUCfg ${resources_dir}/GPUCfg
            COMMENT "Copying GPU config..."
        )
    endif()

    # Define the GPU data file path
    set(gpu_data_file
        "$<$<PLATFORM_ID:Linux>:${TFORGE_DIR}/Common_3/OS/Linux/steamdeck_gpu.data>"
        $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:iOS>>:${TFORGE_DIR}/Common_3/OS/Darwin/apple_gpu.data>
        "$<$<PLATFORM_ID:Windows>:${TFORGE_DIR}/Common_3/OS/Windows/pc_gpu.data>"
        "$<$<PLATFORM_ID:Android>:${TFORGE_DIR}/Common_3/OS/Android/android_gpu.data>"
    )

    # Copy GPU data
    add_custom_command(
        TARGET ${forge_utils_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${gpu_data_file} ${resources_dir}/gpu.data
        COMMENT "Copying GPU data..."
    )

    # Copy fonts
    add_custom_command(
        TARGET ${forge_utils_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${TFORGE_DIR}/Art/Fonts ${resources_dir}/Fonts
        COMMENT "Copying fonts..."
    )

    # Copy global scripts
    add_custom_command(
        TARGET ${forge_utils_name}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${TFORGE_DIR}/Art/Scripts ${resources_dir}/Scripts
        COMMENT "Copying scripts..."
    )

    # Copy unit test scripts
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Scripts)
        add_custom_command(
            TARGET ${forge_utils_name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/Scripts ${resources_dir}/Scripts
            COMMENT "Copying scripts..."
        )
    endif()
endfunction()
