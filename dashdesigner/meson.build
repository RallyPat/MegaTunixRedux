# Dashboard Designer application
if get_option('dashdesigner')
  dashdesigner_sources = files(
    'main.c',
    'events.c',
    'loadsave.c',
    'menu_handlers.c',
    'rtv_parser.c',
    'xml.c',
  )

  dashdesigner_deps = [
    glib_dep,
    gio_dep,
    gobject_dep,
    gtk4_dep,
    libxml2_dep,
    widgets_dep,
    mtxcommon_dep,
  ]

  executable('dashdesigner',
    dashdesigner_sources,
    dependencies: dashdesigner_deps,
    include_directories: [
      include_directories('../include'),
      include_directories('../widgets'),
    ],
    install: true,
    install_dir: get_option('bindir'),
  )
endif
