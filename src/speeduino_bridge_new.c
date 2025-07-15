/*
 * Speeduino Bridge for MegaTunix Redux
 * 
 * This bridge provides a simplified interface to communicate with Speeduino ECUs
 * using direct serial communication based on the successful standalone test.
 */

#include "speeduino_plugin.h"
#include "plugin_system.h"
#include <glib.h>
#include <glib-object.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>

/* Speeduino command constants */
#define SPEEDUINO_SIGNATURE_CMD 'S'
#define SPEEDUINO_VERSION_CMD 'V'
#define SPEEDUINO_REALTIME_CMD 'A'
#define SPEEDUINO_Q_CMD 'Q'

/* Bridge state structure */
typedef struct {
    gchar *device_path;
    gint baud_rate;
    gint serial_fd;
    gboolean connected;
    gchar *ecu_signature;
    gchar *firmware_version;
    SpeeduinoOutputChannels output_channels;
    GMutex comm_mutex;
} SpeeduinoBridge;

/* Global bridge instance */
static SpeeduinoBridge *g_bridge = NULL;

/* Forward declarations */
static gboolean configure_serial_port(gint fd, gint baud_rate);
static gint send_command_and_read(gint fd, gchar cmd, gchar *buffer, gint max_len, gint timeout_ms);
static gboolean update_ecu_info(void);
static gboolean update_runtime_data(void);

/* Initialize bridge */
gboolean speeduino_bridge_initialize(void)
{
    if (g_bridge != NULL) {
        g_debug("Speeduino bridge already initialized");
        return TRUE;
    }
    
    g_message("Initializing Speeduino bridge...");
    
    g_bridge = g_new0(SpeeduinoBridge, 1);
    g_bridge->device_path = NULL;
    g_bridge->baud_rate = 115200;
    g_bridge->serial_fd = -1;
    g_bridge->connected = FALSE;
    g_bridge->ecu_signature = NULL;
    g_bridge->firmware_version = NULL;
    g_mutex_init(&g_bridge->comm_mutex);
    
    /* Initialize output channels structure */
    memset(&g_bridge->output_channels, 0, sizeof(SpeeduinoOutputChannels));
    
    g_message("Speeduino bridge initialized successfully");
    return TRUE;
}

/* Cleanup bridge */
void speeduino_bridge_shutdown(void)
{
    if (g_bridge == NULL) {
        return;
    }
    
    g_message("Shutting down Speeduino bridge...");
    
    if (g_bridge->connected) {
        speeduino_bridge_disconnect();
    }
    
    g_free(g_bridge->device_path);
    g_free(g_bridge->ecu_signature);
    g_free(g_bridge->firmware_version);
    g_mutex_clear(&g_bridge->comm_mutex);
    
    g_free(g_bridge);
    g_bridge = NULL;
    
    g_message("Speeduino bridge shutdown complete");
}

/* Connect to Speeduino ECU */
gboolean speeduino_bridge_connect(const gchar *device_path, gint baud_rate, GError **error)
{
    if (g_bridge == NULL) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_INITIALIZED,
                   "Speeduino bridge not initialized");
        return FALSE;
    }
    
    if (g_bridge->connected) {
        g_message("Already connected to Speeduino, disconnecting first...");
        speeduino_bridge_disconnect();
    }
    
    g_message("Connecting to Speeduino at %s (baud: %d)", device_path, baud_rate);
    
    /* Open serial port */
    g_bridge->serial_fd = open(device_path, O_RDWR | O_NOCTTY | O_SYNC);
    if (g_bridge->serial_fd < 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to open %s: %s", device_path, strerror(errno));
        return FALSE;
    }
    
    /* Configure serial port */
    if (!configure_serial_port(g_bridge->serial_fd, baud_rate)) {
        close(g_bridge->serial_fd);
        g_bridge->serial_fd = -1;
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to configure serial port");
        return FALSE;
    }
    
    /* Store connection info */
    g_free(g_bridge->device_path);
    g_bridge->device_path = g_strdup(device_path);
    g_bridge->baud_rate = baud_rate;
    
    /* Wait for device to initialize */
    g_usleep(2000000); /* 2 seconds */
    
    /* Test connection by getting ECU info */
    if (!update_ecu_info()) {
        close(g_bridge->serial_fd);
        g_bridge->serial_fd = -1;
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to communicate with Speeduino ECU");
        return FALSE;
    }
    
    g_bridge->connected = TRUE;
    g_message("Successfully connected to Speeduino: %s", g_bridge->ecu_signature);
    
    return TRUE;
}

/* Disconnect from Speeduino ECU */
void speeduino_bridge_disconnect(void)
{
    if (g_bridge == NULL || !g_bridge->connected) {
        return;
    }
    
    g_message("Disconnecting from Speeduino...");
    
    g_mutex_lock(&g_bridge->comm_mutex);
    
    if (g_bridge->serial_fd >= 0) {
        close(g_bridge->serial_fd);
        g_bridge->serial_fd = -1;
    }
    
    g_bridge->connected = FALSE;
    
    g_mutex_unlock(&g_bridge->comm_mutex);
    
    g_message("Disconnected from Speeduino");
}

/* Check if connected */
gboolean speeduino_bridge_is_connected(void)
{
    return g_bridge != NULL && g_bridge->connected;
}

/* Get ECU signature */
const gchar *speeduino_bridge_get_ecu_signature(void)
{
    return g_bridge ? g_bridge->ecu_signature : NULL;
}

/* Get firmware version */
const gchar *speeduino_bridge_get_firmware_version(void)
{
    return g_bridge ? g_bridge->firmware_version : NULL;
}

