#!/bin/bash

# Test ECU communication functionality

echo "=== MegaTunix Redux ECU Communication Test ==="
echo

# Build the project
echo "Building project..."
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux/build
make > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo "✅ Build successful"
else
    echo "❌ Build failed"
    exit 1
fi

echo

# Test basic ECU discovery (without actual devices)
echo "Testing ECU port discovery..."
echo "Expected: Should list available serial ports (if any) and handle gracefully when no ECU is found"
echo

# Start the application briefly and capture logs
timeout 3s ./megatunix-redux --demo-mode 2>&1 | grep -E "(ECU|connection|port|serial)" || echo "No ECU-related logs found (expected in demo mode)"

echo
echo "=== Test completed ==="
echo
echo "Manual Test Instructions:"
echo "1. Run: ./megatunix-redux --demo-mode"
echo "2. Press 'C' key to test ECU connection discovery"
echo "3. Press 'E' key to toggle engine simulation" 
echo "4. Press 'L' key to toggle data logging"
echo "5. Press 'Esc' to quit"
echo
echo "Expected behavior:"
echo "- Application starts with working dashboard UI"
echo "- 'C' key shows ECU discovery attempt (will fail with no real ECU connected)"
echo "- 'E' and 'L' keys toggle dashboard indicators"
echo "- Real-time dashboard gauges update when engine is running"
