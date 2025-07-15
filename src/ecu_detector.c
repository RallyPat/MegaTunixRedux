/*
 * Universal ECU Detection System for MegaTunix Redux
 * 
 * This system probes serial devices to detect and identify different ECU types:
 * - Speeduino
 * - MegaSquirt 1 (MS1)
 * - MegaSquirt 2 (MS2) 
 * - MegaSquirt 3 (MS3)
 * - JimStim
 * - Other compatible ECUs
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include "ecu_detector.h"
#include "ecu_types.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

/* ECU Detection signatures and commands */
static const EcuProbeCommand probe_commands[] = {
    /* Speeduino detection */
    {
        .name = "Speeduino",
        .type = ECU_TYPE_SPEEDUINO,
        .command = 'S',
        .expected_response = "Speeduino",
        .response_min_length = 1,  // Reduced to allow binary responses
        .timeout_ms = 2000,
        .baud_rates = {115200, 57600, 38400, 19200, 9600, 0}  // 115200 first as per manual
    },
    
    /* MegaSquirt 2 detection */
    {
        .name = "MegaSquirt 2",
        .type = ECU_TYPE_MS2,
        .command = 'Q',
        .expected_response = "MegaSquirt",
        .response_min_length = 10,
        .timeout_ms = 2000,
        .baud_rates = {115200, 57600, 38400, 19200, 9600, 0}
    },
    
    /* MegaSquirt 1 detection */
    {
        .name = "MegaSquirt 1",
        .type = ECU_TYPE_MS1,
        .command = 'Q',
        .expected_response = "MS1",
        .response_min_length = 3,
        .timeout_ms = 2000,
        .baud_rates = {9600, 19200, 38400, 57600, 0}
    },
    
    /* MegaSquirt 3 detection */
    {
        .name = "MegaSquirt 3",
        .type = ECU_TYPE_MS3,
        .command = 'Q',
        .expected_response = "MS3",
        .response_min_length = 3,
        .timeout_ms = 2000,
        .baud_rates = {115200, 57600, 38400, 19200, 9600, 0}
    },
    
    /* JimStim detection */
    {
        .name = "JimStim",
        .type = ECU_TYPE_JIMSTIM,
        .command = 'Q',
        .expected_response = "JimStim",
        .response_min_length = 7,
        .timeout_ms = 2000,
        .baud_rates = {115200, 57600, 38400, 19200, 9600, 0}
    }
};

static const gint num_probe_commands = sizeof(probe_commands) / sizeof(probe_commands[0]);

/* Serial device paths to check */
static const gchar *serial_device_paths[] = {
    "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3",
    "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3",
    "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3",
    "/dev/ttyAMA0", "/dev/ttyAMA1",  /* Raspberry Pi */
    NULL
};

/* Configure serial port for ECU communication */
static gboolean configure_serial_port(gint fd, gint baud_rate)
{
    struct termios tty;
    
    if (tcgetattr(fd, &tty) != 0) {
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
    
    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);
    
    /* Configure for raw mode (8N1) */
    tty.c_cflag &= ~PARENB;   /* No parity */
    tty.c_cflag &= ~CSTOPB;   /* 1 stop bit */
    tty.c_cflag &= ~CSIZE;    /* Clear size bits */
    tty.c_cflag |= CS8;       /* 8 data bits */
    tty.c_cflag &= ~CRTSCTS;  /* No hardware flow control */
    
    tty.c_cflag |= CREAD | CLOCAL;  /* Enable receiver, ignore modem control lines */
    
    /* Configure input mode */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* No software flow control */
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Raw input */
    
    /* Configure output mode */
    tty.c_oflag &= ~OPOST; /* Raw output */
    
    /* Configure control characters */
    tty.c_cc[VMIN] = 0;   /* Non-blocking read */
    tty.c_cc[VTIME] = 5;  /* 0.5 second timeout */
    
    /* Apply settings */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        g_warning("Error setting terminal attributes: %s", strerror(errno));
        return FALSE;
    }
    
    return TRUE;
}

