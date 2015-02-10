#include <pebble.h>
#include "main.h"
#include "PDUtils.h"


#define KEY_MESSAGE_TYPE_SLEEPING 0
#define KEY_MESSAGE_TYPE_SITTING 1
#define KEY_MESSAGE_TYPE_WALKING 2
#define KEY_MESSAGE_TYPE_STEPS 3
#define KEY_MESSAGE_TYPE_JOGGING 4
#define KEY_NOTIFY_GET_MOVING 5
#define KEY_APP_MESSAGE_TYPE_ISSLEEPING 6
#define KEY_APP_MESSAGE_TYPE_PSENSITIVITY 7
#define KEY_SETTING_ISSLEEPING 8
#define KEY_REQUEST_UPDATE 9
#define KEY_APP_MESSAGE_TYPE_RESET 10
#define KEY_APP_MESSAGE_TYPE_PEDOMETER_SENSITIVITY 11
#define KEY_APP_MESSAGE_TYPE_INACTIVITY_MINUTES 12
#define KEY_APP_MESSAGE_TYPE_ENABLE_LOGGING 13
//#define KEY_MESSAGE_LOG 14
// define the appkeys used for appMessages
#define AK_PEDOMETER_SENSITIVITY 0
#define AK_ENABLE_RESET          1
#define AK_INACTIVITY_MINUTES    2
#define AK_ENABLE_DATA_LOGGING   3
  
#define AK_PERSIST_SETTING_PEDOMETER 0
#define AK_PERSIST_SETTING_ENABLE_RESET 1
#define AK_PERSIST_SETTING_INACTITY_MINUTES 2
#define AK_PERSIST_SETTING_ISSLEEPING 3

static DisplayCounter mDisplayCounter;  
static uint32_t mGetMovingVibe = 3;

static Window *s_main_window;
static Layer *s_main_layer;
static TextLayer *s_walk_text_layer;
static TextLayer *s_steps_text_layer;
static TextLayer *s_sleep_text_layer;
static TextLayer *s_sit_text_layer;
static TextLayer *s_jog_text_layer;
static TextLayer *s_message_layer;
static BitmapLayer *bmp_steps_layer;
static BitmapLayer *bmp_sit_layer;
static BitmapLayer *bmp_walk_layer;
static BitmapLayer *bmp_sleep_layer;
static GBitmap *image_steps_icon;
static GBitmap *image_sit_icon;
static GBitmap *image_walk_icon;
static GBitmap *image_sleep_icon;
static GBitmap *image_inverted_sleep_icon;
static BitmapLayer *bmp_jog_layer;
static GBitmap *image_jog_icon;

Persist_Settings settings = {
  .pedometerSensitivity = 0,
  .inactivityMinutes = 0,
  .enableReset = 0,
  .isSleeping = false,
  .enableDataLogging = false,
};

static void toggle_message() {
  layer_set_hidden((Layer *) s_message_layer, true);
}

void generate_vibe(uint32_t vibe_pattern_number) {
  vibes_cancel();
  switch ( vibe_pattern_number ) {
  case 0: // No Vibration
    return;
  case 1: // Single short
    vibes_short_pulse();
    break;
  case 2: // Double short
    vibes_double_pulse();
    break;
  case 3: // Triple
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {200, 100, 200, 100, 200},
      .num_segments = 5
    } );
  case 4: // Long
    vibes_long_pulse();
    break;
  case 5: // Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {50, 200, 50, 200, 50, 200, 50},
      .num_segments = 7
    } );
    break;
  case 6: // Less Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {100, 200, 100, 200, 100, 200, 100},
      .num_segments = 7
    } );
    break;
  case 7: // Not Subtle
    vibes_enqueue_custom_pattern( (VibePattern) {
      .durations = (uint32_t []) {500, 250, 500, 250, 500, 250, 500},
      .num_segments = 7
    } );
    break;
  default: // No Vibration
    return;
  }
}

