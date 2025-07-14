/*
 * MegaTunix Redux - Modern GTK4 Application
 * Main application class and window management
 */

#include <gtk/gtk.h>
#include <glib.h>
#include "modern_widgets.h"
#include "security_utils.h"

#define MTX_TYPE_APPLICATION (mtx_application_get_type())
G_DECLARE_FINAL_TYPE(MtxApplication, mtx_application, MTX, APPLICATION, GtkApplication)

struct _MtxApplication {
    GtkApplication parent_instance;
    
    /* Application state */
    GtkWindow *main_window;
    GtkBuilder *builder;
    gboolean connected_to_ecu;
    gchar *current_project_file;
    
    /* Security */
    MtxRateLimiter *ui_rate_limiter;
};

G_DEFINE_TYPE(MtxApplication, mtx_application, GTK_TYPE_APPLICATION)

/* Forward declarations */
static void mtx_application_activate(GApplication *app);
static void mtx_application_open(GApplication *app, GFile **files, gint n_files, const gchar *hint);
static void mtx_application_shutdown(GApplication *app);
static void setup_actions(MtxApplication *app);
static void load_main_interface(MtxApplication *app);

/* Action handlers */
static void on_action_quit(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void on_action_about(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void on_action_preferences(GSimpleAction *action, GVariant *parameter, gpointer user_data);
static void on_action_interrogate(GSimpleAction *action, GVariant *parameter, gpointer user_data);

/* Signal handlers */
static gboolean on_main_window_close(GtkWindow *window, gpointer user_data);
static void on_connect_clicked(GtkButton *button, gpointer user_data);
static void on_settings_clicked(GtkButton *button, gpointer user_data);

/* Action entries for GtkApplication */
static const GActionEntry app_actions[] = {
    { "quit", on_action_quit, NULL, NULL, NULL },
    { "about", on_action_about, NULL, NULL, NULL },
    { "preferences", on_action_preferences, NULL, NULL, NULL },
    { "open", NULL, NULL, NULL, NULL },  /* TODO: Implement */
    { "save", NULL, NULL, NULL, NULL },  /* TODO: Implement */
    { "recent", NULL, NULL, NULL, NULL }, /* TODO: Implement */
    { "interrogate", on_action_interrogate, NULL, NULL, NULL },
    { "read_tables", NULL, NULL, NULL, NULL }, /* TODO: Implement */
    { "burn", NULL, NULL, NULL, NULL }, /* TODO: Implement */
    { "dashboard_designer", NULL, NULL, NULL, NULL }, /* TODO: Implement */
    { "gauge_designer", NULL, NULL, NULL, NULL }, /* TODO: Implement */
};

static void mtx_application_class_init(MtxApplicationClass *klass)
{
    GApplicationClass *app_class = G_APPLICATION_CLASS(klass);
    
    app_class->activate = mtx_application_activate;
    app_class->open = mtx_application_open;
    app_class->shutdown = mtx_application_shutdown;
}

static void mtx_application_init(MtxApplication *app)
{
    app->main_window = NULL;
    app->builder = NULL;
    app->connected_to_ecu = FALSE;
    app->current_project_file = NULL;
    app->ui_rate_limiter = mtx_rate_limiter_new(10); /* 10 operations per second max */
}

static void mtx_application_activate(GApplication *app)
{
    MtxApplication *mtx_app = MTX_APPLICATION(app);
    
    /* Initialize widget registry */
    init_widget_registry();
    
    /* Setup application CSS */
    setup_application_css();
    
    /* Setup actions */
    setup_actions(mtx_app);
    
    /* Load and show main interface */
    load_main_interface(mtx_app);
    
    /* Present the main window */
    if (mtx_app->main_window) {
        gtk_window_present(mtx_app->main_window);
    }
}

static void mtx_application_open(GApplication *app, GFile **files, gint n_files, const gchar *hint)
{
    /* Activate first, then handle file opening */
    mtx_application_activate(app);
    
    /* TODO: Handle opening project files */
    for (gint i = 0; i < n_files; i++) {
        gchar *path = g_file_get_path(files[i]);
        g_print("Opening file: %s\n", path);
        
        /* Validate filename for security */
        gchar *basename = g_path_get_basename(path);
        if (mtx_validate_filename(basename)) {
            /* TODO: Load project file */
        } else {
            g_warning("Invalid filename rejected: %s", basename);
        }
        
        g_free(basename);
        g_free(path);
    }
}

static void mtx_application_shutdown(GApplication *app)
{
    MtxApplication *mtx_app = MTX_APPLICATION(app);
    
    /* Cleanup */
    if (mtx_app->builder) {
        g_object_unref(mtx_app->builder);
    }
    
    if (mtx_app->current_project_file) {
        g_free(mtx_app->current_project_file);
    }
    
    if (mtx_app->ui_rate_limiter) {
        mtx_rate_limiter_free(mtx_app->ui_rate_limiter);
    }
    
    cleanup_widget_registry();
    
    G_APPLICATION_CLASS(mtx_application_parent_class)->shutdown(app);
}

static void setup_actions(MtxApplication *app)
{
    g_action_map_add_action_entries(G_ACTION_MAP(app),
                                   app_actions,
                                   G_N_ELEMENTS(app_actions),
                                   app);
    
    /* Set up keyboard shortcuts */
    const char *quit_accels[] = { "<Ctrl>Q", NULL };
    const char *preferences_accels[] = { "<Ctrl>comma", NULL };
    const char *open_accels[] = { "<Ctrl>O", NULL };
    const char *save_accels[] = { "<Ctrl>S", NULL };
    
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.quit", quit_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.preferences", preferences_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.open", open_accels);
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), "app.save", save_accels);
}

