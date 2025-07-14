/*
 * Speeduino Protocol Plugin Header for MegaTunix Redux 2025
 */

#ifndef SPEEDUINO_PLUGIN_H
#define SPEEDUINO_PLUGIN_H

#include <glib.h>
#include "plugin_system.h"

G_BEGIN_DECLS

/* Speeduino output channels structure for runtime data */
typedef struct {
    /* Essential engine parameters */
    uint8_t secl;              // Seconds counter
    uint8_t status1;           // Status bits 1
    uint8_t engine;            // Engine status
    uint8_t dwell;             // Dwell time
    uint16_t map;              // MAP sensor reading
    uint8_t iat;               // Intake air temperature
    uint8_t coolant;           // Coolant temperature
    uint8_t batCorrection;     // Battery voltage correction
    uint8_t battery10;         // Battery voltage * 10
    uint8_t o2;                // O2 sensor reading
    uint8_t egoCorrection;     // EGO correction
    uint8_t iatCorrection;     // IAT correction
    uint8_t wueCorrection;     // WUE correction
    uint16_t rpm;              // Engine RPM
    uint8_t taeCorrection;     // TAE correction
    uint8_t gammaEnrich;       // Gamma enrichment
    uint8_t ve;                // VE value
    uint8_t afrtgt1;           // AFR target 1
    uint8_t pw1;               // Pulse width 1
    uint8_t tpsDOT;            // TPS rate of change
    uint8_t advance;           // Ignition advance
    uint8_t tps;               // TPS reading
    uint16_t loopsPerSecond;   // Main loop frequency
    uint16_t freeRAM;          // Free RAM
    uint8_t boostTarget;       // Boost target
    uint8_t boostDuty;         // Boost duty cycle
    uint8_t spark;             // Spark status
    uint16_t rpmDOT;           // RPM rate of change
    uint8_t ethanolPct;        // Ethanol percentage
    uint8_t flexCorrection;    // Flex fuel correction
    uint8_t flexIgnCorrection; // Flex ignition correction
    uint8_t idleLoad;          // Idle load
    uint8_t testOutputs;       // Test outputs status
    uint8_t o2_2;              // O2 sensor 2
    uint8_t baro;              // Barometric pressure
    
    /* Extended parameters */
    uint8_t canin[16];         // CAN input channels
    uint8_t tpsADC;            // TPS ADC raw value
    uint8_t errors;            // Error flags
    uint8_t launchCorrection;  // Launch control correction
    uint8_t pw2;               // Pulse width 2
    uint8_t pw3;               // Pulse width 3
    uint8_t pw4;               // Pulse width 4
    uint8_t status3;           // Status bits 3
    uint8_t engineProtect;     // Engine protection status
    uint8_t fuelLoad;          // Fuel load
    uint8_t ignLoad;           // Ignition load
    uint8_t injAngle;          // Injection angle
    uint8_t idleDuty;          // Idle duty cycle
    uint8_t CLIdleTarget;      // Closed loop idle target
    uint8_t mapDOT;            // MAP rate of change
    uint8_t vvt1Angle;         // VVT angle 1
    uint8_t vvt1TargetAngle;   // VVT target angle 1
    uint8_t vvt1Duty;          // VVT duty 1
    uint8_t flexBoostCorrection; // Flex boost correction
    uint8_t baroCorrection;    // Barometric pressure correction
    uint8_t ASEValue;          // After start enrichment
    uint8_t vss;               // Vehicle speed sensor
    uint8_t gear;              // Current gear
    uint8_t fuelPressure;      // Fuel pressure
    uint8_t oilPressure;       // Oil pressure
    uint8_t wmiPW;             // Water/methanol injection pulse width
    uint8_t status4;           // Status bits 4
    uint8_t vvt2Angle;         // VVT angle 2
    uint8_t vvt2TargetAngle;   // VVT target angle 2
    uint8_t vvt2Duty;          // VVT duty 2
    uint8_t outputsStatus;     // Outputs status
    uint8_t fuelTemp;          // Fuel temperature
    uint8_t fuelTempCorrection; // Fuel temperature correction
    uint8_t advance1;          // Ignition advance 1
    uint8_t advance2;          // Ignition advance 2
    uint8_t egt1;              // EGT sensor 1
    uint8_t egt2;              // EGT sensor 2
    uint8_t nitrous1;          // Nitrous system 1
    uint8_t nitrous2;          // Nitrous system 2
    uint8_t nitrous3;          // Nitrous system 3
} SpeeduinoOutputChannels;

