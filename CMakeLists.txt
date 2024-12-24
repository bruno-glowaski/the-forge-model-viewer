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

file(COPY Assets DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY PathStatement.txt DESTINATION "${CMAKE_BINARY_DIR}")

target_include_directories(ModelViewer PRIVATE 
  "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge"
	"${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Common_3" 
	"${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Tools"
)

set(MODEL_VIEWER_LIBS OS Renderer)
if (LINUX)
	list(APPEND MODEL_VIEWER_LIBS udev)
endif()

target_link_libraries(ModelViewer PRIVATE ${MODEL_VIEWER_LIBS})
set(VS_STARTUP_PROJECT ModelViewer)

tf_add_shader(ModelViewer "${CMAKE_SOURCE_DIR}/src/shaders/ShaderList.fsl")
tf_add_forge_utils(ModelViewer)

