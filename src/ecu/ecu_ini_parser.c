/*
 * ECU INI Parser - Universal ECU Configuration Support
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Supports TunerStudio INI file parsing for Speeduino, rusEFI, MegaSquirt, and LibreEMS.
 */

#include "../../include/ecu/ecu_ini_parser.h"
#include "../../include/ecu/ecu_communication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// Global error state
static char g_ini_error[256] = {0};

// Forward declarations
bool ecu_extract_string_value(const char* content, const char* key, char* value, size_t max_len);
bool ecu_extract_int_value(const char* content, const char* key, int* value);
bool ecu_extract_float_value(const char* content, const char* key, float* value);
bool ecu_extract_bool_value(const char* content, const char* key, bool* value);

// INI Parser Implementation
INIConfig* ecu_load_ini_file(const char* file_path) {
    if (!file_path) return NULL;
    
    // Create new config structure
    INIConfig* config = (INIConfig*)malloc(sizeof(INIConfig));
    if (!config) return NULL;
    
    // Initialize config structure
    memset(config, 0, sizeof(INIConfig));
    
    // Set default values
    config->baud_rate = 115200;
    config->timeout_ms = 1000;
    config->auto_connect = false;
    config->auto_reconnect = false;
    config->reconnect_interval = 5000;
    config->enable_advanced = false;
    config->enable_debug = false;
    config->enable_logging = false;
    config->has_errors = false;
    
    // Initialize table arrays
    config->table_capacity = 20;
    config->tables = (INITableInfo*)malloc(config->table_capacity * sizeof(INITableInfo));
    config->table_count = 0;
    
    if (!config->tables) {
        free(config);
        return NULL;
    }
    
    // Initialize all table structures to prevent undefined behavior
    for (int i = 0; i < config->table_capacity; i++) {
        memset(&config->tables[i], 0, sizeof(INITableInfo));
    }
    
    // Read file content
    char* content = ecu_read_file_content(file_path);
    if (!content) {
        ecu_set_ini_error("Failed to read INI file");
        ecu_free_ini_config(config);
        return NULL;
    }
    
    // Parse INI sections
    ecu_parse_tunerstudio_section(content, config);
    ecu_parse_constants_section(content, config);
    ecu_parse_megatune_section(content, config);
    
    // Parse table dimensions
    ecu_parse_table_dimensions(content, config);
    
    // Detect protocol from INI content
    ProtocolDetectionResult detection = ecu_detect_protocol_from_ini(config);
    if (detection.protocol_type != ECU_PROTOCOL_NONE) {
        strncpy(config->signature, detection.detected_signature, sizeof(config->signature) - 1);
        strncpy(config->ecu_version, detection.detected_version, sizeof(config->ecu_version) - 1);
        config->protocol_type = detection.protocol_type;
    }
    
    free(content);
    return config;
}

void ecu_free_ini_config(INIConfig* config) {
    if (!config) return;
    
    // Free table arrays
    if (config->tables) {
        free(config->tables);
        config->tables = NULL;
        config->table_count = 0;
        config->table_capacity = 0;
    }
    
    // Free the config structure itself
    free(config);
}

bool ecu_validate_ini_config(const INIConfig* config) {
    if (!config) return false;
    
    // Basic validation
    if (config->table_count < 0) return false;
    if (config->table_count > config->table_capacity) return false;
    
    // Check if we have at least one table
    if (config->table_count == 0) {
        ecu_set_ini_error("No tables found in INI file");
        return false;
    }
    
    return true;
}

// INI Section Parsing
bool ecu_parse_tunerstudio_section(const char* content, INIConfig* config) {
    if (!content || !config) return false;
    
    // Parse TunerStudio section
    ecu_parse_ini_section(content, "TunerStudio", config);
    
    // Extract basic communication settings
    ecu_extract_string_value(content, "queryCommand", config->signature, sizeof(config->signature));
    ecu_extract_string_value(content, "versionInfo", config->ecu_version, sizeof(config->ecu_version));
    ecu_extract_string_value(content, "burnCommand", config->protocol_name, sizeof(config->protocol_name));
    
    // Extract communication settings
    ecu_extract_bool_value(content, "noCommReadDelay", &config->enable_debug);
    ecu_extract_bool_value(content, "writeBlocks", &config->enable_advanced);
    ecu_extract_bool_value(content, "tsWriteBlocks", &config->enable_logging);
    ecu_extract_int_value(content, "interWriteDelay", &config->timeout_ms);
    ecu_extract_int_value(content, "pageActivationDelay", &config->reconnect_interval);
    
    return true;
}

