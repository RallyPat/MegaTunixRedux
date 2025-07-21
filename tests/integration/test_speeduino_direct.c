/*
 * Simple Speeduino Communication Test
 * Test basic communication with Speeduino hardware
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libserialport.h>

int main() {
    struct sp_port *port;
    enum sp_return result;
    
    printf("=== Direct Speeduino Communication Test ===\n\n");
    
    // Get port by name
    printf("1. Opening /dev/ttyACM0...\n");
    result = sp_get_port_by_name("/dev/ttyACM0", &port);
    if (result != SP_OK) {
        printf("❌ Failed to find port: %s\n", sp_last_error_message());
        return 1;
    }
    
    // Open port
    result = sp_open(port, SP_MODE_READ_WRITE);
    if (result != SP_OK) {
        printf("❌ Failed to open port: %s\n", sp_last_error_message());
        sp_free_port(port);
        return 1;
    }
    
    printf("✅ Port opened successfully\n");
    
    // Configure port
    sp_set_baudrate(port, 115200);
    sp_set_bits(port, 8);
    sp_set_parity(port, SP_PARITY_NONE);
    sp_set_stopbits(port, 1);
    sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
    
    printf("2. Port configured (115200 8N1)\n");
    
    // Flush any existing data
    sp_flush(port, SP_BUF_BOTH);
    
    // Test Speeduino version command
    printf("3. Sending Speeduino version command 'V'...\n");
    char cmd = 'V';
    result = sp_blocking_write(port, &cmd, 1, 1000);
    if (result < 0) {
        printf("❌ Failed to send command: %s\n", sp_last_error_message());
    } else {
        printf("✅ Command sent (%d bytes)\n", result);
        
        // Read response
        printf("4. Reading response...\n");
        char response[256];
        result = sp_blocking_read(port, response, sizeof(response)-1, 2000);
        if (result > 0) {
            response[result] = '\0';
            printf("✅ Received %d bytes: '%s'\n", result, response);
            
            // Check if it looks like Speeduino
            if (strstr(response, "speeduino") || strstr(response, "Speeduino") || 
                (result >= 3 && response[0] >= '0' && response[0] <= '9')) {
                printf("🎯 Response looks like Speeduino!\n");
            } else {
                printf("⚠️  Response doesn't match expected Speeduino format\n");
            }
        } else {
            printf("❌ No response received (timeout or error)\n");
        }
    }
    
    // Cleanup
    sp_close(port);
    sp_free_port(port);
    
    printf("\n=== Test completed ===\n");
    printf("If you saw a response, your Speeduino is communicating!\n");
    printf("Now try: ./run_speeduino_test.sh for full GUI test\n");
    
    return 0;
}
