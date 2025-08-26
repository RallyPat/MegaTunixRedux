/*
 * MegaTunix Redux - ECU Communication Integration Module
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

#ifndef ECU_INTEGRATION_H
#define ECU_INTEGRATION_H

#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "../ecu/ecu_communication.h"

// Forward declarations
struct ImGuiContext;

// ECU Integration state
typedef struct {
    ECUContext* ecu_context;
    bool ecu_connected;
    char ecu_status[256];
    ECUData ecu_data;
    bool demo_mode_enabled;
    uint32_t last_demo_update;
    uint32_t last_realtime_update;
    uint32_t realtime_update_interval;
} ECUIntegrationState;

// Speeduino-specific structures and constants - using existing definitions from ecu_communication.h
// Note: These are already defined in ecu_communication.h, so we don't redefine them here

// Module initialization and cleanup
bool init_ecu_integration(void);
void cleanup_ecu_integration(void);

// ECU communication management
bool init_ecu_communication(void);
void cleanup_ecu_communication(void);

// Speeduino protocol implementation
void speeduino_init(void);
void speeduino_cleanup(void);
bool speeduino_connect(const char* port);
void speeduino_disconnect(void);
bool speeduino_is_connected(void);
bool speeduino_send_packet(SpeeduinoPacket* packet);
uint8_t speeduino_calculate_crc(uint8_t* data, int length);
bool speeduino_get_realtime_data(void);
bool speeduino_get_table_data(uint8_t table_id);
bool speeduino_set_table_data(uint8_t table_id, uint8_t* data, int length);
void speeduino_update_connection_status(void);

// ECU data management
void update_ecu_data(void);
const ECUData* get_ecu_data(void);
bool is_ecu_connected(void);
const char* get_ecu_status(void);

// Demo mode management
void enable_ecu_demo_mode(bool enabled);
void update_ecu_demo_data(void);

// UI rendering
void render_ecu_connection_panel(void);
void render_ecu_status_indicator(void);

// Legacy function names for compatibility
void init_ecu_communication_legacy(void);
void cleanup_ecu_communication_legacy(void);
void speeduino_init_legacy(void);
void speeduino_cleanup_legacy(void);
bool speeduino_connect_legacy(const char* port);
void speeduino_disconnect_legacy(void);
bool speeduino_is_connected_legacy(void);
bool speeduino_send_packet_legacy(SpeeduinoPacket* packet);
uint8_t speeduino_calculate_crc_legacy(uint8_t* data, int length);
bool speeduino_get_realtime_data_legacy(void);
bool speeduino_get_table_data_legacy(uint8_t table_id);
bool speeduino_set_table_data_legacy(uint8_t table_id, uint8_t* data, int length);
void speeduino_update_connection_status_legacy(void);
void render_ecu_connection_panel_legacy(void);

// Global state access for compatibility
extern ECUIntegrationState* g_ecu_integration_global;

#endif // ECU_INTEGRATION_H
