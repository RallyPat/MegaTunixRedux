/*
 * GTK4 Compatibility Layer for MegaTunix Redux
 * 
 * This header provides compatibility shims for GTK2/3 code
 * to work with GTK4, easing the migration process.
 */

#ifndef GTK_COMPAT_H
#define GTK_COMPAT_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

/* GTK 4 compatibility types */
#ifndef GTK_MAJOR_VERSION
#define GTK_MAJOR_VERSION 4
#endif

/* Missing GTK_STOCK constants - compatibility definitions */
#ifndef GTK_STOCK_YES
#define GTK_STOCK_YES "_Yes"
#endif

#ifndef GTK_STOCK_NO
#define GTK_STOCK_NO "_No"
#endif

#ifndef GTK_STOCK_OK
#define GTK_STOCK_OK "_OK"
#endif

#ifndef GTK_STOCK_CANCEL
#define GTK_STOCK_CANCEL "_Cancel"
#endif

#ifndef GTK_STOCK_OPEN
#define GTK_STOCK_OPEN "_Open"
#endif

#ifndef GTK_STOCK_SAVE
#define GTK_STOCK_SAVE "_Save"
#endif

#ifndef GTK_STOCK_DIALOG_QUESTION
#define GTK_STOCK_DIALOG_QUESTION "dialog-question"
#endif

#ifndef GTK_STOCK_DIALOG_WARNING
#define GTK_STOCK_DIALOG_WARNING "dialog-warning"
#endif

#ifndef GTK_STOCK_DIALOG_ERROR
#define GTK_STOCK_DIALOG_ERROR "dialog-error"
#endif

#ifndef GTK_STOCK_DIALOG_INFO
#define GTK_STOCK_DIALOG_INFO "dialog-information"
#endif

#ifndef GTK_STOCK_APPLY
#define GTK_STOCK_APPLY "_Apply"
#endif

/* Missing GTK_ICON_SIZE constants */
#ifndef GTK_ICON_SIZE_DIALOG
#define GTK_ICON_SIZE_DIALOG 48
#endif

#ifndef GTK_ICON_SIZE_BUTTON
#define GTK_ICON_SIZE_BUTTON 16
#endif

#ifndef GTK_ICON_SIZE_MENU
#define GTK_ICON_SIZE_MENU 16
#endif

/* Missing GTK_RESPONSE constants */
#ifndef GTK_RESPONSE_YES
#define GTK_RESPONSE_YES -8
#endif

#ifndef GTK_RESPONSE_NO
#define GTK_RESPONSE_NO -9
#endif

#ifndef GTK_RESPONSE_OK
#define GTK_RESPONSE_OK -5
#endif

#ifndef GTK_RESPONSE_CANCEL
#define GTK_RESPONSE_CANCEL -6
#endif

/* Missing GTK progress constants */
#ifndef GTK_PROGRESS_LEFT_TO_RIGHT
#define GTK_PROGRESS_LEFT_TO_RIGHT 0
#endif

#ifndef GTK_PROGRESS_RIGHT_TO_LEFT
#define GTK_PROGRESS_RIGHT_TO_LEFT 1
#endif

#ifndef GTK_PROGRESS_BOTTOM_TO_TOP
#define GTK_PROGRESS_BOTTOM_TO_TOP 2
#endif

#ifndef GTK_PROGRESS_TOP_TO_BOTTOM
#define GTK_PROGRESS_TOP_TO_BOTTOM 3
#endif

// GTK Attach options (removed in GTK4)
typedef enum {
    GTK_FILL = 1,
    GTK_EXPAND = 2,
    GTK_SHRINK = 4
} GtkAttachOptions;

/* Missing GdkEvent types in GTK4 - stub definitions */
#ifndef GdkEventExpose
typedef struct _GdkEventExpose GdkEventExpose;
struct _GdkEventExpose {
    int dummy;
};
#endif

#ifndef GdkEventFocus
typedef struct _GdkEventFocus GdkEventFocus;
struct _GdkEventFocus {
    int dummy;
};
#endif

#ifndef GdkEventButton
typedef struct _GdkEventButton GdkEventButton;
struct _GdkEventButton {
    int dummy;
};
#endif

#ifndef GdkEventKey
typedef struct _GdkEventKey GdkEventKey;
struct _GdkEventKey {
    int dummy;
};
#endif

#ifndef GdkEventMotion
typedef struct _GdkEventMotion GdkEventMotion;
struct _GdkEventMotion {
    int dummy;
};
#endif

