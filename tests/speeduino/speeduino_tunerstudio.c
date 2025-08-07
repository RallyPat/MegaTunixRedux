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

void test_tunerstudio_command(int fd, const char* command, const char* name) {
    printf("\n=== Testing %s command: '%s' ===\n", name, command);
    
    // Flush buffer
    tcflush(fd, TCIOFLUSH);
    
    // Send ASCII command
    printf("Sending command: '%s'\n", command);
    int cmd_len = strlen(command);
    int bytes_written = write(fd, command, cmd_len);
    if (bytes_written != cmd_len) {
        printf("Failed to write command: %s\n", strerror(errno));
        return;
    }
    
    printf("Command sent successfully!\n");
    
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

void test_tunerstudio_data_command(int fd) {
    printf("\n=== Testing TunerStudio Data Command ===\n");
    
    // The INI shows: ochGetCommand = "r\$tsCanId\x30%2o%2c"
    // This means: "r" + CAN_ID + 0x30 + offset + count
    // For basic test, let's try: "r0000000000" (r + CAN_ID=0 + offset=0 + count=130)
    
    const char* data_commands[] = {
        "r0000000000",  // r + CAN_ID=0 + offset=0 + count=0
        "r0000000082",  // r + CAN_ID=0 + offset=0 + count=130 (0x82)
        "r0000000001",  // r + CAN_ID=0 + offset=0 + count=1
        "r0000000002",  // r + CAN_ID=0 + offset=0 + count=2
    };
    
    for (int i = 0; i < 4; i++) {
        test_tunerstudio_command(fd, data_commands[i], "Data");
        usleep(1000000); // 1 second between attempts
    }
}

int main() {
    const char* port = "/dev/ttyACM0";
    
    printf("TunerStudio Protocol Speeduino Test on %s\n", port);
    printf("Based on INI file analysis: queryCommand='Q', signature='speeduino 202501'\n\n");
    
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
    
    // Test basic TunerStudio commands
    test_tunerstudio_command(fd, "Q", "Query");
    usleep(1000000);
    
    test_tunerstudio_command(fd, "S", "Version Info");
    usleep(1000000);
    
    test_tunerstudio_command(fd, "V", "Version");
    usleep(1000000);
    
    test_tunerstudio_command(fd, "A", "Data (legacy)");
    usleep(1000000);
    
    // Test TunerStudio data command
    test_tunerstudio_data_command(fd);
    
    close(fd);
    printf("\nTest completed.\n");
    return 0;
} 