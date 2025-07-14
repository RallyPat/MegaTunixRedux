/*
 * Modern widget system header for MegaTunix Redux
 */

#ifndef __MODERN_WIDGETS_H__
#define __MODERN_WIDGETS_H__

#include "gtk_compat.h"
#include <gtk/gtk.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Widget registry functions */
void init_widget_registry(void);
void register_widget(const gchar *name, GtkWidget *widget);
GtkWidget *lookup_widget(const gchar *name);
gboolean deregister_widget(const gchar *name);
void cleanup_widget_registry(void);

/* GTK4 compatibility helpers */
void mtx_widget_set_text_color(GtkWidget *widget, const GdkRGBA *color);
void mtx_widget_set_background_color(GtkWidget *widget, const GdkRGBA *color);

/* Modern OpenGL support */
GtkGLArea *create_gl_area(void);

/* CSS and theming */
void setup_application_css(void);

/* Compatibility macros for common deprecated functions */
#define gtk_widget_show_all(widget) gtk_widget_show(widget)
#define gtk_main_quit() g_application_quit(G_APPLICATION(g_application_get_default()))

#ifdef __cplusplus
}
#endif

#endif /* __MODERN_WIDGETS_H__ */
