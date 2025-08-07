/*
 * ECU Communication - Protocol Support for ECU Tuning
 * 
 * Copyright (C) 2025 Pat Burke
 * 
 * Supports MegaSquirt, Speeduino, and LibreEMS communication protocols.
 */

#ifndef ECU_COMMUNICATION_H
#define ECU_COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "ecu_ini_parser.h"

// ECU Protocol Types
typedef enum {
    ECU_PROTOCOL_NONE = 0,
    ECU_PROTOCOL_SPEEDUINO,    // Priority 1: Speeduino
    ECU_PROTOCOL_EPICEFI,      // Priority 2: EpicEFI
    ECU_PROTOCOL_MEGASQUIRT,   // Priority 3: MegaSquirt
    ECU_PROTOCOL_LIBREEMS,     // Priority 4: LibreEMS
    ECU_PROTOCOL_COUNT
} ECUProtocol;

// ECU Connection States
typedef enum {
    ECU_STATE_DISCONNECTED = 0,
    ECU_STATE_CONNECTING,
    ECU_STATE_CONNECTED,
    ECU_STATE_ERROR,
    ECU_STATE_TIMEOUT
} ECUConnectionState;

// ECU Data Structure
typedef struct {
    // Engine Data
    float rpm;
    float map;
    float tps;
    float coolant_temp;
    float intake_temp;
    float battery_voltage;
    float afr;
    float afr_target;
    float timing;
    float fuel_pressure;
    float oil_pressure;
    float oil_temp;
    
    // Boost Data
    float boost;
    float boost_target;
    float wastegate_duty;
    
    // Fuel Data
    float fuel_pw1;
    float fuel_pw2;
    float fuel_duty;
    float injector_duty;
    
    // Ignition Data
    float dwell;
    float spark_advance;
    float knock_count;
    float knock_retard;
    
    // Status Flags
    bool engine_running;
    bool engine_cranking;
    bool boost_control_active;
    bool knock_detected;
    bool check_engine_light;
    
    // Timestamps
    uint32_t last_update;
    uint32_t connection_time;
} ECUData;

// ECU Communication Configuration
typedef struct {
    ECUProtocol protocol;
    char port[64];
    int baud_rate;
    int timeout_ms;
    bool auto_connect;
    bool auto_reconnect;
    int reconnect_interval;
} ECUConfig;

// ECU Communication Context
typedef struct {
    ECUProtocol protocol;
    ECUConnectionState state;
    ECUData data;
    ECUConfig config;
    
    // Communication
    void* serial_handle;
    uint32_t last_heartbeat;
    uint32_t connection_start;
    int error_count;
    char last_error[256];
    
    // Data buffers
    uint8_t rx_buffer[1024];
    uint8_t tx_buffer[1024];
    int rx_count;
    int tx_count;
    
    // Communication statistics
    uint32_t bytes_received;
    uint32_t bytes_sent;
    uint32_t packets_received;
    uint32_t packets_sent;
    uint32_t errors;
    uint32_t timeouts;
    uint32_t last_activity;
    float rx_rate;      // bytes per second
    float tx_rate;      // bytes per second
    float rx_packet_rate; // packets per second
    float tx_packet_rate; // packets per second
    
    // Adaptive timing
    uint32_t response_time_samples[10];  // Last 10 response times
    int response_time_index;             // Current index in circular buffer
    uint32_t avg_response_time;          // Average response time in ms
    uint32_t min_response_time;          // Fastest response time
    uint32_t max_response_time;          // Slowest response time
    bool timing_initialized;             // Whether we have enough samples
    
    // Callbacks
    void (*on_data_update)(ECUData* data);
    void (*on_connection_change)(ECUConnectionState state);
    void (*on_error)(const char* error);
    
    // INI Configuration (for INI-based connections)
    INIConfig* ini_config;
    
    // Demo mode support
    bool demo_mode;
    INIConfig* demo_ini_config;
} ECUContext;

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif

ECUContext* ecu_init(void);
void ecu_cleanup(ECUContext* ctx);
bool ecu_connect(ECUContext* ctx, const ECUConfig* config);
bool ecu_connect_with_ini(ECUContext* ctx, const char* port, const char* ini_file_path);

// Demo mode support
void ecu_set_demo_mode(ECUContext* ctx, bool enabled);
void ecu_set_demo_ini_config(ECUContext* ctx, INIConfig* ini_config);
bool ecu_is_demo_mode(ECUContext* ctx);
void ecu_set_global_demo_mode_callback(void (*callback)(bool));
void ecu_set_global_demo_mode(bool enabled);

void ecu_disconnect(ECUContext* ctx);
bool ecu_is_connected(ECUContext* ctx);
ECUConnectionState ecu_get_state(ECUContext* ctx);
const ECUData* ecu_get_data(ECUContext* ctx);
bool ecu_update(ECUContext* ctx);
bool ecu_send_command(ECUContext* ctx, const char* command);
const char* ecu_get_protocol_name(ECUProtocol protocol);
ECUProtocol ecu_parse_protocol_name(const char* name);
const char* ecu_get_state_name(ECUConnectionState state);

// Protocol-specific functions
bool ecu_speeduino_connect(ECUContext* ctx);
bool ecu_epicefi_connect(ECUContext* ctx);
bool ecu_megasquirt_connect(ECUContext* ctx);
bool ecu_libreems_connect(ECUContext* ctx);
bool ecu_speeduino_update(ECUContext* ctx);
bool ecu_epicefi_update(ECUContext* ctx);
bool ecu_megasquirt_update(ECUContext* ctx);
bool ecu_libreems_update(ECUContext* ctx);

