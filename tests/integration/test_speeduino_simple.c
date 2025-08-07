/*
 * Simple Speeduino Test
 * Uses the same method as our working test programs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libserialport.h>

int main() {
    struct sp_port *port;
    enum sp_return result;
    
    printf("=== Simple Speeduino Test ===\n\n");
    
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
    
    // Test different baud rates
    int baud_rates[] = {9600, 38400, 57600, 115200};
    const char* baud_names[] = {"9600", "38400", "57600", "115200"};
    
    for (int b = 0; b < 4; b++) {
        printf("\n--- Testing baud rate: %s ---\n", baud_names[b]);
        
        // Configure port
        sp_set_baudrate(port, baud_rates[b]);
        sp_set_bits(port, 8);
        sp_set_parity(port, SP_PARITY_NONE);
        sp_set_stopbits(port, 1);
        sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
        
        // Flush any existing data
        sp_flush(port, SP_BUF_BOTH);
        
        // Test Speeduino 'Q' command (version)
        printf("Testing 'Q' command...\n");
        char cmd_q = 'Q';
        result = sp_blocking_write(port, &cmd_q, 1, 1000);
        if (result < 0) {
            printf("❌ Failed to send 'Q' command: %s\n", sp_last_error_message());
        } else {
            printf("✅ 'Q' command sent (%d bytes)\n", result);
            
            // Read response
            char response[256];
            result = sp_blocking_read(port, response, sizeof(response)-1, 2000);
            if (result > 0) {
                response[result] = '\0';
                printf("✅ Received %d bytes: ", result);
                
                // Print hex dump
                for (int i = 0; i < result && i < 32; i++) {
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
                    sp_close(port);
                    sp_free_port(port);
                    return 0;
                } else if (result == 20) {
                    printf("🎯 Got 20-byte response (likely Speeduino version)\n");
                    sp_close(port);
                    sp_free_port(port);
                    return 0;
                }
            } else {
                printf("❌ No response to 'Q' command\n");
            }
        }
        
        // Test Speeduino 'A' command (real-time data)
        printf("Testing 'A' command...\n");
        char cmd_a = 'A';
        result = sp_blocking_write(port, &cmd_a, 1, 1000);
        if (result < 0) {
            printf("❌ Failed to send 'A' command: %s\n", sp_last_error_message());
        } else {
            printf("✅ 'A' command sent (%d bytes)\n", result);
            
            // Read response
            char response[256];
            result = sp_blocking_read(port, response, sizeof(response)-1, 2000);
            if (result > 0) {
                response[result] = '\0';
                printf("✅ Received %d bytes: ", result);
                
                // Print hex dump
                for (int i = 0; i < result && i < 32; i++) {
                    if (response[i] >= 32 && response[i] <= 126) {
                        printf("%c", response[i]);
                    } else {
                        printf("[%02X]", (unsigned char)response[i]);
                    }
                }
                printf("\n");
                
                if (result == 120) {
                    printf("🎯 Got 120-byte real-time data response!\n");
                    sp_close(port);
                    sp_free_port(port);
                    return 0;
                }
            } else {
                printf("❌ No response to 'A' command\n");
            }
        }
        
        // Wait between baud rate tests
        usleep(500000);
    }
    
    // Cleanup
    sp_close(port);
    sp_free_port(port);
    
    printf("\n=== Test completed ===\n");
    printf("No successful Speeduino communication found.\n");
    printf("The device is responding but not with expected Speeduino protocol.\n");
    
    return 1;
} 