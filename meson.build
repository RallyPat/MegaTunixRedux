project('megatunix-redux', 'c', 'cpp',
  version : '1.0.0',
  license : 'GPL-2.0',
  meson_version : '>= 0.60.0',
  default_options : [
    'c_std=c17',
    'cpp_std=c++17',
    'warning_level=2',
    'werror=false',
    'b_ndebug=if-release'
  ]
)

# Version information
version_split = meson.project_version().split('.')
version_major = version_split[0].to_int()
version_minor = version_split[1].to_int()
version_micro = version_split[2].to_int()

# Configuration
conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('MTX_VER_STRING', meson.project_version())
conf_data.set('MTX_MAJOR_VERSION', version_major)
conf_data.set('MTX_MINOR_VERSION', version_minor)
conf_data.set('MTX_MICRO_VERSION', version_micro)
# Add short versions for legacy compatibility
conf_data.set('MTX_MAJOR', version_major)
conf_data.set('MTX_MINOR', version_minor)
conf_data.set('MTX_MICRO', version_micro)
# Add missing directory constants
conf_data.set_quoted('DATA_DIR', '/tmp/megatunix-test')
conf_data.set_quoted('DASHES_DATA_DIR', 'dashboards')
conf_data.set_quoted('INTERROGATOR_DATA_DIR', 'interrogator')
conf_data.set_quoted('LOOKUPTABLES_DATA_DIR', 'lookuptables') 
conf_data.set_quoted('REALTIME_MAPS_DATA_DIR', 'realtime_maps')
conf_data.set_quoted('RTSLIDERS_DATA_DIR', 'rtsliders')
conf_data.set_quoted('RTSTATUS_DATA_DIR', 'rtstatus')
conf_data.set_quoted('TABLE_DATA_DIR', 'tables')
conf_data.set_quoted('GUI_DATA_DIR', 'glade')
conf_data.set_quoted('MTXPLUGINDIR', get_option('prefix') / get_option('libdir') / 'megatunix' / 'plugins')
conf_data.set_quoted('PACKAGE', 'megatunix-redux')
conf_data.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))

# Git hash (if available)
git = find_program('git', required : false)
if git.found()
  git_hash = run_command(git, 'rev-parse', '--short', 'HEAD', check : false)
  if git_hash.returncode() == 0
    conf_data.set_quoted('GIT_HASH', git_hash.stdout().strip())
  else
    conf_data.set_quoted('GIT_HASH', 'unknown')
  endif
else
  conf_data.set_quoted('GIT_HASH', 'unknown')
endif

# Platform detection
host_system = host_machine.system()
if host_system == 'windows'
  conf_data.set('_WIN32_', 1)
elif host_system == 'darwin'
  conf_data.set('__APPLE__', 1)
endif

# Dependencies
glib_req = '>= 2.70.0'
gtk4_req = '>= 4.10.0'

glib_dep = dependency('glib-2.0', version : glib_req)
gio_dep = dependency('gio-2.0', version : glib_req)
gobject_dep = dependency('gobject-2.0', version : glib_req)
gmodule_dep = dependency('gmodule-2.0', version : glib_req)
gtk4_dep = dependency('gtk4', version : gtk4_req)
libxml2_dep = dependency('libxml-2.0')

# OpenGL dependencies
gl_dep = dependency('gl', required : true)
glu_dep = dependency('glu', required : true)

# Math library
m_dep = declare_dependency()
cc = meson.get_compiler('c')
if not cc.has_function('sin')
  m_dep = cc.find_library('m', required : true)
endif

# Threading
thread_dep = dependency('threads')

# Global include directories
global_inc = include_directories('.', 'include')
build_inc = include_directories('.')

# Optional dependencies
bison = find_program('bison', required : true)
flex = find_program('flex', required : true)

# Internationalization
i18n = import('i18n')
gettext_package = 'megatunix-redux'
conf_data.set_quoted('GETTEXT_PACKAGE', gettext_package)

# Build configuration
debug = get_option('debug')
optimization = get_option('optimization')

c_args = []
if debug
  c_args += ['-DDEBUG']
endif

# Deprecated API handling
if get_option('enable_deprecated')
  c_args += ['-DENABLE_DEPRECATED']
else
  c_args += [
    '-DG_DISABLE_DEPRECATED',
    '-DGDK_DISABLE_DEPRECATED',
    '-DGTK_DISABLE_DEPRECATED'
  ]
endif

# Network security features
epoxy_dep = dependency('epoxy', version: '>= 1.5', required: true)

# Base dependencies list
deps = [
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
  epoxy_dep
]

# Network security features
if get_option('enable_network_security')
  # Find network security dependencies
  gnutls_dep = dependency('gnutls', version : '>=3.6.0', required : true)
  json_glib_dep = dependency('json-glib-1.0', version : '>=1.4.0', required : true)
  
  # Add to dependencies
  deps += [gnutls_dep, json_glib_dep]
  
  # Add compile definitions
  c_args += ['-DENABLE_NETWORK_SECURITY']
  conf_data.set('ENABLE_NETWORK_SECURITY', 1)
  
  message('Network security features enabled')
else
  message('Network security features disabled')
endif

# Subdirectories
subdir('include')
subdir('mtxcommon')
subdir('mtxmatheval')
subdir('widgets')
subdir('src')
subdir('gaugedesigner')
subdir('dashdesigner')
subdir('po')

# Tests (if enabled)
if get_option('enable_tests')
  subdir('tests')
endif

# Generate config.h
configure_file(
  output : 'config.h',
  configuration : conf_data
)

# Summary
summary({
  'Version': meson.project_version(),
  'Git Hash': conf_data.get('GIT_HASH'),
  'Debug': debug,
  'Deprecated APIs': get_option('enable_deprecated'),
}, section: 'Configuration')

summary({
  'GTK4': gtk4_dep.version(),
  'GLib': glib_dep.version(),
  'LibXML2': libxml2_dep.version(),
}, section: 'Dependencies')
