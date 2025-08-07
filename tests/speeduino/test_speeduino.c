#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>
#include <stdint.h>

int main() {
    const char* port = "/dev/ttyACM0";
    
    printf("Testing Speeduino communication on %s\n", port);
    
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
    
    // Test different commands
    const char* commands[] = {"Q", "S", "V", "A", "r"};
    const char* names[] = {"Query", "Status", "Version", "Data", "Real-time"};
    
    for (int i = 0; i < 5; i++) {
        printf("\n--- Testing %s command ('%s') ---\n", names[i], commands[i]);
        
        // Flush buffer
        tcflush(fd, TCIOFLUSH);
        
        // Send command
        int cmd_len = strlen(commands[i]);
        int bytes_written = write(fd, commands[i], cmd_len);
        if (bytes_written != cmd_len) {
            printf("Failed to write command: %s\n", strerror(errno));
            continue;
        }
        
        printf("Sent %d bytes\n", bytes_written);
        
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
    
    close(fd);
    printf("\nTest completed\n");
    return 0;
} 