static void request_update_from_worker() {
  bool running = app_worker_is_running();
  
  if(running) {
    AppWorkerMessage msg_data = {
    .data0 = true
  };
  
  if (!running) {
    app_worker_launch();
  }
  
  app_worker_send_message(KEY_REQUEST_UPDATE, &msg_data);
  }
}
static void send_worker_message(uint16_t type) {
  AppWorkerMessage msg_data;
  bool running = app_worker_is_running();
  
  if(running) {
    switch (type) {
      case KEY_APP_MESSAGE_TYPE_PEDOMETER_SENSITIVITY:
        msg_data.data0 = settings.pedometerSensitivity;
        app_worker_send_message(KEY_APP_MESSAGE_TYPE_PEDOMETER_SENSITIVITY, &msg_data);
        break;
      case KEY_APP_MESSAGE_TYPE_INACTIVITY_MINUTES:
        msg_data.data0 = settings.inactivityMinutes;
        app_worker_send_message(KEY_APP_MESSAGE_TYPE_INACTIVITY_MINUTES, &msg_data);
        break;
      case KEY_APP_MESSAGE_TYPE_ENABLE_LOGGING:
        msg_data.data0 = settings.enableDataLogging;
        app_worker_send_message(KEY_APP_MESSAGE_TYPE_ENABLE_LOGGING, &msg_data);
      default:
        return;
    }
  }
}
static void update_sleep() {
  
   if(settings.isSleeping) {
    text_layer_set_background_color(s_sleep_text_layer, GColorBlack);
	  text_layer_set_text_color(s_sleep_text_layer, GColorWhite);
    text_layer_set_text(s_sleep_text_layer, mDisplayCounter.sleepTime);
    bitmap_layer_set_bitmap(bmp_sleep_layer, image_sleep_icon);
  } else {
    text_layer_set_background_color(s_sleep_text_layer, GColorWhite);
	  text_layer_set_text_color(s_sleep_text_layer, GColorBlack);
    text_layer_set_text(s_sleep_text_layer, mDisplayCounter.sleepTime);
    bitmap_layer_set_bitmap(bmp_sleep_layer, image_inverted_sleep_icon);
  }
}
static void notify_get_moving() {
  generate_vibe(mGetMovingVibe);
  text_layer_set_text(s_message_layer, "Time to get Moving");
  layer_set_hidden((Layer *) s_message_layer, false);
  app_timer_register(10000, &toggle_message, NULL);
}

static void down_multi_click_handler(ClickRecognizerRef recognizer, void *context) {
  bool running = app_worker_is_running();
  
  AppWorkerMessage msg_data = {
    .data0 = true
  };
  
  if(settings.enableReset && !settings.isSleeping) {
    if (!running) {
      app_worker_launch();
    }
  
    app_worker_send_message(KEY_APP_MESSAGE_TYPE_RESET, &msg_data);
    request_update_from_worker();
  }
}

static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  uint32_t hours;
  uint32_t minutes;
  uint32_t seconds;
  char tmpStr[64];
  
  switch(type){
    case KEY_MESSAGE_TYPE_SLEEPING:
      seconds = data->data0;
      minutes = seconds / 60UL;
      hours = minutes / 60UL;
      minutes %= 60UL;
      seconds %= 60UL;
      
      snprintf(mDisplayCounter.sleepTime, sizeof(mDisplayCounter.sleepTime), "\n%.2d:%.2d", (int)hours, (int)minutes);
      text_layer_set_text(s_sleep_text_layer, mDisplayCounter.sleepTime);     
      break;
    case KEY_MESSAGE_TYPE_SITTING:
      seconds = data->data0;
      minutes = seconds / 60UL;
      hours = minutes / 60UL;
      minutes %= 60UL;
      seconds %= 60UL;
        
      snprintf(mDisplayCounter.sitTime, sizeof(mDisplayCounter.sitTime), "\n%.2d:%.2d", (int)hours, (int)minutes);
      text_layer_set_text(s_sit_text_layer, mDisplayCounter.sitTime);
      break;
    case KEY_MESSAGE_TYPE_WALKING:
    case KEY_MESSAGE_TYPE_STEPS:
      seconds = data->data0;
      minutes = seconds / 60UL;
      hours = minutes / 60UL;
      minutes %= 60UL;
      seconds %= 60UL;
      
      snprintf(mDisplayCounter.walkTime, sizeof(mDisplayCounter.walkTime), "\n%.2d:%.2d", (int)hours, (int)minutes);
      text_layer_set_text(s_walk_text_layer, mDisplayCounter.walkTime);
      snprintf(mDisplayCounter.steps, sizeof(mDisplayCounter.steps), "\n%d", (int)data->data1);
      text_layer_set_text(s_steps_text_layer, mDisplayCounter.steps);
      break;
    case KEY_MESSAGE_TYPE_JOGGING:
      seconds = data->data0;
      minutes = seconds / 60UL;
      hours = minutes / 60UL;
      minutes %= 60UL;
      seconds %= 60UL;
      
      snprintf(mDisplayCounter.jogTime, sizeof(mDisplayCounter.jogTime), "\n%.2d:%.2d", (int)hours, (int)minutes);
      text_layer_set_text(s_jog_text_layer, mDisplayCounter.jogTime);
      snprintf(mDisplayCounter.steps, sizeof(mDisplayCounter.steps), "\n%d", (int)data->data1);
      text_layer_set_text(s_steps_text_layer, mDisplayCounter.steps);
      break;
    case KEY_NOTIFY_GET_MOVING:
      notify_get_moving();
      break;
    //case KEY_MESSAGE_LOG:
    //  snprintf(tmpStr, 32, "steps:%d  %d", (int)data->data0, (int)data->data1);
		//	app_log(APP_LOG_LEVEL_INFO, "Steps change", 0, tmpStr, s_main_window);
    //  break;
    default:
      return;
  }
  
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  bool running = app_worker_is_running();
  
  // Toggle is sleeping
  settings.isSleeping = settings.isSleeping? false: true;
    
  AppWorkerMessage msg_data = {
    .data0 = settings.isSleeping
  };
  
  if (!running) {
    app_worker_launch();
  }
  
  app_worker_send_message(KEY_APP_MESSAGE_TYPE_ISSLEEPING, &msg_data);
  
  update_sleep();
}


