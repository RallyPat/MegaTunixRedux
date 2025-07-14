/*
 * Speeduino Bridge Header for MegaTunix Redux
 * 
 * This bridge provides a simplified interface to integrate the Speeduino plugin
 * with the main GUI without requiring the full plugin manager system.
 */

#ifndef SPEEDUINO_BRIDGE_H
#define SPEEDUINO_BRIDGE_H

#include <glib.h>
#include "speeduino_plugin.h"

G_BEGIN_DECLS

/* Bridge initialization and cleanup */
gboolean speeduino_bridge_initialize(void);
void speeduino_bridge_shutdown(void);

/* Connection management */
gboolean speeduino_bridge_connect(const gchar *device_path, gint baud_rate, GError **error);
void speeduino_bridge_disconnect(void);
gboolean speeduino_bridge_is_connected(void);
gboolean speeduino_bridge_auto_detect(GError **error);

/* ECU information */
const gchar *speeduino_bridge_get_ecu_signature(void);
const gchar *speeduino_bridge_get_firmware_version(void);

/* Runtime data */
const SpeeduinoOutputChannels *speeduino_bridge_get_runtime_data(void);
const SpeeduinoOutputChannels *speeduino_bridge_get_extended_runtime_data(void);

/* Configuration */
gboolean speeduino_bridge_set_config_value(guint page, guint offset, guint value, GError **error);
gboolean speeduino_bridge_read_page_with_validation(guint8 page_id, guint16 offset, 
                                                   guint16 length, GError **error);

/* Logging */
gboolean speeduino_bridge_start_tooth_logging(GError **error);
gboolean speeduino_bridge_stop_tooth_logging(GError **error);
gboolean speeduino_bridge_start_composite_logging_cams(GError **error);
gboolean speeduino_bridge_stop_composite_logging_cams(GError **error);

/* Commands */
gboolean speeduino_bridge_send_button_command(guint16 button_id, GError **error);
gboolean speeduino_bridge_reset_ecu(GError **error);

G_END_DECLS

#endif /* SPEEDUINO_BRIDGE_H */
