#!/bin/bash

# Debug script for segmentation fault analysis
# This script helps identify the exact cause of segmentation faults

echo "=== MegaTunix Redux Segmentation Fault Debug Script ==="
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Create debug build directory
echo "1. Creating debug build with AddressSanitizer..."
mkdir -p build_debug
cd build_debug

# Configure with debug flags
echo "2. Configuring CMake with debug flags..."
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build the application
echo "3. Building application with AddressSanitizer..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Error: Build failed!"
    exit 1
fi

echo ""
echo "4. Running application with memory debugging..."
echo "   - AddressSanitizer is enabled"
echo "   - Memory corruption will be detected"
echo "   - Stack traces will be provided"
echo ""
echo "Instructions:"
echo "1. Navigate to the VE Table tab (tab 8)"
echo "2. Try clicking on table cells"
echo "3. If a crash occurs, AddressSanitizer will show detailed information"
echo "4. Look for 'AddressSanitizer' messages in the output"
echo ""

# Set environment variables for better debugging
export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:symbolize=1:print_stacktrace=1"
export LSAN_OPTIONS="suppressions=../asan_suppressions.txt"

# Run the application
echo "Starting application..."
./megatunix-redux --demo-mode

echo ""
echo "=== Debug Session Complete ==="
echo ""
echo "If a segmentation fault occurred:"
echo "1. Look for 'AddressSanitizer' messages above"
echo "2. The stack trace will show exactly where the crash occurred"
echo "3. Memory corruption issues will be clearly identified"
echo ""
echo "Common AddressSanitizer messages:"
echo "- 'heap-buffer-overflow': Writing beyond allocated memory"
echo "- 'stack-buffer-overflow': Writing beyond stack array bounds"
echo "- 'global-buffer-overflow': Writing beyond global array bounds"
echo "- 'use-after-free': Using memory after it was freed"
echo "- 'double-free': Freeing memory twice"
echo "- 'invalid-free': Freeing invalid memory address"

