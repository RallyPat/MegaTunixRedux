#ifndef MEGATUNIX_REDUX_LOGGING_H
#define MEGATUNIX_REDUX_LOGGING_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "../ecu/ecu_communication.h"

// Logging formats
typedef enum {
    LOG_FORMAT_CSV,
    LOG_FORMAT_JSON,
    LOG_FORMAT_BINARY
} LogFormat;

// Logging configuration
typedef struct {
    bool enabled;
    LogFormat format;
    char log_directory[256];
    char filename_prefix[64];
    uint32_t max_file_size_mb;
    uint32_t max_files;
    uint32_t log_interval_ms;
    bool include_timestamp;
    bool include_metadata;
    bool auto_rotate;
    bool compress_old_logs;
} LoggingConfig;

// Logging state
typedef struct {
    LoggingConfig config;
    FILE* current_file;
    char current_filename[512];
    time_t session_start;
    uint64_t total_records;
    uint64_t current_file_records;
    uint32_t current_file_size;
    bool is_active;
    char error_message[256];
} LoggingState;

// Data record structure
typedef struct {
    time_t timestamp;
    ECUProtocol protocol;
    ECUConnectionState connection_state;
    ECUData ecu_data;
} LogRecord;

// Function declarations
bool logging_init(LoggingState* state, const LoggingConfig* config);
void logging_cleanup(LoggingState* state);
bool logging_start_session(LoggingState* state);
bool logging_stop_session(LoggingState* state);
bool logging_write_record(LoggingState* state, const LogRecord* record);
bool logging_write_ecu_data(LoggingState* state, const ECUContext* ecu_ctx);

// Configuration functions
LoggingConfig logging_get_default_config(void);
bool logging_validate_config(const LoggingConfig* config);
bool logging_fix_config(LoggingConfig* config);

// File management
bool logging_rotate_file(LoggingState* state);
bool logging_compress_old_logs(LoggingState* state);
char* logging_generate_filename(const LoggingConfig* config, time_t timestamp);

// Data export functions
bool logging_export_csv(const char* input_file, const char* output_file);
bool logging_export_json(const char* input_file, const char* output_file);
bool logging_convert_format(const char* input_file, LogFormat input_format, 
                           const char* output_file, LogFormat output_format);

// Playback functions
typedef struct {
    FILE* file;
    LogFormat format;
    LogRecord current_record;
    bool has_more_data;
} LogPlayback;

bool logging_playback_init(LogPlayback* playback, const char* filename);
void logging_playback_cleanup(LogPlayback* playback);
bool logging_playback_next_record(LogPlayback* playback, LogRecord* record);
bool logging_playback_seek_to_time(LogPlayback* playback, time_t timestamp);
uint64_t logging_playback_get_total_records(const char* filename);

// Utility functions
const char* logging_get_format_name(LogFormat format);
LogFormat logging_parse_format_name(const char* name);
bool logging_is_valid_filename(const char* filename);
void logging_set_error(LoggingState* state, const char* error);
const char* logging_get_error(const LoggingState* state);

#endif // MEGATUNIX_REDUX_LOGGING_H 