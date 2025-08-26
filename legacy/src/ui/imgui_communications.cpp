#include "../../include/ui/imgui_communications.h"
#include "../../include/ui/imgui_file_dialog.h"
#include "../../include/ecu/ecu_dynamic_protocols.h"
#include "../../include/ui/imgui_ve_table.h"
#include "../../external/imgui/imgui.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <math.h>
#include <SDL2/SDL.h>
#include <stdio.h>

// Common baud rates
static const int baud_rates[] = {9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600};
static const int baud_rate_count = 8;

// Protocol names
static const char* protocol_names[] = {"None", "Speeduino", "EpicEFI", "MegaSquirt", "LibreEMS"};

// Global log callback
static LogCallback g_log_callback = NULL;

// Asynchronous connection thread function
static int connection_thread_function(void* data) {
    ImGuiCommunications* comms = (ImGuiCommunications*)data;
    
    // Perform the actual connection
    bool success = ecu_connect(comms->ecu_ctx, &comms->pending_connection_config);
    
    // Set the result
    comms->connection_result = success;
    comms->connection_completed = true;
    comms->connection_thread_running = false;
    
    return 0;
}

// Create communications UI
ImGuiCommunications* imgui_communications_create(ECUContext* ecu_ctx) {
    ImGuiCommunications* comms = (ImGuiCommunications*)malloc(sizeof(ImGuiCommunications));
    if (!comms) return NULL;
    
    // Initialize state
    memset(comms, 0, sizeof(ImGuiCommunications));
    comms->ecu_ctx = ecu_ctx;
    comms->initialized = true;
    
    // Set default values
    comms->selected_protocol = ECU_PROTOCOL_SPEEDUINO;
    comms->selected_baud_rate = 115200;
    comms->timeout_ms = 1000;
    comms->reconnect_interval = 5000;
    comms->auto_connect_enabled = false;
    comms->auto_reconnect_enabled = true;
    
    // Initialize INI file support
    comms->use_ini_file = false;
    memset(comms->ini_file_path, 0, sizeof(comms->ini_file_path));
    memset(comms->detected_ecu_name, 0, sizeof(comms->detected_ecu_name));
    memset(comms->detected_ecu_version, 0, sizeof(comms->detected_ecu_version));
    comms->ini_file_valid = false;
    memset(comms->ini_error_message, 0, sizeof(comms->ini_error_message));
    
    // Initialize file dialog support
    comms->file_dialog = imgui_file_dialog_create();
    comms->show_file_dialog = false;
    
    // Initialize dynamic protocol support
    comms->dynamic_protocol_manager = ecu_dynamic_protocols_init();
    comms->show_protocol_manager = false;
    memset(comms->import_ini_path, 0, sizeof(comms->import_ini_path));
    comms->show_import_dialog = false;
    
    // Initialize demo mode support
    comms->demo_mode_enabled = false;
    memset(comms->demo_ini_file_path, 0, sizeof(comms->demo_ini_file_path));
    comms->demo_ini_file_valid = false;
    memset(comms->demo_ecu_name, 0, sizeof(comms->demo_ecu_name));
    memset(comms->demo_ecu_version, 0, sizeof(comms->demo_ecu_version));
    memset(comms->demo_error_message, 0, sizeof(comms->demo_error_message));
    
    // Initialize statistics
    memset(&comms->stats, 0, sizeof(CommunicationsStats));
    comms->stats_last_update = SDL_GetTicks();
    
    // Initialize port list
    memset(&comms->detected_ports, 0, sizeof(SerialPortList));
    
    // Initialize connection state
    comms->connecting = false;
    comms->connect_start_time = 0;
    strcpy(comms->connecting_message, "");
    
    return comms;
}

// Set log callback for communications module
void imgui_communications_set_log_callback(ImGuiCommunications* comms, LogCallback callback) {
    if (comms) {
        comms->log_callback = callback;
    }
}

void imgui_communications_set_ve_table_callbacks(ImGuiCommunications* comms,
                                                VETableResizeCallback resize_callback,
                                                VETableSetAxisRangesCallback axis_ranges_callback,
                                                VETableSetAxisNamesCallback axis_names_callback,
                                                VETableSetMetadataCallback metadata_callback) {
    if (comms) {
        comms->ve_table_resize_callback = resize_callback;
        comms->ve_table_set_axis_ranges_callback = axis_ranges_callback;
        comms->ve_table_set_axis_names_callback = axis_names_callback;
        comms->ve_table_set_metadata_callback = metadata_callback;
    }
}

// Destroy communications UI
void imgui_communications_destroy(ImGuiCommunications* comms) {
    if (comms) {
        if (comms->file_dialog) {
            imgui_file_dialog_destroy((ImGuiFileDialog*)comms->file_dialog);
        }
        if (comms->dynamic_protocol_manager) {
            ecu_dynamic_protocols_cleanup((DynamicProtocolManager*)comms->dynamic_protocol_manager);
        }
        free(comms);
    }
}

// Update communications state
void imgui_communications_update(ImGuiCommunications* comms) {
    if (!comms || !comms->initialized) return;
    
    uint32_t current_time = SDL_GetTicks();
    
    // Update statistics if connected
    if (comms->ecu_ctx && ecu_is_connected(comms->ecu_ctx)) {
        // Calculate data rates (simplified - in real implementation, you'd track actual bytes)
        if (current_time - comms->stats_last_update >= 1000) { // Update every second
            // This is a simplified calculation - in practice you'd track actual bytes transferred
            comms->stats.data_rate_rx = (float)comms->stats.bytes_received / 1.0f;
            comms->stats.data_rate_tx = (float)comms->stats.bytes_sent / 1.0f;
            comms->stats.packet_rate_rx = (float)comms->stats.packets_received / 1.0f;
            comms->stats.packet_rate_tx = (float)comms->stats.packets_sent / 1.0f;
            
            // Reset counters for next calculation
            comms->stats.bytes_received = 0;
            comms->stats.bytes_sent = 0;
            comms->stats.packets_received = 0;
            comms->stats.packets_sent = 0;
            
            comms->stats_last_update = current_time;
        }
    }
    
    // Update port scanning
    if (comms->scanning_ports) {
        if (current_time - comms->scan_start_time >= 3000) { // 3 second timeout
            comms->scanning_ports = false;
        }
    }
    
    // Handle asynchronous connection completion
    if (comms->connection_thread_running && comms->connection_completed) {
        // Connection attempt completed
        if (g_log_callback) {
            if (comms->connection_result) {
                g_log_callback(0, "Quick Connect: Connection to %s successful", 
                             comms->pending_connection_config.port);
            } else {
                g_log_callback(2, "Quick Connect: Connection to %s failed", 
                             comms->pending_connection_config.port);
            }
        }
        
        // Clean up thread
        if (comms->connection_thread) {
            SDL_WaitThread(comms->connection_thread, NULL);
            comms->connection_thread = NULL;
        }
        
        comms->connection_thread_running = false;
        comms->connection_completed = false;
    }
    
    // Clear connecting state after minimum time (1 second)
    if (comms->connecting) {
        printf("[DEBUG] Connecting state is true, time elapsed: %u ms\n", 
               current_time - comms->connect_start_time);
        if (current_time - comms->connect_start_time > 1000) {
            printf("[DEBUG] Clearing connecting state after timeout\n");
            comms->connecting = false;
        }
    }
}

