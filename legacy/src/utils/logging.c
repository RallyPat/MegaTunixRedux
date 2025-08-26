#include "../include/utils/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <zlib.h>
#include <strings.h>

// Forward declarations for static functions
static bool logging_write_csv_record(LoggingState* state, const LogRecord* record);
static bool logging_write_json_record(LoggingState* state, const LogRecord* record);
static bool logging_write_binary_record(LoggingState* state, const LogRecord* record);

// Default configuration
LoggingConfig logging_get_default_config(void) {
    LoggingConfig config = {0};
    config.enabled = true;
    config.format = LOG_FORMAT_CSV;
    strcpy(config.log_directory, "logs");
    strcpy(config.filename_prefix, "ecu_data");
    config.max_file_size_mb = 10;
    config.max_files = 10;
    config.log_interval_ms = 100;
    config.include_timestamp = true;
    config.include_metadata = true;
    config.auto_rotate = true;
    config.compress_old_logs = true;
    return config;
}

// Initialize logging system
bool logging_init(LoggingState* state, const LoggingConfig* config) {
    if (!state || !config) {
        return false;
    }
    
    memset(state, 0, sizeof(LoggingState));
    state->config = *config;
    
    // Create log directory if it doesn't exist
    struct stat st = {0};
    if (stat(config->log_directory, &st) == -1) {
        if (mkdir(config->log_directory, 0755) != 0) {
            logging_set_error(state, "Failed to create log directory");
            return false;
        }
    }
    
    return true;
}

// Cleanup logging system
void logging_cleanup(LoggingState* state) {
    if (!state) return;
    
    if (state->is_active) {
        logging_stop_session(state);
    }
    
    if (state->current_file) {
        fclose(state->current_file);
        state->current_file = NULL;
    }
}

// Start a new logging session
bool logging_start_session(LoggingState* state) {
    if (!state || !state->config.enabled) {
        return false;
    }
    
    if (state->is_active) {
        logging_stop_session(state);
    }
    
    // Generate filename
    char* filename = logging_generate_filename(&state->config, time(NULL));
    if (!filename) {
        logging_set_error(state, "Failed to generate filename");
        return false;
    }
    
    // Open file
    state->current_file = fopen(filename, "w");
    if (!state->current_file) {
        logging_set_error(state, "Failed to open log file");
        free(filename);
        return false;
    }
    
    strcpy(state->current_filename, filename);
    free(filename);
    
    // Write header based on format
    switch (state->config.format) {
        case LOG_FORMAT_CSV:
            fprintf(state->current_file, "Timestamp,Protocol,ConnectionState");
            if (state->config.include_metadata) {
                fprintf(state->current_file, ",RPM,EngineTemp,ThrottlePosition,AFR");
            }
            fprintf(state->current_file, "\n");
            break;
            
        case LOG_FORMAT_JSON:
            fprintf(state->current_file, "{\n");
            fprintf(state->current_file, "  \"session_start\": %ld,\n", time(NULL));
            fprintf(state->current_file, "  \"format\": \"%s\",\n", logging_get_format_name(state->config.format));
            fprintf(state->current_file, "  \"records\": [\n");
            break;
            
        case LOG_FORMAT_BINARY:
            // Write binary header
            uint32_t magic = 0x4D54584C; // "MTXL"
            fwrite(&magic, sizeof(magic), 1, state->current_file);
            uint32_t version = 1;
            fwrite(&version, sizeof(version), 1, state->current_file);
            break;
    }
    
    state->session_start = time(NULL);
    state->is_active = true;
    state->total_records = 0;
    state->current_file_records = 0;
    state->current_file_size = 0;
    
    return true;
}

// Stop current logging session
bool logging_stop_session(LoggingState* state) {
    if (!state || !state->is_active) {
        return false;
    }
    
    if (state->current_file) {
        // Write footer based on format
        switch (state->config.format) {
            case LOG_FORMAT_JSON:
                fprintf(state->current_file, "\n  ],\n");
                fprintf(state->current_file, "  \"session_end\": %ld,\n", time(NULL));
                fprintf(state->current_file, "  \"total_records\": %lu\n", state->total_records);
                fprintf(state->current_file, "}\n");
                break;
                
            case LOG_FORMAT_BINARY:
                // Write binary footer
                uint32_t record_count = (uint32_t)state->total_records;
                fwrite(&record_count, sizeof(record_count), 1, state->current_file);
                break;
                
            default:
                break;
        }
        
        fclose(state->current_file);
        state->current_file = NULL;
    }
    
    state->is_active = false;
    return true;
}

