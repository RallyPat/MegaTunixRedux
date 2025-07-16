#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "include/ecu_detector.h"

int main()
{
    printf("=== ECU Detection Test ===\n");
    
    // Test the fixed detection logic
    printf("Testing ECU detection on /dev/ttyACM0 at 115200 baud...\n");
    
    EcuDetectionResult *result = ecu_detector_test_device("/dev/ttyACM0", 115200);
    
    if (result) {
        printf("✅ ECU detected!\n");
        printf("   Name: %s\n", result->ecu_name);
        printf("   Type: %d\n", result->ecu_type);
        printf("   Device: %s\n", result->device_path);
        printf("   Baud: %d\n", result->baud_rate);
        printf("   Signature: %s\n", result->signature);
        printf("   Confidence: %d%%\n", result->confidence);
        
        // Clean up
        g_free(result->ecu_name);
        g_free(result->device_path);
        g_free(result->signature);
        g_free(result);
        
        return 0;
    } else {
        printf("❌ No ECU detected\n");
        return 1;
    }
}
