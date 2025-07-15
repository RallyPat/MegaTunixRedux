/*
 * Copyright (C) 2002-2012 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 * Copyright (C) 2025 by Pat Burke <tetsuyablue at gmail dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file src/core_gui_modern.c
  \ingroup CoreMtx
  \brief Modern GTK4 implementation of the Core MegaTunix Redux GUI
  \author Pat Burke
  */

#include <args.h>
#include <builder_compat.h>
#include <core_gui.h>
#include <dashboard.h>
#include <debugging.h>
#include <gui_handlers.h>
#include <getfiles.h>
#include <logo.h>
#include <plugin.h>
#include <serialio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <widgetmgmt.h>

/* Modern GTK4 - no compatibility layer needed */
#include <gtk/gtk.h>

/* Forward declarations for signal handlers */
extern void on_connect_clicked(GtkButton *button, gpointer user_data);
extern void on_disconnect_clicked(GtkButton *button, gpointer user_data);
extern void on_settings_clicked(GtkButton *button, gpointer user_data);
extern void on_interrogate_clicked(GtkButton *button, gpointer user_data);

/* Default window size and MINIMUM size */
static gint def_width = 640;
static gint def_height = 400;
gint width = 0;
gint height = 0;
extern gconstpointer *global_data;

/* Modern GTK4 helper functions */
static GtkWidget *get_widget_from_builder(GtkBuilder *builder, const gchar *name) {
    GObject *object = gtk_builder_get_object(builder, name);
    if (!object) {
        g_warning("Widget '%s' not found in UI file", name);
        return NULL;
    }
    return GTK_WIDGET(object);
}

static void setup_css_styling(void) {
    GtkCssProvider *css_provider = gtk_css_provider_new();
    
    /* Modern CSS styling for automotive application */
    const gchar *css_data = 
        "window {"
        "  background-color: #2d2d2d;"
        "  color: #ffffff;"
        "}"
        "button {"
        "  background: linear-gradient(to bottom, #4a4a4a, #2d2d2d);"
        "  border: 1px solid #666666;"
        "  border-radius: 4px;"
        "  color: #ffffff;"
        "  padding: 8px 16px;"
        "  margin: 2px;"
        "}"
        "button:hover {"
        "  background: linear-gradient(to bottom, #5a5a5a, #3d3d3d);"
        "  border-color: #888888;"
        "}"
        "button:active {"
        "  background: linear-gradient(to bottom, #1a1a1a, #2d2d2d);"
        "}"
        "button.connect {"
        "  background: linear-gradient(to bottom, #4a8c4a, #2d5a2d);"
        "}"
        "button.disconnect {"
        "  background: linear-gradient(to bottom, #8c4a4a, #5a2d2d);"
        "}"
        "notebook {"
        "  background-color: #3d3d3d;"
        "}"
        "notebook tab {"
        "  background: linear-gradient(to bottom, #4a4a4a, #2d2d2d);"
        "  border: 1px solid #666666;"
        "  border-radius: 4px 4px 0 0;"
        "  color: #ffffff;"
        "  padding: 8px 16px;"
        "}"
        "notebook tab:checked {"
        "  background: linear-gradient(to bottom, #5a5a5a, #3d3d3d);"
        "}"
        "entry {"
        "  background-color: #1a1a1a;"
        "  color: #ffffff;"
        "  border: 1px solid #666666;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
        "label {"
        "  color: #ffffff;"
        "}"
        ".status-connected {"
        "  color: #4a8c4a;"
        "  font-weight: bold;"
        "}"
        ".status-disconnected {"
        "  color: #8c4a4a;"
        "  font-weight: bold;"
        "}"
        ".gauge {"
        "  background-color: #1a1a1a;"
        "  border: 2px solid #666666;"
        "  border-radius: 8px;"
        "}";
    
    gtk_css_provider_load_from_data(css_provider, css_data, -1);
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    
    g_object_unref(css_provider);
}

static void connect_signal_handlers(GtkBuilder *builder) {
    /* Modern GTK4 signal connections - pass builder as user_data */
    GtkWidget *connect_btn = get_widget_from_builder(builder, "connect_button");
    GtkWidget *disconnect_btn = get_widget_from_builder(builder, "disconnect_button");
    GtkWidget *settings_btn = get_widget_from_builder(builder, "settings_button");
    GtkWidget *interrogate_btn = get_widget_from_builder(builder, "interrogate_button");
    
    if (connect_btn) {
        g_signal_connect(connect_btn, "clicked", G_CALLBACK(on_connect_clicked), builder);
        gtk_widget_add_css_class(connect_btn, "connect");
    }
    if (disconnect_btn) {
        g_signal_connect(disconnect_btn, "clicked", G_CALLBACK(on_disconnect_clicked), builder);
        gtk_widget_add_css_class(disconnect_btn, "disconnect");
    }
    if (settings_btn) {
        g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), builder);
    }
    if (interrogate_btn) {
        g_signal_connect(interrogate_btn, "clicked", G_CALLBACK(on_interrogate_clicked), builder);
    }
}

