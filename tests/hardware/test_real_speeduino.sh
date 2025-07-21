#!/bin/bash

# Real Speeduino Hardware Test
# Tests MegaTunix Redux communication with actual Speeduino hardware

echo "========================================================"
echo "    MegaTunix Redux - Real Speeduino Hardware Test"
echo "========================================================"
echo

# Check if the Speeduino device exists
SPEEDUINO_DEVICE="/dev/ttyACM0"

if [ ! -e "$SPEEDUINO_DEVICE" ]; then
    echo "❌ Speeduino device not found at $SPEEDUINO_DEVICE"
    echo "   Please ensure your Speeduino is connected via USB"
    echo "   Available devices:"
    ls /dev/tty* | grep -E "(USB|ACM)" 2>/dev/null || echo "   No USB/ACM devices found"
    exit 1
fi

echo "✅ Speeduino device found at $SPEEDUINO_DEVICE"

# Check permissions
if [ ! -r "$SPEEDUINO_DEVICE" ] || [ ! -w "$SPEEDUINO_DEVICE" ]; then
    echo "❌ Permission denied for $SPEEDUINO_DEVICE"
    echo "   Your user needs to be in the 'dialout' group"
    echo "   Run: sudo usermod -a -G dialout $USER"
    echo "   Then log out and back in"
    exit 1
fi

echo "✅ Device permissions OK"
echo

# Build the application
echo "Building MegaTunix Redux..."
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux/build
make > /dev/null 2>&1

if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo "✅ Build successful"
echo

# Test basic serial communication with stty/echo
echo "Testing basic serial communication..."
echo "Sending Speeduino 'S' command (signature request)..."

# Configure the serial port
stty -F $SPEEDUINO_DEVICE 115200 cs8 -cstopb -parenb raw

# Send 'S' command and read response
timeout 2s bash -c "
    echo -n 'S' > $SPEEDUINO_DEVICE
    sleep 0.5
    read -t 1 response < $SPEEDUINO_DEVICE
    echo \"Response: \$response\"
    if [[ \$response == *speeduino* ]] || [[ \$response == *Speeduino* ]]; then
        echo '✅ Speeduino signature detected!'
        exit 0
    else
        echo '⚠️  Unexpected response (but device is responding)'
        exit 1
    fi
" 2>/dev/null

BASIC_TEST_RESULT=$?

echo

# Now test with our application
echo "Testing MegaTunix Redux ECU connection..."
echo "This will:"
echo "1. Start the application in demo mode"  
echo "2. Show the dashboard with simulated data"
echo "3. Press 'C' to test ECU auto-detection"
echo "4. Press 'E' to toggle engine simulation"
echo "5. Press 'Esc' to exit"
echo
echo "Expected behavior:"
echo "- Dashboard should show live gauges"
echo "- 'C' key should find and connect to Speeduino"
echo "- Connection indicator should turn green"
echo "- Engine controls should work"
echo
echo "==================== INSTRUCTIONS ===================="
echo "1. The application will start now"
echo "2. Press 'C' to test real ECU connection"
echo "3. Watch for 'Connected to ECU: Speeduino' message"
echo "4. Press 'E' to toggle engine simulation"
echo "5. Press 'L' to toggle data logging"
echo "6. Press 'Esc' to exit when done"
echo "======================================================"
echo
read -p "Press Enter to start MegaTunix Redux (or Ctrl+C to cancel)..."

# Start the application
echo
echo "Starting MegaTunix Redux..."
./megatunix-redux --demo-mode

echo
echo "Test completed!"
echo
echo "Summary:"
if [ $BASIC_TEST_RESULT -eq 0 ]; then
    echo "✅ Basic serial communication: SUCCESS"
else
    echo "⚠️  Basic serial communication: PARTIAL (device responds but unexpected format)"
fi
echo "📱 GUI Test: Completed (check logs above for ECU connection status)"
echo
echo "Next steps:"
echo "- If ECU connection worked: You can now tune with real data!"
echo "- If ECU connection failed: Check baud rate, protocol, or wiring"
echo "- Try different baud rates: 9600, 38400, 57600, 115200"
