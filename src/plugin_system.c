#include "plugin_system.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

/* Create new plugin manager */
MtxPluginManager *mtx_plugin_manager_new(void)
{
    MtxPluginManager *manager = g_new0(MtxPluginManager, 1);
    
    manager->plugins = g_hash_table_new_full(g_str_hash, g_str_equal,
                                           g_free, (GDestroyNotify)mtx_plugin_free);
    manager->plugin_types = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                 NULL, (GDestroyNotify)g_list_free);
    manager->event_listeners = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                    g_free, (GDestroyNotify)g_list_free);
    
    manager->search_paths = g_ptr_array_new_with_free_func(g_free);
    
    g_mutex_init(&manager->plugins_mutex);
    g_mutex_init(&manager->events_mutex);
    
    manager->auto_load = TRUE;
    manager->sandboxing_enabled = FALSE;
    
    return manager;
}

/* Free plugin manager */
void mtx_plugin_manager_free(MtxPluginManager *manager)
{
    if (!manager) return;
    
    mtx_plugin_manager_cleanup(manager);
    
    g_hash_table_destroy(manager->plugins);
    g_hash_table_destroy(manager->plugin_types);
    g_hash_table_destroy(manager->event_listeners);
    g_ptr_array_free(manager->search_paths, TRUE);
    
    g_mutex_clear(&manager->plugins_mutex);
    g_mutex_clear(&manager->events_mutex);
    
    g_free(manager->plugins_dir);
    g_free(manager->config_dir);
    
    if (manager->global_config) {
        json_node_unref(manager->global_config);
    }
    
    g_free(manager);
}

/* Initialize plugin manager */
gboolean mtx_plugin_manager_initialize(MtxPluginManager *manager, 
                                      const gchar *plugins_dir,
                                      GError **error)
{
    g_return_val_if_fail(manager != NULL, FALSE);
    g_return_val_if_fail(plugins_dir != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    manager->plugins_dir = g_strdup(plugins_dir);
    manager->config_dir = g_build_filename(plugins_dir, "config", NULL);
    
    /* Create directories if they don't exist */
    if (g_mkdir_with_parents(manager->plugins_dir, 0755) < 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to create plugins directory: %s", g_strerror(errno));
        return FALSE;
    }
    
    if (g_mkdir_with_parents(manager->config_dir, 0755) < 0) {
        g_set_error(error, G_FILE_ERROR, g_file_error_from_errno(errno),
                   "Failed to create config directory: %s", g_strerror(errno));
        return FALSE;
    }
    
    /* Add default search paths */
    mtx_plugin_manager_add_search_path(manager, manager->plugins_dir);
    
    /* Add system plugin directories */
    mtx_plugin_manager_add_search_path(manager, "/usr/lib/megatunix-redux/plugins");
    mtx_plugin_manager_add_search_path(manager, "/usr/local/lib/megatunix-redux/plugins");
    
    /* Add user plugin directory */
    gchar *user_plugins = g_build_filename(g_get_user_data_dir(), 
                                          "megatunix-redux", "plugins", NULL);
    mtx_plugin_manager_add_search_path(manager, user_plugins);
    g_free(user_plugins);
    
    g_info("Plugin manager initialized with directory: %s", plugins_dir);
    return TRUE;
}

/* Cleanup plugin manager */
void mtx_plugin_manager_cleanup(MtxPluginManager *manager)
{
    if (!manager) return;
    
    g_mutex_lock(&manager->plugins_mutex);
    
    /* Unload all plugins */
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, manager->plugins);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        MtxPlugin *plugin = (MtxPlugin *)value;
        mtx_plugin_cleanup(plugin);
        mtx_plugin_unload(plugin);
    }
    
    g_hash_table_remove_all(manager->plugins);
    g_hash_table_remove_all(manager->plugin_types);
    
    g_mutex_unlock(&manager->plugins_mutex);
    
    g_info("Plugin manager cleanup completed");
}

/* Add search path */
void mtx_plugin_manager_add_search_path(MtxPluginManager *manager, 
                                       const gchar *path)
{
    g_return_if_fail(manager != NULL);
    g_return_if_fail(path != NULL);
    
    /* Check if path already exists */
    for (guint i = 0; i < manager->search_paths->len; i++) {
        if (g_strcmp0(g_ptr_array_index(manager->search_paths, i), path) == 0) {
            return;
        }
    }
    
    g_ptr_array_add(manager->search_paths, g_strdup(path));
    g_debug("Added plugin search path: %s", path);
}

