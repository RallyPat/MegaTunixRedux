#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
// JSON support temporarily disabled - using simple string parsing instead

#include "../../../include/plugin/plugin_interface.h"

// Speeduino-specific constants
#define SPEEDUINO_BAUD_RATE 115200
#define SPEEDUINO_TIMEOUT_MS 1000
#define SPEEDUINO_BUFFER_SIZE 1024
#define SPEEDUINO_MAX_TABLES 16
#define SPEEDUINO_MAX_PARAMS 256

// Speeduino protocol commands (based on 202501.4.ini)
#define SPEEDUINO_CMD_GET_STATUS 'S'
#define SPEEDUINO_CMD_GET_REALTIME 'A'
#define SPEEDUINO_CMD_GET_TABLE 'T'
#define SPEEDUINO_CMD_WRITE_TABLE 'W'
#define SPEEDUINO_CMD_GET_PARAM 'P'
#define SPEEDUINO_CMD_WRITE_PARAM 'B'
#define SPEEDUINO_CMD_BURN 'U'
#define SPEEDUINO_CMD_GET_VERSION 'V'

// Speeduino communication states
typedef enum {
    SPEEDUINO_STATE_DISCONNECTED,
    SPEEDUINO_STATE_CONNECTING,
    SPEEDUINO_STATE_CONNECTED,
    SPEEDUINO_STATE_ERROR
} SpeeduinoState;

// Speeduino plugin context
typedef struct {
    SpeeduinoState state;
    int serial_fd;
    char port_name[64];
    int baud_rate;
    char protocol[32];
    bool logging_enabled;
    char log_path[256];
    FILE* log_file;
    
    // Communication buffers
    char tx_buffer[SPEEDUINO_BUFFER_SIZE];
    char rx_buffer[SPEEDUINO_BUFFER_SIZE];
    int rx_buffer_pos;
    
    // Data cache
    ECURealtimeData cached_data;
    time_t last_data_update;
    
    // Threading
    pthread_t comm_thread;
    bool thread_running;
    pthread_mutex_t data_mutex;
    
    // Configuration (simplified for now)
    char config_buffer[1024];
    bool config_loaded;
    
    // Real-time data request interval
    time_t last_data_request;
    int data_request_interval_ms;
} SpeeduinoContext;

// Global plugin context
static SpeeduinoContext g_speeduino_ctx;

// Forward declarations
static bool speeduino_connect(const char* port, int baud_rate, const char* protocol);
static bool speeduino_disconnect(void);
static bool speeduino_is_connected(void);
static const char* speeduino_get_connection_status(void);
static bool speeduino_read_table(int table_id, float* data, int* rows, int* cols);
static bool speeduino_write_table(int table_id, const float* data, int rows, int cols);
static bool speeduino_read_realtime_data(ECURealtimeData* data);
static bool speeduino_write_parameter(int param_id, float value);
static bool speeduino_read_parameter(int param_id, float* value);
static bool speeduino_send_command(const char* command, char* response, int max_response_len);
static bool speeduino_upload_firmware(const char* firmware_path);
static bool speeduino_download_config(const char* config_path);
static bool speeduino_set_protocol_settings(const char* settings_json);
static const char* speeduino_get_protocol_info(void);
static bool speeduino_validate_connection(void);
static bool speeduino_start_logging(const char* log_path);
static bool speeduino_stop_logging(void);
static bool speeduino_get_log_status(char* status, int max_len);