static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  //window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  //window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  // multi click config:
  window_multi_click_subscribe(BUTTON_ID_DOWN, 2, 0, 0, true, down_multi_click_handler);

}

void message_out_sent_handler(DictionaryIterator *sent, void *context) {
// outgoing message was delivered
  //if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AppMessage Delivered"); }
}

void message_out_fail_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
// outgoing message failed
  //if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AppMessage Failed to Send: %d", reason); }
}

void inbox_configuration_handler(DictionaryIterator *received, void *context) {
   // Read first item
	Tuple *appkey = dict_read_first(received);
	
 	// For all items
	while(appkey != NULL) {
    switch(appkey->key) {
      case AK_PEDOMETER_SENSITIVITY:
         if (appkey != NULL) {
            if (appkey->value->uint32 != 0) {
              settings.pedometerSensitivity = appkey->value->uint32;
              send_worker_message(KEY_APP_MESSAGE_TYPE_PEDOMETER_SENSITIVITY);
            }
          }
          break;
      case AK_ENABLE_RESET:
          if (appkey != NULL) {
            settings.enableReset = appkey->value->uint8;
          }
          break;
      case AK_INACTIVITY_MINUTES:
          if (appkey != NULL) {
             if (appkey->value->uint32 > 0) {
                settings.inactivityMinutes = appkey->value->uint32;
                send_worker_message(KEY_APP_MESSAGE_TYPE_INACTIVITY_MINUTES);
             }
          }
          break;
      case AK_ENABLE_DATA_LOGGING:
          if (appkey != NULL) {
            settings.enableDataLogging = appkey->value->uint8;
            send_worker_message(KEY_APP_MESSAGE_TYPE_ENABLE_LOGGING);
          }
          break;
      default:
        break;
    }
    // Look for next item
		appkey = dict_read_next(received);
  }
}

void inbox_rcv_handler(DictionaryIterator *received, void *context) {
  inbox_configuration_handler(received, context);
}

void inbox_drp_handler(AppMessageResult reason, void *context) {
// incoming message dropped
  //if (debug.general) { app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, "AppMessage Dropped: %d", reason); }
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(inbox_rcv_handler);
  app_message_register_inbox_dropped(inbox_drp_handler);
  app_message_register_outbox_sent(message_out_sent_handler);
  app_message_register_outbox_failed(message_out_fail_handler);
}

