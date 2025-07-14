/*
 * Speeduino Plugin Integration Tests for MegaTunix Redux
 */

#include "test_framework.h"
#include "speeduino_plugin.h"
#include "serial_utils.h"
#include <glib.h>

/* Test data and mock functions */
static gboolean mock_serial_connected = FALSE;
static guchar mock_response_data[256];
static gsize mock_response_size = 0;

/* Mock serial functions for testing */
static GIOChannel *mock_serial_open_port(const gchar *device_path, gint baud_rate, GError **error)
{
    if (g_strcmp0(device_path, "/dev/mock_speeduino") == 0) {
        mock_serial_connected = TRUE;
        return (GIOChannel *)0x12345678; /* Mock channel pointer */
    }
    
    g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND,
               "Mock device not found");
    return NULL;
}

static void mock_serial_close_port(GIOChannel *channel)
{
    mock_serial_connected = FALSE;
}

static gboolean mock_serial_send_command(GIOChannel *channel, guchar cmd,
                                        const guchar *data, gsize data_len,
                                        guchar *response, gsize *response_len)
{
    if (!mock_serial_connected) return FALSE;
    
    /* Simulate Speeduino responses */
    switch (cmd) {
        case 'C': /* Test communication */
            mock_response_data[0] = 0x00; /* OK response */
            mock_response_size = 1;
            break;
            
        case 'S': /* Get signature */
            mock_response_data[0] = 0x00; /* OK response */
            strcpy((char *)&mock_response_data[1], "speeduino 202504-dev");
            mock_response_size = 1 + strlen("speeduino 202504-dev");
            break;
            
        case 'Q': /* Get version */
            mock_response_data[0] = 0x00; /* OK response */
            strcpy((char *)&mock_response_data[1], "speeduino 202504-dev");
            mock_response_size = 1 + strlen("speeduino 202504-dev");
            break;
            
        case 'A': /* Get output channels */
            mock_response_data[0] = 0x00; /* secl */
            mock_response_data[1] = 0x80; /* status1 - engine running */
            mock_response_data[2] = 0x80; /* engine - running */
            mock_response_data[3] = 15;   /* dwell */
            mock_response_data[4] = 0x00; /* MAP high byte */
            mock_response_data[5] = 100;  /* MAP low byte = 100 kPa */
            mock_response_data[6] = 25;   /* IAT = 25°C */
            mock_response_data[7] = 85;   /* Coolant = 85°C */
            mock_response_data[8] = 100;  /* Battery correction */
            mock_response_data[9] = 140;  /* Battery voltage * 10 = 14.0V */
            mock_response_data[10] = 147; /* O2 = 14.7 AFR */
            mock_response_data[11] = 100; /* EGO correction */
            mock_response_data[12] = 100; /* IAT correction */
            mock_response_data[13] = 100; /* WUE correction */
            mock_response_data[14] = 0x0F; /* RPM high byte */
            mock_response_data[15] = 0xA0; /* RPM low byte = 4000 RPM */
            mock_response_data[16] = 100; /* TAE correction */
            mock_response_data[17] = 100; /* Gamma enrich */
            mock_response_data[18] = 80;  /* VE = 80% */
            mock_response_data[19] = 147; /* AFR target = 14.7 */
            mock_response_data[20] = 12;  /* PW1 = 12ms */
            mock_response_data[21] = 0;   /* TPS DOT */
            mock_response_data[22] = 15;  /* Advance = 15° */
            mock_response_data[23] = 45;  /* TPS = 45% */
            mock_response_data[24] = 0x03; /* Loops per second high */
            mock_response_data[25] = 0xE8; /* Loops per second low = 1000 */
            /* Fill remaining bytes with test data */
            for (int i = 26; i < 85; i++) {
                mock_response_data[i] = i;
            }
            mock_response_size = 85;
            break;
            
        default:
            mock_response_data[0] = 0x89; /* Unknown command error */
            mock_response_size = 1;
            break;
    }
    
    if (response && response_len) {
        gsize copy_size = MIN(*response_len, mock_response_size);
        memcpy(response, mock_response_data, copy_size);
        *response_len = copy_size;
    }
    
    return TRUE;
}

