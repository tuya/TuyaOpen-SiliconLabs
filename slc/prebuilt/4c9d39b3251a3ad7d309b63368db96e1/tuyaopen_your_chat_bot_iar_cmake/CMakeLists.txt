# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	tuyaopen_your_chat_bot
	VERSION 1.0
	LANGUAGES C CXX ASM
)

# Include the definition of the slc_tuyaopen_your_chat_bot target,
# which contains the content of the SLC project
include(tuyaopen_your_chat_bot.cmake)

add_executable(tuyaopen_your_chat_bot
    # Add additional sources here
)

target_include_directories(tuyaopen_your_chat_bot PUBLIC
    # Add additional include paths here
)

target_compile_definitions(tuyaopen_your_chat_bot PUBLIC
    # Add additional macros here
)

target_compile_options(tuyaopen_your_chat_bot PUBLIC
    # Set additional compiler flags here
)

target_link_options(tuyaopen_your_chat_bot PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(tuyaopen_your_chat_bot PRIVATE
    slc_tuyaopen_your_chat_bot
)

# Include managed project content if available
include(tuyaopen_your_chat_bot_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(tuyaopen_your_chat_bot PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET tuyaopen_your_chat_bot
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:tuyaopen_your_chat_bot>" "$<TARGET_FILE_DIR:tuyaopen_your_chat_bot>/$<TARGET_FILE_BASE_NAME:tuyaopen_your_chat_bot>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:tuyaopen_your_chat_bot>" "$<TARGET_FILE_DIR:tuyaopen_your_chat_bot>/$<TARGET_FILE_BASE_NAME:tuyaopen_your_chat_bot>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:tuyaopen_your_chat_bot>" "$<TARGET_FILE_DIR:tuyaopen_your_chat_bot>/$<TARGET_FILE_BASE_NAME:tuyaopen_your_chat_bot>.bin" 
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET tuyaopen_your_chat_bot
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()