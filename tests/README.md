# MegaTunix Redux - Test Suite

This directory contains all test files and scripts for MegaTunix Redux.

## Directory Structure

### `hardware/`
Hardware-specific tests that require actual ECU hardware:
- `test_speeduino_hardware.sh` - Tests with physical Speeduino ECU
- `test_real_speeduino.sh` - Real hardware communication tests
- `run_speeduino_test.sh` - Speeduino hardware test runner

### `integration/`
Integration tests for ECU communication and protocols:
- `test_ecu_communication.sh` - ECU communication protocol tests
- `test_realtime_streaming.sh` - Real-time data streaming tests
- `test_speeduino_direct.c` - Direct Speeduino communication test program
- `test_speeduino_enhanced.c` - Enhanced Speeduino protocol tests
- `test_speeduino_direct` - Compiled test binary

### `main/`
Main application unit tests (future use)

### `renderer/`
Renderer and UI component tests (future use)

### `e2e/`
End-to-end application tests (future use)

## Running Tests

### Hardware Tests (Require Physical ECU)
```bash
# Run all hardware tests
cd tests/hardware
./run_speeduino_test.sh

# Individual hardware tests
./test_speeduino_hardware.sh
./test_real_speeduino.sh
```

### Integration Tests
```bash
# ECU communication tests
cd tests/integration
./test_ecu_communication.sh
./test_realtime_streaming.sh

# Direct protocol tests
./test_speeduino_direct
```

## Test Requirements

- **Hardware Tests**: Require physical Speeduino ECU connected via USB/serial
- **Integration Tests**: Can run with or without hardware (some have simulation modes)
- **Unit Tests**: No hardware required (future implementation)

## Adding New Tests

When adding new test files:
1. Place them in the appropriate subdirectory based on test type
2. Use descriptive filenames with `test_` prefix
3. Make shell scripts executable (`chmod +x`)
4. Update this README with new test descriptions
