#!/bin/bash
# MegaTunix Redux GUI Demo Script
# Shows the working MegaTunix Redux GUI with Speeduino plugin

cd /home/pat/Documents/Github\ Repos/MegaTunixRedux

echo "=========================================="
echo "MegaTunix Redux - GUI Demo"
echo "=========================================="
echo ""
echo "🚀 Starting MegaTunix Redux GUI..."
echo "✨ Features:"
echo "   - GTK4 Modern UI"
echo "   - Speeduino ECU Plugin"
echo "   - Runtime Data Display"
echo "   - Connection Management"
echo "   - Multiple Tabs (Runtime, Tuning, Dashboard)"
echo ""
echo "💡 The GUI will show simulated ECU data and connection status"
echo "🔧 Click 'Connect ECU' to test the Speeduino plugin integration"
echo ""
echo "Press Ctrl+C to exit the GUI when done..."
echo ""

# Run the GUI application
HOME=/tmp/megatunix-test ./build/src/megatunix-redux --offline -p Speeduino