/* Discover plugins */
GList *mtx_plugin_manager_discover_plugins(MtxPluginManager *manager)
{
    g_return_val_if_fail(manager != NULL, NULL);
    
    GList *discovered = NULL;
    
    for (guint i = 0; i < manager->search_paths->len; i++) {
        const gchar *path = g_ptr_array_index(manager->search_paths, i);
        
        if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
            continue;
        }
        
        GDir *dir = g_dir_open(path, 0, NULL);
        if (!dir) continue;
        
        const gchar *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            /* Look for .so files */
            if (!g_str_has_suffix(filename, ".so")) {
                continue;
            }
            
            gchar *full_path = g_build_filename(path, filename, NULL);
            discovered = g_list_prepend(discovered, full_path);
        }
        
        g_dir_close(dir);
    }
    
    g_info("Discovered %d plugin files", g_list_length(discovered));
    return discovered;
}

/* Load plugin */
gboolean mtx_plugin_manager_load_plugin(MtxPluginManager *manager,
                                       const gchar *filename,
                                       GError **error)
{
    g_return_val_if_fail(manager != NULL, FALSE);
    g_return_val_if_fail(filename != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    /* Create plugin instance */
    MtxPlugin *plugin = mtx_plugin_new(filename);
    if (!plugin) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Failed to create plugin instance");
        return FALSE;
    }
    
    /* Load the plugin module */
    if (!mtx_plugin_load(plugin, error)) {
        mtx_plugin_free(plugin);
        return FALSE;
    }
    
    /* Check for conflicts */
    if (!mtx_plugin_check_conflicts(plugin, manager, error)) {
        mtx_plugin_free(plugin);
        return FALSE;
    }
    
    /* Check dependencies */
    if (!mtx_plugin_check_dependencies(plugin, manager, error)) {
        mtx_plugin_free(plugin);
        return FALSE;
    }
    
    g_mutex_lock(&manager->plugins_mutex);
    
    /* Check if plugin is already loaded */
    if (g_hash_table_contains(manager->plugins, plugin->id)) {
        g_mutex_unlock(&manager->plugins_mutex);
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin '%s' is already loaded", plugin->id);
        mtx_plugin_free(plugin);
        return FALSE;
    }
    
    /* Add to plugin registry */
    g_hash_table_insert(manager->plugins, g_strdup(plugin->id), plugin);
    
    /* Add to type registry */
    GList *type_list = g_hash_table_lookup(manager->plugin_types, 
                                          GINT_TO_POINTER(plugin->info->type));
    type_list = g_list_append(type_list, plugin);
    g_hash_table_insert(manager->plugin_types, 
                       GINT_TO_POINTER(plugin->info->type), type_list);
    
    g_mutex_unlock(&manager->plugins_mutex);
    
    /* Create plugin context */
    plugin->context = mtx_plugin_context_new(plugin->id);
    plugin->context->plugin_dir = g_path_get_dirname(plugin->filename);
    plugin->context->config_dir = g_build_filename(manager->config_dir, 
                                                  plugin->id, NULL);
    plugin->context->data_dir = g_build_filename(plugin->context->config_dir,
                                                "data", NULL);
    
    /* Create directories */
    g_mkdir_with_parents(plugin->context->config_dir, 0755);
    g_mkdir_with_parents(plugin->context->data_dir, 0755);
    
    /* Initialize plugin */
    if (!mtx_plugin_initialize(plugin, plugin->context, error)) {
        mtx_plugin_manager_unload_plugin(manager, plugin->id);
        return FALSE;
    }
    
    /* Notify callbacks */
    if (manager->plugin_loaded) {
        manager->plugin_loaded(plugin, manager->callback_data);
    }
    
    g_info("Plugin loaded successfully: %s v%s", 
           plugin->info->name, plugin->info->version);
    
    return TRUE;
}

