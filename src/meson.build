# Source files
src_files = [
  'main.c',
  # 'core_gui.c',  # Replaced by core_gui_modern.c for GTK4 migration
  'core_gui_modern.c',  # Modern GTK4 implementation
  'modern_widgets.c',
  'security_utils.c',
  # 'mtx_application.c',  # Temporarily disabled - has conflicting main function
  'modern_opengl.c',
  'network_security.c',
  'plugin_system.c',
  'test_framework.c',
  'init.c',
  'notifications.c',
  # 'dashboard.c',  # GTK4 migration in progress - complex event handling needs more work
  'gui_handlers.c',
  'offline.c',
  'runtime_status.c',
  'widgetmgmt.c',  # Re-enabling - will fix GTK4 compatibility
  'args.c',
  'binlogger.c',
  'debugging.c',
  'dispatcher.c',
  'locking.c',
  'serialio.c',
  'sleep_calib.c',
  'stringmatch.c',
  'timeout_handlers.c',
  'conversions.c',
  'lookuptables.c',
  'plugin.c',
  'tabloader.c',  # Re-enabled - migrating to GTK4
  # '3d_vetable.c', # Temporarily disabled - requires GL and extensive GTK4 porting
  # Add serial utilities and Speeduino plugin
  'serial_utils.c',
  'speeduino_plugin.c',
  'speeduino_bridge.c',  # Add bridge for simplified plugin integration
  'builder_compat.c', # Re-enabled with globals.h created
  # Add missing core files
  'threads.c',
  'personalities.c',
  'listmgmt.c',
  'watches.c',       # Add for rtv_watch_destroy
  'rtv_processor.c', # Add for rtv processing functions
  'mem_mgmt.c',      # Add for _get_sized_data function
  'keyparser.c',     # Add for parse_keys function
  'stubs.c',         # Add stub functions for missing functionality
  # 'tabloader.c',     # Temporarily disabled - requires glade library
  # '3d_vetable.c',    # Temporarily disabled - fixing GTK4 compatibility issues
  # 'logviewer_gui.c', # Temporarily disabled - requires extensive GTK4 porting (GDK drawing, libglade)
  'comms_gui.c',     # Re-enabling - will fix GTK4 compatibility
  # 'datalogging_gui.c', # Temporarily disabled - has syntax error that needs fixing
]

# Dependencies for main executable
main_deps = [
  glib_dep,
  gio_dep,
  gobject_dep,
  gmodule_dep,
  gtk4_dep,
  libxml2_dep,
  gl_dep,
  glu_dep,
  m_dep,
  thread_dep,
  json_glib_dep,
  gnutls_dep,
  epoxy_dep,
  mtxcommon_dep,
  mtxmatheval_dep,
  widgets_dep,
]

# Compile arguments
main_c_args = c_args + [
  '-DGUI_DATA_DIR="@0@"'.format(get_option('prefix') / get_option('datadir') / 'megatunix-redux' / 'glade'),
  '-DGAUGES_DATA_DIR="@0@"'.format(get_option('prefix') / get_option('datadir') / 'megatunix-redux' / 'gauges'),
  '-DDASHBOARDS_DATA_DIR="@0@"'.format(get_option('prefix') / get_option('datadir') / 'megatunix-redux' / 'dashboards'),
]

# Include directories
main_inc = global_inc

# Build main executable
megatunix_redux_exe = executable('megatunix-redux',
  src_files,
  dependencies : main_deps,
  include_directories : main_inc,
  c_args : main_c_args,
  install : true,
  install_dir : get_option('bindir')
)
