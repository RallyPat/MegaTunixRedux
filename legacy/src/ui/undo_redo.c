/*
 * Undo/Redo - Simple ring buffers for foundation
 */

#include "../../include/ui/undo_redo.h"
#include <string.h>

#define UNDO_STACK_CAPACITY 4096

typedef struct {
	CellEdit edits[UNDO_STACK_CAPACITY];
	int top;
} EditStack;

static EditStack g_undo = {0};
static EditStack g_redo = {0};

bool undo_redo_init(void) {
	memset(&g_undo, 0, sizeof(g_undo));
	memset(&g_redo, 0, sizeof(g_redo));
	return true;
}

void undo_redo_shutdown(void) {
	memset(&g_undo, 0, sizeof(g_undo));
	memset(&g_redo, 0, sizeof(g_redo));
}

static void stack_push(EditStack* s, const CellEdit* edit) {
	if (s->top < UNDO_STACK_CAPACITY) {
		s->edits[s->top++] = *edit;
	} else {
		// shift left to make room (simple implementation)
		memmove(&s->edits[0], &s->edits[1], sizeof(CellEdit) * (UNDO_STACK_CAPACITY - 1));
		s->edits[UNDO_STACK_CAPACITY - 1] = *edit;
	}
}

static bool stack_pop(EditStack* s, CellEdit* out) {
	if (s->top <= 0) return false;
	*out = s->edits[--s->top];
	return true;
}

void undo_redo_push(const CellEdit* edit) {
	if (!edit) return;
	stack_push(&g_undo, edit);
	// invalidate redo stack on new edit
	g_redo.top = 0;
}

bool undo_redo_can_undo(void) { return g_undo.top > 0; }
bool undo_redo_can_redo(void) { return g_redo.top > 0; }

bool undo_redo_undo(CellEdit* out_edit) {
	CellEdit e;
	if (!stack_pop(&g_undo, &e)) return false;
	stack_push(&g_redo, &e);
	if (out_edit) *out_edit = e;
	return true;
}

bool undo_redo_redo(CellEdit* out_edit) {
	CellEdit e;
	if (!stack_pop(&g_redo, &e)) return false;
	stack_push(&g_undo, &e);
	if (out_edit) *out_edit = e;
	return true;
}


