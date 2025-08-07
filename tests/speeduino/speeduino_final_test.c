#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>
#include <stdint.h>

#define PORT "/dev/ttyACM0"
#define BAUD_RATE B115200

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

int test_command(int fd, const char *command, const char *description) {
    printf("\n=== Testing %s ===\n", description);
    printf("Sending: '%s'\n", command);
    
    // Send command
    int bytes_written = write(fd, command, strlen(command));
    if (bytes_written != strlen(command)) {
        perror("Write failed");
        return 0;
    }
    printf("Command sent successfully!\n");
    
    // Wait a bit for processing
    usleep(100000); // 100ms
    
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
        
        if (result < 0) {
            perror("select failed");
            return 0;
        } else if (result == 0) {
            printf("Attempt %d: Timeout\n", attempt);
            continue;
        }
        
        if (FD_ISSET(fd, &readfds)) {
            int bytes_read = read(fd, buffer + total_read, sizeof(buffer) - total_read);
            if (bytes_read > 0) {
                total_read += bytes_read;
                printf("Attempt %d: Read %d bytes\n", attempt, bytes_read);
                
                // Check if we got a complete response
                if (total_read >= 3) {
                    // Look for "002" pattern
                    if (buffer[0] == '0' && buffer[1] == '0' && buffer[2] == '2') {
                        printf("✅ Found '002' status code!\n");
                        hex_dump("Response", buffer, total_read);
                        return 1;
                    }
                }
                
                // Check for "speeduino" signature
                if (total_read >= 9) {
                    if (strstr((char*)buffer, "speeduino") != NULL) {
                        printf("✅ Found 'speeduino' signature!\n");
                        hex_dump("Response", buffer, total_read);
                        return 1;
                    }
                }
            }
        }
    }
    
    if (total_read > 0) {
        printf("⚠️  Got some data but not expected format:\n");
        hex_dump("Partial response", buffer, total_read);
    } else {
        printf("❌ No response received\n");
    }
    
    return 0;
}

int main() {
    printf("=== Final Speeduino Communication Test ===\n");
    printf("Based on TunerStudio capture observations\n\n");
    
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
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
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
    
    // Test various commands that might trigger the "002" response
    test_command(fd, "Q", "Query command");
    test_command(fd, "S", "Signature command");
    test_command(fd, "V", "Version command");
    test_command(fd, "A", "Data command");
    test_command(fd, "r0000000000", "TunerStudio data command");
    test_command(fd, "r0000000082", "TunerStudio data command with offset");
    
    // Test some variations
    test_command(fd, "\r\n", "Carriage return + newline");
    test_command(fd, "\n", "Newline only");
    test_command(fd, "\r", "Carriage return only");
    
    close(fd);
    printf("\nTest completed.\n");
    return 0;
} 