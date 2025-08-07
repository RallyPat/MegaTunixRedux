# MegaTunix Redux Tests

This directory contains all test files and utilities for MegaTunix Redux.

## Directory Structure

### `/speeduino/` - Speeduino Communication Tests
Contains all Speeduino protocol communication tests and utilities:

- **Protocol Tests**: `speeduino_*_test.c` - Various Speeduino protocol implementations
- **CRC Tests**: `speeduino_crc_test.c` - CRC calculation and validation tests
- **Communication Tests**: `speeduino_listen.c`, `speeduino_debug.c` - Communication debugging tools
- **Protocol Variants**: `speeduino_*_protocol.c` - Different protocol implementations

### `/communication/` - General Communication Tests
Contains general communication and protocol tests:

- **TunerStudio Capture**: `capture_tunerstudio.c` - TunerStudio protocol analysis

### `/misc/` - Miscellaneous Tests
Contains various utility tests and debugging tools:

- **Simple Tests**: `simple_test.c` - Basic functionality tests
- **Clay Tests**: `test_clay*.c` - Clay-specific test implementations

### `/hardware/` - Hardware Integration Tests
Contains hardware-specific test scripts and utilities:

- **Speeduino Hardware**: `test_speeduino_hardware.sh` - Hardware integration tests
- **Real Speeduino**: `test_real_speeduino.sh` - Tests with actual Speeduino hardware

## Running Tests

### Speeduino Tests
```bash
cd tests/speeduino/
# Compile and run specific tests
gcc -o speeduino_test speeduino_test.c
./speeduino_test
```

### Hardware Tests
```bash
cd tests/hardware/
# Run hardware integration tests
./test_speeduino_hardware.sh
```

### Communication Tests
```bash
cd tests/communication/
# Run communication protocol tests
gcc -o capture_tunerstudio capture_tunerstudio.c
./capture_tunerstudio
```

## Test Categories

### Unit Tests
- Individual component testing
- Protocol validation
- CRC calculation tests

### Integration Tests
- Hardware communication
- Protocol compatibility
- End-to-end communication

### Debugging Tools
- Protocol analyzers
- Communication monitors
- Error detection utilities

## Building Tests

Most test files can be compiled individually:
```bash
gcc -o test_name test_name.c -lserialport
```

For tests requiring additional libraries, see individual test files for specific requirements.

## Notes

- Test executables are moved here from the root directory during cleanup
- Original test functionality is preserved
- Tests can be run independently or as part of CI/CD pipeline
- Hardware tests require actual Speeduino hardware
