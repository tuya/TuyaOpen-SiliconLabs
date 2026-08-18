# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	tuyaopen_switch_demo
	VERSION 1.0
	LANGUAGES C CXX ASM
)

# Include the definition of the slc_tuyaopen_switch_demo target,
# which contains the content of the SLC project
include(tuyaopen_switch_demo.cmake)

add_executable(tuyaopen_switch_demo
    # Add additional sources here
)

target_include_directories(tuyaopen_switch_demo PUBLIC
    # Add additional include paths here
)

target_compile_definitions(tuyaopen_switch_demo PUBLIC
    # Add additional macros here
)

target_compile_options(tuyaopen_switch_demo PUBLIC
    # Set additional compiler flags here
)

target_link_options(tuyaopen_switch_demo PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(tuyaopen_switch_demo PRIVATE
    slc_tuyaopen_switch_demo
)

# Include managed project content if available
include(tuyaopen_switch_demo_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(tuyaopen_switch_demo PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET tuyaopen_switch_demo
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:tuyaopen_switch_demo>" "$<TARGET_FILE_DIR:tuyaopen_switch_demo>/$<TARGET_FILE_BASE_NAME:tuyaopen_switch_demo>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:tuyaopen_switch_demo>" "$<TARGET_FILE_DIR:tuyaopen_switch_demo>/$<TARGET_FILE_BASE_NAME:tuyaopen_switch_demo>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:tuyaopen_switch_demo>" "$<TARGET_FILE_DIR:tuyaopen_switch_demo>/$<TARGET_FILE_BASE_NAME:tuyaopen_switch_demo>.bin" 
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET tuyaopen_switch_demo
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()