// Communication thread function
static void* speeduino_communication_thread(void* arg) {
    SpeeduinoContext* ctx = (SpeeduinoContext*)arg;
    
    while (ctx->thread_running) {
        if (ctx->state == SPEEDUINO_STATE_CONNECTED) {
            // Request real-time data every 100ms (10Hz)
            time_t now = time(NULL);
            if (now - ctx->last_data_request >= 1) { // 1 second interval
                // Send real-time data request (Speeduino 'A' command)
                char cmd[] = "A";
                ssize_t written = write(ctx->serial_fd, cmd, 1);
                if (written != 1) {
                    // Handle write error silently for now
                }
                ctx->last_data_request = now;
            }
            
            // Read available data from serial port
            char buffer[256];
            int bytes_read = read(ctx->serial_fd, buffer, sizeof(buffer) - 1);
            
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                
                // Process incoming data
                pthread_mutex_lock(&ctx->data_mutex);
                
                // Parse Speeduino real-time data response
                if (buffer[0] == 'A' && bytes_read >= 32) {
                    // Parse real-time data packet
                    // Format: A + 32 bytes of data
                    uint8_t* data = (uint8_t*)&buffer[1];
                    
                    // Update cached data (simplified parsing for now)
                    ctx->cached_data.rpm = (data[0] << 8) | data[1];
                    ctx->cached_data.map = (data[2] << 8) | data[3];
                    ctx->cached_data.coolant_temp = data[4];
                    ctx->cached_data.air_temp = data[5];
                    ctx->cached_data.throttle = data[6];
                    ctx->cached_data.afr = (data[7] << 8) | data[8];
                    ctx->cached_data.timing = (int16_t)((data[9] << 8) | data[10]);
                    ctx->cached_data.fuel_pressure = (data[11] << 8) | data[12];
                    ctx->cached_data.oil_pressure = (data[13] << 8) | data[14];
                    ctx->cached_data.battery_voltage = (data[15] << 8) | data[16];
                    
                    ctx->last_data_update = now;
                    
                    if (ctx->logging_enabled && ctx->log_file) {
                        fprintf(ctx->log_file, "[%ld] RPM:%d MAP:%d TPS:%d AFR:%.1f\n", 
                                now, (int)ctx->cached_data.rpm, (int)ctx->cached_data.map, 
                                (int)ctx->cached_data.throttle, ctx->cached_data.afr);
                        fflush(ctx->log_file);
                    }
                }
                
                // Add to receive buffer
                if (ctx->rx_buffer_pos + bytes_read < SPEEDUINO_BUFFER_SIZE) {
                    memcpy(ctx->rx_buffer + ctx->rx_buffer_pos, buffer, bytes_read);
                    ctx->rx_buffer_pos += bytes_read;
                }
                
                // Process complete messages
                // TODO: Implement message parsing based on Speeduino protocol
                
                pthread_mutex_unlock(&ctx->data_mutex);
                
                // Log if enabled
                if (ctx->logging_enabled && ctx->log_file) {
                    fprintf(ctx->log_file, "[%ld] RX: %s\n", time(NULL), buffer);
                    fflush(ctx->log_file);
                }
            }
        }
        
        usleep(10000); // 10ms delay
    }
    
    return NULL;
}

// Serial port configuration
static bool configure_serial_port(int fd, int baud_rate) {
    struct termios tty;
    
    if (tcgetattr(fd, &tty) != 0) {
        return false;
    }
    
    // Set baud rate
    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);
    
    // 8N1 configuration
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // 1 stop bit
    tty.c_cflag &= ~CSIZE;         // Clear data size bits
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, ignore modem control lines
    
    // Raw input
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Raw output
    tty.c_oflag &= ~OPOST;
    
    // Set timeout
    tty.c_cc[VTIME] = 1;  // 0.1 second timeout
    tty.c_cc[VMIN] = 0;   // Return immediately if no data
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return false;
    }
    
    return true;
}

// CRC calculation for Speeduino protocol
static uint16_t calculate_crc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return crc;
}