/* Test basic plugin initialization */
static void test_speeduino_plugin_init(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino plugin initialization");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    mtx_test_assert_not_null(test_ctx, context, "Plugin context creation");
    
    GError *error = NULL;
    gboolean result = speeduino_initialize(context, &error);
    
    mtx_test_assert_true(test_ctx, result, "Plugin initialization");
    mtx_test_assert_null(test_ctx, error, "No initialization error");
    
    /* Test that context data was set up */
    gpointer speeduino_ctx = g_object_get_data(G_OBJECT(context), "speeduino-context");
    mtx_test_assert_not_null(test_ctx, speeduino_ctx, "Speeduino context created");
    
    /* Cleanup */
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test communication functions */
static void test_speeduino_communication(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino communication");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    
    /* Test connection status (should be disconnected initially) */
    gboolean connected = speeduino_is_connected(context);
    mtx_test_assert_false(test_ctx, connected, "Initially disconnected");
    
    /* Test mock device connection */
    GError *error = NULL;
    gboolean result = speeduino_connect_device(context, "/dev/mock_speeduino", 115200, &error);
    mtx_test_assert_true(test_ctx, result, "Mock device connection");
    mtx_test_assert_null(test_ctx, error, "No connection error");
    
    /* Test connection status after connecting */
    connected = speeduino_is_connected(context);
    mtx_test_assert_true(test_ctx, connected, "Connected after connect");
    
    /* Test getting ECU info */
    const gchar *signature = speeduino_get_ecu_signature(context);
    mtx_test_assert_not_null(test_ctx, signature, "ECU signature retrieved");
    mtx_test_assert_string_contains(test_ctx, signature, "speeduino", "Signature contains 'speeduino'");
    
    const gchar *version = speeduino_get_firmware_version(context);
    mtx_test_assert_not_null(test_ctx, version, "Firmware version retrieved");
    
    /* Cleanup */
    speeduino_disconnect_device(context);
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test runtime data acquisition */
static void test_speeduino_runtime_data(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino runtime data");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    speeduino_connect_device(context, "/dev/mock_speeduino", 115200, NULL);
    
    /* Get runtime data */
    const SpeeduinoOutputChannels *data = speeduino_get_runtime_data(context);
    mtx_test_assert_not_null(test_ctx, data, "Runtime data retrieved");
    
    /* Test specific values from mock data */
    mtx_test_assert_equals_int(test_ctx, data->rpm, 4000, "RPM value");
    mtx_test_assert_equals_int(test_ctx, data->map, 100, "MAP value");
    mtx_test_assert_equals_int(test_ctx, data->coolant, 85, "Coolant temperature");
    mtx_test_assert_equals_int(test_ctx, data->iat, 25, "Intake air temperature");
    mtx_test_assert_equals_int(test_ctx, data->battery10, 140, "Battery voltage * 10");
    mtx_test_assert_equals_int(test_ctx, data->tps, 45, "TPS value");
    mtx_test_assert_equals_int(test_ctx, data->advance, 15, "Ignition advance");
    mtx_test_assert_equals_int(test_ctx, data->ve, 80, "VE value");
    
    /* Test status bits */
    gboolean engine_running = (data->engine & 0x80) != 0;
    mtx_test_assert_true(test_ctx, engine_running, "Engine running status");
    
    /* Cleanup */
    speeduino_disconnect_device(context);
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test data field mappings */
static void test_speeduino_data_mapping(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino data field mapping");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    speeduino_connect_device(context, "/dev/mock_speeduino", 115200, NULL);
    
    const SpeeduinoOutputChannels *data = speeduino_get_runtime_data(context);
    mtx_test_assert_not_null(test_ctx, data, "Runtime data for mapping test");
    
    /* Test conversions and calculations */
    gfloat battery_voltage = data->battery10 / 10.0f;
    mtx_test_assert_float_equals(test_ctx, battery_voltage, 14.0f, 0.1f, "Battery voltage conversion");
    
    gfloat afr_value = data->o2 / 10.0f;
    mtx_test_assert_float_equals(test_ctx, afr_value, 14.7f, 0.1f, "AFR conversion");
    
    /* Test range validation */
    mtx_test_assert_true(test_ctx, data->rpm >= 0 && data->rpm <= 20000, "RPM in valid range");
    mtx_test_assert_true(test_ctx, data->map >= 0 && data->map <= 500, "MAP in valid range");
    mtx_test_assert_true(test_ctx, data->tps >= 0 && data->tps <= 100, "TPS in valid range");
    
    /* Cleanup */
    speeduino_disconnect_device(context);
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test configuration operations */
static void test_speeduino_configuration(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino configuration operations");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    speeduino_connect_device(context, "/dev/mock_speeduino", 115200, NULL);
    
    /* Test configuration value setting */
    GError *error = NULL;
    gboolean result = speeduino_set_config_value(context, 1, 10, 85, &error);
    mtx_test_assert_true(test_ctx, result, "Set config value");
    mtx_test_assert_null(test_ctx, error, "No config set error");
    
    /* Test invalid page */
    result = speeduino_set_config_value(context, 999, 10, 85, &error);
    mtx_test_assert_false(test_ctx, result, "Invalid page rejected");
    mtx_test_assert_not_null(test_ctx, error, "Error returned for invalid page");
    g_clear_error(&error);
    
    /* Cleanup */
    speeduino_disconnect_device(context);
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test logging functions */
static void test_speeduino_logging(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino logging functions");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    speeduino_connect_device(context, "/dev/mock_speeduino", 115200, NULL);
    
    /* Test tooth logging */
    GError *error = NULL;
    gboolean result = speeduino_start_tooth_logging(context, &error);
    mtx_test_assert_true(test_ctx, result, "Start tooth logging");
    mtx_test_assert_null(test_ctx, error, "No tooth logging start error");
    
    result = speeduino_stop_tooth_logging(context, &error);
    mtx_test_assert_true(test_ctx, result, "Stop tooth logging");
    mtx_test_assert_null(test_ctx, error, "No tooth logging stop error");
    
    /* Cleanup */
    speeduino_disconnect_device(context);
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test auto-detection */
static void test_speeduino_auto_detect(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino auto-detection");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    
    /* Test auto-detection (should fail with mock setup) */
    GError *error = NULL;
    gboolean result = speeduino_auto_detect(context, &error);
    mtx_test_assert_false(test_ctx, result, "Auto-detect fails without mock device");
    mtx_test_assert_not_null(test_ctx, error, "Error returned for failed auto-detect");
    g_clear_error(&error);
    
    /* Cleanup */
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test error handling */
static void test_speeduino_error_handling(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino error handling");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    
    /* Test operations without connection */
    const SpeeduinoOutputChannels *data = speeduino_get_runtime_data(context);
    mtx_test_assert_null(test_ctx, data, "No data without connection");
    
    GError *error = NULL;
    gboolean result = speeduino_set_config_value(context, 1, 10, 85, &error);
    mtx_test_assert_false(test_ctx, result, "Config set fails without connection");
    mtx_test_assert_not_null(test_ctx, error, "Error returned without connection");
    g_clear_error(&error);
    
    /* Test invalid device connection */
    result = speeduino_connect_device(context, "/dev/nonexistent", 115200, &error);
    mtx_test_assert_false(test_ctx, result, "Invalid device connection fails");
    mtx_test_assert_not_null(test_ctx, error, "Error returned for invalid device");
    g_clear_error(&error);
    
    /* Cleanup */
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Performance test */
static void test_speeduino_performance(MtxTestContext *test_ctx)
{
    g_test_message("Testing Speeduino performance");
    
    MtxPluginContext *context = mtx_plugin_context_new("speeduino-test");
    speeduino_initialize(context, NULL);
    speeduino_connect_device(context, "/dev/mock_speeduino", 115200, NULL);
    
    /* Time multiple data acquisitions */
    const gint iterations = 100;
    GTimeVal start_time, end_time;
    
    g_get_current_time(&start_time);
    
    for (gint i = 0; i < iterations; i++) {
        const SpeeduinoOutputChannels *data = speeduino_get_runtime_data(context);
        mtx_test_assert_not_null(test_ctx, data, "Runtime data in performance test");
    }
    
    g_get_current_time(&end_time);
    
    gdouble elapsed_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                        (end_time.tv_usec - start_time.tv_usec) / 1000.0;
    gdouble avg_time_ms = elapsed_ms / iterations;
    
    g_test_message("Average data acquisition time: %.2f ms", avg_time_ms);
    
    /* Should be able to get data in under 10ms on average */
    mtx_test_assert_true(test_ctx, avg_time_ms < 10.0, "Data acquisition performance");
    
    /* Cleanup */
    speeduino_disconnect_device(context);
    speeduino_cleanup(context);
    mtx_plugin_context_free(context);
}

/* Test suite setup */
void speeduino_plugin_test_suite(MtxTestFramework *framework)
{
    MtxTestSuite *suite = mtx_test_suite_new("Speeduino Plugin Tests");
    
    /* Add test cases */
    mtx_test_suite_add_test(suite, "init", test_speeduino_plugin_init);
    mtx_test_suite_add_test(suite, "communication", test_speeduino_communication);
    mtx_test_suite_add_test(suite, "runtime_data", test_speeduino_runtime_data);
    mtx_test_suite_add_test(suite, "data_mapping", test_speeduino_data_mapping);
    mtx_test_suite_add_test(suite, "configuration", test_speeduino_configuration);
    mtx_test_suite_add_test(suite, "logging", test_speeduino_logging);
    mtx_test_suite_add_test(suite, "auto_detect", test_speeduino_auto_detect);
    mtx_test_suite_add_test(suite, "error_handling", test_speeduino_error_handling);
    mtx_test_suite_add_test(suite, "performance", test_speeduino_performance);
    
    /* Add suite to framework */
    mtx_test_framework_add_suite(framework, suite);
}
