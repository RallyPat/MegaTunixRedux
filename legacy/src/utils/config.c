/*
 * Configuration System Implementation
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Cross-platform configuration management with JSON support.
 */

#include "../../include/utils/config.h"
#include "../../include/megatunix_redux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <shlobj.h>
#else
    #include <pwd.h>
    #include <sys/types.h>
#endif

// Global configuration state
static AppConfig g_config = {0};
static bool g_config_initialized = false;
static char g_config_dir[512] = {0};
static char g_config_file[512] = {0};
static char g_log_dir[512] = {0};
static char g_cache_dir[512] = {0};

// Platform-specific path detection
static void detect_platform_paths(void) {
#ifdef PLATFORM_WINDOWS
    // Windows: %APPDATA%\MegaTunixRedux
    char appdata_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata_path))) {
        snprintf(g_config_dir, sizeof(g_config_dir), "%s\\MegaTunixRedux", appdata_path);
        snprintf(g_config_file, sizeof(g_config_file), "%s\\config.json", g_config_dir);
        snprintf(g_log_dir, sizeof(g_log_dir), "%s\\logs", g_config_dir);
        snprintf(g_cache_dir, sizeof(g_cache_dir), "%s\\cache", g_config_dir);
    }
#elif defined(PLATFORM_MACOS)
    // macOS: ~/Library/Application Support/MegaTunixRedux/
    const char* home = getenv("HOME");
    if (home) {
        snprintf(g_config_dir, sizeof(g_config_dir), "%s/Library/Application Support/MegaTunixRedux", home);
        snprintf(g_config_file, sizeof(g_config_file), "%s/config.json", g_config_dir);
        snprintf(g_log_dir, sizeof(g_log_dir), "%s/logs", g_config_dir);
        snprintf(g_cache_dir, sizeof(g_cache_dir), "%s/cache", g_config_dir);
    }
#else
    // Linux: ~/.config/megatunix-redux/
    const char* home = getenv("HOME");
    if (home) {
        snprintf(g_config_dir, sizeof(g_config_dir), "%s/.config/megatunix-redux", home);
        snprintf(g_config_file, sizeof(g_config_file), "%s/config.json", g_config_dir);
        snprintf(g_log_dir, sizeof(g_log_dir), "%s/logs", g_config_dir);
        snprintf(g_cache_dir, sizeof(g_cache_dir), "%s/cache", g_config_dir);
    }
#endif
}