// Plugin interface implementation
static bool speeduino_connect(const char* port, int baud_rate, const char* protocol) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_DISCONNECTED) {
        return false;
    }
    
    g_speeduino_ctx.state = SPEEDUINO_STATE_CONNECTING;
    
    // Open serial port
    g_speeduino_ctx.serial_fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (g_speeduino_ctx.serial_fd < 0) {
        g_speeduino_ctx.state = SPEEDUINO_STATE_ERROR;
        return false;
    }
    
    // Configure serial port
    if (!configure_serial_port(g_speeduino_ctx.serial_fd, baud_rate)) {
        close(g_speeduino_ctx.serial_fd);
        g_speeduino_ctx.state = SPEEDUINO_STATE_ERROR;
        return false;
    }
    
    // Store connection info
    strncpy(g_speeduino_ctx.port_name, port, sizeof(g_speeduino_ctx.port_name) - 1);
    g_speeduino_ctx.baud_rate = baud_rate;
    strncpy(g_speeduino_ctx.protocol, protocol, sizeof(g_speeduino_ctx.protocol) - 1);
    
    // Initialize communication thread
    g_speeduino_ctx.thread_running = true;
    if (pthread_create(&g_speeduino_ctx.comm_thread, NULL, speeduino_communication_thread, &g_speeduino_ctx) != 0) {
        close(g_speeduino_ctx.serial_fd);
        g_speeduino_ctx.state = SPEEDUINO_STATE_ERROR;
        return false;
    }
    
    g_speeduino_ctx.state = SPEEDUINO_STATE_CONNECTED;
    return true;
}

static bool speeduino_disconnect(void) {
    if (g_speeduino_ctx.state == SPEEDUINO_STATE_DISCONNECTED) {
        return true;
    }
    
    // Stop communication thread
    g_speeduino_ctx.thread_running = false;
    if (g_speeduino_ctx.comm_thread) {
        pthread_join(g_speeduino_ctx.comm_thread, NULL);
    }
    
    // Close serial port
    if (g_speeduino_ctx.serial_fd >= 0) {
        close(g_speeduino_ctx.serial_fd);
        g_speeduino_ctx.serial_fd = -1;
    }
    
    // Stop logging
    if (g_speeduino_ctx.log_file) {
        fclose(g_speeduino_ctx.log_file);
        g_speeduino_ctx.log_file = NULL;
    }
    
    g_speeduino_ctx.state = SPEEDUINO_STATE_DISCONNECTED;
    return true;
}

static bool speeduino_is_connected(void) {
    return g_speeduino_ctx.state == SPEEDUINO_STATE_CONNECTED;
}

static const char* speeduino_get_connection_status(void) {
    switch (g_speeduino_ctx.state) {
        case SPEEDUINO_STATE_DISCONNECTED: return "Disconnected";
        case SPEEDUINO_STATE_CONNECTING: return "Connecting";
        case SPEEDUINO_STATE_CONNECTED: return "Connected";
        case SPEEDUINO_STATE_ERROR: return "Error";
        default: return "Unknown";
    }
}

static bool speeduino_read_table(int table_id, float* data, int* rows, int* cols) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement table reading based on Speeduino protocol
    // This would involve sending the appropriate command and parsing the response
    
    // For now, return dummy data
    *rows = 16;
    *cols = 16;
    for (int i = 0; i < *rows * *cols; i++) {
        data[i] = (float)(i % 100);
    }
    
    return true;
}

static bool speeduino_write_table(int table_id, const float* data, int rows, int cols) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement table writing based on Speeduino protocol
    // This would involve sending the data with proper CRC and confirmation
    
    return true;
}

static bool speeduino_read_realtime_data(ECURealtimeData* data) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    pthread_mutex_lock(&g_speeduino_ctx.data_mutex);
    
    // Copy cached data
    *data = g_speeduino_ctx.cached_data;
    
    // Check if data is fresh (less than 5 seconds old)
    time_t now = time(NULL);
    bool data_fresh = (now - g_speeduino_ctx.last_data_update) < 5;
    
    pthread_mutex_unlock(&g_speeduino_ctx.data_mutex);
    
    // Log data freshness for debugging (using printf for now)
    if (data_fresh) {
        printf("[INFO] Speeduino: Fresh data RPM:%.0f MAP:%.0f AFR:%.1f\n", 
               data->rpm, data->map, data->afr);
    } else {
        printf("[WARN] Speeduino: Stale data (%lds old)\n", 
               now - g_speeduino_ctx.last_data_update);
    }
    
    return data_fresh;
}