/* Unload plugin */
void mtx_plugin_manager_unload_plugin(MtxPluginManager *manager,
                                     const gchar *plugin_id)
{
    g_return_if_fail(manager != NULL);
    g_return_if_fail(plugin_id != NULL);
    
    g_mutex_lock(&manager->plugins_mutex);
    
    MtxPlugin *plugin = g_hash_table_lookup(manager->plugins, plugin_id);
    if (!plugin) {
        g_mutex_unlock(&manager->plugins_mutex);
        return;
    }
    
    /* Stop plugin if running */
    mtx_plugin_stop(plugin);
    mtx_plugin_cleanup(plugin);
    
    /* Remove from type registry */
    GList *type_list = g_hash_table_lookup(manager->plugin_types, 
                                          GINT_TO_POINTER(plugin->info->type));
    type_list = g_list_remove(type_list, plugin);
    if (type_list) {
        g_hash_table_insert(manager->plugin_types, 
                           GINT_TO_POINTER(plugin->info->type), type_list);
    } else {
        g_hash_table_remove(manager->plugin_types, 
                           GINT_TO_POINTER(plugin->info->type));
    }
    
    /* Remove from plugin registry */
    g_hash_table_remove(manager->plugins, plugin_id);
    
    g_mutex_unlock(&manager->plugins_mutex);
    
    /* Notify callbacks */
    if (manager->plugin_unloaded) {
        manager->plugin_unloaded(plugin, manager->callback_data);
    }
    
    g_info("Plugin unloaded: %s", plugin_id);
}

/* Create new plugin */
MtxPlugin *mtx_plugin_new(const gchar *filename)
{
    g_return_val_if_fail(filename != NULL, NULL);
    
    MtxPlugin *plugin = g_new0(MtxPlugin, 1);
    
    plugin->filename = g_strdup(filename);
    plugin->directory = g_path_get_dirname(filename);
    plugin->state = MTX_PLUGIN_STATE_UNLOADED;
    plugin->enabled = TRUE;
    plugin->instances = g_hash_table_new_full(g_str_hash, g_str_equal,
                                             g_free, NULL);
    
    g_mutex_init(&plugin->state_mutex);
    
    return plugin;
}

/* Free plugin */
void mtx_plugin_free(MtxPlugin *plugin)
{
    if (!plugin) return;
    
    mtx_plugin_unload(plugin);
    
    g_free(plugin->id);
    g_free(plugin->filename);
    g_free(plugin->directory);
    
    if (plugin->info) {
        mtx_plugin_info_free(plugin->info);
    }
    
    if (plugin->context) {
        mtx_plugin_context_free(plugin->context);
    }
    
    if (plugin->config) {
        json_node_unref(plugin->config);
    }
    
    if (plugin->last_error) {
        g_error_free(plugin->last_error);
    }
    
    g_hash_table_destroy(plugin->instances);
    g_mutex_clear(&plugin->state_mutex);
    
    g_free(plugin);
}

/* Load plugin module */
gboolean mtx_plugin_load(MtxPlugin *plugin, GError **error)
{
    g_return_val_if_fail(plugin != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    g_mutex_lock(&plugin->state_mutex);
    
    if (plugin->state != MTX_PLUGIN_STATE_UNLOADED) {
        g_mutex_unlock(&plugin->state_mutex);
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin is not in unloaded state");
        return FALSE;
    }
    
    plugin->state = MTX_PLUGIN_STATE_LOADING;
    g_mutex_unlock(&plugin->state_mutex);
    
    /* Load module */
    plugin->module = g_module_open(plugin->filename, G_MODULE_BIND_LAZY);
    if (!plugin->module) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Failed to load module: %s", g_module_error());
        return FALSE;
    }
    
    /* Get plugin info function */
    gpointer get_info_func;
    if (!g_module_symbol(plugin->module, "mtx_plugin_get_info", &get_info_func)) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_module_close(plugin->module);
        plugin->module = NULL;
        
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin missing mtx_plugin_get_info function");
        return FALSE;
    }
    
    /* Get plugin interface function */
    gpointer get_interface_func;
    if (!g_module_symbol(plugin->module, "mtx_plugin_get_interface", &get_interface_func)) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_module_close(plugin->module);
        plugin->module = NULL;
        
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin missing mtx_plugin_get_interface function");
        return FALSE;
    }
    
    /* Get plugin info */
    const MtxPluginInfo *(*info_func)(void) = get_info_func;
    const MtxPluginInfo *info = info_func();
    if (!info) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_module_close(plugin->module);
        plugin->module = NULL;
        
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin get_info function returned NULL");
        return FALSE;
    }
    
    /* Validate plugin info */
    if (!mtx_plugin_validate_info(info, error)) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_module_close(plugin->module);
        plugin->module = NULL;
        return FALSE;
    }
    
    /* Get plugin interface */
    const MtxPluginInterface *(*interface_func)(void) = get_interface_func;
    const MtxPluginInterface *interface = interface_func();
    if (!interface) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_module_close(plugin->module);
        plugin->module = NULL;
        
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin get_interface function returned NULL");
        return FALSE;
    }
    
    /* Validate plugin interface */
    if (!mtx_plugin_validate_interface(interface, error)) {
        g_mutex_lock(&plugin->state_mutex);
        plugin->state = MTX_PLUGIN_STATE_ERROR;
        g_mutex_unlock(&plugin->state_mutex);
        
        g_module_close(plugin->module);
        plugin->module = NULL;
        return FALSE;
    }
    
    /* Copy plugin info and interface */
    plugin->info = mtx_plugin_info_copy(info);
    plugin->interface = g_memdup2(interface, sizeof(MtxPluginInterface));
    plugin->id = g_strdup(plugin->info->name);
    plugin->load_time = g_get_real_time();
    
    g_mutex_lock(&plugin->state_mutex);
    plugin->state = MTX_PLUGIN_STATE_LOADED;
    g_mutex_unlock(&plugin->state_mutex);
    
    g_debug("Plugin module loaded: %s", plugin->filename);
    return TRUE;
}

