/*
 * Config Compatibility for MegaTunix Redux
 * 
 * Defines constants that may be missing from config.h
 * or need compatibility definitions.
 */

#ifndef CONFIG_COMPAT_H
#define CONFIG_COMPAT_H

/* Include the generated config.h if it exists */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Fallback definitions for missing constants */
#ifndef DATA_DIR
#define DATA_DIR "/usr/share/megatunix-redux"
#endif

#ifndef PACKAGE_DATA_DIR
#define PACKAGE_DATA_DIR DATA_DIR
#endif

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#ifndef VERSION
#define VERSION "1.0.0"
#endif

#ifndef PACKAGE
#define PACKAGE "megatunix-redux"
#endif

/* Math library availability */
#ifndef HAVE_MATH_H
#define HAVE_MATH_H 1
#endif

/* Standard library includes for math functions */
#include <math.h>
#include <stdlib.h>

#endif /* CONFIG_COMPAT_H */
