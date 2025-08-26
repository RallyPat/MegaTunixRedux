/*
 * Test Plugin - MegaTunix Redux
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Simple test plugin for demonstration
 */

#include "../../include/plugin/plugin_interface.h"
#include <cstdio>

// Plugin state
static bool g_plugin_initialized = false;

// Plugin lifecycle functions
static bool test_plugin_init(PluginContext* ctx) {
    printf("Test Plugin: Initializing\n");
    g_plugin_initialized = true;
    return true;
}

static void test_plugin_cleanup(void) {
    printf("Test Plugin: Cleaning up\n");
    g_plugin_initialized = false;
}

static void test_plugin_update(void) {
    // Plugin update logic (called every frame)
}

// ECU plugin interface stubs
static bool test_plugin_connect(const char* port, int baud_rate, const char* protocol) { return false; }
static bool test_plugin_disconnect(void) { return false; }
static bool test_plugin_is_connected(void) { return false; }
static const char* test_plugin_get_connection_status(void) { return "Disconnected"; }
static bool test_plugin_read_table(int table_id, float* data, int* rows, int* cols) { return false; }
static bool test_plugin_write_table(int table_id, const float* data, int rows, int cols) { return false; }
static bool test_plugin_read_realtime_data(ECURealtimeData* data) { return false; }
static bool test_plugin_write_parameter(int param_id, float value) { return false; }
static bool test_plugin_read_parameter(int param_id, float* value) { return false; }
static bool test_plugin_send_command(const char* command, char* response, int max_response_len) { return false; }
static bool test_plugin_upload_firmware(const char* firmware_path) { return false; }
static bool test_plugin_download_config(const char* config_path) { return false; }
static bool test_plugin_set_protocol_settings(const char* settings_json) { return false; }
static const char* test_plugin_get_protocol_info(void) { return "Test Protocol v1.0"; }
static bool test_plugin_validate_connection(void) { return false; }
static bool test_plugin_start_logging(const char* log_path) { return false; }
static bool test_plugin_stop_logging(void) { return false; }
static bool test_plugin_get_log_status(char* status, int max_len) { return false; }

// ECU plugin interface
static ECUPluginInterface g_ecu_interface = {
    .connect = test_plugin_connect,
    .disconnect = test_plugin_disconnect,
    .is_connected = test_plugin_is_connected,
    .get_connection_status = test_plugin_get_connection_status,
    .read_table = test_plugin_read_table,
    .write_table = test_plugin_write_table,
    .read_realtime_data = test_plugin_read_realtime_data,
    .write_parameter = test_plugin_write_parameter,
    .read_parameter = test_plugin_read_parameter,
    .send_command = test_plugin_send_command,
    .upload_firmware = test_plugin_upload_firmware,
    .download_config = test_plugin_download_config,
    .set_protocol_settings = test_plugin_set_protocol_settings,
    .get_protocol_info = test_plugin_get_protocol_info,
    .validate_connection = test_plugin_validate_connection,
    .start_logging = test_plugin_start_logging,
    .stop_logging = test_plugin_stop_logging,
    .get_log_status = test_plugin_get_log_status
};

// Main plugin interface
static PluginInterface g_plugin_interface;

// Initialize the plugin interface
static void init_plugin_interface() {
    g_plugin_interface.name = "Test ECU Plugin";
    g_plugin_interface.version = "1.0.0";
    g_plugin_interface.author = "Pat Burke";
    g_plugin_interface.description = "Simple test plugin for demonstration";
    g_plugin_interface.type = PLUGIN_TYPE_ECU;
    g_plugin_interface.status = PLUGIN_STATUS_UNLOADED;
    g_plugin_interface.init = test_plugin_init;
    g_plugin_interface.cleanup = test_plugin_cleanup;
    g_plugin_interface.update = test_plugin_update;
    g_plugin_interface.interface.ecu = g_ecu_interface;
    g_plugin_interface.internal_data = nullptr;
    g_plugin_interface.library_handle = nullptr;
}

// Plugin interface function (required by plugin system)
extern "C" PluginInterface* get_plugin_interface(void) {
    static bool initialized = false;
    if (!initialized) {
        init_plugin_interface();
        initialized = true;
    }
    return &g_plugin_interface;
}
