/*
 * Serial communication utilities for MegaTunix Redux
 */

#include "serial_utils.h"
#include <glib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Define CRTSCTS if not defined (some systems don't have it) */
#ifndef CRTSCTS
#define CRTSCTS 0
#endif

/* Open serial port with specified settings */
GIOChannel *serial_open_port(const gchar *device_path, gint baud_rate, GError **error)
{
    g_return_val_if_fail(device_path != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);
    
    /* Open the device */
    gint fd = open(device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to open serial device %s: %s", device_path, g_strerror(errno));
        return NULL;
    }
    
    /* Configure serial port */
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to get terminal attributes: %s", g_strerror(errno));
        close(fd);
        return NULL;
    }
    
    /* Set baud rate */
    speed_t speed;
    switch (baud_rate) {
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        default:
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT,
                       "Unsupported baud rate: %d", baud_rate);
            close(fd);
            return NULL;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    /* Configure port settings */
    tty.c_cflag &= ~PARENB;     // No parity
    tty.c_cflag &= ~CSTOPB;     // One stop bit
    tty.c_cflag &= ~CSIZE;      // Clear size bits
    tty.c_cflag |= CS8;         // 8 data bits
    tty.c_cflag &= ~CRTSCTS;    // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable reading, ignore modem control lines
    
    tty.c_lflag &= ~ICANON;     // Non-canonical mode
    tty.c_lflag &= ~ECHO;       // No echo
    tty.c_lflag &= ~ECHOE;      // No echo erase
    tty.c_lflag &= ~ECHONL;     // No echo newline
    tty.c_lflag &= ~ISIG;       // No signal processing
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    tty.c_oflag &= ~OPOST;      // No output processing
    tty.c_oflag &= ~ONLCR;      // No newline conversion
    
    /* Set timeouts */
    tty.c_cc[VTIME] = 10;       // 1 second timeout
    tty.c_cc[VMIN] = 0;         // Return immediately
    
    /* Apply settings */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to set terminal attributes: %s", g_strerror(errno));
        close(fd);
        return NULL;
    }
    
    /* Create GIOChannel */
    GIOChannel *channel = g_io_channel_unix_new(fd);
    if (!channel) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Failed to create GIOChannel");
        close(fd);
        return NULL;
    }
    
    /* Set binary mode */
    g_io_channel_set_encoding(channel, NULL, NULL);
    g_io_channel_set_buffered(channel, FALSE);
    
    return channel;
}

/* Close serial port */
void serial_close_port(GIOChannel *channel)
{
    if (channel) {
        g_io_channel_shutdown(channel, TRUE, NULL);
        g_io_channel_unref(channel);
    }
}

/* Write a single byte to serial port */
gboolean serial_write_byte(GIOChannel *channel, guchar byte, GError **error)
{
    g_return_val_if_fail(channel != NULL, FALSE);
    
    gsize bytes_written;
    GIOStatus status = g_io_channel_write_chars(channel, (gchar *)&byte, 1, 
                                               &bytes_written, error);
    
    if (status != G_IO_STATUS_NORMAL || bytes_written != 1) {
        if (error && !*error) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Failed to write byte to serial port");
        }
        return FALSE;
    }
    
    g_io_channel_flush(channel, NULL);
    return TRUE;
}

/* Write data to serial port */
gboolean serial_write_data(GIOChannel *channel, const guchar *data, gsize data_len, GError **error)
{
    g_return_val_if_fail(channel != NULL, FALSE);
    g_return_val_if_fail(data != NULL, FALSE);
    
    gsize bytes_written;
    GIOStatus status = g_io_channel_write_chars(channel, (gchar *)data, data_len,
                                               &bytes_written, error);
    
    if (status != G_IO_STATUS_NORMAL || bytes_written != data_len) {
        if (error && !*error) {
            g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                       "Failed to write data to serial port");
        }
        return FALSE;
    }
    
    g_io_channel_flush(channel, NULL);
    return TRUE;
}

