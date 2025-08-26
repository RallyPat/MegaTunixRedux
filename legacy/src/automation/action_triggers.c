/*
 * Action Triggers - Stub implementation
 */

#include "../../include/automation/action_triggers.h"
#include <string.h>

typedef struct {
	ActionTrigger items[64];
	unsigned int count;
} TriggerStore;

static TriggerStore g_store = {0};

bool action_triggers_init(void) {
	memset(&g_store, 0, sizeof(g_store));
	return true;
}

void action_triggers_shutdown(void) {
	memset(&g_store, 0, sizeof(g_store));
}

bool action_triggers_add(const ActionTrigger* trigger) {
	if (!trigger) return false;
	if (g_store.count >= (sizeof(g_store.items)/sizeof(g_store.items[0]))) return false;
	g_store.items[g_store.count++] = *trigger;
	return true;
}

void action_triggers_clear(void) {
	g_store.count = 0;
}

static bool evaluate(TriggerComparison cmp, double a, double b) {
	switch (cmp) {
		case TRIGGER_OP_GREATER: return a > b;
		case TRIGGER_OP_GREATER_EQUAL: return a >= b;
		case TRIGGER_OP_LESS: return a < b;
		case TRIGGER_OP_LESS_EQUAL: return a <= b;
		case TRIGGER_OP_EQUAL: return a == b;
		case TRIGGER_OP_NOT_EQUAL: return a != b;
	}
	return false;
}

void action_triggers_process_sample(const char* signal, double value) {
	for (unsigned int i = 0; i < g_store.count; ++i) {
		ActionTrigger* t = &g_store.items[i];
		if (strcmp(t->signal_name, signal) == 0) {
			if (evaluate(t->comparison, value, t->threshold_value)) {
				// Placeholder: invoke named action via future dispatcher/hook system
				// e.g., dispatch_action(t->action_name);
			}
		}
	}
}


