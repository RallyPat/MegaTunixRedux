/*
 * Universal ECU Manager for MegaTunix Redux
 * 
 * This replaces the Speeduino-specific bridge with a universal system
 * that can handle multiple ECU types through a common interface
 */

#include "ecu_manager.h"
#include "ecu_detector.h"
#include "speeduino_bridge.h"  /* Will be refactored */
#include <glib.h>
#include <stdio.h>
#include <string.h>

/* Global ECU manager instance */
static EcuManager *g_ecu_manager = NULL;

/* ECU-specific interface implementations */
static EcuInterface speeduino_interface = {
    .name = "Speeduino",
    .type = ECU_TYPE_SPEEDUINO,
    .initialize = speeduino_interface_initialize,
    .connect = speeduino_interface_connect,
    .disconnect = speeduino_interface_disconnect,
    .is_connected = speeduino_interface_is_connected,
    .get_signature = speeduino_interface_get_signature,
    .get_firmware_version = speeduino_interface_get_firmware_version,
    .get_runtime_data = speeduino_interface_get_runtime_data,
    .send_command = speeduino_interface_send_command,
    .shutdown = speeduino_interface_shutdown
};

/* TODO: Add MS1, MS2, MS3 interfaces */

/* Available ECU interfaces */
static EcuInterface *available_interfaces[] = {
    &speeduino_interface,
    /* TODO: Add other interfaces */
    NULL
};

/* Initialize ECU manager */
gboolean ecu_manager_initialize(void)
{
    if (g_ecu_manager != NULL) {
        g_debug("ECU manager already initialized");
        return TRUE;
    }
    
    g_message("Initializing ECU manager...");
    
    g_ecu_manager = g_new0(EcuManager, 1);
    g_ecu_manager->current_ecu = NULL;
    g_ecu_manager->current_interface = NULL;
    g_ecu_manager->connected = FALSE;
    g_ecu_manager->auto_detect_enabled = TRUE;
    
    g_mutex_init(&g_ecu_manager->mutex);
    
    /* Initialize all available interfaces */
    for (gint i = 0; available_interfaces[i] != NULL; i++) {
        EcuInterface *interface = available_interfaces[i];
        if (interface->initialize) {
            if (!interface->initialize()) {
                g_warning("Failed to initialize %s interface", interface->name);
            } else {
                g_debug("Initialized %s interface", interface->name);
            }
        }
    }
    
    g_message("ECU manager initialized successfully");
    return TRUE;
}

/* Shutdown ECU manager */
void ecu_manager_shutdown(void)
{
    if (g_ecu_manager == NULL) {
        return;
    }
    
    g_message("Shutting down ECU manager...");
    
    /* Disconnect if connected */
    if (g_ecu_manager->connected) {
        ecu_manager_disconnect();
    }
    
    /* Shutdown all interfaces */
    for (gint i = 0; available_interfaces[i] != NULL; i++) {
        EcuInterface *interface = available_interfaces[i];
        if (interface->shutdown) {
            interface->shutdown();
        }
    }
    
    /* Free current ECU info */
    if (g_ecu_manager->current_ecu) {
        ecu_detection_result_free(g_ecu_manager->current_ecu);
    }
    
    g_mutex_clear(&g_ecu_manager->mutex);
    g_free(g_ecu_manager);
    g_ecu_manager = NULL;
    
    g_message("ECU manager shutdown complete");
}

/* Auto-detect ECUs */
GList *ecu_manager_detect_ecus(void)
{
    g_message("Starting ECU auto-detection...");
    
    GList *detected_ecus = ecu_detector_scan_all_devices();
    
    if (detected_ecus) {
        g_message("Auto-detection found %d ECU(s)", g_list_length(detected_ecus));
        
        /* Print detected ECUs */
        for (GList *l = detected_ecus; l != NULL; l = l->next) {
            EcuDetectionResult *result = (EcuDetectionResult *)l->data;
            g_message("  - %s at %s (%d baud): %s", 
                     result->ecu_name, result->device_path, result->baud_rate, 
                     result->signature);
        }
    } else {
        g_message("No ECUs detected");
    }
    
    return detected_ecus;
}

/* Find interface for ECU type */
static EcuInterface *find_interface_for_type(EcuType type)
{
    for (gint i = 0; available_interfaces[i] != NULL; i++) {
        if (available_interfaces[i]->type == type) {
            return available_interfaces[i];
        }
    }
    return NULL;
}