// Write a single record
bool logging_write_record(LoggingState* state, const LogRecord* record) {
    if (!state || !state->is_active || !record) {
        return false;
    }
    
    if (!state->current_file) {
        logging_set_error(state, "No active log file");
        return false;
    }
    
    // Check if we need to rotate the file
    if (state->config.auto_rotate && 
        state->current_file_size > (state->config.max_file_size_mb * 1024 * 1024)) {
        if (!logging_rotate_file(state)) {
            return false;
        }
    }
    
    // Write record based on format
    bool success = false;
    switch (state->config.format) {
        case LOG_FORMAT_CSV:
            success = logging_write_csv_record(state, record);
            break;
            
        case LOG_FORMAT_JSON:
            success = logging_write_json_record(state, record);
            break;
            
        case LOG_FORMAT_BINARY:
            success = logging_write_binary_record(state, record);
            break;
    }
    
    if (success) {
        state->total_records++;
        state->current_file_records++;
        state->current_file_size = ftell(state->current_file);
    }
    
    return success;
}

// Write ECU data directly
bool logging_write_ecu_data(LoggingState* state, const ECUContext* ecu_ctx) {
    if (!state || !ecu_ctx) {
        return false;
    }
    
    LogRecord record = {0};
    record.timestamp = time(NULL);
    record.protocol = ecu_ctx->protocol;
    record.connection_state = ecu_ctx->state;
    record.ecu_data = ecu_ctx->data;
    
    return logging_write_record(state, &record);
}

// CSV record writing
static bool logging_write_csv_record(LoggingState* state, const LogRecord* record) {
    if (state->config.include_timestamp) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&record->timestamp));
        fprintf(state->current_file, "%s,", time_str);
    }
    
    fprintf(state->current_file, "%s,%s", 
            ecu_get_protocol_name(record->protocol),
            record->connection_state == ECU_STATE_CONNECTED ? "Connected" : "Disconnected");
    
    if (state->config.include_metadata) {
        fprintf(state->current_file, ",%.0f,%.1f,%.1f,%.2f",
                record->ecu_data.rpm,
                record->ecu_data.coolant_temp,
                record->ecu_data.tps,
                record->ecu_data.afr);
    }
    
    fprintf(state->current_file, "\n");
    return true;
}

// JSON record writing
static bool logging_write_json_record(LoggingState* state, const LogRecord* record) {
    if (state->current_file_records > 0) {
        fprintf(state->current_file, ",\n");
    }
    
    fprintf(state->current_file, "    {\n");
    fprintf(state->current_file, "      \"timestamp\": %ld,\n", record->timestamp);
    fprintf(state->current_file, "      \"protocol\": \"%s\",\n", ecu_get_protocol_name(record->protocol));
    fprintf(state->current_file, "      \"connection_state\": \"%s\"", 
            record->connection_state == ECU_STATE_CONNECTED ? "connected" : "disconnected");
    
    if (state->config.include_metadata) {
        fprintf(state->current_file, ",\n      \"data\": {\n");
        fprintf(state->current_file, "        \"rpm\": %.0f,\n", record->ecu_data.rpm);
        fprintf(state->current_file, "        \"engine_temp\": %.1f,\n", record->ecu_data.coolant_temp);
        fprintf(state->current_file, "        \"throttle_position\": %.1f,\n", record->ecu_data.tps);
        fprintf(state->current_file, "        \"afr\": %.2f\n", record->ecu_data.afr);
        fprintf(state->current_file, "      }\n");
    }
    
    fprintf(state->current_file, "    }");
    return true;
}

// Binary record writing
static bool logging_write_binary_record(LoggingState* state, const LogRecord* record) {
    // Write record header
    fwrite(&record->timestamp, sizeof(record->timestamp), 1, state->current_file);
    fwrite(&record->protocol, sizeof(record->protocol), 1, state->current_file);
    fwrite(&record->connection_state, sizeof(record->connection_state), 1, state->current_file);
    
    // Write ECU data
    fwrite(&record->ecu_data, sizeof(record->ecu_data), 1, state->current_file);
    
    return true;
}

