/*
 * Modern widget system for MegaTunix Redux
 * GTK4 compatibility layer and widget management
 */

#include "gtk_compat.h"
#include <gtk/gtk.h>
#include <glib.h>

/* Widget registry for GTK4 compatibility */
// NOTE: Widget management functions are now in widgetmgmt.c
// These functions are commented out to avoid conflicts

// static GHashTable *widget_registry = NULL;

/* Initialize widget registry - moved to widgetmgmt.c */
/*
void init_widget_registry(void)
{
    if (!widget_registry) {
        widget_registry = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    }
}
*/

/* Register a widget by name - moved to widgetmgmt.c */
/*
void register_widget(const gchar *name, GtkWidget *widget)
{
    if (!widget_registry) {
        init_widget_registry();
    }
    
    g_hash_table_insert(widget_registry, g_strdup(name), widget);
}
*/

/* Lookup a widget by name - moved to widgetmgmt.c */
/*
GtkWidget *lookup_widget(const gchar *name)
{
    if (!widget_registry) {
        return NULL;
    }
    
    return GTK_WIDGET(g_hash_table_lookup(widget_registry, name));
}
*/

/* Deregister a widget - moved to widgetmgmt.c */
/*
gboolean deregister_widget(const gchar *name)
{
    if (!widget_registry) {
        return FALSE;
    }
    
    return g_hash_table_remove(widget_registry, name);
}
*/

/* Cleanup widget registry - moved to widgetmgmt.c */
/*
void cleanup_widget_registry(void)
{
    if (widget_registry) {
        g_hash_table_destroy(widget_registry);
        widget_registry = NULL;
    }
}
*/

/* GTK4 compatibility helpers */

/* Replace deprecated gtk_widget_modify_text */
void mtx_widget_set_text_color(GtkWidget *widget, const GdkRGBA *color)
{
    GtkCssProvider *provider;
    gchar *css_string;
    
    provider = gtk_css_provider_new();
    css_string = g_strdup_printf("* { color: rgba(%d, %d, %d, %f); }", 
                                (int)(color->red * 255),
                                (int)(color->green * 255),
                                (int)(color->blue * 255),
                                color->alpha);
    
    gtk_css_provider_load_from_data(provider, css_string, -1);
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                 GTK_STYLE_PROVIDER(provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_free(css_string);
    g_object_unref(provider);
}

/* Replace deprecated gtk_widget_modify_bg */
void mtx_widget_set_background_color(GtkWidget *widget, const GdkRGBA *color)
{
    GtkCssProvider *provider;
    gchar *css_string;
    
    provider = gtk_css_provider_new();
    css_string = g_strdup_printf("* { background-color: rgba(%d, %d, %d, %f); }", 
                                (int)(color->red * 255),
                                (int)(color->green * 255),
                                (int)(color->blue * 255),
                                color->alpha);
    
    gtk_css_provider_load_from_data(provider, css_string, -1);
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                 GTK_STYLE_PROVIDER(provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_free(css_string);
    g_object_unref(provider);
}

/* Color conversion helpers are provided by gtk_compat.h */

/* Modern OpenGL context creation for GTK4 */
GtkGLArea *create_gl_area(void)
{
    GtkGLArea *gl_area = GTK_GL_AREA(gtk_gl_area_new());
    
    /* Set required OpenGL version */
    gtk_gl_area_set_required_version(gl_area, 3, 3);
    
    /* Enable depth buffer */
    gtk_gl_area_set_has_depth_buffer(gl_area, TRUE);
    
    return gl_area;
}

/* Application-wide CSS setup */
void setup_application_css(void)
{
    GtkCssProvider *provider;
    const gchar *css_data = 
        "/* MegaTunix Redux Modern Theme */\n"
        ".mtx-gauge {\n"
        "    border-radius: 8px;\n"
        "    box-shadow: 0 2px 4px rgba(0,0,0,0.2);\n"
        "}\n"
        "\n"
        ".mtx-dashboard {\n"
        "    background: linear-gradient(45deg, #1e1e1e, #2d2d2d);\n"
        "}\n"
        "\n"
        ".mtx-warning {\n"
        "    color: #ff6b35;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        ".mtx-critical {\n"
        "    color: #ff0000;\n"
        "    font-weight: bold;\n"
        "    animation: blink 1s infinite;\n"
        "}\n"
        "\n"
        "@keyframes blink {\n"
        "    0% { opacity: 1; }\n"
        "    50% { opacity: 0.5; }\n"
        "    100% { opacity: 1; }\n"
        "}\n";
    
    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css_data, -1);
    
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    g_object_unref(provider);
}
