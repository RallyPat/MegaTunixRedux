# Widget library (temporarily excluding GTK2/3 legacy widgets)
widget_sources = files(
  # 'combo_mask.c',     # Needs GTK4 porting
  # 'curve.c',          # Needs GTK4 porting
  # 'curve-private.c',  # Needs GTK4 porting
  # 'gauge.c',          # Needs GTK4 porting
  # 'gauge-private.c',  # Needs GTK4 porting
  # 'gauge-xml.c',      # Needs GTK4 porting
  # 'piegauge.c',       # Needs GTK4 porting
  # 'piegauge-private.c', # Needs GTK4 porting
  # 'progress.c',       # Needs GTK4 porting
  # 'progress-private.c', # Needs GTK4 porting
  # 'stripchart.c',     # Needs GTK4 porting
  # 'stripchart-private.c', # Needs GTK4 porting
)

widgets_lib = static_library(
  'widgets',
  widget_sources,
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
  install: false,
)

widgets_dep = declare_dependency(
  link_with: widgets_lib,
  include_directories: [
    include_directories('.'),
  ],
)

# Demo programs (optional)
if get_option('build_demos')
  demo_deps = [
    glib_dep,
    gtk4_dep,
    widgets_dep,
  ]

  executable('curve-demo',
    'curve-demo.c',
    dependencies: demo_deps,
    install: false,
  )

  executable('gauge-demo',
    'gauge-demo.c',
    dependencies: demo_deps,
    install: false,
  )

  executable('pie-demo',
    'pie-demo.c',
    dependencies: demo_deps,
    install: false,
  )

  executable('progress-demo',
    'progress-demo.c',
    dependencies: demo_deps,
    install: false,
  )

  executable('stripchart-demo',
    'stripchart-demo.c',
    dependencies: demo_deps,
    install: false,
  )
endif
