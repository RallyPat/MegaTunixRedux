/*
 * Temporary stub functions for missing functionality in MegaTunix Redux
 * These provide basic no-op implementations to allow linking and basic operation
 */

#include <gtk/gtk.h>
#include <glib.h>
#include "defines.h"
#include "args.h"
#include "speeduino_plugin.h"
#include "speeduino_bridge.h"
#include "plugin_system.h"

extern gconstpointer *global_data;

// Global variables for runtime simulation
static guint simulation_timeout_id = 0;

// Helper function prototypes
static gboolean simulate_connection_result(gpointer user_data);
static gboolean attempt_real_connection_result(gpointer user_data);
static gboolean real_connection_success_result(gpointer user_data);
static gboolean update_runtime_data_simulation(gpointer user_data);
static gboolean update_runtime_data_real(gpointer user_data);
static gboolean simulate_interrogation_progress(gpointer user_data);

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
    // Real Speeduino plugin integration
    g_message("Connect button clicked - attempting real ECU connection via Speeduino plugin");
    
    // Debug output
    g_message("Button handler called! Button: %p, user_data: %p", button, user_data);
    
    // Get the builder and update status
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    g_message("Builder from global_data: %p", builder);
    
    if (!builder) {
        g_warning("Builder is NULL! Cannot proceed with connection");
        return;
    }
    
    if (builder) {
        GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
        GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
        GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
        GtkWidget *port_entry = GTK_WIDGET(gtk_builder_get_object(builder, "port_entry"));
        GtkWidget *baud_entry = GTK_WIDGET(gtk_builder_get_object(builder, "baud_entry"));
        
        g_message("Found widgets: status_label=%p, connect_btn=%p, disconnect_btn=%p, port_entry=%p, baud_entry=%p", 
                  status_label, connect_btn, disconnect_btn, port_entry, baud_entry);
        
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Connecting...");
        }
        
        // Disable connect button during attempt
        if (connect_btn) {
            gtk_widget_set_sensitive(connect_btn, FALSE);
        }
        
        // Get connection parameters from UI
        const gchar *device_path = "/dev/ttyUSB0";  // Default
        gint baud_rate = 115200;  // Default
        
        if (port_entry) {
            GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(port_entry));
            if (buffer) {
                const gchar *text = gtk_entry_buffer_get_text(buffer);
                if (text && strlen(text) > 0) {
                    device_path = text;
                }
            }
        }
        
        if (baud_entry) {
            GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(baud_entry));
            if (buffer) {
                const gchar *text = gtk_entry_buffer_get_text(buffer);
                if (text && strlen(text) > 0) {
                    baud_rate = atoi(text);
                }
            }
        }
        
        g_message("Attempting connection to %s at %d baud", device_path, baud_rate);
        
        // Try real connection first, fall back to simulation if no hardware
        gboolean real_connection_attempted = FALSE;
        
        // Check if device exists
        if (g_file_test(device_path, G_FILE_TEST_EXISTS)) {
            real_connection_attempted = TRUE;
            g_message("Serial device found, attempting real connection...");
            
            // Initialize bridge if not already done
            if (!speeduino_bridge_initialize()) {
                g_warning("Failed to initialize Speeduino bridge");
                // Fall back to simulation
                g_timeout_add(500, (GSourceFunc)simulate_connection_result, builder);
                return;
            }
            
            // Try real connection using bridge
            GError *error = NULL;
            if (speeduino_bridge_connect(device_path, baud_rate, &error)) {
                g_message("Real connection successful!");
                g_timeout_add(1000, (GSourceFunc)real_connection_success_result, builder);
            } else {
                g_message("Real connection failed: %s", error ? error->message : "Unknown error");
                g_clear_error(&error);
                // Fall back to simulation
                g_timeout_add(1000, (GSourceFunc)simulate_connection_result, builder);
            }
        } else {
            g_message("Serial device not found, using simulation mode");
            // Immediately fall back to simulation
            g_timeout_add(500, (GSourceFunc)simulate_connection_result, builder);
        }
    }
}

// Helper function to handle real connection success
static gboolean real_connection_success_result(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
    GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
    GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
    
    // Update status
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
    
    // Get and display ECU information
    const gchar *ecu_signature = speeduino_bridge_get_ecu_signature();
    const gchar *firmware_version = speeduino_bridge_get_firmware_version();
    
    if (ecu_sig_label) {
        if (ecu_signature) {
            gchar *sig_text = g_strdup_printf("Signature: %s", ecu_signature);
            gtk_label_set_text(GTK_LABEL(ecu_sig_label), sig_text);
            g_free(sig_text);
        } else {
            gtk_label_set_text(GTK_LABEL(ecu_sig_label), "Signature: Reading...");
        }
    }
    
    if (fw_ver_label) {
        if (firmware_version) {
            gchar *ver_text = g_strdup_printf("Version: %s", firmware_version);
            gtk_label_set_text(GTK_LABEL(fw_ver_label), ver_text);
            g_free(ver_text);
        } else {
            gtk_label_set_text(GTK_LABEL(fw_ver_label), "Version: Reading...");
        }
    }
    
    // Start real runtime data updates
    g_timeout_add(1000, (GSourceFunc)update_runtime_data_real, builder);
    
    return FALSE; // Remove timeout
}

// Helper function to attempt real connection and handle result
static gboolean attempt_real_connection_result(gpointer user_data)
{
    GtkBuilder *builder = GTK_BUILDER(user_data);
    if (!builder) return FALSE;
    
    GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
    GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
    GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
    GtkWidget *ecu_sig_label = GTK_WIDGET(gtk_builder_get_object(builder, "ecu_signature_label"));
    GtkWidget *fw_ver_label = GTK_WIDGET(gtk_builder_get_object(builder, "firmware_version_label"));
    
    // For now, we'll simulate a real connection attempt that fails
    // TODO: Complete the plugin system integration to enable real hardware connection
    
    g_message("Real connection attempt: Plugin system integration not yet complete");
    g_message("This is where we would:");
    g_message("1. Initialize the plugin manager");
    g_message("2. Load the Speeduino plugin");
    g_message("3. Create a plugin context");
    g_message("4. Attempt auto-detection and connection");
    g_message("5. Get ECU signature and firmware version");
    
    if (status_label) {
        gtk_label_set_text(GTK_LABEL(status_label), "Plugin integration in progress, using simulation...");
    }
    
    // Fall back to simulation for now
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
    if (!speeduino_bridge_is_connected()) {
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
    
    // Get real runtime data from Speeduino
    const SpeeduinoOutputChannels *data = speeduino_bridge_get_runtime_data();
    if (data) {
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
    
    // Disconnect from real hardware if connected
    if (speeduino_bridge_is_connected()) {
        speeduino_bridge_disconnect();
        g_message("Disconnected from real hardware");
    }
    
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    if (builder) {
        GtkWidget *status_label = GTK_WIDGET(gtk_builder_get_object(builder, "connection_status_label"));
        GtkWidget *connect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "connect_button"));
        GtkWidget *disconnect_btn = GTK_WIDGET(gtk_builder_get_object(builder, "disconnect_button"));
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
    if (!speeduino_bridge_is_connected()) {
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