/* Send command and wait for response */
static gint send_command_and_read(gint fd, gchar command, gchar *buffer, gint buffer_size, gint timeout_ms)
{
    /* Send command */
    if (write(fd, &command, 1) != 1) {
        g_warning("Failed to send command 0x%02X: %s", command, strerror(errno));
        return -1;
    }
    
    /* Wait for response */
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;
    
    gint bytes_read = 0;
    gint total_bytes = 0;
    
    while (total_bytes < buffer_size - 1) {
        gint result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (result == 0) {
            /* Timeout */
            break;
        } else if (result < 0) {
            g_warning("Select error: %s", strerror(errno));
            break;
        }
        
        bytes_read = read(fd, buffer + total_bytes, buffer_size - total_bytes - 1);
        if (bytes_read <= 0) {
            break;
        }
        
        total_bytes += bytes_read;
        
        /* Check if we have a complete response */
        if (total_bytes > 0 && (buffer[total_bytes - 1] == '\n' || buffer[total_bytes - 1] == '\r')) {
            break;
        }
    }
    
    buffer[total_bytes] = '\0';
    return total_bytes;
}

/* Probe a specific device/baud combination with a command */
static gboolean probe_device_with_command(const gchar *device_path, gint baud_rate, 
                                         const EcuProbeCommand *probe, 
                                         EcuDetectionResult *result)
{
    gint fd;
    gchar buffer[256];
    gint bytes_read;
    
    g_message("Probing %s at %d baud for %s...", device_path, baud_rate, probe->name);
    
    /* Open serial port */
    fd = open(device_path, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        g_message("Failed to open %s: %s", device_path, strerror(errno));
        return FALSE;
    }
    
    g_message("Successfully opened %s", device_path);
    
    /* Configure serial port */
    if (!configure_serial_port(fd, baud_rate)) {
        g_message("Failed to configure %s at %d baud", device_path, baud_rate);
        close(fd);
        return FALSE;
    }
    
    g_message("Successfully configured %s at %d baud", device_path, baud_rate);
    
    /* Wait for device to settle */
    g_usleep(100000); /* 100ms */
    
    /* Send probe command */
    g_message("Sending probe command 0x%02X to %s...", probe->command, device_path);
    bytes_read = send_command_and_read(fd, probe->command, buffer, sizeof(buffer), probe->timeout_ms);
    
    g_message("Probe command completed, got %d bytes", bytes_read);
    
    close(fd);
    
    if (bytes_read < probe->response_min_length) {
        return FALSE;
    }
    
    /* Check if response matches expected pattern */
    if (strstr(buffer, probe->expected_response) != NULL) {
        /* Found match! */
        result->ecu_type = probe->type;
        result->device_path = g_strdup(device_path);
        result->baud_rate = baud_rate;
        result->ecu_name = g_strdup(probe->name);
        result->signature = g_strdup(buffer);
        result->confidence = 100; /* Exact match */
        
        return TRUE;
    }
    
    /* SPECIAL CASE: Arduino-based devices (ACM) might send binary data instead of text */
    if (probe->type == ECU_TYPE_SPEEDUINO && strstr(device_path, "ACM") != NULL && bytes_read > 0) {
        g_message("Found Arduino device at %s with binary response (%d bytes), assuming Speeduino", device_path, bytes_read);
        result->ecu_type = probe->type;
        result->device_path = g_strdup(device_path);
        result->baud_rate = baud_rate;
        result->ecu_name = g_strdup("Speeduino (Arduino-based)");
        result->signature = g_strdup_printf("Binary response from %s", device_path);
        result->confidence = 80; /* Good confidence for Arduino devices */
        
        return TRUE;
    }
    
    /* TEMPORARY FIX: If we get any response for Speeduino detection, consider it a match */
    if (probe->type == ECU_TYPE_SPEEDUINO && bytes_read > 0) {
        g_message("Found potential Speeduino with non-text response (%d bytes), treating as match", bytes_read);
        result->ecu_type = probe->type;
        result->device_path = g_strdup(device_path);
        result->baud_rate = baud_rate;
        result->ecu_name = g_strdup("Speeduino (Non-standard response)");
        result->signature = g_strdup_printf("Binary response: %d bytes", bytes_read);
        result->confidence = 75; /* Lower confidence for non-standard response */
        
        return TRUE;
    }
    
    return FALSE;
}

