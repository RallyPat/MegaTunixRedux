/*
 * Temporary stub functions for missing functionality in MegaTunix Redux
 * These provide basic no-op implementations to allow linking and basic operation
 */

#include <gtk/gtk.h>
#include <glib.h>
#include "defines.h"
#include "args.h"
#include "speeduino_plugin.h"
#include "ecu_manager.h"
#include "ecu_detector.h"
#include "plugin_system.h"

extern gconstpointer *global_data;

// Global variables for runtime simulation
static guint simulation_timeout_id = 0;

// Structure for passing ECU detection results between threads
typedef struct {
    GtkBuilder *builder;
    gboolean success;
    gchar *error_message;
} EcuDetectionThreadResult;

// Structure for passing user selections to detection thread
typedef struct {
    GtkBuilder *builder;
    gchar *selected_port;
    gchar *selected_baud;
    gboolean auto_detect_port;
    gboolean auto_detect_baud;
} EcuConnectionRequest;

// Helper function prototypes
static gboolean simulate_connection_result(gpointer user_data);
static gboolean update_runtime_data_simulation(gpointer user_data);
static gboolean update_runtime_data_real(gpointer user_data);
static gboolean simulate_interrogation_progress(gpointer user_data);
static gpointer ecu_detection_thread(gpointer user_data);
static gboolean ecu_detection_thread_complete(gpointer user_data);
static gboolean start_ecu_detection(gpointer user_data);
static gboolean ecu_detection_success(gpointer user_data);
static gboolean offer_simulation_mode(gpointer user_data);

// Natural sorting comparison function for device names
static gint natural_string_compare(gconstpointer a, gconstpointer b)
{
    const gchar *str1 = (const gchar*)a;
    const gchar *str2 = (const gchar*)b;
    
    // Extract numeric parts for natural sorting
    const gchar *ptr1 = str1;
    const gchar *ptr2 = str2;
    
    while (*ptr1 && *ptr2) {
        if (g_ascii_isdigit(*ptr1) && g_ascii_isdigit(*ptr2)) {
            // Both are digits, compare numerically
            glong num1 = g_ascii_strtoll(ptr1, (gchar**)&ptr1, 10);
            glong num2 = g_ascii_strtoll(ptr2, (gchar**)&ptr2, 10);
            
            if (num1 != num2) {
                return (num1 < num2) ? -1 : 1;
            }
        } else if (*ptr1 == *ptr2) {
            // Same character, move to next
            ptr1++;
            ptr2++;
        } else {
            // Different characters, compare directly
            return (*ptr1 < *ptr2) ? -1 : 1;
        }
    }
    
    // One string is a prefix of the other
    return (*ptr1 == '\0') ? ((*ptr2 == '\0') ? 0 : -1) : 1;
}

// Helper function to get list of serial ports
static GList *get_serial_ports(void)
{
    GList *devices = NULL;
    
    // Scan /dev/ directory for serial devices
    GDir *dev_dir = g_dir_open("/dev", 0, NULL);
    if (dev_dir) {
        const gchar *filename;
        
        // Collect all potential serial devices
        while ((filename = g_dir_read_name(dev_dir)) != NULL) {
            if (g_str_has_prefix(filename, "ttyUSB") ||
                g_str_has_prefix(filename, "ttyACM") ||
                g_str_has_prefix(filename, "ttyS") ||
                g_str_has_prefix(filename, "ttyAMA") ||
                g_str_has_prefix(filename, "ttyO") ||     /* BeagleBone */
                g_str_has_prefix(filename, "ttymxc") ||   /* i.MX */
                g_str_has_prefix(filename, "ttyTHS")) {   /* Tegra */
                
                gchar *device_path = g_strdup_printf("/dev/%s", filename);
                
                // Check if device exists (serial devices are character devices, not regular files)
                if (g_file_test(device_path, G_FILE_TEST_EXISTS)) {
                    devices = g_list_insert_sorted(devices, device_path, natural_string_compare);
                } else {
                    g_free(device_path);
                }
            }
        }
        g_dir_close(dev_dir);
    } else {
        // Fallback: check common serial device paths
        const gchar *device_paths[] = {
            "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3",
            "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3",
            "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3",
            "/dev/ttyAMA0", "/dev/ttyAMA1",  /* Raspberry Pi */
            NULL
        };
        
        for (gint i = 0; device_paths[i] != NULL; i++) {
            if (g_file_test(device_paths[i], G_FILE_TEST_EXISTS)) {
                devices = g_list_insert_sorted(devices, g_strdup(device_paths[i]), natural_string_compare);
            }
        }
    }
    
    return devices;
}

// Helper function to populate available serial ports
static void populate_serial_ports(GtkComboBoxText *combo)
{
    if (!combo) return;
    
    // Clear existing items (except "Auto Detect")
    gtk_combo_box_text_remove_all(combo);
    
    // Add "Auto Detect" as first option
    gtk_combo_box_text_append(combo, "auto", "Auto Detect");
    
    // Scan /dev/ directory for serial devices
    GDir *dev_dir = g_dir_open("/dev", 0, NULL);
    if (dev_dir) {
        const gchar *filename;
        GList *devices = NULL;
        
        // Collect all potential serial devices
        while ((filename = g_dir_read_name(dev_dir)) != NULL) {
            if (g_str_has_prefix(filename, "ttyUSB") ||
                g_str_has_prefix(filename, "ttyACM") ||
                g_str_has_prefix(filename, "ttyS") ||
                g_str_has_prefix(filename, "ttyAMA") ||
                g_str_has_prefix(filename, "ttyO") ||     /* BeagleBone */
                g_str_has_prefix(filename, "ttymxc") ||   /* i.MX */
                g_str_has_prefix(filename, "ttyTHS")) {   /* Tegra */
                
                gchar *device_path = g_strdup_printf("/dev/%s", filename);
                
                // Check if device exists (serial devices are character devices, not regular files)
                if (g_file_test(device_path, G_FILE_TEST_EXISTS)) {
                    devices = g_list_insert_sorted(devices, device_path, natural_string_compare);
                } else {
                    g_free(device_path);
                }
            }
        }
        g_dir_close(dev_dir);
        
        // Add sorted devices to combo box
        for (GList *item = devices; item != NULL; item = item->next) {
            const gchar *device_path = (const gchar *)item->data;
            gtk_combo_box_text_append(combo, device_path, device_path);
        }
        
        // Clean up
        g_list_free_full(devices, g_free);
    } else {
        // Fallback: check common serial device paths
        const gchar *device_paths[] = {
            "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3",
            "/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyACM2", "/dev/ttyACM3",
            "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3",
            "/dev/ttyAMA0", "/dev/ttyAMA1",  /* Raspberry Pi */
            NULL
        };
        
        for (gint i = 0; device_paths[i] != NULL; i++) {
            if (g_file_test(device_paths[i], G_FILE_TEST_EXISTS)) {
                gtk_combo_box_text_append(combo, device_paths[i], device_paths[i]);
            }
        }
    }
    
    // Set "Auto Detect" as default
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
}