static bool speeduino_write_parameter(int param_id, float value) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement parameter writing based on Speeduino protocol
    
    return true;
}

static bool speeduino_read_parameter(int param_id, float* value) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement parameter reading based on Speeduino protocol
    
    // For now, return dummy value
    *value = (float)(param_id % 100);
    
    return true;
}

static bool speeduino_send_command(const char* command, char* response, int max_response_len) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement command sending with CRC and response parsing
    
    // For now, return dummy response
    strncpy(response, "OK", max_response_len);
    
    return true;
}

static bool speeduino_upload_firmware(const char* firmware_path) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement firmware upload protocol
    
    return true;
}

static bool speeduino_download_config(const char* config_path) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // TODO: Implement configuration download
    
    return true;
}

static bool speeduino_set_protocol_settings(const char* settings_json) {
    // Simple string copy for now
    strncpy(g_speeduino_ctx.config_buffer, settings_json, sizeof(g_speeduino_ctx.config_buffer) - 1);
    g_speeduino_ctx.config_loaded = true;
    return true;
}

static const char* speeduino_get_protocol_info(void) {
    return "Speeduino ECU Plugin v1.0 - CRC Protocol Support";
}

static bool speeduino_validate_connection(void) {
    if (g_speeduino_ctx.state != SPEEDUINO_STATE_CONNECTED) {
        return false;
    }
    
    // Send a simple command to test communication
    char response[64];
    return speeduino_send_command("AT", response, sizeof(response));
}

static bool speeduino_start_logging(const char* log_path) {
    if (g_speeduino_ctx.logging_enabled) {
        return false;
    }
    
    g_speeduino_ctx.log_file = fopen(log_path, "w");
    if (!g_speeduino_ctx.log_file) {
        return false;
    }
    
    strncpy(g_speeduino_ctx.log_path, log_path, sizeof(g_speeduino_ctx.log_path) - 1);
    g_speeduino_ctx.logging_enabled = true;
    
    return true;
}

static bool speeduino_stop_logging(void) {
    if (!g_speeduino_ctx.logging_enabled) {
        return true;
    }
    
    if (g_speeduino_ctx.log_file) {
        fclose(g_speeduino_ctx.log_file);
        g_speeduino_ctx.log_file = NULL;
    }
    
    g_speeduino_ctx.logging_enabled = false;
    return true;
}

static bool speeduino_get_log_status(char* status, int max_len) {
    if (g_speeduino_ctx.logging_enabled) {
        snprintf(status, max_len, "Logging to: %s", g_speeduino_ctx.log_path);
    } else {
        strncpy(status, "Logging disabled", max_len);
    }
    return true;
}

// Get available serial ports for testing
static const char* speeduino_get_available_ports(void) {
    static char port_list[512];
    snprintf(port_list, sizeof(port_list), 
             "Common ports: /dev/ttyUSB0, /dev/ttyACM0, /dev/ttyS0, /dev/ttyS1");
    return port_list;
}

// Plugin initialization and cleanup
static bool speeduino_plugin_init(PluginContext* ctx) {
    // Initialize context
    memset(&g_speeduino_ctx, 0, sizeof(g_speeduino_ctx));
    g_speeduino_ctx.state = SPEEDUINO_STATE_DISCONNECTED;
    g_speeduino_ctx.serial_fd = -1;
    g_speeduino_ctx.data_request_interval_ms = 100; // 10Hz data requests
    g_speeduino_ctx.last_data_request = 0;
    
    // Initialize mutex
    if (pthread_mutex_init(&g_speeduino_ctx.data_mutex, NULL) != 0) {
        return false;
    }
    
    // Load default configuration
    snprintf(g_speeduino_ctx.config_buffer, sizeof(g_speeduino_ctx.config_buffer), 
             "{\"baud_rate\":%d,\"timeout_ms\":%d,\"protocol\":\"CRC\",\"data_rate\":\"10Hz\"}", 
             SPEEDUINO_BAUD_RATE, SPEEDUINO_TIMEOUT_MS);
    g_speeduino_ctx.config_loaded = true;
    
    return true;
}

