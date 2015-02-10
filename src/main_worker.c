#include <pebble_worker.h>
#include "recognizer.h"
#include "PDUtils.h"
  
#define KEY_MESSAGE_TYPE_SLEEPING 0
#define KEY_MESSAGE_TYPE_SITTING 1
#define KEY_MESSAGE_TYPE_WALKING 2
#define KEY_MESSAGE_TYPE_STEPS 3
#define KEY_MESSAGE_TYPE_JOGGING 4
#define KEY_NOTIFY_GET_MOVING 5
#define KEY_APP_MESSAGE_TYPE_ISSLEEPING 6
#define KEY_APP_MESSAGE_TYPE_PSENSITIVITY 7
#define KEY_COUNTER_TIMESTAMP 8
#define KEY_REQUEST_UPDATE  9
#define KEY_APP_MESSAGE_TYPE_RESET 10
#define KEY_APP_MESSAGE_TYPE_PEDOMETER_SENSITIVITY 11
#define KEY_APP_MESSAGE_TYPE_INACTIVITY_MINUTES 12
#define KEY_APP_MESSAGE_TYPE_ENABLE_LOGGING 13
#define KEY_MESSAGE_LOG 14
  
static LowPassFilter mFilter;
static uint32_t mCurrentType = 1;
//static bool mNotifyInactivity = false;

// Config
static bool mIsDriving = false;
static bool mIsSleeping = false;
static int32_t mPedometerSensitivity = 45;	// Range: [0, 100]
static int32_t mNotifyMinutesInactivity = 60;
static uint32_t mStepCount = 0;
static time_t mLastActiveTime;
static uint32_t mActiveSteps = 5;
static bool mEnableLogging = false;
static Counter mCounter;
static Counter mLastCounter;
DataLoggingSessionRef mDataLog;



static void saveLogValues() {
  mLastCounter.sleepTime = mCounter.sleepTime;
  mLastCounter.sitTime = mCounter.sitTime;
  mLastCounter.walkTime = mCounter.walkTime;
  mLastCounter.jogTime = mCounter.jogTime;
  mLastCounter.totalSteps = mCounter.totalSteps; 
}

static void logData() {
  // For each log (a sample cycle), copy values to the mDataLog
  uint32_t sleepTime = mCounter.sleepTime - mLastCounter.sleepTime;
  uint32_t sitTime = mCounter.sitTime - mLastCounter.sitTime;
  uint32_t walkTime = mCounter.walkTime - mLastCounter.walkTime;
  uint32_t jogTime = mCounter.jogTime - mLastCounter.jogTime;
  uint32_t steps = mCounter.totalSteps - mLastCounter.totalSteps;
  uint32_t timestamp = mCounter.timestamp;
  
  // Save the current values to mLastCounter
  //saveLogValues(sleepTime, sitTime, walkTime, jogTime, steps);
  
  // DataLogging: send to the companion app
  int len = sizeof(uint32_t);
  uint8_t bytes[len * 6];
  memcpy(bytes + len * 0, &sleepTime, len);
  memcpy(bytes + len * 1, &sitTime, len);
  memcpy(bytes + len * 2, &walkTime, len);
  memcpy(bytes + len * 3, &jogTime, len);
  memcpy(bytes + len * 4, &steps, len);
  memcpy(bytes + len * 5, &timestamp, len);
  
  if(!data_logging_log(mDataLog, &bytes, 1)) {
     APP_LOG(APP_LOG_LEVEL_ERROR, "Failed to log data to companion app");
  }
  //APP_LOG(APP_LOG_LEVEL_INFO, "sitTIme: %d walkTime %d Steps: %d ", (int)sitTime, (int)walkTime, (int)steps);
  //saveLogValues(sleepTime, sitTime, walkTime, jogTime, steps);
}