/* Plugin type to string */
const gchar *mtx_plugin_type_to_string(MtxPluginType type)
{
    switch (type) {
        case MTX_PLUGIN_TYPE_DATA_SOURCE: return "data_source";
        case MTX_PLUGIN_TYPE_DATA_PROCESSOR: return "data_processor";
        case MTX_PLUGIN_TYPE_WIDGET: return "widget";
        case MTX_PLUGIN_TYPE_EXPORT: return "export";
        case MTX_PLUGIN_TYPE_THEME: return "theme";
        case MTX_PLUGIN_TYPE_PROTOCOL: return "protocol";
        case MTX_PLUGIN_TYPE_ANALYSIS: return "analysis";
        case MTX_PLUGIN_TYPE_EXTENSION: return "extension";
        default: return "unknown";
    }
}

/* String to plugin type */
MtxPluginType mtx_plugin_type_from_string(const gchar *type_str)
{
    if (!type_str) return MTX_PLUGIN_TYPE_EXTENSION;
    
    if (g_strcmp0(type_str, "data_source") == 0) return MTX_PLUGIN_TYPE_DATA_SOURCE;
    if (g_strcmp0(type_str, "data_processor") == 0) return MTX_PLUGIN_TYPE_DATA_PROCESSOR;
    if (g_strcmp0(type_str, "widget") == 0) return MTX_PLUGIN_TYPE_WIDGET;
    if (g_strcmp0(type_str, "export") == 0) return MTX_PLUGIN_TYPE_EXPORT;
    if (g_strcmp0(type_str, "theme") == 0) return MTX_PLUGIN_TYPE_THEME;
    if (g_strcmp0(type_str, "protocol") == 0) return MTX_PLUGIN_TYPE_PROTOCOL;
    if (g_strcmp0(type_str, "analysis") == 0) return MTX_PLUGIN_TYPE_ANALYSIS;
    if (g_strcmp0(type_str, "extension") == 0) return MTX_PLUGIN_TYPE_EXTENSION;
    
    return MTX_PLUGIN_TYPE_EXTENSION;
}

/* Plugin state to string */
const gchar *mtx_plugin_state_to_string(MtxPluginState state)
{
    switch (state) {
        case MTX_PLUGIN_STATE_UNLOADED: return "unloaded";
        case MTX_PLUGIN_STATE_LOADING: return "loading";
        case MTX_PLUGIN_STATE_LOADED: return "loaded";
        case MTX_PLUGIN_STATE_INITIALIZING: return "initializing";
        case MTX_PLUGIN_STATE_ACTIVE: return "active";
        case MTX_PLUGIN_STATE_ERROR: return "error";
        case MTX_PLUGIN_STATE_DISABLED: return "disabled";
        default: return "unknown";
    }
}

/* Create plugin info */
MtxPluginInfo *mtx_plugin_info_new(void)
{
    return g_new0(MtxPluginInfo, 1);
}

/* Free plugin info */
void mtx_plugin_info_free(MtxPluginInfo *info)
{
    if (!info) return;
    
    g_free(info->name);
    g_free(info->version);
    g_free(info->description);
    g_free(info->author);
    g_free(info->license);
    g_free(info->website);
    g_free(info->api_version);
    g_strfreev(info->dependencies);
    g_strfreev(info->conflicts);
    
    if (info->metadata) {
        json_node_unref(info->metadata);
    }
    
    g_free(info);
}