/* Get runtime data */
const SpeeduinoOutputChannels *speeduino_bridge_get_runtime_data(void)
{
    if (g_bridge == NULL || !g_bridge->connected) {
        return NULL;
    }
    
    /* Update runtime data from ECU */
    if (!update_runtime_data()) {
        return NULL;
    }
    
    return &g_bridge->output_channels;
}

/* Configure serial port */
static gboolean configure_serial_port(gint fd, gint baud_rate)
{
    struct termios tty;
    
    if (tcgetattr(fd, &tty) < 0) {
        g_warning("Error getting terminal attributes: %s", strerror(errno));
        return FALSE;
    }
    
    /* Set baud rate */
    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600;   break;
        case 19200:  speed = B19200;  break;
        case 38400:  speed = B38400;  break;
        case 57600:  speed = B57600;  break;
        case 115200: speed = B115200; break;
        default:
            g_warning("Unsupported baud rate: %d", baud_rate);
            return FALSE;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    /* Configure for 8N1 */
    tty.c_cflag &= ~PARENB;   
    tty.c_cflag &= ~CSTOPB;   
    tty.c_cflag &= ~CSIZE;    
    tty.c_cflag |= CS8;       
    tty.c_cflag &= ~CRTSCTS;  
    tty.c_cflag |= CREAD | CLOCAL;
    
    /* Configure input modes */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    
    /* Configure output modes */
    tty.c_oflag &= ~OPOST;
    
    /* Configure local modes */
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    
    /* Set timeouts */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;
    
    if (tcsetattr(fd, TCSANOW, &tty) < 0) {
        g_warning("Error setting terminal attributes: %s", strerror(errno));
        return FALSE;
    }
    
    return TRUE;
}

/* Send command and read response */
static gint send_command_and_read(gint fd, gchar cmd, gchar *buffer, gint max_len, gint timeout_ms)
{
    fd_set readfds;
    struct timeval timeout;
    gint total_bytes = 0;
    gint bytes_read;
    
    /* Send command */
    if (write(fd, &cmd, 1) != 1) {
        g_warning("Error sending command '%c': %s", cmd, strerror(errno));
        return -1;
    }
    
    tcdrain(fd);
    
    /* Read response */
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    while (total_bytes < max_len - 1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (result < 0) {
            g_warning("Select error: %s", strerror(errno));
            break;
        } else if (result == 0) {
            /* Timeout - but we might have some data */
            break;
        }
        
        bytes_read = read(fd, buffer + total_bytes, max_len - total_bytes - 1);
        if (bytes_read < 0) {
            g_warning("Error reading response: %s", strerror(errno));
            break;
        } else if (bytes_read == 0) {
            break;
        }
        
        total_bytes += bytes_read;
    }
    
    return total_bytes;
}

/* Update ECU info */
static gboolean update_ecu_info(void)
{
    gchar buffer[256];
    gint bytes_read;
    
    /* Get signature via Q command */
    bytes_read = send_command_and_read(g_bridge->serial_fd, SPEEDUINO_Q_CMD, buffer, sizeof(buffer), 2000);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        g_free(g_bridge->ecu_signature);
        g_bridge->ecu_signature = g_strdup(buffer);
        g_message("ECU signature: %s", g_bridge->ecu_signature);
    }
    
    /* Get version */
    bytes_read = send_command_and_read(g_bridge->serial_fd, SPEEDUINO_VERSION_CMD, buffer, sizeof(buffer), 2000);
    if (bytes_read > 0) {
        /* Version response is binary, convert to string */
        g_free(g_bridge->firmware_version);
        g_bridge->firmware_version = g_strdup_printf("v%d.%d.%d", 
                                                      (unsigned char)buffer[0], 
                                                      (unsigned char)buffer[1], 
                                                      (unsigned char)buffer[2]);
        g_message("Firmware version: %s", g_bridge->firmware_version);
    }
    
    return (g_bridge->ecu_signature != NULL);
}

/* Update runtime data */
static gboolean update_runtime_data(void)
{
    gchar buffer[256];
    gint bytes_read;
    
    g_mutex_lock(&g_bridge->comm_mutex);
    
    if (g_bridge->serial_fd < 0) {
        g_mutex_unlock(&g_bridge->comm_mutex);
        return FALSE;
    }
    
    /* Get realtime data */
    bytes_read = send_command_and_read(g_bridge->serial_fd, SPEEDUINO_REALTIME_CMD, buffer, sizeof(buffer), 1000);
    
    g_mutex_unlock(&g_bridge->comm_mutex);
    
    if (bytes_read < 4) {
        g_warning("Invalid realtime data received (%d bytes)", bytes_read);
        return FALSE;
    }
    
    /* Parse realtime data (simplified - these are approximate positions) */
    unsigned char *data = (unsigned char *)buffer;
    
    /* RPM is typically in bytes 14-15 as big-endian uint16 */
    if (bytes_read > 15) {
        g_bridge->output_channels.rpm = (data[14] << 8) | data[15];
    }
    
    /* MAP is typically in byte 4 */
    if (bytes_read > 4) {
        g_bridge->output_channels.map = data[4];
    }
    
    /* TPS is typically in byte 6 */
    if (bytes_read > 6) {
        g_bridge->output_channels.tps = data[6];
    }
    
    /* Coolant is typically in byte 7 */
    if (bytes_read > 7) {
        g_bridge->output_channels.coolant = data[7];
    }
    
    /* Battery voltage is typically in bytes 28-29 */
    if (bytes_read > 29) {
        g_bridge->output_channels.battery10 = (data[28] << 8) | data[29];
    }
    
    /* Advance is typically in byte 24 */
    if (bytes_read > 24) {
        g_bridge->output_channels.advance = data[24];
    }
    
    return TRUE;
}
