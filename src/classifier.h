#ifndef CLASSIFIER_H
#define CLASSIFIER_H

#include <pebble_worker.h>

typedef struct {
	double meanH;
	double meanV;
	double deviationH;
	double deviationV;
} Feature;

uint32_t classify(Feature feature);

#endif