bool ecu_parse_constants_section(const char* content, INIConfig* config) {
    if (!content || !config) return false;
    
    // Parse Constants section
    ecu_parse_ini_section(content, "Constants", config);
    
    // Extract communication format settings
    ecu_extract_string_value(content, "messageEnvelopeFormat", config->signature, sizeof(config->signature));
    ecu_extract_string_value(content, "endianness", config->ecu_version, sizeof(config->ecu_version));
    ecu_extract_bool_value(content, "enable2ndByteCanID", &config->enable_advanced);
    
    // Extract page settings
    ecu_extract_int_value(content, "nPages", &config->table_count);
    
    // Parse page definitions
    char page_section[64];
    for (int page = 0; page < 4; page++) {
        snprintf(page_section, sizeof(page_section), "Page %d", page);
        ecu_parse_ini_section(content, page_section, config);
        
        // Extract page identifier
        char identifier_key[32];
        snprintf(identifier_key, sizeof(identifier_key), "pageIdentifier%d", page);
        ecu_extract_string_value(content, identifier_key, config->signature, sizeof(config->signature));
        
        // Extract page size
        char size_key[32];
        snprintf(size_key, sizeof(size_key), "pageSize%d", page);
        int page_size = 0;
        ecu_extract_int_value(content, size_key, &page_size);
        
        // Extract page read command
        char read_key[32];
        snprintf(read_key, sizeof(read_key), "pageReadCommand%d", page);
        ecu_extract_string_value(content, read_key, config->protocol_name, sizeof(config->protocol_name));
    }
    
    return true;
}

bool ecu_parse_megatune_section(const char* content, INIConfig* config) {
    if (!content || !config) return false;
    
    // Parse MegaTune section
    ecu_parse_ini_section(content, "MegaTune", config);
    
    // Extract MegaTune specific settings
    ecu_extract_string_value(content, "version", config->ecu_version, sizeof(config->ecu_version));
    ecu_extract_string_value(content, "author", config->signature, sizeof(config->signature));
    ecu_extract_string_value(content, "description", config->protocol_name, sizeof(config->protocol_name));
    
    return true;
}

bool ecu_parse_ini_section(const char* content, const char* section_name, INIConfig* config) {
    if (!content || !section_name || !config) {
        return false;
    }
    
    char section_header[64];
    snprintf(section_header, sizeof(section_header), "[%s]", section_name);
    
    // Find section
    char* section_start = strstr(content, section_header);
    if (!section_start) {
        return false;
    }
    
    // Find end of section (next [ or end of file)
    char* section_end = strchr(section_start + strlen(section_header), '[');
    if (!section_end) {
        section_end = content + strlen(content);
    }
    
    // Create temporary section content
    size_t section_len = section_end - section_start;
    char* section_content = malloc(section_len + 1);
    if (!section_content) {
        return false;
    }
    
    strncpy(section_content, section_start, section_len);
    section_content[section_len] = '\0';
    
    // Parse section content (this will be handled by specific section parsers)
    free(section_content);
    
    return true;
}

// Value Extraction Functions
bool ecu_extract_string_value(const char* content, const char* key, char* value, size_t max_len) {
    if (!content || !key || !value) {
        return false;
    }
    
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "%s", key);
    
    char* line_start = strstr(content, search_key);
    if (!line_start) {
        return false;
    }
    
    // Find the '=' sign
    char* equals = strchr(line_start, '=');
    if (!equals) {
        return false;
    }
    
    // Skip whitespace after '='
    equals++;
    while (*equals && isspace(*equals)) {
        equals++;
    }
    
    // Find end of value (semicolon or newline)
    char* value_end = equals;
    while (*value_end && *value_end != ';' && *value_end != '\n' && *value_end != '\r') {
        value_end++;
    }
    
    // Copy value, removing quotes if present
    size_t value_len = value_end - equals;
    if (value_len > 0) {
        const char* start = equals;
        const char* end = value_end;
        
        // Remove leading/trailing quotes
        if (*start == '"') start++;
        if (end > start && *(end-1) == '"') end--;
        
        // Remove leading/trailing whitespace
        while (start < end && isspace(*start)) start++;
        while (end > start && isspace(*(end-1))) end--;
        
        size_t copy_len = end - start;
        if (copy_len > 0 && copy_len < max_len) {
            strncpy(value, start, copy_len);
            value[copy_len] = '\0';
            return true;
        }
    }
    
    return false;
}

