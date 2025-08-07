#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>
#include <stdint.h>
#include <stdbool.h>

// TunerStudio msEnvelope_1.0 format
// Format: [size_high][size_low][command][data...][crc_high][crc_low]
// Size includes command + data + CRC (6 bytes total overhead)

uint16_t ts_calculate_crc16(const uint8_t* data, int length) {
    uint16_t crc = 0x0000;
    
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

uint8_t* ts_build_envelope(uint8_t command, const uint8_t* data, int data_length, int* envelope_length) {
    // Calculate total size: command + data + CRC
    int payload_size = 1 + data_length + 2; // command + data + crc
    int total_size = 2 + payload_size; // size + payload
    
    uint8_t* envelope = malloc(total_size);
    if (!envelope) {
        *envelope_length = 0;
        return NULL;
    }
    
    // Size (big endian)
    envelope[0] = (payload_size >> 8) & 0xFF;
    envelope[1] = payload_size & 0xFF;
    
    // Command
    envelope[2] = command;
    
    // Data
    if (data && data_length > 0) {
        memcpy(&envelope[3], data, data_length);
    }
    
    // Calculate CRC (command + data)
    uint16_t crc = ts_calculate_crc16(&envelope[2], 1 + data_length);
    
    // Add CRC
    envelope[3 + data_length] = (crc >> 8) & 0xFF;
    envelope[4 + data_length] = crc & 0xFF;
    
    *envelope_length = total_size;
    return envelope;
}

void test_ts_envelope_command(int fd, uint8_t command, const char* name) {
    printf("\n=== Testing TS Envelope %s command: 0x%02X ===\n", name, command);
    
    // Flush buffer
    tcflush(fd, TCIOFLUSH);
    
    // Wait before sending
    usleep(10000); // 10ms
    
    // Build TS envelope
    int envelope_length;
    uint8_t* envelope = ts_build_envelope(command, NULL, 0, &envelope_length);
    
    if (!envelope) {
        printf("Failed to build envelope\n");
        return;
    }
    
    // Send envelope
    printf("Sending TS envelope (%d bytes): ", envelope_length);
    for (int i = 0; i < envelope_length; i++) {
        printf("[%02X]", envelope[i]);
    }
    printf("\n");
    
    int bytes_written = write(fd, envelope, envelope_length);
    if (bytes_written != envelope_length) {
        printf("Failed to write envelope: %s\n", strerror(errno));
        free(envelope);
        return;
    }
    
    printf("TS envelope sent successfully!\n");
    free(envelope);
    
    // Wait for response
    usleep(100000); // 100ms delay
    
    // Try to read response
    printf("Reading response...\n");
    
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
            
            // Print as hex
            for (int j = 0; j < bytes_read && j < 64; j++) {
                printf("[%02X]", buffer[j]);
            }
            if (bytes_read > 64) {
                printf("... (truncated)");
            }
            printf("\n");
        } else {
            printf("Read returned %d: %s\n", bytes_read, strerror(errno));
        }
    } else if (select_result == 0) {
        printf("Timeout - no data available\n");
    } else {
        printf("Select error: %s\n", strerror(errno));
    }
}

void test_ascii_command(int fd, const char* command, const char* name) {
    printf("\n=== Testing ASCII %s command: '%s' ===\n", name, command);
    
    // Flush buffer
    tcflush(fd, TCIOFLUSH);
    
    // Wait before sending
    usleep(10000); // 10ms
    
    // Send ASCII command
    printf("Sending ASCII command: '%s'\n", command);
    int cmd_len = strlen(command);
    int bytes_written = write(fd, command, cmd_len);
    if (bytes_written != cmd_len) {
        printf("Failed to write command: %s\n", strerror(errno));
        return;
    }
    
    printf("ASCII command sent successfully!\n");
    
    // Wait for response
    usleep(100000); // 100ms delay
    
    // Try to read response
    printf("Reading response...\n");
    
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
            
            // Check if it's ASCII text or binary data
            bool is_ascii = true;
            for (int j = 0; j < bytes_read && j < 20; j++) {
                if (buffer[j] < 32 || buffer[j] > 126) {
                    is_ascii = false;
                    break;
                }
            }
            
            if (is_ascii) {
                // Print as ASCII
                printf("'%.*s'", bytes_read, buffer);
            } else {
                // Print as hex
                for (int j = 0; j < bytes_read && j < 64; j++) {
                    printf("[%02X]", buffer[j]);
                }
                if (bytes_read > 64) {
                    printf("... (truncated)");
                }
            }
            printf("\n");
        } else {
            printf("Read returned %d: %s\n", bytes_read, strerror(errno));
        }
    } else if (select_result == 0) {
        printf("Timeout - no data available\n");
    } else {
        printf("Select error: %s\n", strerror(errno));
    }
}

int main() {
    const char* port = "/dev/ttyACM0";
    
    printf("TunerStudio Envelope Protocol Speeduino Test on %s\n", port);
    printf("Testing both ASCII and TS Envelope protocols\n\n");
    
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
    
    printf("Serial port configured successfully at 115200 baud\n");
    printf("Waiting 1 second after port open...\n");
    
    // Wait after port open
    usleep(1000000); // 1 second
    
    printf("Starting TS Envelope tests...\n\n");
    
    // Test ASCII commands first
    printf("=== TESTING ASCII PROTOCOL ===\n");
    test_ascii_command(fd, "Q", "Query");
    usleep(1000000);
    
    test_ascii_command(fd, "S", "Version Info");
    usleep(1000000);
    
    // Test TS Envelope commands
    printf("\n=== TESTING TS ENVELOPE PROTOCOL ===\n");
    test_ts_envelope_command(fd, 'Q', "Query");
    usleep(1000000);
    
    test_ts_envelope_command(fd, 'S', "Version Info");
    usleep(1000000);
    
    test_ts_envelope_command(fd, 'V', "Version");
    usleep(1000000);
    
    test_ts_envelope_command(fd, 'A', "Data");
    usleep(1000000);
    
    close(fd);
    printf("\nTest completed.\n");
    return 0;
} 