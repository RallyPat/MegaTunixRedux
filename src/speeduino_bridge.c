/*
 * Speeduino Bridge for MegaTunix Redux 2025
 * 
 * This bridge provides a simplified interface to integrate the Speeduino plugin
 * with the main GUI without requiring the full plugin manager system.
 */

#include "speeduino_plugin.h"
#include "plugin_system.h"
#include "serial_utils.h"
#include <glib.h>
#include <glib-object.h>

/* Speeduino constants needed for bridge */
#define SPEEDUINO_CONFIG_PAGES 15

/* Forward declarations */
typedef struct {
    /* Communication settings */
    gchar *device_path;
    gint baud_rate;
    gint timeout_ms;
    
    /* Protocol state */
    gboolean connected;
    gboolean auto_detect;
    gchar *ecu_signature;
    gchar *firmware_version;
    
    /* Data buffers */
    SpeeduinoOutputChannels output_channels;
    GByteArray *page_data[SPEEDUINO_CONFIG_PAGES];
    
    /* Communication */
    GIOChannel *serial_channel;
    guint watch_id;
    GMutex comm_mutex;
    
    /* Logging */
    gboolean tooth_logging;
    gboolean composite_logging;
    GByteArray *tooth_log_data;
    GByteArray *composite_log_data;
} SpeeduinoContext;

/* Global bridge context - simplified singleton approach */
static MtxPluginContext *g_speeduino_context = NULL;
static GObject *g_context_object = NULL;

/* Forward declarations */
static gboolean speeduino_bridge_cleanup(void);
static gboolean speeduino_bridge_init(void);

/* Initialize the Speeduino bridge */
gboolean speeduino_bridge_init(void)
{
    GError *error = NULL;
    
    if (g_speeduino_context != NULL) {
        g_debug("Speeduino bridge already initialized");
        return TRUE;
    }
    
    g_message("Initializing Speeduino bridge...");
    
    /* Create a basic plugin context object */
    g_context_object = g_object_new(G_TYPE_OBJECT, NULL);
    if (!g_context_object) {
        g_error("Failed to create context object");
        return FALSE;
    }
    
    /* Create a minimal plugin context structure */
    g_speeduino_context = g_new0(MtxPluginContext, 1);
    g_speeduino_context->plugin_id = g_strdup("speeduino-bridge");
    g_speeduino_context->plugin_dir = g_strdup("/tmp");
    g_speeduino_context->config_dir = g_strdup("/tmp");
    g_speeduino_context->data_dir = g_strdup("/tmp");
    g_speeduino_context->config = NULL;
    g_speeduino_context->event_handlers = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    g_speeduino_context->cancellable = g_cancellable_new();
    g_speeduino_context->main_context = g_main_context_get_thread_default();
    g_mutex_init(&g_speeduino_context->event_mutex);
    
    /* Initialize the Speeduino plugin context directly */
    SpeeduinoContext *speeduino_ctx = g_new0(SpeeduinoContext, 1);
    
    /* Initialize default settings */
    speeduino_ctx->device_path = g_strdup("/dev/ttyUSB0");
    speeduino_ctx->baud_rate = 115200;
    speeduino_ctx->timeout_ms = 1000;
    speeduino_ctx->auto_detect = TRUE;
    speeduino_ctx->connected = FALSE;
    
    /* Initialize page data arrays */
    for (gint i = 0; i < SPEEDUINO_CONFIG_PAGES; i++) {
        speeduino_ctx->page_data[i] = g_byte_array_new();
    }
    
    /* Initialize logging arrays */
    speeduino_ctx->tooth_log_data = g_byte_array_new();
    speeduino_ctx->composite_log_data = g_byte_array_new();
    
    g_mutex_init(&speeduino_ctx->comm_mutex);
    
    /* Store context in plugin context */
    g_object_set_data(G_OBJECT(g_context_object), "speeduino-context", speeduino_ctx);
    
    g_message("Speeduino bridge initialized successfully");
    return TRUE;
}

