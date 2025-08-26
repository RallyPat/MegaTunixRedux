#!/bin/bash

echo "=== Speeduino Hardware Test Script ==="
echo "This script will help test your Speeduino ECU connection"
echo ""

# Check if we're in the right directory
if [ ! -f "./megatunix-redux" ]; then
    echo "Error: megatunix-redux executable not found in current directory"
    echo "Please run this script from the build_linux directory"
    exit 1
fi

echo "1. Checking available serial ports..."
echo "Available serial devices:"
ls -la /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* 2>/dev/null || echo "No standard serial ports found"

echo ""
echo "2. Checking if any process is using serial ports..."
echo "Processes using serial ports:"
lsof /dev/ttyUSB* /dev/ttyACM* /dev/ttyS* 2>/dev/null || echo "No processes using serial ports"

echo ""
echo "3. Testing Speeduino plugin with real hardware..."
echo "Starting MegaTunix Redux in demo mode..."
echo "Look for Speeduino plugin messages in the output"
echo ""

# Run the application for a short time to see plugin output
timeout 10s ./megatunix-redux --demo-mode 2>&1 | grep -E "(Speeduino|ECU|Connection|Serial|Port|RPM|MAP|AFR)" | head -20

echo ""
echo "4. Test completed!"
echo ""
echo "Next steps:"
echo "- If you see Speeduino plugin messages, the plugin is working"
echo "- If you want to test real serial connection, you'll need to:"
echo "  1. Identify your Speeduino's serial port (usually /dev/ttyUSB0 or /dev/ttyACM0)"
echo "  2. Ensure no other software is using the port"
echo "  3. Run the application and use the Plugin Manager to connect to your port"
echo ""
echo "Common Speeduino ports:"
echo "- USB: /dev/ttyUSB0, /dev/ttyACM0"
echo "- Serial: /dev/ttyS0, /dev/ttyS1"
echo "- Bluetooth: /dev/rfcomm0 (if using Bluetooth adapter)"
