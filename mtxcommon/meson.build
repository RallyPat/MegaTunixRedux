# Common utility library
mtxcommon_sources = files(
  'configfile.c',
  'getfiles.c',
  'xmlbase.c',
)

mtxcommon_lib = static_library(
  'mtxcommon',
  mtxcommon_sources,
  dependencies: [
    glib_dep,
    gio_dep,
    gobject_dep,
    gtk4_dep,
    libxml2_dep,
  ],
  include_directories: [
    global_inc,
    include_directories('.'),
  ],
  c_args: ['-DHAVE_CONFIG_H'],
  install: false,
)

mtxcommon_dep = declare_dependency(
  link_with: mtxcommon_lib,
  include_directories: [
    include_directories('.'),
  ],
)
