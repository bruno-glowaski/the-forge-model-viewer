# TODO (maybe): add ARM support
# TODO: keep intermidiate artifacts in build dir

set(ISPC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/TheForge/Common_3/Tools/ThirdParty/OpenSource/ISPCTextureCompressor)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	set(ISPC_CMD ${ISPC_DIR}/ISPC/linux/ispc)
elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	set(ISPC_CMD ${ISPC_DIR}/ISPC/win/ispc.exe)
else()
	message(FATAL_ERROR "Unsupported platform")
endif()
set(ISPC_ARCH x86-64)
set(ISPC_TARGETS sse2,avx)
set(ISPC_EXTS sse2 avx)
set(ISPC_ARCH_CXXFLAGS -msse2)

# OBJS = ispc_texcomp/kernel_astc_ispc.o \
# $(foreach target,$(ISPC_OBJS),ispc_texcomp/kernel_astc_ispc_$(target).o ) \
# ispc_texcomp/kernel_ispc.o \
# $(foreach target,$(ISPC_OBJS),ispc_texcomp/kernel_ispc_$(target).o ) \
# ispc_texcomp/ispc_texcomp_astc.o \
# ispc_texcomp/ispc_texcomp.o

set(ISPC_FLAGS -O2 --arch=${ISPC_ARCH} --target=${ISPC_TARGETS} --opt=fast-math --pic)

set(ISPC_CXX_SRC ${ISPC_DIR}/ispc_texcomp/ispc_texcomp.cpp ${ISPC_DIR}/ispc_texcomp/ispc_texcomp_astc.cpp)
set(ISPC_ISPC_SRC ${ISPC_DIR}/ispc_texcomp/kernel.ispc ${ISPC_DIR}/ispc_texcomp/kernel_astc.ispc)

foreach(ISPC_EXT ${ISPC_EXTS})
	foreach(ISPC_SRC_FILE ${ISPC_ISPC_SRC})
		string(REGEX REPLACE "[.]ispc$" "_ispc_${ISPC_EXT}.o" ISPC_OBJ_FILE ${ISPC_SRC_FILE})
		string(REGEX REPLACE "[.]ispc$" "_ispc_${ISPC_EXT}.h" ISPC_H_FILE ${ISPC_SRC_FILE})
		add_custom_command(
			OUTPUT ${ISPC_OBJ_FILE} ${ISPC_H_FILE}
			COMMAND ${ISPC_CMD} ${ISPC_FLAGS} -o ${ISPC_OBJ_FILE} -h ${ISPC_H_FILE} ${ISPC_SRC_FILE}
			DEPENDS ${ISPC_SRC_FILE}
			VERBATIM
		)
		list(APPEND ISPC_ISPC_OBJ ${ISPC_OBJ_FILE})
		list(APPEND ISPC_ISPC_H ${ISPC_H_FILE})
	endforeach()
endforeach()
foreach(ISPC_SRC_FILE ${ISPC_ISPC_SRC})
	string(REGEX REPLACE "[.]ispc$" "_ispc.o" ISPC_OBJ_FILE ${ISPC_SRC_FILE})
	string(REGEX REPLACE "[.]ispc$" "_ispc.h" ISPC_H_FILE ${ISPC_SRC_FILE})
	add_custom_command(
		OUTPUT ${ISPC_OBJ_FILE} ${ISPC_H_FILE}
		COMMAND ${ISPC_CMD} ${ISPC_FLAGS} -o ${ISPC_OBJ_FILE} -h ${ISPC_H_FILE} ${ISPC_SRC_FILE}
		DEPENDS ${ISPC_SRC_FILE}
		VERBATIM
	)
	list(APPEND ISPC_ISPC_OBJ ${ISPC_OBJ_FILE})
	list(APPEND ISPC_ISPC_H ${ISPC_H_FILE})
endforeach()
add_custom_target(ISPCHeaders DEPENDS ${ISPC_ISPC_H})

add_library(ISPCTextureCompressor ${ISPC_ISPC_OBJ} ${ISPC_CXX_SRC})
add_dependencies(ISPCTextureCompressor ISPCHeaders)
target_include_directories(ISPCTextureCompressor PRIVATE "${ISPC_DIR}/ispc_texcomp")
