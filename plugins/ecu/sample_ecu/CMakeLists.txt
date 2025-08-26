# Sample ECU Plugin - MegaTunix Redux
# CMakeLists.txt for building the sample ECU plugin

cmake_minimum_required(VERSION 3.20)
project(SampleECUPlugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Plugin source files
set(PLUGIN_SOURCES
    sample_ecu.cpp
)

# Create shared library for the plugin
add_library(sample_ecu_plugin SHARED ${PLUGIN_SOURCES})

# Set output name and properties
set_target_properties(sample_ecu_plugin PROPERTIES
    OUTPUT_NAME "sample_ecu"
    PREFIX ""
    SUFFIX ".so"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/ecu"
)

# Include directories
target_include_directories(sample_ecu_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/plugin
)

# Compiler flags
target_compile_options(sample_ecu_plugin PRIVATE
    -Wall
    -Wextra
    -fPIC
)

# Link libraries
target_link_libraries(sample_ecu_plugin
    dl
)

# Install target
install(TARGETS sample_ecu_plugin
    LIBRARY DESTINATION lib/megatunix-redux/plugins/ecu
)

# Copy plugin to build directory for testing
add_custom_command(TARGET sample_ecu_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/plugins/ecu
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sample_ecu_plugin> ${CMAKE_BINARY_DIR}/plugins/ecu/
)