/* Copy plugin info */
MtxPluginInfo *mtx_plugin_info_copy(const MtxPluginInfo *info)
{
    if (!info) return NULL;
    
    MtxPluginInfo *copy = mtx_plugin_info_new();
    
    copy->name = g_strdup(info->name);
    copy->version = g_strdup(info->version);
    copy->description = g_strdup(info->description);
    copy->author = g_strdup(info->author);
    copy->license = g_strdup(info->license);
    copy->website = g_strdup(info->website);
    copy->api_version = g_strdup(info->api_version);
    copy->type = info->type;
    copy->capabilities = info->capabilities;
    copy->dependencies = g_strdupv(info->dependencies);
    copy->conflicts = g_strdupv(info->conflicts);
    
    if (info->metadata) {
        copy->metadata = json_node_ref((JsonNode *)info->metadata);
    }
    
    return copy;
}

/* Create plugin context */
MtxPluginContext *mtx_plugin_context_new(const gchar *plugin_id)
{
    MtxPluginContext *context = g_new0(MtxPluginContext, 1);
    
    context->plugin_id = g_strdup(plugin_id);
    context->event_handlers = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                   g_free, NULL);
    context->cancellable = g_cancellable_new();
    context->main_context = g_main_context_ref_thread_default();
    
    g_mutex_init(&context->event_mutex);
    
    return context;
}

/* Free plugin context */
void mtx_plugin_context_free(MtxPluginContext *context)
{
    if (!context) return;
    
    g_free(context->plugin_id);
    g_free(context->plugin_dir);
    g_free(context->config_dir);
    g_free(context->data_dir);
    
    if (context->config) {
        json_node_unref(context->config);
    }
    
    g_hash_table_destroy(context->event_handlers);
    g_object_unref(context->cancellable);
    g_main_context_unref(context->main_context);
    
    g_mutex_clear(&context->event_mutex);
    
    g_free(context);
}

/* Validate plugin info */
gboolean mtx_plugin_validate_info(const MtxPluginInfo *info, GError **error)
{
    g_return_val_if_fail(info != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    if (!info->name || strlen(info->name) == 0) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin name is required");
        return FALSE;
    }
    
    if (!info->version || strlen(info->version) == 0) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin version is required");
        return FALSE;
    }
    
    if (!info->api_version || 
        g_strcmp0(info->api_version, MTX_PLUGIN_API_VERSION) != 0) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin API version mismatch. Expected: %s, Got: %s",
                   MTX_PLUGIN_API_VERSION, info->api_version ? info->api_version : "NULL");
        return FALSE;
    }
    
    return TRUE;
}

/* Validate plugin interface */
gboolean mtx_plugin_validate_interface(const MtxPluginInterface *interface, 
                                      GError **error)
{
    g_return_val_if_fail(interface != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);
    
    if (!interface->initialize) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin missing required initialize function");
        return FALSE;
    }
    
    if (!interface->cleanup) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin missing required cleanup function");
        return FALSE;
    }
    
    if (!interface->get_info) {
        g_set_error(error, G_MODULE_ERROR, G_MODULE_ERROR_FAILED,
                   "Plugin missing required get_info function");
        return FALSE;
    }
    
    return TRUE;
}

/* Stub implementations for remaining functions */
gboolean mtx_plugin_initialize(MtxPlugin *plugin, MtxPluginContext *context, GError **error)
{
    /* Implementation would call plugin's initialize function */
    return TRUE;
}

void mtx_plugin_cleanup(MtxPlugin *plugin)
{
    /* Implementation would call plugin's cleanup function */
}

gboolean mtx_plugin_check_dependencies(MtxPlugin *plugin, MtxPluginManager *manager, GError **error)
{
    /* Implementation would check if all dependencies are satisfied */
    return TRUE;
}

gboolean mtx_plugin_check_conflicts(MtxPlugin *plugin, MtxPluginManager *manager, GError **error)
{
    /* Implementation would check for conflicting plugins */
    return TRUE;
}

gboolean mtx_plugin_start(MtxPlugin *plugin, GError **error)
{
    /* Implementation would start the plugin */
    return TRUE;
}

void mtx_plugin_stop(MtxPlugin *plugin)
{
    /* Implementation would stop the plugin */
}

void mtx_plugin_pause(MtxPlugin *plugin)
{
    /* Implementation would pause the plugin */
}

void mtx_plugin_resume(MtxPlugin *plugin)
{
    /* Implementation would resume the plugin */
}

void mtx_plugin_unload(MtxPlugin *plugin)
{
    if (!plugin || !plugin->module) return;
    
    g_module_close(plugin->module);
    plugin->module = NULL;
    
    plugin->state = MTX_PLUGIN_STATE_UNLOADED;
}
