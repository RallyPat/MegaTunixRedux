#ifndef __PLUGIN_SYSTEM_H__
#define __PLUGIN_SYSTEM_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <json-glib/json-glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Modern Plugin System for MegaTunix Redux */

/* Plugin API version */
#define MTX_PLUGIN_API_VERSION "1.0"

/* Plugin types */
typedef enum {
    MTX_PLUGIN_TYPE_DATA_SOURCE,      /* ECU communication plugins */
    MTX_PLUGIN_TYPE_DATA_PROCESSOR,   /* Data transformation plugins */
    MTX_PLUGIN_TYPE_WIDGET,           /* Custom widget plugins */
    MTX_PLUGIN_TYPE_EXPORT,           /* Data export plugins */
    MTX_PLUGIN_TYPE_THEME,            /* Theme and styling plugins */
    MTX_PLUGIN_TYPE_PROTOCOL,         /* Communication protocol plugins */
    MTX_PLUGIN_TYPE_ANALYSIS,         /* Data analysis plugins */
    MTX_PLUGIN_TYPE_EXTENSION         /* General extension plugins */
} MtxPluginType;

/* Plugin states */
typedef enum {
    MTX_PLUGIN_STATE_UNLOADED,
    MTX_PLUGIN_STATE_LOADING,
    MTX_PLUGIN_STATE_LOADED,
    MTX_PLUGIN_STATE_INITIALIZING,
    MTX_PLUGIN_STATE_ACTIVE,
    MTX_PLUGIN_STATE_ERROR,
    MTX_PLUGIN_STATE_DISABLED
} MtxPluginState;

/* Plugin capabilities */
typedef enum {
    MTX_PLUGIN_CAP_CONFIGURABLE     = 1 << 0,  /* Has configuration options */
    MTX_PLUGIN_CAP_REAL_TIME        = 1 << 1,  /* Handles real-time data */
    MTX_PLUGIN_CAP_BACKGROUND       = 1 << 2,  /* Runs in background */
    MTX_PLUGIN_CAP_NETWORK          = 1 << 3,  /* Uses network */
    MTX_PLUGIN_CAP_FILE_IO          = 1 << 4,  /* File input/output */
    MTX_PLUGIN_CAP_HARDWARE         = 1 << 5,  /* Hardware access */
    MTX_PLUGIN_CAP_MULTI_INSTANCE   = 1 << 6,  /* Multiple instances allowed */
    MTX_PLUGIN_CAP_GUI              = 1 << 7   /* Provides GUI elements */
} MtxPluginCapabilities;

/* Forward declarations */
typedef struct _MtxPlugin MtxPlugin;
typedef struct _MtxPluginManager MtxPluginManager;
typedef struct _MtxPluginInfo MtxPluginInfo;
typedef struct _MtxPluginInterface MtxPluginInterface;
typedef struct _MtxPluginContext MtxPluginContext;

/* Plugin information structure */
struct _MtxPluginInfo {
    gchar *name;
    gchar *version;
    gchar *description;
    gchar *author;
    gchar *license;
    gchar *website;
    gchar *api_version;
    MtxPluginType type;
    MtxPluginCapabilities capabilities;
    gchar **dependencies;
    gchar **conflicts;
    JsonNode *metadata;
};

/* Plugin interface - functions that plugins must implement */
struct _MtxPluginInterface {
    /* Required functions */
    gboolean (*initialize)(MtxPlugin *plugin, MtxPluginContext *context, GError **error);
    void (*cleanup)(MtxPlugin *plugin);
    const MtxPluginInfo *(*get_info)(void);
    
    /* Optional functions */
    gboolean (*configure)(MtxPlugin *plugin, JsonNode *config, GError **error);
    JsonNode *(*get_config)(MtxPlugin *plugin);
    gboolean (*start)(MtxPlugin *plugin, GError **error);
    void (*stop)(MtxPlugin *plugin);
    void (*pause)(MtxPlugin *plugin);
    void (*resume)(MtxPlugin *plugin);
    
    /* Data handling functions */
    gboolean (*process_data)(MtxPlugin *plugin, JsonNode *input, JsonNode **output, GError **error);
    void (*data_updated)(MtxPlugin *plugin, const gchar *source, JsonNode *data);
    
    /* GUI functions */
    GtkWidget *(*create_widget)(MtxPlugin *plugin);
    GtkWidget *(*create_config_widget)(MtxPlugin *plugin);
    void (*update_widget)(MtxPlugin *plugin, GtkWidget *widget, JsonNode *data);
    
