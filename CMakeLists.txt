# We need at least 3.12 for list(JOIN....), add_compile_definitions
# 3.15 for CMAKE_CXX_COMPILER_FRONTEND_VARIANT
cmake_minimum_required(VERSION 3.15)

project(TheForgeUtils)

project(The-Forge LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_UNIT_TESTS "Build The-Forge's unit tests" ON)
option(BUILD_ASSETPIPELINE "Build The-Forge's asset pipeline command" OFF)

# if (NOT FBX2GLTF_BIN) 
# 	message(FATAL_ERROR "FBX2GLTF_BIN is unset. Download it (such as from https://github.com/facebookincubator/FBX2glT) and set FBX2GLTF_BIN with its path.")
# elseif(NOT EXISTS "${FBX2GLTF_BIN}")
# 	message(FATAL_ERROR "Could not find \"${FBX2GLTF_BIN}\"")
# endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(TheForge)
include(TheForgeUtils)
include(SpirVTools)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Vendor/OpenFBX")

file(GLOB MODEL_VIEWER_SRC "${CMAKE_SOURCE_DIR}/src/*.*")
add_executable(ModelViewer ${MODEL_VIEWER_SRC})

target_include_directories(ModelViewer PRIVATE 
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge"
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Common_3" 
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Tools"
)

set(MODEL_VIEWER_LIBS OS Renderer OpenFBX)
if (LINUX)
	list(APPEND MODEL_VIEWER_LIBS udev)
elseif(WIN32)
	list(APPEND MODEL_VIEWER_LIBS Xinput9_1_0 ws2_32)
endif()

target_link_libraries(ModelViewer PRIVATE ${MODEL_VIEWER_LIBS})

determine_resources_dir(ModelViewer MODELVIEWER_RESOURCES_DIR)

# file(GLOB MODEL_VIEWER_FBX_MESHES "${CMAKE_SOURCE_DIR}/Assets/Meshes/*.fbx")
# file(GLOB MODEL_VIEWER_GLTF_MESHES "${CMAKE_SOURCE_DIR}/Assets/Meshes/*.gltf")
# file(GLOB MODEL_VIEWER_GLB_MESHES "${CMAKE_SOURCE_DIR}/Assets/Meshes/*.glb")

# foreach(MODEL_VIEWER_FBX_MESH ${MODEL_VIEWER_FBX_MESHES})
# 	string(REGEX REPLACE "[.]fbx$" ".glb" MODEL_VIEWER_GLB_MESH ${MODEL_VIEWER_FBX_MESH})
# 	add_custom_command(OUTPUT ${MODEL_VIEWER_GLB_MESH}
# 		COMMAND ${FBX2GLTF_BIN} -b -i ${MODEL_VIEWER_FBX_MESH} -o ${MODEL_VIEWER_GLB_MESH}
# 		DEPENDS ${MODEL_VIEWER_FBX_MESH}
# 	)
# 	list(APPEND MODEL_VIEWER_GLB_MESHES ${MODEL_VIEWER_GLB_MESH})
# endforeach()

# foreach(MODEL_VIEWER_GLTF_MESH ${MODEL_VIEWER_GLTF_MESHES} ${MODEL_VIEWER_GLB_MESHES})
# 	string(REGEX REPLACE "[.](gltf|glb)$" ".bin" MODEL_VIEWER_TF_MESH ${MODEL_VIEWER_GLTF_MESH})
#
# 	add_custom_command(OUTPUT ${MODEL_VIEWER_TF_MESH}
# 		COMMAND ${ASSETPIPELINE_CMD} -pgltf --input-file ${MODEL_VIEWER_GLTF_MESH}
# 		DEPENDS ${MODEL_VIEWER_GLTF_MESH}
# 	)
# 	list(APPEND MODEL_VIEWER_TF_MESHES ${MODEL_VIEWER_TF_MESH})
# endforeach()

add_custom_target(ModelViewerAssets
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/PathStatement.txt" "${MODELVIEWER_RESOURCES_DIR}/PathStatement.txt"
	COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/gpu.cfg" "${MODELVIEWER_RESOURCES_DIR}/gpu.cfg"
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/Assets" "${MODELVIEWER_RESOURCES_DIR}/Assets"
)

add_dependencies(ModelViewer ModelViewerAssets)

set(VS_STARTUP_PROJECT ModelViewer)

tf_add_shader(ModelViewer "${CMAKE_SOURCE_DIR}/src/shaders/ShaderList.fsl")
tf_add_forge_utils(ModelViewer)

