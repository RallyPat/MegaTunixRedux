/*
 * Plugin Manager - MegaTunix Redux
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Plugin management system implementation
 */

#include "../../include/plugin/plugin_interface.h"
#include "../../include/core/data_bridge.h"
#include "../../include/ui/logging_system.h"
#include <dlfcn.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <chrono>

// Plugin manager instance
static struct {
    PluginInterface* plugins;
    int count;
    int capacity;
    char plugin_directory[256];
    bool initialized;
} g_plugin_manager = {0};

// Event system instance
static struct {
    struct {
        const char* event_name;
        EventCallback callback;
        const char* plugin_name;
    } subscriptions[100];
    int subscription_count;
    
    struct {
        PluginEvent event;
        bool processed;
    } event_queue[1000];
    int event_count;
    int event_head;
    int event_tail;
} g_event_system = {0};

// Plugin manager functions
static bool load_plugin(const char* plugin_path);
static bool unload_plugin(const char* plugin_name);
static void scan_plugin_directory(const char* directory);
static PluginInterface* find_plugin(const char* name);
static bool init_all_plugins(void);
static void cleanup_all_plugins(void);
static void update_all_plugins(void);
static bool send_event(const char* plugin_name, const char* event, void* data);
static bool broadcast_event(const char* event, void* data);

// Event system functions
static bool subscribe(const char* event_name, EventCallback callback);
static bool unsubscribe(const char* event_name, EventCallback callback);
static bool publish(const char* event_name, void* data, size_t size);
static bool publish_to_plugin(const char* plugin_name, const char* event_name, void* data, size_t size);
static void process_events(void);
static void clear_events(void);

// Plugin validation
static bool validate_plugin(PluginInterface* plugin);
static PluginContext create_plugin_context(void);
static bool register_plugin(PluginInterface* plugin, void* handle);

// Plugin manager interface
static PluginManager g_plugin_manager_interface = {
    .plugins = nullptr,
    .count = 0,
    .capacity = 0,
    .load_plugin = load_plugin,
    .unload_plugin = unload_plugin,
    .scan_plugin_directory = scan_plugin_directory,
    .find_plugin = find_plugin,
    .init_all_plugins = init_all_plugins,
    .cleanup_all_plugins = cleanup_all_plugins,
    .update_all_plugins = update_all_plugins,
    .send_event = send_event,
    .broadcast_event = broadcast_event
};

// Event system interface
static EventSystem g_event_system_interface = {
    .subscribe = subscribe,
    .unsubscribe = unsubscribe,
    .publish = publish,
    .publish_to_plugin = publish_to_plugin,
    .process_events = process_events,
    .clear_events = clear_events
};

// Initialize plugin system
bool plugin_system_init(void) {
    if (g_plugin_manager.initialized) {
        add_log_entry(LOG_LEVEL_WARNING, "Plugin system already initialized");
        return true;
    }
    
    // Initialize plugin manager
    g_plugin_manager.capacity = 20;
    g_plugin_manager.plugins = (PluginInterface*)calloc(g_plugin_manager.capacity, sizeof(PluginInterface));
    if (!g_plugin_manager.plugins) {
        add_log_entry(LOG_LEVEL_ERROR, "Failed to allocate plugin manager memory");
        return false;
    }
    
    // Set default plugin directory
    strcpy(g_plugin_manager.plugin_directory, "plugins");
    
    // Initialize event system
    g_event_system.subscription_count = 0;
    g_event_system.event_count = 0;
    g_event_system.event_head = 0;
    g_event_system.event_tail = 0;
    
    g_plugin_manager.initialized = true;
    add_log_entry(LOG_LEVEL_INFO, "Plugin system initialized successfully");
    
    return true;
}

// Cleanup plugin system
void plugin_system_cleanup(void) {
    if (!g_plugin_manager.initialized) {
        return;
    }
    
    // Cleanup all plugins
    cleanup_all_plugins();
    
    // Free plugin manager memory
    if (g_plugin_manager.plugins) {
        free(g_plugin_manager.plugins);
        g_plugin_manager.plugins = nullptr;
    }
    
    g_plugin_manager.initialized = false;
    add_log_entry(LOG_LEVEL_INFO, "Plugin system cleaned up");
}

