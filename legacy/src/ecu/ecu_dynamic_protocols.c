/*
 * Dynamic ECU Protocol Support
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Allows users to add new ECU protocols by importing INI files.
 */

#include "../../include/ecu/ecu_dynamic_protocols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Initialize dynamic protocol manager
DynamicProtocolManager* ecu_dynamic_protocols_init(void) {
    DynamicProtocolManager* manager = malloc(sizeof(DynamicProtocolManager));
    if (!manager) return NULL;
    
    manager->count = 0;
    manager->capacity = 10;
    manager->protocols = malloc(sizeof(DynamicProtocol) * manager->capacity);
    if (!manager->protocols) {
        free(manager);
        return NULL;
    }
    
    strcpy(manager->protocols_file, "dynamic_protocols.json");
    
    // Load existing protocols
    ecu_dynamic_protocols_load(manager);
    
    return manager;
}

void ecu_dynamic_protocols_cleanup(DynamicProtocolManager* manager) {
    if (!manager) return;
    
    // Free INI configs
    for (int i = 0; i < manager->count; i++) {
        if (manager->protocols[i].ini_config) {
            ecu_free_ini_config(manager->protocols[i].ini_config);
        }
    }
    
    if (manager->protocols) {
        free(manager->protocols);
    }
    
    free(manager);
}

// Add a new protocol from INI file
bool ecu_dynamic_protocols_add_from_ini(DynamicProtocolManager* manager, const char* ini_file_path) {
    if (!manager || !ini_file_path) return false;
    
    // Load and validate INI file
    INIConfig* ini_config = ecu_load_ini_file(ini_file_path);
    if (!ini_config) {
        printf("[ERROR] Failed to load INI file: %s\n", ecu_get_ini_error());
        return false;
    }
    
    // Detect protocol
    ProtocolDetectionResult detection = ecu_detect_protocol_from_ini(ini_config);
    if (detection.confidence < 0.5f) {
        printf("[ERROR] Could not detect protocol from INI file (confidence: %.2f)\n", detection.confidence);
        ecu_free_ini_config(ini_config);
        return false;
    }
    
    // Check if protocol already exists
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->protocols[i].signature, detection.detected_signature) == 0) {
            printf("[WARNING] Protocol already exists: %s\n", manager->protocols[i].name);
            ecu_free_ini_config(ini_config);
            return false;
        }
    }
    
    // Expand array if needed
    if (manager->count >= manager->capacity) {
        int new_capacity = manager->capacity * 2;
        DynamicProtocol* new_protocols = realloc(manager->protocols, sizeof(DynamicProtocol) * new_capacity);
        if (!new_protocols) {
            ecu_free_ini_config(ini_config);
            return false;
        }
        manager->protocols = new_protocols;
        manager->capacity = new_capacity;
    }
    
    // Add new protocol
    DynamicProtocol* protocol = &manager->protocols[manager->count];
    memset(protocol, 0, sizeof(DynamicProtocol));
    
    // Generate protocol name from signature
    const char* signature = detection.detected_signature;
    if (strstr(signature, "rusEFI") != NULL) {
        strcpy(protocol->name, "EpicEFI (rusEFI)");
    } else if (strstr(signature, "speeduino") != NULL) {
        strcpy(protocol->name, "Speeduino");
    } else if (strstr(signature, "MegaSquirt") != NULL) {
        strcpy(protocol->name, "MegaSquirt");
    } else if (strstr(signature, "LibreEMS") != NULL) {
        strcpy(protocol->name, "LibreEMS");
    } else {
        // Generate generic name
        snprintf(protocol->name, sizeof(protocol->name), "Custom ECU (%s)", signature);
    }
    
    strncpy(protocol->signature, detection.detected_signature, sizeof(protocol->signature) - 1);
    strncpy(protocol->ini_file_path, ini_file_path, sizeof(protocol->ini_file_path) - 1);
    strncpy(protocol->ecu_name, ini_config->ecu_name, sizeof(protocol->ecu_name) - 1);
    strncpy(protocol->ecu_version, ini_config->ecu_version, sizeof(protocol->ecu_version) - 1);
    protocol->confidence = detection.confidence;
    protocol->enabled = true;
    protocol->ini_config = ini_config;
    
    manager->count++;
    
    printf("[INFO] Added dynamic protocol: %s (confidence: %.2f)\n", protocol->name, protocol->confidence);
    
    // Save to file
    ecu_dynamic_protocols_save(manager);
    
    return true;
}

