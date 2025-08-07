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

int main() {
    printf("=== TunerStudio Communication Capture Tool ===\n");
    printf("This tool will monitor the serial port to see what TunerStudio sends/receives\n");
    printf("Instructions:\n");
    printf("1. Run this program first\n");
    printf("2. Open TunerStudio and connect to Speeduino\n");
    printf("3. Watch the captured communication below\n");
    printf("4. Press Ctrl+C to stop\n\n");
    
    int fd = open(PORT, O_RDWR | O_NOCTTY | O_NONBLOCK);
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
    
    printf("Serial port configured. Waiting for TunerStudio communication...\n\n");
    
    unsigned char buffer[1024];
    fd_set readfds;
    struct timeval timeout;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (result < 0) {
            perror("select failed");
            break;
        } else if (result == 0) {
            // Timeout - no data
            continue;
        }
        
        if (FD_ISSET(fd, &readfds)) {
            int bytes_read = read(fd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                printf("[RECEIVED] ");
                hex_dump("", buffer, bytes_read);
            }
        }
    }
    
    close(fd);
    return 0;
} 