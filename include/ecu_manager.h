/*
 * Universal ECU Manager Header
 * 
 * Provides a unified interface for connecting to different ECU types
 */

#ifndef ECU_MANAGER_H
#define ECU_MANAGER_H

#include "ecu_types.h"
#include <glib.h>

/* ECU Interface - defines how to communicate with a specific ECU type */
typedef struct {
    const gchar *name;
    EcuType type;
    
    /* Interface functions */
    gboolean (*initialize)(void);
    gboolean (*connect)(const gchar *device_path, gint baud_rate, GError **error);
    void (*disconnect)(void);
    gboolean (*is_connected)(void);
    const gchar *(*get_signature)(void);
    const gchar *(*get_firmware_version)(void);
    gpointer (*get_runtime_data)(void);
    gboolean (*send_command)(gchar command, gchar *response, gint response_size, GError **error);
    void (*shutdown)(void);
} EcuInterface;

/* ECU Manager - manages current ECU connection */
typedef struct {
    EcuDetectionResult *current_ecu;
    EcuInterface *current_interface;
    gboolean connected;
    gboolean auto_detect_enabled;
    GMutex mutex;
} EcuManager;

/* ECU Manager Functions */

/**
 * Initialize ECU manager
 * @return TRUE on success
 */
gboolean ecu_manager_initialize(void);

/**
 * Shutdown ECU manager
 */
void ecu_manager_shutdown(void);

/**
 * Detect all available ECUs
 * @return GList of EcuDetectionResult*, caller must free with ecu_detector_free_results()
 */
GList *ecu_manager_detect_ecus(void);

/**
 * Connect to a specific ECU
 * @param ecu ECU detection result
 * @param error Error location
 * @return TRUE on success
 */
gboolean ecu_manager_connect_to_ecu(EcuDetectionResult *ecu, GError **error);

/**
 * Auto-connect to best available ECU
 * @param error Error location
 * @return TRUE on success
 */
gboolean ecu_manager_auto_connect(GError **error);

/**
 * Disconnect from current ECU
 */
void ecu_manager_disconnect(void);

/**
 * Check if connected to ECU
 * @return TRUE if connected
 */
gboolean ecu_manager_is_connected(void);

/**
 * Get current ECU info
 * @return Current ECU info, or NULL if not connected
 */
const EcuDetectionResult *ecu_manager_get_current_ecu(void);

/**
 * Get ECU signature
 * @return ECU signature string, or NULL if not available
 */
const gchar *ecu_manager_get_signature(void);

/**
 * Get firmware version
 * @return Firmware version string, or NULL if not available
 */
const gchar *ecu_manager_get_firmware_version(void);

/**
 * Get runtime data (ECU-specific format)
 * @return Runtime data pointer, or NULL if not available
 */
gpointer ecu_manager_get_runtime_data(void);

/**
 * Send command to ECU
 * @param command Command to send
 * @param response Response buffer
 * @param response_size Response buffer size
 * @param error Error location
 * @return TRUE on success
 */
gboolean ecu_manager_send_command(gchar command, gchar *response, gint response_size, GError **error);

/* Speeduino interface functions (internal) */
static gboolean speeduino_interface_initialize(void);
static gboolean speeduino_interface_connect(const gchar *device_path, gint baud_rate, GError **error);
static void speeduino_interface_disconnect(void);
static gboolean speeduino_interface_is_connected(void);
static const gchar *speeduino_interface_get_signature(void);
static const gchar *speeduino_interface_get_firmware_version(void);
static gpointer speeduino_interface_get_runtime_data(void);
static gboolean speeduino_interface_send_command(gchar command, gchar *response, gint response_size, GError **error);
static void speeduino_interface_shutdown(void);

#endif /* ECU_MANAGER_H */