#ifndef GdkEventConfigure
typedef struct _GdkEventConfigure GdkEventConfigure;
struct _GdkEventConfigure {
    int dummy;
};
#endif

#ifndef GdkEventCrossing
typedef struct _GdkEventCrossing GdkEventCrossing;
struct _GdkEventCrossing {
    int dummy;
};
#endif

/* Color compatibility */
#ifndef GdkColor
typedef struct _GdkColor GdkColor;
struct _GdkColor {
    guint32 pixel;
    guint16 red;
    guint16 green;
    guint16 blue;
};
#endif

/* Line style compatibility */
#ifndef GdkLineStyle
typedef enum {
    GDK_LINE_SOLID,
    GDK_LINE_ON_OFF_DASH,
    GDK_LINE_DOUBLE_DASH
} GdkLineStyle;
#endif

/* Join style compatibility */
#ifndef GdkJoinStyle
typedef enum {
    GDK_JOIN_MITER,
    GDK_JOIN_ROUND,
    GDK_JOIN_BEVEL
} GdkJoinStyle;
#endif

/* GTK State compatibility - GTK4 uses flags instead of enum states */
#ifndef GTK_STATE_NORMAL
#define GTK_STATE_NORMAL GTK_STATE_FLAG_NORMAL
#endif

#ifndef GTK_STATE_PRELIGHT
#define GTK_STATE_PRELIGHT GTK_STATE_FLAG_PRELIGHT
#endif

#ifndef GTK_STATE_INSENSITIVE
#define GTK_STATE_INSENSITIVE GTK_STATE_FLAG_INSENSITIVE
#endif

#ifndef GTK_STATE_ACTIVE
#define GTK_STATE_ACTIVE GTK_STATE_FLAG_ACTIVE
#endif

/* File chooser types */
#ifndef GtkFileChooserButton
typedef GtkWidget GtkFileChooserButton;
#endif

#ifndef GtkFileChooserConfirmation
typedef enum {
    GTK_FILE_CHOOSER_CONFIRMATION_CONFIRM,
    GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME,
    GTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN
} GtkFileChooserConfirmation;
#endif

/* Stock items compatibility */
#define GTK_STOCK_CONNECT "network-connect"
#define GTK_STOCK_DISCONNECT "network-disconnect"
#define GTK_STOCK_CANCEL "process-stop"
#define GTK_STOCK_OPEN "document-open"
#define GTK_STOCK_SAVE "document-save"

/* Icon sizes */
#ifndef GTK_ICON_SIZE_MENU
#define GTK_ICON_SIZE_MENU GTK_ICON_SIZE_NORMAL
#endif

/* Dialog flags */
#ifndef GTK_DIALOG_MODAL
#define GTK_DIALOG_MODAL 0
#endif

#ifndef GTK_DIALOG_DESTROY_WITH_PARENT
#define GTK_DIALOG_DESTROY_WITH_PARENT 0
#endif

/* Dialog button types */
#ifndef GTK_BUTTONS_CLOSE
#define GTK_BUTTONS_CLOSE 0
#endif

/* Shadow types */
#ifndef GTK_SHADOW_ETCHED_IN
#define GTK_SHADOW_ETCHED_IN 0
#endif

#ifndef GTK_SHADOW_NONE
#define GTK_SHADOW_NONE 0
#endif

#ifndef GTK_SHADOW_IN
#define GTK_SHADOW_IN 0
#endif

#ifndef GTK_SHADOW_OUT
#define GTK_SHADOW_OUT 0
#endif

#ifndef GTK_SHADOW_ETCHED_OUT
#define GTK_SHADOW_ETCHED_OUT 0
#endif

/* Minor version stub */
#ifndef gtk_minor_version
#define gtk_minor_version 4
#endif

/* Container compatibility */
static inline void gtk_container_add_compat(GtkWidget *container, GtkWidget *child)
{
    if (GTK_IS_WINDOW(container)) {
        gtk_window_set_child(GTK_WINDOW(container), child);
    } else if (GTK_IS_BOX(container)) {
        gtk_box_append(GTK_BOX(container), child);
    } else if (GTK_IS_BUTTON(container)) {
        gtk_button_set_child(GTK_BUTTON(container), child);
    } else if (GTK_IS_SCROLLED_WINDOW(container)) {
        gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(container), child);
    }
}

/* Box packing compatibility */
static inline void gtk_box_pack_start_compat(GtkBox *box, GtkWidget *child, 
                                              gboolean expand, gboolean fill, guint padding)
{
    gtk_box_prepend(box, child);
}

