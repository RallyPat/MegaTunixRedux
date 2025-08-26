/*
 * MegaTunix Redux - VE Table Editor Module
 * 
 * Copyright (C) 2025 Patrick Burke
 * Based on original MegaTunix by David J. Andruczyk
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef VE_TABLE_EDITOR_H
#define VE_TABLE_EDITOR_H

#include <stdbool.h>

// VE Table Editor Functions - Core rendering functions (renamed to avoid conflicts)
void ve_table_editor_render_tab(void);
void ve_table_editor_render_2d_view(void);
void ve_table_editor_render_3d_view(void);
void ve_table_editor_render_editor_view(void);

// VE Table Operations - Basic operations (renamed to avoid conflicts)
void ve_table_editor_create_backup(void);
void ve_table_editor_restore_backup(void);
void ve_table_editor_export_to_file(const char* filename);
void ve_table_editor_import_from_file(const char* filename);

// VE Table State Management - Simple state queries (renamed to avoid conflicts)
bool ve_table_editor_is_available(void);
bool ve_table_editor_is_initialized(void);

// VE Table Initialization and Cleanup
bool init_ve_table_editor(void);
void cleanup_ve_table_editor(void);

#endif // VE_TABLE_EDITOR_H
