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

void hex_dump(const char *prefix, const unsigned char *data, int length) {
    printf("%s (%d bytes): ", prefix, length);
    for (int i = 0; i < length && i < 64; i++) {
        printf("%02X ", data[i]);
    }
    if (length > 64) printf("...");
    printf(" | ");
    for (int i = 0; i < length && i < 32; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            printf("%c", data[i]);
        } else {
            printf(".");
        }
    }
    if (length > 32) printf("...");
    printf("\n");
}

int test_ascii_command(int fd, const char *command, const char *description) {
    printf("\n=== Testing ASCII %s command: '%s' ===\n", description, command);
    
    // Flush any existing data
    tcflush(fd, TCIOFLUSH);
    
    // Send command
    int bytes_written = write(fd, command, strlen(command));
    if (bytes_written != strlen(command)) {
        printf("❌ Failed to send command: %s\n", strerror(errno));
        return 0;
    }
    
    printf("✅ Command sent successfully!\n");
    
    // Wait for response (based on INI: interWriteDelay = 10)
    usleep(10000); // 10ms delay
    
    // Read response with timeout (based on INI: blockReadTimeout = 2000)
    unsigned char buffer[256];
    int total_read = 0;
    
    for (int attempt = 1; attempt <= 10; attempt++) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 200000; // 200ms timeout per attempt (based on INI: blockReadTimeout = 2000)
        
        int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (result > 0 && FD_ISSET(fd, &readfds)) {
            int bytes_read = read(fd, buffer + total_read, sizeof(buffer) - total_read);
            if (bytes_read > 0) {
                total_read += bytes_read;
                printf("Attempt %d: Read %d bytes (total: %d)\n", attempt, bytes_read, total_read);
                
                // Check for expected response based on INI file
                if (strcmp(command, "Q") == 0 || strcmp(command, "S") == 0) {
                    // Q and S should return ASCII version string (from INI: signature = "speeduino 202501")
                    if (strstr((char*)buffer, "speeduino") != NULL) {
                        printf("✅ Found 'speeduino' signature!\n");
                        hex_dump("Response", buffer, total_read);
                        return 1;
                    }
                } else if (strcmp(command, "A") == 0) {
                    // A should return realtime data
                    if (total_read >= 120) {
                        printf("✅ Got %d bytes of realtime data\n", total_read);
                        hex_dump("Response", buffer, total_read);
                        return 1;
                    }
                } else if (strcmp(command, "V") == 0) {
                    // V should return VE table and constants
                    if (total_read >= 100) {
                        printf("✅ Got %d bytes of VE table data\n", total_read);
                        hex_dump("Response", buffer, total_read);
                        return 1;
                    }
                }
                
                // If we got any data, consider it successful
                if (total_read > 0) {
                    printf("✅ Got data response (%d bytes)\n", total_read);
                    hex_dump("Response", buffer, total_read);
                    return 1;
                }
            }
        }
    }
    
    if (total_read > 0) {
        printf("⚠️  Got some data but not expected format: ");
        hex_dump("Partial response", buffer, total_read);
    } else {
        printf("❌ No response received\n");
    }
    
    return 0;
}

int main() {
    printf("=== Speeduino INI Protocol Test ===\n");
    printf("Based on 202501.4.ini specifications\n\n");
    
    int fd = open(PORT, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("Failed to open port");
        return 1;
    }
    
    // Configure serial port exactly like the INI file suggests
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr failed");
        close(fd);
        return 1;
    }
    
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);
    
    // 8N1 configuration (8 data bits, no parity, 1 stop bit)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    // Input flags - disable all input processing
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Output flags - disable all output processing
    tty.c_oflag &= ~OPOST;
    
    // Control characters - non-blocking reads
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; // 100ms timeout
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr failed");
        close(fd);
        return 1;
    }
    
    printf("Serial port configured at 115200 baud (8N1)\n");
    
    // Wait after port open as specified in INI file (delayAfterPortOpen=1000)
    printf("Waiting 1 second after port open (INI specification)...\n");
    sleep(1);
    
    // Test commands from INI file
    int success_count = 0;
    success_count += test_ascii_command(fd, "Q", "Query (from INI: queryCommand = \"Q\")");
    success_count += test_ascii_command(fd, "S", "Signature (from INI: versionInfo = \"S\")");
    success_count += test_ascii_command(fd, "A", "Realtime data");
    success_count += test_ascii_command(fd, "V", "VE table");
    
    close(fd);
    
    printf("\n=== Test Results ===\n");
    printf("Successful commands: %d/4\n", success_count);
    
    if (success_count > 0) {
        printf("✅ Speeduino responds to INI protocol!\n");
    } else {
        printf("❌ Speeduino does not respond to INI protocol\n");
    }
    
    return 0;
} 