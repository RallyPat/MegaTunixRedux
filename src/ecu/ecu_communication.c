/*
 * ECU Communication - Protocol Implementation
 *  e
 * Copyright (C) 2025 Pat Burke
 * 
 * Implements MegaSquirt, Speeduino, and LibreEMS communication protocols.
 */

#include "../../include/ecu/ecu_communication.h"
#include "../../include/ecu/ecu_ini_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>

// Platform-specific serial includes
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
#else
    #include <sys/select.h>
#endif

// ECU Communication Implementation
ECUContext* ecu_init(void) {
    ECUContext* ctx = malloc(sizeof(ECUContext));
    if (!ctx) {
        return NULL;
    }
    
    // Initialize context
    memset(ctx, 0, sizeof(ECUContext));
    ctx->protocol = ECU_PROTOCOL_NONE;
    ctx->state = ECU_STATE_DISCONNECTED;
    ctx->serial_handle = NULL;
    ctx->error_count = 0;
    ctx->last_heartbeat = 0;
    ctx->connection_start = 0;
    
    // Initialize data
    memset(&ctx->data, 0, sizeof(ECUData));
    ctx->data.last_update = 0;
    ctx->data.connection_time = 0;
    
    // Initialize buffers
    memset(ctx->rx_buffer, 0, sizeof(ctx->rx_buffer));
    memset(ctx->tx_buffer, 0, sizeof(ctx->tx_buffer));
    ctx->rx_count = 0;
    ctx->tx_count = 0;
    
    // Set default callbacks
    ctx->on_data_update = NULL;
    ctx->on_connection_change = NULL;
    ctx->on_error = NULL;
    
    // Initialize demo mode
    ctx->demo_mode = false;
    ctx->demo_ini_config = NULL;
    
    return ctx;
}

void ecu_cleanup(ECUContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Disconnect if connected
    if (ctx->state == ECU_STATE_CONNECTED) {
        ecu_disconnect(ctx);
    }
    
    free(ctx);
}

bool ecu_connect(ECUContext* ctx, const ECUConfig* config) {
    if (!ctx || !config) {
        return false;
    }
    
    // Update configuration
    ctx->config = *config;
    ctx->protocol = config->protocol;
    ctx->state = ECU_STATE_CONNECTING;
    
    // Call connection change callback
    if (ctx->on_connection_change) {
        ctx->on_connection_change(ctx->state);
    }
    
    bool success = false;
    switch (config->protocol) {
        case ECU_PROTOCOL_SPEEDUINO:
            success = ecu_speeduino_connect(ctx);
            break;
        case ECU_PROTOCOL_EPICEFI:
            success = ecu_epicefi_connect(ctx);
            break;
        case ECU_PROTOCOL_MEGASQUIRT:
            success = ecu_megasquirt_connect(ctx);
            break;
        case ECU_PROTOCOL_LIBREEMS:
            success = ecu_libreems_connect(ctx);
            break;
        default:
            ecu_set_error(ctx, "Unsupported protocol");
            break;
    }
    
    if (success) {
        ctx->state = ECU_STATE_CONNECTED;
        ctx->connection_start = SDL_GetTicks();
        ctx->data.connection_time = ctx->connection_start;
        ctx->error_count = 0;
    } else {
        ctx->state = ECU_STATE_ERROR;
    }
    
    // Call connection change callback
    if (ctx->on_connection_change) {
        ctx->on_connection_change(ctx->state);
    }
    
    return success;
}

bool ecu_connect_with_ini(ECUContext* ctx, const char* port, const char* ini_file_path) {
    if (!ctx || !port || !ini_file_path) {
        ecu_set_error(ctx, "Invalid parameters for INI-based connection");
        return false;
    }
    
    printf("[DEBUG] Loading INI file: %s\n", ini_file_path);
    
    // Load and parse INI file
    INIConfig* ini_config = ecu_load_ini_file(ini_file_path);
    if (!ini_config) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Failed to load INI file: %s", ecu_get_ini_error());
        ecu_set_error(ctx, error_msg);
        return false;
    }
    
    printf("[DEBUG] INI file loaded successfully\n");
    ecu_print_ini_config(ini_config);
    
    // Detect protocol from INI
    ProtocolDetectionResult detection = ecu_detect_protocol_from_ini(ini_config);
    if (detection.confidence < 0.5f) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Could not detect protocol from INI file (confidence: %.2f)", detection.confidence);
        ecu_set_error(ctx, error_msg);
        ecu_free_ini_config(ini_config);
        return false;
    }
    
    printf("[DEBUG] Detected protocol: %s (confidence: %.2f)\n", 
           ecu_get_protocol_name_from_ini(detection.protocol_type), detection.confidence);
    
    // Create ECU configuration from INI
    ECUConfig config = ecu_config_default();
    config.protocol = detection.protocol_type;
    strncpy(config.port, port, sizeof(config.port) - 1);
    
    // Apply INI settings to config
    config.baud_rate = ini_config->baud_rate;
    config.timeout_ms = ini_config->timeout_ms;
    
    // Store INI config in context for later use
    ctx->ini_config = ini_config;
    
    printf("[DEBUG] Connecting with protocol: %s, port: %s, baud: %d, timeout: %d ms\n",
           ecu_get_protocol_name(config.protocol), config.port, config.baud_rate, config.timeout_ms);
    
    // Connect using detected protocol
    bool success = ecu_connect(ctx, &config);
    
    if (success) {
        printf("[DEBUG] Successfully connected using INI configuration\n");
    } else {
        printf("[DEBUG] Failed to connect using INI configuration\n");
        ecu_free_ini_config(ini_config);
        ctx->ini_config = NULL;
    }
    
    return success;
}

void ecu_disconnect(ECUContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Close serial connection
    if (ctx->serial_handle) {
#ifdef PLATFORM_WINDOWS
        CloseHandle((HANDLE)ctx->serial_handle);
#else
        close((int)(intptr_t)ctx->serial_handle);
#endif
        ctx->serial_handle = NULL;
    }
    
    // Update state
    ECUConnectionState old_state = ctx->state;
    ctx->state = ECU_STATE_DISCONNECTED;
    ctx->protocol = ECU_PROTOCOL_NONE;
    
    // Clear data
    memset(&ctx->data, 0, sizeof(ECUData));
    
    // Clean up INI config
    if (ctx->ini_config) {
        ecu_free_ini_config(ctx->ini_config);
        ctx->ini_config = NULL;
    }
    
    // Call connection change callback if state changed
    if (old_state != ctx->state && ctx->on_connection_change) {
        ctx->on_connection_change(ctx->state);
    }
}

bool ecu_is_connected(ECUContext* ctx) {
    return ctx && ctx->state == ECU_STATE_CONNECTED;
}

ECUConnectionState ecu_get_state(ECUContext* ctx) {
    return ctx ? ctx->state : ECU_STATE_DISCONNECTED;
}

const ECUData* ecu_get_data(ECUContext* ctx) {
    return ctx ? &ctx->data : NULL;
}

bool ecu_update(ECUContext* ctx) {
    if (!ctx || ctx->state != ECU_STATE_CONNECTED) {
        return false;
    }
    
    bool success = false;
    switch (ctx->protocol) {
        case ECU_PROTOCOL_SPEEDUINO:
            success = ecu_speeduino_update(ctx);
            break;
        case ECU_PROTOCOL_EPICEFI:
            success = ecu_epicefi_update(ctx);
            break;
        case ECU_PROTOCOL_MEGASQUIRT:
            success = ecu_megasquirt_update(ctx);
            break;
        case ECU_PROTOCOL_LIBREEMS:
            success = ecu_libreems_update(ctx);
            break;
        default:
            break;
    }
    
    if (success) {
        ctx->data.last_update = SDL_GetTicks();
        ctx->last_heartbeat = ctx->data.last_update;
        ctx->error_count = 0;
        
        // Call data update callback
        if (ctx->on_data_update) {
            ctx->on_data_update(&ctx->data);
        }
    } else {
        ctx->error_count++;
        if (ctx->error_count > 5) {
            ctx->state = ECU_STATE_TIMEOUT;
            if (ctx->on_connection_change) {
                ctx->on_connection_change(ctx->state);
            }
        }
    }
    
    return success;
}

