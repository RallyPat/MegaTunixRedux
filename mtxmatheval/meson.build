# Math evaluation library
matheval_sources = files(
  'error.c',
  'matheval.c',
  'node.c',
  'symbol_table.c',
  'xmalloc.c',
  'xmath.c',
  'yywrap.c',
)

# Generate parser and scanner
bison_gen = generator(bison,
  output: ['@BASENAME@.c', '@BASENAME@.h'],
  arguments: ['@INPUT@', '--defines=@OUTPUT1@', '--output=@OUTPUT0@']
)

flex_gen = generator(flex,
  output: '@BASENAME@.c',
  arguments: ['-o', '@OUTPUT@', '@INPUT@']
)

parser_sources = bison_gen.process('parser.y')
scanner_sources = flex_gen.process('scanner.l')

mtxmatheval_lib = static_library(
  'mtxmatheval',
  matheval_sources + parser_sources + scanner_sources,
  dependencies: [
    glib_dep,
    gtk4_dep,
    m_dep,
  ],
  include_directories: [
    global_inc,
    include_directories('.'),
  ],
  c_args: ['-DHAVE_CONFIG_H'],
  install: false,
)

mtxmatheval_dep = declare_dependency(
  link_with: mtxmatheval_lib,
  include_directories: [
    include_directories('.'),
  ],
)