// Helper function to populate baud rates
static void populate_baud_rates(GtkComboBoxText *combo)
{
    if (!combo) return;
    
    // Clear existing items
    gtk_combo_box_text_remove_all(combo);
    
    // Add "Auto Detect" as first option
    gtk_combo_box_text_append(combo, "auto", "Auto Detect");
    
    // Add standard baud rates
    gtk_combo_box_text_append(combo, "115200", "115200");
    gtk_combo_box_text_append(combo, "57600", "57600");
    gtk_combo_box_text_append(combo, "38400", "38400");
    gtk_combo_box_text_append(combo, "19200", "19200");
    gtk_combo_box_text_append(combo, "9600", "9600");
    
    // Set "Auto Detect" as default
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
}

// Dashboard stub functions - temporarily disabled for GTK4 event system porting

G_MODULE_EXPORT void dash_set_chooser_button_defaults(GtkFileChooser *chooser)
{
    // Stub - dashboard functionality temporarily disabled
    g_debug("dash_set_chooser_button_defaults called but dashboard support is temporarily disabled");
}

G_MODULE_EXPORT gboolean remove_dashboard(GtkWidget *widget, gpointer data)
{
    // Stub - dashboard functionality temporarily disabled  
    g_debug("remove_dashboard called but dashboard support is temporarily disabled");
    return FALSE;
}

G_MODULE_EXPORT void update_dashboards(void)
{
    // Stub - dashboards temporarily disabled
    g_debug("update_dashboards called but dashboard support is temporarily disabled");
}

G_MODULE_EXPORT void print_dash_choices(void)
{
    g_print("Dashboard choices: (dashboard support temporarily disabled)\n");
}

G_MODULE_EXPORT gboolean validate_dash_choice(const gchar *choice)
{
    g_debug("validate_dash_choice called but dashboard support is temporarily disabled");
    return FALSE;
}

G_MODULE_EXPORT GtkWidget* load_dashboard(const gchar *filename, gint index)
{
    // Stub - dashboard functionality temporarily disabled
    g_debug("load_dashboard called but dashboard support is temporarily disabled");
    return NULL;
}

G_MODULE_EXPORT void initialize_dashboards_pf(void)
{
    // Stub - dashboard functionality temporarily disabled
    g_debug("initialize_dashboards_pf called but dashboard support is temporarily disabled");
}

// Logviewer stub functions
G_MODULE_EXPORT void read_logviewer_defaults(gpointer cfgfile)
{
    // Stub - logviewer functionality disabled
    g_warning("read_logviewer_defaults called but logviewer support is disabled");
}

G_MODULE_EXPORT void write_logviewer_defaults(gpointer cfgfile)
{
    // Stub - logviewer functionality disabled
    g_warning("write_logviewer_defaults called but logviewer support is disabled");
}

G_MODULE_EXPORT void set_logviewer_mode(gboolean mode)
{
    // Stub - logviewer functionality disabled
    g_warning("set_logviewer_mode called but logviewer support is disabled");
}

G_MODULE_EXPORT void present_viewer_choices(void)
{
    // Stub - logviewer functionality disabled
    g_warning("present_viewer_choices called but logviewer support is disabled");
}

G_MODULE_EXPORT gboolean lv_configure_event(GtkWidget *widget, gpointer event, gpointer data)
{
    // Stub - logviewer functionality disabled
    g_warning("lv_configure_event called but logviewer support is disabled");
    return FALSE;
}

G_MODULE_EXPORT gboolean pb_update_logview_traces_wrapper(gpointer data)
{
    // Stub - logviewer functionality disabled
    return FALSE;
}

// Memory management stub functions  
G_MODULE_EXPORT void free_multi_source(gpointer data)
{
    // Stub - memory management functionality limited
    if (data)
        g_free(data);
}

// Table/import/export stub functions
G_MODULE_EXPORT void export_single_table(void)
{
    g_warning("export_single_table called but table export is disabled");
}

G_MODULE_EXPORT void import_single_table(void)
{
    g_warning("import_single_table called but table import is disabled");
}

G_MODULE_EXPORT void rescale_table(void)
{
    g_warning("rescale_table called but table rescaling is disabled");
}

// Runtime display stub functions
G_MODULE_EXPORT void update_rtsliders(void)
{
    // Stub - runtime sliders disabled
}

G_MODULE_EXPORT void update_rttext(void)
{
    // Stub - runtime text disabled  
}

// Dashboard functions are now enabled in dashboard.c

// Additional stub functions

G_MODULE_EXPORT gboolean close_dash(GtkWidget *widget, gpointer data)
{
    // Stub - dashboard functionality temporarily disabled
    g_debug("close_dash called but dashboard support is temporarily disabled");
    return FALSE;
}

G_MODULE_EXPORT gboolean present_dash_filechooser(GtkWidget *widget, gpointer data)
{
    // Stub - dashboard functionality temporarily disabled
    g_debug("present_dash_filechooser called but dashboard support is temporarily disabled");
    return FALSE;
}