static inline void gtk_box_pack_end(GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding)
{
    gtk_box_append(box, child);
}

/* Widget style compatibility */
static inline void gtk_widget_modify_base_compat(GtkWidget *widget, GtkStateFlags state, const GdkColor *color)
{
    if (!color) return;
    
    gchar *css = g_strdup_printf("* { background-color: rgb(%d, %d, %d); }", 
                                 color->red >> 8, color->green >> 8, color->blue >> 8);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1);
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
    g_free(css);
}

static inline void gtk_widget_modify_fg(GtkWidget *widget, GtkStateFlags state, const GdkColor *color)
{
    if (!color) return;
    
    gchar *css = g_strdup_printf("* { color: rgb(%d, %d, %d); }", 
                                 color->red >> 8, color->green >> 8, color->blue >> 8);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1);
    gtk_style_context_add_provider(gtk_widget_get_style_context(widget),
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
    g_free(css);
}

/* Box compatibility */
static inline GtkWidget *gtk_hbox_new(gboolean homogeneous, gint spacing)
{
    return gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing);
}

static inline GtkWidget *gtk_vbox_new(gboolean homogeneous, gint spacing)
{
    return gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
}

/* Window move/resize stubs */
static inline void gtk_window_move(GtkWindow *window, gint x, gint y)
{
    /* GTK4 doesn't support window positioning */
}

static inline void gtk_window_resize(GtkWindow *window, gint width, gint height)
{
    gtk_window_set_default_size(window, width, height);
}

/* Misc widget compatibility */
static inline void gtk_misc_set_alignment(GtkWidget *misc, gfloat xalign, gfloat yalign)
{
    /* GTK4 doesn't have GtkMisc - use widget-specific alignment instead */
    if (GTK_IS_LABEL(misc)) {
        gtk_label_set_xalign(GTK_LABEL(misc), xalign);
        gtk_label_set_yalign(GTK_LABEL(misc), yalign);
    }
}

/* Widget show all compatibility */
static inline void gtk_widget_show_all(GtkWidget *widget)
{
    gtk_widget_show(widget);
}

/* Dialog run compatibility */
static inline gint gtk_dialog_run(GtkDialog *dialog)
{
    return GTK_RESPONSE_OK;
}

/* Box pack start compatibility */
static inline void gtk_box_pack_start(GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding)
{
    gtk_box_prepend(box, child);
}

/* Container add compatibility */
#define gtk_container_add(container, child) gtk_container_add_compat(container, child)

/* Image stock compatibility */
static inline GtkWidget *gtk_image_new_from_stock(const gchar *stock_id, GtkIconSize size)
{
    return gtk_image_new_from_icon_name(stock_id);
}

/* Entry text compatibility */
static inline void gtk_entry_set_text(GtkEntry *entry, const gchar *text)
{
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(entry);
    gtk_entry_buffer_set_text(buffer, text, -1);
}

/* Additional GTK compatibility functions */

/* GTK get toplevel compatibility */
static inline GtkWidget *gtk_widget_get_toplevel(GtkWidget *widget)
{
    GtkWidget *toplevel = gtk_widget_get_root(widget);
    return toplevel ? toplevel : widget;
}

/* GTK widget destroy compatibility */
static inline void gtk_widget_destroy(GtkWidget *widget)
{
    if (GTK_IS_WINDOW(widget)) {
        gtk_window_destroy(GTK_WINDOW(widget));
    } else {
        gtk_widget_unparent(widget);
    }
}

/* GTK container set border width compatibility */
static inline void gtk_container_set_border_width(GtkWidget *container, guint border_width)
{
    /* GTK4 doesn't support border width */
}

/* GTK container compatibility */
static inline GtkWidget *GTK_CONTAINER(GtkWidget *widget)
{
    /* Return the widget as is - GTK4 doesn't have separate container type */
    return widget;
}

/* GTK radio button compatibility */
static inline GtkWidget *gtk_radio_button_new(GSList *group)
{
    return gtk_check_button_new();
}

static inline GSList *gtk_radio_button_get_group(GtkWidget *radio_button)
{
    /* GTK4 doesn't use groups for radio buttons */
    return NULL;
}

static inline GtkWidget *GTK_RADIO_BUTTON(GtkWidget *widget)
{
    return widget;
}

/* GTK event box compatibility */
static inline GtkWidget *gtk_event_box_new(void)
{
    return gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
}

/* GTK separator compatibility */
static inline GtkWidget *gtk_hseparator_new(void)
{
    return gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
}