/* Read data from serial port with timeout */
gboolean serial_read_data(GIOChannel *channel, guchar *buffer, gsize *buffer_len,
                         gint timeout_ms, GError **error)
{
    g_return_val_if_fail(channel != NULL, FALSE);
    g_return_val_if_fail(buffer != NULL, FALSE);
    g_return_val_if_fail(buffer_len != NULL, FALSE);
    
    gsize bytes_read = 0;
    gsize total_bytes = *buffer_len;
    GTimeVal start_time, current_time;
    
    g_get_current_time(&start_time);
    
    while (bytes_read < total_bytes) {
        gsize chunk_bytes_read;
        GIOStatus status = g_io_channel_read_chars(channel, 
                                                  (gchar *)&buffer[bytes_read],
                                                  total_bytes - bytes_read,
                                                  &chunk_bytes_read, error);
        
        if (status == G_IO_STATUS_NORMAL) {
            bytes_read += chunk_bytes_read;
        } else if (status == G_IO_STATUS_AGAIN) {
            /* No data available, check timeout */
            g_get_current_time(&current_time);
            glong elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                              (current_time.tv_usec - start_time.tv_usec) / 1000;
            
            if (elapsed_ms >= timeout_ms) {
                break; /* Timeout reached */
            }
            
            g_usleep(1000); /* Wait 1ms before retrying */
        } else {
            /* Error occurred */
            return FALSE;
        }
    }
    
    *buffer_len = bytes_read;
    return bytes_read > 0;
}

/* Read a line from serial port */
gboolean serial_read_line(GIOChannel *channel, gchar *buffer, gsize buffer_size,
                         gint timeout_ms, GError **error)
{
    g_return_val_if_fail(channel != NULL, FALSE);
    g_return_val_if_fail(buffer != NULL, FALSE);
    g_return_val_if_fail(buffer_size > 0, FALSE);
    
    gsize bytes_read = 0;
    GTimeVal start_time, current_time;
    
    g_get_current_time(&start_time);
    
    while (bytes_read < buffer_size - 1) {
        gchar ch;
        gsize chunk_bytes_read;
        GIOStatus status = g_io_channel_read_chars(channel, &ch, 1, 
                                                  &chunk_bytes_read, error);
        
        if (status == G_IO_STATUS_NORMAL && chunk_bytes_read == 1) {
            if (ch == '\n' || ch == '\r') {
                break; /* End of line */
            }
            buffer[bytes_read++] = ch;
        } else if (status == G_IO_STATUS_AGAIN) {
            /* No data available, check timeout */
            g_get_current_time(&current_time);
            glong elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                              (current_time.tv_usec - start_time.tv_usec) / 1000;
            
            if (elapsed_ms >= timeout_ms) {
                break; /* Timeout reached */
            }
            
            g_usleep(1000); /* Wait 1ms before retrying */
        } else {
            /* Error occurred */
            return FALSE;
        }
    }
    
    buffer[bytes_read] = '\0';
    return bytes_read > 0;
}

/* Flush serial port buffers */
void serial_flush(GIOChannel *channel)
{
    if (channel) {
        g_io_channel_flush(channel, NULL);
        
        /* Also flush the underlying file descriptor */
        gint fd = g_io_channel_unix_get_fd(channel);
        if (fd >= 0) {
            tcflush(fd, TCIOFLUSH);
        }
    }
}

/* Set serial port timeout */
gboolean serial_set_timeout(GIOChannel *channel, gint timeout_ms, GError **error)
{
    g_return_val_if_fail(channel != NULL, FALSE);
    
    gint fd = g_io_channel_unix_get_fd(channel);
    if (fd < 0) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Invalid file descriptor");
        return FALSE;
    }
    
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to get terminal attributes: %s", g_strerror(errno));
        return FALSE;
    }
    
    /* Convert timeout to deciseconds */
    tty.c_cc[VTIME] = (timeout_ms + 99) / 100; /* Round up to nearest decisecond */
    tty.c_cc[VMIN] = 0;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to set terminal attributes: %s", g_strerror(errno));
        return FALSE;
    }
    
    return TRUE;
}

/* Check if data is available for reading */
gboolean serial_data_available(GIOChannel *channel)
{
    if (!channel) return FALSE;
    
    GIOCondition condition;
    return g_io_channel_get_buffer_condition(channel) & G_IO_IN;
}

/* Get available serial devices */
GList *serial_get_available_devices(void)
{
    GList *devices = NULL;
    const gchar *device_patterns[] = {
        "/dev/ttyUSB*",
        "/dev/ttyACM*", 
        "/dev/ttyS*",
        "/dev/cu.*",     /* macOS */
        NULL
    };
    
    for (gint i = 0; device_patterns[i] != NULL; i++) {
        glob_t glob_result;
        
        if (glob(device_patterns[i], GLOB_NOSORT, NULL, &glob_result) == 0) {
            for (gsize j = 0; j < glob_result.gl_pathc; j++) {
                const gchar *device = glob_result.gl_pathv[j];
                
                /* Check if device exists and is accessible */
                if (g_file_test(device, G_FILE_TEST_EXISTS)) {
                    devices = g_list_prepend(devices, g_strdup(device));
                }
            }
        }
        
        globfree(&glob_result);
    }
    
    return g_list_reverse(devices);
}

/* Free device list */
void serial_free_device_list(GList *devices)
{
    g_list_free_full(devices, g_free);
}