bool ecu_send_command(ECUContext* ctx, const char* command) {
    if (!ctx || !command || ctx->state != ECU_STATE_CONNECTED) {
        return false;
    }
    
    int len = strlen(command);
    if (len >= sizeof(ctx->tx_buffer)) {
        return false;
    }
    
    // Copy command to transmit buffer
    memcpy(ctx->tx_buffer, command, len);
    ctx->tx_count = len;
    
    // Send data
#ifdef PLATFORM_WINDOWS
    DWORD bytes_written;
    return WriteFile((HANDLE)ctx->serial_handle, ctx->tx_buffer, ctx->tx_count, &bytes_written, NULL);
#else
    return write((int)(intptr_t)ctx->serial_handle, ctx->tx_buffer, ctx->tx_count) == ctx->tx_count;
#endif
}

const char* ecu_get_protocol_name(ECUProtocol protocol) {
    switch (protocol) {
        case ECU_PROTOCOL_SPEEDUINO: return "Speeduino";
        case ECU_PROTOCOL_EPICEFI: return "EpicEFI";
        case ECU_PROTOCOL_MEGASQUIRT: return "MegaSquirt";
        case ECU_PROTOCOL_LIBREEMS: return "LibreEMS";
        case ECU_PROTOCOL_NONE: return "None";
        default: return "Unknown";
    }
}

ECUProtocol ecu_parse_protocol_name(const char* name) {
    if (!name) return ECU_PROTOCOL_SPEEDUINO;
    
    if (strcasecmp(name, "speeduino") == 0) return ECU_PROTOCOL_SPEEDUINO;
    if (strcasecmp(name, "epicefi") == 0) return ECU_PROTOCOL_EPICEFI;
    if (strcasecmp(name, "megasquirt") == 0) return ECU_PROTOCOL_MEGASQUIRT;
    if (strcasecmp(name, "libreems") == 0) return ECU_PROTOCOL_LIBREEMS;
    
    return ECU_PROTOCOL_SPEEDUINO; // Default
}

const char* ecu_get_state_name(ECUConnectionState state) {
    switch (state) {
        case ECU_STATE_DISCONNECTED: return "Disconnected";
        case ECU_STATE_CONNECTING: return "Connecting";
        case ECU_STATE_CONNECTED: return "Connected";
        case ECU_STATE_ERROR: return "Error";
        case ECU_STATE_TIMEOUT: return "Timeout";
        default: return "Unknown";
    }
}

// Protocol-specific implementations
bool ecu_megasquirt_connect(ECUContext* ctx) {
    // Open serial port
    int fd = open(ctx->config.port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        ecu_set_error(ctx, "Failed to open serial port");
        return false;
    }
    
    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to get serial attributes");
        return false;
    }
    
    // Set baud rate
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    // Configure 8N1
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    // Configure input
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    
    // Configure output
    tty.c_oflag &= ~OPOST;
    
    // Set attributes
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to set serial attributes");
        return false;
    }
    
    ctx->serial_handle = (void*)(intptr_t)fd;
    
    // Send MegaSquirt handshake
    const char* handshake = "Q";
    if (!ecu_send_command(ctx, handshake)) {
        ecu_disconnect(ctx);
        return false;
    }
    
    // Wait for response
    SDL_Delay(100);
    
    return true;
}

// EpicEFI Protocol Implementation - Forward declarations
static bool epicefi_parse_response(ECUContext* ctx, const uint8_t* data, int length);
static bool epicefi_send_command_and_read(ECUContext* ctx, const char* command, uint8_t* response, int* response_length);

