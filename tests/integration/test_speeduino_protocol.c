/*
 * Speeduino Protocol Test
 * Follows the official Speeduino protocol documentation exactly
 * Based on: https://wiki.speeduino.com/en/reference/Interface_Protocol
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>

int configure_serial_port(int fd, int baudrate) {
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(fd, &tty) != 0) {
        printf("Failed to get serial attributes\n");
        return -1;
    }
    
    // Set baud rate
    speed_t baud;
    switch (baudrate) {
        case 9600: baud = B9600; break;
        case 38400: baud = B38400; break;
        case 57600: baud = B57600; break;
        case 115200: baud = B115200; break;
        default: printf("Unsupported baud rate: %d\n", baudrate); return -1;
    }
    
    cfsetospeed(&tty, baud);
    cfsetispeed(&tty, baud);
    
    // Configure for 8N1
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    // Raw input/output
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Failed to set serial attributes\n");
        return -1;
    }
    
    return 0;
}

int send_command_and_read(int fd, const char* command, char* response, int max_response_len) {
    // Send command without line terminator (as per protocol)
    int cmd_len = strlen(command);
    int bytes_written = write(fd, command, cmd_len);
    if (bytes_written != cmd_len) {
        printf("Failed to send command '%s'\n", command);
        return -1;
    }
    
    printf("Sent command: '%s' (%d bytes)\n", command, cmd_len);
    
    // Wait for response
    usleep(100000); // 100ms delay
    
    // Read with timeout
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result <= 0) {
        printf("Timeout waiting for response\n");
        return -1;
    }
    
    int bytes_read = read(fd, response, max_response_len - 1);
    if (bytes_read <= 0) {
        printf("Failed to read response\n");
        return -1;
    }
    
    response[bytes_read] = '\0';
    return bytes_read;
}

void print_hex_dump(const char* data, int length) {
    printf("Response (%d bytes): ", length);
    for (int i = 0; i < length && i < 32; i++) {
        if (data[i] >= 32 && data[i] <= 126) {
            printf("%c", data[i]);
        } else {
            printf("[%02X]", (unsigned char)data[i]);
        }
    }
    printf("\n");
}

int test_baud_rate(int fd, int baudrate) {
    printf("\n=== Testing baud rate: %d ===\n", baudrate);
    
    if (configure_serial_port(fd, baudrate) != 0) {
        return -1;
    }
    
    char response[256];
    
    // Test 1: 'Q' command (should return 20-byte ASCII string)
    printf("\n1. Testing 'Q' command (version info)...\n");
    int result = send_command_and_read(fd, "Q", response, sizeof(response));
    if (result > 0) {
        print_hex_dump(response, result);
        
        // Check if it looks like Speeduino version string
        if (strstr(response, "speeduino") || strstr(response, "Speeduino")) {
            printf("✅ Found Speeduino version string!\n");
            return 0; // Success!
        } else if (result == 20) {
            printf("✅ Got 20-byte response (likely Speeduino version)\n");
            return 0;
        } else {
            printf("⚠️  Got response but doesn't match expected format\n");
        }
    }
    
    // Test 2: 'S' command (alternative version command)
    printf("\n2. Testing 'S' command (signature)...\n");
    result = send_command_and_read(fd, "S", response, sizeof(response));
    if (result > 0) {
        print_hex_dump(response, result);
        
        if (strstr(response, "speeduino") || strstr(response, "Speeduino")) {
            printf("✅ Found Speeduino signature!\n");
            return 0;
        } else if (result == 20) {
            printf("✅ Got 20-byte response (likely Speeduino signature)\n");
            return 0;
        }
    }
    
    // Test 3: 'A' command (real-time data - 120 bytes binary)
    printf("\n3. Testing 'A' command (real-time data)...\n");
    result = send_command_and_read(fd, "A", response, sizeof(response));
    if (result > 0) {
        print_hex_dump(response, result);
        
        if (result == 120) {
            printf("✅ Got 120-byte real-time data response!\n");
            return 0;
        } else if (result > 0) {
            printf("⚠️  Got %d-byte response (not 120 bytes)\n", result);
        }
    }
    
    return -1;
}

int main() {
    printf("=== Speeduino Protocol Test ===\n");
    printf("Following official protocol: https://wiki.speeduino.com/en/reference/Interface_Protocol\n\n");
    
    // Open serial port
    int fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("❌ Failed to open /dev/ttyACM0\n");
        printf("   Make sure Speeduino is connected and you have permissions\n");
        return 1;
    }
    
    printf("✅ Opened /dev/ttyACM0\n");
    
    // Test baud rates that we know work from our previous tests
    int baud_rates[] = {9600, 38400, 57600, 115200};
    int num_baud_rates = 4;
    
    for (int i = 0; i < num_baud_rates; i++) {
        if (test_baud_rate(fd, baud_rates[i]) == 0) {
            printf("\n🎯 SUCCESS: Speeduino responding at %d baud!\n", baud_rates[i]);
            printf("   This is the correct baud rate for your Speeduino.\n");
            close(fd);
            return 0;
        }
        
        // Wait between tests
        usleep(500000);
    }
    
    printf("\n❌ No successful communication at any baud rate\n");
    printf("   Check:\n");
    printf("   1. Speeduino is powered and connected\n");
    printf("   2. You have permissions (run: sudo usermod -a -G dialout $USER)\n");
    printf("   3. No other software is using the port\n");
    
    close(fd);
    return 1;
} 