// Create directories if they don't exist
static bool ensure_directories_exist(void) {
    // Create config directory
    if (mkdir(g_config_dir, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    // Create log directory
    if (mkdir(g_log_dir, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    // Create cache directory
    if (mkdir(g_cache_dir, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    
    return true;
}

// Get default configuration
AppConfig config_get_defaults(void) {
    AppConfig config = {0};
    
    // ECU Settings
    strcpy(config.default_protocol, "Speeduino");
    strcpy(config.default_port, "/dev/ttyUSB0");
    config.default_baud_rate = 115200;
    config.connection_timeout_ms = 1000;
    config.auto_connect = false;
    config.auto_reconnect = true;
    config.reconnect_interval_ms = 5000;
    
    // Dashboard Settings
    strcpy(config.default_layout, "default");
    config.show_alerts = true;
    config.color_coded_gauges = true;
    config.gauge_update_rate_ms = 100;
    config.rpm_warning_threshold = 6000.0f;
    config.rpm_danger_threshold = 7000.0f;
    config.boost_warning_threshold = 15.0f;
    config.boost_danger_threshold = 20.0f;
    config.temp_warning_threshold = 100.0f;
    config.temp_danger_threshold = 110.0f;
    
    // UI Settings
    config.window_width = 1024;
    config.window_height = 768;
    config.fullscreen = false;
    strcpy(config.theme, "dark");
    config.ui_scale = 1.0f;
    config.show_fps = false;
    
    // Logging Settings
    config.enable_logging = false;
    strcpy(config.log_directory, "");
    strcpy(config.log_format, "csv");
    config.log_interval_ms = 1000;
    config.log_timestamps = true;
    config.max_log_size_mb = 100;
    
    // Application Settings
    config.check_for_updates = true;
    config.start_minimized = false;
    strcpy(config.language, "en");
    config.debug_mode = false;
    
    return config;
}

// Simple JSON-like configuration file format
static bool write_config_file(const AppConfig* config) {
    FILE* file = fopen(g_config_file, "w");
    if (!file) {
        return false;
    }
    
    fprintf(file, "{\n");
    
    // ECU Settings
    fprintf(file, "  \"ecu\": {\n");
    fprintf(file, "    \"default_protocol\": \"%s\",\n", config->default_protocol);
    fprintf(file, "    \"default_port\": \"%s\",\n", config->default_port);
    fprintf(file, "    \"default_baud_rate\": %d,\n", config->default_baud_rate);
    fprintf(file, "    \"connection_timeout_ms\": %d,\n", config->connection_timeout_ms);
    fprintf(file, "    \"auto_connect\": %s,\n", config->auto_connect ? "true" : "false");
    fprintf(file, "    \"auto_reconnect\": %s,\n", config->auto_reconnect ? "true" : "false");
    fprintf(file, "    \"reconnect_interval_ms\": %d\n", config->reconnect_interval_ms);
    fprintf(file, "  },\n");
    
    // Dashboard Settings
    fprintf(file, "  \"dashboard\": {\n");
    fprintf(file, "    \"default_layout\": \"%s\",\n", config->default_layout);
    fprintf(file, "    \"show_alerts\": %s,\n", config->show_alerts ? "true" : "false");
    fprintf(file, "    \"color_coded_gauges\": %s,\n", config->color_coded_gauges ? "true" : "false");
    fprintf(file, "    \"gauge_update_rate_ms\": %d,\n", config->gauge_update_rate_ms);
    fprintf(file, "    \"rpm_warning_threshold\": %.1f,\n", config->rpm_warning_threshold);
    fprintf(file, "    \"rpm_danger_threshold\": %.1f,\n", config->rpm_danger_threshold);
    fprintf(file, "    \"boost_warning_threshold\": %.1f,\n", config->boost_warning_threshold);
    fprintf(file, "    \"boost_danger_threshold\": %.1f,\n", config->boost_danger_threshold);
    fprintf(file, "    \"temp_warning_threshold\": %.1f,\n", config->temp_warning_threshold);
    fprintf(file, "    \"temp_danger_threshold\": %.1f\n", config->temp_danger_threshold);
    fprintf(file, "  },\n");
    
    // UI Settings
    fprintf(file, "  \"ui\": {\n");
    fprintf(file, "    \"window_width\": %d,\n", config->window_width);
    fprintf(file, "    \"window_height\": %d,\n", config->window_height);
    fprintf(file, "    \"fullscreen\": %s,\n", config->fullscreen ? "true" : "false");
    fprintf(file, "    \"theme\": \"%s\",\n", config->theme);
    fprintf(file, "    \"ui_scale\": %.2f,\n", config->ui_scale);
    fprintf(file, "    \"show_fps\": %s\n", config->show_fps ? "true" : "false");
    fprintf(file, "  },\n");
    
    // Logging Settings
    fprintf(file, "  \"logging\": {\n");
    fprintf(file, "    \"enable_logging\": %s,\n", config->enable_logging ? "true" : "false");
    fprintf(file, "    \"log_directory\": \"%s\",\n", config->log_directory);
    fprintf(file, "    \"log_format\": \"%s\",\n", config->log_format);
    fprintf(file, "    \"log_interval_ms\": %d,\n", config->log_interval_ms);
    fprintf(file, "    \"log_timestamps\": %s,\n", config->log_timestamps ? "true" : "false");
    fprintf(file, "    \"max_log_size_mb\": %d\n", config->max_log_size_mb);
    fprintf(file, "  },\n");
    
    // Application Settings
    fprintf(file, "  \"app\": {\n");
    fprintf(file, "    \"check_for_updates\": %s,\n", config->check_for_updates ? "true" : "false");
    fprintf(file, "    \"start_minimized\": %s,\n", config->start_minimized ? "true" : "false");
    fprintf(file, "    \"language\": \"%s\",\n", config->language);
    fprintf(file, "    \"debug_mode\": %s\n", config->debug_mode ? "true" : "false");
    fprintf(file, "  }\n");
    
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

// Simple JSON-like configuration file parsing
static bool read_config_file(AppConfig* config) {
    FILE* file = fopen(g_config_file, "r");
    if (!file) {
        return false;
    }
    
    char line[512];
    char section[64] = {0};
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines and comments
        if (strlen(line) == 0 || line[0] == '#') {
            continue;
        }
        
        // Parse sections
        if (strstr(line, "\"ecu\"")) {
            strcpy(section, "ecu");
            continue;
        } else if (strstr(line, "\"dashboard\"")) {
            strcpy(section, "dashboard");
            continue;
        } else if (strstr(line, "\"ui\"")) {
            strcpy(section, "ui");
            continue;
        } else if (strstr(line, "\"logging\"")) {
            strcpy(section, "logging");
            continue;
        } else if (strstr(line, "\"app\"")) {
            strcpy(section, "app");
            continue;
        }
        
        // Parse key-value pairs
        char* colon = strchr(line, ':');
        if (!colon) continue;
        
        *colon = 0;
        char* key = line;
        char* value = colon + 1;
        
        // Remove quotes and whitespace
        while (*key == ' ' || *key == '"') key++;
        while (*value == ' ' || *value == '"') value++;
        
        char* end = strchr(key, '"');
        if (end) *end = 0;
        
        end = strchr(value, '"');
        if (end) *end = 0;
        
        // Remove trailing comma
        end = strchr(value, ',');
        if (end) *end = 0;
        
        // Parse based on section and key
        if (strcmp(section, "ecu") == 0) {
            if (strcmp(key, "default_protocol") == 0) {
                strncpy(config->default_protocol, value, sizeof(config->default_protocol) - 1);
            } else if (strcmp(key, "default_port") == 0) {
                strncpy(config->default_port, value, sizeof(config->default_port) - 1);
            } else if (strcmp(key, "default_baud_rate") == 0) {
                config->default_baud_rate = atoi(value);
            } else if (strcmp(key, "connection_timeout_ms") == 0) {
                config->connection_timeout_ms = atoi(value);
            } else if (strcmp(key, "auto_connect") == 0) {
                config->auto_connect = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "auto_reconnect") == 0) {
                config->auto_reconnect = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "reconnect_interval_ms") == 0) {
                config->reconnect_interval_ms = atoi(value);
            }
        } else if (strcmp(section, "dashboard") == 0) {
            if (strcmp(key, "default_layout") == 0) {
                strncpy(config->default_layout, value, sizeof(config->default_layout) - 1);
            } else if (strcmp(key, "show_alerts") == 0) {
                config->show_alerts = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "color_coded_gauges") == 0) {
                config->color_coded_gauges = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "gauge_update_rate_ms") == 0) {
                config->gauge_update_rate_ms = atoi(value);
            } else if (strcmp(key, "rpm_warning_threshold") == 0) {
                config->rpm_warning_threshold = atof(value);
            } else if (strcmp(key, "rpm_danger_threshold") == 0) {
                config->rpm_danger_threshold = atof(value);
            } else if (strcmp(key, "boost_warning_threshold") == 0) {
                config->boost_warning_threshold = atof(value);
            } else if (strcmp(key, "boost_danger_threshold") == 0) {
                config->boost_danger_threshold = atof(value);
            } else if (strcmp(key, "temp_warning_threshold") == 0) {
                config->temp_warning_threshold = atof(value);
            } else if (strcmp(key, "temp_danger_threshold") == 0) {
                config->temp_danger_threshold = atof(value);
            }
        } else if (strcmp(section, "ui") == 0) {
            if (strcmp(key, "window_width") == 0) {
                config->window_width = atoi(value);
            } else if (strcmp(key, "window_height") == 0) {
                config->window_height = atoi(value);
            } else if (strcmp(key, "fullscreen") == 0) {
                config->fullscreen = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "theme") == 0) {
                strncpy(config->theme, value, sizeof(config->theme) - 1);
            } else if (strcmp(key, "ui_scale") == 0) {
                config->ui_scale = atof(value);
            } else if (strcmp(key, "show_fps") == 0) {
                config->show_fps = (strcmp(value, "true") == 0);
            }
        } else if (strcmp(section, "logging") == 0) {
            if (strcmp(key, "enable_logging") == 0) {
                config->enable_logging = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "log_directory") == 0) {
                strncpy(config->log_directory, value, sizeof(config->log_directory) - 1);
            } else if (strcmp(key, "log_format") == 0) {
                strncpy(config->log_format, value, sizeof(config->log_format) - 1);
            } else if (strcmp(key, "log_interval_ms") == 0) {
                config->log_interval_ms = atoi(value);
            } else if (strcmp(key, "log_timestamps") == 0) {
                config->log_timestamps = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "max_log_size_mb") == 0) {
                config->max_log_size_mb = atoi(value);
            }
        } else if (strcmp(section, "app") == 0) {
            if (strcmp(key, "check_for_updates") == 0) {
                config->check_for_updates = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "start_minimized") == 0) {
                config->start_minimized = (strcmp(value, "true") == 0);
            } else if (strcmp(key, "language") == 0) {
                strncpy(config->language, value, sizeof(config->language) - 1);
            } else if (strcmp(key, "debug_mode") == 0) {
                config->debug_mode = (strcmp(value, "true") == 0);
            }
        }
    }
    
    fclose(file);
    return true;
}