static void load_main_interface(MtxApplication *app)
{
    GError *error = NULL;
    gchar *ui_file_path;
    
    /* Build path to UI file */
    ui_file_path = g_build_filename(GUI_DATA_DIR, "main.ui", NULL);
    
    /* Create builder and load UI */
    app->builder = gtk_builder_new();
    
    if (!gtk_builder_add_from_file(app->builder, ui_file_path, &error)) {
        g_critical("Error loading UI file %s: %s", ui_file_path, error->message);
        g_error_free(error);
        g_free(ui_file_path);
        return;
    }
    
    g_free(ui_file_path);
    
    /* Get main window */
    app->main_window = GTK_WINDOW(gtk_builder_get_object(app->builder, "mtx_main_window"));
    if (!app->main_window) {
        g_critical("Could not find main window in UI file");
        return;
    }
    
    /* Set application for window */
    gtk_window_set_application(app->main_window, GTK_APPLICATION(app));
    
    /* Connect signals */
    g_signal_connect(app->main_window, "close-request", G_CALLBACK(on_main_window_close), app);
    
    /* Connect button signals */
    GtkWidget *connect_button = GTK_WIDGET(gtk_builder_get_object(app->builder, "connect_button"));
    if (connect_button) {
        g_signal_connect(connect_button, "clicked", G_CALLBACK(on_connect_clicked), app);
    }
    
    GtkWidget *settings_button = GTK_WIDGET(gtk_builder_get_object(app->builder, "settings_button"));
    if (settings_button) {
        g_signal_connect(settings_button, "clicked", G_CALLBACK(on_settings_clicked), app);
    }
    
    /* Register important widgets */
    register_widget("main_window", GTK_WIDGET(app->main_window));
    register_widget("toplevel_notebook", GTK_WIDGET(gtk_builder_get_object(app->builder, "toplevel_notebook")));
    register_widget("dashboard_container", GTK_WIDGET(gtk_builder_get_object(app->builder, "dashboard_container")));
    register_widget("connection_status_label", GTK_WIDGET(gtk_builder_get_object(app->builder, "connection_status_label")));
}

