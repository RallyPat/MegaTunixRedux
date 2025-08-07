#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>
#include <stdint.h>

// CRC-16 calculation for Speeduino protocol
uint16_t speeduino_calculate_crc16(const uint8_t* data, int length) {
    uint16_t crc = 0xFFFF;
    
    for (int i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return crc;
}

// Build Speeduino packet with proper formatting
uint8_t* speeduino_build_packet(uint8_t command, const uint8_t* data, int data_length, int* packet_length) {
    // Calculate total packet size
    int total_size = 6 + data_length; // start + command + length + data + crc_high + crc_low + stop
    uint8_t* packet = malloc(total_size);
    
    if (!packet) {
        *packet_length = 0;
        return NULL;
    }
    
    // Build packet
    packet[0] = 0x72;  // Start byte ('r')
    packet[1] = command;               // Command
    packet[2] = data_length;           // Data length
    
    // Copy data
    if (data && data_length > 0) {
        memcpy(&packet[3], data, data_length);
    }
    
    // Calculate CRC (excluding start byte, including command, length, and data)
    uint16_t crc = speeduino_calculate_crc16(&packet[1], 2 + data_length);
    
    // Add CRC
    packet[3 + data_length] = (crc >> 8) & 0xFF;     // CRC high byte
    packet[4 + data_length] = crc & 0xFF;            // CRC low byte
    packet[5 + data_length] = 0x03;   // Stop byte (ETX)
    
    *packet_length = total_size;
    return packet;
}

void test_command(int fd, uint8_t command, const char* name) {
    printf("\n--- Testing %s command (0x%02X) ---\n", name, command);
    
    // Flush buffer
    tcflush(fd, TCIOFLUSH);
    
    // Build proper Speeduino packet
    int packet_length;
    uint8_t* packet = speeduino_build_packet(command, NULL, 0, &packet_length);
    
    if (!packet) {
        printf("Failed to build packet\n");
        return;
    }
    
    // Send packet
    int bytes_written = write(fd, packet, packet_length);
    if (bytes_written != packet_length) {
        printf("Failed to write packet: %s\n", strerror(errno));
        free(packet);
        return;
    }
    
    printf("Sent %d bytes: ", bytes_written);
    for (int i = 0; i < packet_length && i < 16; i++) {
        printf("[%02X]", packet[i]);
    }
    printf("\n");
    
    free(packet);
    
    // Wait for response
    usleep(500000); // 500ms delay
    
    // Try to read response
    uint8_t buffer[256];
    int bytes_read = read(fd, buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        printf("Received %d bytes: ", bytes_read);
        for (int j = 0; j < bytes_read && j < 32; j++) {
            if (buffer[j] >= 32 && buffer[j] <= 126) {
                printf("%c", buffer[j]);
            } else {
                printf("[%02X]", buffer[j]);
            }
        }
        printf("\n");
    } else {
        printf("No response received\n");
    }
}

int main() {
    const char* port = "/dev/ttyACM0";
    
    printf("Comprehensive Speeduino communication test on %s\n", port);
    printf("Make sure your Speeduino is powered on and connected!\n\n");
    
    // Open serial port
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        printf("Failed to open %s: %s\n", port, strerror(errno));
        return 1;
    }
    
    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        printf("Failed to get serial attributes: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Failed to set serial attributes: %s\n", strerror(errno));
        close(fd);
        return 1;
    }
    
    printf("Serial port configured successfully\n");
    
    // Speeduino uses 115200 baud - test at this rate
    printf("\n=== Testing at 115200 baud (Speeduino standard) ===\n");
    
    printf("\n=== Testing binary protocol commands ===\n");
    
    // Test different binary commands
    test_command(fd, 0x53, "Status");      // 'S'
    test_command(fd, 0x56, "Version");     // 'V'
    test_command(fd, 0x41, "Data");        // 'A'
    test_command(fd, 0x72, "Real-time");   // 'r'
    
    printf("\n=== Testing with different timing ===\n");
    
    // Test with different timing approaches
    printf("\n--- Attempt 1: Standard timing ---\n");
    test_command(fd, 0x51, "Query");    // 'Q'
    
    printf("\n--- Attempt 2: Longer delay ---\n");
    usleep(1000000); // 1 second delay
    test_command(fd, 0x53, "Status");   // 'S'
    
    printf("\n--- Attempt 3: Multiple rapid commands ---\n");
    test_command(fd, 0x56, "Version");  // 'V'
    usleep(100000);  // 100ms delay
    test_command(fd, 0x41, "Data");     // 'A'
    
    close(fd);
    printf("\nTest completed. If no responses were received:\n");
    printf("1. Check that Speeduino is powered on\n");
    printf("2. Check that Speeduino is not in bootloader mode\n");
    printf("3. Check that Speeduino firmware is loaded\n");
    printf("4. Try a different USB cable\n");
    printf("5. Check Speeduino documentation for correct protocol\n");
    
    return 0;
} 