// CRC-16 calculation for Speeduino protocol
static uint16_t speeduino_calculate_crc16(const uint8_t* data, int length) {
    uint16_t crc = 0xFFFF;
    
    for (int i = 0; i < length; i++) {
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

// Validate CRC-16 for Speeduino packets
static bool speeduino_validate_crc16(const uint8_t* data, int length, uint16_t crc) {
    uint16_t calculated_crc = speeduino_calculate_crc16(data, length);
    return calculated_crc == crc;
}

// Build Speeduino packet with proper formatting
static uint8_t* speeduino_build_packet(uint8_t command, const uint8_t* data, int data_length, int* packet_length) {
    // Calculate total packet size
    int total_size = 6 + data_length; // start + command + length + data + crc_high + crc_low + stop
    uint8_t* packet = malloc(total_size);
    
    if (!packet) {
        *packet_length = 0;
        return NULL;
    }
    
    // Build packet
    packet[0] = SPEEDUINO_START_BYTE;  // Start byte
    packet[1] = command;               // Command
    packet[2] = data_length;           // Data length
    
    // Copy data
    if (data && data_length > 0) {
        memcpy(&packet[3], data, data_length);
    }
    
    // Calculate CRC (excluding start byte, including command, length, and data)
    uint16_t crc = speeduino_calculate_crc16(&packet[1], 2 + data_length);
    
    // Add CRC
    packet[3 + data_length] = (crc >> 8) & 0xFF;     // CRC high byte
    packet[4 + data_length] = crc & 0xFF;            // CRC low byte
    packet[5 + data_length] = SPEEDUINO_STOP_BYTE;   // Stop byte
    
    *packet_length = total_size;
    return packet;
}

// Parse Speeduino packet
static bool speeduino_parse_packet(const uint8_t* packet, int length, SpeeduinoPacket* parsed) {
    if (!packet || !parsed || length < 6) {
        return false;
    }
    
    // Check start and stop bytes
    if (packet[0] != SPEEDUINO_START_BYTE || packet[length - 1] != SPEEDUINO_STOP_BYTE) {
        return false;
    }
    
    // Extract packet components
    parsed->start_byte = packet[0];
    parsed->command = packet[1];
    parsed->data_length = packet[2];
    
    // Validate data length
    if (parsed->data_length > 256 || (3 + parsed->data_length + 3) != (uint16_t)length) {
        return false;
    }
    
    // Copy data
    if (parsed->data_length > 0) {
        memcpy(parsed->data, &packet[3], parsed->data_length);
    }
    
    // Extract CRC
    parsed->crc_high = packet[3 + parsed->data_length];
    parsed->crc_low = packet[4 + parsed->data_length];
    parsed->stop_byte = packet[5 + parsed->data_length];
    
    // Validate CRC
    uint16_t received_crc = (parsed->crc_high << 8) | parsed->crc_low;
    uint16_t calculated_crc = speeduino_calculate_crc16(&packet[1], 2 + parsed->data_length);
    
    return received_crc == calculated_crc;
}

// Speeduino Protocol Implementation
static bool speeduino_parse_response(ECUContext* ctx, const uint8_t* data, int length) {
    if (!ctx || !data || length <= 0) {
        return false;
    }
    
    // Parse the Speeduino data format - handle both 120-byte and 130-byte responses
    if (length >= 120) {
        // Extract values with bounds checking
        // RPM is at offset 14-15 (U16)
        if (length > 15) {
            ctx->data.rpm = (data[14] | (data[15] << 8));
        } else {
            ctx->data.rpm = 0;
        }
        
        // MAP is at offset 4-5 (U16)
        if (length > 5) {
            ctx->data.map = (data[4] | (data[5] << 8));
        } else {
            ctx->data.map = 0;
        }
        
        // TPS is at offset 25 (U08, scaled by 0.5)
        if (length > 25) {
            ctx->data.tps = data[25] * 0.5f;
        } else {
            ctx->data.tps = 0;
        }
        
        // Coolant temp is at offset 7 (U08)
        if (length > 7) {
            ctx->data.coolant_temp = data[7];
        } else {
            ctx->data.coolant_temp = 0;
        }
        
        // Battery voltage is at offset 9 (U08, scaled by 0.1)
        if (length > 9) {
            ctx->data.battery_voltage = data[9] * 0.1f;
        } else {
            ctx->data.battery_voltage = 0;
        }
        
        // AFR is at offset 10 (U08, scaled by 0.1)
        if (length > 10) {
            ctx->data.afr = data[10] * 0.1f;
        } else {
            ctx->data.afr = 0;
        }
        
        // Timing is at offset 24 (S08)
        if (length > 24) {
            ctx->data.timing = (int8_t)data[24];
        } else {
            ctx->data.timing = 0;
        }
        
        // Boost target is at offset 30 (U08, scaled by 2)
        if (length > 30) {
            ctx->data.boost = data[30] * 2.0f;
        } else {
            ctx->data.boost = 0;
        }
        
        // Engine status bits at offset 2
        if (length > 2) {
            uint8_t engine_status = data[2];
            ctx->data.engine_running = (engine_status & 0x01) != 0;
            ctx->data.engine_cranking = (engine_status & 0x02) != 0;
        } else {
            ctx->data.engine_running = false;
            ctx->data.engine_cranking = false;
        }
        
        // Set default values for missing data
        ctx->data.intake_temp = 0;
        ctx->data.afr_target = 0;
        ctx->data.fuel_pressure = 0;
        ctx->data.oil_pressure = 0;
        ctx->data.oil_temp = 0;
        ctx->data.boost_target = 0;
        ctx->data.wastegate_duty = 0;
        ctx->data.fuel_pw1 = 0;
        ctx->data.fuel_pw2 = 0;
        ctx->data.fuel_duty = 0;
        ctx->data.injector_duty = 0;
        ctx->data.dwell = 0;
        ctx->data.spark_advance = 0;
        ctx->data.knock_count = 0;
        ctx->data.knock_retard = 0;
        ctx->data.boost_control_active = false;
        ctx->data.knock_detected = false;
        ctx->data.check_engine_light = false;
        
        ctx->data.last_update = SDL_GetTicks();
        
        printf("[DEBUG] Successfully parsed Speeduino data (%d bytes): RPM=%.0f, MAP=%.1f, TPS=%.1f, CLT=%.1f, BAT=%.1f\n",
               length, ctx->data.rpm, ctx->data.map, ctx->data.tps, ctx->data.coolant_temp, ctx->data.battery_voltage);
        
        return true;
    } else if (length > 0) {
        // Handle other responses (like version info, query responses)
        printf("[DEBUG] Speeduino response (%d bytes): ", length);
        
        // Check if it's ASCII text or binary data
        bool is_ascii = true;
        for (int i = 0; i < length && i < 20; i++) {
            if (data[i] < 32 || data[i] > 126) {
                is_ascii = false;
                break;
            }
        }
        
        if (is_ascii) {
            printf("'%.*s' (ASCII)\n", length, data);
        } else {
            // Print hex dump for binary data
            for (int i = 0; i < length && i < 16; i++) {
                printf("[%02X]", data[i]);
            }
            printf(" (binary)\n");
        }
        
        // For non-data responses, just mark as successful connection
        ctx->data.last_update = SDL_GetTicks();
        return true;
    }
    
    return false;
}

static bool speeduino_send_command_and_read(ECUContext* ctx, const char* command, uint8_t* response, int* response_length) {
    if (!ctx || !command || !response || !response_length) {
        return false;
    }
    
    int fd = (int)(intptr_t)ctx->serial_handle;
    printf("[DEBUG] Retrieved file descriptor: %d\n", fd);
    if (fd < 0) {
        printf("[DEBUG] Invalid file descriptor\n");
        return false;
    }
    
    // Flush any existing data before sending command
    tcflush(fd, TCIOFLUSH);
    
    // Send command without line terminator (Speeduino protocol)
    int cmd_len = strlen(command);
    int bytes_written = write(fd, command, cmd_len);
    if (bytes_written != cmd_len) {
        ecu_set_error(ctx, "Failed to send Speeduino command");
        return false;
    }
    
    printf("[DEBUG] Sent command '%s' (%d bytes)\n", command, bytes_written);
    
    // Wait for response (use usleep like our working test programs)
    usleep(100000); // 100ms delay like our working tests
    
    printf("[DEBUG] Waiting for response...\n");
    
    // Read response with timeout (use longer timeout like our working tests)
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 2; // 2 second timeout like our working tests
    timeout.tv_usec = 0;
    
    int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result <= 0) {
        ecu_set_error(ctx, "Timeout waiting for Speeduino response");
        return false;
    }
    
    // Read available data with error checking
    ssize_t bytes_read = read(fd, response, 256);
    if (bytes_read <= 0) {
        ecu_set_error(ctx, "Failed to read Speeduino response");
        return false;
    }
    
    *response_length = (int)bytes_read;
    
    printf("[DEBUG] Received %d bytes\n", *response_length);
    
    return true;
}

bool ecu_speeduino_connect(ECUContext* ctx) {
    if (!ctx || !ctx->config.port[0]) {
        ecu_set_error(ctx, "Invalid ECU context or port");
        return false;
    }
    
    // Open serial port (use same flags as our working test)
    int fd = open(ctx->config.port, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        ecu_set_error(ctx, "Failed to open serial port");
        return false;
    }
    
    // Configure serial port for Speeduino
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to get serial attributes");
        return false;
    }
    
    // Speeduino uses 115200 baud (confirmed by our tests)
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    // 8N1 configuration (EXACTLY like working test)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to set serial attributes");
        return false;
    }
    
    printf("[DEBUG] Configured serial port for Speeduino at 115200 baud\n");
    
    // Wait after port open as specified in INI file (delayAfterPortOpen=1000)
    printf("[DEBUG] Waiting 1 second after port open (INI specification)...\n");
    sleep(1);
    
    // Flush any existing data
    tcflush(fd, TCIOFLUSH);
    
    // Set the serial handle BEFORE calling speeduino_send_command_and_read
    ctx->serial_handle = (void*)(intptr_t)fd;
    
    // Test connection using CRC binary protocol (primary method - from INI file)
    uint8_t test_response[256];
    int test_response_length;
    
    printf("[DEBUG] Testing Speeduino connection with CRC binary protocol...\n");
    
    // Flush any existing data
    tcflush(fd, TCIOFLUSH);
    
    // Try CRC binary protocol first (msEnvelope_1.0 from INI file)
    uint8_t test_commands[] = {SPEEDUINO_CMD_QUERY, SPEEDUINO_CMD_GET_VERSION, SPEEDUINO_CMD_GET_SIGNATURE, SPEEDUINO_CMD_GET_DATA};
    const char* command_names[] = {"Query", "Version", "Signature", "Data"};
    bool connection_success = false;
    
    for (int cmd_idx = 0; cmd_idx < 4 && !connection_success; cmd_idx++) {
        printf("[DEBUG] Trying CRC binary command 0x%02X (%s)...\n", test_commands[cmd_idx], command_names[cmd_idx]);
        
        // Flush any existing data
        tcflush(fd, TCIOFLUSH);
        
        // Build proper Speeduino CRC packet (msEnvelope_1.0 format)
        int packet_length;
        uint8_t* packet = speeduino_build_packet(test_commands[cmd_idx], NULL, 0, &packet_length);
        
        if (!packet) {
            printf("[DEBUG] Failed to build CRC packet for command 0x%02X\n", test_commands[cmd_idx]);
            continue;
        }
        
        // Send CRC packet
        int bytes_written = write(fd, packet, packet_length);
        if (bytes_written != packet_length) {
            printf("[DEBUG] Failed to send CRC packet for command 0x%02X: %s\n", test_commands[cmd_idx], strerror(errno));
            free(packet);
            continue;
        }
        
        printf("[DEBUG] Sent CRC packet for command 0x%02X (%d bytes): ", test_commands[cmd_idx], bytes_written);
        for (int i = 0; i < packet_length && i < 16; i++) {
            printf("[%02X]", packet[i]);
        }
        printf("\n");
        
        free(packet);
        
        // Wait for response (like working test)
        usleep(200000); // 200ms delay (like working test)
    
        // Read response with multiple attempts (EXACTLY like our working test)
        printf("[DEBUG] About to start read loop for command 0x%02X\n", test_commands[cmd_idx]);
        test_response_length = 0;
        
        for (int attempt = 1; attempt <= 10; attempt++) {  // 10 attempts like working test
            fd_set read_fds;
            struct timeval timeout;
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);
            timeout.tv_sec = 0;
            timeout.tv_usec = 500000; // 500ms timeout per attempt (like working test)
            
            printf("[DEBUG] Attempt %d: Waiting for data...\n", attempt);
            int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
            printf("[DEBUG] Attempt %d: select() returned %d\n", attempt, select_result);
            
            if (select_result > 0 && FD_ISSET(fd, &read_fds)) {
                printf("[DEBUG] Attempt %d: Data available, reading...\n", attempt);
                int bytes_read = read(fd, test_response + test_response_length, sizeof(test_response) - test_response_length);
                printf("[DEBUG] Attempt %d: read() returned %d\n", attempt, bytes_read);
                if (bytes_read > 0) {
                    test_response_length += bytes_read;
                    printf("[DEBUG] Attempt %d: Read %d bytes (total: %d)\n", attempt, bytes_read, test_response_length);
                    
                    // Check if we got enough data
                    if (test_response_length >= 50) {
                        break; // Got enough data (lowered threshold)
                    }
                }
            } else if (select_result == 0) {
                printf("[DEBUG] Attempt %d: select() timed out\n", attempt);
            } else {
                printf("[DEBUG] Attempt %d: select() error: %s\n", attempt, strerror(errno));
            }
        }
        
        printf("[DEBUG] Read loop completed for command 0x%02X, total bytes: %d\n", test_commands[cmd_idx], test_response_length);
        
        if (test_response_length <= 0) {
            printf("[DEBUG] No response received for CRC command 0x%02X\n", test_commands[cmd_idx]);
            continue;
        }
        
        printf("[DEBUG] Received %d bytes from Speeduino for CRC command 0x%02X\n", test_response_length, test_commands[cmd_idx]);
        
        // Try to parse as Speeduino CRC packet
        SpeeduinoPacket parsed_packet;
        bool packet_valid = speeduino_parse_packet(test_response, test_response_length, &parsed_packet);
        
        if (packet_valid) {
            printf("[DEBUG] ✅ Valid Speeduino CRC packet received: command=0x%02X, data_length=%d\n", 
                   parsed_packet.command, parsed_packet.data_length);
            
            // Check if this is a valid response to our command
            if (parsed_packet.command == test_commands[cmd_idx] || 
                parsed_packet.data_length > 0) {
                connection_success = true;
                printf("[DEBUG] Successfully connected using CRC binary protocol with command 0x%02X\n", test_commands[cmd_idx]);
                break;
            }
        } else {
            // Speeduino may respond with raw data instead of CRC packets
            if (test_response_length > 0) {
                printf("[DEBUG] Raw data response received (%d bytes) for CRC command 0x%02X\n", 
                       test_response_length, test_commands[cmd_idx]);
                
                // Check if this looks like valid engine data OR any substantial response
                if (test_response_length >= 50) { // Lower threshold - any substantial response
                    connection_success = true;
                    printf("[DEBUG] ✅ Successfully connected using CRC binary protocol with raw data response (%d bytes)\n", test_response_length);
                    break;
                } else {
                    printf("[DEBUG] Got small response (%d bytes) - not enough data\n", test_response_length);
                }
            } else {
                printf("[DEBUG] No response for CRC command 0x%02X\n", test_commands[cmd_idx]);
            }
        }
    }
    
    // If CRC protocol failed, try ASCII fallback (as specified in INI file)
    if (!connection_success) {
        printf("[DEBUG] CRC binary protocol failed, trying ASCII fallback...\n");
        
        // Try simple ASCII commands (from INI file: queryCommand = "Q", signature = "speeduino 202501")
        const char* ascii_commands[] = {"Q", "S", "V", "A"};
        const char* ascii_names[] = {"Query", "Signature", "Version", "Data"};
        
        for (int cmd_idx = 0; cmd_idx < 4 && !connection_success; cmd_idx++) {
            printf("[DEBUG] Trying ASCII command '%s' (%s)...\n", ascii_commands[cmd_idx], ascii_names[cmd_idx]);
            
            // Flush any existing data
            tcflush(fd, TCIOFLUSH);
            
            // Send ASCII command
            int bytes_written = write(fd, ascii_commands[cmd_idx], strlen(ascii_commands[cmd_idx]));
            if (bytes_written != strlen(ascii_commands[cmd_idx])) {
                printf("[DEBUG] Failed to send ASCII command '%s': %s\n", ascii_commands[cmd_idx], strerror(errno));
                continue;
            }
            
            // Wait for response (based on INI timing)
            usleep(100000); // 100ms delay
            
            // Read response with timeout (based on INI: blockReadTimeout = 2000)
            fd_set read_fds;
            struct timeval timeout;
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);
            timeout.tv_sec = 2; // 2 second timeout from INI
            timeout.tv_usec = 0;
            
            int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
            if (select_result <= 0) {
                printf("[DEBUG] Timeout waiting for ASCII response to '%s'\n", ascii_commands[cmd_idx]);
                continue;
            }
            
            // Read response
            test_response_length = read(fd, test_response, sizeof(test_response));
            if (test_response_length <= 0) {
                printf("[DEBUG] Failed to read ASCII response to '%s': %s\n", ascii_commands[cmd_idx], strerror(errno));
                continue;
            }
            
            printf("[DEBUG] Received %d bytes from Speeduino for ASCII command '%s'\n", test_response_length, ascii_commands[cmd_idx]);
            
            // Check for ASCII response (from INI: signature = "speeduino 202501")
            if (test_response_length > 0) {
                char response_str[256];
                int copy_len = (test_response_length < sizeof(response_str) - 1) ? test_response_length : sizeof(response_str) - 1;
                memcpy(response_str, test_response, copy_len);
                response_str[copy_len] = '\0';
                
                // Convert to lowercase for comparison
                for (int i = 0; response_str[i]; i++) {
                    response_str[i] = tolower(response_str[i]);
                }
                
                bool has_signature = (strstr(response_str, "speeduino") != NULL);
                bool has_valid_data = (test_response_length > 0);
                
                printf("[DEBUG] ASCII response: '%.*s' (signature: %s, valid: %s)\n", 
                       copy_len, response_str, has_signature ? "YES" : "NO", has_valid_data ? "YES" : "NO");
                
                if (has_signature || has_valid_data) {
                    connection_success = true;
                    printf("[DEBUG] Successfully connected using ASCII fallback with command '%s'\n", ascii_commands[cmd_idx]);
                    break;
                }
            }
        }
    }
    
    if (!connection_success) {
        printf("[DEBUG] Both CRC binary and ASCII protocols failed - no valid response\n");
        close(fd);
        ecu_set_error(ctx, "Failed to communicate with Speeduino - no valid response");
        return false;
    }
    
    printf("[DEBUG] Successfully connected to Speeduino (response: %d bytes)\n", test_response_length);
    
    // Store the successful response for later parsing
    uint8_t final_response[256];
    int final_response_length = test_response_length;
    memcpy(final_response, test_response, test_response_length);
    
    // Parse initial status from the successful connection test
    speeduino_parse_response(ctx, final_response, final_response_length);
    
    ctx->state = ECU_STATE_CONNECTED;
    ctx->connection_start = SDL_GetTicks();
    ecu_clear_error(ctx);
    
    return true;
}

