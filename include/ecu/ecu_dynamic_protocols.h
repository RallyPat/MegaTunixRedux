/*
 * Dynamic ECU Protocol Support
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Allows users to add new ECU protocols by importing INI files.
 */

#ifndef ECU_DYNAMIC_PROTOCOLS_H
#define ECU_DYNAMIC_PROTOCOLS_H

#include "ecu_communication.h"
#include "ecu_ini_parser.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Dynamic protocol entry
typedef struct {
    char name[64];
    char signature[256];
    char ini_file_path[512];
    char ecu_name[64];
    char ecu_version[64];
    float confidence;
    bool enabled;
    INIConfig* ini_config;
} DynamicProtocol;

// Dynamic protocol manager
typedef struct {
    DynamicProtocol* protocols;
    int count;
    int capacity;
    char protocols_file[256];
} DynamicProtocolManager;

// Function declarations
DynamicProtocolManager* ecu_dynamic_protocols_init(void);
void ecu_dynamic_protocols_cleanup(DynamicProtocolManager* manager);

// Protocol management
bool ecu_dynamic_protocols_add_from_ini(DynamicProtocolManager* manager, const char* ini_file_path);
bool ecu_dynamic_protocols_remove(DynamicProtocolManager* manager, const char* protocol_name);
bool ecu_dynamic_protocols_enable(DynamicProtocolManager* manager, const char* protocol_name, bool enable);

// Protocol detection
int ecu_dynamic_protocols_detect_from_ini(DynamicProtocolManager* manager, const INIConfig* ini_config);
const DynamicProtocol* ecu_dynamic_protocols_get_by_name(DynamicProtocolManager* manager, const char* name);
const DynamicProtocol* ecu_dynamic_protocols_get_by_signature(DynamicProtocolManager* manager, const char* signature);

// Persistence
bool ecu_dynamic_protocols_save(DynamicProtocolManager* manager);
bool ecu_dynamic_protocols_load(DynamicProtocolManager* manager);

// Utility functions
void ecu_dynamic_protocols_list(DynamicProtocolManager* manager);
bool ecu_dynamic_protocols_validate_ini_for_import(const char* ini_file_path);

#ifdef __cplusplus
}
#endif

#endif // ECU_DYNAMIC_PROTOCOLS_H 