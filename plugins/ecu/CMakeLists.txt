# Test Plugin - MegaTunix Redux
# CMakeLists.txt for building the test plugin

cmake_minimum_required(VERSION 3.20)
project(TestPlugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin source files
set(PLUGIN_SOURCES
    test_plugin.cpp
)

# Create shared library for the plugin
add_library(test_plugin SHARED ${PLUGIN_SOURCES})

# Set output name and properties
set_target_properties(test_plugin PROPERTIES
    OUTPUT_NAME "test_plugin"
    PREFIX ""
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/ecu"
)

# Include directories
target_include_directories(test_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/plugin
)

# Compiler flags
target_compile_options(test_plugin PRIVATE
    -Wall
    -Wextra
    -fPIC
)

# Link libraries
target_link_libraries(test_plugin
    dl
)

# Copy plugin to build directory for testing
add_custom_command(TARGET test_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/plugins/ecu
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test_plugin> ${CMAKE_BINARY_DIR}/plugins/ecu/
)
