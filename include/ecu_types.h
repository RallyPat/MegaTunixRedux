/*
 * ECU Types and Detection System Headers
 * 
 * Defines ECU types and structures for universal ECU detection
 */

#ifndef ECU_TYPES_H
#define ECU_TYPES_H

#include <glib.h>

/* ECU Types */
typedef enum {
    ECU_TYPE_UNKNOWN = 0,
    ECU_TYPE_SPEEDUINO,
    ECU_TYPE_MS1,        /* MegaSquirt 1 */
    ECU_TYPE_MS2,        /* MegaSquirt 2 */
    ECU_TYPE_MS3,        /* MegaSquirt 3 */
    ECU_TYPE_JIMSTIM,    /* JimStim simulator */
    ECU_TYPE_GENERIC     /* Generic/fallback */
} EcuType;

/* ECU Detection Result */
typedef struct {
    EcuType ecu_type;
    gchar *device_path;
    gint baud_rate;
    gchar *ecu_name;
    gchar *signature;
    gint confidence;     /* 0-100 confidence score */
} EcuDetectionResult;

/* ECU Probe Command */
typedef struct {
    const gchar *name;
    EcuType type;
    gchar command;
    const gchar *expected_response;
    gint response_min_length;
    gint timeout_ms;
    gint baud_rates[8];  /* Terminated with 0 */
} EcuProbeCommand;

#endif /* ECU_TYPES_H */
