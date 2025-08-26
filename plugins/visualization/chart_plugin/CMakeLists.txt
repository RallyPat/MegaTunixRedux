cmake_minimum_required(VERSION 3.16)
project(chart_plugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Plugin source files
set(PLUGIN_SOURCES
    chart_plugin.cpp
)

# Create shared library
add_library(chart_plugin SHARED ${PLUGIN_SOURCES})

# Set output directory
set_target_properties(chart_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/visualization/
    OUTPUT_NAME "chart_plugin"
)

# Include directories
target_include_directories(chart_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(chart_plugin
    Threads::Threads
)

# Compiler flags
target_compile_options(chart_plugin PRIVATE
    -Wall
    -Wextra
    -fPIC
    -O2
)

# Copy plugin to build directory
add_custom_command(TARGET chart_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/plugins/visualization/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:chart_plugin> ${CMAKE_BINARY_DIR}/plugins/visualization/
)

# Install target
install(TARGETS chart_plugin
    LIBRARY DESTINATION lib/megatunix-redux/plugins/visualization/
)
