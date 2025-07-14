#!/bin/bash

# Test script to verify Connect and Settings buttons work
echo "=== MegaTunix Redux Button Test ==="
echo "Testing Connect and Settings buttons..."

# Check if the application is running
if ! pgrep -f megatunix-redux > /dev/null; then
    echo "Starting MegaTunix Redux application..."
    cd /home/pat/Documents/Github\ Repos/MegaTunixRedux
    ./build/src/megatunix-redux > /tmp/mtx_button_test.log 2>&1 &
    sleep 3
    echo "Application started"
else
    echo "Application is already running"
fi

echo ""
echo "INSTRUCTIONS:"
echo "1. Click the Connect button - you should see debug messages in the terminal"
echo "2. Click the Settings button - a settings dialog should appear"
echo "3. The Connect button should attempt real connection then fall back to simulation"
echo "4. The Settings button should show a dialog with communication settings"
echo ""
echo "To monitor debug output, run:"
echo "    tail -f /tmp/mtx_button_test.log"
echo ""
echo "Press Ctrl+C to stop monitoring the log"
echo ""

# Show the log output
tail -f /tmp/mtx_button_test.log
