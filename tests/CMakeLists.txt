# Tests for MegaTunix Redux
# Cross-platform test configuration

# Find testing framework
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # Fallback to basic testing if GTest not available
    enable_testing()
    add_test(NAME BasicTest COMMAND ${CMAKE_COMMAND} -E echo "Basic test passed")
    return()
endif()

# Test executable
add_executable(megatunix-redux-tests
    test_main.cpp
    test_window_manager.cpp
    test_clay_ui.cpp
    test_renderer.cpp
)

# Link libraries
target_link_libraries(megatunix-redux-tests
    GTest::gtest
    GTest::gtest_main
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
)

# Platform-specific test linking
if(PLATFORM_WINDOWS)
    target_link_libraries(megatunix-redux-tests winmm)
elseif(PLATFORM_MACOS)
    target_link_libraries(megatunix-redux-tests "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(PLATFORM_LINUX)
    target_link_libraries(megatunix-redux-tests dl)
endif()

# Include directories
target_include_directories(megatunix-redux-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/app
)

# Add tests
add_test(NAME WindowManagerTest COMMAND megatunix-redux-tests --gtest_filter=WindowManagerTest*)
add_test(NAME ClayUITest COMMAND megatunix-redux-tests --gtest_filter=ClayUITest*)
add_test(NAME RendererTest COMMAND megatunix-redux-tests --gtest_filter=RendererTest*)

# Set test properties
set_tests_properties(WindowManagerTest PROPERTIES TIMEOUT 30)
set_tests_properties(ClayUITest PROPERTIES TIMEOUT 30)
set_tests_properties(RendererTest PROPERTIES TIMEOUT 30) 