// Tab loading stub functions - removed conflicting definitions
// load_actual_tab is now in tabloader.c

// Tab loading support functions - removed conflicting definition
// populate_master is now in widgetmgmt.c

G_MODULE_EXPORT void bind_keys(GtkWidget *widget, gpointer data)
{
    g_warning("bind_keys called but implementation is disabled");
}

G_MODULE_EXPORT void load_tags(GtkWidget *widget, gpointer data)
{
    g_warning("load_tags called but implementation is disabled");
}

G_MODULE_EXPORT void combo_setup(GtkWidget *widget, gpointer data)
{
    g_warning("combo_setup called but implementation is disabled");
}

// Gauge stub functions  
G_MODULE_EXPORT GType mtx_gauge_face_get_type(void)
{
    // Return a basic GObject type as fallback
    return G_TYPE_OBJECT;
}

G_MODULE_EXPORT void mtx_gauge_face_set_value(GtkWidget *gauge, gdouble value)
{
    g_warning("mtx_gauge_face_set_value called but gauge support is disabled");
}

// Color functions stub
G_MODULE_EXPORT void get_colors_from_hue(gdouble hue, gpointer color1, gpointer color2)
{
    // Set to basic colors as fallback
    g_warning("get_colors_from_hue called but color support is disabled");
    // color1 and color2 are expected to be GdkColor pointers in legacy code
    // but we can't use them since GdkColor was removed in GTK4
}

// File API stub
G_MODULE_EXPORT gpointer get_file_api(void)
{
    g_warning("get_file_api called but file API is disabled");
    return NULL;
}

// List binding stub - removed conflicting definition
// bind_to_lists is now in tabloader.c

// Dashboard/argument stub functions
// Dashboard-related functions - temporarily disabled for GTK4 porting

// Data processing stub functions - removed conflicting definition
// run_post_functions is now in tabloader.c
// update_errcounts_wrapper is now in comms_gui.c

// Thread communication stub
G_MODULE_EXPORT void write_data(gpointer data)
{
    g_warning("write_data called but data writing is disabled");
}

// Communication stub  
G_MODULE_EXPORT void load_comm_xml(const gchar *filename)
{
    g_warning("load_comm_xml called but communication XML loading is disabled");
}

// Widget state management stub - removed conflicting definition
// alter_widget_state is now in widgetmgmt.c

// Multiplier calculation stub - removed conflicting definition  
// get_multiplier is now in widgetmgmt.c

// Widget sensitivity stub - removed conflicting definition
// set_widget_sensitive is now in widgetmgmt.c

G_MODULE_EXPORT void update_ve3d_if_necessary(void)
{
    // Stub - 3D VE table disabled
}

// Widget management stub functions have been moved to widgetmgmt.c
// alter_widget_state, get_multiplier, set_widget_sensitive, populate_master are now in widgetmgmt.c

// Simple GUI handlers for main UI  
G_MODULE_EXPORT void on_connect_clicked(GtkButton *button, gpointer user_data)
{
    g_message("Connect button clicked - starting ECU detection");
    
    // Debug output
    g_message("global_data pointer: %p", global_data);
    if (!global_data) {
        g_critical("global_data is NULL! Cannot proceed with connection");
        return;
    }
    
    // Get the builder
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    g_message("Retrieved builder from global_data: %p", builder);
    
    if (!builder) {
        g_warning("Builder is NULL! Cannot proceed with connection");
        // Try to get builder from user_data as fallback
        if (user_data && GTK_IS_BUILDER(user_data)) {
            builder = GTK_BUILDER(user_data);
            g_message("Using builder from user_data: %p", builder);
        } else {
            g_critical("No valid builder available, cannot proceed");
            return;
        }
    }
    
    // Get UI widgets
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
    GtkWidget *port_combo = GTK_WIDGET(gtk_builder_get_object(builder, "port_combo"));
    GtkWidget *baud_combo = GTK_WIDGET(gtk_builder_get_object(builder, "baud_combo"));
    GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
    GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
    
    // Check what user has selected BEFORE updating dropdowns
    gchar *selected_port = NULL;
    gchar *selected_baud = NULL;
    gboolean auto_detect_port = TRUE;
    gboolean auto_detect_baud = TRUE;
    
    if (port_combo) {
        selected_port = gtk_combo_box_get_active_id(GTK_COMBO_BOX(port_combo));
        if (selected_port && g_strcmp0(selected_port, "auto") != 0) {
            auto_detect_port = FALSE;
            g_message("User selected specific port: %s", selected_port);
        }
    }
    
    if (baud_combo) {
        selected_baud = gtk_combo_box_get_active_id(GTK_COMBO_BOX(baud_combo));
        if (selected_baud && g_strcmp0(selected_baud, "auto") != 0) {
            auto_detect_baud = FALSE;
            g_message("User selected specific baud: %s", selected_baud);
        }
    }
    
    // Update the dropdowns to show current available devices but preserve selection
    if (port_combo) {
        populate_serial_ports(GTK_COMBO_BOX_TEXT(port_combo));
        // Restore user's selection if it was manual
        if (selected_port && g_strcmp0(selected_port, "auto") != 0) {
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(port_combo), selected_port);
        }
    }
    if (baud_combo) {
        populate_baud_rates(GTK_COMBO_BOX_TEXT(baud_combo));
        // Restore user's selection if it was manual
        if (selected_baud && g_strcmp0(selected_baud, "auto") != 0) {
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(baud_combo), selected_baud);
        }
    }
    
    // Clear previous information
    if (ecu_sig_label) {
        gtk_label_set_text(GTK_LABEL(ecu_sig_label), "Signature: Unknown");
    }
    if (fw_ver_label) {
        gtk_label_set_text(GTK_LABEL(fw_ver_label), "Version: Unknown");
    }
    
    // Update UI for detection phase
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "Detecting ECUs... Please Wait");
    }
    
    // Disable connect button during detection
    if (connect_btn) {
        gtk_widget_set_sensitive(connect_btn, FALSE);
    }
    
    // Start ECU detection process
    g_message("Starting ECU detection...");
    
    // Initialize ECU manager
    g_message("Attempting to initialize ECU manager...");
    
    gboolean ecu_manager_init_result = ecu_manager_initialize();
    g_message("ECU manager initialization result: %s", ecu_manager_init_result ? "SUCCESS" : "FAILED");
    
    if (!ecu_manager_init_result) {
        g_warning("Failed to initialize ECU manager");
        
        // Show error and reset UI
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "ECU Manager initialization failed");
        }
        if (connect_btn) {
            gtk_widget_set_sensitive(connect_btn, TRUE);
        }
        
        // Do not fall back to simulation mode - user must have real hardware
        g_message("ECU manager initialization failed - simulation mode is disabled");
        return;
    }
    
    // Run ECU detection in background thread to avoid blocking UI
    EcuConnectionRequest *request = g_new0(EcuConnectionRequest, 1);
    request->builder = builder;
    request->selected_port = g_strdup(selected_port);
    request->selected_baud = g_strdup(selected_baud);
    request->auto_detect_port = auto_detect_port;
    request->auto_detect_baud = auto_detect_baud;
    
    g_thread_new("ecu_detection", ecu_detection_thread, request);
}