static void CheckTimeToReset() {
	// Get a tm structure
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
  
  if(tick_time->tm_sec == 59) {
     if(mEnableLogging) {
      logData();
      //send_app_message(KEY_MESSAGE_LOG);
      saveLogValues();
    }
  }
  // Reset health values
  if(tick_time->tm_hour == 0 && tick_time->tm_min == 0 && tick_time->tm_sec == 0) {
    mCounter.totalSteps = 0;
    
    if(!mIsSleeping) {
      mCounter.sleepTime = 0;
    }
      
    mCounter.sitTime = 0;
	  mCounter.walkTime = 0;
	  mCounter.jogTime = 0;
    saveLogValues();
  }
  
  //Reset sleep time at noon time
  if(tick_time->tm_hour == 12 && tick_time->tm_min == 0) { 
     if(!mIsSleeping) {
       mCounter.sleepTime = 0;
    }
  }
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	CheckTimeToReset();
}

static void send_app_message(uint32_t type) {
  AppWorkerMessage msg_data;
    
		switch(type) {
			case KEY_MESSAGE_TYPE_SLEEPING:
        msg_data.data0 = mCounter.sleepTime;
        app_worker_send_message(KEY_MESSAGE_TYPE_SLEEPING, &msg_data);
				break;
			case KEY_MESSAGE_TYPE_SITTING:
        msg_data.data0 = mCounter.sitTime;
        app_worker_send_message(KEY_MESSAGE_TYPE_SITTING, &msg_data);
       	break;
      case KEY_MESSAGE_TYPE_WALKING:
			case KEY_MESSAGE_TYPE_STEPS:
        msg_data.data0 = mCounter.walkTime;
        msg_data.data1 = mCounter.totalSteps;
        msg_data.data2 = mCounter.currentSteps;
        app_worker_send_message(KEY_MESSAGE_TYPE_STEPS, &msg_data);
				break;
			case KEY_MESSAGE_TYPE_JOGGING:
				msg_data.data0 = mCounter.jogTime;
        msg_data.data1 = mCounter.totalSteps;
      
        app_worker_send_message(KEY_MESSAGE_TYPE_JOGGING, &msg_data);
				break;
      case KEY_NOTIFY_GET_MOVING:
        msg_data.data0 = true;
        app_worker_send_message(KEY_NOTIFY_GET_MOVING, &msg_data);
        //mNotifyInactivity = false;
        break;
      case KEY_MESSAGE_LOG:
        msg_data.data0 = mCounter.totalSteps - mLastCounter.totalSteps;
        msg_data.data1 = mCounter.sitTime - mLastCounter.sitTime;
        app_worker_send_message(KEY_MESSAGE_LOG, &msg_data);
        break;
		}
}

static void resetLastActiveTime() {
  time_t t = time(NULL);
  struct tm *now;
  
  now = localtime(&t);
  time_t currentTime = p_mktime(now);
  
  mLastActiveTime = currentTime;
}
static void message_handler(uint16_t type, AppWorkerMessage *data) {
  
  switch(type) {
    case KEY_APP_MESSAGE_TYPE_ISSLEEPING:
      mIsSleeping = data->data0;
      if(!mIsSleeping) {
        resetLastActiveTime();
      }
      break;
    case KEY_REQUEST_UPDATE:
      for(int i = 0; i < KEY_MESSAGE_TYPE_JOGGING + 1; i++){
        send_app_message(i);
      }
      break;
    case KEY_APP_MESSAGE_TYPE_RESET:
        mCounter.sleepTime = 0;
        mCounter.sitTime = 0;
	      mCounter.walkTime = 0;
	      mCounter.jogTime = 0;
        mCounter.totalSteps = 0;
      break;
    case KEY_APP_MESSAGE_TYPE_PEDOMETER_SENSITIVITY:
      mPedometerSensitivity = data->data0;
      break;
    case KEY_APP_MESSAGE_TYPE_INACTIVITY_MINUTES:
      mNotifyMinutesInactivity = data->data0;
      break;
    case KEY_APP_MESSAGE_TYPE_ENABLE_LOGGING:
      mEnableLogging = data->data0;
    default:
      return;
  }
}