/* Scan all serial devices for ECUs */
GList *ecu_detector_scan_all_devices(void)
{
    GList *detected_ecus = NULL;
    
    g_message("Starting ECU detection scan...");
    
    /* Check each serial device */
    for (gint dev_idx = 0; serial_device_paths[dev_idx] != NULL; dev_idx++) {
        const gchar *device_path = serial_device_paths[dev_idx];
        
        /* Skip if device doesn't exist */
        if (!g_file_test(device_path, G_FILE_TEST_EXISTS)) {
            continue;
        }
        
        g_message("Probing device: %s", device_path);
        
        /* Try each probe command */
        for (gint cmd_idx = 0; cmd_idx < num_probe_commands; cmd_idx++) {
            const EcuProbeCommand *probe = &probe_commands[cmd_idx];
            
            /* Try each baud rate for this command */
            for (gint baud_idx = 0; probe->baud_rates[baud_idx] != 0; baud_idx++) {
                gint baud_rate = probe->baud_rates[baud_idx];
                
                EcuDetectionResult *result = g_new0(EcuDetectionResult, 1);
                
                if (probe_device_with_command(device_path, baud_rate, probe, result)) {
                    g_message("✅ Detected %s at %s (%d baud): %s", 
                             result->ecu_name, result->device_path, result->baud_rate, 
                             result->signature);
                    
                    detected_ecus = g_list_prepend(detected_ecus, result);
                    
                    /* Found ECU on this device, move to next device */
                    goto next_device;
                } else {
                    g_free(result);
                }
            }
        }
        
        next_device:
        continue;
    }
    
    /* Reverse list to maintain order */
    detected_ecus = g_list_reverse(detected_ecus);
    
    g_message("ECU detection scan complete. Found %d ECU(s)", g_list_length(detected_ecus));
    
    return detected_ecus;
}

/* Get user-friendly ECU type name */
const gchar *ecu_detector_get_type_name(EcuType type)
{
    switch (type) {
        case ECU_TYPE_SPEEDUINO:  return "Speeduino";
        case ECU_TYPE_MS1:        return "MegaSquirt 1";
        case ECU_TYPE_MS2:        return "MegaSquirt 2";
        case ECU_TYPE_MS3:        return "MegaSquirt 3";
        case ECU_TYPE_JIMSTIM:    return "JimStim";
        case ECU_TYPE_UNKNOWN:    return "Unknown";
        default:                  return "Unknown";
    }
}

/* Free detection result */
void ecu_detection_result_free(EcuDetectionResult *result)
{
    if (result) {
        g_free(result->device_path);
        g_free(result->ecu_name);
        g_free(result->signature);
        g_free(result);
    }
}

/* Free list of detection results */
void ecu_detector_free_results(GList *results)
{
    g_list_free_full(results, (GDestroyNotify)ecu_detection_result_free);
}

/* Get the best ECU from detection results (highest confidence, preferred types) */
EcuDetectionResult *ecu_detector_get_best_ecu(GList *results)
{
    if (!results) {
        return NULL;
    }
    
    EcuDetectionResult *best = NULL;
    gint best_score = -1;
    
    for (GList *l = results; l != NULL; l = l->next) {
        EcuDetectionResult *result = (EcuDetectionResult *)l->data;
        
        gint score = result->confidence;
        
        /* Prefer certain ECU types */
        switch (result->ecu_type) {
            case ECU_TYPE_SPEEDUINO:  score += 10; break;  /* Prefer Speeduino */
            case ECU_TYPE_MS2:        score += 8;  break;  /* Then MS2 */
            case ECU_TYPE_MS3:        score += 9;  break;  /* Then MS3 */
            case ECU_TYPE_MS1:        score += 5;  break;  /* Then MS1 */
            case ECU_TYPE_JIMSTIM:    score += 3;  break;  /* JimStim is simulator */
            default:                  break;
        }
        
        if (score > best_score) {
            best_score = score;
            best = result;
        }
    }
    
    return best;
}
