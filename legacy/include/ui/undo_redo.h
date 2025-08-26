/*
 * Undo/Redo Foundation for table edits
 */

#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int x;
	int y;
	float old_value;
	float new_value;
} CellEdit;

bool undo_redo_init(void);
void undo_redo_shutdown(void);

void undo_redo_push(const CellEdit* edit);
bool undo_redo_can_undo(void);
bool undo_redo_can_redo(void);
bool undo_redo_undo(CellEdit* out_edit);
bool undo_redo_redo(CellEdit* out_edit);

#ifdef __cplusplus
}
#endif

#endif // UNDO_REDO_H