// Configuration management functions
bool config_init(void) {
    if (g_config_initialized) {
        return true;
    }
    
    // Detect platform-specific paths
    detect_platform_paths();
    
    // Ensure directories exist
    if (!ensure_directories_exist()) {
        return false;
    }
    
    // Load configuration or create default
    if (!config_load(&g_config)) {
        g_config = config_get_defaults();
        config_save(&g_config);
    }
    
    g_config_initialized = true;
    return true;
}

void config_cleanup(void) {
    if (g_config_initialized) {
        config_save(&g_config);
        g_config_initialized = false;
    }
}

bool config_load(AppConfig* config) {
    if (!config) {
        return false;
    }
    
    // Start with defaults
    *config = config_get_defaults();
    
    // Try to load from file
    if (!read_config_file(config)) {
        return false;
    }
    
    // Validate and fix any issues
    config_fix_defaults(config);
    
    return true;
}

bool config_save(const AppConfig* config) {
    if (!config) {
        return false;
    }
    
    return write_config_file(config);
}

// Configuration validation
bool config_validate(const AppConfig* config) {
    if (!config) {
        return false;
    }
    
    // Basic validation
    if (config->window_width < 800 || config->window_width > 3840) return false;
    if (config->window_height < 600 || config->window_height > 2160) return false;
    if (config->ui_scale < 0.5f || config->ui_scale > 3.0f) return false;
    if (config->gauge_update_rate_ms < 10 || config->gauge_update_rate_ms > 1000) return false;
    if (config->log_interval_ms < 100 || config->log_interval_ms > 10000) return false;
    if (config->max_log_size_mb < 1 || config->max_log_size_mb > 1000) return false;
    
    return true;
}

