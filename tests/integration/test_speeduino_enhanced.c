/*
 * Enhanced Speeduino Communication Test
 * Try different baud rates and commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libserialport.h>

void test_baud_rate(int baudrate) {
    struct sp_port *port;
    enum sp_return result;
    
    printf("\n--- Testing baud rate: %d ---\n", baudrate);
    
    result = sp_get_port_by_name("/dev/ttyACM0", &port);
    if (result != SP_OK) {
        printf("❌ Failed to find port\n");
        return;
    }
    
    result = sp_open(port, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        printf("❌ Failed to open port\n");
        sp_free_port(port);
        return;
    }
    
    // Configure port
    sp_set_baudrate(port, baudrate);
    sp_set_bits(port, 8);
    sp_set_parity(port, SP_PARITY_NONE);
    sp_set_stopbits(port, 1);
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
    sp_flush(port, SP_BUF_BOTH);
    
    // Wait a moment for device to settle
    usleep(100000);
    
    // Try version command
    char cmd = 'V';
    result = sp_blocking_write(port, &cmd, 1, 1000);
    if (result > 0) {
        char response[256];
        result = sp_blocking_read(port, response, sizeof(response)-1, 1000);
        if (result > 0) {
            response[result] = '\0';
            printf("Response (%d bytes): ", result);
            
            // Print readable characters and hex for non-readable
            for (int i = 0; i < result && i < 64; i++) {
                if (response[i] >= 32 && response[i] <= 126) {
                    printf("%c", response[i]);
                } else {
                    printf("[%02X]", (unsigned char)response[i]);
                }
            }
            printf("\n");
            
            // Check for Speeduino patterns
            if (strstr(response, "speeduino") || strstr(response, "Speeduino")) {
                printf("🎯 Found Speeduino text!\n");
            } else if (result >= 8) {
                printf("📊 Got binary data - might be real-time data\n");
            }
        } else {
            printf("No response\n");
        }
    }
    
    sp_close(port);
    sp_free_port(port);
}

int main() {
    printf("=== Enhanced Speeduino Communication Test ===\n");
    
    // Common Arduino/Speeduino baud rates
    int baud_rates[] = {9600, 38400, 57600, 115200, 250000, 0};
    
    for (int i = 0; baud_rates[i] != 0; i++) {
        test_baud_rate(baud_rates[i]);
        usleep(500000);  // Wait between tests
    }
    
    printf("\n=== Summary ===\n");
    printf("- Device is responding at /dev/ttyACM0\n");
    printf("- Try the GUI test next: ./run_speeduino_test.sh\n");
    printf("- MegaTunix Redux will try auto-detection with multiple baud rates\n");
    
    return 0;
}