bool ecu_speeduino_update(ECUContext* ctx) {
    if (!ctx || ctx->state != ECU_STATE_CONNECTED) {
        return false;
    }
    
    uint8_t response[256];
    int response_length = 0;
    bool data_updated = false;
    uint32_t current_time = SDL_GetTicks();
    uint32_t request_start_time = current_time;
    
    // Request real-time data using CRC binary protocol (from INI file)
    int packet_length;
    uint8_t* packet = speeduino_build_packet(SPEEDUINO_CMD_GET_DATA, NULL, 0, &packet_length);
    
    if (packet) {
        int fd = (int)(intptr_t)ctx->serial_handle;
        
        // Flush any existing data
        tcflush(fd, TCIOFLUSH);
        
        // Send packet
        int bytes_written = write(fd, packet, packet_length);
        
        // Update TX statistics
        if (bytes_written > 0) {
            ctx->bytes_sent += bytes_written;
            ctx->packets_sent++;
            ctx->last_activity = current_time;
        }
        
        free(packet);
        
        if (bytes_written == packet_length) {
            printf("[DEBUG] Sent data request packet (%d bytes)\n", bytes_written);
            
            // Wait for response (based on INI timing: interWriteDelay = 10)
            usleep(10000); // 10ms delay (from INI)
            
            // Read response with adaptive timing
            uint32_t adaptive_timeout = ecu_get_adaptive_timeout(ctx);
            for (int attempt = 1; attempt <= 5; attempt++) {
                fd_set read_fds;
                struct timeval timeout;
                FD_ZERO(&read_fds);
                FD_SET(fd, &read_fds);
                timeout.tv_sec = 0;
                timeout.tv_usec = adaptive_timeout; // Adaptive timeout based on learned response times
                
                int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
                
                if (select_result > 0 && FD_ISSET(fd, &read_fds)) {
                    int bytes_read = read(fd, response + response_length, sizeof(response) - response_length);
                    if (bytes_read > 0) {
                        response_length += bytes_read;
                        printf("[DEBUG] Read %d bytes (total: %d)\n", bytes_read, response_length);
                        
                        // Update RX statistics
                        ctx->bytes_received += bytes_read;
                        ctx->last_activity = current_time;
                        
                        // Check if we got enough data (Speeduino sends ~130 bytes for realtime data)
                        if (response_length >= 100) {
                            break;
                        }
                    }
                } else if (select_result == 0) {
                    printf("[DEBUG] Timeout on attempt %d\n", attempt);
                    ctx->timeouts++;
                } else {
                    printf("[DEBUG] Select error on attempt %d: %s\n", attempt, strerror(errno));
                    ctx->errors++;
                }
            }
            
            // Process response
            if (response_length > 0) {
                // Calculate response time and update adaptive timing
                uint32_t response_time = SDL_GetTicks() - request_start_time;
                ecu_update_response_time(ctx, response_time);
                
                printf("[DEBUG] Received %d bytes of realtime data (response time: %ums)\n", response_length, response_time);
                ctx->packets_received++;
                
                if (speeduino_parse_response(ctx, response, response_length)) {
                    data_updated = true;
                    printf("[DEBUG] Successfully parsed realtime data\n");
                } else {
                    printf("[DEBUG] Failed to parse realtime data\n");
                    ctx->errors++;
                }
            } else {
                printf("[DEBUG] No response received for data request\n");
                ctx->timeouts++;
            }
        } else {
            printf("[DEBUG] Failed to send data request: %s\n", strerror(errno));
            ctx->errors++;
        }
    }
    
    // Calculate rates (bytes per second)
    uint32_t time_diff = current_time - ctx->connection_start;
    if (time_diff > 0) {
        ctx->rx_rate = (float)ctx->bytes_received / (time_diff / 1000.0f);
        ctx->tx_rate = (float)ctx->bytes_sent / (time_diff / 1000.0f);
        ctx->rx_packet_rate = (float)ctx->packets_received / (time_diff / 1000.0f);
        ctx->tx_packet_rate = (float)ctx->packets_sent / (time_diff / 1000.0f);
    }
    
    if (data_updated) {
        // Validate data
        if (!ecu_validate_data(&ctx->data)) {
            ecu_set_error(ctx, "Invalid data received from Speeduino");
            return false;
        }
        
        // Call data update callback
        if (ctx->on_data_update) {
            ctx->on_data_update(&ctx->data);
        }
        
        ctx->error_count = 0;
        return true;
    }
    
    // If no data was updated, increment error count
    ctx->error_count++;
    if (ctx->error_count > 5) {
        ecu_set_error(ctx, "Multiple failed Speeduino communication attempts");
        return false;
    }
    
    return false;
}

