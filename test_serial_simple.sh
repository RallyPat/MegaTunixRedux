#!/bin/bash

echo "=== Speeduino Serial Connection Test (shell timeout) ==="
echo "This uses the shell 'timeout' command for guaranteed timeouts"
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
echo "2. Testing serial port access..."
if [ -r /dev/ttyACM0 ] && [ -w /dev/ttyACM0 ]; then
    echo "✓ /dev/ttyACM0 is readable and writable"
else
    echo "✗ /dev/ttyACM0 access denied"
    echo "Check permissions and ensure you're in the 'dialout' group"
    exit 1
fi

echo ""
echo "3. Testing basic serial communication with guaranteed 3-second timeout..."

# Test 1: Try to open and configure the port
echo ""
echo "Test 1: Port configuration test"
timeout 3s bash -c '
    exec 3<>/dev/ttyACM0
    if [ $? -eq 0 ]; then
        echo "✓ Port opened successfully"
        # Configure port (simplified)
        stty -F /dev/ttyACM0 115200 raw -echo -echoe -echok
        if [ $? -eq 0 ]; then
            echo "✓ Port configured successfully"
        else
            echo "✗ Port configuration failed"
        fi
        exec 3>&-
    else
        echo "✗ Failed to open port"
        exit 1
    fi
' 2>/dev/null

if [ $? -eq 124 ]; then
    echo "✗ Port configuration test timed out after 3 seconds"
else
    echo "✓ Port configuration test completed"
fi

# Test 2: Send command and wait for response
echo ""
echo "Test 2: Communication test (3s timeout)"
timeout 3s bash -c '
    exec 3<>/dev/ttyACM0
    stty -F /dev/ttyACM0 115200 raw -echo -echoe -echok
    
    # Send Speeduino status request
    echo -n "S" >&3
    
    # Wait for response
    read -t 2 -r response <&3
    
    if [ -n "$response" ]; then
        echo "✓ Received response: $response"
        echo "Response in hex:"
        echo -n "$response" | od -t x1
    else
        echo "✗ No response received"
    fi
    
    exec 3>&-
' 2>/dev/null

if [ $? -eq 124 ]; then
    echo "✗ Communication test timed out after 3 seconds"
    echo "This is normal if Speeduino is not responding or not powered on"
else
    echo "✓ Communication test completed"
fi

echo ""
echo "4. Test completed!"
echo ""
echo "Results summary:"
echo "- If you saw '✓' messages, the port is working"
echo "- If you saw '✗' messages, there are issues to resolve"
echo "- If you saw timeouts, the port is open but Speeduino may not be responding"
echo ""
echo "Next steps:"
echo "1. Ensure your Speeduino is powered on and connected"
echo "2. Check that no other software is using the port"
echo "3. Test with MegaTunix Redux Plugin Manager"
echo ""
echo "Common Speeduino ports:"
echo "- USB: /dev/ttyUSB0, /dev/ttyACM0"
echo "- Serial: /dev/ttyS0, /dev/ttyS1"
