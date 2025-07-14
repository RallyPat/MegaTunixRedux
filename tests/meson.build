# Tests meson.build for MegaTunix Redux

# Test framework sources
test_framework_sources = [
  '../src/test_framework.c',
  '../src/security_utils.c', 
  '../src/modern_widgets.c',
  '../src/plugin_system.c',
  '../src/network_security.c'
]

# Test dependencies (same as main executable plus test-specific)
test_deps = [
  glib_dep,
  gio_dep,
  gobject_dep,
  gtk4_dep,
  libxml2_dep,
  json_glib_dep,
  gnutls_dep,
  epoxy_dep,
  gl_dep,
  glu_dep,
  m_dep,
  thread_dep,
  mtxcommon_dep,
  mtxmatheval_dep,
  widgets_dep,
]

# Include directories
test_inc = include_directories('../include', '../src', '../mtxcommon', '../mtxmatheval', '../widgets')

# Test compilation arguments
test_c_args = [
  '-DTESTING',
  '-DDEBUG',
]

# Test executable
test_exe = executable(
  'mtx-tests',
  [
    'test_main.c',
    test_framework_sources
  ],
  dependencies: test_deps,
  include_directories: test_inc,
  c_args: test_c_args,
  install: false
)

# Register test
test(
  'mtx-unit-tests',
  test_exe,
  timeout: 300,
  workdir: meson.current_build_dir()
)

# Individual test suites (can be run separately)
test(
  'mtx-security-tests',
  test_exe,
  args: ['--suite=security'],
  timeout: 60,
  workdir: meson.current_build_dir()
)

test(
  'mtx-ui-tests',
  test_exe,
  args: ['--suite=ui'],
  timeout: 120,
  workdir: meson.current_build_dir()
)

test(
  'mtx-plugin-tests',
  test_exe,
  args: ['--suite=plugin'],
  timeout: 60,
  workdir: meson.current_build_dir()
)

test(
  'mtx-network-tests',
  test_exe,
  args: ['--suite=network'],
  timeout: 60,
  workdir: meson.current_build_dir()
)

test(
  'mtx-performance-tests',
  test_exe,
  args: ['--suite=performance'],
  timeout: 120,
  workdir: meson.current_build_dir()
)

# Memory leak testing (if valgrind is available)
valgrind = find_program('valgrind', required: false)
if valgrind.found() and get_option('enable_valgrind')
  test(
    'mtx-memory-leak-test',
    valgrind,
    args: [
      '--tool=memcheck',
      '--leak-check=full',
      '--show-leak-kinds=all',
      '--track-origins=yes',
      '--error-exitcode=1',
      test_exe.full_path()
    ],
    timeout: 600,
    workdir: meson.current_build_dir(),
    depends: test_exe
  )
endif

# Code coverage (if lcov is available)
lcov = find_program('lcov', required: false)
genhtml = find_program('genhtml', required: false)
if lcov.found() and genhtml.found() and get_option('enable_coverage')
  # Coverage data collection
  coverage_dir = meson.current_build_dir() / 'coverage'
  
  run_target(
    'coverage',
    command: [
      'sh', '-c',
      '''
      mkdir -p @0@ &&
      lcov --capture --directory . --output-file @0@/coverage.info &&
      genhtml @0@/coverage.info --output-directory @0@/html
      '''.format(coverage_dir)
    ],
    depends: test_exe
  )
endif

# Benchmarking executable (separate from unit tests)
if get_option('enable_benchmarks')
  benchmark_sources = [
    'benchmark_main.c',
    test_framework_sources
  ]

  benchmark_exe = executable(
    'mtx-benchmark',
    benchmark_sources,
    dependencies: test_deps,
    include_directories: test_inc,
    c_args: test_c_args,
    install: false
  )

  # Register benchmark
  benchmark(
    'mtx-performance-benchmark',
    benchmark_exe,
    timeout: 600,
    workdir: meson.current_build_dir()
  )
endif