    /* Event handlers */
    void (*on_connect)(MtxPlugin *plugin);
    void (*on_disconnect)(MtxPlugin *plugin);
    void (*on_error)(MtxPlugin *plugin, GError *error);
};

/* Plugin context - provides access to MegaTunix Redux services */
struct _MtxPluginContext {
    /* Core services */
    gpointer core_api;           /* Pointer to core API functions */
    gpointer data_manager;       /* Data management service */
    gpointer gui_manager;        /* GUI management service */
    gpointer config_manager;     /* Configuration service */
    gpointer log_manager;        /* Logging service */
    
    /* Plugin-specific data */
    gchar *plugin_id;
    gchar *plugin_dir;
    gchar *config_dir;
    gchar *data_dir;
    JsonNode *config;
    
    /* Communication */
    GHashTable *event_handlers;  /* Event name -> callback function */
    GMutex event_mutex;
    
    /* Resources */
    GCancellable *cancellable;
    GMainContext *main_context;
};

/* Plugin structure */
struct _MtxPlugin {
    gchar *id;
    gchar *filename;
    gchar *directory;
    GModule *module;
    MtxPluginInfo *info;
    MtxPluginInterface *interface;
    MtxPluginContext *context;
    MtxPluginState state;
    gboolean enabled;
    JsonNode *config;
    GHashTable *instances;       /* For multi-instance plugins */
    GMutex state_mutex;
    gint64 load_time;
    gint64 last_activity;
    GError *last_error;
};

/* Plugin manager */
struct _MtxPluginManager {
    GHashTable *plugins;         /* plugin_id -> MtxPlugin */
    GHashTable *plugin_types;    /* type -> GList of plugins */
    GMutex plugins_mutex;
    
    gchar *plugins_dir;
    gchar *config_dir;
    
    /* Plugin search paths */
    GPtrArray *search_paths;
    
    /* Event system */
    GHashTable *event_listeners; /* event_name -> GList of callbacks */
    GMutex events_mutex;
    
    /* Configuration */
    JsonNode *global_config;
    gboolean auto_load;
    gboolean sandboxing_enabled;
    
    /* Callbacks */
    void (*plugin_loaded)(MtxPlugin *plugin, gpointer user_data);
    void (*plugin_unloaded)(MtxPlugin *plugin, gpointer user_data);
    void (*plugin_error)(MtxPlugin *plugin, GError *error, gpointer user_data);
    gpointer callback_data;
};

/* Plugin manager functions */
MtxPluginManager *mtx_plugin_manager_new(void);
void mtx_plugin_manager_free(MtxPluginManager *manager);

gboolean mtx_plugin_manager_initialize(MtxPluginManager *manager, 
                                      const gchar *plugins_dir,
                                      GError **error);
void mtx_plugin_manager_cleanup(MtxPluginManager *manager);

/* Plugin discovery and loading */
void mtx_plugin_manager_add_search_path(MtxPluginManager *manager, 
                                       const gchar *path);
GList *mtx_plugin_manager_discover_plugins(MtxPluginManager *manager);
gboolean mtx_plugin_manager_load_plugin(MtxPluginManager *manager,
                                       const gchar *filename,
                                       GError **error);
void mtx_plugin_manager_unload_plugin(MtxPluginManager *manager,
                                     const gchar *plugin_id);

/* Plugin management */
gboolean mtx_plugin_manager_enable_plugin(MtxPluginManager *manager,
                                         const gchar *plugin_id,
                                         GError **error);
void mtx_plugin_manager_disable_plugin(MtxPluginManager *manager,
                                      const gchar *plugin_id);
gboolean mtx_plugin_manager_configure_plugin(MtxPluginManager *manager,
                                            const gchar *plugin_id,
                                            JsonNode *config,
                                            GError **error);

/* Plugin queries */
MtxPlugin *mtx_plugin_manager_get_plugin(MtxPluginManager *manager,
                                        const gchar *plugin_id);
GList *mtx_plugin_manager_get_plugins_by_type(MtxPluginManager *manager,
                                             MtxPluginType type);
GList *mtx_plugin_manager_get_all_plugins(MtxPluginManager *manager);
GList *mtx_plugin_manager_get_active_plugins(MtxPluginManager *manager);

/* Event system */
void mtx_plugin_manager_register_event_listener(MtxPluginManager *manager,
                                               const gchar *event_name,
                                               GCallback callback,
                                               gpointer user_data);
