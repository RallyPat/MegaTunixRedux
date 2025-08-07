#!/bin/bash

# MegaTunix Redux Cross-Platform Build Script
# Linux-first development with cross-platform infrastructure ready

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to detect current platform
detect_platform() {
    case "$(uname -s)" in
        Linux*)     echo "linux";;
        Darwin*)    echo "macos";;
        CYGWIN*|MINGW32*|MSYS*|MINGW*) echo "windows";;
        *)          echo "unknown";;
    esac
}

# Function to install Linux dependencies
install_linux_dependencies() {
    print_status "Installing Linux dependencies..."
    if command_exists apt-get; then
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libsdl2-dev \
            libsdl2-ttf-dev \
            libgtest-dev \
            doxygen
    elif command_exists dnf; then
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            pkg-config \
            SDL2-devel \
            SDL2_ttf-devel \
            gtest-devel \
            doxygen
    elif command_exists pacman; then
        sudo pacman -S --needed \
            base-devel \
            cmake \
            pkg-config \
            sdl2 \
            sdl2_ttf \
            gtest \
            doxygen
    else
        print_warning "Unknown package manager. Please install dependencies manually."
    fi
}

# Function to build for Linux development
build_linux_dev() {
    local build_type=${1:-Release}
    
    print_status "Building for Linux development ($build_type)..."
    
    # Create build directory
    local build_dir="build_linux"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure with CMake (Linux development mode)
    cmake .. -DCMAKE_BUILD_TYPE=$build_type
    
    # Build
    if command_exists ninja; then
        ninja
    else
        make -j$(nproc)
    fi
    
    # Run tests
    if [ -f "megatunix-redux-tests" ]; then
        print_status "Running tests..."
        ./megatunix-redux-tests
    fi
    
    # Test the application
    if [ -f "megatunix-redux" ]; then
        print_status "Testing application..."
        timeout 5s ./megatunix-redux --demo-mode || true
    fi
    
    cd ..
    print_success "Linux development build completed"
}

# Function to build for cross-platform (when needed)
build_cross_platform() {
    local platform=$1
    local build_type=${2:-Release}
    
    print_status "Building for $platform ($build_type)..."
    
    # Create build directory
    local build_dir="build_${platform}"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Configure with CMake (cross-platform mode)
    cmake .. -DCMAKE_BUILD_TYPE=$build_type -DCROSS_PLATFORM_BUILD=ON
    
    # Build
    if command_exists ninja; then
        ninja
    else
        make -j$(nproc)
    fi
    
    # Create package
    print_status "Creating package..."
    cpack
    
    cd ..
    print_success "Cross-platform build completed for $platform"
}

# Function to build all platforms (development focus)
build_all() {
    local current_platform=$(detect_platform)
    
    print_status "Current platform: $current_platform"
    
    if [ "$current_platform" = "linux" ]; then
        # Linux development mode
        install_linux_dependencies
        build_linux_dev
    else
        # Cross-platform mode for other platforms
        print_warning "Cross-platform build mode - minimal configuration"
        build_cross_platform "$current_platform"
    fi
}

# Function to clean build directories
clean_builds() {
    print_status "Cleaning build directories..."
    rm -rf build_linux build_windows build_macos build
    print_success "Build directories cleaned"
}

# Function to show help
show_help() {
    echo "MegaTunix Redux Build Script - Linux-First Development"
    echo ""
    echo "Usage: $0 [OPTION]"
    echo ""
    echo "Development Options (Linux-focused):"
    echo "  --dev         Build for Linux development (default)"
    echo "  --debug       Build for Linux development with debug"
    echo "  --clean       Clean build directories"
    echo "  --test        Build and run tests"
    echo ""
    echo "Cross-Platform Options (when needed):"
    echo "  --linux       Build for Linux (development mode)"
    echo "  --windows     Build for Windows (cross-platform mode)"
    echo "  --macos       Build for macOS (cross-platform mode)"
    echo "  --all         Build for current platform"
    echo ""
    echo "Examples:"
    echo "  $0 --dev          # Linux development build"
    echo "  $0 --debug        # Linux debug build"
    echo "  $0 --test         # Build and test"
    echo "  $0 --clean        # Clean builds"
    echo ""
    echo "Note: Development focuses on Linux for faster iteration."
    echo "Cross-platform builds are available when needed."
}

# Function to run tests
run_tests() {
    print_status "Building and running tests..."
    
    # Clean previous build
    rm -rf build_test
    mkdir -p build_test
    cd build_test
    
    # Configure and build
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    make -j$(nproc)
    
    # Run tests
    if [ -f "megatunix-redux-tests" ]; then
        print_status "Running unit tests..."
        ./megatunix-redux-tests
    fi
    
    # Test application
    if [ -f "megatunix-redux" ]; then
        print_status "Testing application..."
        timeout 5s ./megatunix-redux --demo-mode || true
    fi
    
    cd ..
    print_success "Tests completed"
}

# Main script logic
main() {
    if [ $# -eq 0 ]; then
        # Default to Linux development
        install_linux_dependencies
        build_linux_dev
        return
    fi
    
    while [ $# -gt 0 ]; do
        case $1 in
            --dev)
                install_linux_dependencies
                build_linux_dev
                ;;
            --debug)
                install_linux_dependencies
                build_linux_dev Debug
                ;;
            --test)
                run_tests
                ;;
            --linux)
                install_linux_dependencies
                build_linux_dev
                ;;
            --windows)
                print_warning "Cross-platform Windows build - minimal configuration"
                build_cross_platform "windows"
                ;;
            --macos)
                print_warning "Cross-platform macOS build - minimal configuration"
                build_cross_platform "macos"
                ;;
            --all)
                build_all
                ;;
            --clean)
                clean_builds
                ;;
            --help)
                show_help
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
        shift
    done
}

# Run main function with all arguments
main "$@" 