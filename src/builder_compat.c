/*
 * Builder Compatibility Helper for MegaTunix Redux
 * 
 * This file provides compatibility functions to help migrate from GladeXML to GtkBuilder
 */

#include "builder_compat.h"
#include "defines.h"
#include "globals.h"

static GtkBuilder *global_builder = NULL;

void set_global_builder(GtkBuilder *builder)
{
    if (global_builder)
        g_object_unref(global_builder);
    global_builder = g_object_ref(builder);
}

GtkBuilder *get_global_builder(void)
{
    return global_builder;
}

GtkWidget *lookup_widget_by_name(GtkWidget *widget, const gchar *name)
{
    GtkBuilder *builder = (GtkBuilder *)DATA_GET(global_data, "main_builder");
    if (!builder) {
        builder = global_builder;
    }
    
    if (!builder) {
        g_warning("No GtkBuilder available for widget lookup: %s", name);
        return NULL;
    }
    
    GObject *object = gtk_builder_get_object(builder, name);
    if (object && GTK_IS_WIDGET(object)) {
        return GTK_WIDGET(object);
    }
    
    g_warning("Widget not found or not a widget: %s", name);
    return NULL;
}

/* Compatibility wrapper for glade_xml_get_widget */
GtkWidget *glade_xml_get_widget_compat(gpointer unused, const gchar *name)
{
    return lookup_widget_by_name(NULL, name);
}

// GTK4 file chooser compatibility implementation
GFile* g_file_new_for_path_compat(const char* path) {
    if (path && *path) {
        return g_file_new_for_path(path);
    }
    return NULL;
}
