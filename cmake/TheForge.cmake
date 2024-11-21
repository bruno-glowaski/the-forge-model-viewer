# OS LIBRARY
set(TFORGE_COMMON_DIR ${CMAKE_SOURCE_DIR}/Vendor/TheForge/Common_3/)

set(OSBASE_SRC
    # cmake-format: sortable
    ${TFORGE_COMMON_DIR}Application/CameraController.cpp
    ${TFORGE_COMMON_DIR}Application/Fonts/FontSystem.cpp
    ${TFORGE_COMMON_DIR}Application/Fonts/stbtt.cpp
    ${TFORGE_COMMON_DIR}Application/Profiler/GpuProfiler.cpp
    ${TFORGE_COMMON_DIR}Application/Profiler/ProfilerBase.cpp
    ${TFORGE_COMMON_DIR}Application/Screenshot.cpp
    ${TFORGE_COMMON_DIR}Application/ThirdParty/OpenSource/imgui/imgui.cpp
    ${TFORGE_COMMON_DIR}Application/ThirdParty/OpenSource/imgui/imgui_demo.cpp
    ${TFORGE_COMMON_DIR}Application/ThirdParty/OpenSource/imgui/imgui_draw.cpp
    ${TFORGE_COMMON_DIR}Application/ThirdParty/OpenSource/imgui/imgui_tables.cpp
    ${TFORGE_COMMON_DIR}Application/ThirdParty/OpenSource/imgui/imgui_widgets.cpp
    ${TFORGE_COMMON_DIR}Application/UI/UI.cpp
    ${TFORGE_COMMON_DIR}Game/Scripting/LuaManager.cpp
    ${TFORGE_COMMON_DIR}Game/Scripting/LuaManagerImpl.cpp
    ${TFORGE_COMMON_DIR}Game/Scripting/LuaSystem.cpp
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lapi.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lauxlib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lbaselib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lbitlib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lcode.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lcorolib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lctype.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ldblib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ldebug.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ldo.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ldump.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lfunc.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lgc.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/linit.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/liolib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/llex.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lmathlib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lmem.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/loadlib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lobject.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lopcodes.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/loslib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lparser.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lstate.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lstring.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lstrlib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ltable.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ltablib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/ltm.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lundump.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lutf8lib.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lvm.c
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/lua-5.3.5/src/lzio.c
    ${TFORGE_COMMON_DIR}OS/CPUConfig.cpp
    ${TFORGE_COMMON_DIR}OS/Input/InputCommon.h
    ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/cpu_features/src/impl_aarch64_linux_or_android.c
    ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/cpu_features/src/impl_x86_linux_or_android.c
    ${TFORGE_COMMON_DIR}OS/WindowSystem/WindowSystem.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/AnimatedObject.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/Animation.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/Clip.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/ClipController.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/ClipMask.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/Rig.cpp
    ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/SkeletonBatcher.cpp
    ${TFORGE_COMMON_DIR}Tools/Network/Network.c
    ${TFORGE_COMMON_DIR}Tools/ReloadServer/ReloadClient.cpp
    ${TFORGE_COMMON_DIR}Utilities/FileSystem/FileSystem.c
    ${TFORGE_COMMON_DIR}Utilities/Log/Log.c
    ${TFORGE_COMMON_DIR}Utilities/Math/Algorithms.c
    ${TFORGE_COMMON_DIR}Utilities/Math/StbDs.c
    ${TFORGE_COMMON_DIR}Utilities/MemoryTracking/MemoryTracking.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/bstrlib/bstrlib.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/lz4/lz4.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/debug.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/entropy_common.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/error_private.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/fse_decompress.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/pool.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/threading.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/xxhash.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/zstd_common.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/decompress/huf_decompress.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/decompress/zstd_ddict.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/decompress/zstd_decompress.c
    ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/decompress/zstd_decompress_block.c
    ${TFORGE_COMMON_DIR}Utilities/Threading/ThreadSystem.c
    ${TFORGE_COMMON_DIR}Utilities/Timer.c
)