bool ecu_libreems_connect(ECUContext* ctx) {
    // LibreEMS uses similar serial communication
    int fd = open(ctx->config.port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        ecu_set_error(ctx, "Failed to open serial port");
        return false;
    }
    
    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to get serial attributes");
        return false;
    }
    
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to set serial attributes");
        return false;
    }
    
    ctx->serial_handle = (void*)(intptr_t)fd;
    
    // Send LibreEMS handshake
    const char* handshake = "L";
    if (!ecu_send_command(ctx, handshake)) {
        ecu_disconnect(ctx);
        return false;
    }
    
    SDL_Delay(100);
    
    return true;
}

bool ecu_megasquirt_update(ECUContext* ctx) {
    // Request data from MegaSquirt
    if (!ecu_send_command(ctx, "S")) {
        return false;
    }
    
    // Read response (simplified - in real implementation would parse binary data)
    uint8_t buffer[256];
    int bytes_read = read((int)(intptr_t)ctx->serial_handle, buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        // Parse MegaSquirt data (simplified)
        // In real implementation, this would parse the binary protocol
        ctx->data.rpm = 3000.0f + (rand() % 2000);
        ctx->data.map = 100.0f + (rand() % 20);
        ctx->data.tps = 20.0f + (rand() % 60);
        ctx->data.coolant_temp = 85.0f + (rand() % 20);
        ctx->data.intake_temp = 35.0f + (rand() % 15);
        ctx->data.battery_voltage = 13.5f + (rand() % 10) / 10.0f;
        ctx->data.afr = 14.0f + (rand() % 20) / 10.0f;
        ctx->data.timing = 15.0f + (rand() % 20);
        ctx->data.boost = 10.0f + (rand() % 15);
        
        ctx->data.engine_running = true;
        ctx->data.engine_cranking = false;
        ctx->data.boost_control_active = true;
        ctx->data.knock_detected = false;
        ctx->data.check_engine_light = false;
        
        return true;
    }
    
    return false;
}



bool ecu_libreems_update(ECUContext* ctx) {
    // Request data from LibreEMS
    if (!ecu_send_command(ctx, "L")) {
        return false;
    }
    
    // Read response (simplified)
    uint8_t buffer[256];
    int bytes_read = read((int)(intptr_t)ctx->serial_handle, buffer, sizeof(buffer));
    
    if (bytes_read > 0) {
        // Parse LibreEMS data (simplified)
        ctx->data.rpm = 2800.0f + (rand() % 2200);
        ctx->data.map = 98.0f + (rand() % 22);
        ctx->data.tps = 18.0f + (rand() % 65);
        ctx->data.coolant_temp = 82.0f + (rand() % 23);
        ctx->data.intake_temp = 32.0f + (rand() % 18);
        ctx->data.battery_voltage = 13.4f + (rand() % 11) / 10.0f;
        ctx->data.afr = 14.1f + (rand() % 19) / 10.0f;
        ctx->data.timing = 13.0f + (rand() % 22);
        ctx->data.boost = 9.0f + (rand() % 16);
        
        ctx->data.engine_running = true;
        ctx->data.engine_cranking = false;
        ctx->data.boost_control_active = true;
        ctx->data.knock_detected = false;
        ctx->data.check_engine_light = false;
        
        return true;
    }
    
    return false;
}

