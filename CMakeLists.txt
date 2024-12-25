# We need at least 3.12 for list(JOIN....), add_compile_definitions
# 3.15 for CMAKE_CXX_COMPILER_FRONTEND_VARIANT
cmake_minimum_required(VERSION 3.15)

project(TheForgeUtils)

project(The-Forge LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_UNIT_TESTS "Build The-Forge's unit tests" ON)
option(BUILD_ASSETPIPELINE "Build The-Forge's asset pipeline command" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(TheForge)
include(TheForgeUtils)
include(SpirVTools)

file(GLOB MODEL_VIEWER_SRC "${CMAKE_SOURCE_DIR}/src/*.*")
add_executable(ModelViewer ${MODEL_VIEWER_SRC})

target_include_directories(ModelViewer PRIVATE 
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge"
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Common_3" 
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Tools"
)

set(MODEL_VIEWER_LIBS OS Renderer)
if (LINUX)
	list(APPEND MODEL_VIEWER_LIBS udev)
elseif(WIN32)
	list(APPEND MODEL_VIEWER_LIBS Xinput9_1_0 ws2_32)
endif()

target_link_libraries(ModelViewer PRIVATE ${MODEL_VIEWER_LIBS})

determine_resources_dir(ModelViewer MODELVIEWER_RESOURCES_DIR)

file(GLOB MODEL_VIEWER_GLTF_MESHES "${CMAKE_SOURCE_DIR}/Assets/Meshes/*.gltf")
add_custom_target(ModelViewerAssets 
	COMMAND ${ASSETPIPELINE_CMD} -pgltf --input "${CMAKE_SOURCE_DIR}/Assets" --output "${CMAKE_SOURCE_DIR}/Assets"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/PathStatement.txt" "${MODELVIEWER_RESOURCES_DIR}/PathStatement.txt"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Assets" "${MODELVIEWER_RESOURCES_DIR}/Assets"
	DEPENDS AssetPipeline ${MODEL_VIEWER_GLTF_MESHES}
)

add_dependencies(ModelViewer ModelViewerAssets)

set(VS_STARTUP_PROJECT ModelViewer)

tf_add_shader(ModelViewer "${CMAKE_SOURCE_DIR}/src/shaders/ShaderList.fsl")
tf_add_forge_utils(ModelViewer)