/* Cleanup the Speeduino bridge */
static gboolean speeduino_bridge_cleanup(void)
{
    if (g_speeduino_context) {
        /* Disconnect if connected */
        if (speeduino_is_connected(g_speeduino_context)) {
            speeduino_disconnect_device(g_speeduino_context);
        }
        
        /* Cleanup plugin - the cleanup function is static, so just clean up directly */
        /* speeduino_cleanup(NULL); - static function, clean up manually */
        
        /* Free context resources */
        g_free(g_speeduino_context->plugin_id);
        g_free(g_speeduino_context->plugin_dir);
        g_free(g_speeduino_context->config_dir);
        g_free(g_speeduino_context->data_dir);
        g_hash_table_destroy(g_speeduino_context->event_handlers);
        g_object_unref(g_speeduino_context->cancellable);
        g_mutex_clear(&g_speeduino_context->event_mutex);
        g_free(g_speeduino_context);
        g_speeduino_context = NULL;
    }
    
    if (g_context_object) {
        g_object_unref(g_context_object);
        g_context_object = NULL;
    }
    
    return TRUE;
}

/* Public bridge functions for GUI integration */

/* Initialize bridge - call once at startup */
gboolean speeduino_bridge_initialize(void)
{
    return speeduino_bridge_init();
}

/* Cleanup bridge - call at shutdown */
void speeduino_bridge_shutdown(void)
{
    speeduino_bridge_cleanup();
}

/* Connect to Speeduino ECU */
gboolean speeduino_bridge_connect(const gchar *device_path, gint baud_rate, GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_connect_device(g_speeduino_context, device_path, baud_rate, error);
}

/* Disconnect from ECU */
void speeduino_bridge_disconnect(void)
{
    if (g_speeduino_context) {
        speeduino_disconnect_device(g_speeduino_context);
    }
}

/* Check connection status */
gboolean speeduino_bridge_is_connected(void)
{
    if (!g_speeduino_context) {
        return FALSE;
    }
    
    return speeduino_is_connected(g_speeduino_context);
}

/* Auto-detect and connect to Speeduino */
gboolean speeduino_bridge_auto_detect(GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_auto_detect(g_speeduino_context, error);
}

/* Get ECU signature */
const gchar *speeduino_bridge_get_ecu_signature(void)
{
    if (!g_speeduino_context) {
        return NULL;
    }
    
    return speeduino_get_ecu_signature(g_speeduino_context);
}

/* Get firmware version */
const gchar *speeduino_bridge_get_firmware_version(void)
{
    if (!g_speeduino_context) {
        return NULL;
    }
    
    return speeduino_get_firmware_version(g_speeduino_context);
}

/* Get runtime data */
const SpeeduinoOutputChannels *speeduino_bridge_get_runtime_data(void)
{
    if (!g_speeduino_context) {
        return NULL;
    }
    
    return speeduino_get_runtime_data(g_speeduino_context);
}

/* Get extended runtime data */
const SpeeduinoOutputChannels *speeduino_bridge_get_extended_runtime_data(void)
{
    if (!g_speeduino_context) {
        return NULL;
    }
    
    return speeduino_get_extended_runtime_data(g_speeduino_context);
}

/* Set configuration value */
gboolean speeduino_bridge_set_config_value(guint page, guint offset, guint value, GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_set_config_value(g_speeduino_context, page, offset, value, error);
}

/* Start tooth logging */
gboolean speeduino_bridge_start_tooth_logging(GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_start_tooth_logging(g_speeduino_context, error);
}

/* Stop tooth logging */
gboolean speeduino_bridge_stop_tooth_logging(GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_stop_tooth_logging(g_speeduino_context, error);
}

/* Reset ECU */
gboolean speeduino_bridge_reset_ecu(GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_reset_ecu(g_speeduino_context, error);
}

/* Send button command */
gboolean speeduino_bridge_send_button_command(guint16 button_id, GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_send_button_command(g_speeduino_context, button_id, error);
}

/* Start composite logging */
gboolean speeduino_bridge_start_composite_logging_cams(GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_start_composite_logging_cams(g_speeduino_context, error);
}

/* Stop composite logging */
gboolean speeduino_bridge_stop_composite_logging_cams(GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_stop_composite_logging_cams(g_speeduino_context, error);
}

/* Read configuration page with validation */
gboolean speeduino_bridge_read_page_with_validation(guint8 page_id, guint16 offset, 
                                                   guint16 length, GError **error)
{
    if (!g_speeduino_context) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    return speeduino_read_page_with_validation(g_speeduino_context, page_id, offset, length, error);
}