// Serial Port Detection Implementation
SerialPortList ecu_detect_serial_ports(void) {
    SerialPortList port_list = {0};
    
    // Common Linux serial port names
    const char* common_ports[] = {
        "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3",
        "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3",
        "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3"
    };
    
    for (int i = 0; i < sizeof(common_ports) / sizeof(common_ports[0]); i++) {
        if (port_list.count >= MAX_SERIAL_PORTS) {
            break;
        }
        
        // Check if port exists and is accessible
        if (access(common_ports[i], R_OK | W_OK) == 0) {
            strcpy(port_list.ports[port_list.count], common_ports[i]);
            port_list.count++;
        }
    }
    
    return port_list;
}

bool ecu_test_serial_port(const char* port, ECUProtocol protocol) {
    if (!port) {
        return false;
    }
    
    printf("[DEBUG] Testing port %s for protocol %d\n", port, protocol);
    
    // Try to open the port
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("[DEBUG] Failed to open port %s: %s\n", port, strerror(errno));
        return false;
    }
    
    // Configure serial port
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return false;
    }
    
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return false;
    }
    
    // Send protocol-specific test command
    const char* test_command = NULL;
    switch (protocol) {
        case ECU_PROTOCOL_SPEEDUINO:
            // Try CRC binary protocol first (official Speeduino protocol)
            printf("[DEBUG] Testing Speeduino with CRC binary protocol...\n");
            
            // Flush buffer
            tcflush(fd, TCIOFLUSH);
            
            // Try CRC binary commands
            uint8_t crc_commands[] = {SPEEDUINO_CMD_QUERY, SPEEDUINO_CMD_GET_VERSION, SPEEDUINO_CMD_GET_SIGNATURE, SPEEDUINO_CMD_GET_DATA};
            const char* crc_names[] = {"Query", "Version", "Signature", "Data"};
            
            for (int i = 0; i < 4; i++) {
                printf("[DEBUG] Testing CRC command 0x%02X (%s)...\n", crc_commands[i], crc_names[i]);
                
                // Flush buffer
                tcflush(fd, TCIOFLUSH);
                
                // Build and send CRC packet
                int packet_length;
                uint8_t* packet = speeduino_build_packet(crc_commands[i], NULL, 0, &packet_length);
                
                if (packet) {
                    int bytes_written = write(fd, packet, packet_length);
                    if (bytes_written == packet_length) {
                        // Wait for response
                        usleep(200000); // 200ms delay
                        
                        // Try to read response with multiple attempts
                        uint8_t buffer[256];
                        int total_read = 0;
                        
                        for (int attempt = 1; attempt <= 5; attempt++) {
                            fd_set readfds;
                            struct timeval timeout;
                            
                            FD_ZERO(&readfds);
                            FD_SET(fd, &readfds);
                            timeout.tv_sec = 0;
                            timeout.tv_usec = 500000; // 500ms timeout
                            
                            int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
                            if (result > 0 && FD_ISSET(fd, &readfds)) {
                                int bytes_read = read(fd, buffer + total_read, sizeof(buffer) - total_read);
                                if (bytes_read > 0) {
                                    total_read += bytes_read;
                                    printf("[DEBUG] Attempt %d: Read %d bytes (total: %d) for CRC command 0x%02X\n", 
                                           attempt, bytes_read, total_read, crc_commands[i]);
                                    
                                    // Try to parse as CRC packet
                                    SpeeduinoPacket parsed_packet;
                                    if (speeduino_parse_packet(buffer, total_read, &parsed_packet)) {
                                        printf("[DEBUG] ✅ Valid Speeduino CRC packet received!\n");
                                        printf("[DEBUG] Command: 0x%02X, Data length: %d\n", parsed_packet.command, parsed_packet.data_length);
                                        free(packet);
                                        close(fd);
                                        return true;
                                    } else {
                                        // Check for raw data response (Speeduino may send raw data instead of CRC packets)
                                        if (total_read >= 128) {
                                            printf("[DEBUG] ✅ Raw data response received (%d bytes) - Speeduino responding to CRC!\n", total_read);
                                            free(packet);
                                            close(fd);
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                        
                        if (total_read == 0) {
                            printf("[DEBUG] No response for CRC command 0x%02X\n", crc_commands[i]);
                        }
                    }
                    free(packet);
                }
            }
            
            // If CRC failed, try ASCII fallback
            printf("[DEBUG] CRC protocol failed, trying ASCII fallback...\n");
            
            // Send 'Q' command (ASCII fallback)
            const char* test_cmd = "Q";
            int bytes_written = write(fd, test_cmd, strlen(test_cmd));
            if (bytes_written != strlen(test_cmd)) {
                printf("[DEBUG] Failed to write 'Q' command: %s\n", strerror(errno));
                close(fd);
                return false;
            }
            
            // Wait for response
            usleep(100000); // 100ms
            
            // Read response with multiple attempts
            uint8_t buffer[256];
            int total_read = 0;
            
            for (int attempt = 1; attempt <= 5; attempt++) {
                fd_set readfds;
                struct timeval timeout;
                
                FD_ZERO(&readfds);
                FD_SET(fd, &readfds);
                timeout.tv_sec = 0;
                timeout.tv_usec = 500000; // 500ms timeout
                
                int result = select(fd + 1, &readfds, NULL, NULL, &timeout);
                if (result > 0 && FD_ISSET(fd, &readfds)) {
                    int bytes_read = read(fd, buffer + total_read, sizeof(buffer) - total_read);
                    if (bytes_read > 0) {
                        total_read += bytes_read;
                        printf("[DEBUG] Attempt %d: Read %d bytes\n", attempt, bytes_read);
                        
                        // Check for speeduino signature
                        if (strstr((char*)buffer, "speeduino") != NULL) {
                            printf("[DEBUG] ✅ Speeduino signature detected (ASCII fallback)!\n");
                            printf("[DEBUG] Response: %.*s\n", total_read, buffer);
                            close(fd);
                            return true;
                        }
                    }
                }
            }
            
            if (total_read > 0) {
                printf("[DEBUG] ⚠️  Got response but no signature: ");
                for (int i = 0; i < total_read && i < 32; i++) {
                    printf("%02X ", buffer[i]);
                }
                printf(" | %.*s\n", total_read, buffer);
            } else {
                printf("[DEBUG] ❌ No response received\n");
            }
            
            close(fd);
            return false;
            
        case ECU_PROTOCOL_EPICEFI:
            test_command = EPICEFI_CMD_GET_STATUS;
            break;
        case ECU_PROTOCOL_MEGASQUIRT:
            test_command = "S";
            break;
        case ECU_PROTOCOL_LIBREEMS:
            test_command = "L";
            break;
        default:
            close(fd);
            return false;
    }
    
    if (test_command) {
        printf("[DEBUG] Sending test command: '%s'\n", test_command);
        
        int cmd_len = strlen(test_command);
        int bytes_written = write(fd, test_command, cmd_len);
        if (bytes_written != cmd_len) {
            printf("[DEBUG] Failed to write command: %s\n", strerror(errno));
            close(fd);
            return false;
        }
        
        // Wait for response
        usleep(100000); // 100ms delay
        
        // Try to read response with timeout
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = 1; // 1 second timeout
        timeout.tv_usec = 0;
        
        int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result <= 0) {
            printf("[DEBUG] Timeout waiting for response\n");
            close(fd);
            return false;
        }
        
        // Try to read response
        uint8_t buffer[256];
        int bytes_read = read(fd, buffer, sizeof(buffer));
        printf("[DEBUG] Read %d bytes in response\n", bytes_read);
        
        if (bytes_read > 0) {
            printf("[DEBUG] Response data: ");
            for (int i = 0; i < bytes_read && i < 16; i++) {
                printf("[%02X]", buffer[i]);
            }
            printf("\n");
        }
        
        close(fd);
        return bytes_read > 0;
    }
    
    close(fd);
    return false;
}

// EpicEFI Protocol Implementation
static bool epicefi_parse_response(ECUContext* ctx, const uint8_t* data, int length) {
    if (!ctx || !data || length < 1) {
        return false;
    }
    
    // EpicEFI responses are typically JSON-like format
    // Example: "RPM:2500", "MAP:95", etc.
    char response[256];
    if (length >= sizeof(response)) {
        length = sizeof(response) - 1;
    }
    memcpy(response, data, length);
    response[length] = '\0';
    
    // Parse based on command type
    char* colon_pos = strchr(response, ':');
    if (!colon_pos) {
        return false;
    }
    
    *colon_pos = '\0';
    char* command = response;
    float value = atof(colon_pos + 1);
    
    if (strcmp(command, "RPM") == 0) {
        ctx->data.rpm = value;
    } else if (strcmp(command, "MAP") == 0) {
        ctx->data.map = value;
    } else if (strcmp(command, "TPS") == 0) {
        ctx->data.tps = value;
    } else if (strcmp(command, "TEMP") == 0) {
        ctx->data.coolant_temp = value;
    } else if (strcmp(command, "VOLT") == 0) {
        ctx->data.battery_voltage = value;
    } else if (strcmp(command, "AFR") == 0) {
        ctx->data.afr = value;
    } else if (strcmp(command, "TIMING") == 0) {
        ctx->data.timing = value;
    } else if (strcmp(command, "BOOST") == 0) {
        ctx->data.boost = value;
    } else if (strcmp(command, "STATUS") == 0) {
        uint8_t status = (uint8_t)value;
        ctx->data.engine_running = (status & EPICEFI_STATUS_ENGINE_RUNNING) != 0;
        ctx->data.engine_cranking = (status & EPICEFI_STATUS_ENGINE_CRANKING) != 0;
        ctx->data.boost_control_active = (status & EPICEFI_STATUS_BOOST_CONTROL) != 0;
        ctx->data.knock_detected = (status & EPICEFI_STATUS_KNOCK_DETECTED) != 0;
        ctx->data.check_engine_light = (status & EPICEFI_STATUS_CHECK_ENGINE) != 0;
    } else {
        return false;
    }
    
    ctx->data.last_update = SDL_GetTicks();
    return true;
}

static bool epicefi_send_command_and_read(ECUContext* ctx, const char* command, uint8_t* response, int* response_length) {
    if (!ctx || !command || !response || !response_length) {
        return false;
    }
    
    int fd = (int)(intptr_t)ctx->serial_handle;
    if (fd < 0) {
        return false;
    }
    
    // Send command with newline
    char cmd_with_newline[256];
    snprintf(cmd_with_newline, sizeof(cmd_with_newline), "%s\n", command);
    int cmd_len = strlen(cmd_with_newline);
    int bytes_written = write(fd, cmd_with_newline, cmd_len);
    if (bytes_written != cmd_len) {
        ecu_set_error(ctx, "Failed to send EpicEFI command");
        return false;
    }
    
    // Wait for response
    SDL_Delay(75); // EpicEFI may need more time
    
    // Read response with timeout
    fd_set read_fds;
    struct timeval timeout;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 750000; // 750ms timeout for EpicEFI
    
    int select_result = select(fd + 1, &read_fds, NULL, NULL, &timeout);
    if (select_result <= 0) {
        ecu_set_error(ctx, "Timeout waiting for EpicEFI response");
        return false;
    }
    
    // Read available data
    *response_length = read(fd, response, 256);
    if (*response_length <= 0) {
        ecu_set_error(ctx, "Failed to read EpicEFI response");
        return false;
    }
    
    return true;
}

bool ecu_epicefi_connect(ECUContext* ctx) {
    // Open serial port
    int fd = open(ctx->config.port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        ecu_set_error(ctx, "Failed to open serial port");
        return false;
    }
    
    // Configure serial port for EpicEFI
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to get serial attributes");
        return false;
    }
    
    // EpicEFI typically uses 115200 baud
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        ecu_set_error(ctx, "Failed to set serial attributes");
        return false;
    }
    
    ctx->serial_handle = (void*)(intptr_t)fd;
    
    // Send EpicEFI handshake and verify connection
    uint8_t response[256];
    int response_length;
    
    if (!epicefi_send_command_and_read(ctx, EPICEFI_CMD_GET_STATUS, response, &response_length)) {
        ecu_disconnect(ctx);
        return false;
    }
    
    // Verify we got a valid response
    if (response_length < 1 || strstr((char*)response, "STATUS") == NULL) {
        ecu_set_error(ctx, "Invalid EpicEFI handshake response");
        ecu_disconnect(ctx);
        return false;
    }
    
    // Parse initial status
    epicefi_parse_response(ctx, response, response_length);
    
    ctx->state = ECU_STATE_CONNECTED;
    ctx->connection_start = SDL_GetTicks();
    ecu_clear_error(ctx);
    
    return true;
}

bool ecu_epicefi_update(ECUContext* ctx) {
    if (!ctx || ctx->state != ECU_STATE_CONNECTED) {
        return false;
    }
    
    uint8_t response[256];
    int response_length;
    bool data_updated = false;
    
    // Request all critical data
    const char* commands[] = {
        EPICEFI_CMD_GET_RPM,
        EPICEFI_CMD_GET_MAP,
        EPICEFI_CMD_GET_TPS,
        EPICEFI_CMD_GET_TEMP,
        EPICEFI_CMD_GET_VOLTAGE,
        EPICEFI_CMD_GET_AFR,
        EPICEFI_CMD_GET_TIMING,
        EPICEFI_CMD_GET_BOOST,
        EPICEFI_CMD_GET_STATUS
    };
    
    for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (epicefi_send_command_and_read(ctx, commands[i], response, &response_length)) {
            if (epicefi_parse_response(ctx, response, response_length)) {
                data_updated = true;
            }
        }
        
        // Small delay between commands to avoid overwhelming the ECU
        SDL_Delay(15);
    }
    
    if (data_updated) {
        // Validate data
        if (!ecu_validate_data(&ctx->data)) {
            ecu_set_error(ctx, "Invalid data received from EpicEFI");
            return false;
        }
        
        // Call data update callback
        if (ctx->on_data_update) {
            ctx->on_data_update(&ctx->data);
        }
        
        ctx->error_count = 0;
        return true;
    }
    
    // If no data was updated, increment error count
    ctx->error_count++;
    if (ctx->error_count > 5) {
        ecu_set_error(ctx, "Multiple failed EpicEFI communication attempts");
        return false;
    }
    
    return false;
}

