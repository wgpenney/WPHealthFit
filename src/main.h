#include <pebble.h>
  
typedef struct {
	char sleepTime[64];
	char sitTime[64];
	char walkTime[64];
	char jogTime[64];
	char steps[64];
} DisplayCounter;

typedef struct {
  uint32_t pedometerSensitivity;
  uint32_t inactivityMinutes;
  uint8_t enableReset;
  bool isSleeping;
  bool enableDataLogging;
} Persist_Settings;