if(LINUX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(XRANDR REQUIRED xrandr)
    find_package(X11 REQUIRED)

    list(
        APPEND
        OSBASE_SRC
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxBase.cpp
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxFileSystem.c
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxInput.cpp
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxLog.c
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxThread.c
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxTime.c
        ${TFORGE_COMMON_DIR}OS/Linux/LinuxWindow.cpp
        ${TFORGE_COMMON_DIR}Utilities/FileSystem/UnixFileSystem.c
        ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/decompress/huf_decompress_amd64.S
    )
endif()

if(WIN32)
    list(
        APPEND
        OSBASE_SRC
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsBase.cpp
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsFileSystem.cpp
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsInput.cpp
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsLog.c
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsThread.c
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsTime.c
        ${TFORGE_COMMON_DIR}OS/Windows/WindowsWindow.cpp
        ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/cpu_features/src/impl_x86_windows.c
        ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/hidapi/windows/hid.c
    )
endif()

if(APPLE)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(METAL_FRAMEWORK Metal REQUIRED)
    find_library(QUARTZ_CORE_FRAMEWORK QuartzCore REQUIRED)

    if(IOS)
        find_library(UIKIT_FRAMEWORK UIKit REQUIRED)

        list(
            APPEND
            OSBASE_SRC
            ${TFORGE_COMMON_DIR}OS/Darwin/iOSBase.mm
            ${TFORGE_COMMON_DIR}OS/Darwin/CocoaFileSystem.mm
            ${TFORGE_COMMON_DIR}OS/Darwin/DarwinInput.mm
            ${TFORGE_COMMON_DIR}OS/Darwin/DarwinLog.c
            ${TFORGE_COMMON_DIR}OS/Darwin/DarwinThread.c
            ${TFORGE_COMMON_DIR}OS/Darwin/iOSWindow.mm
            ${TFORGE_COMMON_DIR}Utilities/FileSystem/UnixFileSystem.c
            ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/cpu_features/src/impl_aarch64_iOS.c
        )
    else()
        find_library(APPKIT_FRAMEWORK AppKit REQUIRED)

        list(
            APPEND
            OSBASE_SRC
            ${TFORGE_COMMON_DIR}OS/Darwin/macOSBase.mm
            ${TFORGE_COMMON_DIR}OS/Darwin/CocoaFileSystem.mm
            ${TFORGE_COMMON_DIR}OS/Darwin/DarwinInput.mm
            ${TFORGE_COMMON_DIR}OS/Darwin/DarwinLog.c
            ${TFORGE_COMMON_DIR}OS/Darwin/DarwinThread.c
            ${TFORGE_COMMON_DIR}OS/Darwin/macOSWindow.mm
            ${TFORGE_COMMON_DIR}Utilities/FileSystem/UnixFileSystem.c
            ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/cpu_features/src/impl_aarch64_iOS.c
        )
    endif()
    set(CXX_AS_OBJCXX_SRC
        # cmake-format: sortable
        ${TFORGE_COMMON_DIR}Application/Fonts/FontSystem.cpp
        ${TFORGE_COMMON_DIR}Application/Profiler/GpuProfiler.cpp
        ${TFORGE_COMMON_DIR}Application/Profiler/ProfilerBase.cpp
        ${TFORGE_COMMON_DIR}Application/Screenshot.cpp
        ${TFORGE_COMMON_DIR}Application/UI/UI.cpp
        ${TFORGE_COMMON_DIR}Game/Scripting/LuaSystem.cpp
        ${TFORGE_COMMON_DIR}OS/WindowSystem/WindowSystem.cpp
        ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/SkeletonBatcher.cpp
    )
    set_source_files_properties(${CXX_AS_OBJCXX_SRC} PROPERTIES COMPILE_FLAGS "-x objective-c++")

    set(OBJCXX_ARC OS/Darwin/macOSWindow.mm)
    set_source_files_properties(${OBJCXX_ARC} PROPERTIES COMPILE_FLAGS "-fobjc-arc")
endif()

if(ANDROID)
    find_library(LOG_LIBRARY log)
    find_library(
        VULKAN_LIBRARY
        NAMES vulkan
        PATHS ${ANDROID_NDK}/sources/third_party/vulkan/src/build-android/jniLibs/${ANDROID_ABI}
    )

    list(
        APPEND
        OSBASE_SRC
        ${TFORGE_COMMON_DIR}OS/Android/AndroidBase.cpp
        ${TFORGE_COMMON_DIR}OS/Android/AndroidFileSystem.cpp
        ${TFORGE_COMMON_DIR}Utilities/FileSystem/UnixFileSystem.c
        ${TFORGE_COMMON_DIR}OS/Android/AndroidInput.cpp
        ${TFORGE_COMMON_DIR}OS/Android/AndroidLog.c
        ${TFORGE_COMMON_DIR}OS/Android/AndroidThread.c
        ${TFORGE_COMMON_DIR}OS/Android/AndroidTime.c
        ${TFORGE_COMMON_DIR}OS/Android/AndroidWindow.cpp
        ${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/cpu_features/src/hwcaps.c
    )
endif()

include(ozz-animation)
add_library(OS STATIC ${OSBASE_SRC})
target_link_libraries(
    OS
    PUBLIC
        ozz_base
        ozz_animation
        $<$<PLATFORM_ID:Linux>:${GTK3_LIBRARIES}>
        $<$<PLATFORM_ID:Linux>:${X11_LIBRARIES}>
        $<$<PLATFORM_ID:Linux>:${XRANDR_LIBRARIES}>
        $<$<PLATFORM_ID:Darwin>:${COREFOUNDATION_FRAMEWORK}>
        $<$<PLATFORM_ID:Darwin>:${APPKIT_FRAMEWORK}>
        $<$<PLATFORM_ID:Darwin>:${METAL_FRAMEWORK}>
        $<$<PLATFORM_ID:Darwin>:${QUARTZ_CORE_FRAMEWORK}>
        $<$<PLATFORM_ID:iOS>:${UIKIT_FRAMEWORK}>
        $<$<PLATFORM_ID:Android>:${LOG_LIBRARY}>
        $<$<PLATFORM_ID:Android>:${VULKAN_LIBRARY}>
        $<$<PLATFORM_ID:Android>:${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/agdk/libs/arm64-v8a_cpp_static_Release/libmemory_advice_static.a>
        $<$<PLATFORM_ID:Android>:${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/agdk/libs/arm64-v8a_cpp_static_Release/libswappy_static.a>
)
target_include_directories(
    OS PUBLIC $<$<PLATFORM_ID:Linux>:${GTK3_INCLUDE_DIRS}> ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/TaskScheduler/Scheduler/Include/
              $<$<PLATFORM_ID:Android>:${TFORGE_COMMON_DIR}OS/ThirdParty/OpenSource/agdk/include/>
)
if(WIN32)
    target_compile_definitions(OS PUBLIC UNICODE)
endif()

# RENDERER LIBRARY

include(TheForgeUtils)

set(RENDERER_SRC
    # cmake-format: sortable
    ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/flecs/flecs.c
    ${TFORGE_COMMON_DIR}Graphics/CommonShaderReflection.cpp
    ${TFORGE_COMMON_DIR}Graphics/GraphicsConfig.cpp
    ${TFORGE_COMMON_DIR}Renderer/ParticleSystem/ParticleSystem.cpp
    ${TFORGE_COMMON_DIR}Renderer/VisibilityBuffer/VisibilityBuffer.cpp
    ${TFORGE_COMMON_DIR}Resources/ResourceLoader/ResourceLoader.cpp
)

if(LINUX)
    list(APPEND RENDERER_SRC
	${TFORGE_COMMON_DIR}Graphics/Vulkan/Vulkan.cpp
	${TFORGE_COMMON_DIR}Graphics/Vulkan/VulkanRaytracing.cpp
	${TFORGE_COMMON_DIR}Graphics/Vulkan/VulkanShaderReflection.cpp)
endif()

if(WIN32)
    list(
        APPEND
        RENDERER_SRC
        ${TFORGE_COMMON_DIR}Graphics/Vulkan/Vulkan.cpp
        ${TFORGE_COMMON_DIR}Graphics/Vulkan/VulkanRaytracing.cpp
        ${TFORGE_COMMON_DIR}Graphics/Vulkan/VulkanShaderReflection.cpp
        ${TFORGE_COMMON_DIR}Graphics/Direct3D12/Direct3D12.cpp
        ${TFORGE_COMMON_DIR}Graphics/Direct3D12/Direct3D12Raytracing.cpp
        ${TFORGE_COMMON_DIR}Graphics/Direct3D12/Direct3D12ShaderReflection.cpp
        ${TFORGE_COMMON_DIR}Graphics/Direct3D12/Direct3D12Hooks.cpp
        ${TFORGE_COMMON_DIR}Graphics/Direct3D11/Direct3D11.cpp
        ${TFORGE_COMMON_DIR}Graphics/Direct3D11/Direct3D11ShaderReflection.cpp
    )
endif()

if(APPLE)
    find_library(MPS_FRAMEWORK MetalPerformanceShaders)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)

    list(APPEND RENDERER_SRC
	    ${TFORGE_COMMON_DIR}Graphics/Metal/MetalRenderer.mm
	    ${TFORGE_COMMON_DIR}Graphics/Metal/MetalRaytracing.mm
	    ${TFORGE_COMMON_DIR}Graphics/Metal/MetalShaderReflection.mm
    )
    set_source_files_properties(${RENDERER_SRC} PROPERTIES COMPILE_FLAGS "-x objective-c++ -fobjc-arc")

    set(C_AS_CXX_SRC # cmake-format: sortable
        ${TFORGE_COMMON_DIR}Game/ThirdParty/OpenSource/flecs/flecs.c
    )
    set_source_files_properties(${C_AS_CXX_SRC} PROPERTIES COMPILE_FLAGS "-x c")
endif()

if(ANDROID)
    find_library(EGL_LIBRARY EGL)
    find_library(GLESv2_LIBRARY GLESv2)

    list(
        APPEND
        RENDERER_SRC
        ${TFORGE_COMMON_DIR}Graphics/Vulkan/Vulkan.cpp
        ${TFORGE_COMMON_DIR}Graphics/Vulkan/VulkanRaytracing.cpp
        ${TFORGE_COMMON_DIR}Graphics/Vulkan/VulkanShaderReflection.cpp
        ${TFORGE_COMMON_DIR}Graphics/OpenGLES/GLES.cpp
        ${TFORGE_COMMON_DIR}Graphics/OpenGLES/GLESShaderReflection.cpp
        ${TFORGE_COMMON_DIR}Graphics/OpenGLES/EGLContextCreator.cpp
    )
endif()

add_library(Renderer STATIC ${RENDERER_SRC})
target_link_libraries(
    Renderer
    PUBLIC SpirVTools
           OS
           $<$<PLATFORM_ID:Windows>:${TFORGE_COMMON_DIR}Graphics/ThirdParty/OpenSource/ags/ags_lib/lib/amd_ags_x64.lib>
           $<$<PLATFORM_ID:Windows>:${TFORGE_COMMON_DIR}Graphics/ThirdParty/OpenSource/nvapi/amd64/nvapi64.lib>
           $<$<PLATFORM_ID:Windows>:${TFORGE_COMMON_DIR}Graphics/ThirdParty/OpenSource/DirectXShaderCompiler/lib/x64/dxcompiler.lib>
           $<$<PLATFORM_ID:Windows>:${TFORGE_COMMON_DIR}Graphics/ThirdParty/OpenSource/winpixeventruntime/bin/WinPixEventRuntime.lib>
           $<$<OR:$<PLATFORM_ID:Darwin>,$<PLATFORM_ID:iOS>>:${MPS_FRAMEWORK}>
           $<$<PLATFORM_ID:iOS>:${COREGRAPHICS_FRAMEWORK}>
           $<$<PLATFORM_ID:iOS>:${METAL_FRAMEWORK}>
           $<$<PLATFORM_ID:iOS>:${QUARTZ_CORE_FRAMEWORK}>
           $<$<PLATFORM_ID:iOS>:${COREVIDEO_FRAMEWORK}>
           $<$<PLATFORM_ID:Android>:${EGL_LIBRARY}>
           $<$<PLATFORM_ID:Android>:${GLESv2_LIBRARY}>
)

if(WIN32)
    target_compile_definitions(Renderer PUBLIC D3D12_AGILITY_SDK_VERSION=715 UNICODE)
endif()

tf_add_shader(Renderer ${TFORGE_COMMON_DIR}Application/Fonts/Shaders/FSL/Fonts_ShaderList.fsl)
tf_add_shader(Renderer ${TFORGE_COMMON_DIR}Application/UI/Shaders/FSL/UI_ShaderList.fsl)
tf_add_shader(Renderer ${TFORGE_COMMON_DIR}Resources/AnimationSystem/Animation/Shaders/FSL/Animation_ShaderList.fsl)