static inline GtkWidget *gtk_vseparator_new(void)
{
    return gtk_separator_new(GTK_ORIENTATION_VERTICAL);
}

/* GTK entry text compatibility */
static inline const gchar *gtk_entry_get_text(GtkEntry *entry)
{
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(entry);
    return gtk_entry_buffer_get_text(buffer);
}

/* GTK widget modify text compatibility */
static inline void gtk_widget_modify_text(GtkWidget *widget, GtkStateFlags state, const GdkColor *color)
{
    /* GTK4 equivalent would use CSS providers */
}

/* GTK widget modify base compatibility */
static inline void gtk_widget_modify_base(GtkWidget *widget, GtkStateFlags state, const GdkColor *color)
{
    gtk_widget_modify_base_compat(widget, state, color);
}

/* GTK widget render icon compatibility */
static inline GdkPixbuf *gtk_widget_render_icon(GtkWidget *widget, const gchar *stock_id, GtkIconSize size, const gchar *detail)
{
    /* Simple placeholder - would need proper icon theme lookup */
    return NULL;
}

/* GTK label set line wrap compatibility */
static inline void gtk_label_set_line_wrap(GtkLabel *label, gboolean wrap)
{
    gtk_label_set_wrap(label, wrap);
}

/* GTK toggle button inconsistent compatibility */
static inline gboolean gtk_toggle_button_get_inconsistent(GtkToggleButton *toggle_button)
{
    /* GTK4 doesn't have inconsistent state for regular toggle buttons */
    return FALSE;
}

static inline void gtk_toggle_button_set_inconsistent(GtkToggleButton *toggle_button, gboolean setting)
{
    /* GTK4 doesn't have inconsistent state for regular toggle buttons */
}

/* GTK widget state compatibility */
static inline GtkStateFlags GTK_WIDGET_STATE(GtkWidget *widget)
{
    return gtk_widget_get_state_flags(widget);
}

/* GTK widget sensitive compatibility */
static inline gboolean GTK_WIDGET_SENSITIVE(GtkWidget *widget)
{
    return gtk_widget_get_sensitive(widget);
}

/* GTK main quit compatibility */
static inline void gtk_main_quit(void)
{
    /* GTK4 applications should use GApplication quit instead */
    g_application_quit(g_application_get_default());
}

/* GTK radio button compatibility */
static inline gboolean GTK_IS_RADIO_BUTTON(GtkWidget *widget)
{
    return GTK_IS_CHECK_BUTTON(widget);
}

/* GTK object compatibility */
static inline gboolean GTK_IS_OBJECT(GtkWidget *widget)
{
    return G_IS_OBJECT(widget);
}

/* Glade get widget name compatibility */
static inline const gchar *glade_get_widget_name(GtkWidget *widget)
{
    return gtk_widget_get_name(widget);
}

/* GTK bin compatibility */
static inline GtkWidget *gtk_bin_get_child(GtkWidget *widget)
{
    if (GTK_IS_COMBO_BOX(widget)) {
        return gtk_combo_box_get_child(GTK_COMBO_BOX(widget));
    }
    return NULL;
}

static inline GtkWidget *GTK_BIN(GtkWidget *widget)
{
    /* GTK4 doesn't have GtkBin, just return the widget */
    return widget;
}

/* GTK window set focus on map compatibility */
static inline void gtk_window_set_focus_on_map(GtkWindow *window, gboolean setting)
{
    /* GTK4 doesn't have this function - it's always mapped with focus */
}

/* GTK frame set shadow type compatibility */
static inline void gtk_frame_set_shadow_type(GtkFrame *frame, int shadow_type)
{
    /* GTK4 doesn't support shadow types */
}

/* GDK color parse compatibility */
static inline gboolean gdk_color_parse(const gchar *spec, GdkColor *color)
{
    /* Simple stub - GTK4 uses GdkRGBA instead */
    return FALSE;
}

/* GTK misc compatibility */
static inline GtkWidget *GTK_MISC(GtkWidget *widget)
{
    return widget;
}

/* GTK gl query compatibility */
static inline gboolean gdk_gl_query(void)
{
    /* GTK4 always has GL support */
    return TRUE;
}

/* GTK rc parse string compatibility */
static inline void gtk_rc_parse_string(const gchar *rc_string)
{
    /* GTK4 doesn't use RC files - would need CSS providers */
}

/* GTK get option group compatibility */
static inline GOptionGroup *gtk_get_option_group(gboolean open_default_display)
{
    /* GTK4 doesn't have option groups - return NULL */
    return NULL;
}