static void setup_window_properties(GtkWidget *window) {
    /* Modern GTK4 window setup */
    gtk_window_set_title(GTK_WINDOW(window), "MegaTunix Redux " GIT_HASH);
    gtk_window_set_default_size(GTK_WINDOW(window), def_width, def_height);
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    
    /* Set window icon if available */
    gtk_window_set_icon_name(GTK_WINDOW(window), "megatunix-redux");
    
    /* Modern window hints */
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);
    gtk_window_set_deletable(GTK_WINDOW(window), TRUE);
    
    /* Get saved window position and size */
    gint x = (GINT)DATA_GET(global_data, "main_x_origin");
    gint y = (GINT)DATA_GET(global_data, "main_y_origin");
    gint w = (GINT)DATA_GET(global_data, "main_width");
    gint h = (GINT)DATA_GET(global_data, "main_height");
    
    if (w > 0 && h > 0) {
        gtk_window_set_default_size(GTK_WINDOW(window), w, h);
    }
}

static void setup_notebook_tabs(GtkBuilder *builder) {
    GtkWidget *notebook = get_widget_from_builder(builder, "toplevel_notebook");
    if (!notebook) return;
    
    /* Modern notebook setup */
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    
    /* Handle tab visibility */
    gint tabcount = gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook));
    gboolean *hidden_list = (gboolean *)DATA_GET(global_data, "hidden_list");
    
    for (gint i = 0; i < tabcount; i++) {
        GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), i);
        if (child && hidden_list && hidden_list[i]) {
            gtk_widget_set_visible(child, FALSE);
        }
    }
}

/**
 * finalize_core_gui - Initialize GUI components that can't be set statically
 * @builder: GTK Builder object
 */
void finalize_core_gui(GtkBuilder * builder)
{
    /* Initializes base gui and installs things like the logo and
     * other dynamic bits that can't be set via glade statically 
     */
    GtkTextBuffer * textbuffer = NULL;
    GtkTextTag *tag = NULL;
    GtkWidget *button = NULL;
    GtkWidget *close_button = NULL;
    GtkWidget *ebox = NULL;
    GtkWidget *label = NULL;
    GtkWidget *frame = NULL;
    GtkWidget *widget = NULL;
    GtkWidget *image = NULL;
    GdkPixbuf *pixbuf = NULL;
    gchar * tmpbuf = NULL;
    gint mtx_temp_units;
    gint mtx_color_scale;
    Serial_Params *serial_params = NULL;

    ENTER();

    serial_params = (Serial_Params *)DATA_GET(global_data,"serial_params");
    mtx_temp_units = (GINT)DATA_GET(global_data,"mtx_temp_units");
    mtx_color_scale = (GINT)DATA_GET(global_data,"mtx_color_scale");

    // Initialize connection status icons
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "connected_icon"));
    if (widget)
        DATA_SET(global_data, "connected_icon", widget);
    
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "disconnected_icon"));
    if (widget)
        DATA_SET(global_data, "disconnected_icon", widget);

    // Set up status bar text
    widget = GTK_WIDGET(gtk_builder_get_object(builder, "status_textview"));
    if (widget) {
        textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
        if (textbuffer) {
            gtk_text_buffer_create_tag(textbuffer, "info", 
                                     "foreground", "#0000FF", NULL);
            gtk_text_buffer_create_tag(textbuffer, "warning", 
                                     "foreground", "#FF8000", NULL);
            gtk_text_buffer_create_tag(textbuffer, "error", 
                                     "foreground", "#FF0000", NULL);
        }
    }

    // Initialize temperature units
    if (mtx_temp_units == FAHRENHEIT) {
        widget = GTK_WIDGET(gtk_builder_get_object(builder, "fahrenheit_radiobutton"));
        if (widget)
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), TRUE);
    } else {
        widget = GTK_WIDGET(gtk_builder_get_object(builder, "celsius_radiobutton"));
        if (widget)
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), TRUE);
    }

    EXIT();
}

/**
 * set_connected_icons_state - Update connection status icons
 * @state: TRUE for connected, FALSE for disconnected
 */
