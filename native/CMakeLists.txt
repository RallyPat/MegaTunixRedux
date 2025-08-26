cmake_minimum_required(VERSION 3.16)
project(megatunix_native VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C standard  
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build shared library for Flutter FFI
set(BUILD_SHARED_LIBS ON)

# Compiler flags for optimization
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -fPIC")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -fPIC") 
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG -fPIC")

# Include directories
include_directories(include)

# Source files for ECU communication
set(ECU_SOURCES
    ecu_communication/speeduino_protocol.c
    ecu_communication/ecu_base.c
)

# Source files for data bridge
set(BRIDGE_SOURCES
    data_bridge/stream_manager.cpp
    data_bridge/bridge_ffi.cpp
)

# Create the native library
add_library(megatunix_native SHARED
    ${ECU_SOURCES}
    ${BRIDGE_SOURCES}
)

# Link libraries (add as needed)
target_link_libraries(megatunix_native
    pthread
    m
)

# Install configuration
install(TARGETS megatunix_native
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)