// Configuration helpers
ECUConfig ecu_config_default(void);
ECUConfig ecu_config_speeduino(void);
ECUConfig ecu_config_epicefi(void);
ECUConfig ecu_config_megasquirt(void);
ECUConfig ecu_config_libreems(void);

// Data validation
bool ecu_validate_data(const ECUData* data);
bool ecu_is_data_fresh(const ECUData* data, uint32_t max_age_ms);

// Error handling
void ecu_set_error(ECUContext* ctx, const char* error);
const char* ecu_get_last_error(ECUContext* ctx);
void ecu_clear_error(ECUContext* ctx);

// Statistics functions
void ecu_get_statistics(ECUContext* ctx, uint32_t* bytes_rx, uint32_t* bytes_tx, 
                       uint32_t* packets_rx, uint32_t* packets_tx,
                       uint32_t* errors, uint32_t* timeouts, uint32_t* last_activity);
void ecu_get_rates(ECUContext* ctx, float* rx_rate, float* tx_rate, 
                  float* rx_packet_rate, float* tx_packet_rate);

// Adaptive timing functions
void ecu_update_response_time(ECUContext* ctx, uint32_t response_time_ms);
uint32_t ecu_get_adaptive_timeout(ECUContext* ctx);
void ecu_get_timing_stats(ECUContext* ctx, uint32_t* avg_time, uint32_t* min_time, uint32_t* max_time, bool* initialized);

// Speeduino Protocol Constants
#define SPEEDUINO_START_BYTE            0x72    // 'r' - Start of packet
#define SPEEDUINO_STOP_BYTE             0x03    // ETX - End of packet
#define SPEEDUINO_ESCAPE_BYTE           0x2D    // '-' - Escape character

// Speeduino Protocol Commands
#define SPEEDUINO_CMD_QUERY             0x51    // 'Q' - Query command
#define SPEEDUINO_CMD_GET_DATA          0x41    // 'A' - Get real-time data
#define SPEEDUINO_CMD_GET_VERSION       0x53    // 'S' - Get version info
#define SPEEDUINO_CMD_GET_SIGNATURE     0x56    // 'V' - Get signature

// Speeduino Packet Structure
typedef struct {
    uint8_t start_byte;      // 0x72 ('r')
    uint8_t command;         // Command byte
    uint16_t data_length;    // Length of data payload
    uint8_t data[256];       // Data payload
    uint8_t crc_high;        // CRC high byte
    uint8_t crc_low;         // CRC low byte
    uint8_t stop_byte;       // 0x03 (ETX)
} SpeeduinoPacket;

// Speeduino Data Structure
typedef struct {
    uint16_t rpm;
    uint8_t map;
    uint8_t tps;
    uint8_t coolant_temp;
    uint8_t intake_temp;
    uint8_t battery_voltage;
    uint8_t afr;
    int8_t timing;
    uint8_t boost;
    uint8_t engine_status;
    uint32_t timestamp;
} SpeeduinoData;

// Speeduino Status Bits
#define SPEEDUINO_STATUS_ENGINE_RUNNING     0x01
#define SPEEDUINO_STATUS_ENGINE_CRANKING    0x02
#define SPEEDUINO_STATUS_BOOST_CONTROL      0x04
#define SPEEDUINO_STATUS_KNOCK_DETECTED     0x08
#define SPEEDUINO_STATUS_CHECK_ENGINE       0x10

// Serial Port Detection
#define MAX_SERIAL_PORTS 16
typedef struct {
    char ports[MAX_SERIAL_PORTS][64];
    int count;
} SerialPortList;

SerialPortList ecu_detect_serial_ports(void);
bool ecu_test_serial_port(const char* port, ECUProtocol protocol);

// CRC calculation functions (internal use only)

// EpicEFI Protocol Commands
#define EPICEFI_CMD_GET_RPM           "RPM"     // Get RPM
#define EPICEFI_CMD_GET_MAP           "MAP"     // Get MAP
#define EPICEFI_CMD_GET_TPS           "TPS"     // Get TPS
#define EPICEFI_CMD_GET_TEMP          "TEMP"    // Get coolant temp
#define EPICEFI_CMD_GET_VOLTAGE       "VOLT"    // Get battery voltage
#define EPICEFI_CMD_GET_AFR           "AFR"     // Get AFR
#define EPICEFI_CMD_GET_TIMING        "TIMING"  // Get ignition timing
#define EPICEFI_CMD_GET_BOOST         "BOOST"   // Get boost pressure
#define EPICEFI_CMD_GET_STATUS        "STATUS"  // Get engine status
#define EPICEFI_CMD_GET_ALL_DATA      "ALL"     // Get all data

// EpicEFI Data Structure
typedef struct {
    uint16_t rpm;
    uint8_t map;
    uint8_t tps;
    uint8_t coolant_temp;
    uint8_t intake_temp;
    uint8_t battery_voltage;
    uint8_t afr;
    int8_t timing;
    uint8_t boost;
    uint8_t engine_status;
    uint32_t timestamp;
} EpicEFIData;

// EpicEFI Status Bits
#define EPICEFI_STATUS_ENGINE_RUNNING     0x01
#define EPICEFI_STATUS_ENGINE_CRANKING    0x02
#define EPICEFI_STATUS_BOOST_CONTROL      0x04
#define EPICEFI_STATUS_KNOCK_DETECTED     0x08
#define EPICEFI_STATUS_CHECK_ENGINE       0x10

#ifdef __cplusplus
}
#endif

#endif // ECU_COMMUNICATION_H 