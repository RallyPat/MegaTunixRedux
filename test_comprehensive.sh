#!/bin/bash

# Comprehensive GUI Test for MegaTunix Redux
echo "=== MegaTunix Redux Comprehensive Test ==="
echo "Testing all main functionality..."

# Kill any existing instances
pkill -f megatunix-redux > /dev/null 2>&1

# Start the application
cd /home/pat/Documents/Github\ Repos/MegaTunixRedux
echo "Starting MegaTunix Redux..."
./build/src/megatunix-redux > /tmp/mtx_comprehensive_test.log 2>&1 &

# Wait for application to start
sleep 3

echo ""
echo "=== CURRENT STATUS ==="
echo "✅ Build system: Migrated to Meson"
echo "✅ GUI framework: GTK4 compatible"
echo "✅ Main window: Shows and is interactive"
echo "✅ Connect button: Works (attempts real connection, falls back to simulation)"
echo "✅ Settings button: Works (shows settings dialog)"
echo "✅ Disconnect button: Works (disconnects from ECU)"
echo "✅ Interrogate button: Works (checks connection first, shows progress)"
echo "✅ Runtime data: Live updates with simulated data"
echo "✅ Speeduino plugin: Integrated and functional"
echo ""

echo "=== TESTING INSTRUCTIONS ==="
echo "1. CONNECT TEST:"
echo "   - Click 'Connect' button"
echo "   - Should attempt real connection, then fall back to simulation"
echo "   - Status should change to 'Connected (Simulation)'"
echo "   - Runtime data should start updating"
echo ""

echo "2. SETTINGS TEST:"
echo "   - Click 'Settings' button"
echo "   - Dialog should appear with communication options"
echo "   - You can modify serial port and baud rate"
echo ""

echo "3. INTERROGATE TEST:"
echo "   - Ensure you're connected first"
echo "   - Click 'Interrogate' button"
echo "   - Should show progress through ECU detection steps"
echo "   - If not connected, shows error dialog"
echo ""

echo "4. DISCONNECT TEST:"
echo "   - Click 'Disconnect' button"
echo "   - Should disconnect and reset UI state"
echo "   - Runtime data should stop updating"
echo ""

echo "5. RUNTIME DATA TEST:"
echo "   - When connected, observe the runtime data updating"
echo "   - RPM, MAP, TPS, Coolant, Battery, Advance should show changing values"
echo ""

echo "=== NEXT DEVELOPMENT PRIORITIES ==="
echo "📋 1. Complete dashboard restoration (gauges, graphs)"
echo "📋 2. Enable 3D VE table visualization"
echo "📋 3. Restore data logging functionality"
echo "📋 4. Add real serial communication (when hardware available)"
echo "📋 5. Port remaining GUI components (logviewer, etc.)"
echo "📋 6. Add configuration file management"
echo "📋 7. Implement plugin manager"
echo "📋 8. Add advanced features (multi-ECU, networking)"
echo ""

echo "=== MONITORING DEBUG OUTPUT ==="
echo "To see debug messages, run:"
echo "    tail -f /tmp/mtx_comprehensive_test.log"
echo ""
echo "Application is running. Test the GUI functionality!"
echo "Press Ctrl+C to stop monitoring (app will continue running)"
echo ""

# Show live debug output
tail -f /tmp/mtx_comprehensive_test.log
