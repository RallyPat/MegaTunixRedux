/*
 * Datalog Manager - Stub implementation
 *
 * This establishes the foundation for professional data logging:
 * - Central configuration & lifecycle
 * - Session start/stop
 * - Append-only logging API (CSV/JSON/Binary to be implemented)
 */

#include "../../include/data/datalog_manager.h"
#include "../../include/utils/config.h"
#include "../../include/megatunix_redux.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
	DatalogSettings settings;
	FILE* file;
	bool active;
	char current_file_path[512];
} DatalogState;

static DatalogState g_datalog = {0};

static void build_default_settings(DatalogSettings* s) {
	*s = (DatalogSettings){0};
	strcpy(s->output_directory, config_get_log_dir());
	strcpy(s->session_name, "session");
	s->format = DATALOG_FORMAT_CSV;
	s->interval_ms = 100;
	s->include_timestamps = true;
	s->max_file_size_mb = 256;
}

bool datalog_manager_init(void) {
	if (g_datalog.active) return true;
	build_default_settings(&g_datalog.settings);
	g_datalog.file = NULL;
	g_datalog.active = false;
	g_datalog.current_file_path[0] = '\0';
	return true;
}

void datalog_manager_shutdown(void) {
	if (g_datalog.active) {
		datalog_manager_stop_session();
	}
}

void datalog_manager_set_settings(const DatalogSettings* settings) {
	if (!settings) return;
	g_datalog.settings = *settings;
}

void datalog_manager_get_settings(DatalogSettings* out_settings) {
	if (!out_settings) return;
	*out_settings = g_datalog.settings;
}

static void build_timestamped_filename(char* out, size_t out_size, const char* dir, const char* base, const char* ext) {
	char ts[32];
	time_t now = time(NULL);
	struct tm tm_now;
	localtime_r(&now, &tm_now);
	strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_now);
	snprintf(out, out_size, "%s/%s_%s.%s", dir, base, ts, ext);
}

bool datalog_manager_start_session(const char* optional_session_name) {
	if (g_datalog.active) return true;
	const char* name = optional_session_name && optional_session_name[0] ? optional_session_name : g_datalog.settings.session_name;
	const char* ext = (g_datalog.settings.format == DATALOG_FORMAT_JSON) ? "json" : (g_datalog.settings.format == DATALOG_FORMAT_BINARY ? "bin" : "csv");
	build_timestamped_filename(g_datalog.current_file_path, sizeof(g_datalog.current_file_path), g_datalog.settings.output_directory, name, ext);

	g_datalog.file = fopen(g_datalog.current_file_path, "w");
	if (!g_datalog.file) return false;

	// Write CSV header as an initial foundation
	if (g_datalog.settings.format == DATALOG_FORMAT_CSV) {
		if (g_datalog.settings.include_timestamps) {
			fprintf(g_datalog.file, "timestamp_ms");
		}
		// Columns will be added on first batch write; keep minimal now
		fprintf(g_datalog.file, "\n");
	}

	g_datalog.active = true;
	return true;
}

void datalog_manager_stop_session(void) {
	if (!g_datalog.active) return;
	if (g_datalog.file) {
		fclose(g_datalog.file);
		g_datalog.file = NULL;
	}
	g_datalog.active = false;
}

bool datalog_manager_is_active(void) {
	return g_datalog.active;
}

static long current_time_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (long)(ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
}

bool datalog_manager_log_scalar(const char* key, double value) {
	if (!g_datalog.active || !g_datalog.file || !key) return false;
	if (g_datalog.settings.format == DATALOG_FORMAT_CSV) {
		if (g_datalog.settings.include_timestamps) {
			fprintf(g_datalog.file, "%ld,", current_time_ms());
		}
		fprintf(g_datalog.file, "%s=%.6f\n", key, value);
		fflush(g_datalog.file);
		return true;
	}
	// JSON/Binary formats to be implemented
	return false;
}

bool datalog_manager_log_multiple(const char** keys, const double* values, size_t count) {
	if (!g_datalog.active || !g_datalog.file || !keys || !values || count == 0) return false;
	if (g_datalog.settings.format == DATALOG_FORMAT_CSV) {
		if (g_datalog.settings.include_timestamps) {
			fprintf(g_datalog.file, "%ld", current_time_ms());
		}
		for (size_t i = 0; i < count; ++i) {
			fprintf(g_datalog.file, "%s%s=%.6f", g_datalog.settings.include_timestamps || i > 0 ? "," : "", keys[i], values[i]);
		}
		fprintf(g_datalog.file, "\n");
		fflush(g_datalog.file);
		return true;
	}
	return false;
}


