/*
 * Action Triggers - Foundation for conditional automation
 */

#ifndef ACTION_TRIGGERS_H
#define ACTION_TRIGGERS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TRIGGER_OP_GREATER,
	TRIGGER_OP_GREATER_EQUAL,
	TRIGGER_OP_LESS,
	TRIGGER_OP_LESS_EQUAL,
	TRIGGER_OP_EQUAL,
	TRIGGER_OP_NOT_EQUAL
} TriggerComparison;

typedef struct {
	char signal_name[64];
	TriggerComparison comparison;
	double threshold_value;
	char action_name[64];
} ActionTrigger;

bool action_triggers_init(void);
void action_triggers_shutdown(void);

bool action_triggers_add(const ActionTrigger* trigger);
void action_triggers_clear(void);
void action_triggers_process_sample(const char* signal, double value);

#ifdef __cplusplus
}
#endif

#endif // ACTION_TRIGGERS_H


