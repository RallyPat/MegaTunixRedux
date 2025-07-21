#!/bin/bash

echo "=== INTERACTIVE SPEEDUINO TEST ==="
echo
echo "Speeduino detected at: /dev/ttyACM0"
echo "Arduino device: usb-Arduino__www.arduino.cc__0042_3433132"
echo
echo "Starting MegaTunix Redux for manual testing..."
echo
echo "🎯 TEST INSTRUCTIONS:"
echo "1. Application will start with dashboard"
echo "2. Press 'C' key to test Speeduino connection"
echo "3. Watch console output for connection results"
echo "4. If connected, press 'E' to test engine simulation"
echo "5. Press 'L' to test logging toggle"
echo "6. Press 'Esc' to quit"
echo
echo "Expected: Speeduino should be detected and identified"
echo
echo "Starting in 3 seconds..."
sleep 3

# Run the application
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux/build
./megatunix-redux --demo-mode
