/*
 * Plugin Manager - MegaTunix Redux
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Plugin management system public interface
 */

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "plugin_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// Plugin system initialization
bool plugin_system_init(void);
void plugin_system_cleanup(void);

// Plugin manager access
PluginManager* get_plugin_manager(void);
EventSystem* get_event_system(void);

// Plugin information
int get_plugin_count(void);
PluginInterface* get_plugin_by_index(int index);

// Plugin system status
bool is_plugin_system_initialized(void);

// Additional plugin functions
bool is_plugin_system_initialized(void);
int get_plugin_count(void);
PluginInterface* get_plugin_by_index(int index);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_MANAGER_H
