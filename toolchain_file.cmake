# set toolchain
set(TOOLCHAIN_DIR ${PLATFORM_PATH}/tools/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-arm-none-eabi/bin)
set(TOOLCHAIN_PRE "arm-none-eabi-")

set(CMAKE_SYSTEM_NAME              Generic)
set(CMAKE_SYSTEM_PROCESSOR         ARM)

set(CMAKE_C_COMPILER               "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc")
set(CMAKE_CXX_COMPILER             "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++")
set(CMAKE_ASM_COMPILER             "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc")
set(CMAKE_RANLIB                   "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ranlib")
set(CMAKE_AR                       "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar")
set(CMAKE_SIZE                     "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}size")

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
