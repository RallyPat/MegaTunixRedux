/*
 * Serial communication utilities header for MegaTunix Redux 2025
 */

#ifndef SERIAL_UTILS_H
#define SERIAL_UTILS_H

#include <glib.h>
#include <gio/gio.h>
#include <glob.h>

G_BEGIN_DECLS

/**
 * Open a serial port with specified settings
 * @param device_path Path to serial device (e.g., "/dev/ttyUSB0")
 * @param baud_rate Baud rate (9600, 19200, 38400, 57600, 115200, 230400)
 * @param error Error return location
 * @return GIOChannel for the serial port or NULL on error
 */
GIOChannel *serial_open_port(const gchar *device_path, gint baud_rate, GError **error);

/**
 * Close a serial port
 * @param channel Serial port channel to close
 */
void serial_close_port(GIOChannel *channel);

/**
 * Write a single byte to serial port
 * @param channel Serial port channel
 * @param byte Byte to write
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean serial_write_byte(GIOChannel *channel, guchar byte, GError **error);

/**
 * Write data to serial port
 * @param channel Serial port channel
 * @param data Data to write
 * @param data_len Length of data
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean serial_write_data(GIOChannel *channel, const guchar *data, gsize data_len, GError **error);

/**
 * Read data from serial port with timeout
 * @param channel Serial port channel
 * @param buffer Buffer to store received data
 * @param buffer_len Input: buffer size, Output: bytes actually read
 * @param timeout_ms Timeout in milliseconds
 * @param error Error return location
 * @return TRUE if any data was read, FALSE on error
 */
gboolean serial_read_data(GIOChannel *channel, guchar *buffer, gsize *buffer_len,
                         gint timeout_ms, GError **error);

/**
 * Read a line from serial port
 * @param channel Serial port channel
 * @param buffer Buffer to store the line
 * @param buffer_size Size of buffer
 * @param timeout_ms Timeout in milliseconds
 * @param error Error return location
 * @return TRUE if a line was read, FALSE on error
 */
gboolean serial_read_line(GIOChannel *channel, gchar *buffer, gsize buffer_size,
                         gint timeout_ms, GError **error);

/**
 * Flush serial port buffers
 * @param channel Serial port channel
 */
void serial_flush(GIOChannel *channel);

/**
 * Set serial port timeout
 * @param channel Serial port channel
 * @param timeout_ms Timeout in milliseconds
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean serial_set_timeout(GIOChannel *channel, gint timeout_ms, GError **error);

/**
 * Check if data is available for reading
 * @param channel Serial port channel
 * @return TRUE if data is available, FALSE otherwise
 */
gboolean serial_data_available(GIOChannel *channel);

/**
 * Get list of available serial devices
 * @return GList of device paths (must be freed with serial_free_device_list)
 */
GList *serial_get_available_devices(void);

/**
 * Free device list returned by serial_get_available_devices
 * @param devices Device list to free
 */
void serial_free_device_list(GList *devices);

G_END_DECLS

#endif /* SERIAL_UTILS_H */