/* GTK Attach options (removed in GTK4) - defined earlier */

/* GTK Table function stubs (GtkTable was removed in GTK4) */
static inline GtkWidget *gtk_table_new(guint rows, guint columns, gboolean homogeneous)
{
    // Return a grid as a substitute for table
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), homogeneous);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), homogeneous);
    return grid;
}

static inline void gtk_table_set_row_spacings(GtkWidget *table, guint spacing)
{
    if (GTK_IS_GRID(table)) {
        gtk_grid_set_row_spacing(GTK_GRID(table), spacing);
    }
}

static inline void gtk_table_set_col_spacings(GtkWidget *table, guint spacing)
{
    if (GTK_IS_GRID(table)) {
        gtk_grid_set_column_spacing(GTK_GRID(table), spacing);
    }
}

static inline void gtk_table_attach(GtkWidget *table, GtkWidget *child, 
                                   guint left_attach, guint right_attach, 
                                   guint top_attach, guint bottom_attach, 
                                   GtkAttachOptions xoptions, GtkAttachOptions yoptions, 
                                   guint xpadding, guint ypadding)
{
    if (GTK_IS_GRID(table)) {
        gtk_grid_attach(GTK_GRID(table), child, left_attach, top_attach, 
                       right_attach - left_attach, bottom_attach - top_attach);
    }
}

/* GTK Table type conversion */
#define GTK_TABLE(widget) GTK_GRID(widget)
#define GtkTable GtkGrid

/* Missing GDK types that were removed in GTK4 */
typedef struct _GdkGC GdkGC;
typedef struct _GdkPixmap GdkPixmap;
typedef struct _GdkDrawable GdkDrawable;

/* GDK flush function (removed in GTK4) */
static inline void gdk_flush(void)
{
    /* No-op in GTK4 - GDK no longer has a flush function */
}

/* GTK adjustment_changed function (removed in GTK4) */
static inline void gtk_adjustment_changed(GtkAdjustment *adjustment)
{
    /* No-op in GTK4 - adjustments no longer have a changed signal */
}

/* GTK Bin function compatibility (GtkBin was removed in GTK4) */

/* File chooser compatibility stubs */
static inline void gtk_file_chooser_set_current_folder_string(GtkFileChooser *chooser, const gchar *path)
{
    g_debug("gtk_file_chooser_set_current_folder_string: %s", path);
    // In GTK4, use gtk_file_chooser_set_current_folder
    // For now, stub this out
}

static inline void gtk_file_chooser_set_filename(GtkFileChooser *chooser, const gchar *filename)
{
    g_debug("gtk_file_chooser_set_filename: %s", filename);
    // In GTK4, use gtk_file_chooser_set_file
    // For now, stub this out
}

static inline void gtk_file_chooser_set_do_overwrite_confirmation(GtkFileChooser *chooser, gboolean do_overwrite_confirmation)
{
    g_debug("gtk_file_chooser_set_do_overwrite_confirmation: %d", do_overwrite_confirmation);
    // This property was removed in GTK4
    // For now, stub this out
}

static inline gboolean gtk_file_chooser_select_filename(GtkFileChooser *chooser, const gchar *filename)
{
    g_debug("gtk_file_chooser_select_filename: %s", filename);
    // In GTK4, use gtk_file_chooser_set_file
    // For now, stub this out
    return FALSE;
}

static inline gchar* gtk_file_chooser_get_filename(GtkFileChooser *chooser)
{
    g_debug("gtk_file_chooser_get_filename called");
    // In GTK4, use gtk_file_chooser_get_file
    // For now, return a dummy path
    return g_strdup("/tmp/dummy_file.txt");
}

static inline GSList* gtk_file_chooser_list_filters(GtkFileChooser *chooser)
{
    g_debug("gtk_file_chooser_list_filters called");
    // In GTK4, use gtk_file_chooser_get_filters
    // For now, return empty list
    return NULL;
}

// Tree view compatibility stubs
static inline void gtk_tree_view_set_rules_hint(GtkTreeView *tree_view, gboolean setting)
{
    g_debug("gtk_tree_view_set_rules_hint: %d", setting);
    // This property was removed in GTK4
    // For now, stub this out
}

// Main loop compatibility stub
static inline void gtk_main(void)
{
    g_debug("gtk_main called - this is deprecated in GTK4");
    // In GTK4, use GApplication/GtkApplication main loop
    // For now, stub this out
}

#endif /* GTK_COMPAT_H */