// Remove a protocol
bool ecu_dynamic_protocols_remove(DynamicProtocolManager* manager, const char* protocol_name) {
    if (!manager || !protocol_name) return false;
    
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->protocols[i].name, protocol_name) == 0) {
            // Free INI config
            if (manager->protocols[i].ini_config) {
                ecu_free_ini_config(manager->protocols[i].ini_config);
            }
            
            // Remove from array
            for (int j = i; j < manager->count - 1; j++) {
                manager->protocols[j] = manager->protocols[j + 1];
            }
            manager->count--;
            
            printf("[INFO] Removed dynamic protocol: %s\n", protocol_name);
            ecu_dynamic_protocols_save(manager);
            return true;
        }
    }
    
    return false;
}

// Enable/disable a protocol
bool ecu_dynamic_protocols_enable(DynamicProtocolManager* manager, const char* protocol_name, bool enable) {
    if (!manager || !protocol_name) return false;
    
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->protocols[i].name, protocol_name) == 0) {
            manager->protocols[i].enabled = enable;
            printf("[INFO] %s dynamic protocol: %s\n", enable ? "Enabled" : "Disabled", protocol_name);
            ecu_dynamic_protocols_save(manager);
            return true;
        }
    }
    
    return false;
}

// Detect protocol from INI config
int ecu_dynamic_protocols_detect_from_ini(DynamicProtocolManager* manager, const INIConfig* ini_config) {
    if (!manager || !ini_config) return -1;
    
    const char* signature = ini_config->signature;
    
    // Check dynamic protocols first
    for (int i = 0; i < manager->count; i++) {
        if (manager->protocols[i].enabled && 
            strcmp(manager->protocols[i].signature, signature) == 0) {
            return i;
        }
    }
    
    // Check built-in protocols
    if (strstr(signature, "rusEFI") != NULL) {
        return ECU_PROTOCOL_EPICEFI;
    } else if (strstr(signature, "speeduino") != NULL) {
        return ECU_PROTOCOL_SPEEDUINO;
    } else if (strstr(signature, "MegaSquirt") != NULL) {
        return ECU_PROTOCOL_MEGASQUIRT;
    } else if (strstr(signature, "LibreEMS") != NULL) {
        return ECU_PROTOCOL_LIBREEMS;
    }
    
    return -1;
}

// Get protocol by name
const DynamicProtocol* ecu_dynamic_protocols_get_by_name(DynamicProtocolManager* manager, const char* name) {
    if (!manager || !name) return NULL;
    
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->protocols[i].name, name) == 0) {
            return &manager->protocols[i];
        }
    }
    
    return NULL;
}

// Get protocol by signature
const DynamicProtocol* ecu_dynamic_protocols_get_by_signature(DynamicProtocolManager* manager, const char* signature) {
    if (!manager || !signature) return NULL;
    
    for (int i = 0; i < manager->count; i++) {
        if (strcmp(manager->protocols[i].signature, signature) == 0) {
            return &manager->protocols[i];
        }
    }
    
    return NULL;
}

