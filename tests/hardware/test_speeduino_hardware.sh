#!/bin/bash

# Test MegaTunix Redux with real Speeduino hardware

echo "=== MegaTunix Redux Speeduino Hardware Test ==="
echo

# Build the project
echo "1. Building project..."
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux/build
make > build.log 2>&1

if [ $? -eq 0 ]; then
    echo "   ✅ Build successful"
else
    echo "   ❌ Build failed"
    echo "   Build log:"
    tail -10 build.log
    exit 1
fi

echo

# Check for serial ports
echo "2. Checking for available serial ports..."
if command -v ls /dev/ttyUSB* >/dev/null 2>&1; then
    echo "   USB serial ports found:"
    ls -la /dev/ttyUSB* 2>/dev/null || echo "   No /dev/ttyUSB* ports"
fi

if command -v ls /dev/ttyACM* >/dev/null 2>&1; then
    echo "   ACM serial ports found:"
    ls -la /dev/ttyACM* 2>/dev/null || echo "   No /dev/ttyACM* ports"
fi

echo

# Test ECU detection
echo "3. Testing ECU detection..."
echo "   Starting application for 8 seconds to test Speeduino detection..."
echo "   Press 'C' key in the application window to test ECU connection"
echo

# Start application and capture logs
timeout 8s ./megatunix-redux --demo-mode > speeduino_test.log 2>&1 &
APP_PID=$!

# Wait a moment for startup
sleep 2

echo "   Application started (PID: $APP_PID)"
echo "   Logs will be captured in speeduino_test.log"
echo
echo "   === MANUAL TEST INSTRUCTIONS ==="
echo "   1. Focus the MegaTunix Redux window"
echo "   2. Press 'C' key to attempt ECU connection"
echo "   3. Check the terminal logs for connection results"
echo "   4. If connected, press 'E' to start engine simulation"
echo "   5. Application will close automatically in ~5 seconds"
echo

# Wait for the timeout to complete
wait $APP_PID 2>/dev/null

echo
echo "4. Analyzing test results..."

# Check for Speeduino-specific logs
if grep -q "Speeduino" speeduino_test.log; then
    echo "   ✅ Speeduino detection attempted"
    grep "Speeduino\|speeduino" speeduino_test.log | head -5
else
    echo "   ℹ️  No Speeduino-specific activity detected"
fi

# Check for serial port activity
if grep -q "port\|serial\|ECU" speeduino_test.log; then
    echo "   ✅ Serial/ECU activity detected"
    grep -i "port\|serial\|ecu.*connect" speeduino_test.log | head -5
else
    echo "   ℹ️  No serial port activity detected"
fi

# Check for errors
if grep -q "ERROR\|Failed" speeduino_test.log; then
    echo "   ⚠️  Errors detected:"
    grep "ERROR\|Failed" speeduino_test.log | head -3
fi

echo
echo "5. Connection test summary:"
echo

# Show last few relevant log lines
echo "   Recent logs:"
tail -10 speeduino_test.log | grep -E "(INFO|ERROR|connection|ECU|Speeduino)" || echo "   No relevant connection logs found"

echo
echo "=== Test completed ==="
echo
echo "Next steps:"
echo "• If Speeduino was detected: Great! The communication is working"
echo "• If no ECU found: Check USB connection and permissions"
echo "• For manual testing: ./megatunix-redux --demo-mode"
echo "• Press 'C' in the app to test ECU auto-detection"
echo "• Check speeduino_test.log for detailed logs"

# Show permissions reminder
echo
echo "Troubleshooting:"
echo "• Ensure Speeduino is connected via USB"
echo "• Check user permissions: sudo usermod -a -G dialout \$USER"
echo "• Verify port exists: ls -la /dev/ttyUSB* /dev/ttyACM*"
echo "• Try different baud rates if needed"
