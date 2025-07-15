#!/bin/bash

echo "=== Testing Speeduino Connection ==="
echo "Testing connection to /dev/ttyACM0"

# Check if device exists
if [ ! -e /dev/ttyACM0 ]; then
    echo "ERROR: Device /dev/ttyACM0 not found"
    exit 1
fi

# Check device permissions
echo "Device permissions:"
ls -la /dev/ttyACM0

echo ""
echo "Current user groups:"
groups

echo ""
echo "Testing basic serial communication..."

# Test basic serial connection with timeout
timeout 5 stty -F /dev/ttyACM0 57600 raw -echo
if [ $? -eq 0 ]; then
    echo "Successfully configured serial port at 57600 baud"
else
    echo "Failed to configure serial port"
    exit 1
fi

echo ""
echo "Attempting to read device info..."

# Try to get device info (Speeduino should respond to 'Q' command)
echo -n "Q" > /dev/ttyACM0 2>/dev/null
if [ $? -eq 0 ]; then
    echo "Successfully sent query command to device"
    # Try to read response with timeout
    timeout 2 head -c 100 /dev/ttyACM0 2>/dev/null | hexdump -C
else
    echo "Failed to send query command"
fi

echo ""
echo "=== Running MegaTunix Redux ==="

# Build and run the application
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux
ninja -C build
if [ $? -eq 0 ]; then
    echo "Build successful, launching application..."
    export GTK_DEBUG=interactive
    ./build/src/megatunix
else
    echo "Build failed"
    exit 1
fi
