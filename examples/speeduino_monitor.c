/*
 * Speeduino Plugin Usage Example for MegaTunix Redux
 * 
 * This example demonstrates how to use the Speeduino plugin to connect
 * to and communicate with a Speeduino ECU.
 */

#include <gtk/gtk.h>
#include "plugin_system.h"
#include "speeduino_plugin.h"

/* Application data structure */
typedef struct {
    GtkApplication *app;
    GtkWidget *main_window;
    GtkWidget *connect_button;
    GtkWidget *disconnect_button;
    GtkWidget *status_label;
    GtkWidget *rpm_label;
    GtkWidget *map_label;
    GtkWidget *coolant_label;
    GtkWidget *tps_label;
    
    MtxPluginManager *plugin_manager;
    MtxPluginContext *speeduino_context;
    guint update_timer_id;
} SpeeduinoApp;

/* Update display with current ECU data */
static gboolean update_display(gpointer user_data)
{
    SpeeduinoApp *app = (SpeeduinoApp *)user_data;
    
    if (!speeduino_is_connected(app->speeduino_context)) {
        return G_SOURCE_CONTINUE;
    }
    
    /* Get runtime data from Speeduino */
    const SpeeduinoOutputChannels *data = speeduino_get_runtime_data(app->speeduino_context);
    if (!data) {
        return G_SOURCE_CONTINUE;
    }
    
    /* Update labels with current values */
    gchar *rpm_text = g_strdup_printf("RPM: %d", data->rpm);
    gtk_label_set_text(GTK_LABEL(app->rpm_label), rpm_text);
    g_free(rpm_text);
    
    gchar *map_text = g_strdup_printf("MAP: %d kPa", data->map);
    gtk_label_set_text(GTK_LABEL(app->map_label), map_text);
    g_free(map_text);
    
    gchar *coolant_text = g_strdup_printf("Coolant: %d°C", data->coolant);
    gtk_label_set_text(GTK_LABEL(app->coolant_label), coolant_text);
    g_free(coolant_text);
    
    gchar *tps_text = g_strdup_printf("TPS: %d%%", data->tps);
    gtk_label_set_text(GTK_LABEL(app->tps_label), tps_text);
    g_free(tps_text);
    
    /* Check for warnings */
    if (data->coolant > 100) {
        gtk_label_set_text(GTK_LABEL(app->status_label), 
                          "⚠️ High coolant temperature!");
    } else if (data->rpm > 6000) {
        gtk_label_set_text(GTK_LABEL(app->status_label), 
                          "⚠️ High RPM!");
    } else {
        const gchar *signature = speeduino_get_ecu_signature(app->speeduino_context);
        gchar *status_text = g_strdup_printf("Connected to: %s", 
                                           signature ? signature : "Speeduino");
        gtk_label_set_text(GTK_LABEL(app->status_label), status_text);
        g_free(status_text);
    }
    
    return G_SOURCE_CONTINUE;
}

/* Connect button clicked */
static void on_connect_clicked(GtkButton *button, gpointer user_data)
{
    SpeeduinoApp *app = (SpeeduinoApp *)user_data;
    
    gtk_label_set_text(GTK_LABEL(app->status_label), "Connecting...");
    
    /* Try auto-detection first */
    GError *error = NULL;
    gboolean connected = speeduino_auto_detect(app->speeduino_context, &error);
    
    if (!connected) {
        /* If auto-detection fails, try common devices */
        const gchar *devices[] = {
            "/dev/ttyUSB0", "/dev/ttyUSB1", 
            "/dev/ttyACM0", "/dev/ttyACM1",
            NULL
        };
        
        for (gint i = 0; devices[i] && !connected; i++) {
            g_clear_error(&error);
            connected = speeduino_connect_device(app->speeduino_context, 
                                                devices[i], 115200, &error);
        }
    }
    
    if (connected) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Connected!");
        gtk_widget_set_sensitive(app->connect_button, FALSE);
        gtk_widget_set_sensitive(app->disconnect_button, TRUE);
        
        /* Start update timer */
        app->update_timer_id = g_timeout_add(100, update_display, app);
        
        /* Get ECU information */
        const gchar *signature = speeduino_get_ecu_signature(app->speeduino_context);
        const gchar *version = speeduino_get_firmware_version(app->speeduino_context);
        
        g_print("Connected to Speeduino ECU\n");
        g_print("Signature: %s\n", signature ? signature : "Unknown");
        g_print("Version: %s\n", version ? version : "Unknown");
    } else {
        gchar *error_text = g_strdup_printf("Connection failed: %s", 
                                          error ? error->message : "Unknown error");
        gtk_label_set_text(GTK_LABEL(app->status_label), error_text);
        g_free(error_text);
        g_clear_error(&error);
    }
}

/* Disconnect button clicked */
static void on_disconnect_clicked(GtkButton *button, gpointer user_data)
{
    SpeeduinoApp *app = (SpeeduinoApp *)user_data;
    
    /* Stop update timer */
    if (app->update_timer_id > 0) {
        g_source_remove(app->update_timer_id);
        app->update_timer_id = 0;
    }
    
    /* Disconnect from ECU */
    speeduino_disconnect_device(app->speeduino_context);
    
    /* Update UI */
    gtk_label_set_text(GTK_LABEL(app->status_label), "Disconnected");
    gtk_label_set_text(GTK_LABEL(app->rpm_label), "RPM: --");
    gtk_label_set_text(GTK_LABEL(app->map_label), "MAP: --");
    gtk_label_set_text(GTK_LABEL(app->coolant_label), "Coolant: --");
    gtk_label_set_text(GTK_LABEL(app->tps_label), "TPS: --");
    
    gtk_widget_set_sensitive(app->connect_button, TRUE);
    gtk_widget_set_sensitive(app->disconnect_button, FALSE);
    
    g_print("Disconnected from Speeduino ECU\n");
}