/* Connect to a specific ECU */
gboolean ecu_manager_connect_to_ecu(EcuDetectionResult *ecu, GError **error)
{
    if (g_ecu_manager == NULL) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "ECU manager not initialized");
        return FALSE;
    }
    
    if (g_ecu_manager->connected) {
        g_message("Already connected to ECU, disconnecting first...");
        ecu_manager_disconnect();
    }
    
    g_mutex_lock(&g_ecu_manager->mutex);
    
    /* Find appropriate interface */
    EcuInterface *interface = find_interface_for_type(ecu->ecu_type);
    if (!interface) {
        g_mutex_unlock(&g_ecu_manager->mutex);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                   "No interface available for ECU type: %s", 
                   ecu_detector_get_type_name(ecu->ecu_type));
        return FALSE;
    }
    
    g_message("Connecting to %s at %s (%d baud)...", 
             ecu->ecu_name, ecu->device_path, ecu->baud_rate);
    
    /* Attempt connection */
    if (interface->connect(ecu->device_path, ecu->baud_rate, error)) {
        g_ecu_manager->current_ecu = g_new0(EcuDetectionResult, 1);
        g_ecu_manager->current_ecu->ecu_type = ecu->ecu_type;
        g_ecu_manager->current_ecu->device_path = g_strdup(ecu->device_path);
        g_ecu_manager->current_ecu->baud_rate = ecu->baud_rate;
        g_ecu_manager->current_ecu->ecu_name = g_strdup(ecu->ecu_name);
        g_ecu_manager->current_ecu->signature = g_strdup(ecu->signature);
        g_ecu_manager->current_ecu->confidence = ecu->confidence;
        
        g_ecu_manager->current_interface = interface;
        g_ecu_manager->connected = TRUE;
        
        g_message("✅ Successfully connected to %s", ecu->ecu_name);
        g_mutex_unlock(&g_ecu_manager->mutex);
        return TRUE;
    } else {
        g_mutex_unlock(&g_ecu_manager->mutex);
        g_message("❌ Failed to connect to %s: %s", ecu->ecu_name, 
                 error && *error ? (*error)->message : "Unknown error");
        return FALSE;
    }
}

/* Auto-connect to best available ECU */
gboolean ecu_manager_auto_connect(GError **error)
{
    g_message("Starting auto-connect...");
    
    /* Detect ECUs */
    GList *detected_ecus = ecu_manager_detect_ecus();
    if (!detected_ecus) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                   "No ECUs detected");
        return FALSE;
    }
    
    /* Find best ECU */
    EcuDetectionResult *best_ecu = ecu_detector_get_best_ecu(detected_ecus);
    if (!best_ecu) {
        ecu_detector_free_results(detected_ecus);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                   "No suitable ECU found");
        return FALSE;
    }
    
    g_message("Auto-connecting to best ECU: %s", best_ecu->ecu_name);
    
    /* Connect to best ECU */
    gboolean result = ecu_manager_connect_to_ecu(best_ecu, error);
    
    ecu_detector_free_results(detected_ecus);
    return result;
}

/* Manual connect to specific port and baud rate */
gboolean ecu_manager_manual_connect(const gchar *device_path, gint baud_rate, GError **error)
{
    g_message("🔧 Starting manual connect to %s at %d baud...", device_path, baud_rate);
    
    if (g_ecu_manager == NULL) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "ECU manager not initialized");
        return FALSE;
    }
    
    if (g_ecu_manager->connected) {
        g_message("Already connected to ECU, disconnecting first...");
        ecu_manager_disconnect();
    }
    
    // Add debugging about the device path
    g_message("🔧 Checking device path: %s", device_path);
    if (!g_file_test(device_path, G_FILE_TEST_EXISTS)) {
        g_warning("🔧 Device path %s does not exist!", device_path);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                   "Device %s does not exist", device_path);
        return FALSE;
    }
    
    // Try to detect ECU at specific port and baud rate
    g_message("🔧 Calling ecu_detector_test_device for %s at %d baud", device_path, baud_rate);
    EcuDetectionResult *ecu = ecu_detector_test_device(device_path, baud_rate);
    if (!ecu) {
        g_warning("🔧 ecu_detector_test_device returned NULL for %s at %d baud", device_path, baud_rate);
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
                   "No ECU detected at %s (%d baud)", device_path, baud_rate);
        return FALSE;
    }
    
    g_message("🔧 Detected ECU at manual selection: %s (type: %d)", ecu->ecu_name, ecu->ecu_type);
    
    /* Connect to detected ECU */
    g_message("🔧 Calling ecu_manager_connect_to_ecu...");
    gboolean result = ecu_manager_connect_to_ecu(ecu, error);
    
    if (result) {
        g_message("🔧 Manual connection successful!");
    } else {
        g_warning("🔧 Manual connection failed: %s", error && *error ? (*error)->message : "Unknown error");
    }
    
    ecu_detection_result_free(ecu);
    return result;
}