// Background thread for ECU detection
static gpointer ecu_detection_thread(gpointer user_data)
{
    EcuConnectionRequest *request = (EcuConnectionRequest *)user_data;
    if (!request || !request->builder) {
        if (request) {
            g_free(request->selected_port);
            g_free(request->selected_baud);
            g_free(request);
        }
        return NULL;
    }
    
    GtkBuilder *builder = request->builder;
    
    g_message("Running ECU detection scan in background thread...");
    g_message("Connection request: port=%s (auto=%s), baud=%s (auto=%s)",
             request->selected_port ? request->selected_port : "NULL",
             request->auto_detect_port ? "YES" : "NO",
             request->selected_baud ? request->selected_baud : "NULL",
             request->auto_detect_baud ? "YES" : "NO");
    
    // Add timeout protection - if detection takes more than 10 seconds, abort
    GTimer *timer = g_timer_new();
    g_timer_start(timer);
    
    // Try to connect using user's selection or auto-detect
    GError *error = NULL;
    gboolean connection_result = FALSE;
    
    if (request->auto_detect_port && request->auto_detect_baud) {
        // Full auto-detect mode
        g_message("Using full auto-detect mode");
        connection_result = ecu_manager_auto_connect(&error);
    } else if (!request->auto_detect_port && !request->auto_detect_baud) {
        // Full manual mode - both port and baud specified
        g_message("Manual connection mode: port=%s, baud=%s", 
                 request->selected_port, request->selected_baud);
        gint baud_rate = g_ascii_strtoll(request->selected_baud, NULL, 10);
        g_message("🔧 Parsed baud rate: %d from string '%s'", baud_rate, request->selected_baud);
        if (baud_rate == 0) {
            g_warning("🔧 Failed to parse baud rate from '%s'", request->selected_baud);
        }
        connection_result = ecu_manager_manual_connect(request->selected_port, baud_rate, &error);
    } else {
        // Mixed mode - implement proper mixed mode support
        g_message("Mixed manual/auto mode: port=%s (auto=%s), baud=%s (auto=%s)", 
                 request->selected_port ? request->selected_port : "auto",
                 request->auto_detect_port ? "YES" : "NO",
                 request->selected_baud ? request->selected_baud : "auto",
                 request->auto_detect_baud ? "YES" : "NO");
        
        if (!request->auto_detect_port && request->auto_detect_baud) {
            // Manual port, auto baud - try common baud rates on specified port
            g_message("Trying manual port %s with auto baud detection", request->selected_port);
            gint common_bauds[] = {115200, 57600, 38400, 19200, 9600, 0};
            for (gint i = 0; common_bauds[i] != 0; i++) {
                g_message("Trying %s at %d baud...", request->selected_port, common_bauds[i]);
                connection_result = ecu_manager_manual_connect(request->selected_port, common_bauds[i], &error);
                if (connection_result) {
                    g_message("✅ Connected to %s at %d baud", request->selected_port, common_bauds[i]);
                    break;
                }
                if (error) {
                    g_clear_error(&error);
                }
            }
        } else if (request->auto_detect_port && !request->auto_detect_baud) {
            // Auto port, manual baud - scan all ports with specified baud
            g_message("Trying auto port detection with manual baud %s", request->selected_baud);
            gint baud_rate = g_ascii_strtoll(request->selected_baud, NULL, 10);
            
            // Get list of serial ports and try each one
            GList *serial_ports = get_serial_ports();
            for (GList *l = serial_ports; l != NULL; l = l->next) {
                const gchar *port = (const gchar *)l->data;
                g_message("Trying %s at %d baud...", port, baud_rate);
                connection_result = ecu_manager_manual_connect(port, baud_rate, &error);
                if (connection_result) {
                    g_message("✅ Connected to %s at %d baud", port, baud_rate);
                    break;
                }
                if (error) {
                    g_clear_error(&error);
                }
            }
            g_list_free_full(serial_ports, g_free);
        }
    }
    
    gdouble elapsed = g_timer_elapsed(timer, NULL);
    g_timer_destroy(timer);
    
    g_message("ECU connection attempt completed in %.2f seconds", elapsed);
    g_message("Connection result: %s", connection_result ? "SUCCESS" : "FAILED");
    if (error) {
        g_message("Error details: %s", error->message);
    }
    
    // Create result data for main thread callback
    EcuDetectionThreadResult *result = g_new0(EcuDetectionThreadResult, 1);
    result->builder = builder;
    result->success = connection_result;
    result->error_message = error ? g_strdup(error->message) : NULL;
    
    g_clear_error(&error);
    
    // Clean up request
    g_free(request->selected_port);
    g_free(request->selected_baud);
    g_free(request);
    
    // Update UI from main thread
    g_idle_add((GSourceFunc)ecu_detection_thread_complete, result);
    
    return NULL;
}

