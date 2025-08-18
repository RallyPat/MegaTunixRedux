/*
 * Export/Import Foundation - CSV/JSON table and session data
 */

#ifndef EXPORT_IMPORT_H
#define EXPORT_IMPORT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool export_table_csv(const char* path, const float* data, int width, int height);
bool import_table_csv(const char* path, float* out_data, int width, int height);

bool export_session_json(const char* path);
bool import_session_json(const char* path);

#ifdef __cplusplus
}
#endif

#endif // EXPORT_IMPORT_H