static void main_window_load(Window *window) {
    
  // Create Additional Info Layer container
  s_main_layer = layer_create(GRect(0, 0, 144, 168));
  
  snprintf(mDisplayCounter.walkTime, sizeof(mDisplayCounter.walkTime), "\n00:00");
  
  s_walk_text_layer = text_layer_create(GRect(0, 0, 72, 49));
  text_layer_set_background_color(s_walk_text_layer, GColorBlack);
	text_layer_set_text_color(s_walk_text_layer, GColorWhite);
	text_layer_set_text_alignment(s_walk_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_walk_text_layer, mDisplayCounter.walkTime);
  text_layer_set_font(s_walk_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(s_main_layer, text_layer_get_layer(s_walk_text_layer));
  
  bmp_walk_layer = bitmap_layer_create( GRect(0, 0, 72, 27) );
  image_walk_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WALK_ICON);
  bitmap_layer_set_bitmap(bmp_walk_layer, image_walk_icon);
  layer_add_child(s_main_layer, bitmap_layer_get_layer(bmp_walk_layer));
  
  snprintf(mDisplayCounter.steps, sizeof(mDisplayCounter.steps), "\n%d", 0);
  
  s_steps_text_layer = text_layer_create(GRect(72, 0, 72, 49));
  text_layer_set_background_color(s_steps_text_layer, GColorBlack);
	text_layer_set_text_color(s_steps_text_layer, GColorWhite);
	text_layer_set_text_alignment(s_steps_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_steps_text_layer, mDisplayCounter.steps);
  text_layer_set_font(s_steps_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(s_main_layer, text_layer_get_layer(s_steps_text_layer));
  
  bmp_steps_layer = bitmap_layer_create( GRect(72, 5, 72, 25) );
  image_steps_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STEPS_ICON);
  layer_add_child(s_main_layer, bitmap_layer_get_layer(bmp_steps_layer));
  bitmap_layer_set_bitmap(bmp_steps_layer, image_steps_icon);
  
  s_sleep_text_layer = text_layer_create(GRect(0, 100, 71, 70));
  text_layer_set_text_alignment(s_sleep_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_sleep_text_layer, mDisplayCounter.sleepTime);
  text_layer_set_font(s_sleep_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(s_main_layer, text_layer_get_layer(s_sleep_text_layer));
  
  if(settings.isSleeping) {
    text_layer_set_background_color(s_sleep_text_layer, GColorBlack);
	  text_layer_set_text_color(s_sleep_text_layer, GColorWhite);
  } else {
    text_layer_set_background_color(s_sleep_text_layer, GColorWhite);
	  text_layer_set_text_color(s_sleep_text_layer, GColorBlack);
  }
  
  snprintf(mDisplayCounter.sleepTime, sizeof(mDisplayCounter.sleepTime), "\n00:00");
  
  bmp_sleep_layer = bitmap_layer_create( GRect(0, 105, 71, 25) );
  image_sleep_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SLEEP_ICON);
  image_inverted_sleep_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_INVERTED_SLEEP_ICON);
  layer_add_child(s_main_layer, bitmap_layer_get_layer(bmp_sleep_layer));
  
  if(settings.isSleeping) {
    bitmap_layer_set_bitmap(bmp_sleep_layer, image_sleep_icon);  
  } else {
    bitmap_layer_set_bitmap(bmp_sleep_layer, image_inverted_sleep_icon);
  }
  
  snprintf(mDisplayCounter.sitTime, sizeof(mDisplayCounter.sitTime), "\n00:00");
  
  s_sit_text_layer = text_layer_create(GRect(73, 100, 72, 70));
  text_layer_set_background_color(s_sit_text_layer, GColorBlack);
	text_layer_set_text_color(s_sit_text_layer, GColorWhite);
	text_layer_set_text_alignment(s_sit_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_sit_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_sit_text_layer, mDisplayCounter.sitTime);
  layer_add_child(s_main_layer, text_layer_get_layer(s_sit_text_layer));
  
  bmp_sit_layer = bitmap_layer_create( GRect(73, 105, 72, 25) );
  image_sit_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SIT_ICON);
  bitmap_layer_set_bitmap(bmp_sit_layer, image_sit_icon);
  layer_add_child(s_main_layer, bitmap_layer_get_layer(bmp_sit_layer));
  
  snprintf(mDisplayCounter.jogTime, sizeof(mDisplayCounter.jogTime), "\n00:00");
  
  s_jog_text_layer =  text_layer_create(GRect(0, 50, 144, 49));
  text_layer_set_background_color(s_jog_text_layer, GColorBlack);
	text_layer_set_text_color(s_jog_text_layer, GColorWhite);
	text_layer_set_text_alignment(s_jog_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_jog_text_layer, mDisplayCounter.jogTime);
  text_layer_set_font(s_jog_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(s_main_layer, text_layer_get_layer(s_jog_text_layer));
  
  bmp_jog_layer = bitmap_layer_create( GRect(40, 52, 65, 25) );
  image_jog_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_JOG_ICON);
  bitmap_layer_set_bitmap(bmp_jog_layer, image_jog_icon);
  layer_add_child(s_main_layer, bitmap_layer_get_layer(bmp_jog_layer));
  
  s_message_layer = text_layer_create(GRect(0, 50, 144, 49));
  text_layer_set_background_color(s_message_layer, GColorBlack);
	text_layer_set_text_color(s_message_layer, GColorWhite);
	text_layer_set_text_alignment(s_message_layer, GTextAlignmentCenter);
  text_layer_set_font(s_message_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  layer_add_child(s_main_layer, text_layer_get_layer(s_message_layer));
  
  layer_set_hidden((Layer *) s_message_layer, true);
    
  layer_add_child(window_get_root_layer(window), s_main_layer);
  
  request_update_from_worker();
  
  if(launch_reason() == APP_LAUNCH_WORKER){
    notify_get_moving();
  }
}
static void main_window_unload(Window *window) {
  text_layer_destroy(s_steps_text_layer);
  text_layer_destroy(s_walk_text_layer);
  text_layer_destroy(s_sleep_text_layer);
  text_layer_destroy(s_sit_text_layer);
  layer_destroy(s_main_layer);
  
  bitmap_layer_destroy(bmp_steps_layer);
  gbitmap_destroy(image_steps_icon);
  bitmap_layer_destroy(bmp_sit_layer);
  gbitmap_destroy(image_sit_icon);
  bitmap_layer_destroy(bmp_walk_layer);
  gbitmap_destroy(image_walk_icon);
  bitmap_layer_destroy(bmp_sleep_layer);
  gbitmap_destroy(image_sleep_icon);
  gbitmap_destroy(image_inverted_sleep_icon);
  bitmap_layer_destroy(bmp_jog_layer);
  gbitmap_destroy(image_jog_icon);
}

