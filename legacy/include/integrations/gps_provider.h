/*
 * GPS Provider Interface - Foundation for GPS integration
 */

#ifndef GPS_PROVIDER_H
#define GPS_PROVIDER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	double latitude;
	double longitude;
	double altitude_m;
	double speed_mps;
	double heading_deg;
} GpsSample;

bool gps_provider_init(void);
void gps_provider_shutdown(void);

bool gps_provider_has_fix(void);
bool gps_provider_get_sample(GpsSample* out_sample);

#ifdef __cplusplus
}
#endif

#endif // GPS_PROVIDER_H


