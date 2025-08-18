/*
 * GPS Provider - Stub implementation
 */

#include "../../include/integrations/gps_provider.h"
#include <string.h>

static bool g_has_fix = false;
static GpsSample g_last = {0};

bool gps_provider_init(void) {
	g_has_fix = false;
	memset(&g_last, 0, sizeof(g_last));
	return true;
}

void gps_provider_shutdown(void) {
	g_has_fix = false;
}

bool gps_provider_has_fix(void) {
	return g_has_fix;
}

bool gps_provider_get_sample(GpsSample* out_sample) {
	if (!out_sample) return false;
	*out_sample = g_last;
	return g_has_fix;
}


