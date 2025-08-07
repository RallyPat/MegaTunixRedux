#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#define PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

// Speeduino CRC protocol constants
#define SPEEDUINO_START_BYTE 0x72
#define SPEEDUINO_STOP_BYTE 0x03
#define SPEEDUINO_CMD_QUERY 0x51
#define SPEEDUINO_CMD_GET_VERSION 0x56
#define SPEEDUINO_CMD_GET_SIGNATURE 0x53
#define SPEEDUINO_CMD_GET_DATA 0x41

typedef struct {
    uint8_t start_byte;
    uint8_t command;
    uint8_t data_length;
    uint8_t data[256];
    uint8_t crc_high;
    uint8_t crc_low;
    uint8_t stop_byte;
} SpeeduinoPacket;

// Calculate CRC-16 for Speeduino packets
static uint16_t speeduino_calculate_crc16(const uint8_t* data, int length) {
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
static uint8_t* speeduino_build_packet(uint8_t command, const uint8_t* data, int data_length, int* packet_length) {
    // Calculate total packet size
    int total_size = 6 + data_length; // start + command + length + data + crc_high + crc_low + stop
    uint8_t* packet = malloc(total_size);
    
    if (!packet) {
        *packet_length = 0;
        return NULL;
    }
    
    // Build packet
    packet[0] = SPEEDUINO_START_BYTE;  // Start byte
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
    packet[5 + data_length] = SPEEDUINO_STOP_BYTE;   // Stop byte
    
    *packet_length = total_size;
    return packet;
}

// Parse Speeduino packet
static bool speeduino_parse_packet(const uint8_t* packet, int length, SpeeduinoPacket* parsed) {
    if (!packet || !parsed || length < 6) {
        return false;
    }
    
    // Check start and stop bytes
    if (packet[0] != SPEEDUINO_START_BYTE || packet[length - 1] != SPEEDUINO_STOP_BYTE) {
        return false;
    }
    
    // Extract packet components
    parsed->start_byte = packet[0];
    parsed->command = packet[1];
    parsed->data_length = packet[2];
    
    // Validate data length
    if (parsed->data_length > 256 || (3 + parsed->data_length + 3) != (uint16_t)length) {
        return false;
    }
    
    // Copy data
    if (parsed->data_length > 0) {
        memcpy(parsed->data, &packet[3], parsed->data_length);
    }
    
    // Extract CRC
    parsed->crc_high = packet[3 + parsed->data_length];
    parsed->crc_low = packet[4 + parsed->data_length];
    parsed->stop_byte = packet[5 + parsed->data_length];
    
    // Validate CRC
    uint16_t received_crc = (parsed->crc_high << 8) | parsed->crc_low;
    uint16_t calculated_crc = speeduino_calculate_crc16(&packet[1], 2 + parsed->data_length);
    
    return received_crc == calculated_crc;
}

void hex_dump(const char *prefix, const unsigned char *data, int length) {
    printf("%s (%d bytes): ", prefix, length);
    for (int i = 0; i < length; i++) {
        printf("%02X ", data[i]);
    }
    printf(" | ");
    for (int i = 0; i < length; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            printf("%c", data[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

int test_crc_command(int fd, uint8_t command, const char *description) {
    printf("\n=== Testing CRC %s command (0x%02X) ===\n", description, command);
    
    // Build CRC packet
    int packet_length;
    uint8_t* packet = speeduino_build_packet(command, NULL, 0, &packet_length);
    
    if (!packet) {
        printf("❌ Failed to build CRC packet\n");
        return 0;
    }
    
    printf("Sending CRC packet: ");
    hex_dump("", packet, packet_length);
    
    // Send packet
    int bytes_written = write(fd, packet, packet_length);
    if (bytes_written != packet_length) {
        printf("❌ Failed to send CRC packet: %s\n", strerror(errno));
        free(packet);
        return 0;
    }
    
    printf("✅ CRC packet sent successfully!\n");
    free(packet);
    
    // Wait for response
    usleep(200000); // 200ms delay
    
    // Read response with multiple attempts
    unsigned char buffer[256];
    int total_read = 0;
    
    for (int attempt = 1; attempt <= 10; attempt++) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; // 500ms timeout
        
        int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (result > 0 && FD_ISSET(fd, &readfds)) {
            int bytes_read = read(fd, buffer + total_read, sizeof(buffer) - total_read);
            if (bytes_read > 0) {
                total_read += bytes_read;
                printf("Attempt %d: Read %d bytes\n", attempt, bytes_read);
                
                // Try to parse as CRC packet
                SpeeduinoPacket parsed_packet;
                if (speeduino_parse_packet(buffer, total_read, &parsed_packet)) {
                    printf("✅ Valid CRC packet received!\n");
                    printf("Command: 0x%02X, Data length: %d\n", parsed_packet.command, parsed_packet.data_length);
                    hex_dump("CRC Response", buffer, total_read);
                    return 1;
                }
                
                // Check for ASCII response (fallback)
                if (strstr((char*)buffer, "speeduino") != NULL) {
                    printf("⚠️  ASCII response received (not CRC): ");
                    hex_dump("", buffer, total_read);
                    return 1;
                }
            }
        }
    }
    
    if (total_read > 0) {
        printf("⚠️  Got response but not valid CRC format: ");
        hex_dump("", buffer, total_read);
    } else {
        printf("❌ No response received\n");
    }
    
    return 0;
}

int main() {
    printf("=== Speeduino CRC Binary Protocol Test ===\n");
    printf("Testing the official Speeduino CRC protocol\n\n");
    
    int fd = open(PORT, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Failed to open port");
        return 1;
    }
    
    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr failed");
        close(fd);
        return 1;
    }
    
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);
    
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr failed");
        close(fd);
        return 1;
    }
    
    printf("Serial port configured at 115200 baud\n");
    printf("Waiting 1 second after port open...\n");
    sleep(1);
    
    // Test CRC commands
    int success_count = 0;
    success_count += test_crc_command(fd, SPEEDUINO_CMD_QUERY, "Query");
    success_count += test_crc_command(fd, SPEEDUINO_CMD_GET_VERSION, "Version");
    success_count += test_crc_command(fd, SPEEDUINO_CMD_GET_SIGNATURE, "Signature");
    success_count += test_crc_command(fd, SPEEDUINO_CMD_GET_DATA, "Data");
    
    close(fd);
    
    printf("\n=== Test Results ===\n");
    printf("Successful CRC commands: %d/4\n", success_count);
    
    if (success_count > 0) {
        printf("✅ Speeduino responds to CRC protocol!\n");
    } else {
        printf("❌ Speeduino does not respond to CRC protocol\n");
    }
    
    return 0;
} 