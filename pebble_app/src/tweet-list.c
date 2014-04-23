#include <pebble.h>
#include <stdbool.h>

//~STRUCT DEFINITIONS============================================================================================================================
typedef struct tweet_item_t {
  //String that states the item name.
  char *tweet;
  //String that states the twitter account
  char *author;
} TweetItem;

//~CONSTANTS====================================================================================================================================
static const char *FILENAME = "tweet-list.c";

static const int MAX_TWEET_SIZE = 140;
static const int MIN_CHARS_PER_LINE = 10;
static const int VERT_SCROLL_TEXT_PADDING = 4;
static const int TEXT_SIZE = 24;

//The PebbleDictionary key value used to retrieve the identifying transactionId field.
static const int TRANSACTION_ID_KEY = 79;

//Enum to store the ids that identify the contents of a dictionary
enum TransactionId {
  TWEET_REQUEST_ID = 94,
  TWEET_SEND_ID = 95,
};

//Enum to store all of the keys used for storing various values in the dictionaries
enum FieldKeys {
  TWEET_KEY = 37,
  AUTHOR_KEY = 38,
};

//~UI VARIABLES=================================================================================================================================
//The Window that is the basis for this app
static Window *window;

// Current TweetItem
static TweetItem *tweet_item = NULL;

static ScrollLayer *scroll_layer;
static TextLayer *tweet_layer;
static TextLayer *author_layer;

static char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

//Request Tweet List from phone
static void send_request_message() {

  app_log(4, FILENAME, 161, "Requesting Tweet List from phone....\n");

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  Tuplet value = TupletInteger(TRANSACTION_ID_KEY, TWEET_REQUEST_ID);
  dict_write_tuplet(iter, &value);

  app_message_outbox_send();    
}

void tap_handler(AccelAxisType axis, int32_t direction)
{
  if (axis == ACCEL_AXIS_Z)
  {
        send_request_message();
  }
}

//~APP MESSAGE CALLBACKS========================================================================================================================
//Handles Receipt of a message
static void in_received_handler(DictionaryIterator *received, void *context) {
  app_log(4, FILENAME, 161, "Received the message!\n");

  // incoming message received
  //Get the size of the list that was sent
  Tuple *tweet_tuple = dict_find(received, TWEET_KEY);
  Tuple *author_tuple = dict_find(received, AUTHOR_KEY);

  if (tweet_tuple != NULL && author_tuple != NULL) {
    uint8_t j;
    app_log(4, FILENAME, 161, "Got data\n");
    //Get tuple for each entry and unpack byte array
    //Allocate memory for new TweetItem
    if (tweet_item != NULL) {
      if (tweet_item->tweet != NULL) {
        free(tweet_item->tweet);
      }
      if (tweet_item->author != NULL) {
        free(tweet_item->author);
      }
      free(tweet_item);
    }
    tweet_item = malloc(sizeof(TweetItem));
    tweet_item->tweet = malloc(tweet_tuple->length);
    tweet_item->author = malloc(author_tuple->length);

    for (j = 0; j < tweet_tuple->length; j++) {
      tweet_item->tweet[j] = tweet_tuple->value->data[j];
    }
    tweet_item->tweet[j] = '\0';

    for (j = 0; j < author_tuple->length; j++) {
      tweet_item->author[j] = author_tuple->value->data[j];
    }
    tweet_item->author[j]='\0';

    text_layer_set_text(tweet_layer, tweet_item->tweet);
    text_layer_set_text(author_layer, tweet_item->author);

    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);

    GSize author_size = text_layer_get_content_size(author_layer);
    text_layer_set_size(author_layer, author_size);
    GSize tweet_size = text_layer_get_content_size(tweet_layer);
    text_layer_set_size(tweet_layer, tweet_size);

    scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, author_size.h + tweet_size.h + VERT_SCROLL_TEXT_PADDING));

    app_log(4, FILENAME, 161, "author: %s, tweet: %s", tweet_item->author, tweet_item->tweet);

    vibes_double_pulse();
  }
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered
  //Do nothing here
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
  //Do nothing here
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  app_log(4, FILENAME, 161, "Incoming message dropped!\n%d - %s\n",reason,translate_error(reason));
  //Do nothing here
}

//~UI SETUP FUNCTIONS=========================================================================================================================
//Setup the layers in the Window
static void window_load(Window *window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GRect max_text_bounds = GRect(0, 0, bounds.size.w, TEXT_SIZE);

  // Initialize the scroll layer
  scroll_layer = scroll_layer_create(bounds);

  // This binds the scroll layer to the window so that up and down map to scrolling
  // You may use scroll_layer_set_callbacks to add or override interactivity
  scroll_layer_set_click_config_onto_window(scroll_layer, window);

  // Initialize the text layers
  author_layer = text_layer_create(max_text_bounds);
  tweet_layer = text_layer_create(GRect(0, TEXT_SIZE + VERT_SCROLL_TEXT_PADDING, bounds.size.w, (MAX_TWEET_SIZE/MIN_CHARS_PER_LINE)*TEXT_SIZE));
  text_layer_set_font(tweet_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_font(author_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  
  //Motion Sensing
  accel_tap_service_subscribe(tap_handler);

  scroll_layer_add_child(scroll_layer, text_layer_get_layer(tweet_layer));
  scroll_layer_add_child(scroll_layer, text_layer_get_layer(author_layer));

  layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));
}

//Destroy everything in the Window
static void window_unload(Window *window) {
  accel_tap_service_unsubscribe();
  text_layer_destroy(tweet_layer);
  text_layer_destroy(author_layer);
  scroll_layer_destroy(scroll_layer);
}

//Initialize the window
static void init(void) {

  // Create a window
  window = window_create();

  //Set window handlers 
  window_set_window_handlers(window,
      (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
      });

  //Register AppMessage Callbacks
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  // Push the window
  window_stack_push(window, true);
}

//Destroy the window
static void deinit(void) {

  //Destroy anything using dynamic memory
  window_destroy(window);
  if (tweet_item != NULL) {
    if (tweet_item->tweet != NULL) {
      free(tweet_item->tweet);
    }
    if (tweet_item->author != NULL) {
      free(tweet_item->author);
    }
    free(tweet_item);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