bool ecu_extract_int_value(const char* content, const char* key, int* value) {
    char str_value[64];
    if (ecu_extract_string_value(content, key, str_value, sizeof(str_value))) {
        *value = atoi(str_value);
        return true;
    }
    return false;
}

bool ecu_extract_float_value(const char* content, const char* key, float* value) {
    char str_value[64];
    if (ecu_extract_string_value(content, key, str_value, sizeof(str_value))) {
        *value = atof(str_value);
        return true;
    }
    return false;
}

bool ecu_extract_bool_value(const char* content, const char* key, bool* value) {
    char str_value[16];
    if (ecu_extract_string_value(content, key, str_value, sizeof(str_value))) {
        if (strcasecmp(str_value, "true") == 0 || strcasecmp(str_value, "on") == 0 || strcmp(str_value, "1") == 0) {
            *value = true;
            return true;
        } else if (strcasecmp(str_value, "false") == 0 || strcasecmp(str_value, "off") == 0 || strcmp(str_value, "0") == 0) {
            *value = false;
            return true;
        }
    }
    return false;
}

// Protocol Detection
ProtocolDetectionResult ecu_detect_protocol_from_ini(const INIConfig* config) {
    ProtocolDetectionResult result = {0};
    result.protocol_type = ECU_PROTOCOL_NONE;
    result.confidence = 0.0f;
    
    if (!config) return result;
    
    // Check for Speeduino signature
    if (strstr(config->signature, "Speeduino") || strstr(config->ecu_name, "Speeduino")) {
        result.protocol_type = ECU_PROTOCOL_SPEEDUINO;
        result.confidence = 0.9f;
        strncpy(result.detected_signature, "Speeduino", sizeof(result.detected_signature) - 1);
        strncpy(result.detected_version, config->ecu_version, sizeof(result.detected_version) - 1);
        return result;
    }
    
    // Check for EpicEFI/rusEFI signature
    if (strstr(config->signature, "EpicEFI") || strstr(config->ecu_name, "EpicEFI") ||
        strstr(config->signature, "rusEFI") || strstr(config->ecu_name, "rusEFI")) {
        result.protocol_type = ECU_PROTOCOL_EPICEFI;
        result.confidence = 0.9f;
        strncpy(result.detected_signature, "EpicEFI", sizeof(result.detected_signature) - 1);
        strncpy(result.detected_version, config->ecu_version, sizeof(result.detected_version) - 1);
        return result;
    }
    
    // Check for MegaSquirt signature
    if (strstr(config->signature, "MegaSquirt") || strstr(config->ecu_name, "MegaSquirt") ||
        strstr(config->signature, "MS") || strstr(config->ecu_name, "MS")) {
        result.protocol_type = ECU_PROTOCOL_MEGASQUIRT;
        result.confidence = 0.8f;
        strncpy(result.detected_signature, "MegaSquirt", sizeof(result.detected_signature) - 1);
        strncpy(result.detected_version, config->ecu_version, sizeof(result.detected_version) - 1);
        return result;
    }
    
    // Check for LibreEMS signature
    if (strstr(config->signature, "LibreEMS") || strstr(config->ecu_name, "LibreEMS")) {
        result.protocol_type = ECU_PROTOCOL_LIBREEMS;
        result.confidence = 0.8f;
        strncpy(result.detected_signature, "LibreEMS", sizeof(result.detected_signature) - 1);
        strncpy(result.detected_version, config->ecu_version, sizeof(result.detected_version) - 1);
        return result;
    }
    
    // Generic detection based on table patterns
    if (config->table_count > 0) {
        // Look for VE table to determine protocol
        for (int i = 0; i < config->table_count; i++) {
            if (strstr(config->tables[i].name, "veTable")) {
                // Check table dimensions for clues
                if (config->tables[i].width == 32 && config->tables[i].height == 32) {
                    result.protocol_type = ECU_PROTOCOL_EPICEFI;
                    result.confidence = 0.7f;
                    strncpy(result.detected_signature, "EpicEFI (detected by table size)", sizeof(result.detected_signature) - 1);
                } else if (config->tables[i].width == 16 && config->tables[i].height == 16) {
                    result.protocol_type = ECU_PROTOCOL_SPEEDUINO;
                    result.confidence = 0.6f;
                    strncpy(result.detected_signature, "Speeduino (detected by table size)", sizeof(result.detected_signature) - 1);
                }
                strncpy(result.detected_version, config->ecu_version, sizeof(result.detected_version) - 1);
                break;
            }
        }
    }
    
    return result;
}