void mtx_plugin_manager_unregister_event_listener(MtxPluginManager *manager,
                                                 const gchar *event_name,
                                                 GCallback callback);
void mtx_plugin_manager_emit_event(MtxPluginManager *manager,
                                  const gchar *event_name,
                                  JsonNode *event_data);

/* Plugin functions */
MtxPlugin *mtx_plugin_new(const gchar *filename);
void mtx_plugin_free(MtxPlugin *plugin);

gboolean mtx_plugin_load(MtxPlugin *plugin, GError **error);
void mtx_plugin_unload(MtxPlugin *plugin);
gboolean mtx_plugin_initialize(MtxPlugin *plugin, MtxPluginContext *context, GError **error);
void mtx_plugin_cleanup(MtxPlugin *plugin);

gboolean mtx_plugin_start(MtxPlugin *plugin, GError **error);
void mtx_plugin_stop(MtxPlugin *plugin);
void mtx_plugin_pause(MtxPlugin *plugin);
void mtx_plugin_resume(MtxPlugin *plugin);

gboolean mtx_plugin_configure(MtxPlugin *plugin, JsonNode *config, GError **error);
JsonNode *mtx_plugin_get_config(MtxPlugin *plugin);

/* Plugin context functions */
MtxPluginContext *mtx_plugin_context_new(const gchar *plugin_id);
void mtx_plugin_context_free(MtxPluginContext *context);
void mtx_plugin_context_set_services(MtxPluginContext *context,
                                    gpointer core_api,
                                    gpointer data_manager,
                                    gpointer gui_manager,
                                    gpointer config_manager,
                                    gpointer log_manager);

/* Plugin info functions */
MtxPluginInfo *mtx_plugin_info_new(void);
void mtx_plugin_info_free(MtxPluginInfo *info);
MtxPluginInfo *mtx_plugin_info_copy(const MtxPluginInfo *info);

/* Utility functions */
const gchar *mtx_plugin_type_to_string(MtxPluginType type);
MtxPluginType mtx_plugin_type_from_string(const gchar *type_str);
const gchar *mtx_plugin_state_to_string(MtxPluginState state);
gchar *mtx_plugin_capabilities_to_string(MtxPluginCapabilities caps);

gboolean mtx_plugin_check_dependencies(MtxPlugin *plugin, 
                                      MtxPluginManager *manager,
                                      GError **error);
gboolean mtx_plugin_check_conflicts(MtxPlugin *plugin,
                                   MtxPluginManager *manager,
                                   GError **error);

/* Plugin data exchange functions */
gboolean mtx_plugin_send_data(MtxPlugin *plugin, 
                             const gchar *target_plugin,
                             JsonNode *data,
                             GError **error);
void mtx_plugin_register_data_handler(MtxPlugin *plugin,
                                     const gchar *data_type,
                                     GCallback handler);

/* Plugin sandboxing (future implementation) */
gboolean mtx_plugin_enable_sandbox(MtxPlugin *plugin, GError **error);
void mtx_plugin_disable_sandbox(MtxPlugin *plugin);

/* Configuration helpers */
gboolean mtx_plugin_manager_load_config(MtxPluginManager *manager,
                                       const gchar *config_file,
                                       GError **error);
gboolean mtx_plugin_manager_save_config(MtxPluginManager *manager,
                                       const gchar *config_file,
                                       GError **error);

/* Plugin validation */
gboolean mtx_plugin_validate_info(const MtxPluginInfo *info, GError **error);
gboolean mtx_plugin_validate_interface(const MtxPluginInterface *interface, 
                                      GError **error);

/* Standard plugin entry points (for plugin implementers) */
#define MTX_PLUGIN_DEFINE(get_info_func, interface_var) \
    G_MODULE_EXPORT const MtxPluginInfo *mtx_plugin_get_info(void) { \
        return get_info_func(); \
    } \
    G_MODULE_EXPORT const MtxPluginInterface *mtx_plugin_get_interface(void) { \
        return &interface_var; \
    }

/* Plugin helper macros */
#define MTX_PLUGIN_INFO_INIT(name, version, desc, author) \
    { \
        .name = name, \
        .version = version, \
        .description = desc, \
        .author = author, \
        .api_version = MTX_PLUGIN_API_VERSION \
    }

G_END_DECLS

#endif /* __PLUGIN_SYSTEM_H__ */
