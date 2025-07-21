#!/bin/bash

echo "Testing MegaTunix Redux Real-Time Streaming..."
echo "==============================================="

# Function to test Speeduino connection
test_speeduino_streaming() {
    local device="$1"
    
    echo "Testing real-time data streaming from Speeduino on $device..."
    
    # Test basic connectivity first
    echo -n "S" > "$device"
    sleep 0.1
    
    # Read response (should be Speeduino signature)
    timeout 2 cat "$device" | head -c 20 > /tmp/speeduino_sig 2>/dev/null
    
    if [ -s /tmp/speeduino_sig ]; then
        echo "✓ Speeduino detected on $device"
        echo "Signature: $(cat /tmp/speeduino_sig | tr -d '\0' | head -c 20)"
        
        # Now test real-time data streaming (A command)
        echo "Testing real-time data stream..."
        for i in {1..5}; do
            echo "Request $i: Sending 'A' command..."
            echo -n "A" > "$device"
            sleep 0.1
            
            # Read 120 bytes of real-time data
            timeout 2 dd if="$device" bs=120 count=1 2>/dev/null | hexdump -C | head -10
            echo "---"
            sleep 1
        done
        
        return 0
    else
        echo "✗ No response from Speeduino on $device"
        return 1
    fi
}

# Check if running as root or in dialout group
if ! groups | grep -q dialout && [ "$EUID" -ne 0 ]; then
    echo "Warning: You may need to be in the 'dialout' group or run as root to access serial devices"
    echo "To add yourself to dialout group: sudo usermod -a -G dialout \$USER"
    echo "Then log out and log back in."
fi

# Find Speeduino devices
devices=()
for dev in /dev/ttyACM* /dev/ttyUSB*; do
    if [ -e "$dev" ]; then
        devices+=("$dev")
    fi
done

if [ ${#devices[@]} -eq 0 ]; then
    echo "No USB serial devices found. Please connect your Speeduino."
    exit 1
fi

echo "Found ${#devices[@]} USB serial device(s): ${devices[*]}"

# Test each device
for device in "${devices[@]}"; do
    echo ""
    echo "Testing device: $device"
    echo "================================"
    
    # Configure serial port settings for Speeduino
    if command -v stty >/dev/null 2>&1; then
        stty -F "$device" 115200 cs8 -cstopb -parenb raw -echo 2>/dev/null || true
    fi
    
    test_speeduino_streaming "$device"
    
    echo ""
done

echo ""
echo "Real-time streaming test completed."
echo ""
echo "If successful, you should see:"
echo "1. Speeduino signature response to 'S' command"
echo "2. 120 bytes of binary real-time data for each 'A' command"
echo ""
echo "The MegaTunix Redux application will parse this data and display it on the dashboard."
echo "Use the 'C' key in the application to connect and start real-time streaming."

# Cleanup
rm -f /tmp/speeduino_sig