// Rotate log file
bool logging_rotate_file(LoggingState* state) {
    if (!state || !state->current_file) {
        return false;
    }
    
    // Close current file
    fclose(state->current_file);
    state->current_file = NULL;
    
    // Compress old file if enabled
    if (state->config.compress_old_logs) {
        char compressed_name[512];
        snprintf(compressed_name, sizeof(compressed_name), "%s.gz", state->current_filename);
        
        FILE* input = fopen(state->current_filename, "rb");
        gzFile output = gzopen(compressed_name, "wb");
        
        if (input && output) {
            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
                gzwrite(output, buffer, bytes_read);
            }
        }
        
        if (input) fclose(input);
        if (output) gzclose(output);
        
        // Remove original file
        unlink(state->current_filename);
    }
    
    // Start new file
    return logging_start_session(state);
}

// Generate filename
char* logging_generate_filename(const LoggingConfig* config, time_t timestamp) {
    char* filename = malloc(512);
    if (!filename) return NULL;
    
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", localtime(&timestamp));
    
    const char* ext = "";
    switch (config->format) {
        case LOG_FORMAT_CSV: ext = ".csv"; break;
        case LOG_FORMAT_JSON: ext = ".json"; break;
        case LOG_FORMAT_BINARY: ext = ".bin"; break;
    }
    
    snprintf(filename, 512, "%s/%s_%s%s", 
             config->log_directory, config->filename_prefix, time_str, ext);
    
    return filename;
}

// Utility functions
const char* logging_get_format_name(LogFormat format) {
    switch (format) {
        case LOG_FORMAT_CSV: return "CSV";
        case LOG_FORMAT_JSON: return "JSON";
        case LOG_FORMAT_BINARY: return "BINARY";
        default: return "UNKNOWN";
    }
}

LogFormat logging_parse_format_name(const char* name) {
    if (!name) return LOG_FORMAT_CSV;
    
    if (strcasecmp(name, "csv") == 0) return LOG_FORMAT_CSV;
    if (strcasecmp(name, "json") == 0) return LOG_FORMAT_JSON;
    if (strcasecmp(name, "binary") == 0) return LOG_FORMAT_BINARY;
    
    return LOG_FORMAT_CSV;
}

void logging_set_error(LoggingState* state, const char* error) {
    if (state && error) {
        strncpy(state->error_message, error, sizeof(state->error_message) - 1);
        state->error_message[sizeof(state->error_message) - 1] = '\0';
    }
}

const char* logging_get_error(const LoggingState* state) {
    return state ? state->error_message : "Invalid logging state";
}

// Configuration validation
bool logging_validate_config(const LoggingConfig* config) {
    if (!config) return false;
    
    if (config->max_file_size_mb == 0) return false;
    if (config->max_files == 0) return false;
    if (config->log_interval_ms == 0) return false;
    
    return true;
}

bool logging_fix_config(LoggingConfig* config) {
    if (!config) return false;
    
    if (config->max_file_size_mb == 0) config->max_file_size_mb = 10;
    if (config->max_files == 0) config->max_files = 10;
    if (config->log_interval_ms == 0) config->log_interval_ms = 100;
    
    if (strlen(config->log_directory) == 0) {
        strcpy(config->log_directory, "logs");
    }
    
    if (strlen(config->filename_prefix) == 0) {
        strcpy(config->filename_prefix, "ecu_data");
    }
    
    return true;
}

// Stub implementations for advanced features
bool logging_export_csv(const char* input_file, const char* output_file) {
    (void)input_file; (void)output_file;
    return false; // TODO: Implement
}

bool logging_export_json(const char* input_file, const char* output_file) {
    (void)input_file; (void)output_file;
    return false; // TODO: Implement
}

bool logging_convert_format(const char* input_file, LogFormat input_format, 
                           const char* output_file, LogFormat output_format) {
    (void)input_file; (void)input_format; (void)output_file; (void)output_format;
    return false; // TODO: Implement
}

bool logging_playback_init(LogPlayback* playback, const char* filename) {
    (void)playback; (void)filename;
    return false; // TODO: Implement
}

void logging_playback_cleanup(LogPlayback* playback) {
    (void)playback; // TODO: Implement
}

bool logging_playback_next_record(LogPlayback* playback, LogRecord* record) {
    (void)playback; (void)record;
    return false; // TODO: Implement
}

bool logging_playback_seek_to_time(LogPlayback* playback, time_t timestamp) {
    (void)playback; (void)timestamp;
    return false; // TODO: Implement
}

uint64_t logging_playback_get_total_records(const char* filename) {
    (void)filename;
    return 0; // TODO: Implement
}

bool logging_is_valid_filename(const char* filename) {
    (void)filename;
    return true; // TODO: Implement
}

bool logging_compress_old_logs(LoggingState* state) {
    (void)state;
    return false; // TODO: Implement
} 