// Render connection dialog
void imgui_render_connection_dialog(ImGuiCommunications* comms) {
    if (!comms) return;
    
    ImGui::Begin("ECU Connection", &comms->show_connection_dialog, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Connection Settings");
    ImGui::Separator();
    
    // Protocol selection
    ImGui::Text("Protocol:");
    if (ImGui::Combo("##Protocol", &comms->selected_protocol, protocol_names, ECU_PROTOCOL_COUNT)) {
        // Protocol changed - update default settings
        switch (comms->selected_protocol) {
            case ECU_PROTOCOL_SPEEDUINO:
                comms->selected_baud_rate = 115200;
                break;
            case ECU_PROTOCOL_EPICEFI:
                comms->selected_baud_rate = 115200;
                break;
            case ECU_PROTOCOL_MEGASQUIRT:
                comms->selected_baud_rate = 115200;
                break;
            case ECU_PROTOCOL_LIBREEMS:
                comms->selected_baud_rate = 115200;
                break;
        }
    }
    
    // Port selection
    ImGui::Text("Port:");
    if (ImGui::BeginCombo("##Port", comms->selected_port >= 0 && comms->selected_port < comms->detected_ports.count ? 
                         comms->detected_ports.ports[comms->selected_port] : "Select Port")) {
        for (int i = 0; i < comms->detected_ports.count; i++) {
            if (ImGui::Selectable(comms->detected_ports.ports[i], comms->selected_port == i)) {
                comms->selected_port = i;
            }
        }
        ImGui::EndCombo();
    }
    
    // Scan ports button
    ImGui::SameLine();
    if (ImGui::Button("Scan Ports")) {
        comms->detected_ports = ecu_detect_serial_ports();
        comms->scanning_ports = true;
        comms->scan_start_time = SDL_GetTicks();
    }
    
    // Baud rate selection
    ImGui::Text("Baud Rate:");
    if (ImGui::BeginCombo("##BaudRate", std::to_string(comms->selected_baud_rate).c_str())) {
        for (int i = 0; i < baud_rate_count; i++) {
            if (ImGui::Selectable(std::to_string(baud_rates[i]).c_str(), comms->selected_baud_rate == baud_rates[i])) {
                comms->selected_baud_rate = baud_rates[i];
            }
        }
        ImGui::EndCombo();
    }
    
    // Timeout setting
    ImGui::SliderInt("Timeout (ms)", &comms->timeout_ms, 100, 10000);
    
    // INI file support
    ImGui::Separator();
    ImGui::Text("INI File Configuration (Universal ECU Support)");
    
    ImGui::Checkbox("Use INI file for automatic protocol detection", &comms->use_ini_file);
    
    if (comms->use_ini_file) {
        ImGui::Text("INI File Path:");
        if (ImGui::InputText("##INIFilePath", comms->ini_file_path, sizeof(comms->ini_file_path))) {
            // Validate INI file when path changes
            if (strlen(comms->ini_file_path) > 0) {
                comms->ini_file_valid = ecu_validate_ini_file(comms->ini_file_path);
                if (comms->ini_file_valid) {
                    ecu_get_ini_file_info(comms->ini_file_path, comms->detected_ecu_name, comms->detected_ecu_version);
                    memset(comms->ini_error_message, 0, sizeof(comms->ini_error_message));
                } else {
                    strncpy(comms->ini_error_message, ecu_get_ini_error(), sizeof(comms->ini_error_message) - 1);
                }
            } else {
                comms->ini_file_valid = false;
                memset(comms->ini_error_message, 0, sizeof(comms->ini_error_message));
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Browse##INIFile")) {
            comms->show_file_dialog = true;
            imgui_file_dialog_open((ImGuiFileDialog*)comms->file_dialog, "Select INI File", "INI files (*.ini)");
        }
        
        // Show INI file validation status
        if (comms->ini_file_valid) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ INI file is valid");
            if (strlen(comms->detected_ecu_name) > 0) {
                ImGui::Text("Detected ECU: %s", comms->detected_ecu_name);
            }
            if (strlen(comms->detected_ecu_version) > 0) {
                ImGui::Text("ECU Version: %s", comms->detected_ecu_version);
            }
        } else if (strlen(comms->ini_file_path) > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "❌ INI file error: %s", comms->ini_error_message);
        }
        
        ImGui::Separator();
    }
    
    // Demo Mode Support
    ImGui::Separator();
    ImGui::Text("Demo Mode (Testing with Simulated Data)");
    
    ImGui::Checkbox("Enable Demo Mode", &comms->demo_mode_enabled);
    
    if (comms->demo_mode_enabled) {
        ImGui::Text("Demo INI File Path (for protocol detection):");
        if (ImGui::InputText("##DemoINIFilePath", comms->demo_ini_file_path, sizeof(comms->demo_ini_file_path))) {
            // Validate demo INI file when path changes
            if (strlen(comms->demo_ini_file_path) > 0) {
                comms->demo_ini_file_valid = ecu_validate_ini_file(comms->demo_ini_file_path);
                if (comms->demo_ini_file_valid) {
                    ecu_get_ini_file_info(comms->demo_ini_file_path, comms->demo_ecu_name, comms->demo_ecu_version);
                    memset(comms->demo_error_message, 0, sizeof(comms->demo_error_message));
                } else {
                    strncpy(comms->demo_error_message, ecu_get_ini_error(), sizeof(comms->demo_error_message) - 1);
                }
            } else {
                comms->demo_ini_file_valid = false;
                memset(comms->demo_error_message, 0, sizeof(comms->demo_error_message));
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Browse##DemoINIFile")) {
            comms->show_file_dialog = true;
            imgui_file_dialog_open((ImGuiFileDialog*)comms->file_dialog, "Select Demo INI File", "INI files (*.ini)");
        }
        
        // Show demo INI file validation status
        if (comms->demo_ini_file_valid) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Demo INI file is valid");
            if (strlen(comms->demo_ecu_name) > 0) {
                ImGui::Text("Demo ECU: %s", comms->demo_ecu_name);
            }
            if (strlen(comms->demo_ecu_version) > 0) {
                ImGui::Text("Demo ECU Version: %s", comms->demo_ecu_version);
            }
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⚠️  Using simulated data - no real ECU connection");
        } else if (strlen(comms->demo_ini_file_path) > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "❌ Demo INI file error: %s", comms->demo_error_message);
        }
        
        ImGui::Separator();
    }
    
    // Auto-connect options
    ImGui::Checkbox("Auto-connect on startup", &comms->auto_connect_enabled);
    ImGui::Checkbox("Auto-reconnect on disconnect", &comms->auto_reconnect_enabled);
    
    if (comms->auto_reconnect_enabled) {
        ImGui::SliderInt("Reconnect Interval (ms)", &comms->reconnect_interval, 1000, 30000);
    }
    
    ImGui::Separator();
    
    // Connection buttons
    if (comms->ecu_ctx && ecu_is_connected(comms->ecu_ctx)) {
        if (ImGui::Button("Disconnect", ImVec2(120, 30))) {
            ecu_disconnect(comms->ecu_ctx);
        }
    } else {
        // Check if we're currently connecting
        ECUConnectionState state = ecu_get_state(comms->ecu_ctx);
        bool is_connecting = (state == ECU_STATE_CONNECTING);
        
        if (is_connecting) {
            // Show connecting state with animated dots
            uint32_t current_time = SDL_GetTicks();
            int dot_count = ((current_time / 500) % 4); // Animate dots every 500ms
            char connecting_text[64];
            snprintf(connecting_text, sizeof(connecting_text), "Connecting%s", 
                    dot_count == 0 ? "" : 
                    dot_count == 1 ? "." : 
                    dot_count == 2 ? ".." : "...");
            
            ImGui::Button(connecting_text, ImVec2(120, 30));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Please wait...");
        } else {
            if (ImGui::Button("Connect", ImVec2(120, 30))) {
                printf("[DEBUG] CONNECT BUTTON CLICKED!\n");
                printf("[DEBUG] Demo mode enabled: %s\n", comms->demo_mode_enabled ? "YES" : "NO");
                printf("[DEBUG] Demo INI file valid: %s\n", comms->demo_ini_file_valid ? "YES" : "NO");
                printf("[DEBUG] Demo INI file path: %s\n", comms->demo_ini_file_path ? comms->demo_ini_file_path : "NULL");
                
                bool connection_success = false;
                
                if (comms->demo_mode_enabled && comms->demo_ini_file_valid) {
                    // Demo mode with INI file
                    printf("[DEBUG] Starting DEMO mode with INI file: %s\n", comms->demo_ini_file_path);
                    
                    if (g_log_callback) {
                        g_log_callback(0, "Starting DEMO mode with INI file: %s", comms->demo_ini_file_path);
                        g_log_callback(0, "Using simulated data - no real ECU connection");
                    }
                    
                    // Load INI file for protocol detection and VE table configuration
                    printf("[DEBUG] Loading INI file...\n");
                    INIConfig* ini_config = ecu_load_ini_file(comms->demo_ini_file_path);
                    if (ini_config) {
                        printf("[DEBUG] INI file loaded successfully\n");
                        printf("[DEBUG] INI Config - ECU Name: %s, Version: %s\n", ini_config->ecu_name, ini_config->ecu_version);
                        printf("[DEBUG] INI Config - Table Count: %d\n", ini_config->table_count);
                        
                        // Log all tables found
                        for (int i = 0; i < ini_config->table_count; i++) {
                            printf("[DEBUG] Table %d: %s - %dx%d\n", i, ini_config->tables[i].name, 
                                   ini_config->tables[i].width, ini_config->tables[i].height);
                        }
                        
                        if (g_log_callback) {
                            g_log_callback(0, "INI file loaded successfully");
                        }
                        
                        // Set demo mode in ECU context
                        printf("[DEBUG] Setting demo mode...\n");
                        ecu_set_demo_mode(comms->ecu_ctx, true);
                        ecu_set_demo_ini_config(comms->ecu_ctx, ini_config);
                        
                        // Also set global demo mode for consistency
                        ecu_set_global_demo_mode(true);
                        
                        // Configure VE table based on INI file
                        printf("[DEBUG] Configuring VE table from INI...\n");
                        if (g_log_callback) {
                            g_log_callback(0, "Configuring VE table from INI file...");
                        }
                        
                        // Find VE table in the configuration
                        printf("[DEBUG] Finding VE table...\n");
                        INITableInfo* ve_table = ecu_find_table_by_name(ini_config, "veTable");
                                        if (ve_table) {
                    printf("[DEBUG] Found VE table: %dx%d\n", ve_table->width, ve_table->height);
                    printf("[DEBUG] VE table name: %s\n", ve_table->name);
                            if (g_log_callback) {
                                g_log_callback(0, "Found VE table: %dx%d", ve_table->width, ve_table->height);
                                g_log_callback(0, "VE table axis ranges: X(%.0f-%.0f %s), Y(%.0f-%.0f %s)", 
                                              ve_table->x_min, ve_table->x_max, ve_table->x_units,
                                              ve_table->y_min, ve_table->y_max, ve_table->y_units);
                            }
                            
                            // Check if callbacks are set
                            printf("[DEBUG] Checking callbacks...\n");
                            if (!comms->ve_table_resize_callback) {
                                printf("[DEBUG] ERROR: VE table resize callback is NULL!\n");
                                if (g_log_callback) {
                                    g_log_callback(2, "ERROR: VE table resize callback is NULL!");
                                }
                            } else {
                                printf("[DEBUG] Calling VE table resize callback...\n");
                                if (g_log_callback) {
                                    g_log_callback(0, "Calling VE table resize callback...");
                                }
                                
                                // Use callbacks to configure the VE table
                                bool resize_success = false;
                                if (comms->ve_table_resize_callback) {
                                    printf("[DEBUG] Calling resize callback with %dx%d\n", ve_table->width, ve_table->height);
                                    resize_success = comms->ve_table_resize_callback(ve_table->width, ve_table->height);
                                    printf("[DEBUG] Resize callback returned: %s\n", resize_success ? "SUCCESS" : "FAILED");
                                } else {
                                    printf("[DEBUG] ERROR: VE table resize callback is NULL\n");
                                    if (g_log_callback) {
                                        g_log_callback(2, "ERROR: VE table resize callback is NULL");
                                    }
                                }
                                
                                if (resize_success) {
                                    printf("[DEBUG] Setting axis ranges...\n");
                                    
                                    // Set axis ranges from INI
                                    if (comms->ve_table_set_axis_ranges_callback) {
                                        comms->ve_table_set_axis_ranges_callback(ve_table->x_min, ve_table->x_max, 
                                                                               ve_table->y_min, ve_table->y_max);
                                    }
                                    
                                    // Set axis names and units
                                    if (comms->ve_table_set_axis_names_callback) {
                                        comms->ve_table_set_axis_names_callback(ve_table->x_axis_name, ve_table->y_axis_name,
                                                                              ve_table->x_units, ve_table->y_units);
                                    }
                                    
                                    // Skip metadata callback in demo mode since min/max values are calculated from demo data
                                    // The demo data loading already sets the correct min/max values
                                    
                                    printf("[DEBUG] VE table configuration complete\n");
                                    if (g_log_callback) {
                                        g_log_callback(0, "VE table configured: %dx%d, X: %.0f-%.0f %s, Y: %.0f-%.0f %s", 
                                                      ve_table->width, ve_table->height,
                                                      ve_table->x_min, ve_table->x_max, ve_table->x_units,
                                                      ve_table->y_min, ve_table->y_max, ve_table->y_units);
                                    }
                                } else {
                                    printf("[DEBUG] Failed to resize VE table\n");
                                    if (g_log_callback) {
                                        g_log_callback(2, "Failed to resize VE table to %dx%d", ve_table->width, ve_table->height);
                                    }
                                }
                            }
                        } else {
                            printf("[DEBUG] No VE table found in INI file\n");
                            printf("[DEBUG] Available tables:\n");
                            for (int i = 0; i < ini_config->table_count; i++) {
                                printf("[DEBUG]   - '%s' (length: %zu)\n", ini_config->tables[i].name, strlen(ini_config->tables[i].name));
                            }
                            printf("[DEBUG] Searching for 'veTable' (length: %zu)\n", strlen("veTable"));
                            if (g_log_callback) {
                                g_log_callback(1, "No VE table found in INI file");
                            }
                        }
                        
                        // IMPORTANT: Don't free ini_config here - it's stored in ecu_ctx
                        // ecu_free_ini_config(ini_config); // REMOVED - causes double-free
                        
                        // Simulate successful connection
                        connection_success = true;
                        
                        printf("[DEBUG] DEMO mode activated successfully\n");
                        if (g_log_callback) {
                            g_log_callback(0, "DEMO mode activated successfully");
                        }
                    } else {
                        printf("[DEBUG] Failed to load demo INI file\n");
                        if (g_log_callback) {
                            g_log_callback(2, "Failed to load demo INI file: %s", ecu_get_ini_error());
                        }
                    }
                } else if (comms->selected_port >= 0 && comms->selected_port < comms->detected_ports.count) {
                    if (comms->use_ini_file && comms->ini_file_valid) {
                        // Use INI-based connection
                        printf("[DEBUG] Attempting INI-based connection to %s using %s\n", 
                               comms->detected_ports.ports[comms->selected_port], comms->ini_file_path);
                        
                        if (g_log_callback) {
                            g_log_callback(0, "Initiating INI-based connection to %s using %s", 
                                         comms->detected_ports.ports[comms->selected_port], comms->ini_file_path);
                        }
                        
                        connection_success = ecu_connect_with_ini(comms->ecu_ctx, 
                                                               comms->detected_ports.ports[comms->selected_port], 
                                                               comms->ini_file_path);
                    } else {
                        // Use traditional connection
                        ECUConfig config = ecu_config_default();
                        config.protocol = (ECUProtocol)comms->selected_protocol;
                        strcpy(config.port, comms->detected_ports.ports[comms->selected_port]);
                        config.baud_rate = comms->selected_baud_rate;
                        config.timeout_ms = comms->timeout_ms;
                        config.auto_reconnect = comms->auto_reconnect_enabled;
                        config.reconnect_interval = comms->reconnect_interval;
                        
                        printf("[DEBUG] Attempting to connect to %s on %s at %d baud\n", 
                               protocol_names[comms->selected_protocol], config.port, config.baud_rate);
                        
                        if (g_log_callback) {
                            g_log_callback(0, "Initiating connection to %s on %s at %d baud", 
                                         protocol_names[comms->selected_protocol], config.port, config.baud_rate);
                        }
                        
                        connection_success = ecu_connect(comms->ecu_ctx, &config);
                    }
                    
                    if (connection_success) {
                        // Add to connection history
                        const char* port_name = comms->detected_ports.ports[comms->selected_port];
                        const char* protocol_name = comms->use_ini_file ? "Auto-detected" : protocol_names[comms->selected_protocol];
                        
                        snprintf(comms->connection_history[comms->history_index], 256, 
                                "Connected to %s on %s", 
                                protocol_name, port_name);
                        comms->history_index = (comms->history_index + 1) % 10;
                        if (comms->history_count < 10) comms->history_count++;
                        
                        printf("[DEBUG] Connection successful!\n");
                        if (g_log_callback) {
                            g_log_callback(0, "Connection to %s successful", port_name);
                        }
                    } else {
                        printf("[DEBUG] Connection failed!\n");
                        const char* port_name = comms->detected_ports.ports[comms->selected_port];
                        if (g_log_callback) {
                            g_log_callback(2, "Connection to %s failed", port_name);
                        }
                        // Add failed connection to history
                        const char* protocol_name = comms->use_ini_file ? "Auto-detected" : protocol_names[comms->selected_protocol];
                        snprintf(comms->connection_history[comms->history_index], 256, 
                                "Failed to connect to %s on %s", 
                                protocol_name, port_name);
                        comms->history_index = (comms->history_index + 1) % 10;
                        if (comms->history_count < 10) comms->history_count++;
                    }
                } else {
                    // No port selected - show error
                    ImGui::OpenPopup("No Port Selected");
                }
            }
        }
    }
    
    // Error popup for no port selected
    if (ImGui::BeginPopupModal("No Port Selected", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("No port selected.");
        ImGui::Text("Please scan for ports and select one to connect.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Test Port", ImVec2(120, 30))) {
        if (comms->selected_port >= 0 && comms->selected_port < comms->detected_ports.count) {
            printf("[DEBUG] Testing port: %s\n", comms->detected_ports.ports[comms->selected_port]);
            
            // Add safety check to prevent crashes
            if (comms->ecu_ctx) {
                bool success = ecu_test_serial_port(comms->detected_ports.ports[comms->selected_port], 
                                                   (ECUProtocol)comms->selected_protocol);
                if (success) {
                    ImGui::OpenPopup("Port Test Success");
                } else {
                    ImGui::OpenPopup("Port Test Failed");
                }
            } else {
                printf("[DEBUG] ECU context is NULL, cannot test port\n");
                ImGui::OpenPopup("Port Test Failed");
            }
        } else {
            printf("[DEBUG] Invalid port selection\n");
            ImGui::OpenPopup("Port Test Failed");
        }
    }
    
    // Test result popups
    if (ImGui::BeginPopupModal("Port Test Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Port test successful! ECU detected.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    if (ImGui::BeginPopupModal("Port Test Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Port test failed. No ECU detected or wrong protocol.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::End();
}

// Render port scan results
void imgui_render_port_scan(ImGuiCommunications* comms) {
    if (!comms) return;
    
    ImGui::Begin("Port Scan Results", &comms->show_port_scan, ImGuiWindowFlags_AlwaysAutoResize);
    
    if (comms->scanning_ports) {
        ImGui::Text("Scanning for serial ports...");
        ImGui::ProgressBar(0.5f, ImVec2(200, 20), ""); // Animated progress bar
    } else {
        ImGui::Text("Detected Ports (%d found):", comms->detected_ports.count);
        ImGui::Separator();
        
        if (comms->detected_ports.count == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No serial ports detected");
        } else {
            for (int i = 0; i < comms->detected_ports.count; i++) {
                ImGui::Text("%d: %s", i + 1, comms->detected_ports.ports[i]);
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button("Refresh Scan")) {
            comms->detected_ports = ecu_detect_serial_ports();
        }
    }
    
    ImGui::End();
}

// Render communications statistics
void imgui_render_statistics(ImGuiCommunications* comms) {
    if (!comms) return;
    
    ImGui::Begin("Communications Statistics", &comms->show_statistics, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Data Transfer Statistics");
    ImGui::Separator();
    
    // Connection status
    if (comms->ecu_ctx) {
        ECUConnectionState state = ecu_get_state(comms->ecu_ctx);
        ImGui::Text("Status: %s", ecu_get_state_name(state));
        
        if (ecu_is_connected(comms->ecu_ctx)) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Connected");
            
            // Get real-time statistics from ECU
            uint32_t bytes_rx, bytes_tx, packets_rx, packets_tx, errors, timeouts, last_activity;
            float rx_rate, tx_rate, rx_packet_rate, tx_packet_rate;
            
            ecu_get_statistics(comms->ecu_ctx, &bytes_rx, &bytes_tx, &packets_rx, &packets_tx, &errors, &timeouts, &last_activity);
            ecu_get_rates(comms->ecu_ctx, &rx_rate, &tx_rate, &rx_packet_rate, &tx_packet_rate);
            
            // Get adaptive timing statistics
            uint32_t avg_time, min_time, max_time;
            bool timing_initialized;
            ecu_get_timing_stats(comms->ecu_ctx, &avg_time, &min_time, &max_time, &timing_initialized);
            
            // Data rates
            ImGui::Separator();
            ImGui::Text("Data Rates:");
            ImGui::Text("  RX: %.1f bytes/s", rx_rate);
            ImGui::Text("  TX: %.1f bytes/s", tx_rate);
            ImGui::Text("  RX Packets: %.1f packets/s", rx_packet_rate);
            ImGui::Text("  TX Packets: %.1f packets/s", tx_packet_rate);
            
            // Totals
            ImGui::Separator();
            ImGui::Text("Totals:");
            ImGui::Text("  Bytes Received: %u", bytes_rx);
            ImGui::Text("  Bytes Sent: %u", bytes_tx);
            ImGui::Text("  Packets Received: %u", packets_rx);
            ImGui::Text("  Packets Sent: %u", packets_tx);
            
            // Errors
            ImGui::Separator();
            ImGui::Text("Errors:");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "  Errors: %u", errors);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Timeouts: %u", timeouts);
            
            // Last activity
            ImGui::Separator();
            uint32_t current_time = SDL_GetTicks();
            uint32_t time_since_activity = current_time - last_activity;
            ImGui::Text("Last Activity: %.1f seconds ago", time_since_activity / 1000.0f);
            
            // Adaptive timing statistics
            ImGui::Separator();
            ImGui::Text("Adaptive Timing:");
            if (timing_initialized) {
                ImGui::Text("  Average Response: %ums", avg_time);
                ImGui::Text("  Fastest Response: %ums", min_time);
                ImGui::Text("  Slowest Response: %ums", max_time);
                ImGui::Text("  Current Timeout: %ums", ecu_get_adaptive_timeout(comms->ecu_ctx) / 1000);
            } else {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "  Learning response times...");
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Disconnected");
        }
    }
    
    ImGui::End();
}

// Render protocol information
void imgui_render_protocol_info(ImGuiCommunications* comms) {
    if (!comms) return;
    
    ImGui::Begin("Protocol Information", &comms->show_protocol_info, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Supported ECU Protocols");
    ImGui::Separator();
    
    // Speeduino
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Speeduino");
    ImGui::Text("  Default Baud Rate: 115200");
    ImGui::Text("  Protocol: Serial ASCII");
    ImGui::Text("  Features: Real-time data, table editing, logging");
    ImGui::Text("  Status: Fully supported");
    
    ImGui::Separator();
    
    // EpicEFI
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "EpicEFI");
    ImGui::Text("  Default Baud Rate: 115200");
    ImGui::Text("  Protocol: Serial ASCII");
    ImGui::Text("  Features: Real-time data, table editing, logging");
    ImGui::Text("  Status: Fully supported");
    
    ImGui::Separator();
    
    // MegaSquirt
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "MegaSquirt");
    ImGui::Text("  Default Baud Rate: 115200");
    ImGui::Text("  Protocol: Serial ASCII");
    ImGui::Text("  Features: Real-time data, table editing, logging");
    ImGui::Text("  Status: Basic support");
    
    ImGui::Separator();
    
    // LibreEMS
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "LibreEMS");
    ImGui::Text("  Default Baud Rate: 115200");
    ImGui::Text("  Protocol: Serial ASCII");
    ImGui::Text("  Features: Real-time data, table editing, logging");
    ImGui::Text("  Status: Basic support");
    
    ImGui::End();
}