// Callback to handle ECU detection completion from background thread
static gboolean ecu_detection_thread_complete(gpointer user_data)
{
    EcuDetectionThreadResult *result = (EcuDetectionThreadResult *)user_data;
    if (!result) return FALSE;
    
    GtkBuilder *builder = result->builder;
    if (!builder) {
        g_free(result->error_message);
        g_free(result);
        return FALSE;
    }
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    
    if (result->success) {
        g_message("ECU detection and connection successful!");
        g_timeout_add(100, (GSourceFunc)ecu_detection_success, builder);
    } else {
        g_message("ECU detection failed: %s", result->error_message ? result->error_message : "No ECUs found");
        
        // Show failure and reset UI
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "No ECUs detected");
        }
        if (connect_btn) {
            gtk_widget_set_sensitive(connect_btn, TRUE);
        }
        
        // Do not offer simulation mode - user must have real hardware
        g_message("Connection failed - simulation mode is disabled");
    }
    
    g_free(result->error_message);
    g_free(result);
    return FALSE;
}

// Helper function to start ECU detection (legacy, now unused)
static gboolean start_ecu_detection(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
    GtkWidget *port_combo = GTK_WIDGET(gtk_builder_get_object(builder, "port_combo"));
    GtkWidget *baud_combo = GTK_WIDGET(gtk_builder_get_object(builder, "baud_combo"));
    GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
    GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
    
    g_message("Running ECU detection scan...");
    
    // Add debug output
    g_message("About to call ecu_manager_auto_connect...");
    
    // Try to auto-connect (this includes detection)
    GError *error = NULL;
    gboolean connection_result = ecu_manager_auto_connect(&error);
    
    g_message("ecu_manager_auto_connect returned: %s", connection_result ? "SUCCESS" : "FAILED");
    if (error) {
        g_message("Error details: %s", error->message);
    }
    
    if (connection_result) {
        g_message("ECU detection and connection successful!");
        g_timeout_add(100, (GSourceFunc)ecu_detection_success, builder);
    } else {
        g_message("ECU detection failed: %s", error ? error->message : "No ECUs found");
        g_clear_error(&error);
        
        // Show failure and reset UI
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "No ECUs detected");
        }
        if (connect_btn) {
            gtk_widget_set_sensitive(connect_btn, TRUE);
        }
        
        // Do not offer simulation mode - user must have real hardware
        g_message("Connection failed - simulation mode is disabled");
    }
    
    return FALSE; // Remove timeout
}

// Helper function to handle successful ECU detection
static gboolean ecu_detection_success(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
    GtkWidget *port_combo = GTK_WIDGET(gtk_builder_get_object(builder, "port_combo"));
    GtkWidget *baud_combo = GTK_WIDGET(gtk_builder_get_object(builder, "baud_combo"));
    GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
    GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
    
    // Get current ECU info
    const EcuDetectionResult *current_ecu = ecu_manager_get_current_ecu();
    if (!current_ecu) {
        g_warning("No current ECU available after connection");
        // Reset UI to allow retry
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Connection failed");
        }
        if (connect_btn) {
            gtk_widget_set_sensitive(connect_btn, TRUE);
        }
        return FALSE;
    }
    
    // Check if we can read the firmware version - this validates real communication
    const gchar *firmware_version = ecu_manager_get_firmware_version();
    if (!firmware_version) {
        g_message("Firmware version not available yet, keeping detection message...");
        // Keep showing detection message and retry after a short delay
        g_timeout_add(500, (GSourceFunc)ecu_detection_success, builder);
        return FALSE;
    }
    
    g_message("Firmware version available: %s - connection fully validated", firmware_version);
    
    // Update status - now we know we can actually communicate
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "Connected (Real Hardware)");
    }
    
    // Update button states
    if (connect_btn) {
        gtk_widget_set_sensitive(connect_btn, FALSE);
    }
    if (disconnect_btn) {
        gtk_widget_set_sensitive(disconnect_btn, TRUE);
    }
    
    // Show detected device info
    if (port_combo) {
        // Clear and add the detected port
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(port_combo));
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(port_combo), 
                                current_ecu->device_path, current_ecu->device_path);
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(port_combo), current_ecu->device_path);
    }
    
    if (baud_combo) {
        // Clear and add the detected baud rate
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(baud_combo));
        gchar *baud_str = g_strdup_printf("%d", current_ecu->baud_rate);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(baud_combo), baud_str, baud_str);
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(baud_combo), baud_str);
        g_free(baud_str);
    }
    
    if (ecu_sig_label) {
        gchar *sig_text = g_strdup_printf("Signature: %s", current_ecu->signature);
        gtk_label_set_text(GTK_LABEL(ecu_sig_label), sig_text);
        g_free(sig_text);
    }
    
    if (fw_ver_label) {
        gchar *ver_text = g_strdup_printf("Version: %s", firmware_version);
        gtk_label_set_text(GTK_LABEL(fw_ver_label), ver_text);
        g_free(ver_text);
    }
    
    g_message("Connected to %s at %s (%d baud) - firmware %s", 
             current_ecu->ecu_name, current_ecu->device_path, current_ecu->baud_rate, firmware_version);
    
    // Start real runtime data updates
    g_timeout_add(1000, (GSourceFunc)update_runtime_data_real, builder);
    
    return FALSE; // Remove timeout
}

// Helper function to offer simulation mode when no ECUs found
static gboolean offer_simulation_mode(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "No ECUs found - Starting simulation mode");
    }
    
    g_message("No real ECUs detected, starting simulation mode");
    
    // Start simulation after a brief delay
    g_timeout_add(1000, (GSourceFunc)simulate_connection_result, builder);
    
    return FALSE; // Remove timeout
}

