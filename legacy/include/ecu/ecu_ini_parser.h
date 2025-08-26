/*
 * ECU INI Parser - Universal ECU Configuration Support
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Supports TunerStudio INI file parsing for Speeduino, rusEFI, MegaSquirt, and LibreEMS.
 */

#ifndef ECU_INI_PARSER_H
#define ECU_INI_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// INI Field Definition
typedef struct {
    char name[64];
    char type[16];
    int offset;
    char units[32];
    float scale;
    float translate;
    float min_value;
    float max_value;
    int digits;
    char description[256];
} INIField;

// Enhanced table information structure for TunerStudio compatibility
typedef struct {
    char name[64];
    char display_name[64];
    int width;
    int height;
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    char x_axis_name[32];
    char y_axis_name[32];
    char z_axis_name[32];
    char x_units[16];
    char y_units[16];
    char z_units[16];
    int offset;
    int data_type; // 0=U08, 1=U16, 2=S16, 3=F32, etc.
    float scale;
    float min_value;
    float max_value;
    int digits;
    bool enabled;
    bool editable;
    char x_axis_units[16];
    char y_axis_units[16];
    char z_axis_units[16];
    float x_axis_scale;
    float y_axis_scale;
    float z_axis_scale;
    int x_axis_offset;
    int y_axis_offset;
    int z_axis_offset;
} INITableInfo;

// Enhanced INI configuration structure
typedef struct {
    char signature[256];
    char ecu_name[64];
    char ecu_version[64];
    char protocol_name[64];
    int protocol_type;
    int baud_rate;
    int timeout_ms;
    bool auto_connect;
    bool auto_reconnect;
    int reconnect_interval;
    
    // Enhanced table support
    INITableInfo* tables;
    int table_count;
    int table_capacity;
    
    // TunerStudio specific fields
    char ini_version[16];
    char author[64];
    char description[256];
    char notes[512];
    char website[128];
    char email[64];
    
    // Communication settings
    char comm_port[32];
    int comm_baud;
    int comm_timeout;
    bool comm_auto_connect;
    
    // Advanced settings
    bool enable_advanced;
    bool enable_debug;
    bool enable_logging;
    char log_file[256];
    
    // Error tracking
    char last_error[256];
    bool has_errors;
} INIConfig;

// Protocol Detection Result
typedef struct {
    int protocol_type;          // ECUProtocol enum value
    float confidence;           // 0.0 to 1.0
    char detected_signature[256];
    char detected_version[64];
    INIConfig* ini_config;     // Loaded from INI file
} ProtocolDetectionResult;

// INI Parser Functions
INIConfig* ecu_load_ini_file(const char* file_path);
void ecu_free_ini_config(INIConfig* config);
bool ecu_validate_ini_config(const INIConfig* config);

// INI Section Parsing
bool ecu_parse_ini_section(const char* content, const char* section_name, INIConfig* config);
bool ecu_parse_tunerstudio_section(const char* content, INIConfig* config);
bool ecu_parse_constants_section(const char* content, INIConfig* config);
bool ecu_parse_megatune_section(const char* content, INIConfig* config);

// Table parsing functions
bool ecu_parse_table_dimensions(const char* content, INIConfig* config);
INITableInfo* ecu_find_table_by_name(INIConfig* config, const char* table_name);
bool ecu_parse_table_axis_info(const char* content, INITableInfo* table);
bool ecu_parse_table_array_definition(const char* line, INITableInfo* table);
bool ecu_parse_table_axis_definitions(const char* content, INIConfig* config);

// Value Extraction Functions
bool ecu_extract_string_value(const char* content, const char* key, char* value, size_t max_len);
bool ecu_extract_int_value(const char* content, const char* key, int* value);
bool ecu_extract_float_value(const char* content, const char* key, float* value);
bool ecu_extract_bool_value(const char* content, const char* key, bool* value);

// Protocol Detection
ProtocolDetectionResult ecu_detect_protocol_from_ini(const INIConfig* config);
int ecu_parse_protocol_name_from_ini(const char* protocol_name);
const char* ecu_get_protocol_name_from_ini(int protocol_type);

// INI File Validation
bool ecu_validate_ini_file(const char* file_path);
bool ecu_check_ini_file_exists(const char* file_path);
bool ecu_get_ini_file_info(const char* file_path, char* ecu_name, char* ecu_version);

// Utility Functions
char* ecu_read_file_content(const char* file_path);
bool ecu_write_ini_config_to_file(const INIConfig* config, const char* file_path);
void ecu_print_ini_config(const INIConfig* config);

// Protocol-Specific INI Parsers
bool ecu_parse_speeduino_ini(const char* content, INIConfig* config);
bool ecu_parse_rusefi_ini(const char* content, INIConfig* config);
bool ecu_parse_megasquirt_ini(const char* content, INIConfig* config);
bool ecu_parse_libreems_ini(const char* content, INIConfig* config);

// Field Parsing
bool ecu_parse_ini_field(const char* line, INIField* field);
bool ecu_add_ini_field(INIConfig* config, const INIField* field);
INIField* ecu_find_ini_field(const INIConfig* config, const char* field_name);

// Error Handling
void ecu_set_ini_error(const char* error);
const char* ecu_get_ini_error(void);
void ecu_clear_ini_error(void);

#endif // ECU_INI_PARSER_H 