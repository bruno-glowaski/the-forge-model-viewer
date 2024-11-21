set(SPIRV_DIR ${CMAKE_SOURCE_DIR}/Vendor/TheForge/Common_3/Graphics/ThirdParty/OpenSource/SPIRV_Cross/)

set(SPIRVTOOLS_SRC
    ${SPIRV_DIR}spirv_cfg.cpp
    ${SPIRV_DIR}spirv_cpp.cpp
    ${SPIRV_DIR}spirv_cross.cpp
    ${SPIRV_DIR}spirv_cross_parsed_ir.cpp
    ${SPIRV_DIR}spirv_cross_util.cpp
    ${SPIRV_DIR}spirv_glsl.cpp
    ${SPIRV_DIR}spirv_hlsl.cpp
    ${SPIRV_DIR}spirv_msl.cpp
    ${SPIRV_DIR}spirv_parser.cpp
    ${SPIRV_DIR}spirv_reflect.cpp
    ${SPIRV_DIR}SpirvTools.cpp
)

add_library(SpirVTools STATIC ${SPIRVTOOLS_SRC})

target_include_directories(SpirVTools PUBLIC $<$<PLATFORM_ID:Android>:${ANDROID_NDK}/sources/android/native_app_glue/>)
