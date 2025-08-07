/*
 * Termios Speeduino Test
 * Uses the exact same termios approach as our application
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

int main() {
    printf("=== Termios Speeduino Test ===\n");
    printf("Using exact same method as our application\n\n");
    
    // Open serial port (same as our application)
    int fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("❌ Failed to open /dev/ttyACM0\n");
        return 1;
    }
    
    printf("✅ Opened /dev/ttyACM0\n");
    
    // Configure serial port (same as our application)
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        printf("❌ Failed to get serial attributes\n");
        close(fd);
        return 1;
    }
    
    // Set baud rate to 115200 (same as our application)
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
        printf("❌ Failed to set serial attributes\n");
        close(fd);
        return 1;
    }
    
    printf("✅ Configured serial port for Speeduino at 115200 baud\n");
    
    // Send 'Q' command (same as our application)
    printf("Sending 'Q' command...\n");
    char cmd = 'Q';
    int bytes_written = write(fd, &cmd, 1);
    if (bytes_written != 1) {
        printf("❌ Failed to send command\n");
        close(fd);
        return 1;
    }
    
    printf("✅ Command sent (%d bytes)\n", bytes_written);
    
    // Wait for response (same as our application)
    printf("Waiting for response...\n");
    usleep(100000); // 100ms delay
    
    // Read response with timeout (same as our application)
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    
    int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result <= 0) {
        printf("❌ Timeout waiting for response\n");
        close(fd);
        return 1;
    }
    
    // Read available data (same as our application)
    char response[256];
    int bytes_read = read(fd, response, sizeof(response) - 1);
    if (bytes_read <= 0) {
        printf("❌ Failed to read response\n");
        close(fd);
        return 1;
    }
    
    response[bytes_read] = '\0';
    printf("✅ Received %d bytes: ", bytes_read);
    
    // Print response
    for (int i = 0; i < bytes_read && i < 32; i++) {
        if (response[i] >= 32 && response[i] <= 126) {
            printf("%c", response[i]);
        } else {
            printf("[%02X]", (unsigned char)response[i]);
        }
    }
    printf("\n");
    
    // Check if it looks like Speeduino
    if (strstr(response, "speeduino") || strstr(response, "Speeduino")) {
        printf("🎯 Found Speeduino version string!\n");
        close(fd);
        return 0;
    } else if (bytes_read == 20) {
        printf("🎯 Got 20-byte response (likely Speeduino version)\n");
        close(fd);
        return 0;
    } else {
        printf("⚠️  Got response but doesn't match expected format\n");
    }
    
    close(fd);
    return 1;
} 