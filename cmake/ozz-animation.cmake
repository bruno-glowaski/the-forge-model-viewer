set(OZZ_DIR ${CMAKE_SOURCE_DIR}/Vendor/TheForge/Common_3/Resources/AnimationSystem/ThirdParty/OpenSource/ozz-animation/src/)

set(OZZ_BASE_SRC
    # cmake-format: sortable
    ${OZZ_DIR}base/containers/string_archive.cc
    ${OZZ_DIR}base/io/archive.cc
    ${OZZ_DIR}base/maths/math_archive.cc
    ${OZZ_DIR}base/maths/simd_math_archive.cc
    ${OZZ_DIR}base/maths/soa_math_archive.cc
    ${OZZ_DIR}base/memory/allocator.cc
    ${OZZ_DIR}base/platform.cc
)

add_library(ozz_base STATIC ${OZZ_BASE_SRC})
target_include_directories(ozz_base PUBLIC $<$<PLATFORM_ID:Android>:${ANDROID_NDK}/sources/android/native_app_glue/>)

set(OZZ_ANIMATION_SRC
    # cmake-format: sortable
    ${OZZ_DIR}animation/runtime/animation.cc
    ${OZZ_DIR}animation/runtime/blending_job.cc
    ${OZZ_DIR}animation/runtime/ik_aim_job.cc
    ${OZZ_DIR}animation/runtime/ik_two_bone_job.cc
    ${OZZ_DIR}animation/runtime/local_to_model_job.cc
    ${OZZ_DIR}animation/runtime/sampling_job.cc
    ${OZZ_DIR}animation/runtime/skeleton.cc
    ${OZZ_DIR}animation/runtime/skeleton_utils.cc
    ${OZZ_DIR}animation/runtime/track.cc
    ${OZZ_DIR}animation/runtime/track_sampling_job.cc
    ${OZZ_DIR}animation/runtime/track_triggering_job.cc
)

add_library(ozz_animation STATIC ${OZZ_ANIMATION_SRC})
target_link_libraries(ozz_animation ozz_base)

set(OZZ_ANIMATION_OFFLINE_SRC
    # cmake-format: sortable
    ${OZZ_DIR}animation/offline/additive_animation_builder.cc
    ${OZZ_DIR}animation/offline/animation_builder.cc
    ${OZZ_DIR}animation/offline/animation_optimizer.cc
    ${OZZ_DIR}animation/offline/raw_animation.cc
    ${OZZ_DIR}animation/offline/raw_animation_archive.cc
    ${OZZ_DIR}animation/offline/raw_animation_utils.cc
    ${OZZ_DIR}animation/offline/raw_skeleton.cc
    ${OZZ_DIR}animation/offline/raw_skeleton_archive.cc
    ${OZZ_DIR}animation/offline/raw_track.cc
    ${OZZ_DIR}animation/offline/skeleton_builder.cc
    ${OZZ_DIR}animation/offline/track_builder.cc
    ${OZZ_DIR}animation/offline/track_optimizer.cc
)

add_library(ozz_animation_offline STATIC ${OZZ_ANIMATION_OFFLINE_SRC})
target_include_directories(
    ozz_animation_offline PUBLIC ${OZZ_DIR}src $<$<PLATFORM_ID:Android>:${ANDROID_NDK}/sources/android/native_app_glue/>
)
