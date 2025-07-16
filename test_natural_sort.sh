#!/usr/bin/env bash

# Test natural sorting by creating symbolic links to test devices
echo "Testing natural sorting of serial device names..."

# Create test directory
mkdir -p /tmp/test_serial_devices
cd /tmp/test_serial_devices

# Create test device files with numbers that would sort incorrectly alphabetically
touch ttyS1 ttyS2 ttyS10 ttyS11 ttyS20 ttyS3 ttyUSB0 ttyUSB1 ttyUSB10 ttyUSB11 ttyUSB2 ttyACM0 ttyACM1 ttyACM10 ttyACM2

# Show alphabetical sort (what we had before)
echo "Alphabetical sort (old behavior):"
ls -1 | sort

echo ""
echo "Natural sort (what we want):"
ls -1 | sort -V

echo ""
echo "Test completed. Temporary files in /tmp/test_serial_devices"
