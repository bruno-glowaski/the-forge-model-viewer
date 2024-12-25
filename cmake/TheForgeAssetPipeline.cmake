set(ITOOLFILESYSTEM_SRC ${TFORGE_COMMON_DIR}Utilities/FileSystem/ToolFileSystem.c)
file(GLOB ASSETPIPELINEBASE_SRC ${TFORGE_COMMON_DIR}Tools/AssetPipeline/src/*.cpp)
set(BUNYARCHIVE_SRC 
  ${TFORGE_COMMON_DIR}Tools/BunyArchive/Buny.c
)
file(GLOB LZ4_SRC ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/lz4/*.c)
file(GLOB MESHOPTIOMIZER_SRC ${TFORGE_COMMON_DIR}Tools/ThirdParty/OpenSource/meshoptimizer/src/*.cpp)
file(GLOB MESHOPTIOMIZER_SRC ${TFORGE_COMMON_DIR}Tools/ThirdParty/OpenSource/meshoptimizer/src/*.cpp)
set(TRESSFX_SRC ${TFORGE_COMMON_DIR}Resources/AnimationSystem/ThirdParty/OpenSource/TressFX/TressFXAsset.cpp)
file(GLOB ZSTD_COMMON_SRC ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/common/*.c)
file(GLOB ZSTD_COMPRESS_SRC ${TFORGE_COMMON_DIR}Utilities/ThirdParty/OpenSource/zstd/compress/*.c)
file(GLOB ISPCTEXTURECOMPRESSOR_OBJ ${TFORGE_COMMON_DIR}Tools/ThirdParty/OpenSource/ISPCTextureCompressor/ispc_texcomp/*.o)

if (LINUX)
  list(APPEND ITOOLFILESYSTEM_SRC ${TFORGE_COMMON_DIR}OS/Linux/LinuxToolsFileSystem.c)
elseif(WIN32)
  list(APPEND ITOOLFILESYSTEM_SRC ${TFORGE_COMMON_DIR}OS/Windows/WindowsToolsFileSystem.cpp)
endif()

set(ASSETPIPELINE_SRC 
  ${ITOOLFILESYSTEM_SRC} 
  ${ASSETPIPELINEBASE_SRC} 
  ${BUNYARCHIVE_SRC}
  ${LZ4_SRC}
  ${MESHOPTIOMIZER_SRC}
  ${TRESSFX_SRC}
  ${ZSTD_COMMON_SRC}
  ${ZSTD_COMPRESS_SRC}
)

set(ASSETPIPELINE_LIB OS ozz_base ozz_animation ozz_animation_offline ${ISPCTEXTURECOMPRESSOR_OBJ})
if(WIN32)
  list(APPEND ASSETPIPELINE_LIB ws2_32)
endif()

add_executable(AssetPipeline ${ASSETPIPELINE_SRC})
target_link_libraries(AssetPipeline PRIVATE ${ASSETPIPELINE_LIB})