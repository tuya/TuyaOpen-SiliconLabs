if(CMAKE_HOST_APPLE)
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
        set(TOOLCHAIN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../tools/arm-gnu-toolchain-12.2.rel1-darwin-arm64-arm-none-eabi")
    else()
        set(TOOLCHAIN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../tools/arm-gnu-toolchain-12.2.rel1-darwin-x86_64-arm-none-eabi")
    endif()
else()
    set(TOOLCHAIN_ROOT "${CMAKE_CURRENT_LIST_DIR}/../tools/gcc-arm-none-eabi-10.3-2021.10")
endif()

if(NOT EXISTS "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-gcc"
   AND NOT EXISTS "${TOOLCHAIN_ROOT}/bin/arm-none-eabi-gcc.exe")
    message(FATAL_ERROR
        "Toolchain not found under ${TOOLCHAIN_ROOT}.\n"
        "Install it with: ./script/bootstrap arm_toolchain")
endif()

set(TOOLCHAIN_DIR ${TOOLCHAIN_ROOT}/bin)
set(TOOLCHAIN_PRE "arm-none-eabi-")

if(CMAKE_HOST_WIN32)
    set(TOOLCHAIN_EXT ".exe")
else()
    set(TOOLCHAIN_EXT "")
endif()

set(CMAKE_SYSTEM_NAME              Generic)
set(CMAKE_SYSTEM_PROCESSOR         ARM)

set(CMAKE_C_COMPILER               "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc${TOOLCHAIN_EXT}")
set(CMAKE_CXX_COMPILER             "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++${TOOLCHAIN_EXT}")
set(CMAKE_ASM_COMPILER             "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc${TOOLCHAIN_EXT}")
set(CMAKE_RANLIB                   "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ranlib${TOOLCHAIN_EXT}")
set(CMAKE_AR                       "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar${TOOLCHAIN_EXT}")
set(CMAKE_SIZE                     "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}size${TOOLCHAIN_EXT}")

execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE COMPILER_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)

set(COMMON_C_FLAGS                 "-mcpu=cortex-m4 -mthumb -fmessage-length=0 -ffunction-sections -fdata-sections -mfpu=fpv4-sp-d16 -mfloat-abi=softfp")

set(CMAKE_C_FLAGS_INIT             "${COMMON_C_FLAGS} -std=c18")
set(CMAKE_CXX_FLAGS_INIT           "${COMMON_C_FLAGS} -std=c++17 -fno-exceptions -fno-rtti")
set(CMAKE_ASM_FLAGS_INIT           "${CMAKE_C_FLAGS_INIT} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT    "${COMMON_C_FLAGS} -specs=nosys.specs")

set(CMAKE_C_FLAGS_DEBUG            "-Og -g")
set(CMAKE_CXX_FLAGS_DEBUG          "-Og -g")
set(CMAKE_ASM_FLAGS_DEBUG          "-g")

set(CMAKE_C_FLAGS_RELEASE          "-Os")
set(CMAKE_CXX_FLAGS_RELEASE        "-Os")
set(CMAKE_ASM_FLAGS_RELEASE        "")