/* Create the main application window */
static void create_main_window(SpeeduinoApp *app)
{
    /* Create main window */
    app->main_window = gtk_application_window_new(app->app);
    gtk_window_set_title(GTK_WINDOW(app->main_window), "Speeduino Monitor - MegaTunix Redux");
    gtk_window_set_default_size(GTK_WINDOW(app->main_window), 400, 300);
    
    /* Create main container */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    gtk_window_set_child(GTK_WINDOW(app->main_window), vbox);
    
    /* Status label */
    app->status_label = gtk_label_new("Not connected");
    gtk_widget_add_css_class(app->status_label, "title-4");
    gtk_box_append(GTK_BOX(vbox), app->status_label);
    
    /* Connection buttons */
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), button_box);
    
    app->connect_button = gtk_button_new_with_label("Connect");
    gtk_widget_add_css_class(app->connect_button, "suggested-action");
    g_signal_connect(app->connect_button, "clicked", 
                     G_CALLBACK(on_connect_clicked), app);
    gtk_box_append(GTK_BOX(button_box), app->connect_button);
    
    app->disconnect_button = gtk_button_new_with_label("Disconnect");
    gtk_widget_add_css_class(app->disconnect_button, "destructive-action");
    gtk_widget_set_sensitive(app->disconnect_button, FALSE);
    g_signal_connect(app->disconnect_button, "clicked", 
                     G_CALLBACK(on_disconnect_clicked), app);
    gtk_box_append(GTK_BOX(button_box), app->disconnect_button);
    
    /* Separator */
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(vbox), separator);
    
    /* Data display grid */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), grid);
    
    /* Engine data labels */
    app->rpm_label = gtk_label_new("RPM: --");
    gtk_widget_add_css_class(app->rpm_label, "title-3");
    gtk_grid_attach(GTK_GRID(grid), app->rpm_label, 0, 0, 1, 1);
    
    app->map_label = gtk_label_new("MAP: --");
    gtk_widget_add_css_class(app->map_label, "title-3");
    gtk_grid_attach(GTK_GRID(grid), app->map_label, 1, 0, 1, 1);
    
    app->coolant_label = gtk_label_new("Coolant: --");
    gtk_widget_add_css_class(app->coolant_label, "title-3");
    gtk_grid_attach(GTK_GRID(grid), app->coolant_label, 0, 1, 1, 1);
    
    app->tps_label = gtk_label_new("TPS: --");
    gtk_widget_add_css_class(app->tps_label, "title-3");
    gtk_grid_attach(GTK_GRID(grid), app->tps_label, 1, 1, 1, 1);
}

/* Application startup */
static void on_app_startup(GApplication *application, gpointer user_data)
{
    SpeeduinoApp *app = (SpeeduinoApp *)user_data;
    
    /* Initialize plugin system */
    app->plugin_manager = mtx_plugin_manager_new();
    
    GError *error = NULL;
    if (!mtx_plugin_manager_initialize(app->plugin_manager, 
                                      "./plugins", &error)) {
        g_warning("Failed to initialize plugin manager: %s", 
                 error ? error->message : "Unknown error");
        g_clear_error(&error);
        return;
    }
    
    /* Load Speeduino plugin */
    if (!mtx_plugin_manager_load_plugin(app->plugin_manager, 
                                       "./plugins/speeduino_plugin.so", &error)) {
        g_warning("Failed to load Speeduino plugin: %s", 
                 error ? error->message : "Unknown error");
        g_clear_error(&error);
        return;
    }
    
    /* Create plugin context for Speeduino */
    app->speeduino_context = mtx_plugin_context_new("speeduino");
    
    g_print("Speeduino plugin loaded successfully\n");
}

/* Application activate (window creation) */
static void on_app_activate(GApplication *application, gpointer user_data)
{
    SpeeduinoApp *app = (SpeeduinoApp *)user_data;
    
    create_main_window(app);
    gtk_window_present(GTK_WINDOW(app->main_window));
}

/* Application shutdown */
static void on_app_shutdown(GApplication *application, gpointer user_data)
{
    SpeeduinoApp *app = (SpeeduinoApp *)user_data;
    
    /* Stop update timer */
    if (app->update_timer_id > 0) {
        g_source_remove(app->update_timer_id);
    }
    
    /* Disconnect if connected */
    if (speeduino_is_connected(app->speeduino_context)) {
        speeduino_disconnect_device(app->speeduino_context);
    }
    
    /* Cleanup plugin system */
    if (app->speeduino_context) {
        mtx_plugin_context_free(app->speeduino_context);
    }
    
    if (app->plugin_manager) {
        mtx_plugin_manager_cleanup(app->plugin_manager);
        mtx_plugin_manager_free(app->plugin_manager);
    }
    
    g_print("Application shutdown complete\n");
}

/* Main function */
int main(int argc, char *argv[])
{
    SpeeduinoApp app = {0};
    
    /* Create GTK application */
    app.app = gtk_application_new("org.megatunix-redux.speeduino-monitor", 
                                  G_APPLICATION_DEFAULT_FLAGS);
    
    /* Connect signals */
    g_signal_connect(app.app, "startup", G_CALLBACK(on_app_startup), &app);
    g_signal_connect(app.app, "activate", G_CALLBACK(on_app_activate), &app);
    g_signal_connect(app.app, "shutdown", G_CALLBACK(on_app_shutdown), &app);
    
    /* Run application */
    int status = g_application_run(G_APPLICATION(app.app), argc, argv);
    
    /* Cleanup */
    g_object_unref(app.app);
    
    return status;
}