static void processActivity(uint32_t type) {
  time_t t = time(NULL);
  struct tm *now;
  double timeDiff;
  
  now = localtime(&t);
  time_t currentTime = p_mktime(now);
   
  if(!mIsSleeping) {
    if(type == KEY_MESSAGE_TYPE_STEPS) {
      mStepCount += mCounter.currentSteps;
      // let's make sure we have activity and not
      // just a false positive when setting the last active time
      if(mStepCount > mActiveSteps) {
          mLastActiveTime = currentTime;
      }
      return;
    }
    
    timeDiff = (difftime(currentTime, mLastActiveTime) / 60);
    // Reset the accumulated step count to zero if there is no movement
    // allow for 2 minutes to pass before step count reset
    if(type == KEY_MESSAGE_TYPE_SITTING && timeDiff > 2) {
      mStepCount = 0;
      // if there's no activity for the configured amount of time and we're not sleep
      // warn the user
      if(timeDiff >= mNotifyMinutesInactivity) {
        worker_launch_app();
        send_app_message(KEY_NOTIFY_GET_MOVING);
        mLastActiveTime = currentTime;
      }
    }
  }
}

static void processAccelerometerData(AccelData* acceleration, uint32_t size) {
 // Throw the variables into below function, it will update it for you.
	// Return: 0 means no error; 1 means no acceleration sample (Pebble acceleration API was not very reliable); 2 means # of samples is not enough and it will cache and wait for more data.
	if (analyzeAcceleration(&mCurrentType, &mCounter, &mFilter, mIsDriving, mPedometerSensitivity, acceleration, size, mIsSleeping) != 0) {
		send_app_message(mCurrentType);
    processActivity(mCurrentType);
    //if(mEnableLogging) {
    //  logData();
      //send_app_message(KEY_MESSAGE_LOG);
    //  saveLogValues();
    }
  }
}

static void worker_init() {
  // Load Counter persistent values
	mCounter.sleepTime = persist_exists(KEY_MESSAGE_TYPE_SLEEPING) ? persist_read_int(KEY_MESSAGE_TYPE_SLEEPING) : 0;
	mCounter.sitTime = persist_exists(KEY_MESSAGE_TYPE_SITTING) ? persist_read_int(KEY_MESSAGE_TYPE_SITTING) : 0;
	mCounter.walkTime = persist_exists(KEY_MESSAGE_TYPE_WALKING) ? persist_read_int(KEY_MESSAGE_TYPE_WALKING) : 0;
	mCounter.jogTime = persist_exists(KEY_MESSAGE_TYPE_JOGGING) ? persist_read_int(KEY_MESSAGE_TYPE_JOGGING) : 0;
	
  if (persist_exists(KEY_COUNTER_TIMESTAMP)) {
		mCounter.timestamp = persist_read_int(KEY_COUNTER_TIMESTAMP);
	} else {
		mCounter.timestamp = (uint32_t) time(NULL);
	}
  
	mCounter.totalSteps = persist_exists(KEY_MESSAGE_TYPE_STEPS) ? persist_read_int(KEY_MESSAGE_TYPE_STEPS) : 0;
  
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  
  mLastActiveTime  = p_mktime(now);
  
  // Setup accelerometer API
	accel_data_service_subscribe(BATCH_SIZE, &processAccelerometerData);
	accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  
  // Initiate low pass filter for later use
	initLowPassFilter(&mFilter);
  
  // Register with TickTimerService
	tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Intialize mLastCounter members
  //saveLogValues();
  
  app_worker_message_subscribe(message_handler);
  // Initialize a data log
  mDataLog = data_logging_create(0, DATA_LOGGING_BYTE_ARRAY, sizeof(uint32_t) * 6, false);
}

static void worker_deinit() {
  // Save Counter persistent values
	persist_write_int(KEY_MESSAGE_TYPE_SLEEPING, mCounter.sleepTime);
	persist_write_int(KEY_MESSAGE_TYPE_SITTING, mCounter.sitTime);
	persist_write_int(KEY_MESSAGE_TYPE_WALKING, mCounter.walkTime);
	persist_write_int(KEY_MESSAGE_TYPE_JOGGING, mCounter.jogTime);
	persist_write_int(KEY_COUNTER_TIMESTAMP, mCounter.timestamp);
	persist_write_int(KEY_MESSAGE_TYPE_STEPS, mCounter.totalSteps);
  
  // Deinitialize your worker here
  accel_tap_service_unsubscribe();
	accel_data_service_unsubscribe();
  app_worker_message_unsubscribe();
  tick_timer_service_unsubscribe();
  app_worker_message_unsubscribe();
  // Close data logging
  data_logging_finish(mDataLog);
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}