// INI File Validation
bool ecu_validate_ini_file(const char* file_path) {
    if (!ecu_check_ini_file_exists(file_path)) {
        return false;
    }
    
    INIConfig* config = ecu_load_ini_file(file_path);
    if (!config) {
        return false;
    }
    
    bool valid = ecu_validate_ini_config(config);
    ecu_free_ini_config(config);
    
    return valid;
}

bool ecu_check_ini_file_exists(const char* file_path) {
    if (!file_path) {
        return false;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        ecu_set_ini_error("INI file not found or not readable");
        return false;
    }
    
    fclose(file);
    return true;
}

bool ecu_get_ini_file_info(const char* file_path, char* ecu_name, char* ecu_version) {
    INIConfig* config = ecu_load_ini_file(file_path);
    if (!config) {
        return false;
    }
    
    if (ecu_name) {
        strncpy(ecu_name, config->ecu_name, 64);
    }
    
    if (ecu_version) {
        strncpy(ecu_version, config->ecu_version, 64);
    }
    
    ecu_free_ini_config(config);
    return true;
}

// Utility Functions
char* ecu_read_file_content(const char* file_path) {
    if (!file_path) {
        return NULL;
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) {
        ecu_set_ini_error("Failed to open INI file");
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        ecu_set_ini_error("Empty or invalid INI file");
        fclose(file);
        return NULL;
    }
    
    // Allocate buffer
    char* content = malloc(file_size + 1);
    if (!content) {
        ecu_set_ini_error("Failed to allocate memory for INI file");
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t bytes_read = fread(content, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        ecu_set_ini_error("Failed to read INI file completely");
        free(content);
        return NULL;
    }
    
    content[file_size] = '\0';
    return content;
}

// Remove old field-related functions - they're no longer needed
// The new structure focuses on table parsing instead of individual fields

void ecu_print_ini_config(const INIConfig* config) {
    if (!config) {
        printf("INI Config: NULL\n");
        return;
    }
    
    printf("INI Configuration:\n");
    printf("  ECU Name: %s\n", config->ecu_name);
    printf("  ECU Version: %s\n", config->ecu_version);
    printf("  Protocol: %s\n", config->protocol_name);
    printf("  Baud Rate: %d\n", config->baud_rate);
    printf("  Timeout: %d ms\n", config->timeout_ms);
    printf("  Tables: %d\n", config->table_count);
    
    for (int i = 0; i < config->table_count; i++) {
        const INITableInfo* table = &config->tables[i];
        printf("  Table %d: %s (%dx%d)\n", i, table->name, table->width, table->height);
        printf("    X: %.0f-%.0f %s\n", table->x_min, table->x_max, table->x_units);
        printf("    Y: %.0f-%.0f %s\n", table->y_min, table->y_max, table->y_units);
        printf("    Scale: %.3f, Range: %.0f-%.0f\n", table->scale, table->min_value, table->max_value);
    }
}

// Protocol-Specific INI Parsers
bool ecu_parse_speeduino_ini(const char* content, INIConfig* config) {
    // Speeduino-specific parsing
    return ecu_parse_tunerstudio_section(content, config);
}

bool ecu_parse_rusefi_ini(const char* content, INIConfig* config) {
    // rusEFI-specific parsing
    bool success = true;
    success &= ecu_parse_tunerstudio_section(content, config);
    success &= ecu_parse_constants_section(content, config);
    return success;
}

bool ecu_parse_megasquirt_ini(const char* content, INIConfig* config) {
    // MegaSquirt-specific parsing
    return ecu_parse_megatune_section(content, config);
}

bool ecu_parse_libreems_ini(const char* content, INIConfig* config) {
    // LibreEMS-specific parsing
    return ecu_parse_tunerstudio_section(content, config);
}

// Field Parsing
// bool ecu_parse_ini_field(const char* line, INIField* field) {
//     if (!line || !field) {
//         return false;
//     }
    
//     // Parse field definition line
//     // Format: name = type, offset, units, scale, translate, min, max, digits
//     char* token = strtok((char*)line, "=");
//     if (!token) {
//         return false;
//     }
    
//     // Extract field name
//     while (*token && isspace(*token)) token++;
//     char* name_end = token + strlen(token) - 1;
//     while (name_end > token && isspace(*name_end)) name_end--;
//     *(name_end + 1) = '\0';
    
//     strncpy(field->name, token, sizeof(field->name) - 1);
    
//     // Parse field parameters
//     token = strtok(NULL, ",");
//     if (token) {
//         while (*token && isspace(*token)) token++;
//         strncpy(field->type, token, sizeof(field->type) - 1);
//     }
    
//     // Parse offset
//     token = strtok(NULL, ",");
//     if (token) {
//         field->offset = atoi(token);
//     }
    
//     // Parse units
//     token = strtok(NULL, ",");
//     if (token) {
//         while (*token && isspace(*token)) token++;
//         strncpy(field->units, token, sizeof(field->units) - 1);
//     }
    
//     // Parse scale
//     token = strtok(NULL, ",");
//     if (token) {
//         field->scale = atof(token);
//     }
    
//     // Parse translate
//     token = strtok(NULL, ",");
//     if (token) {
//         field->translate = atof(token);
//     }
    
//     // Parse min value
//     token = strtok(NULL, ",");
//     if (token) {
//         field->min_value = atof(token);
//     }
    
//     // Parse max value
//     token = strtok(NULL, ",");
//     if (token) {
//         field->max_value = atof(token);
//     }
    
//     // Parse digits
//     token = strtok(NULL, ",");
//     if (token) {
//         field->digits = atoi(token);
//     }
    
//     return true;
// }

// bool ecu_add_ini_field(INIConfig* config, const INIField* field) {
//     if (!config || !field) {
//         return false;
//     }
    
//     // Expand field array if needed
//     if (config->field_count >= config->field_capacity) {
//         int new_capacity = config->field_capacity * 2;
//         INIField* new_fields = realloc(config->fields, sizeof(INIField) * new_capacity);
//         if (!new_fields) {
//             return false;
//         }
//         config->fields = new_fields;
//         config->field_capacity = new_capacity;
//     }
    
//     // Add field
//     config->fields[config->field_count] = *field;
//     config->field_count++;
    
//     return true;
// }

// INIField* ecu_find_ini_field(const INIConfig* config, const char* field_name) {
//     if (!config || !field_name) {
//         return NULL;
//     }
    
//     for (int i = 0; i < config->field_count; i++) {
//         if (strcmp(config->fields[i].name, field_name) == 0) {
//             return &config->fields[i];
//         }
//     }
    
//     return NULL;
// }

// Table parsing functions
bool ecu_parse_table_dimensions(const char* content, INIConfig* config) {
    if (!content || !config) return false;
    
    // Initialize table arrays if not already done
    if (!config->tables) {
        config->table_capacity = 20;
        config->tables = (INITableInfo*)malloc(config->table_capacity * sizeof(INITableInfo));
        config->table_count = 0;
        
        if (!config->tables) return false;
        
        // Initialize all table structures
        for (int i = 0; i < config->table_capacity; i++) {
            memset(&config->tables[i], 0, sizeof(INITableInfo));
        }
    }
    
    // Create a copy of the content to avoid modifying the original
    char* content_copy = strdup(content);
    if (!content_copy) return false;
    
    // Parse array definitions that look like tables
    char* line_start = content_copy;
    char* line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';
        
        // Look for array definitions with dimensions like [NxN]
        if (strstr(line_start, "array") && strstr(line_start, "[") && strstr(line_start, "x")) {
            // Check if we need to expand the table array
            if (config->table_count >= config->table_capacity) {
                config->table_capacity *= 2;
                INITableInfo* new_tables = (INITableInfo*)realloc(config->tables, 
                                                                 config->table_capacity * sizeof(INITableInfo));
                if (!new_tables) {
                    free(content_copy);
                    return false;
                }
                config->tables = new_tables;
                
                // Initialize new table structures
                for (int i = config->table_count; i < config->table_capacity; i++) {
                    memset(&config->tables[i], 0, sizeof(INITableInfo));
                }
            }
            
            INITableInfo* table = &config->tables[config->table_count];
            memset(table, 0, sizeof(INITableInfo));
            
            // Parse table name and dimensions
            if (ecu_parse_table_array_definition(line_start, table)) {
                config->table_count++;
                printf("[DEBUG] Found table: %s (%dx%d)\n", table->name, table->width, table->height);
            }
        }
        
        line_start = line_end + 1;
    }
    
    // Parse axis definitions for tables
    ecu_parse_table_axis_definitions(content_copy, config);
    
    free(content_copy);
    return true;
}