void set_connected_icons_state(gboolean state)
{
    static GtkWidget * conn = NULL;
    static GtkWidget * disconn = NULL;

    ENTER();
    
    if (!conn)
        conn = (GtkWidget *)DATA_GET(global_data,"connected_icon");
    if (!disconn)
        disconn = (GtkWidget *)DATA_GET(global_data,"disconnected_icon");

    g_return_if_fail(conn);
    g_return_if_fail(disconn);

    if (state)
    {
        gtk_widget_set_sensitive(conn, TRUE);
        gtk_widget_set_sensitive(disconn, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive(conn, FALSE);
        gtk_widget_set_sensitive(disconn, TRUE);
    }
    EXIT();
}

/*!
  \brief setup_gui_modern() creates the main window using modern GTK4 APIs
  This replaces the compatibility layer approach with proper GTK4 implementation
  */
G_MODULE_EXPORT gboolean setup_gui_modern(GtkApplication *app) {
    gchar *fname = NULL;
    gchar *filename = NULL;
    GtkWidget *window = NULL;
    GtkBuilder *builder = NULL;
    GError *error = NULL;
    CmdLineArgs *args = NULL;

    ENTER();

    args = (CmdLineArgs *)DATA_GET(global_data, "args");
    g_return_val_if_fail(args, FALSE);

    /* Setup modern CSS styling first */
    setup_css_styling();

    /* Load UI file */
    fname = g_build_filename(GUI_DATA_DIR, "main.ui", NULL);
    
    /* First try to find the file in current directory (for development) */
    if (g_file_test(fname, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
        filename = g_strdup(fname);
    } else {
        /* Fall back to get_file for installed locations */
        filename = get_file((const gchar *)DATA_GET(global_data, "project_name"), fname, NULL);
    }
    
    if (!filename) {
        g_critical("Could NOT locate file %s. Did you forget to run \"sudo make install\"?", fname);
        g_free(fname);
        
        /* Modern GTK4 error dialog */
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "MegaTunix Redux doesn't appear to be installed correctly!\n\n"
            "Did you forget to run \"sudo make install\"??"
        );
        
        g_signal_connect(dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
        gtk_widget_show(dialog);
        
        if (global_data) {
            g_dataset_destroy(global_data);
            g_free(global_data);
        }
        exit(-1);
    }

    /* Load UI with modern GTK4 builder */
    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, filename, &error)) {
        g_critical("Could not load UI file %s: %s", filename, error ? error->message : "Unknown error");
        if (error) {
            g_error_free(error);
        }
        g_free(fname);
        g_free(filename);
        g_object_unref(builder);
        exit(-1);
    }
    
    g_free(fname);
    g_free(filename);

    /* Get main window */
    window = get_widget_from_builder(builder, "mtx_main_window");
    if (!window) {
        g_critical("Could not find main window 'mtx_main_window' in UI file");
        g_object_unref(builder);
        exit(-1);
    }

    /* Associate window with application */
    if (app) {
        gtk_application_add_window(app, GTK_WINDOW(window));
    }

    /* Register window for global access */
    register_widget("main_window", window);

    /* Setup window properties */
    setup_window_properties(window);

    /* Connect modern signal handlers */
    g_signal_connect(window, "close-request", G_CALLBACK(leave), NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(leave), NULL);

    /* Connect UI signal handlers */
    connect_signal_handlers(builder);

    /* Setup notebook tabs */
    setup_notebook_tabs(builder);

    /* Store builder for later use */
    g_message("Storing builder in global_data: %p", builder);
    DATA_SET_FULL(global_data, "main_builder", builder, g_object_unref);
    set_global_builder(builder);
    g_message("Builder stored successfully in global_data");

    /* Setup tooltips (modern GTK4 way) */
    gboolean tips_in_use = (GBOOLEAN)DATA_GET(global_data, "tips_in_use");
    if (tips_in_use) {
        /* GTK4 handles tooltips automatically when gtk_widget_set_tooltip_text is used */
        g_info("Tooltips enabled");
    }

    /* Finalize core GUI setup */
    finalize_core_gui(builder);

    /* Show window if not hidden */
    if (!args->hide_maingui) {
        g_print("Showing main window...\n");
        gtk_widget_set_visible(window, TRUE);
        gtk_window_present(GTK_WINDOW(window));
        gtk_window_present(GTK_WINDOW(window));
    }

    g_print("Modern GTK4 GUI setup complete\n");
    
    EXIT();
    return TRUE;
}

/*!
  \brief Update the existing setup_gui to use modern implementation
  This allows gradual migration while maintaining compatibility
  */
G_MODULE_EXPORT gboolean setup_gui(GtkApplication *app) {
    /* Use modern implementation */
    return setup_gui_modern(app);
}