// Helper function to simulate connection result
static gboolean simulate_connection_result(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
    GtkWidget *port_combo = GTK_WIDGET(gtk_builder_get_object(builder, "port_combo"));
    GtkWidget *baud_combo = GTK_WIDGET(gtk_builder_get_object(builder, "baud_combo"));
    GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
    GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
    
    // Simulate successful connection (for demo)
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "Connected (Simulation)");
    }
    
    // Update button states
    if (connect_btn) {
        gtk_widget_set_sensitive(connect_btn, FALSE);
    }
    if (disconnect_btn) {
        gtk_widget_set_sensitive(disconnect_btn, TRUE);
    }
    
    // Show simulated device info
    if (port_combo) {
        // Add and select simulation mode
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(port_combo), "simulation", "Simulation Mode");
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(port_combo), "simulation");
    }
    
    if (baud_combo) {
        // Select 115200 for simulation
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(baud_combo), "115200");
    }
    
    // Update ECU info labels
    if (ecu_sig_label) {
        gtk_label_set_text(GTK_LABEL(ecu_sig_label), "Signature: Speeduino 202504-dev (Simulated)");
    }
    if (fw_ver_label) {
        gtk_label_set_text(GTK_LABEL(fw_ver_label), "Version: 202504.13 (Simulated)");
    }
    
    // Start simulated runtime data updates
    g_timeout_add(1000, (GSourceFunc)update_runtime_data_simulation, builder);
    
    return FALSE; // Remove timeout
}

// Helper function to update runtime data from real hardware
static gboolean update_runtime_data_real(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return TRUE; // Continue running
    
    // Check if still connected
    if (!ecu_manager_is_connected()) {
        g_warning("Connection lost, stopping real data updates");
        return FALSE; // Stop updates
    }
    
    // Get runtime data display widgets
    GtkWidget *rpm_value = GTK_WIDGET(gtk_builder_get_object(builder, "rpm_value"));
    GtkWidget *map_value = GTK_WIDGET(gtk_builder_get_object(builder, "map_value"));
    GtkWidget *tps_value = GTK_WIDGET(gtk_builder_get_object(builder, "tps_value"));
    GtkWidget *coolant_value = GTK_WIDGET(gtk_builder_get_object(builder, "coolant_value"));
    GtkWidget *battery_value = GTK_WIDGET(gtk_builder_get_object(builder, "battery_value"));
    GtkWidget *advance_value = GTK_WIDGET(gtk_builder_get_object(builder, "advance_value"));
    
    // Get real runtime data from ECU
    gpointer runtime_data = ecu_manager_get_runtime_data();
    const EcuDetectionResult *ecu = ecu_manager_get_current_ecu();
    
    if (runtime_data && ecu && ecu->ecu_type == ECU_TYPE_SPEEDUINO) {
        // Cast to Speeduino format
        const SpeeduinoOutputChannels *data = (const SpeeduinoOutputChannels *)runtime_data;
        // Update labels with real data
        if (rpm_value) {
            gchar *text = g_strdup_printf("%d", data->rpm);
            gtk_label_set_text(GTK_LABEL(rpm_value), text);
            g_free(text);
        }
        
        if (map_value) {
            gchar *text = g_strdup_printf("%d kPa", data->map);
            gtk_label_set_text(GTK_LABEL(map_value), text);
            g_free(text);
        }
        
        if (tps_value) {
            gchar *text = g_strdup_printf("%d%%", data->tps);
            gtk_label_set_text(GTK_LABEL(tps_value), text);
            g_free(text);
        }
        
        if (coolant_value) {
            gchar *text = g_strdup_printf("%d°C", data->coolant - 40); // Speeduino stores temp + 40
            gtk_label_set_text(GTK_LABEL(coolant_value), text);
            g_free(text);
        }
        
        if (battery_value) {
            gdouble voltage = data->battery10 / 10.0; // Battery voltage * 10
            gchar *text = g_strdup_printf("%.1fV", voltage);
            gtk_label_set_text(GTK_LABEL(battery_value), text);
            g_free(text);
        }
        
        if (advance_value) {
            gint advance = data->advance - 40; // Speeduino stores advance + 40
            gchar *text = g_strdup_printf("%d°", advance);
            gtk_label_set_text(GTK_LABEL(advance_value), text);
            g_free(text);
        }
    } else {
        // Update with error message if data not available
        if (rpm_value) {
            gtk_label_set_text(GTK_LABEL(rpm_value), "ERROR");
        }
        if (map_value) {
            gtk_label_set_text(GTK_LABEL(map_value), "ERROR");
        }
        if (tps_value) {
            gtk_label_set_text(GTK_LABEL(tps_value), "ERROR");
        }
        if (coolant_value) {
            gtk_label_set_text(GTK_LABEL(coolant_value), "ERROR");
        }
        if (battery_value) {
            gtk_label_set_text(GTK_LABEL(battery_value), "ERROR");
        }
        if (advance_value) {
            gtk_label_set_text(GTK_LABEL(advance_value), "ERROR");
        }
    }
    
    return TRUE; // Continue running
}

