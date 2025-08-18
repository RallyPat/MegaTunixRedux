/*
 * Datalog Manager - Foundation for professional data logging
 *
 * Provides a centralized interface for starting/stopping logging sessions
 * and writing samples in different formats. This is a stub implementation
 * intended as the foundation to reach feature parity with TunerStudio Ultra.
 */

#ifndef DATALOG_MANAGER_H
#define DATALOG_MANAGER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	DATALOG_FORMAT_CSV = 0,
	DATALOG_FORMAT_JSON = 1,
	DATALOG_FORMAT_BINARY = 2
} DatalogFormat;

typedef struct {
	char output_directory[256];
	char session_name[128];
	DatalogFormat format;
	int interval_ms;
	bool include_timestamps;
	int max_file_size_mb;
} DatalogSettings;

// Lifecycle
bool datalog_manager_init(void);
void datalog_manager_shutdown(void);

// Configuration
void datalog_manager_set_settings(const DatalogSettings* settings);
void datalog_manager_get_settings(DatalogSettings* out_settings);

// Session control
bool datalog_manager_start_session(const char* optional_session_name);
void datalog_manager_stop_session(void);
bool datalog_manager_is_active(void);

// Sample writing (generic key/value for foundation; specialized APIs can be added later)
bool datalog_manager_log_scalar(const char* key, double value);
bool datalog_manager_log_multiple(const char** keys, const double* values, size_t count);

#ifdef __cplusplus
}
#endif

#endif // DATALOG_MANAGER_H


