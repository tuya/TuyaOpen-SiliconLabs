list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuyaos_adapter)
include(${TOP_SOURCE_DIR}/boards/SiWx917/common/CMakeLists.txt)

set(PLATFORM_PUBINC_2
    ${BOARD_INC})

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})

# Add global definition for SLI_SI917B0 macro
add_definitions(-DSLI_SI917B0)

set(CMAKE_BUILD_TYPE Release)

# WARNING: Changing CMAKE_BUILD_TYPE from Release to Debug may significantly impact
# performance on the SiWx917 platform, potentially causing voice quality degradation,
# audio dropouts, or real-time processing issues. Use Debug builds only for
# development purposes and ensure thorough testing of audio/voice functionality.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "DEBUG BUILD DETECTED: SiWx917 platform performance may be insufficient "
                    "for optimal voice quality. Audio dropouts and processing delays may occur. "
                    "Consider using Release build for voice-enabled applications.")
endif()

# Use auto-generated include directories from SLC
# This file is created by build_setup.py after SLC generation with full absolute paths
set(SLC_INCLUDES_FILE "${CMAKE_CURRENT_BINARY_DIR}/slc_includes.cmake")

if(EXISTS "${SLC_INCLUDES_FILE}")
    include("${SLC_INCLUDES_FILE}")
    message(STATUS "[Platform] Using auto-generated SLC includes (${SLC_INCLUDE_DIRS})")
    set(HW_CRYPTO_INCLUDES ${SLC_INCLUDE_DIRS})
else()
    message(WARNING "[Platform] Auto-generated includes not found: ${SLC_INCLUDES_FILE}")
    set(HW_CRYPTO_INCLUDES "")
endif()

# Hardware crypto interface library
add_library(si91x_sdk_includes INTERFACE)
target_include_directories(si91x_sdk_includes INTERFACE ${HW_CRYPTO_INCLUDES})

target_compile_definitions(si91x_sdk_includes
    INTERFACE
        USE_SI91X_HW_CRYPTO=1
)

# Export to global scope (no PARENT_SCOPE at root level)
set(PLATFORM_SI91X_SDK_INCLUCES si91x_sdk_includes CACHE INTERNAL "Platform Si91x SDK includes")