// Render connection history
void imgui_render_connection_history(ImGuiCommunications* comms) {
    if (!comms) return;
    
    ImGui::Begin("Connection History", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Recent Connections");
    ImGui::Separator();
    
    if (comms->history_count == 0) {
        ImGui::Text("No connection history");
    } else {
        for (int i = 0; i < comms->history_count; i++) {
            int index = (comms->history_index - 1 - i + 10) % 10;
            ImGui::Text("%s", comms->connection_history[index]);
        }
    }
    
    ImGui::Separator();
    if (ImGui::Button("Clear History")) {
        comms->history_count = 0;
        comms->history_index = 0;
    }
    
    ImGui::End();
}

// Main render function
void imgui_communications_render(ImGuiCommunications* comms) {
    if (!comms || !comms->initialized) return;
    
    // Safety check for ECU context
    if (!comms->ecu_ctx) {
        ImGui::Begin("Communications Management");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Error: ECU context not available!");
        ImGui::End();
        return;
    }
    
    // Update communications state
    imgui_communications_update(comms);
    
    // Main communications panel
    ImGui::Begin("Communications Management");
    
    ImGui::Text("ECU Communications");
    ImGui::Separator();
    
    // Connection status
    if (comms->ecu_ctx) {
        ECUConnectionState state = ecu_get_state(comms->ecu_ctx);
        const char* state_name = ecu_get_state_name(state);
        
        ImGui::Text("Status: ");
        ImGui::SameLine();
        
        switch (state) {
            case ECU_STATE_CONNECTED:
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", state_name);
                break;
            case ECU_STATE_CONNECTING: {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", state_name);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "- Attempting to establish connection...");
                
                // Add a progress bar for visual feedback
                uint32_t current_time = SDL_GetTicks();
                float progress = (current_time % 2000) / 2000.0f; // 2 second cycle
                ImGui::ProgressBar(progress, ImVec2(-1, 6), "");
                break;
            }
            case ECU_STATE_ERROR:
            case ECU_STATE_TIMEOUT:
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", state_name);
                break;
            default:
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", state_name);
                break;
        }
        
        if (ecu_is_connected(comms->ecu_ctx)) {
            ImGui::Text("Protocol: %s", ecu_get_protocol_name(comms->ecu_ctx->protocol));
            ImGui::Text("Port: %s", comms->ecu_ctx->config.port);
            ImGui::Text("Baud Rate: %d", comms->ecu_ctx->config.baud_rate);
        }
    }
    
    ImGui::Separator();
    
    // Control buttons
    if (ImGui::Button("Connection Settings", ImVec2(150, 30))) {
        comms->show_connection_dialog = true;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Port Scan", ImVec2(100, 30))) {
        comms->show_port_scan = true;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Statistics", ImVec2(100, 30))) {
        comms->show_statistics = true;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Protocol Info", ImVec2(100, 30))) {
        comms->show_protocol_info = true;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Protocol Manager", ImVec2(120, 30))) {
        comms->show_protocol_manager = true;
    }
    
    ImGui::Separator();
    
    // Quick connection controls
    printf("[DEBUG] Checking ECU connection state for Quick Connect button\n");
    if (comms->ecu_ctx && ecu_is_connected(comms->ecu_ctx)) {
        printf("[DEBUG] ECU is connected, showing Disconnect button\n");
        if (ImGui::Button("Disconnect", ImVec2(120, 30))) {
            ecu_disconnect(comms->ecu_ctx);
        }
    } else {
        printf("[DEBUG] ECU is NOT connected, showing Quick Connect button\n");
        

        
        // Check if we're currently connecting (either ECU state or local state)
        ECUConnectionState state = ecu_get_state(comms->ecu_ctx);
        bool is_connecting = (state == ECU_STATE_CONNECTING) || comms->connecting;
        
        // Debug output for connection state
        static uint32_t last_debug_time = 0;
        uint32_t current_time = SDL_GetTicks();
        if (current_time - last_debug_time > 500) { // Debug every 500ms
            printf("[DEBUG] Render: ECU state=%d, local connecting=%d, is_connecting=%d\n", 
                   state, comms->connecting, is_connecting);
            last_debug_time = current_time;
        }
        
        if (is_connecting) {
            // Show connecting state with animated dots
            uint32_t current_time = SDL_GetTicks();
            int dot_count = ((current_time / 500) % 4); // Animate dots every 500ms
            char connecting_text[64];
            snprintf(connecting_text, sizeof(connecting_text), "Connecting%s", 
                    dot_count == 0 ? "" : 
                    dot_count == 1 ? "." : 
                    dot_count == 2 ? ".." : "...");
            
            ImGui::Button(connecting_text, ImVec2(120, 30));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Please wait...");
        } else {
            printf("[DEBUG] Rendering Quick Connect button (not connecting state)\n");
            bool button_clicked = ImGui::Button("Quick Connect", ImVec2(120, 30));
            if (button_clicked) {
                printf("\n[DEBUG] ==========================================\n");
                printf("[DEBUG] Quick Connect button pressed!\n");
                printf("[DEBUG] ==========================================\n\n");
                
                // Add log entry to UI Log immediately
                printf("[DEBUG] Attempting to log 'Quick Connect button clicked!' to UI Log.\n");
                if (g_log_callback) {
                    printf("[DEBUG] Log callback is available, calling it...\n");
                    g_log_callback(0, "Quick Connect button clicked!");
                    printf("[DEBUG] Log callback completed.\n");
                } else {
                    printf("[DEBUG] Log callback is NULL at button click time!\n");
                }
                
                // First, scan for ports if none are detected
                if (comms->detected_ports.count == 0) {
                    printf("[DEBUG] No ports detected, scanning...\n");
                    comms->detected_ports = ecu_detect_serial_ports();
                    printf("[DEBUG] Found %d ports\n", comms->detected_ports.count);
                }
                
                // Set connecting state and log immediately (before connection attempt)
                printf("[DEBUG] Setting connecting state to true\n");
                comms->connecting = true;
                comms->connect_start_time = SDL_GetTicks();
                printf("[DEBUG] Connecting state set, start time: %u\n", comms->connect_start_time);
                
                // Log connection attempt messages BEFORE starting the connection process
                // This ensures they appear immediately when button is clicked
                if (g_log_callback) {
                    g_log_callback(0, "Quick Connect: Initiating connection to Speeduino");
                    g_log_callback(0, "Quick Connect: Connection attempt in progress...");
                }
                
                // Try to connect with default settings
                if (comms->detected_ports.count > 0) {
                    ECUConfig config = ecu_config_speeduino();
                    strcpy(config.port, comms->detected_ports.ports[0]);
                    
                    // Log connection details
                    if (g_log_callback) {
                        g_log_callback(0, "Quick Connect: Attempting connection to %s on %s at %d baud", 
                                     "Speeduino", config.port, config.baud_rate);
                    }
                    
                    // Start asynchronous connection attempt
                    if (!comms->connection_thread_running) {
                        // Store the config for the thread
                        memcpy(&comms->pending_connection_config, &config, sizeof(ECUConfig));
                        comms->connection_completed = false;
                        comms->connection_thread_running = true;
                        
                        // Create and start the connection thread
                        comms->connection_thread = SDL_CreateThread(connection_thread_function, "ConnectionThread", comms);
                        if (!comms->connection_thread) {
                            // Fallback to synchronous connection if thread creation fails
                            printf("[DEBUG] Thread creation failed, falling back to synchronous connection\n");
                            bool success = ecu_connect(comms->ecu_ctx, &config);
                            
                            // Log connection result
                            if (g_log_callback) {
                                if (success) {
                                    g_log_callback(0, "Quick Connect: Connection to %s successful", config.port);
                                } else {
                                    g_log_callback(2, "Quick Connect: Connection to %s failed", config.port);
                                }
                            }
                        } else {
                            printf("[DEBUG] Connection thread started successfully\n");
                        }
                    }
                } else {
                    // No ports detected - show error
                    if (g_log_callback) {
                        g_log_callback(2, "Quick Connect: No serial ports detected");
                    }
                    ImGui::OpenPopup("No Ports Found");
                }
            }
        }
    }
    
    // Error popup for no ports
    if (ImGui::BeginPopupModal("No Ports Found", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("No serial ports detected.");
        ImGui::Text("Please scan for ports first or check your connections.");
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    // Error display
    if (comms->error_occurred) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Last Error:");
        ImGui::TextWrapped("%s", comms->last_error);
    }
    
    ImGui::End();
    
    // Render sub-windows
    if (comms->show_connection_dialog) {
        imgui_render_connection_dialog(comms);
    }
    
    if (comms->show_port_scan) {
        imgui_render_port_scan(comms);
    }
    
    if (comms->show_statistics) {
        imgui_render_statistics(comms);
    }
    
    if (comms->show_protocol_info) {
        imgui_render_protocol_info(comms);
    }
    
    // Render file dialog
    if (comms->show_file_dialog && comms->file_dialog) {
        ImGuiFileDialog* dialog = (ImGuiFileDialog*)comms->file_dialog;
        if (imgui_file_dialog_render(dialog)) {
            // File was selected
            const char* selected_file = imgui_file_dialog_get_selected_file(dialog);
            if (selected_file && strlen(selected_file) > 0) {
                // Determine which field to update based on context
                if (comms->demo_mode_enabled) {
                    // Update demo INI file path
                    strncpy(comms->demo_ini_file_path, selected_file, sizeof(comms->demo_ini_file_path) - 1);
                    
                    // Validate the selected file for demo mode
                    if (strlen(comms->demo_ini_file_path) > 0) {
                        comms->demo_ini_file_valid = ecu_validate_ini_file(comms->demo_ini_file_path);
                        if (comms->demo_ini_file_valid) {
                            ecu_get_ini_file_info(comms->demo_ini_file_path, comms->demo_ecu_name, comms->demo_ecu_version);
                            memset(comms->demo_error_message, 0, sizeof(comms->demo_error_message));
                        } else {
                            strncpy(comms->demo_error_message, ecu_get_ini_error(), sizeof(comms->demo_error_message) - 1);
                        }
                    }
                } else {
                    // Update regular INI file path
                    strncpy(comms->ini_file_path, selected_file, sizeof(comms->ini_file_path) - 1);
                    
                    // Validate the selected file
                    if (strlen(comms->ini_file_path) > 0) {
                        comms->ini_file_valid = ecu_validate_ini_file(comms->ini_file_path);
                        if (comms->ini_file_valid) {
                            ecu_get_ini_file_info(comms->ini_file_path, comms->detected_ecu_name, comms->detected_ecu_version);
                            memset(comms->ini_error_message, 0, sizeof(comms->ini_error_message));
                        } else {
                            strncpy(comms->ini_error_message, ecu_get_ini_error(), sizeof(comms->ini_error_message) - 1);
                        }
                    }
                }
            }
            comms->show_file_dialog = false;
        } else if (!dialog->is_open) {
            comms->show_file_dialog = false;
        }
    }
    
    // Render protocol manager dialog
    if (comms->show_protocol_manager) {
        imgui_render_protocol_manager(comms);
    }
    
    // Render import dialog
    if (comms->show_import_dialog) {
        imgui_render_import_dialog(comms);
    }
    
    // Always show connection history
    imgui_render_connection_history(comms);
} 

// Render protocol manager dialog
void imgui_render_protocol_manager(ImGuiCommunications* comms) {
    if (!comms || !comms->dynamic_protocol_manager) return;
    
    DynamicProtocolManager* manager = (DynamicProtocolManager*)comms->dynamic_protocol_manager;
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Dynamic ECU Protocol Manager", &comms->show_protocol_manager)) {
        
        ImGui::Text("Dynamic ECU Protocols");
        ImGui::Text("Import INI files to add support for new ECU types");
        ImGui::Separator();
        
        // Import section
        ImGui::Text("Import New ECU Protocol");
        if (ImGui::Button("Import INI File", ImVec2(150, 30))) {
            comms->show_import_dialog = true;
        }
        
        ImGui::SameLine();
        ImGui::Text("Select a TunerStudio INI file to add ECU support");
        
        ImGui::Separator();
        
        // Protocol list
        ImGui::Text("Installed Protocols");
        
        if (manager->count == 0) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No dynamic protocols installed");
            ImGui::Text("Import an INI file to add your first ECU protocol");
        } else {
            for (int i = 0; i < manager->count; i++) {
                const DynamicProtocol* protocol = &manager->protocols[i];
                
                ImGui::PushID(i);
                
                // Protocol name and status
                ImVec4 status_color = protocol->enabled ? 
                    ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                ImGui::TextColored(status_color, "%s", protocol->name);
                
                // Details
                ImGui::SameLine();
                ImGui::Text("(Confidence: %.1f%%)", protocol->confidence * 100.0f);
                
                // Buttons
                ImGui::SameLine();
                if (protocol->enabled) {
                    if (ImGui::Button("Disable")) {
                        ecu_dynamic_protocols_enable(manager, protocol->name, false);
                    }
                } else {
                    if (ImGui::Button("Enable")) {
                        ecu_dynamic_protocols_enable(manager, protocol->name, true);
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Remove")) {
                    ecu_dynamic_protocols_remove(manager, protocol->name);
                }
                
                // Protocol details
                if (ImGui::TreeNode(protocol->name)) {
                    ImGui::Text("Signature: %s", protocol->signature);
                    ImGui::Text("INI File: %s", protocol->ini_file_path);
                    ImGui::Text("ECU Name: %s", protocol->ecu_name);
                    ImGui::Text("ECU Version: %s", protocol->ecu_version);
                    ImGui::Text("Status: %s", protocol->enabled ? "Enabled" : "Disabled");
                    ImGui::TreePop();
                }
                
                ImGui::PopID();
            }
        }
        
        ImGui::Separator();
        
        // Built-in protocols info
        ImGui::Text("Built-in Protocols");
        ImGui::Text("Speeduino, EpicEFI, MegaSquirt, LibreEMS");
        ImGui::Text("These protocols are always available and cannot be removed");
        
        ImGui::End();
    }
}

// Render import dialog
void imgui_render_import_dialog(ImGuiCommunications* comms) {
    if (!comms) return;
    
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Import ECU Protocol", &comms->show_import_dialog)) {
        
        ImGui::Text("Import New ECU Protocol from INI File");
        ImGui::Text("Select a TunerStudio INI file to add support for a new ECU type");
        ImGui::Separator();
        
        // File selection
        ImGui::Text("INI File Path:");
        if (ImGui::InputText("##ImportINIPath", comms->import_ini_path, sizeof(comms->import_ini_path))) {
            // Validate file when path changes
            if (strlen(comms->import_ini_path) > 0) {
                bool valid = ecu_dynamic_protocols_validate_ini_for_import(comms->import_ini_path);
                if (!valid) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Invalid INI file");
                }
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Browse##Import")) {
            // Open file dialog for import
            comms->show_file_dialog = true;
            imgui_file_dialog_open((ImGuiFileDialog*)comms->file_dialog, "Select INI File for Import", "INI files (*.ini)");
        }
        
        // Validation status
        if (strlen(comms->import_ini_path) > 0) {
            bool valid = ecu_dynamic_protocols_validate_ini_for_import(comms->import_ini_path);
            if (valid) {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✅ Valid INI file");
                
                // Show preview
                INIConfig* preview = ecu_load_ini_file(comms->import_ini_path);
                if (preview) {
                    ImGui::Text("ECU Name: %s", preview->ecu_name);
                    ImGui::Text("ECU Version: %s", preview->ecu_version);
                    ImGui::Text("Signature: %s", preview->signature);
                    
                    ProtocolDetectionResult detection = ecu_detect_protocol_from_ini(preview);
                    ImGui::Text("Detected Protocol: %s (%.1f%% confidence)", 
                               ecu_get_protocol_name_from_ini(detection.protocol_type),
                               detection.confidence * 100.0f);
                    
                    ecu_free_ini_config(preview);
                }
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "❌ Invalid INI file");
            }
        }
        
        ImGui::Separator();
        
        // Import button
        bool can_import = (strlen(comms->import_ini_path) > 0 && 
                          ecu_dynamic_protocols_validate_ini_for_import(comms->import_ini_path));
        
        if (can_import) {
            if (ImGui::Button("Import Protocol", ImVec2(150, 30))) {
                DynamicProtocolManager* manager = (DynamicProtocolManager*)comms->dynamic_protocol_manager;
                if (ecu_dynamic_protocols_add_from_ini(manager, comms->import_ini_path)) {
                    ImGui::OpenPopup("Import Success");
                    memset(comms->import_ini_path, 0, sizeof(comms->import_ini_path));
                } else {
                    ImGui::OpenPopup("Import Failed");
                }
            }
        } else {
            ImGui::Button("Import Protocol", ImVec2(150, 30));
            ImGui::SameLine();
            ImGui::Text("Select a valid INI file first");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            comms->show_import_dialog = false;
            memset(comms->import_ini_path, 0, sizeof(comms->import_ini_path));
        }
        
        // Success popup
        if (ImGui::BeginPopupModal("Import Success", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Protocol imported successfully!");
            ImGui::Text("The new ECU protocol is now available for connections.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                comms->show_import_dialog = false;
            }
            ImGui::EndPopup();
        }
        
        // Failure popup
        if (ImGui::BeginPopupModal("Import Failed", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to import protocol!");
            ImGui::Text("The INI file may be invalid or the protocol already exists.");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        ImGui::End();
    }
} 