// Load a plugin from file
static bool load_plugin(const char* plugin_path) {
    if (!g_plugin_manager.initialized) {
        add_log_entry(LOG_LEVEL_ERROR, "Plugin system not initialized");
        return false;
    }
    
    add_log_entry(LOG_LEVEL_INFO, "Loading plugin: %s", plugin_path);
    
    // Load dynamic library
    void* handle = dlopen(plugin_path, RTLD_LAZY);
    if (!handle) {
        add_log_entry(LOG_LEVEL_ERROR, "Failed to load plugin %s: %s", plugin_path, dlerror());
        return false;
    }
    
    // Get plugin interface function
    PluginInterface* (*get_interface)(void) = (PluginInterface*(*)(void))dlsym(handle, "get_plugin_interface");
    if (!get_interface) {
        add_log_entry(LOG_LEVEL_ERROR, "Failed to find get_plugin_interface in %s: %s", plugin_path, dlerror());
        dlclose(handle);
        return false;
    }
    
    // Get plugin interface
    PluginInterface* plugin = get_interface();
    if (!plugin) {
        add_log_entry(LOG_LEVEL_ERROR, "Plugin %s returned null interface", plugin_path);
        dlclose(handle);
        return false;
    }
    
    // Validate plugin
    if (!validate_plugin(plugin)) {
        add_log_entry(LOG_LEVEL_ERROR, "Plugin %s validation failed", plugin_path);
        dlclose(handle);
        return false;
    }
    
    // Register plugin
    if (!register_plugin(plugin, handle)) {
        add_log_entry(LOG_LEVEL_ERROR, "Failed to register plugin %s", plugin_path);
        dlclose(handle);
        return false;
    }
    
    add_log_entry(LOG_LEVEL_INFO, "Plugin %s loaded successfully", plugin->name);
    return true;
}

// Unload a plugin
static bool unload_plugin(const char* plugin_name) {
    if (!g_plugin_manager.initialized) {
        return false;
    }
    
    for (int i = 0; i < g_plugin_manager.count; i++) {
        if (strcmp(g_plugin_manager.plugins[i].name, plugin_name) == 0) {
            PluginInterface* plugin = &g_plugin_manager.plugins[i];
            
            // Cleanup plugin
            if (plugin->cleanup) {
                plugin->cleanup();
            }
            
            // Close library
            if (plugin->library_handle) {
                dlclose(plugin->library_handle);
            }
            
            // Remove from list
            for (int j = i; j < g_plugin_manager.count - 1; j++) {
                g_plugin_manager.plugins[j] = g_plugin_manager.plugins[j + 1];
            }
            g_plugin_manager.count--;
            
            add_log_entry(LOG_LEVEL_INFO, "Plugin %s unloaded", plugin_name);
            return true;
        }
    }
    
    add_log_entry(LOG_LEVEL_WARNING, "Plugin %s not found for unloading", plugin_name);
    return false;
}

// Recursively scan plugin directory for available plugins
static void scan_plugin_directory_recursive(const char* directory) {
    add_log_entry(LOG_LEVEL_INFO, "Scanning directory: %s", directory);
    
    DIR* dir = opendir(directory);
    if (!dir) {
        add_log_entry(LOG_LEVEL_WARNING, "Failed to open directory: %s", directory);
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Skip . and .. directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);
        
        if (entry->d_type == DT_DIR) {
            // Recursively scan subdirectories
            scan_plugin_directory_recursive(full_path);
        } else if (entry->d_type == DT_REG) {
            // Check if it's a plugin file
            const char* name = entry->d_name;
            if (strstr(name, ".so") || strstr(name, ".dll") || strstr(name, ".dylib")) {
                add_log_entry(LOG_LEVEL_INFO, "Found plugin: %s", full_path);
                
                // Try to load the plugin
                if (load_plugin(full_path)) {
                    // Plugin count is tracked in g_plugin_manager.count
                }
            }
        }
    }
    
    closedir(dir);
}

// Scan plugin directory for available plugins
static void scan_plugin_directory(const char* directory) {
    add_log_entry(LOG_LEVEL_INFO, "Starting plugin directory scan: %s", directory);
    
    if (!g_plugin_manager.initialized) {
        add_log_entry(LOG_LEVEL_ERROR, "Plugin manager not initialized");
        return;
    }
    
    // Start recursive scan
    scan_plugin_directory_recursive(directory);
    
    add_log_entry(LOG_LEVEL_INFO, "Plugin directory scan complete. Found %d plugins", g_plugin_manager.count);
}

// Find plugin by name
static PluginInterface* find_plugin(const char* name) {
    if (!g_plugin_manager.initialized) {
        return nullptr;
    }
    
    for (int i = 0; i < g_plugin_manager.count; i++) {
        if (strcmp(g_plugin_manager.plugins[i].name, name) == 0) {
            return &g_plugin_manager.plugins[i];
        }
    }
    
    return nullptr;
}