// Helper function to update runtime data simulation
static gboolean update_runtime_data_simulation(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return TRUE; // Continue running
    
    // Get runtime data display widgets
    GtkWidget *rpm_value = GTK_WIDGET(gtk_builder_get_object(builder, "rpm_value"));
    GtkWidget *map_value = GTK_WIDGET(gtk_builder_get_object(builder, "map_value"));
    GtkWidget *tps_value = GTK_WIDGET(gtk_builder_get_object(builder, "tps_value"));
    GtkWidget *coolant_value = GTK_WIDGET(gtk_builder_get_object(builder, "coolant_value"));
    GtkWidget *battery_value = GTK_WIDGET(gtk_builder_get_object(builder, "battery_value"));
    GtkWidget *advance_value = GTK_WIDGET(gtk_builder_get_object(builder, "advance_value"));
    
    // Generate simulated data
    static int sim_counter = 0;
    sim_counter++;
    
    gint rpm = 800 + (sim_counter % 100) * 20; // Simulate idle fluctuation
    gint map = 30 + (sim_counter % 10) * 2;   // Simulate MAP changes
    gint tps = (sim_counter % 50) > 25 ? 5 + (sim_counter % 20) : 0; // Simulate occasional throttle
    gint coolant = 85 + (sim_counter % 20);   // Simulate temperature changes
    gdouble battery = 12.0 + (sim_counter % 100) / 100.0; // Simulate voltage
    gint advance = 15 + (sim_counter % 10);   // Simulate timing changes
    
    // Update labels
    if (rpm_value) {
        gchar *text = g_strdup_printf("%d", rpm);
        gtk_label_set_text(GTK_LABEL(rpm_value), text);
        g_free(text);
    }
    
    if (map_value) {
        gchar *text = g_strdup_printf("%d", map);
        gtk_label_set_text(GTK_LABEL(map_value), text);
        g_free(text);
    }
    
    if (tps_value) {
        gchar *text = g_strdup_printf("%d", tps);
        gtk_label_set_text(GTK_LABEL(tps_value), text);
        g_free(text);
    }
    
    if (coolant_value) {
        gchar *text = g_strdup_printf("%d", coolant);
        gtk_label_set_text(GTK_LABEL(coolant_value), text);
        g_free(text);
    }
    
    if (battery_value) {
        gchar *text = g_strdup_printf("%.1f", battery);
        gtk_label_set_text(GTK_LABEL(battery_value), text);
        g_free(text);
    }
    
    if (advance_value) {
        gchar *text = g_strdup_printf("%d", advance);
        gtk_label_set_text(GTK_LABEL(advance_value), text);
        g_free(text);
    }
    
    return TRUE; // Continue running
}

G_MODULE_EXPORT void on_settings_clicked(GtkButton *button, gpointer user_data)
{
    // Settings button handler
    g_message("Settings button clicked - opening settings dialog");
    
    // Debug output
    g_message("Settings button handler called! Button: %p, user_data: %p", button, user_data);
    
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    g_message("Builder from global_data: %p", builder);
    
    if (!builder) {
        g_warning("Builder is NULL! Cannot open settings dialog");
        return;
    }
    
    if (builder) {
        GtkWidget *main_window = GTK_WIDGET(gtk_builder_get_object(builder, "mtx_main_window"));
        
        g_message("Found main window: %p", main_window);
        
        // Create a simple settings dialog
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Settings",
            GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            "_Close", GTK_RESPONSE_CLOSE,
            NULL);
        
        g_message("Created settings dialog: %p", dialog);
        
        // Add content to the dialog
        GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
        
        // Add some settings widgets
        GtkWidget *label = gtk_label_new("MegaTunix Redux Settings");
        gtk_box_append(GTK_BOX(vbox), label);
        
        GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_append(GTK_BOX(vbox), sep);
        
        GtkWidget *comm_frame = gtk_frame_new("Communication");
        GtkWidget *comm_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(comm_grid), 6);
        gtk_grid_set_column_spacing(GTK_GRID(comm_grid), 12);
        
        GtkWidget *port_label = gtk_label_new("Serial Port:");
        GtkWidget *port_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(port_entry), "/dev/ttyUSB0");
        
        GtkWidget *baud_label = gtk_label_new("Baud Rate:");
        GtkWidget *baud_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(baud_entry), "115200");
        
        gtk_grid_attach(GTK_GRID(comm_grid), port_label, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(comm_grid), port_entry, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(comm_grid), baud_label, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(comm_grid), baud_entry, 1, 1, 1, 1);
        
        gtk_frame_set_child(GTK_FRAME(comm_frame), comm_grid);
        gtk_box_append(GTK_BOX(vbox), comm_frame);
        
        GtkWidget *info_label = gtk_label_new("Note: Settings will be saved to configuration file when implemented.");
        gtk_box_append(GTK_BOX(vbox), info_label);
        
        gtk_box_append(GTK_BOX(content_area), vbox);
        
        // Show the dialog
        gtk_widget_show(dialog);
        
        // Connect the response signal
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        
        g_message("Settings dialog should now be visible");
    }
}

G_MODULE_EXPORT void on_disconnect_clicked(GtkButton *button, gpointer user_data)
{
    g_message("Disconnect button clicked - disconnecting from ECU");
    
    // Disconnect from ECU using universal manager
    if (ecu_manager_is_connected()) {
        ecu_manager_disconnect();
        g_message("Disconnected from ECU");
    }
    
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    if (builder) {
        GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
        GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
        GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
        GtkWidget *port_combo = GTK_WIDGET(gtk_builder_get_object(builder, "port_combo"));
        GtkWidget *baud_combo = GTK_WIDGET(gtk_builder_get_object(builder, "baud_combo"));
        GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
        GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
        
        // Update connection status
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Disconnected");
        }
        
        // Update button states
        if (connect_btn) {
            gtk_widget_set_sensitive(connect_btn, TRUE);
        }
        if (disconnect_btn) {
            gtk_widget_set_sensitive(disconnect_btn, FALSE);
        }
        
        // Reset combo boxes to Auto Detect and refresh available ports
        if (port_combo) {
            populate_serial_ports(GTK_COMBO_BOX_TEXT(port_combo));
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(port_combo), "auto");
        }
        if (baud_combo) {
            populate_baud_rates(GTK_COMBO_BOX_TEXT(baud_combo));
            gtk_combo_box_set_active_id(GTK_COMBO_BOX(baud_combo), "auto");
        }
        
        // Clear ECU info
        if (ecu_sig_label) {
            gtk_label_set_text(GTK_LABEL(ecu_sig_label), "Signature: Unknown");
        }
        if (fw_ver_label) {
            gtk_label_set_text(GTK_LABEL(fw_ver_label), "Version: Unknown");
        }
        
        // Clear runtime data
        GtkWidget *rpm_value = GTK_WIDGET(gtk_builder_get_object(builder, "rpm_value"));
        GtkWidget *map_value = GTK_WIDGET(gtk_builder_get_object(builder, "map_value"));
        GtkWidget *tps_value = GTK_WIDGET(gtk_builder_get_object(builder, "tps_value"));
        GtkWidget *coolant_value = GTK_WIDGET(gtk_builder_get_object(builder, "coolant_value"));
        
        if (rpm_value) gtk_label_set_text(GTK_LABEL(rpm_value), "0");
        if (map_value) gtk_label_set_text(GTK_LABEL(map_value), "0");
        if (tps_value) gtk_label_set_text(GTK_LABEL(tps_value), "0");
        if (coolant_value) gtk_label_set_text(GTK_LABEL(coolant_value), "0");
    }
}

