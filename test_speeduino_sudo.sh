#!/bin/bash

echo "=== Testing Speeduino Connection with sudo ==="
echo "Testing connection to /dev/ttyACM0"

# Check if device exists
if [ ! -e /dev/ttyACM0 ]; then
    echo "ERROR: Device /dev/ttyACM0 not found"
    exit 1
fi

echo "Device permissions:"
ls -la /dev/ttyACM0

echo ""
echo "Testing basic serial communication with sudo..."

# Test basic serial connection with timeout
sudo timeout 5 stty -F /dev/ttyACM0 115200 raw -echo
if [ $? -eq 0 ]; then
    echo "Successfully configured serial port at 115200 baud"
else
    echo "Failed to configure serial port"
    exit 1
fi

echo ""
echo "Attempting to read device info..."

# Try to get device info (Speeduino should respond to 'Q' command)
echo -n "Q" | sudo timeout 3 tee /dev/ttyACM0 > /dev/null
if [ $? -eq 0 ]; then
    echo "Successfully sent query command to device"
    # Try to read response with timeout
    echo "Reading response..."
    sudo timeout 3 head -c 100 /dev/ttyACM0 2>/dev/null | hexdump -C
else
    echo "Failed to send query command"
fi

echo ""
echo "Trying signature command 'S'..."
echo -n "S" | sudo timeout 3 tee /dev/ttyACM0 > /dev/null
if [ $? -eq 0 ]; then
    echo "Successfully sent signature command"
    echo "Reading signature response..."
    sudo timeout 3 head -c 100 /dev/ttyACM0 2>/dev/null | hexdump -C
else
    echo "Failed to send signature command"
fi

echo ""
echo "Trying test communication 'C'..."
echo -n "C" | sudo timeout 3 tee /dev/ttyACM0 > /dev/null
if [ $? -eq 0 ]; then
    echo "Successfully sent test communication command"
    echo "Reading test response..."
    sudo timeout 3 head -c 100 /dev/ttyACM0 2>/dev/null | hexdump -C
else
    echo "Failed to send test communication command"
fi

echo ""
echo "=== Running MegaTunix Redux ==="

# Build and run the application
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux
ninja -C build
if [ $? -eq 0 ]; then
    echo "Build successful, launching application..."
    # Run with sudo temporarily to access serial port
    sudo ./build/src/megatunix
else
    echo "Build failed"
    exit 1
fi
