/*
 * Standalone Speeduino Connection Test
 * This program tests direct serial communication with a Speeduino ECU
 * without any GUI dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

// Speeduino command constants
#define SPEEDUINO_SIGNATURE_CMD 'S'
#define SPEEDUINO_VERSION_CMD 'V'
#define SPEEDUINO_REALTIME_CMD 'A'
#define SPEEDUINO_Q_CMD 'Q'

// Function prototypes
int open_serial_port(const char *device, int baud_rate);
int configure_port(int fd, int baud_rate);
int send_command(int fd, char cmd);
int read_response(int fd, char *buffer, int max_len, int timeout_ms);
void print_hex_dump(const char *data, int len);
void test_speeduino_connection(const char *device, int baud_rate);

int main(int argc, char *argv[])
{
    const char *device = "/dev/ttyACM0";  // Default device
    int baud_rate = 115200;  // Default baud rate
    
    printf("=== Standalone Speeduino Connection Test ===\n");
    printf("This test attempts direct serial communication with a Speeduino ECU\n\n");
    
    // Parse command line arguments
    if (argc >= 2) {
        device = argv[1];
    }
    if (argc >= 3) {
        baud_rate = atoi(argv[2]);
    }
    
    printf("Testing device: %s\n", device);
    printf("Baud rate: %d\n", baud_rate);
    printf("\n");
    
    // Check if device exists
    if (access(device, F_OK) != 0) {
        printf("❌ Device %s does not exist\n", device);
        printf("Available devices:\n");
        system("ls -la /dev/ttyACM* /dev/ttyUSB* /dev/ttyS* 2>/dev/null || echo '  No serial devices found'");
        return 1;
    }
    
    // Check if device is readable/writable
    if (access(device, R_OK | W_OK) != 0) {
        printf("❌ Cannot access %s (permission denied)\n", device);
        printf("Try: sudo chmod 666 %s\n", device);
        printf("Or add user to dialout group: sudo usermod -a -G dialout $USER\n");
        return 1;
    }
    
    printf("✅ Device %s exists and is accessible\n", device);
    
    // Test the connection
    test_speeduino_connection(device, baud_rate);
    
    return 0;
}

void test_speeduino_connection(const char *device, int baud_rate)
{
    int fd;
    char buffer[256];
    int bytes_read;
    
    printf("\n--- Opening serial connection ---\n");
    
    fd = open_serial_port(device, baud_rate);
    if (fd < 0) {
        printf("❌ Failed to open serial port\n");
        return;
    }
    
    printf("✅ Serial port opened successfully\n");
    
    // Give the device time to initialize
    printf("⏳ Waiting for device to initialize...\n");
    sleep(2);
    
    // Test 1: Send signature command
    printf("\n--- Test 1: Request ECU Signature ---\n");
    if (send_command(fd, SPEEDUINO_SIGNATURE_CMD) < 0) {
        printf("❌ Failed to send signature command\n");
        close(fd);
        return;
    }
    
    bytes_read = read_response(fd, buffer, sizeof(buffer), 2000);
    if (bytes_read > 0) {
        printf("✅ Received signature response (%d bytes):\n", bytes_read);
        buffer[bytes_read] = '\0';
        printf("  Text: \"%s\"\n", buffer);
        printf("  Hex: ");
        print_hex_dump(buffer, bytes_read);
        printf("\n");
    } else {
        printf("❌ No response to signature command\n");
    }
    
    // Test 2: Send version command
    printf("\n--- Test 2: Request Firmware Version ---\n");
    if (send_command(fd, SPEEDUINO_VERSION_CMD) < 0) {
        printf("❌ Failed to send version command\n");
        close(fd);
        return;
    }
    
    bytes_read = read_response(fd, buffer, sizeof(buffer), 2000);
    if (bytes_read > 0) {
        printf("✅ Received version response (%d bytes):\n", bytes_read);
        buffer[bytes_read] = '\0';
        printf("  Text: \"%s\"\n", buffer);
        printf("  Hex: ");
        print_hex_dump(buffer, bytes_read);
        printf("\n");
    } else {
        printf("❌ No response to version command\n");
    }
    
    // Test 3: Send Q command (general query)
    printf("\n--- Test 3: Send Q Command ---\n");
    if (send_command(fd, SPEEDUINO_Q_CMD) < 0) {
        printf("❌ Failed to send Q command\n");
        close(fd);
        return;
    }
    
    bytes_read = read_response(fd, buffer, sizeof(buffer), 2000);
    if (bytes_read > 0) {
        printf("✅ Received Q response (%d bytes):\n", bytes_read);
        printf("  Hex: ");
        print_hex_dump(buffer, bytes_read);
        printf("\n");
    } else {
        printf("❌ No response to Q command\n");
    }
    
    // Test 4: Send realtime data command
    printf("\n--- Test 4: Request Realtime Data ---\n");
    if (send_command(fd, SPEEDUINO_REALTIME_CMD) < 0) {
        printf("❌ Failed to send realtime command\n");
        close(fd);
        return;
    }
    
    bytes_read = read_response(fd, buffer, sizeof(buffer), 2000);
    if (bytes_read > 0) {
        printf("✅ Received realtime data (%d bytes):\n", bytes_read);
        printf("  Hex: ");
        print_hex_dump(buffer, bytes_read);
        printf("\n");
        
        // Try to interpret some basic values
        if (bytes_read >= 4) {
            unsigned char *data = (unsigned char *)buffer;
            printf("  Interpreted data:\n");
            printf("    Byte 0: %d (0x%02X)\n", data[0], data[0]);
            printf("    Byte 1: %d (0x%02X)\n", data[1], data[1]);
            printf("    Byte 2: %d (0x%02X)\n", data[2], data[2]);
            printf("    Byte 3: %d (0x%02X)\n", data[3], data[3]);
        }
    } else {
        printf("❌ No response to realtime command\n");
    }
    
    close(fd);
    printf("\n--- Test Complete ---\n");
}

int open_serial_port(const char *device, int baud_rate)
{
    int fd;
    
    fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("❌ Error opening %s: %s\n", device, strerror(errno));
        return -1;
    }
    
    if (configure_port(fd, baud_rate) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}

int configure_port(int fd, int baud_rate)
{
    struct termios tty;
    
    if (tcgetattr(fd, &tty) < 0) {
        printf("❌ Error getting terminal attributes: %s\n", strerror(errno));
        return -1;
    }
    
    // Set baud rate
    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        default:
            printf("❌ Unsupported baud rate: %d\n", baud_rate);
            return -1;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    // Configure for 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;   // No parity
    tty.c_cflag &= ~CSTOPB;   // 1 stop bit
    tty.c_cflag &= ~CSIZE;    // Clear size bits
    tty.c_cflag |= CS8;       // 8 data bits
    tty.c_cflag &= ~CRTSCTS;  // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL;  // Enable receiver, ignore modem lines
    
    // Configure input modes
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    
    // Configure output modes
    tty.c_oflag &= ~OPOST;
    
    // Configure local modes
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    
    // Set timeouts
    tty.c_cc[VMIN] = 0;   // Non-blocking read
    tty.c_cc[VTIME] = 10; // 1 second timeout
    
    if (tcsetattr(fd, TCSANOW, &tty) < 0) {
        printf("❌ Error setting terminal attributes: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

int send_command(int fd, char cmd)
{
    printf("📤 Sending command: '%c' (0x%02X)\n", cmd, (unsigned char)cmd);
    
    if (write(fd, &cmd, 1) != 1) {
        printf("❌ Error writing command: %s\n", strerror(errno));
        return -1;
    }
    
    // Flush output
    tcdrain(fd);
    return 0;
}

int read_response(int fd, char *buffer, int max_len, int timeout_ms)
{
    fd_set readfds;
    struct timeval timeout;
    int total_bytes = 0;
    int bytes_read;
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    printf("📥 Waiting for response (timeout: %d ms)...\n", timeout_ms);
    
    while (total_bytes < max_len - 1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (result < 0) {
            printf("❌ Select error: %s\n", strerror(errno));
            break;
        } else if (result == 0) {
            printf("⏱️  Timeout waiting for response\n");
            break;
        }
        
        bytes_read = read(fd, buffer + total_bytes, max_len - total_bytes - 1);
        if (bytes_read < 0) {
            printf("❌ Error reading response: %s\n", strerror(errno));
            break;
        } else if (bytes_read == 0) {
            printf("📭 No more data available\n");
            break;
        }
        
        total_bytes += bytes_read;
        printf("📥 Received %d bytes (total: %d)\n", bytes_read, total_bytes);
        
        // For some responses, we might want to stop early
        // For now, we'll continue until timeout or buffer full
    }
    
    return total_bytes;
}

void print_hex_dump(const char *data, int len)
{
    for (int i = 0; i < len; i++) {
        printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n       ");
        }
    }
    if (len % 16 != 0) {
        printf("\n");
    }
}