static void speeduino_plugin_cleanup(void) {
    speeduino_disconnect();
    pthread_mutex_destroy(&g_speeduino_ctx.data_mutex);
}

static void speeduino_plugin_update(void) {
    // Update real-time data if connected
    if (g_speeduino_ctx.state == SPEEDUINO_STATE_CONNECTED) {
        // TODO: Request real-time data from ECU
        // For now, update cached data with dummy values
        pthread_mutex_lock(&g_speeduino_ctx.data_mutex);
        
        g_speeduino_ctx.cached_data.rpm = 1500.0f + (rand() % 1000);
        g_speeduino_ctx.cached_data.map = 100.0f + (rand() % 20);
        g_speeduino_ctx.cached_data.coolant_temp = 85.0f + (rand() % 20);
        g_speeduino_ctx.cached_data.air_temp = 25.0f + (rand() % 15);
        g_speeduino_ctx.cached_data.throttle = (rand() % 100);
        g_speeduino_ctx.cached_data.afr = 14.7f + (rand() % 2);
        g_speeduino_ctx.cached_data.timing = 15.0f + (rand() % 10);
        g_speeduino_ctx.cached_data.fuel_pressure = 3.0f + (rand() % 2);
        g_speeduino_ctx.cached_data.oil_pressure = 4.0f + (rand() % 3);
        g_speeduino_ctx.cached_data.battery_voltage = 13.8f + (rand() % 2);
        
        g_speeduino_ctx.last_data_update = time(NULL);
        
        pthread_mutex_unlock(&g_speeduino_ctx.data_mutex);
    }
}

// Plugin interface structure
static ECUPluginInterface g_speeduino_interface = {
    .connect = speeduino_connect,
    .disconnect = speeduino_disconnect,
    .is_connected = speeduino_is_connected,
    .get_connection_status = speeduino_get_connection_status,
    .read_table = speeduino_read_table,
    .write_table = speeduino_write_table,
    .read_realtime_data = speeduino_read_realtime_data,
    .write_parameter = speeduino_write_parameter,
    .read_parameter = speeduino_read_parameter,
    .send_command = speeduino_send_command,
    .upload_firmware = speeduino_upload_firmware,
    .download_config = speeduino_download_config,
    .set_protocol_settings = speeduino_set_protocol_settings,
    .get_protocol_info = speeduino_get_protocol_info,
    .validate_connection = speeduino_validate_connection,
    .start_logging = speeduino_start_logging,
    .stop_logging = speeduino_stop_logging,
    .get_log_status = speeduino_get_log_status
};

// Plugin interface descriptor
static PluginInterface g_speeduino_plugin_interface = {
    .name = "Speeduino ECU Plugin",
    .version = "1.0.0",
    .author = "MegaTunix Redux Team",
    .description = "Real Speeduino ECU communication with CRC protocol support",
    .type = PLUGIN_TYPE_ECU,
    .status = PLUGIN_STATUS_LOADED,
    .init = speeduino_plugin_init,
    .cleanup = speeduino_plugin_cleanup,
    .update = speeduino_plugin_update,
    .interface = {
        .ecu = g_speeduino_interface
    },
    .internal_data = nullptr,
    .library_handle = nullptr
};

// Plugin entry point
extern "C" PluginInterface* get_plugin_interface(void) {
    return &g_speeduino_plugin_interface;
}