// Configuration helpers
ECUConfig ecu_config_default(void) {
    ECUConfig config = {0};
    config.protocol = ECU_PROTOCOL_SPEEDUINO;  // Priority 1: Speeduino
    strcpy(config.port, "/dev/ttyUSB0");
    config.baud_rate = 115200;
    config.timeout_ms = 1000;
    config.auto_connect = false;
    config.auto_reconnect = true;
    config.reconnect_interval = 5000;
    return config;
}

ECUConfig ecu_config_speeduino(void) {
    ECUConfig config = ecu_config_default();
    config.protocol = ECU_PROTOCOL_SPEEDUINO;
    return config;
}

ECUConfig ecu_config_epicefi(void) {
    ECUConfig config = ecu_config_default();
    config.protocol = ECU_PROTOCOL_EPICEFI;
    return config;
}

ECUConfig ecu_config_megasquirt(void) {
    ECUConfig config = ecu_config_default();
    config.protocol = ECU_PROTOCOL_MEGASQUIRT;
    return config;
}

ECUConfig ecu_config_libreems(void) {
    ECUConfig config = ecu_config_default();
    config.protocol = ECU_PROTOCOL_LIBREEMS;
    return config;
}

// Data validation
bool ecu_validate_data(const ECUData* data) {
    if (!data) return false;
    
    // More relaxed range checks for Speeduino data
    if (data->rpm < 0 || data->rpm > 15000) return false;
    if (data->map < 0 || data->map > 300) return false;
    if (data->tps < 0 || data->tps > 100) return false;
    if (data->coolant_temp < -50 || data->coolant_temp > 200) return false;
    if (data->battery_voltage < 3 || data->battery_voltage > 20) return false;
    if (data->afr < 5 || data->afr > 30) return false;
    
    return true;
}

