#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <stdint.h>

int main() {
    const char* port = "/dev/ttyACM0";
    
    printf("Simple Speeduino Test\n");
    printf("Opening %s...\n", port);
    
    // Open serial port
    int fd = open(port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        printf("Failed to open %s: %s\n", port, strerror(errno));
        return 1;
    }
    
    printf("Port opened successfully\n");
    
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
    
    printf("Serial port configured at 115200 baud\n");
    
    // Flush any existing data
    tcflush(fd, TCIOFLUSH);
    
    // Test 1: Send 'Q' command
    printf("\n=== Test 1: Sending 'Q' command ===\n");
    printf("Sending: 'Q'\n");
    write(fd, "Q", 1);
    
    // Try to read immediately
    printf("Reading response...\n");
    uint8_t buffer[256];
    int bytes_read = read(fd, buffer, sizeof(buffer));
    printf("Read %d bytes\n", bytes_read);
    
    if (bytes_read > 0) {
        printf("Response: ");
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c", buffer[i]);
            } else {
                printf("[%02X]", buffer[i]);
            }
        }
        printf("\n");
    }
    
    // Wait a bit
    usleep(1000000);
    
    // Test 2: Send 'S' command
    printf("\n=== Test 2: Sending 'S' command ===\n");
    printf("Sending: 'S'\n");
    write(fd, "S", 1);
    
    // Try to read immediately
    printf("Reading response...\n");
    bytes_read = read(fd, buffer, sizeof(buffer));
    printf("Read %d bytes\n", bytes_read);
    
    if (bytes_read > 0) {
        printf("Response: ");
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c", buffer[i]);
            } else {
                printf("[%02X]", buffer[i]);
            }
        }
        printf("\n");
    }
    
    // Wait a bit
    usleep(1000000);
    
    // Test 3: Send binary packet
    printf("\n=== Test 3: Sending binary packet ===\n");
    uint8_t packet[] = {0x72, 0x51, 0x00, 0x20, 0x3C, 0x03}; // Our binary 'Q' command
    printf("Sending binary packet: ");
    for (int i = 0; i < 6; i++) {
        printf("[%02X]", packet[i]);
    }
    printf("\n");
    
    write(fd, packet, 6);
    
    // Try to read immediately
    printf("Reading response...\n");
    bytes_read = read(fd, buffer, sizeof(buffer));
    printf("Read %d bytes\n", bytes_read);
    
    if (bytes_read > 0) {
        printf("Response: ");
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c", buffer[i]);
            } else {
                printf("[%02X]", buffer[i]);
            }
        }
        printf("\n");
    }
    
    close(fd);
    printf("\nTest completed.\n");
    return 0;
} 