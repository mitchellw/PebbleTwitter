#include <pebble.h>
#include <stdbool.h>

//~STRUCT DEFINITIONS============================================================================================================================
typedef struct tweet_item_t {

    //String that states the item name.
    char *tweet;
    //String that states the twitter account
    char *author;
    //Pointer to next element, NULL if no more elements
    struct tweet_item_t *next_item;
    
} TweetItem;

//~CONSTANTS====================================================================================================================================
static const char *FILENAME = "tweet-list.c";

//static const char

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
};

//~UI VARIABLES=================================================================================================================================
//The Window that is the basis for this app
static Window *window;


//Singly Linked List of tweet_list_items
static TweetItem *tweet_list = NULL;

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

//~LIST HELPER FUNCTIONS=======================================================================================================================
//Adds the passed item to the passed index
//If index == -1, the item is added to the end of the list
static void list_add_item(TweetItem *item) {

    TweetItem *it = tweet_list;

    if (tweet_list) {
   
        while (it->next_item != NULL) {

            it = it->next_item;
        }

        it->next_item = item;
        item->next_item = NULL;
    }
    else {

        tweet_list = item;
    }
}

//Gets the item at index position in the tweet list
//If the index is invalid, NULL is returned
static TweetItem* list_get_item(uint8_t index) {

    TweetItem *it = tweet_list;
    uint8_t count = 0;
    while(it != NULL && count != index) {

        it = it->next_item;
        count++;
    }

    return it;
}

//Gets the size of the TweetItem list
static uint8_t list_size() {

    TweetItem *it = tweet_list;
    uint8_t size = 0;
    while (it != NULL) {

        it = it->next_item;
        size++;
    }

    return size;
}

static void list_print() {

    TweetItem *it = tweet_list;
    uint8_t count = 0;
    while (it != NULL) {

        app_log(4, FILENAME, 129, "item %d: it->tweet = %s\n", count, it->tweet);
        count++;
        it = it->next_item;
    }
}

static void list_destroy() {

    TweetItem *it = tweet_list;
    TweetItem *it_buff;
    while (it != NULL) {

        it_buff = it;
        it = it->next_item;

        free(it_buff->tweet);
        free(it_buff);
        it_buff = NULL;
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

//Send a check event on a specified index
//to the phone
/*
static void send_check_message(int index) {
    //Setup a dictionary iterator to go through the messages
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    //Set the Transaction_Id to TWEET_SEND_ID into a Tuplet
    Tuplet value1 = TupletInteger(TRANSACTION_ID_KEY, TWEET_SEND_ID);
    dict_write_tuplet(iter, &value1);

    //Sets the index in a tuplet with CHECK_KEY as the key
    Tuplet value2 = TupletInteger(CHECK_KEY, index);
    dict_write_tuplet(iter, &value2);

    //Send the messages in the outbox
    app_message_outbox_send();
}
*/

//~APP MESSAGE CALLBACKS========================================================================================================================
//Handles Receipt of a message
static void in_received_handler(DictionaryIterator *received, void *context) {
    app_log(4, FILENAME, 161, "Received the message!\n");

    // incoming message received
    //Get the size of the list that was sent
    Tuple *temp_tuple = dict_find(received, TWEET_KEY);

    if (temp_tuple) {
        //Destroy the previous list
        //list_destroy();
        
        uint8_t j;
        //Get list size from temp_tuple
        TweetItem *item;
        app_log(4, FILENAME, 161, "Got data\n");
        //Get tuple for each entry and unpack byte array
        //Allocate memory for new TweetItems
        item = malloc(sizeof(TweetItem));
        item->tweet = malloc(temp_tuple->length);
        item->author = malloc(temp_tuple->length);
        bool author = true;
        int author_length = 0;
    
        for (j = 0; j < temp_tuple->length - 1; j++) {
    
            if(temp_tuple->value->data[j] == '|')
            {
                author = false;
                author_length = j;
            }
            else if(author == false)
            {
                item->tweet[j-author_length-1] = temp_tuple->value->data[j];
            }
            else
            {
                item->author[j] = temp_tuple->value->data[j];
            }
        
        author = false;
        item->tweet[j-author_length-1] = '\0';
        item->author[author_length]='\0';
        
        text_layer_set_text(tweet_layer, item->tweet);
        text_layer_set_text(author_layer, item->author);
        
        GRect bounds = layer_get_frame(window_layer);
        GSize max_size = text_layer_get_content_size(tweet_layer);
        text_layer_set_size(tweet_layer, max_size);
        scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, max_size.h + vert_scroll_text_padding));

        //item->is_checked = temp_tuple->value->data[(temp_tuple->length - 1)];
        //list_add_item(item);
        
        //Reload data and re-draw!
        app_log(4, FILENAME, 326, "CALLING REDRAW THING!\n");
        //menu_layer_reload_data(menu_layer);
        return;
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
    GRect max_text_bounds = GRect(0, 0, bounds.size.w, 2000);

      // Initialize the scroll layer
      scroll_layer = scroll_layer_create(bounds);

      // This binds the scroll layer to the window so that up and down map to scrolling
      // You may use scroll_layer_set_callbacks to add or override interactivity
      scroll_layer_set_click_config_onto_window(scroll_layer, window);

      // Initialize the text layers
      tweet_layer = text_layer_create(max_text_bounds);
      author_layer = text_layer_create(max_text_bounds);
      text_layer_set_font(tweet_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
      text_layer_set_font(author_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
      
      scroll_layer_add_child(scroll_layer, text_layer_get_layer(tweet_layer));
      scroll_layer_add_child(scroll_layer, text_layer_get_layer(author_layer));
      
      layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));
      
    /*menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
        .draw_header = NULL,
        .draw_row = menu_draw_row_callback,
        .get_cell_height = menu_get_cell_height_callback,
        .get_header_height = NULL,
        .get_num_rows = menu_get_number_of_rows,
        .get_num_sections = menu_get_number_of_sections,
        .select_click = NULL,
        .select_long_click = NULL,
        .selection_changed = NULL
        });

    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));*/
}

//Destroy everything in the Window
static void window_unload(Window *window) {
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

    //Setup Bitmaps
    //checked_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CHECKED_BOX_ICON);
    //unchecked_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UNCHECKED_BOX_ICON);

    //Register AppMessage Callbacks
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_sent(out_sent_handler);
    app_message_register_outbox_failed(out_failed_handler);

    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    //Request the tweet list
    send_request_message();

    // Push the window
    window_stack_push(window, true);
}

//Destroy the window
static void deinit(void) {
    
    //Destroy anything using dynamic memory
    list_destroy();
    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