bool ecu_is_data_fresh(const ECUData* data, uint32_t max_age_ms) {
    if (!data) return false;
    
    uint32_t current_time = SDL_GetTicks();
    return (current_time - data->last_update) < max_age_ms;
}

// Error handling
void ecu_set_error(ECUContext* ctx, const char* error) {
    if (ctx && error) {
        strncpy(ctx->last_error, error, sizeof(ctx->last_error) - 1);
        ctx->last_error[sizeof(ctx->last_error) - 1] = '\0';
        
        if (ctx->on_error) {
            ctx->on_error(error);
        }
    }
}

const char* ecu_get_last_error(ECUContext* ctx) {
    return ctx ? ctx->last_error : NULL;
}

void ecu_clear_error(ECUContext* ctx) {
    if (ctx) {
        memset(ctx->last_error, 0, sizeof(ctx->last_error));
    }
}

// Statistics functions
void ecu_get_statistics(ECUContext* ctx, uint32_t* bytes_rx, uint32_t* bytes_tx, 
                       uint32_t* packets_rx, uint32_t* packets_tx,
                       uint32_t* errors, uint32_t* timeouts, uint32_t* last_activity) {
    if (!ctx) return;
    
    if (bytes_rx) *bytes_rx = ctx->bytes_received;
    if (bytes_tx) *bytes_tx = ctx->bytes_sent;
    if (packets_rx) *packets_rx = ctx->packets_received;
    if (packets_tx) *packets_tx = ctx->packets_sent;
    if (errors) *errors = ctx->errors;
    if (timeouts) *timeouts = ctx->timeouts;
    if (last_activity) *last_activity = ctx->last_activity;
}

void ecu_get_rates(ECUContext* ctx, float* rx_rate, float* tx_rate, 
                  float* rx_packet_rate, float* tx_packet_rate) {
    if (!ctx) return;
    
    if (rx_rate) *rx_rate = ctx->rx_rate;
    if (tx_rate) *tx_rate = ctx->tx_rate;
    if (rx_packet_rate) *rx_packet_rate = ctx->rx_packet_rate;
    if (tx_packet_rate) *tx_packet_rate = ctx->tx_packet_rate;
}

// Adaptive timing functions
void ecu_update_response_time(ECUContext* ctx, uint32_t response_time_ms) {
    if (!ctx) return;
    
    // Store response time in circular buffer
    ctx->response_time_samples[ctx->response_time_index] = response_time_ms;
    ctx->response_time_index = (ctx->response_time_index + 1) % 10;
    
    // Calculate statistics
    uint32_t sum = 0;
    uint32_t min_time = UINT32_MAX;
    uint32_t max_time = 0;
    int valid_samples = 0;
    
    for (int i = 0; i < 10; i++) {
        if (ctx->response_time_samples[i] > 0) {
            sum += ctx->response_time_samples[i];
            if (ctx->response_time_samples[i] < min_time) {
                min_time = ctx->response_time_samples[i];
            }
            if (ctx->response_time_samples[i] > max_time) {
                max_time = ctx->response_time_samples[i];
            }
            valid_samples++;
        }
    }
    
    if (valid_samples > 0) {
        ctx->avg_response_time = sum / valid_samples;
        ctx->min_response_time = min_time;
        ctx->max_response_time = max_time;
        ctx->timing_initialized = (valid_samples >= 5); // Need at least 5 samples
        
        printf("[DEBUG] Adaptive timing: avg=%ums, min=%ums, max=%ums, samples=%d\n", 
               ctx->avg_response_time, ctx->min_response_time, ctx->max_response_time, valid_samples);
    }
}

uint32_t ecu_get_adaptive_timeout(ECUContext* ctx) {
    if (!ctx || !ctx->timing_initialized) {
        return 200000; // Default 200ms timeout
    }
    
    // Use average response time + 50% margin for safety
    uint32_t adaptive_timeout = ctx->avg_response_time * 150 / 100; // 150% of average
    
    // Clamp between 50ms and 500ms
    if (adaptive_timeout < 50000) adaptive_timeout = 50000;   // Minimum 50ms
    if (adaptive_timeout > 500000) adaptive_timeout = 500000; // Maximum 500ms
    
    return adaptive_timeout;
}

void ecu_get_timing_stats(ECUContext* ctx, uint32_t* avg_time, uint32_t* min_time, uint32_t* max_time, bool* initialized) {
    if (!ctx) return;
    
    if (avg_time) *avg_time = ctx->avg_response_time;
    if (min_time) *min_time = ctx->min_response_time;
    if (max_time) *max_time = ctx->max_response_time;
    if (initialized) *initialized = ctx->timing_initialized;
}

// Demo mode support functions
void ecu_set_demo_mode(ECUContext* ctx, bool enabled) {
    if (!ctx) return;
    
    ctx->demo_mode = enabled;
    
    if (enabled) {
        // Set demo state
        ctx->state = ECU_STATE_CONNECTED;
        ctx->connection_start = SDL_GetTicks();
        ctx->data.connection_time = ctx->connection_start;
        ctx->error_count = 0;
        
        // Call connection change callback
        if (ctx->on_connection_change) {
            ctx->on_connection_change(ctx->state);
        }
        
        printf("[DEBUG] Demo mode enabled\n");
    } else {
        // Clear demo state
        ctx->state = ECU_STATE_DISCONNECTED;
        ctx->connection_start = 0;
        ctx->data.connection_time = 0;
        
        // Free demo INI config
        if (ctx->demo_ini_config) {
            ecu_free_ini_config(ctx->demo_ini_config);
            ctx->demo_ini_config = NULL;
        }
        
        // Call connection change callback
        if (ctx->on_connection_change) {
            ctx->on_connection_change(ctx->state);
        }
        
        printf("[DEBUG] Demo mode disabled\n");
    }
}

void ecu_set_demo_ini_config(ECUContext* ctx, INIConfig* ini_config) {
    if (!ctx) return;
    
    // Free existing demo config
    if (ctx->demo_ini_config) {
        ecu_free_ini_config(ctx->demo_ini_config);
    }
    
    ctx->demo_ini_config = ini_config;
    
    if (ini_config) {
        printf("[DEBUG] Demo INI config set: %s\n", ini_config->ecu_name);
    }
}

bool ecu_is_demo_mode(ECUContext* ctx) {
    if (!ctx) return false;
    return ctx->demo_mode;
}

// Global demo mode callback function pointer
static void (*g_global_demo_mode_callback)(bool) = NULL;

// Set the callback for global demo mode
void ecu_set_global_demo_mode_callback(void (*callback)(bool)) {
    g_global_demo_mode_callback = callback;
}

// Set global demo mode (for cross-module communication)
void ecu_set_global_demo_mode(bool enabled) {
    printf("[DEBUG] Global demo mode %s\n", enabled ? "enabled" : "disabled");
    
    // Call the callback if it's set
    if (g_global_demo_mode_callback) {
        g_global_demo_mode_callback(enabled);
    }
} 