bool ecu_parse_table_axis_definitions(const char* content, INIConfig* config) {
    if (!content || !config) return false;
    
    char* content_copy = strdup(content);
    if (!content_copy) return false;
    
    char* line_start = content_copy;
    char* line_end;
    
    while ((line_end = strchr(line_start, '\n')) != NULL) {
        *line_end = '\0';
        
        // Look for axis definitions like:
        // veTable_xBins = array, U16, 14852, [32], "RPM", 0, 0, 8000, 1
        // veTable_yBins = array, U16, 14884, [32], "kPa", 0, 0, 400, 1
        
        if (strstr(line_start, "_xBins") || strstr(line_start, "_yBins")) {
            char table_name[64];
            char axis_type[8];
            
            // Extract table name and axis type
            char* underscore = strrchr(line_start, '_');
            if (underscore) {
                int name_len = underscore - line_start;
                if (name_len < sizeof(table_name)) {
                    strncpy(table_name, line_start, name_len);
                    table_name[name_len] = '\0';
                    strcpy(axis_type, underscore + 1);
                    
                    // Find the corresponding table
                    INITableInfo* table = ecu_find_table_by_name(config, table_name);
                    if (table) {
                        // Parse axis values
                        char* dim_start = strchr(line_start, '[');
                        if (dim_start) {
                            char* dim_end = strchr(dim_start, ']');
                            if (dim_end) {
                                char dim_str[32];
                                int dim_len = dim_end - dim_start - 1;
                                if (dim_len < sizeof(dim_str)) {
                                    strncpy(dim_str, dim_start + 1, dim_len);
                                    dim_str[dim_len] = '\0';
                                    int axis_size = atoi(dim_str);
                                    
                                    // Parse axis parameters
                                    char* params = strchr(dim_end + 1, ',');
                                    if (params) {
                                        char* params_copy = strdup(params);
                                        if (params_copy) {
                                            char* token = strtok(params_copy, ",");
                                            int param_count = 0;
                                            
                                            while (token && param_count < 10) {
                                                while (*token == ' ') token++;
                                                
                                                switch (param_count) {
                                                    case 0: // units
                                                        if (strstr(axis_type, "xBins")) {
                                                            strncpy(table->x_units, token, sizeof(table->x_units) - 1);
                                                        } else if (strstr(axis_type, "yBins")) {
                                                            strncpy(table->y_units, token, sizeof(table->y_units) - 1);
                                                        }
                                                        break;
                                                    case 1: // scale
                                                        if (strstr(axis_type, "xBins")) {
                                                            table->x_axis_scale = atof(token);
                                                        } else if (strstr(axis_type, "yBins")) {
                                                            table->y_axis_scale = atof(token);
                                                        }
                                                        break;
                                                    case 2: // min value
                                                        if (strstr(axis_type, "xBins")) {
                                                            table->x_min = atof(token);
                                                        } else if (strstr(axis_type, "yBins")) {
                                                            table->y_min = atof(token);
                                                        }
                                                        break;
                                                    case 3: // max value
                                                        if (strstr(axis_type, "xBins")) {
                                                            table->x_max = atof(token);
                                                        } else if (strstr(axis_type, "yBins")) {
                                                            table->y_max = atof(token);
                                                        }
                                                        break;
                                                }
                                                
                                                token = strtok(NULL, ",");
                                                param_count++;
                                            }
                                            
                                            free(params_copy);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        line_start = line_end + 1;
    }
    
    free(content_copy);
    return true;
}

INITableInfo* ecu_find_table_by_name(INIConfig* config, const char* table_name) {
    if (!config || !table_name || !config->tables) return NULL;
    
    for (int i = 0; i < config->table_count; i++) {
        if (strcmp(config->tables[i].name, table_name) == 0) {
            return &config->tables[i];
        }
    }
    
    return NULL;
}

bool ecu_parse_table_axis_info(const char* content, INITableInfo* table) {
    if (!content || !table) return false;
    
    // This would parse axis information from the INI file
    // For now, we'll use default values
    table->x_min = 500.0f;
    table->x_max = 8000.0f;
    table->y_min = 20.0f;
    table->y_max = 400.0f;
    
    strcpy(table->x_axis_name, "RPM");
    strcpy(table->y_axis_name, "MAP");
    strcpy(table->x_units, "RPM");
    strcpy(table->y_units, "kPa");
    
    return true;
}

bool ecu_parse_table_array_definition(const char* line, INITableInfo* table) {
    if (!line || !table) return false;
    
    // Initialize table structure
    memset(table, 0, sizeof(INITableInfo));
    
    // Parse TunerStudio table definition format
    // Examples:
    // veTable = array, U16, 14852, [32x32], "%", 0.01, 0, 0, 650, 2
    // ignitionTable = array, U16, 14916, [32x32], "deg", 0.1, -10, 0, 50, 1
    // boostTable = array, U16, 14980, [8x8], "kPa", 0.1, 100, 0, 300, 1
    
    char* name_start = (char*)line;
    char* name_end = strchr(name_start, '=');
    if (!name_end) return false;
    
    // Extract table name
    int name_len = name_end - name_start;
    if (name_len >= sizeof(table->name)) name_len = sizeof(table->name) - 1;
    strncpy(table->name, name_start, name_len);
    table->name[name_len] = '\0';
    
    // Trim trailing whitespace from table name
    char* end = table->name + strlen(table->name) - 1;
    while (end > table->name && isspace(*end)) {
        *end = '\0';
        end--;
    }
    
    // Set display name (same as name for now)
    strcpy(table->display_name, table->name);
    
    // Find dimensions in [NxN] format
    char* dim_start = strchr(line, '[');
    if (!dim_start) return false;
    
    char* dim_end = strchr(dim_start, ']');
    if (!dim_end) return false;
    
    // Extract dimensions
    char dim_str[32];
    int dim_len = dim_end - dim_start - 1;
    if (dim_len >= sizeof(dim_str)) dim_len = sizeof(dim_str) - 1;
    strncpy(dim_str, dim_start + 1, dim_len);
    dim_str[dim_len] = '\0';
    
    // Parse width x height
    char* x_end = strchr(dim_str, 'x');
    if (!x_end) return false;
    
    *x_end = '\0';
    table->width = atoi(dim_str);
    table->height = atoi(x_end + 1);
    
    // Set default values
    table->enabled = true;
    table->editable = true;
    table->x_min = 0.0f;
    table->x_max = (float)table->width;
    table->y_min = 0.0f;
    table->y_max = (float)table->height;
    table->scale = 1.0f;
    table->min_value = 0.0f;
    table->max_value = 100.0f;
    table->digits = 1;
    
    // Parse parameters after dimensions
    char* params = strchr(dim_end + 1, ',');
    if (params) {
        // Create a copy of the parameters string to avoid modifying the original
        char* params_copy = strdup(params);
        if (params_copy) {
            char* token = strtok(params_copy, ",");
            int param_count = 0;
            
            while (token && param_count < 15) {
                // Skip whitespace
                while (*token == ' ') token++;
                
                switch (param_count) {
                    case 0: // units
                        strncpy(table->z_units, token, sizeof(table->z_units) - 1);
                        table->z_units[sizeof(table->z_units) - 1] = '\0';
                        break;
                    case 1: // scale
                        table->scale = atof(token);
                        break;
                    case 2: // min value
                        table->min_value = atof(token);
                        break;
                    case 3: // max value
                        table->max_value = atof(token);
                        break;
                    case 4: // digits
                        table->digits = atoi(token);
                        break;
                }
                
                token = strtok(NULL, ",");
                param_count++;
            }
            
            free(params_copy);
        }
    }
    
    // Set axis names and units based on table type
    if (strstr(table->name, "veTable")) {
        strcpy(table->x_axis_name, "RPM");
        strcpy(table->y_axis_name, "MAP");
        strcpy(table->x_units, "RPM");
        strcpy(table->y_units, "kPa");
        strcpy(table->x_axis_units, "RPM");
        strcpy(table->y_axis_units, "kPa");
        table->x_min = 500.0f;
        table->x_max = 8000.0f;
        table->y_min = 20.0f;
        table->y_max = 400.0f;
        table->x_axis_scale = 1.0f;
        table->y_axis_scale = 1.0f;
    } else if (strstr(table->name, "ignition")) {
        strcpy(table->x_axis_name, "RPM");
        strcpy(table->y_axis_name, "MAP");
        strcpy(table->x_units, "RPM");
        strcpy(table->y_units, "kPa");
        strcpy(table->x_axis_units, "RPM");
        strcpy(table->y_axis_units, "kPa");
        table->x_min = 500.0f;
        table->x_max = 8000.0f;
        table->y_min = 20.0f;
        table->y_max = 400.0f;
        table->x_axis_scale = 1.0f;
        table->y_axis_scale = 1.0f;
    } else if (strstr(table->name, "boost")) {
        strcpy(table->x_axis_name, "RPM");
        strcpy(table->y_axis_name, "MAP");
        strcpy(table->x_units, "RPM");
        strcpy(table->y_units, "kPa");
        strcpy(table->x_axis_units, "RPM");
        strcpy(table->y_axis_units, "kPa");
        table->x_min = 500.0f;
        table->x_max = 8000.0f;
        table->y_min = 20.0f;
        table->y_max = 400.0f;
        table->x_axis_scale = 1.0f;
        table->y_axis_scale = 1.0f;
    } else {
        strcpy(table->x_axis_name, "X");
        strcpy(table->y_axis_name, "Y");
        strcpy(table->x_units, "");
        strcpy(table->y_units, "");
        strcpy(table->x_axis_units, "");
        strcpy(table->y_axis_units, "");
    }
    
    return (table->width > 0 && table->height > 0);
}

// Error Handling
void ecu_set_ini_error(const char* error) {
    if (error) {
        strncpy(g_ini_error, error, sizeof(g_ini_error) - 1);
        g_ini_error[sizeof(g_ini_error) - 1] = '\0';
    }
}

const char* ecu_get_ini_error(void) {
    return g_ini_error;
}

void ecu_clear_ini_error(void) {
    memset(g_ini_error, 0, sizeof(g_ini_error));
}

// Protocol name conversion
int ecu_parse_protocol_name_from_ini(const char* protocol_name) {
    if (!protocol_name) {
        return ECU_PROTOCOL_NONE;
    }
    
    if (strcasecmp(protocol_name, "speeduino") == 0) {
        return ECU_PROTOCOL_SPEEDUINO;
    } else if (strcasecmp(protocol_name, "epicefi") == 0 || strcasecmp(protocol_name, "rusefi") == 0) {
        return ECU_PROTOCOL_EPICEFI;
    } else if (strcasecmp(protocol_name, "megasquirt") == 0 || strcasecmp(protocol_name, "ms") == 0) {
        return ECU_PROTOCOL_MEGASQUIRT;
    } else if (strcasecmp(protocol_name, "libreems") == 0) {
        return ECU_PROTOCOL_LIBREEMS;
    }
    
    return ECU_PROTOCOL_NONE;
}

const char* ecu_get_protocol_name_from_ini(int protocol_type) {
    switch (protocol_type) {
        case ECU_PROTOCOL_SPEEDUINO: return "Speeduino";
        case ECU_PROTOCOL_EPICEFI: return "EpicEFI (rusEFI)";
        case ECU_PROTOCOL_MEGASQUIRT: return "MegaSquirt";
        case ECU_PROTOCOL_LIBREEMS: return "LibreEMS";
        default: return "Unknown";
    }
} 