/* Speeduino status bit definitions */
#define SPEEDUINO_STATUS1_INJ1    0x01
#define SPEEDUINO_STATUS1_INJ2    0x02
#define SPEEDUINO_STATUS1_INJ3    0x04
#define SPEEDUINO_STATUS1_INJ4    0x08
#define SPEEDUINO_STATUS1_DFO     0x10
#define SPEEDUINO_STATUS1_BOOSTCUT 0x20
#define SPEEDUINO_STATUS1_TOOTHLOG1 0x40
#define SPEEDUINO_STATUS1_TOOTHLOG2 0x80

#define SPEEDUINO_ENGINE_READY    0x01
#define SPEEDUINO_ENGINE_CRANK    0x02
#define SPEEDUINO_ENGINE_WARMUP   0x04
#define SPEEDUINO_ENGINE_TPS_ACC  0x08
#define SPEEDUINO_ENGINE_TPS_DEC  0x10
#define SPEEDUINO_ENGINE_MAP_ACC  0x20
#define SPEEDUINO_ENGINE_MAP_DEC  0x40
#define SPEEDUINO_ENGINE_RUNNING  0x80

/* Configuration page indices */
typedef enum {
    SPEEDUINO_CONFIG_PAGE1_VE = 0,
    SPEEDUINO_CONFIG_PAGE2_SETTINGS,
    SPEEDUINO_CONFIG_PAGE3_IGNITION,
    SPEEDUINO_CONFIG_PAGE4_IGN_SETTINGS,
    SPEEDUINO_CONFIG_PAGE5_AFR,
    SPEEDUINO_CONFIG_PAGE6_AFR_SETTINGS,
    SPEEDUINO_CONFIG_PAGE7_BOOST,
    SPEEDUINO_CONFIG_PAGE8_VVT,
    SPEEDUINO_CONFIG_PAGE9_CAN_OBD,
    SPEEDUINO_CONFIG_PAGE10_WARMUP,
    SPEEDUINO_CONFIG_PAGE11_PROG_OUTPUTS,
    SPEEDUINO_CONFIG_PAGE12_FUEL2,
    SPEEDUINO_CONFIG_PAGE13_IGN2,
    SPEEDUINO_CONFIG_PAGE14_BOOST2,
    SPEEDUINO_CONFIG_PAGE15_TRIM
} SpeeduinoConfigPage;

/* Plugin API functions */

/**
 * Get runtime data from connected Speeduino ECU
 * @param context Plugin context
 * @return Pointer to output channels data or NULL if not connected
 */
const SpeeduinoOutputChannels *speeduino_get_runtime_data(MtxPluginContext *context);

/**
 * Set a configuration value on the ECU
 * @param context Plugin context
 * @param page Configuration page index
 * @param offset Offset within the page
 * @param value Value to set
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_set_config_value(MtxPluginContext *context, guint page, 
                                   guint offset, guint value, GError **error);

/**
 * Start tooth logging for trigger analysis
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_start_tooth_logging(MtxPluginContext *context, GError **error);

/**
 * Stop tooth logging
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_stop_tooth_logging(MtxPluginContext *context, GError **error);

/**
 * Check if connected to ECU
 * @param context Plugin context
 * @return TRUE if connected, FALSE otherwise
 */
gboolean speeduino_is_connected(MtxPluginContext *context);

/**
 * Get ECU signature string
 * @param context Plugin context
 * @return ECU signature or NULL if not available
 */
const gchar *speeduino_get_ecu_signature(MtxPluginContext *context);

/**
 * Get firmware version string
 * @param context Plugin context
 * @return Firmware version or NULL if not available
 */
const gchar *speeduino_get_firmware_version(MtxPluginContext *context);

