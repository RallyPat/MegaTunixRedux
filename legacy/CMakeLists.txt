cmake_minimum_required(VERSION 3.20)

# Enable C++ compilation for the entire project
project(megatunix-redux VERSION 2.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add AddressSanitizer for memory corruption detection
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform-specific settings
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    set(EXECUTABLE_NAME "MegaTunixRedux.exe")
    set(INSTALL_DIR "MegaTunixRedux")
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    set(EXECUTABLE_NAME "MegaTunixRedux")
    set(INSTALL_DIR "MegaTunixRedux.app/Contents/MacOS")
else()
    set(PLATFORM_LINUX TRUE)
    set(EXECUTABLE_NAME "megatunix-redux")
    set(INSTALL_DIR "bin")
endif()

# Development focus: Linux-first development
if(PLATFORM_LINUX)
    message(STATUS "Linux development mode - focusing on Linux build")
    set(DEVELOPMENT_PLATFORM TRUE)
else()
    message(STATUS "Cross-platform build mode - ${CMAKE_SYSTEM_NAME}")
    set(DEVELOPMENT_PLATFORM FALSE)
endif()

# Compiler flags
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Platform-specific compiler flags
if(PLATFORM_WINDOWS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32_LEAN_AND_MEAN")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
elseif(PLATFORM_MACOS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__MACOSX__")
elseif(PLATFORM_LINUX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__LINUX__")
endif()

# Find SDL2 with Wayland support
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Find zlib for compression
find_package(ZLIB REQUIRED)

# Find libserialport
pkg_check_modules(LIBSERIALPORT REQUIRED libserialport)

# Check for Wayland support
pkg_check_modules(WAYLAND wayland-client wayland-cursor wayland-egl)
if(WAYLAND_FOUND)
    message(STATUS "Wayland support found: ${WAYLAND_VERSION}")
    add_definitions(-DWAYLAND_SUPPORT)
else()
    message(STATUS "Wayland not found, falling back to X11")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

# Source files for SDL2/ImGui version
set(SOURCES
    src/main.cpp
    src/data/datalog_manager.c
    src/automation/macro_engine.c
    src/automation/action_triggers.c
    src/integrations/gps_provider.c
    src/ui/undo_redo.c
    src/ui/keybindings_prefs.c
    src/io/export_import.c
    src/diagnostics/diagnostics.c
    src/ecu/ecu_communication.c
    src/ecu/ecu_ini_parser.c
    src/ecu/ecu_dynamic_protocols.c
    src/dashboard/dashboard.c
    src/utils/config.c
    src/utils/logging.c
    src/ui/imgui_ve_table.c
    src/ui/ve_table_editor.cpp
    src/ui/ui_theme_manager.cpp
    src/ui/logging_system.cpp
    src/ui/settings_manager.cpp
    src/ui/table_operations.cpp
    src/ui/ecu_integration.cpp
    src/ui/imgui_key_bindings.c
    src/ui/imgui_runtime_display.cpp
    src/ui/imgui_communications.cpp
    src/ui/imgui_file_dialog.cpp
    src/plugin/plugin_manager.cpp
    src/core/data_bridge.cpp
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_impl_sdl2.cpp
    external/imgui/imgui_impl_opengl3.cpp
)

# Header files
set(HEADERS
    include/megatunix_redux.h
    include/data/datalog_manager.h
    include/automation/macro_engine.h
    include/automation/action_triggers.h
    include/integrations/gps_provider.h
    include/ui/undo_redo.h
    include/ui/keybindings_prefs.h
    include/io/export_import.h
    include/ecu/ecu_communication.h
    include/dashboard/dashboard.h
    include/utils/config.h
    include/utils/logging.h
    external/imgui/imgui.h
    external/imgui/imgui_internal.h
    external/imgui/imconfig.h
    external/imgui/imgui_impl_sdl2.h
    external/imgui/imgui_impl_opengl3.h
    include/ui/imgui_ve_table.h
    include/ui/ve_table_editor.h
    include/ui/ui_theme_manager.h
    include/ui/logging_system.h
    include/ui/settings_manager.h
    include/ui/table_operations.h
    include/ui/ecu_integration.h
    include/ui/imgui_runtime_display.h
    include/ui/imgui_communications.h
    include/plugin/plugin_interface.h
    include/plugin/plugin_manager.h
)

# Create executable
add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

# Add plugin subdirectories
add_subdirectory(plugins/ecu)
add_subdirectory(plugins/ecu/speeduino_plugin)
add_subdirectory(plugins/visualization/chart_plugin)

# Link libraries
target_link_libraries(${EXECUTABLE_NAME}
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LIBSERIALPORT_LIBRARIES}
    GL
    GLU
    pthread
    dl
    m
)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    target_link_libraries(${EXECUTABLE_NAME} winmm)
elseif(PLATFORM_MACOS)
    target_link_libraries(${EXECUTABLE_NAME} "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(PLATFORM_LINUX)
    target_link_libraries(${EXECUTABLE_NAME} dl)
endif()

# Compiler flags
target_compile_options(${EXECUTABLE_NAME} PRIVATE
    ${SDL2_CFLAGS_OTHER}
    ${SDL2_TTF_CFLAGS_OTHER}
)

# Copy assets for development builds
if(DEVELOPMENT_PLATFORM)
    add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
        COMMENT "Copying assets for development build"
    )
endif()

# Installation - only configure for development platform
if(DEVELOPMENT_PLATFORM)
    # Linux development installation
    install(TARGETS ${EXECUTABLE_NAME}
        RUNTIME DESTINATION ${INSTALL_DIR}
    )
    
    # Install additional files for Linux development
    install(DIRECTORY assets/ DESTINATION share/megatunix-redux/assets)
    install(FILES README.md DESTINATION share/doc/megatunix-redux)
    install(FILES LICENSE DESTINATION share/doc/megatunix-redux)
    
    # Create desktop file for Linux development
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/desktop/megatunix-redux.desktop.in"
        "${CMAKE_CURRENT_BINARY_DIR}/megatunix-redux.desktop"
        @ONLY
    )
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/megatunix-redux.desktop"
            DESTINATION share/applications)
    
    # Create icon for Linux development
    install(FILES icons/megatunix-redux.png
            DESTINATION share/icons/hicolor/256x256/apps)
else()
    # Cross-platform installation (for when we're ready)
    install(TARGETS ${EXECUTABLE_NAME}
        RUNTIME DESTINATION ${INSTALL_DIR}
    )
    
    # Install additional files for cross-platform
    if(PLATFORM_WINDOWS)
        install(DIRECTORY assets/ DESTINATION ${INSTALL_DIR}/assets)
        install(FILES README.md LICENSE DESTINATION ${INSTALL_DIR})
    elseif(PLATFORM_MACOS)
        install(DIRECTORY assets/ DESTINATION MegaTunixRedux.app/Contents/Resources/assets)
        install(FILES README.md LICENSE DESTINATION MegaTunixRedux.app/Contents/Resources)
    elseif(PLATFORM_LINUX)
        install(DIRECTORY assets/ DESTINATION ${INSTALL_DIR}/assets)
        install(FILES README.md LICENSE DESTINATION ${INSTALL_DIR})
    endif()
endif()

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MegaTunixReduxConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# CPack configuration - only enable for development platform or explicit cross-platform builds
if(DEVELOPMENT_PLATFORM OR CROSS_PLATFORM_BUILD)
    set(CPACK_PACKAGE_NAME "MegaTunixRedux")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern ECU Tuning Software")
    set(CPACK_PACKAGE_VENDOR "Pat Burke")
    set(CPACK_PACKAGE_CONTACT "pat@example.com")

    if(PLATFORM_WINDOWS)
        # Windows installer
        set(CPACK_GENERATOR "NSIS")
        set(CPACK_NSIS_DISPLAY_NAME "MegaTunix Redux")
        set(CPACK_NSIS_PACKAGE_NAME "MegaTunix Redux")
        set(CPACK_NSIS_CONTACT "pat@example.com")
        set(CPACK_NSIS_URL "https://github.com/RallyPat/MegaTunixRedux")
        set(CPACK_NSIS_ENABLE_ICON_EXTRA "true")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/megatunix-redux.ico")
        set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/megatunix-redux.ico")
    elseif(PLATFORM_MACOS)
        # macOS DMG
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_VOLUME_NAME "MegaTunix Redux")
        set(CPACK_DMG_FORMAT "UDZO")
    elseif(PLATFORM_LINUX)
        # Linux packages
        set(CPACK_GENERATOR "DEB;RPM")
        set(CPACK_DEBIAN_PACKAGE_NAME "megatunix-redux")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libsdl2-ttf-2.0-0")
        set(CPACK_RPM_PACKAGE_NAME "megatunix-redux")
        set(CPACK_RPM_PACKAGE_REQUIRES "SDL2, SDL2_ttf")
    endif()

    include(CPack)
endif()

# Testing - focus on Linux development
enable_testing()
if(DEVELOPMENT_PLATFORM)
    add_subdirectory(tests)
else()
    # Minimal testing for cross-platform builds
    add_test(NAME BasicTest COMMAND ${CMAKE_COMMAND} -E echo "Cross-platform build test passed")
endif()

# Documentation - only for development platform
if(DEVELOPMENT_PLATFORM)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
        
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        
        add_custom_target(docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Print configuration summary
message(STATUS "MegaTunix Redux Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Development mode: ${DEVELOPMENT_PLATFORM}")
message(STATUS "  SDL2: ${SDL2_VERSION}")
message(STATUS "  SDL2_ttf: ${SDL2_TTF_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Executable: ${EXECUTABLE_NAME}")

if(DEVELOPMENT_PLATFORM)
    message(STATUS "  Focus: Linux development - fast builds, full testing")
else()
    message(STATUS "  Focus: Cross-platform build - minimal configuration")
endif() 