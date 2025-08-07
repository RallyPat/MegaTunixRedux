#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <errno.h>

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

int main() {
    printf("=== Speeduino Listen Test ===\n");
    printf("Just listening to see if Speeduino sends any data...\n\n");
    
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
    
    // 8N1 configuration
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    // Input flags
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Output flags
    tty.c_oflag &= ~OPOST;
    
    // Control characters
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr failed");
        close(fd);
        return 1;
    }
    
    printf("Serial port configured at 115200 baud (8N1)\n");
    printf("Waiting 1 second after port open...\n");
    sleep(1);
    
    printf("Listening for 10 seconds...\n");
    
    // Listen for 10 seconds
    for (int second = 1; second <= 10; second++) {
        printf("Second %d: ", second);
        
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (result > 0 && FD_ISSET(fd, &readfds)) {
            unsigned char buffer[256];
            int bytes_read = read(fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                printf("Received %d bytes!\n", bytes_read);
                hex_dump("Data", buffer, bytes_read);
            } else {
                printf("No data\n");
            }
        } else {
            printf("No data\n");
        }
    }
    
    close(fd);
    printf("\nListen test completed.\n");
    return 0;
} 