/* Disconnect from current ECU */
void ecu_manager_disconnect(void)
{
    if (g_ecu_manager == NULL || !g_ecu_manager->connected) {
        return;
    }
    
    g_message("Disconnecting from ECU...");
    
    g_mutex_lock(&g_ecu_manager->mutex);
    
    if (g_ecu_manager->current_interface && g_ecu_manager->current_interface->disconnect) {
        g_ecu_manager->current_interface->disconnect();
    }
    
    if (g_ecu_manager->current_ecu) {
        ecu_detection_result_free(g_ecu_manager->current_ecu);
        g_ecu_manager->current_ecu = NULL;
    }
    
    g_ecu_manager->current_interface = NULL;
    g_ecu_manager->connected = FALSE;
    
    g_mutex_unlock(&g_ecu_manager->mutex);
    
    g_message("Disconnected from ECU");
}

/* Check if connected */
gboolean ecu_manager_is_connected(void)
{
    if (g_ecu_manager == NULL) {
        return FALSE;
    }
    
    return g_ecu_manager->connected && 
           g_ecu_manager->current_interface && 
           g_ecu_manager->current_interface->is_connected();
}

/* Get current ECU info */
const EcuDetectionResult *ecu_manager_get_current_ecu(void)
{
    if (g_ecu_manager == NULL) {
        return NULL;
    }
    
    return g_ecu_manager->current_ecu;
}

/* Get ECU signature */
const gchar *ecu_manager_get_signature(void)
{
    if (g_ecu_manager == NULL || !g_ecu_manager->connected || !g_ecu_manager->current_interface) {
        return NULL;
    }
    
    if (g_ecu_manager->current_interface->get_signature) {
        return g_ecu_manager->current_interface->get_signature();
    }
    
    return g_ecu_manager->current_ecu ? g_ecu_manager->current_ecu->signature : NULL;
}

/* Get firmware version */
const gchar *ecu_manager_get_firmware_version(void)
{
    if (g_ecu_manager == NULL || !g_ecu_manager->connected || !g_ecu_manager->current_interface) {
        return NULL;
    }
    
    if (g_ecu_manager->current_interface->get_firmware_version) {
        return g_ecu_manager->current_interface->get_firmware_version();
    }
    
    return NULL;
}

/* Get runtime data */
gpointer ecu_manager_get_runtime_data(void)
{
    if (g_ecu_manager == NULL || !g_ecu_manager->connected || !g_ecu_manager->current_interface) {
        return NULL;
    }
    
    if (g_ecu_manager->current_interface->get_runtime_data) {
        return g_ecu_manager->current_interface->get_runtime_data();
    }
    
    return NULL;
}

/* Send command to ECU */
gboolean ecu_manager_send_command(gchar command, gchar *response, gint response_size, GError **error)
{
    if (g_ecu_manager == NULL || !g_ecu_manager->connected || !g_ecu_manager->current_interface) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED,
                   "Not connected to ECU");
        return FALSE;
    }
    
    if (g_ecu_manager->current_interface->send_command) {
        return g_ecu_manager->current_interface->send_command(command, response, response_size, error);
    }
    
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
               "Command sending not supported for this ECU type");
    return FALSE;
}

/* Speeduino interface implementation (wrapper around existing bridge) */
static gboolean speeduino_interface_initialize(void)
{
    return speeduino_bridge_initialize();
}

static gboolean speeduino_interface_connect(const gchar *device_path, gint baud_rate, GError **error)
{
    return speeduino_bridge_connect(device_path, baud_rate, error);
}

static void speeduino_interface_disconnect(void)
{
    speeduino_bridge_disconnect();
}

static gboolean speeduino_interface_is_connected(void)
{
    return speeduino_bridge_is_connected();
}

static const gchar *speeduino_interface_get_signature(void)
{
    return speeduino_bridge_get_ecu_signature();
}

static const gchar *speeduino_interface_get_firmware_version(void)
{
    return speeduino_bridge_get_firmware_version();
}

static gpointer speeduino_interface_get_runtime_data(void)
{
    return (gpointer)speeduino_bridge_get_runtime_data();
}

static gboolean speeduino_interface_send_command(gchar command, gchar *response, gint response_size, GError **error)
{
    /* TODO: Implement if needed */
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
               "Direct command sending not implemented for Speeduino");
    return FALSE;
}

static void speeduino_interface_shutdown(void)
{
    speeduino_bridge_shutdown();
}