/**
 * Connect to Speeduino ECU
 * @param context Plugin context
 * @param device_path Serial device path (e.g., "/dev/ttyUSB0")
 * @param baud_rate Baud rate (typically 115200)
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_connect_device(MtxPluginContext *context, const gchar *device_path,
                                 gint baud_rate, GError **error);

/**
 * Disconnect from ECU
 * @param context Plugin context
 */
void speeduino_disconnect_device(MtxPluginContext *context);

/**
 * Auto-detect connected Speeduino devices
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE if device found and connected, FALSE otherwise
 */
gboolean speeduino_auto_detect(MtxPluginContext *context, GError **error);

/**
 * Burn configuration to ECU EEPROM
 * @param context Plugin context
 * @param page_index Page to burn (0 = all pages)
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_burn_config(MtxPluginContext *context, guint page_index, GError **error);

/**
 * Read configuration page from ECU
 * @param context Plugin context
 * @param page_index Page to read
 * @param data Buffer to store page data
 * @param data_size Size of data buffer
 * @param error Error return location
 * @return Number of bytes read or -1 on error
 */
gssize speeduino_read_config_page(MtxPluginContext *context, guint page_index,
                                 guchar *data, gsize data_size, GError **error);

/**
 * Write configuration page to ECU
 * @param context Plugin context
 * @param page_index Page to write
 * @param data Page data
 * @param data_size Size of data
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_write_config_page(MtxPluginContext *context, guint page_index,
                                    const guchar *data, gsize data_size, GError **error);

/**
 * Get calibration data for sensors
 * @param context Plugin context
 * @param sensor_type Sensor type (0=CLT, 1=IAT, 2=O2)
 * @param calibration_data Buffer for calibration data
 * @param data_size Size of buffer
 * @param error Error return location
 * @return Number of bytes read or -1 on error
 */
gssize speeduino_get_calibration(MtxPluginContext *context, guint sensor_type,
                                guchar *calibration_data, gsize data_size, GError **error);

/**
 * Set sensor calibration data
 * @param context Plugin context
 * @param sensor_type Sensor type (0=CLT, 1=IAT, 2=O2)
 * @param calibration_data Calibration data
 * @param data_size Size of data
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_set_calibration(MtxPluginContext *context, guint sensor_type,
                                  const guchar *calibration_data, gsize data_size, GError **error);

/**
 * Enhanced Speeduino Plugin Functions for Modern ECU Features
 */

/**
 * Get serial communication capabilities
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_get_serial_capabilities(MtxPluginContext *context, GError **error);

/**
 * Get CRC checksum for configuration page validation
 * @param context Plugin context
 * @param page_id Configuration page ID (0-14)
 * @param crc CRC value return location
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_get_page_crc(MtxPluginContext *context, guint8 page_id, 
                               guint32 *crc, GError **error);

/**
 * Send button command to ECU (for GUI integration)
 * @param context Plugin context
 * @param button_id Button identifier
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_send_button_command(MtxPluginContext *context, guint16 button_id, 
                                      GError **error);

/**
 * Start composite logging for cam sensors
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_start_composite_logging_cams(MtxPluginContext *context, GError **error);

/**
 * Stop composite logging for cam sensors
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_stop_composite_logging_cams(MtxPluginContext *context, GError **error);

/**
 * Read configuration page with CRC validation
 * @param context Plugin context
 * @param page_id Configuration page ID (0-14)
 * @param offset Offset within page
 * @param length Number of bytes to read
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_read_page_with_validation(MtxPluginContext *context, guint8 page_id,
                                           guint16 offset, guint16 length,
                                           GError **error);

/**
 * Get extended runtime data with all available channels
 * @param context Plugin context
 * @return Pointer to output channels structure or NULL on error
 */
const SpeeduinoOutputChannels *speeduino_get_extended_runtime_data(MtxPluginContext *context);

/**
 * Reset ECU (for firmware updates)
 * @param context Plugin context
 * @param error Error return location
 * @return TRUE on success, FALSE on error
 */
gboolean speeduino_reset_ecu(MtxPluginContext *context, GError **error);

G_END_DECLS

#endif /* SPEEDUINO_PLUGIN_H */
