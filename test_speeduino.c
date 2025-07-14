#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the speeduino plugin function declarations
extern void speeduino_plugin_init(void);
extern int speeduino_connect(const char *port);
extern int speeduino_read_realtime_data(char *buffer, int size);
extern int speeduino_write_config(const char *config_name, int value);
extern void speeduino_close(void);

int main(void) {
    printf("Testing Speeduino Plugin Functions...\n");
    
    // Test 1: Plugin initialization
    printf("1. Testing plugin initialization...\n");
    speeduino_plugin_init();
    printf("   Plugin initialized successfully\n");
    
    // Test 2: Connection attempt (will fail without hardware)
    printf("2. Testing connection (expected to fail without hardware)...\n");
    int result = speeduino_connect("/dev/ttyUSB0");
    if (result == 0) {
        printf("   Connection successful\n");
    } else {
        printf("   Connection failed (expected without hardware)\n");
    }
    
    // Test 3: Read realtime data (will fail without connection)
    printf("3. Testing realtime data read...\n");
    char buffer[256];
    int data_size = speeduino_read_realtime_data(buffer, sizeof(buffer));
    if (data_size > 0) {
        printf("   Read %d bytes of realtime data\n", data_size);
    } else {
        printf("   No realtime data available (expected without connection)\n");
    }
    
    // Test 4: Write config (will fail without connection)
    printf("4. Testing config write...\n");
    result = speeduino_write_config("test_param", 42);
    if (result == 0) {
        printf("   Config write successful\n");
    } else {
        printf("   Config write failed (expected without connection)\n");
    }
    
    // Test 5: Close connection
    printf("5. Testing connection close...\n");
    speeduino_close();
    printf("   Connection closed\n");
    
    printf("\nSpeeduino Plugin Test Complete!\n");
    printf("The plugin functions are available and can be called.\n");
    
    return 0;
}