/* Action handlers */
static void on_action_quit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    MtxApplication *app = MTX_APPLICATION(user_data);
    
    /* TODO: Check for unsaved changes */
    
    g_application_quit(G_APPLICATION(app));
}

static void on_action_about(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    MtxApplication *app = MTX_APPLICATION(user_data);
    
    GtkWidget *about_dialog = gtk_about_dialog_new();
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "MegaTunix Redux");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), "1.0.0");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), 
                                  "Copyright © 2002-2025 David J. Andruczyk");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),
                                 "Modern automotive ECU tuning software");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), 
                                "https://github.com/RallyPat/MegaTunixRedux");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(about_dialog), GTK_LICENSE_GPL_2_0);
    
    const char *authors[] = {
        "David J. Andruczyk <djandruczyk@gmail.com>",
        "Contributors from the MegaSquirt community",
        NULL
    };
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
    
    gtk_window_set_transient_for(GTK_WINDOW(about_dialog), app->main_window);
    gtk_window_set_modal(GTK_WINDOW(about_dialog), TRUE);
    gtk_widget_show(about_dialog);
}

static void on_action_preferences(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    /* TODO: Show preferences dialog */
    g_print("Preferences dialog not yet implemented\n");
}

static void on_action_interrogate(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
    MtxApplication *app = MTX_APPLICATION(user_data);
    
    /* Rate limiting check */
    if (!mtx_rate_limiter_check(app->ui_rate_limiter, "interrogate")) {
        g_warning("Interrogate action rate limited");
        return;
    }
    
    /* TODO: Implement ECU interrogation */
    g_print("ECU interrogation not yet implemented\n");
}

/* Signal handlers */
static gboolean on_main_window_close(GtkWindow *window, gpointer user_data)
{
    MtxApplication *app = MTX_APPLICATION(user_data);
    
    /* TODO: Check for unsaved changes and prompt user */
    
    g_application_quit(G_APPLICATION(app));
    return FALSE; /* Allow the window to close */
}

static void on_connect_clicked(GtkButton *button, gpointer user_data)
{
    MtxApplication *app = MTX_APPLICATION(user_data);
    GtkWidget *status_label;
    
    /* Rate limiting check */
    if (!mtx_rate_limiter_check(app->ui_rate_limiter, "connect")) {
        g_warning("Connect action rate limited");
        return;
    }
    
    status_label = GTK_WIDGET(gtk_builder_get_object(app->builder, "connection_status_label"));
    
    if (app->connected_to_ecu) {
        /* Disconnect */
        app->connected_to_ecu = FALSE;
        gtk_button_set_icon_name(button, "network-wired-symbolic");
        gtk_widget_set_tooltip_text(GTK_WIDGET(button), "Connect to ECU");
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Disconnected");
        }
        /* TODO: Actual disconnect logic */
    } else {
        /* Connect */
        app->connected_to_ecu = TRUE;
        gtk_button_set_icon_name(button, "network-wired-disconnected-symbolic");
        gtk_widget_set_tooltip_text(GTK_WIDGET(button), "Disconnect from ECU");
        if (status_label) {
            gtk_label_set_text(GTK_LABEL(status_label), "Connected");
        }
        /* TODO: Actual connect logic */
    }
}

static void on_settings_clicked(GtkButton *button, gpointer user_data)
{
    /* Trigger preferences action */
    g_action_activate(g_action_map_lookup_action(G_ACTION_MAP(user_data), "preferences"), NULL);
}

/* Public API */
MtxApplication *mtx_application_new(void)
{
    return g_object_new(MTX_TYPE_APPLICATION,
                       "application-id", "org.megatunix-redux.MegaTunixRedux",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

int main(int argc, char *argv[])
{
    MtxApplication *app;
    int status;
    
    /* Create application */
    app = mtx_application_new();
    
    /* Run application */
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
    /* Cleanup */
    g_object_unref(app);
    
    return status;
}
