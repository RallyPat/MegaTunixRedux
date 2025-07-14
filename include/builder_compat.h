#ifndef BUILDER_COMPAT_H
#define BUILDER_COMPAT_H

#include <gtk/gtk.h>
#include <glib.h>

/* Global builder management */
void set_global_builder(GtkBuilder *builder);
GtkBuilder *get_global_builder(void);

/* Widget lookup functions */
GtkWidget *lookup_widget_by_name(GtkWidget *widget, const gchar *name);
GtkWidget *glade_xml_get_widget_compat(gpointer unused, const gchar *name);

/* Compatibility macro */
#define glade_xml_get_widget(xml, name) glade_xml_get_widget_compat(xml, name)

#endif /* BUILDER_COMPAT_H */
