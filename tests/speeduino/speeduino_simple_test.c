#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define PORT "/dev/ttyACM0"

int main() {
    printf("=== Simple Speeduino Test ===\n");
    
    // Open port with minimal flags
    int fd = open(PORT, O_RDWR);
    if (fd < 0) {
        perror("Failed to open port");
        return 1;
    }
    
    printf("Port opened successfully\n");
    
    // Get current settings
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr failed");
        close(fd);
        return 1;
    }
    
    // Set baud rate only
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    // Apply settings
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr failed");
        close(fd);
        return 1;
    }
    
    printf("Baud rate set to 115200\n");
    
    // Test simple commands
    const char* commands[] = {"Q", "S", "A", "V"};
    const char* names[] = {"Query", "Signature", "Data", "Version"};
    
    for (int i = 0; i < 4; i++) {
        printf("\n--- Testing %s command ---\n", names[i]);
        
        // Send command
        printf("Sending: '%s'\n", commands[i]);
        int written = write(fd, commands[i], strlen(commands[i]));
        printf("Written: %d bytes\n", written);
        
        // Wait a bit
        usleep(500000); // 500ms
        
        // Try to read
        char buffer[256];
        int total_read = 0;
        
        // Try multiple reads
        for (int attempt = 0; attempt < 5; attempt++) {
            int bytes_read = read(fd, buffer + total_read, sizeof(buffer) - total_read - 1);
            if (bytes_read > 0) {
                total_read += bytes_read;
                printf("Read %d bytes (total: %d)\n", bytes_read, total_read);
            } else if (bytes_read == 0) {
                printf("No data available\n");
                break;
            } else {
                perror("Read error");
                break;
            }
            
            usleep(100000); // 100ms between reads
        }
        
        if (total_read > 0) {
            buffer[total_read] = '\0';
            printf("Response: ");
            for (int j = 0; j < total_read; j++) {
                printf("%02X ", (unsigned char)buffer[j]);
            }
            printf(" | ");
            for (int j = 0; j < total_read; j++) {
                if (buffer[j] >= 32 && buffer[j] <= 126) {
                    printf("%c", buffer[j]);
                } else {
                    printf(".");
                }
            }
            printf("\n");
        } else {
            printf("No response received\n");
        }
    }
    
    close(fd);
    printf("\nTest completed.\n");
    return 0;
} 