// Save protocols to file (simplified JSON format)
bool ecu_dynamic_protocols_save(DynamicProtocolManager* manager) {
    if (!manager) return false;
    
    FILE* file = fopen(manager->protocols_file, "w");
    if (!file) return false;
    
    fprintf(file, "{\n");
    fprintf(file, "  \"protocols\": [\n");
    
    for (int i = 0; i < manager->count; i++) {
        const DynamicProtocol* protocol = &manager->protocols[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", protocol->name);
        fprintf(file, "      \"signature\": \"%s\",\n", protocol->signature);
        fprintf(file, "      \"ini_file_path\": \"%s\",\n", protocol->ini_file_path);
        fprintf(file, "      \"ecu_name\": \"%s\",\n", protocol->ecu_name);
        fprintf(file, "      \"ecu_version\": \"%s\",\n", protocol->ecu_version);
        fprintf(file, "      \"confidence\": %.2f,\n", protocol->confidence);
        fprintf(file, "      \"enabled\": %s\n", protocol->enabled ? "true" : "false");
        fprintf(file, "    }%s\n", i < manager->count - 1 ? "," : "");
    }
    
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

// Load protocols from file
bool ecu_dynamic_protocols_load(DynamicProtocolManager* manager) {
    if (!manager) return false;
    
    FILE* file = fopen(manager->protocols_file, "r");
    if (!file) {
        printf("[INFO] No dynamic protocols file found, starting fresh\n");
        return true;
    }
    
    // Simple JSON parser (basic implementation)
    char line[1024];
    bool in_protocols = false;
    bool in_protocol = false;
    int protocol_index = -1;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') continue;
        
        // Parse JSON structure
        if (strstr(line, "\"protocols\"") != NULL) {
            in_protocols = true;
        } else if (strstr(line, "{") != NULL && in_protocols) {
            in_protocol = true;
            protocol_index++;
            
            // Expand array if needed
            if (protocol_index >= manager->capacity) {
                int new_capacity = manager->capacity * 2;
                DynamicProtocol* new_protocols = realloc(manager->protocols, sizeof(DynamicProtocol) * new_capacity);
                if (!new_protocols) {
                    fclose(file);
                    return false;
                }
                manager->protocols = new_protocols;
                manager->capacity = new_capacity;
            }
            
            memset(&manager->protocols[protocol_index], 0, sizeof(DynamicProtocol));
        } else if (strstr(line, "}") != NULL && in_protocol) {
            in_protocol = false;
        } else if (in_protocol) {
            // Parse protocol fields
            char* colon = strchr(line, ':');
            if (colon) {
                *colon = '\0';
                char* key = line;
                char* value = colon + 1;
                
                // Remove quotes and whitespace
                while (*key && isspace(*key)) key++;
                while (*value && isspace(*value)) value++;
                if (*value == '"') value++;
                char* end = value + strlen(value) - 1;
                while (end > value && (*end == '"' || *end == ',')) end--;
                *(end + 1) = '\0';
                
                DynamicProtocol* protocol = &manager->protocols[protocol_index];
                
                if (strstr(key, "name") != NULL) {
                    strncpy(protocol->name, value, sizeof(protocol->name) - 1);
                } else if (strstr(key, "signature") != NULL) {
                    strncpy(protocol->signature, value, sizeof(protocol->signature) - 1);
                } else if (strstr(key, "ini_file_path") != NULL) {
                    strncpy(protocol->ini_file_path, value, sizeof(protocol->ini_file_path) - 1);
                } else if (strstr(key, "ecu_name") != NULL) {
                    strncpy(protocol->ecu_name, value, sizeof(protocol->ecu_name) - 1);
                } else if (strstr(key, "ecu_version") != NULL) {
                    strncpy(protocol->ecu_version, value, sizeof(protocol->ecu_version) - 1);
                } else if (strstr(key, "confidence") != NULL) {
                    protocol->confidence = atof(value);
                } else if (strstr(key, "enabled") != NULL) {
                    protocol->enabled = (strcmp(value, "true") == 0);
                }
            }
        }
    }
    
    manager->count = protocol_index + 1;
    
    // Load INI configs for enabled protocols
    for (int i = 0; i < manager->count; i++) {
        if (manager->protocols[i].enabled && strlen(manager->protocols[i].ini_file_path) > 0) {
            manager->protocols[i].ini_config = ecu_load_ini_file(manager->protocols[i].ini_file_path);
        }
    }
    
    fclose(file);
    printf("[INFO] Loaded %d dynamic protocols\n", manager->count);
    return true;
}

// List all protocols
void ecu_dynamic_protocols_list(DynamicProtocolManager* manager) {
    if (!manager) return;
    
    printf("\n=== Dynamic ECU Protocols ===\n");
    if (manager->count == 0) {
        printf("No dynamic protocols loaded.\n");
        return;
    }
    
    for (int i = 0; i < manager->count; i++) {
        const DynamicProtocol* protocol = &manager->protocols[i];
        printf("%d. %s (%s)\n", i + 1, protocol->name, protocol->enabled ? "Enabled" : "Disabled");
        printf("   Signature: %s\n", protocol->signature);
        printf("   INI File: %s\n", protocol->ini_file_path);
        printf("   ECU: %s v%s\n", protocol->ecu_name, protocol->ecu_version);
        printf("   Confidence: %.2f\n", protocol->confidence);
        printf("\n");
    }
}

// Validate INI file for import
bool ecu_dynamic_protocols_validate_ini_for_import(const char* ini_file_path) {
    if (!ini_file_path) return false;
    
    // Check if file exists
    if (!ecu_check_ini_file_exists(ini_file_path)) {
        return false;
    }
    
    // Try to load and validate
    INIConfig* config = ecu_load_ini_file(ini_file_path);
    if (!config) {
        return false;
    }
    
    // Check if it's a valid ECU INI
    if (strlen(config->signature) == 0) {
        ecu_free_ini_config(config);
        return false;
    }
    
    ecu_free_ini_config(config);
    return true;
} 