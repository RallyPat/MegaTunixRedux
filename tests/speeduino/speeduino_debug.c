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

void test_command_with_response(int fd, uint8_t command, const char* name) {
    printf("\n=== Testing %s command (0x%02X) ===\n", name, command);
    
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
    printf("Sending packet (%d bytes): ", packet_length);
    for (int i = 0; i < packet_length; i++) {
        printf("[%02X]", packet[i]);
    }
    printf("\n");
    
    int bytes_written = write(fd, packet, packet_length);
    if (bytes_written != packet_length) {
        printf("Failed to write packet: %s\n", strerror(errno));
        free(packet);
        return;
    }
    
    printf("Packet sent successfully!\n");
    free(packet);
    
    // Wait for response with multiple attempts
    printf("Waiting for response...\n");
    
    for (int attempt = 1; attempt <= 5; attempt++) {
        printf("Attempt %d: ", attempt);
        
        // Use select with timeout
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = 2;  // 2 second timeout
        timeout.tv_usec = 0;
        
        int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (select_result > 0) {
            // Data is available
            uint8_t buffer[512];
            int bytes_read = read(fd, buffer, sizeof(buffer));
            
            if (bytes_read > 0) {
                printf("SUCCESS! Received %d bytes: ", bytes_read);
                for (int j = 0; j < bytes_read && j < 64; j++) {
                    if (buffer[j] >= 32 && buffer[j] <= 126) {
                        printf("%c", buffer[j]);
                    } else {
                        printf("[%02X]", buffer[j]);
                    }
                }
                if (bytes_read > 64) {
                    printf("... (truncated)");
                }
                printf("\n");
                return; // Success!
            } else {
                printf("Read returned %d: %s\n", bytes_read, strerror(errno));
            }
        } else if (select_result == 0) {
            printf("Timeout - no data available\n");
        } else {
            printf("Select error: %s\n", strerror(errno));
        }
        
        // Wait a bit before next attempt
        usleep(500000); // 500ms
    }
    
    printf("No response received after 5 attempts\n");
}

int main() {
    const char* port = "/dev/ttyACM0";
    
    printf("Detailed Speeduino communication test on %s\n", port);
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
    
    printf("Serial port configured successfully at 115200 baud\n\n");
    
    // Test basic commands
    test_command_with_response(fd, 0x51, "Query");     // 'Q'
    test_command_with_response(fd, 0x53, "Status");    // 'S'
    test_command_with_response(fd, 0x56, "Version");   // 'V'
    test_command_with_response(fd, 0x41, "Data");      // 'A'
    
    close(fd);
    printf("\nTest completed.\n");
    return 0;
} 