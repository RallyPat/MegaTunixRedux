/*
 * MegaTunix Redux - ECU Communication Integration Module Implementation
 * 
 * Copyright (C) 2025 Patrick Burke
 * Based on original MegaTunix by David J. Andruczyk
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../../include/ui/ecu_integration.h"
#include "../../include/ui/logging_system.h"
#include "../../include/ui/ui_theme_manager.h"
#include "../../include/ecu/ecu_communication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <math.h>

// Module state
static bool g_module_initialized = false;
static ECUIntegrationState g_ecu_integration = {0};

// Global state access for compatibility
ECUIntegrationState* g_ecu_integration_global = &g_ecu_integration;

// Module initialization and cleanup
bool init_ecu_integration(void) {
    if (g_module_initialized) {
        return true;
    }
    
    // Initialize ECU integration state
    memset(&g_ecu_integration, 0, sizeof(ECUIntegrationState));
    g_ecu_integration.ecu_context = NULL;
    g_ecu_integration.ecu_connected = false;
    strcpy(g_ecu_integration.ecu_status, "Disconnected");
    g_ecu_integration.demo_mode_enabled = false;
    g_ecu_integration.last_demo_update = 0;
    g_ecu_integration.last_realtime_update = 0;
    g_ecu_integration.realtime_update_interval = 100; // 100ms default
    
    // Initialize ECU integration state
    g_ecu_integration.ecu_context = NULL;
    g_ecu_integration.ecu_connected = false;
    strcpy(g_ecu_integration.ecu_status, "Disconnected");
    g_ecu_integration.demo_mode_enabled = false;
    g_ecu_integration.last_demo_update = 0;
    g_ecu_integration.last_realtime_update = 0;
    g_ecu_integration.realtime_update_interval = 100; // 100ms default
    
    g_module_initialized = true;
    add_log_entry(0, "ECU Integration module initialized successfully");
    return true;
}

void cleanup_ecu_integration(void) {
    if (!g_module_initialized) {
        return;
    }
    
    // Cleanup ECU communication
    cleanup_ecu_communication();
    
    g_module_initialized = false;
    add_log_entry(0, "ECU Integration module cleaned up");
}

// ECU communication management
bool init_ecu_communication(void) {
    g_ecu_integration.ecu_context = ecu_init();
    if (!g_ecu_integration.ecu_context) {
        add_log_entry(2, "Failed to initialize ECU context");
        return false;
    }
    
    add_log_entry(0, "ECU communication initialized successfully");
    return true;
}

void cleanup_ecu_communication(void) {
    if (g_ecu_integration.ecu_context) {
        ecu_cleanup(g_ecu_integration.ecu_context);
        g_ecu_integration.ecu_context = NULL;
    }
    
    add_log_entry(0, "ECU communication cleaned up");
}

// Speeduino protocol implementation - simplified for now
void speeduino_init(void) {
    add_log_entry(0, "Speeduino protocol initialization - simplified stub");
}

void speeduino_cleanup(void) {
    add_log_entry(0, "Speeduino protocol cleanup - simplified stub");
}

bool speeduino_connect(const char* port) {
    add_log_entry(0, "Speeduino connect - simplified stub for port %s", port);
    return false; // Return false for now since we're not fully integrated
}

void speeduino_disconnect(void) {
    add_log_entry(0, "Speeduino disconnect - simplified stub");
}

bool speeduino_is_connected(void) {
    add_log_entry(0, "Speeduino is_connected - simplified stub");
    return false; // Return false for now since we're not fully integrated
}

bool speeduino_send_packet(SpeeduinoPacket* packet) {
    add_log_entry(0, "Speeduino send_packet - simplified stub");
    return false; // Return false for now since we're not fully integrated
}

uint8_t speeduino_calculate_crc(uint8_t* data, int length) {
    add_log_entry(0, "Speeduino calculate_crc - simplified stub");
    return 0; // Return 0 for now since we're not fully integrated
}

bool speeduino_get_realtime_data(void) {
    add_log_entry(0, "Speeduino get_realtime_data - simplified stub");
    return false; // Return false for now since we're not fully integrated
}

bool speeduino_get_table_data(uint8_t table_id) {
    add_log_entry(0, "Speeduino get_table_data - simplified stub for table %d", table_id);
    return false; // Return false for now since we're not fully integrated
}

bool speeduino_set_table_data(uint8_t table_id, uint8_t* data, int length) {
    add_log_entry(0, "Speeduino set_table_data - simplified stub for table %d, length %d", table_id, length);
    return false; // Return false for now since we're not fully integrated
}

void speeduino_update_connection_status(void) {
    add_log_entry(0, "Speeduino update_connection_status - simplified stub");
}

// ECU data management
void update_ecu_data(void) {
    if (g_ecu_integration.ecu_context) {
        ecu_update(g_ecu_integration.ecu_context);
        
        bool was_connected = g_ecu_integration.ecu_connected;
        g_ecu_integration.ecu_connected = ecu_is_connected(g_ecu_integration.ecu_context);
        
        if (was_connected != g_ecu_integration.ecu_connected) {
            if (g_ecu_integration.ecu_connected) {
                add_log_entry(0, "ECU connection established");
            } else {
                add_log_entry(0, "ECU connection lost");
            }
        }
        
        // Update status
        ECUConnectionState state = ecu_get_state(g_ecu_integration.ecu_context);
        switch (state) {
            case ECU_STATE_DISCONNECTED:
                strcpy(g_ecu_integration.ecu_status, "Disconnected");
                break;
            case ECU_STATE_CONNECTING:
                strcpy(g_ecu_integration.ecu_status, "Connecting");
                break;
            case ECU_STATE_CONNECTED:
                strcpy(g_ecu_integration.ecu_status, "Connected");
                break;
            case ECU_STATE_ERROR:
                strcpy(g_ecu_integration.ecu_status, "Error");
                break;
            default:
                strcpy(g_ecu_integration.ecu_status, "Unknown");
                break;
        }
        
        // Get data
        const ECUData* data = ecu_get_data(g_ecu_integration.ecu_context);
        if (data) {
            g_ecu_integration.ecu_data = *data;
        }
    }
    
    // Update demo data if enabled
    if (g_ecu_integration.demo_mode_enabled) {
        update_ecu_demo_data();
    }
}

const ECUData* get_ecu_data(void) {
    return &g_ecu_integration.ecu_data;
}

bool is_ecu_connected(void) {
    return g_ecu_integration.ecu_connected;
}

const char* get_ecu_status(void) {
    return g_ecu_integration.ecu_status;
}

// Demo mode management
void enable_ecu_demo_mode(bool enabled) {
    g_ecu_integration.demo_mode_enabled = enabled;
    add_log_entry(0, "ECU demo mode %s", enabled ? "enabled" : "disabled");
}

void update_ecu_demo_data(void) {
    uint32_t current_time = SDL_GetTicks();
    if (current_time - g_ecu_integration.last_demo_update < 100) {
        return; // Update every 100ms
    }
    
    float demo_time = current_time / 1000.0f;
    
    // Generate realistic demo data with sine waves
    g_ecu_integration.ecu_data.rpm = fmax(800.0f, 800.0f + 7200.0f * sin(demo_time * 0.3f) + 1000.0f * sin(demo_time * 1.5f));
    
    // MAP with realistic variations
    float map_base = 100.0f;
    float map_random = 20.0f * sin(demo_time * 0.5f);
    float map_spike = 50.0f * sin(demo_time * 2.0f) * (sin(demo_time * 0.1f) > 0.8f ? 1.0f : 0.0f);
    g_ecu_integration.ecu_data.map = fmax(20.0f, fmin(240.0f, map_base + map_random + map_spike));
    
    g_ecu_integration.ecu_data.tps = fmax(0.0f, 10.0f + 30.0f * sin(demo_time * 0.7f));
    g_ecu_integration.ecu_data.afr = fmax(10.0f, fmin(20.0f, 14.7f + 2.0f * sin(demo_time * 0.4f)));
    g_ecu_integration.ecu_data.boost = fmax(-5.0f, 5.0f + 8.0f * sin(demo_time * 0.6f));
    g_ecu_integration.ecu_data.coolant_temp = fmax(60.0f, fmin(120.0f, 90.0f + 10.0f * sin(demo_time * 0.2f)));
    g_ecu_integration.ecu_data.intake_temp = fmax(60.0f, fmin(120.0f, 85.0f + 8.0f * sin(demo_time * 0.3f)));
    g_ecu_integration.ecu_data.oil_temp = fmax(60.0f, fmin(130.0f, 95.0f + 12.0f * sin(demo_time * 0.4f)));
    g_ecu_integration.ecu_data.battery_voltage = fmax(10.0f, fmin(16.0f, 13.5f + 0.5f * sin(demo_time * 0.8f)));
    g_ecu_integration.ecu_data.timing = fmax(-10.0f, fmin(40.0f, 15.0f + 10.0f * sin(demo_time * 0.9f)));
    g_ecu_integration.ecu_data.oil_pressure = fmax(50.0f, 300.0f + 100.0f * sin(demo_time * 0.5f));
    g_ecu_integration.ecu_data.fuel_pressure = fmax(100.0f, 250.0f + 50.0f * sin(demo_time * 0.6f));
    
    g_ecu_integration.last_demo_update = current_time;
}

// UI rendering
void render_ecu_connection_panel(void) {
    add_log_entry(0, "Render ECU connection panel - simplified stub");
}

void render_ecu_status_indicator(void) {
    add_log_entry(0, "Render ECU status indicator - simplified stub");
}

// Legacy function names for compatibility
void init_ecu_communication_legacy(void) {
    init_ecu_communication();
}

void cleanup_ecu_communication_legacy(void) {
    cleanup_ecu_communication();
}

void speeduino_init_legacy(void) {
    speeduino_init();
}

void speeduino_cleanup_legacy(void) {
    speeduino_cleanup();
}

bool speeduino_connect_legacy(const char* port) {
    return speeduino_connect(port);
}

void speeduino_disconnect_legacy(void) {
    speeduino_disconnect();
}

bool speeduino_is_connected_legacy(void) {
    return speeduino_is_connected();
}

bool speeduino_send_packet_legacy(SpeeduinoPacket* packet) {
    return speeduino_send_packet(packet);
}

uint8_t speeduino_calculate_crc_legacy(uint8_t* data, int length) {
    return speeduino_calculate_crc(data, length);
}

bool speeduino_get_realtime_data_legacy(void) {
    return speeduino_get_realtime_data();
}

bool speeduino_get_table_data_legacy(uint8_t table_id) {
    return speeduino_get_table_data(table_id);
}

bool speeduino_set_table_data_legacy(uint8_t table_id, uint8_t* data, int length) {
    return speeduino_set_table_data(table_id, data, length);
}

void speeduino_update_connection_status_legacy(void) {
    speeduino_update_connection_status();
}

void render_ecu_connection_panel_legacy(void) {
    render_ecu_connection_panel();
}