// Initialize all loaded plugins
static bool init_all_plugins(void) {
    if (!g_plugin_manager.initialized) {
        return false;
    }
    
    add_log_entry(LOG_LEVEL_INFO, "Initializing %d plugins", g_plugin_manager.count);
    
    for (int i = 0; i < g_plugin_manager.count; i++) {
        PluginInterface* plugin = &g_plugin_manager.plugins[i];
        
        if (plugin->status == PLUGIN_STATUS_LOADED) {
            PluginContext ctx = create_plugin_context();
            
            if (plugin->init && plugin->init(&ctx)) {
                plugin->status = PLUGIN_STATUS_INITIALIZED;
                add_log_entry(LOG_LEVEL_INFO, "Plugin %s initialized successfully", plugin->name);
            } else {
                plugin->status = PLUGIN_STATUS_ERROR;
                add_log_entry(LOG_LEVEL_ERROR, "Failed to initialize plugin %s", plugin->name);
            }
        }
    }
    
    return true;
}

// Cleanup all plugins
static void cleanup_all_plugins(void) {
    if (!g_plugin_manager.initialized) {
        return;
    }
    
    add_log_entry(LOG_LEVEL_INFO, "Cleaning up %d plugins", g_plugin_manager.count);
    
    for (int i = 0; i < g_plugin_manager.count; i++) {
        PluginInterface* plugin = &g_plugin_manager.plugins[i];
        
        if (plugin->cleanup) {
            plugin->cleanup();
        }
        
        if (plugin->library_handle) {
            dlclose(plugin->library_handle);
        }
    }
    
    g_plugin_manager.count = 0;
}

// Update all plugins
static void update_all_plugins(void) {
    if (!g_plugin_manager.initialized) {
        return;
    }
    
    for (int i = 0; i < g_plugin_manager.count; i++) {
        PluginInterface* plugin = &g_plugin_manager.plugins[i];
        
        if (plugin->status == PLUGIN_STATUS_INITIALIZED && plugin->update) {
            plugin->update();
        }
    }
}

// Send event to specific plugin
static bool send_event(const char* plugin_name, const char* event, void* data) {
    if (!g_plugin_manager.initialized) {
        return false;
    }
    
    PluginInterface* plugin = find_plugin(plugin_name);
    if (!plugin) {
        return false;
    }
    
    // Add to event queue
    if (g_event_system.event_count < 1000) {
        int idx = g_event_system.event_tail;
        g_event_system.event_queue[idx].event.event_name = event;
        g_event_system.event_queue[idx].event.source_plugin = "system";
        g_event_system.event_queue[idx].event.data = data;
        g_event_system.event_queue[idx].event.data_size = 0;
        g_event_system.event_queue[idx].event.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        g_event_system.event_queue[idx].processed = false;
        
        g_event_system.event_tail = (g_event_system.event_tail + 1) % 1000;
        g_event_system.event_count++;
        
        return true;
    }
    
    return false;
}

// Broadcast event to all plugins
static bool broadcast_event(const char* event, void* data) {
    if (!g_plugin_manager.initialized) {
        return false;
    }
    
    // Add to event queue for all plugins
    for (int i = 0; i < g_plugin_manager.count; i++) {
        send_event(g_plugin_manager.plugins[i].name, event, data);
    }
    
    return true;
}

// Subscribe to events
static bool subscribe(const char* event_name, EventCallback callback) {
    if (g_event_system.subscription_count >= 100) {
        return false;
    }
    
    g_event_system.subscriptions[g_event_system.subscription_count].event_name = event_name;
    g_event_system.subscriptions[g_event_system.subscription_count].callback = callback;
    g_event_system.subscriptions[g_event_system.subscription_count].plugin_name = "system";
    g_event_system.subscription_count++;
    
    return true;
}

// Unsubscribe from events
static bool unsubscribe(const char* event_name, EventCallback callback) {
    for (int i = 0; i < g_event_system.subscription_count; i++) {
        if (g_event_system.subscriptions[i].event_name == event_name && 
            g_event_system.subscriptions[i].callback == callback) {
            
            // Remove subscription
            for (int j = i; j < g_event_system.subscription_count - 1; j++) {
                g_event_system.subscriptions[j] = g_event_system.subscriptions[j + 1];
            }
            g_event_system.subscription_count--;
            return true;
        }
    }
    
    return false;
}

// Publish event
static bool publish(const char* event_name, void* data, size_t size) {
    return broadcast_event(event_name, data);
}

// Publish event to specific plugin
static bool publish_to_plugin(const char* plugin_name, const char* event_name, void* data, size_t size) {
    return send_event(plugin_name, event_name, data);
}