static void init() {
  bool running = app_worker_is_running();
  
  app_message_init();
  
  settings.pedometerSensitivity = persist_exists(AK_PERSIST_SETTING_PEDOMETER) ? persist_read_int(AK_PERSIST_SETTING_PEDOMETER): 55;
  settings.isSleeping = persist_exists(AK_PERSIST_SETTING_ISSLEEPING) ? persist_read_bool(AK_PERSIST_SETTING_ISSLEEPING): false;
  settings.inactivityMinutes = persist_exists(AK_PERSIST_SETTING_INACTITY_MINUTES) ?persist_read_int(AK_PERSIST_SETTING_INACTITY_MINUTES):60;
  settings.enableReset = persist_exists(AK_PERSIST_SETTING_ENABLE_RESET)?persist_read_bool(AK_PERSIST_SETTING_ENABLE_RESET):false;
  
  // Create main Window element and assign to pointer
	s_main_window = window_create();

	//Set handlers to manager the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
  
  //Show the Window on the watch, with animated = true
	window_stack_push(s_main_window, true);
  
  window_set_click_config_provider(s_main_window, click_config_provider);
  
  // Subscribe to Worker messages
  app_worker_message_subscribe(worker_message_handler);
  
  // Launch the worker
  if(!running) {
    app_worker_launch();
  }
  
  // Syncronize is sleeping variable
  AppWorkerMessage msg_data = {
      .data0 = settings.isSleeping
    };
  
  app_worker_send_message(KEY_APP_MESSAGE_TYPE_ISSLEEPING, &msg_data);
  	
	// Open AppMessage
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}
static void deinit() {
  persist_write_int(AK_PEDOMETER_SENSITIVITY, settings.pedometerSensitivity);
  persist_write_int(AK_PERSIST_SETTING_INACTITY_MINUTES, settings.inactivityMinutes);
  persist_write_bool(AK_PERSIST_SETTING_ENABLE_RESET, settings.enableReset);
  persist_write_bool(AK_PERSIST_SETTING_ISSLEEPING, settings.isSleeping); 
  
  app_worker_message_unsubscribe();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