void config_fix_defaults(AppConfig* config) {
    if (!config) {
        return;
    }
    
    AppConfig defaults = config_get_defaults();
    
    // Fix invalid values
    if (config->window_width < 800 || config->window_width > 3840) {
        config->window_width = defaults.window_width;
    }
    if (config->window_height < 600 || config->window_height > 2160) {
        config->window_height = defaults.window_height;
    }
    if (config->ui_scale < 0.5f || config->ui_scale > 3.0f) {
        config->ui_scale = defaults.ui_scale;
    }
    if (config->gauge_update_rate_ms < 10 || config->gauge_update_rate_ms > 1000) {
        config->gauge_update_rate_ms = defaults.gauge_update_rate_ms;
    }
    if (config->log_interval_ms < 100 || config->log_interval_ms > 10000) {
        config->log_interval_ms = defaults.log_interval_ms;
    }
    if (config->max_log_size_mb < 1 || config->max_log_size_mb > 1000) {
        config->max_log_size_mb = defaults.max_log_size_mb;
    }
    
    // Ensure strings are not empty
    if (strlen(config->default_protocol) == 0) {
        strcpy(config->default_protocol, defaults.default_protocol);
    }
    if (strlen(config->default_layout) == 0) {
        strcpy(config->default_layout, defaults.default_layout);
    }
    if (strlen(config->theme) == 0) {
        strcpy(config->theme, defaults.theme);
    }
    if (strlen(config->log_format) == 0) {
        strcpy(config->log_format, defaults.log_format);
    }
    if (strlen(config->language) == 0) {
        strcpy(config->language, defaults.language);
    }
}

// Platform-specific path functions
const char* config_get_config_dir(void) {
    return g_config_dir;
}

const char* config_get_config_file(void) {
    return g_config_file;
}

const char* config_get_log_dir(void) {
    return g_log_dir;
}

const char* config_get_cache_dir(void) {
    return g_cache_dir;
}

// Configuration helpers (simplified implementation)
bool config_set_string(const char* key, const char* value) {
    // TODO: Implement key-value storage
    (void)key;
    (void)value;
    return true;
}

bool config_get_string(const char* key, char* value, size_t max_len) {
    // TODO: Implement key-value retrieval
    (void)key;
    (void)value;
    (void)max_len;
    return false;
}

bool config_set_int(const char* key, int value) {
    // TODO: Implement key-value storage
    (void)key;
    (void)value;
    return true;
}

bool config_get_int(const char* key, int* value) {
    // TODO: Implement key-value retrieval
    (void)key;
    (void)value;
    return false;
}

bool config_set_bool(const char* key, bool value) {
    // TODO: Implement key-value storage
    (void)key;
    (void)value;
    return true;
}

bool config_get_bool(const char* key, bool* value) {
    // TODO: Implement key-value retrieval
    (void)key;
    (void)value;
    return false;
}

bool config_set_float(const char* key, float value) {
    // TODO: Implement key-value storage
    (void)key;
    (void)value;
    return true;
}

bool config_get_float(const char* key, float* value) {
    // TODO: Implement key-value retrieval
    (void)key;
    (void)value;
    return false;
}

// Configuration migration
bool config_migrate_old_config(void) {
    // TODO: Implement migration from old config formats
    return true;
}

void config_backup_current(void) {
    // TODO: Implement configuration backup
} 