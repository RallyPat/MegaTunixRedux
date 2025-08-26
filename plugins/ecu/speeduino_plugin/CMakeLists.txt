cmake_minimum_required(VERSION 3.16)
project(speeduino_plugin)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# JSON support temporarily disabled
# pkg_check_modules(JSONCPP jsoncpp)

# Plugin source files
set(PLUGIN_SOURCES
    speeduino_plugin.cpp
)

# Create shared library
add_library(speeduino_plugin SHARED ${PLUGIN_SOURCES})

# Set output directory
set_target_properties(speeduino_plugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/ecu/
    OUTPUT_NAME "speeduino_plugin"
)

# Include directories
target_include_directories(speeduino_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Link libraries
target_link_libraries(speeduino_plugin
    Threads::Threads
)

# Compiler flags
target_compile_options(speeduino_plugin PRIVATE
    -Wall
    -Wextra
    -fPIC
    -O2
)

# Copy plugin to build directory
add_custom_command(TARGET speeduino_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/plugins/ecu/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:speeduino_plugin> ${CMAKE_BINARY_DIR}/plugins/ecu/
)

# Install target
install(TARGETS speeduino_plugin
    LIBRARY DESTINATION lib/megatunix-redux/plugins/ecu/
)
