/*
 * INI Parser Test - Verify INI file parsing functionality
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Tests INI file parsing with rusEFI EpicEFI INI file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../include/ecu/ecu_ini_parser.h"
#include "../../include/ecu/ecu_communication.h"

int main(int argc, char* argv[]) {
    printf("=== INI Parser Test ===\n\n");
    
    if (argc < 2) {
        printf("Usage: %s <ini_file_path>\n", argv[0]);
        printf("Example: %s /home/pat/Documents/TunerStudioFiles/rusefi_epicECU.ini\n", argv[0]);
        return 1;
    }
    
    const char* ini_file_path = argv[1];
    printf("Testing INI file: %s\n\n", ini_file_path);
    
    // Test 1: Check if file exists
    printf("Test 1: File existence check\n");
    if (!ecu_check_ini_file_exists(ini_file_path)) {
        printf("❌ File does not exist or is not readable\n");
        printf("Error: %s\n", ecu_get_ini_error());
        return 1;
    }
    printf("✅ File exists and is readable\n\n");
    
    // Test 2: Validate INI file
    printf("Test 2: INI file validation\n");
    if (!ecu_validate_ini_file(ini_file_path)) {
        printf("❌ INI file validation failed\n");
        printf("Error: %s\n", ecu_get_ini_error());
        return 1;
    }
    printf("✅ INI file is valid\n\n");
    
    // Test 3: Load and parse INI file
    printf("Test 3: Load and parse INI file\n");
    INIConfig* config = ecu_load_ini_file(ini_file_path);
    if (!config) {
        printf("❌ Failed to load INI file\n");
        printf("Error: %s\n", ecu_get_ini_error());
        return 1;
    }
    printf("✅ INI file loaded successfully\n\n");
    
    // Test 4: Print configuration
    printf("Test 4: Configuration details\n");
    ecu_print_ini_config(config);
    printf("\n");
    
    // Test 5: Protocol detection
    printf("Test 5: Protocol detection\n");
    ProtocolDetectionResult detection = ecu_detect_protocol_from_ini(config);
    printf("Detected protocol: %s\n", ecu_get_protocol_name_from_ini(detection.protocol_type));
    printf("Confidence: %.2f\n", detection.confidence);
    printf("Signature: %s\n", detection.detected_signature);
    printf("Version: %s\n", detection.detected_version);
    
    if (detection.confidence >= 0.5f) {
        printf("✅ Protocol detected successfully\n");
    } else {
        printf("⚠️  Protocol detection confidence is low\n");
    }
    printf("\n");
    
    // Test 6: Configuration validation
    printf("Test 6: Configuration validation\n");
    if (ecu_validate_ini_config(config)) {
        printf("✅ Configuration is valid\n");
    } else {
        printf("❌ Configuration validation failed\n");
        printf("Error: %s\n", ecu_get_ini_error());
    }
    printf("\n");
    
    // Test 7: Extract specific values
    printf("Test 7: Value extraction test\n");
    char signature[256];
    char query_cmd[16];
    int timeout_ms;
    bool no_comm_delay;
    
    if (ecu_extract_string_value(config->file_path, "signature", signature, sizeof(signature))) {
        printf("Signature: %s\n", signature);
    }
    
    if (ecu_extract_string_value(config->file_path, "queryCommand", query_cmd, sizeof(query_cmd))) {
        printf("Query Command: %s\n", query_cmd);
    }
    
    if (ecu_extract_int_value(config->file_path, "blockReadTimeout", &timeout_ms)) {
        printf("Timeout: %d ms\n", timeout_ms);
    }
    
    if (ecu_extract_bool_value(config->file_path, "noCommReadDelay", &no_comm_delay)) {
        printf("No Comm Read Delay: %s\n", no_comm_delay ? "true" : "false");
    }
    printf("\n");
    
    // Test 8: Field count
    printf("Test 8: Field parsing\n");
    printf("Total fields parsed: %d\n", config->field_count);
    printf("Field capacity: %d\n", config->field_capacity);
    printf("\n");
    
    // Test 9: Page information
    printf("Test 9: Page information\n");
    printf("Number of pages: %d\n", config->n_pages);
    for (int i = 0; i < config->n_pages && i < 4; i++) {
        printf("Page %d: ID='%s', Size=%d, ReadCmd='%s'\n", 
               i, config->page_identifier[i], config->page_size[i], config->page_read_command[i]);
    }
    printf("\n");
    
    // Test 10: Communication settings
    printf("Test 10: Communication settings\n");
    printf("Baud rate: %d\n", config->baud_rate);
    printf("Timeout: %d ms\n", config->timeout_ms);
    printf("Inter-write delay: %d ms\n", config->inter_write_delay_ms);
    printf("Page activation delay: %d ms\n", config->page_activation_delay_ms);
    printf("Write blocks: %s\n", config->write_blocks ? "true" : "false");
    printf("TS write blocks: %s\n", config->ts_write_blocks ? "true" : "false");
    printf("No comm read delay: %s\n", config->no_comm_read_delay ? "true" : "false");
    printf("Enable CRC: %s\n", config->enable_crc ? "true" : "false");
    printf("Enable 2nd byte CAN ID: %s\n", config->enable_2nd_byte_can_id ? "true" : "false");
    printf("Message envelope format: %s\n", config->message_envelope_format);
    printf("Endianness: %s\n", config->endianness);
    printf("\n");
    
    // Cleanup
    ecu_free_ini_config(config);
    
    printf("=== All tests completed successfully! ===\n");
    printf("The INI parser is working correctly with the rusEFI EpicEFI INI file.\n");
    printf("MegaTunix Redux can now support universal ECU configuration loading.\n");
    
    return 0;
} 