# We need at least 3.12 for list(JOIN....), add_compile_definitions
# 3.15 for CMAKE_CXX_COMPILER_FRONTEND_VARIANT
cmake_minimum_required(VERSION 3.15)

project(TheForgeCMake)

project(The-Forge LANGUAGES CXX C ASM)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_UNIT_TESTS "Build The-Forge's unit tests" ON)
option(BUILD_ASSETPIPELINECMD "Build The-Forge's asset pipeline command" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(TheForge)
include(TheForgeUtils)
include(SpirVTools)

// Example of how to set up forge project

add_executable(01_Transformations
    "Vendor/TheForge/Examples_3/Unit_Tests/src/01_Transformations/01_Transformations.cpp"
)

set_target_properties(01_Transformations PROPERTIES LINK_FLAGS /MACHINE:X64)
target_include_directories(01_Transformations PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Vendor/TheForge"
	"${CMAKE_CURRENT_LIST_DIR}/Vendor/TheForge/Common_3" 
	"${CMAKE_CURRENT_LIST_DIR}/Vendor/TheForge/Tools"
)
target_link_libraries(01_Transformations PRIVATE Xinput9_1_0 ws2_32 OS Renderer)
set(VS_STARTUP_PROJECT 01_Transformations)

tf_add_shader(01_Transformations ${CMAKE_SOURCE_DIR}/Vendor/TheForge/Examples_3/Unit_Tests/src/01_Transformations/Shaders/FSL/ShaderList.fsl)
tf_add_forge_utils(01_Transformations)

