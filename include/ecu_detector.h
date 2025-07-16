/*
 * ECU Detection System Header
 * 
 * Universal ECU detection for MegaTunix Redux
 */

#ifndef ECU_DETECTOR_H
#define ECU_DETECTOR_H

#include "ecu_types.h"
#include <glib.h>

/* Main detection functions */

/**
 * Scan all available serial devices for ECUs
 * @return GList of EcuDetectionResult*, caller must free with ecu_detector_free_results()
 */
GList *ecu_detector_scan_all_devices(void);

/**
 * Get user-friendly ECU type name
 * @param type ECU type enum
 * @return Human-readable ECU type name
 */
const gchar *ecu_detector_get_type_name(EcuType type);

/**
 * Get the best ECU from detection results (highest confidence, preferred types)
 * @param results List of EcuDetectionResult*
 * @return Best ECU result (still owned by the list), or NULL if no ECUs found
 */
EcuDetectionResult *ecu_detector_get_best_ecu(GList *results);

/**
 * Test a specific device at a specific baud rate
 * @param device_path Device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate Baud rate to test
 * @return EcuDetectionResult if ECU found, NULL otherwise (caller must free)
 */
EcuDetectionResult *ecu_detector_test_device(const gchar *device_path, gint baud_rate);

/**
 * Free a single detection result
 * @param result EcuDetectionResult to free
 */
void ecu_detection_result_free(EcuDetectionResult *result);

/**
 * Free list of detection results
 * @param results GList of EcuDetectionResult*
 */
void ecu_detector_free_results(GList *results);

#endif /* ECU_DETECTOR_H */