// Process events
static void process_events(void) {
    if (!g_plugin_manager.initialized) {
        return;
    }
    
    while (g_event_system.event_count > 0) {
        int idx = g_event_system.event_head;
        PluginEvent* event = &g_event_system.event_queue[idx].event;
        
        if (!g_event_system.event_queue[idx].processed) {
            // Process event
            for (int i = 0; i < g_event_system.subscription_count; i++) {
                if (strcmp(g_event_system.subscriptions[i].event_name, event->event_name) == 0) {
                    g_event_system.subscriptions[i].callback(event);
                }
            }
            
            g_event_system.event_queue[idx].processed = true;
        }
        
        g_event_system.event_head = (g_event_system.event_head + 1) % 1000;
        g_event_system.event_count--;
    }
}

// Clear events
static void clear_events(void) {
    g_event_system.event_count = 0;
    g_event_system.event_head = 0;
    g_event_system.event_tail = 0;
}

// Validate plugin interface
static bool validate_plugin(PluginInterface* plugin) {
    if (!plugin) return false;
    if (!plugin->name) return false;
    if (!plugin->version) return false;
    if (!plugin->author) return false;
    if (!plugin->description) return false;
    if (plugin->type >= PLUGIN_TYPE_COUNT) return false;
    
    // Validate type-specific interface
    switch (plugin->type) {
        case PLUGIN_TYPE_ECU:
            if (!plugin->interface.ecu.connect) return false;
            if (!plugin->interface.ecu.disconnect) return false;
            if (!plugin->interface.ecu.is_connected) return false;
            if (!plugin->interface.ecu.get_connection_status) return false;
            break;
            
        case PLUGIN_TYPE_UI:
            if (!plugin->interface.ui.panel_name) return false;
            if (!plugin->interface.ui.render_panel) return false;
            break;
            
        case PLUGIN_TYPE_DATA:
            if (!plugin->interface.data.processor_name) return false;
            if (!plugin->interface.data.process_data) return false;
            break;
            
        case PLUGIN_TYPE_INTEGRATION:
            if (!plugin->interface.integration.service_name) return false;
            if (!plugin->interface.integration.connect_service) return false;
            break;
    }
    
    return true;
}

// Create plugin context
static PluginContext create_plugin_context(void) {
    PluginContext ctx = {0};
    // Note: In a real implementation, these would be populated with actual module instances
    // For now, we'll leave them as null pointers
    return ctx;
}

// Register plugin
static bool register_plugin(PluginInterface* plugin, void* handle) {
    if (g_plugin_manager.count >= g_plugin_manager.capacity) {
        // Expand capacity
        int new_capacity = g_plugin_manager.capacity * 2;
        PluginInterface* new_plugins = (PluginInterface*)realloc(g_plugin_manager.plugins, 
                                                               new_capacity * sizeof(PluginInterface));
        if (!new_plugins) {
            return false;
        }
        g_plugin_manager.plugins = new_plugins;
        g_plugin_manager.capacity = new_capacity;
    }
    
    // Copy plugin interface
    g_plugin_manager.plugins[g_plugin_manager.count] = *plugin;
    g_plugin_manager.plugins[g_plugin_manager.count].library_handle = handle;
    g_plugin_manager.plugins[g_plugin_manager.count].status = PLUGIN_STATUS_LOADED;
    
    // Register plugin with data bridge based on type
    PluginInterface* registered_plugin = &g_plugin_manager.plugins[g_plugin_manager.count];
    if (plugin->type == PLUGIN_TYPE_ECU) {
        data_bridge_register_ecu_plugin(registered_plugin);
        add_log_entry(LOG_LEVEL_INFO, "Registered ECU plugin '%s' with data bridge", plugin->name);
    } else if (plugin->type == PLUGIN_TYPE_DATA) {
        data_bridge_register_visualization_plugin(registered_plugin);
        add_log_entry(LOG_LEVEL_INFO, "Registered visualization plugin '%s' with data bridge", plugin->name);
    }
    
    g_plugin_manager.count++;
    return true;
}

// Get plugin manager interface
extern "C" PluginManager* get_plugin_manager(void) {
    return &g_plugin_manager_interface;
}

// Get event system interface
extern "C" EventSystem* get_event_system(void) {
    return &g_event_system_interface;
}

// Get plugin count
extern "C" int get_plugin_count(void) {
    return g_plugin_manager.count;
}

// Get plugin by index
extern "C" PluginInterface* get_plugin_by_index(int index) {
    if (index >= 0 && index < g_plugin_manager.count) {
        return &g_plugin_manager.plugins[index];
    }
    return nullptr;
}

// Check if plugin system is initialized
extern "C" bool is_plugin_system_initialized(void) {
    return g_plugin_manager.initialized;
}