G_MODULE_EXPORT void on_interrogate_clicked(GtkButton *button, gpointer user_data)
{
    g_message("Interrogate button clicked - interrogating ECU");
    
    // Debug output
    g_message("Interrogate button handler called! Button: %p, user_data: %p", button, user_data);
    
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    g_message("Builder from global_data: %p", builder);
    
    if (!builder) {
        g_warning("Builder is NULL! Cannot proceed with interrogation");
        return;
    }
    
    // Check if we're connected first
    if (!ecu_manager_is_connected()) {
        g_warning("Cannot interrogate - not connected to ECU");
        
        // Show error dialog
        GtkWidget *main_window = GTK_WIDGET(gtk_builder_get_object(builder, "mtx_main_window"));
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(main_window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Cannot interrogate ECU - not connected.\n\nPlease connect to ECU first.");
        
        gtk_widget_show(dialog);
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        return;
    }
    
    // Start interrogation process
    g_message("Starting ECU interrogation...");
    
    // Disable interrogate button during process
    if (button) {
        gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
    }
    
    // Start interrogation progress simulation
    g_timeout_add(500, (GSourceFunc)simulate_interrogation_progress, builder);
}

// Helper function to simulate interrogation progress
static gboolean simulate_interrogation_progress(gpointer user_data)
{
    static gdouble progress = 0.0;
    static gint step = 0;
    
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) {
        progress = 0.0;
        step = 0;
        return FALSE;
    }
    
    const gchar *steps[] = {
        "Detecting ECU...",
        "Reading signature...",
        "Loading configuration...",
        "Checking capabilities...",
        "Interrogation complete"
    };
    
    g_message("Interrogation step %d: %s (progress: %.1f%%)", step, steps[step], progress * 100);
    
    if (step < 5) {
        if (progress >= 1.0) {
            // Re-enable interrogate button
            GtkWidget *interrogate_btn = GTK_WIDGET(gtk_builder_get_object(builder, "interrogate_button"));
            if (interrogate_btn) {
                gtk_widget_set_sensitive(interrogate_btn, TRUE);
            }
            
            g_message("Interrogation completed successfully");
            
            // Reset for next time
            progress = 0.0;
            step = 0;
            return FALSE; // Stop the timeout
        }
        
        progress += 0.2;
        step++;
        
        // Continue updating every 500ms
        g_timeout_add(500, (GSourceFunc)simulate_interrogation_progress, builder);
    }
    
    return FALSE;
}

G_MODULE_EXPORT void on_offline_mode_toggled(GtkToggleButton *button, gpointer user_data)
{
    gboolean offline_mode = gtk_toggle_button_get_active(button);
    g_message("Offline mode %s", offline_mode ? "enabled" : "disabled");
    
    // TODO: Implement offline mode logic
    // This would enable/disable certain features when not connected to real hardware
}

G_MODULE_EXPORT void on_debug_mode_toggled(GtkToggleButton *button, gpointer user_data)
{
    gboolean debug_mode = gtk_toggle_button_get_active(button);
    g_message("Debug mode %s", debug_mode ? "enabled" : "disabled");
    
    // TODO: Implement debug mode logic
    // This would enable/disable debug logging and additional diagnostics
}

G_MODULE_EXPORT void on_rt_rate_changed(GtkSpinButton *button, gpointer user_data)
{
    gdouble rate = gtk_spin_button_get_value(button);
    g_message("RT update rate changed to %.1f Hz", rate);
    
    // TODO: Implement actual rate change
    // This would update the runtime data update frequency
}

/* Main window close handler */
G_MODULE_EXPORT gboolean on_main_window_close(GtkWidget *widget, gpointer data)
{
    g_info("Main window close requested");
    
    /* Clean up any resources */
    if (simulation_timeout_id > 0) {
        g_source_remove(simulation_timeout_id);
        simulation_timeout_id = 0;
    }
    
    /* Let the application handle the close */
    return FALSE;  /* Allow the window to close */
}

// Datalogging GUI stubs
G_MODULE_EXPORT void start_datalogging(void) {
    g_message("start_datalogging called (stub)");
}

G_MODULE_EXPORT void stop_datalogging(void) {
    g_message("stop_datalogging called (stub)");
}

G_MODULE_EXPORT void dlog_select_all(void) {
    g_message("dlog_select_all called (stub)");
}

G_MODULE_EXPORT void dlog_deselect_all(void) {
    g_message("dlog_deselect_all called (stub)");
}

G_MODULE_EXPORT void dlog_select_defaults(void) {
    g_message("dlog_select_defaults called (stub)");
}

G_MODULE_EXPORT gboolean internal_datalog_dump(GtkWidget *widget, gpointer data) {
    g_message("internal_datalog_dump called (stub)");
    return FALSE;
}

G_MODULE_EXPORT gboolean autolog_dump(gpointer data) {
    g_message("autolog_dump called (stub)");
    return FALSE;
}

// Initialize the UI combo boxes with available options
G_MODULE_EXPORT void initialize_connection_ui(GtkBuilder *builder)
{
    if (!builder) return;
    
    GtkWidget *port_combo = GTK_WIDGET(gtk_builder_get_object(builder, "port_combo"));
    GtkWidget *baud_combo = GTK_WIDGET(gtk_builder_get_object(builder, "baud_combo"));
    
    // Populate combo boxes and set defaults
    if (port_combo) {
        populate_serial_ports(GTK_COMBO_BOX_TEXT(port_combo));
        // Set Auto Detect as default
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(port_combo), "auto");
    }
    if (baud_combo) {
        populate_baud_rates(GTK_COMBO_BOX_TEXT(baud_combo));
        // Set Auto Detect as default
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(baud_combo), "auto");
    }
}


