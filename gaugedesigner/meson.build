# Gauge Designer application
if get_option('gaugedesigner')
  gaugedesigner_sources = files(
    'main.c',
    'alerts.c',
    'events.c',
    'gd_init.c',
    'handlers.c',
    'loadsave.c',
    'menu_handlers.c',
    'polygons.c',
    'tblocks.c',
    'tgroups.c',
    'warnings.c',
  )

  gaugedesigner_deps = [
    glib_dep,
    gio_dep,
    gobject_dep,
    gtk4_dep,
    libxml2_dep,
    widgets_dep,
    mtxcommon_dep,
  ]

  executable('gaugedesigner',
    gaugedesigner_sources,
    dependencies: gaugedesigner_deps,
    include_directories: [
      include_directories('../include'),
      include_directories('../widgets'),
      include_directories('../'),  # For config.h
    ],
    install: true,
    install_dir: get_option('bindir'),
  )
endif
