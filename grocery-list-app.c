#include <pebble.h>

//~STRUCT DEFINITIONS============================================================================================================================
typedef struct grocery_list_item_t {

    //Boolean value indicating if the checkbox should be checked or unchecked
    bool is_checked;
    //String that states the item name.
    char *item_name;
    //Pointer to next element, NULL if no more elements
    struct grocery_list_item_t *next_item;
} GroceryListItem;

//~CONSTANTS====================================================================================================================================
static const char *FILENAME = "grocery-list.c";

//static const char

//The PebbleDictionary key value used to retrieve the identifying transactionId field.
static const int TRANSACTION_ID_KEY = 79;

//The number of rows to display on the Pebble screen
static const int NUM_DISPLAY_ROWS = 5;

//Enum to store the ids that identify the contents of a dictionary
enum TransactionId {
    GROCERY_LIST_REQUEST_ID = 94,
    GROCERY_LIST_SEND_ID = 95,
    GROCERY_LIST_CHECK_RECEIVE_ID = 97,
    GROCERY_LIST_CHECK_SEND_ID = 96,
};

//Enum to store all of the keys used for storing various values in the dictionaries
enum FieldKeys {

    LIST_SIZE_KEY = 37,
    CHECK_KEY = 38
};

//~UI VARIABLES=================================================================================================================================
//The Window that is the basis for this app
static Window *window;

//MenuLayer that displays the grocery list on the pebble
static MenuLayer *menu_layer;

//Singly Linked List of grocery_list_items
static GroceryListItem *grocery_list = NULL;

//Bitmap for the unchecked box
static GBitmap *unchecked_bitmap;

//Bitmap for the checked box
static GBitmap *checked_bitmap;

//~LIST HELPER FUNCTIONS=======================================================================================================================
//Adds the passed item to the passed index
//If index == -1, the item is added to the end of the list
static void list_add_item(GroceryListItem *item) {

    GroceryListItem *it = grocery_list;

    if (grocery_list) {
   
        while (it->next_item != NULL) {

            it = it->next_item;
        }

        it->next_item = item;
        item->next_item = NULL;
    }
    else {

        grocery_list = item;
    }
}

//Gets the item at index position in the grocery list
//If the index is invalid, NULL is returned
static GroceryListItem* list_get_item(uint8_t index) {

    GroceryListItem *it = grocery_list;
    uint8_t count = 0;
    while(it != NULL && count != index) {

        it = it->next_item;
        count++;
    }

    return it;
}

//Gets the size of the GroceryListItem list
static uint8_t list_size() {

    GroceryListItem *it = grocery_list;
    uint8_t size = 0;
    while (it != NULL) {

        it = it->next_item;
        size++;
    }

    return size;
}

static void list_print() {

    GroceryListItem *it = grocery_list;
    uint8_t count = 0;
    while (it != NULL) {

        app_log(4, FILENAME, 129, "item %d: it->item_name = %s\n", count, it->item_name);
        count++;
        it = it->next_item;
    }
}

static void list_destroy() {

    GroceryListItem *it = grocery_list;
    GroceryListItem *it_buff;
    while (it != NULL) {

        it_buff = it;
        it = it->next_item;

        free(it_buff->item_name);
        free(it_buff);
        it_buff = NULL;
    }
}

//Request Grocery List from phone
static void send_request_message() {

    app_log(4, FILENAME, 161, "Requesting Grocery List from phone....\n");

    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    Tuplet value = TupletInteger(TRANSACTION_ID_KEY, GROCERY_LIST_REQUEST_ID);
    dict_write_tuplet(iter, &value);

    app_message_outbox_send();    
}

//Send a check event on a specified index
//to the phone
static void send_check_message(int index) {
    //Setup a dictionary iterator to go through the messages
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    
    //Set the Transaction_Id to GROCERY_LIST_CHECK_SEND_ID into a Tuplet
    Tuplet value1 = TupletInteger(TRANSACTION_ID_KEY, GROCERY_LIST_CHECK_SEND_ID);
    dict_write_tuplet(iter, &value1);

    //Sets the index in a tuplet with CHECK_KEY as the key
    Tuplet value2 = TupletInteger(CHECK_KEY, index);
    dict_write_tuplet(iter, &value2);

    //Send the messages in the outbox
    app_message_outbox_send();
}

//~APP MESSAGE CALLBACKS========================================================================================================================
//Handles Receipt of a message
static void in_received_handler(DictionaryIterator *received, void *context) {
    app_log(4, FILENAME, 161, "Received the message!\n");

    // incoming message received
    //Get the size of the list that was sent
    Tuple *temp_tuple = dict_find(received, LIST_SIZE_KEY);

    if (temp_tuple) {
        //Destroy the previous list
        list_destroy();
        
        uint8_t i, j;
        //Get list size from temp_tuple
        uint32_t grocery_list_size = temp_tuple->value->uint8;
        GroceryListItem *item;
        app_log(4, FILENAME, 161, "Got the list size == %d\n", (int)grocery_list_size);
        for (i = 0; i < grocery_list_size; i++) {
            app_log(4, FILENAME, 333, "start looping...and murdering my watch...\n");
            //Get tuple for each entry and unpack byte array
            //Allocate memoery for new GroceryListItems
            temp_tuple = dict_find(received, i);
            item = malloc(sizeof(GroceryListItem));
            item->item_name = malloc(temp_tuple->length);
        
            for (j = 0; j < temp_tuple->length - 1; j++) {
        
                item->item_name[j] = temp_tuple->value->data[j];
            }
            item->item_name[j] = '\0';
            item->is_checked = temp_tuple->value->data[(temp_tuple->length - 1)];
            app_log(4, FILENAME, 333, "add item looping....\n");
            list_add_item(item);
            app_log(4, FILENAME, 333, "en dlooping...and murdering my watch...\n");
        }
        
        //Reload data and re-draw!
        app_log(4, FILENAME, 326, "CALLING REDRAW THING!\n");
        menu_layer_reload_data(menu_layer);
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
    //Do nothing here
}

//~MENU LAYER CALLBACKS=========================================================================================================================
static void menu_draw_row_callback(GContext *context, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    //Get item this row corresponds to
    GroceryListItem *item = list_get_item(cell_index->row);

    if (item != NULL) {
        //Determine which bitmap to draw
        GBitmap *icon = (item->is_checked) ? checked_bitmap : unchecked_bitmap;
        //Draw the icon and the text
        menu_cell_basic_draw(context, cell_layer, item->item_name, NULL, icon);
    }
    else {
        app_log(4, FILENAME, 198, "The index fetched a NUll grocery item...oops\n");
        menu_cell_title_draw(context, cell_layer, "---ERROR---");
    }
}

//Callback returning the height of a cell
//Display 5 rows on the pebble at once, set size according to this
static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {

    GRect total_bounds = layer_get_bounds(menu_layer_get_layer(menu_layer));
    return total_bounds.size.h / NUM_DISPLAY_ROWS;
}

//Callback returning the number of rows per section, based on the section
//the number of items for the first 1, 0 for all others
static uint16_t menu_get_number_of_rows(MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {

    //based on stuff
    return section_index == 0 ? list_size() : 0;
}

//Callback returning the number of sections in the menu_layer
//Only have 1 section
static uint16_t menu_get_number_of_sections(MenuLayer *menu_layer, void *callback_context) {

    return 1;
}

//Callback for the select button clicked
static void menu_select_grocery_item(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    //Send check event
    send_check_message(cell_index->row);
    //
    //Mark element as checked in list
    GroceryListItem *item = list_get_item(cell_index->row);
    item->is_checked = (item->is_checked) ? false : true;

    //Redraw menu_layer stuff
    menu_layer_reload_data(menu_layer);
}

//~UI SETUP FUNCTIONS=========================================================================================================================
//Setup the layers in the Window
static void window_load(Window *window) {
    Layer* window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
        .draw_header = NULL,
        .draw_row = menu_draw_row_callback,
        .get_cell_height = menu_get_cell_height_callback,
        .get_header_height = NULL,
        .get_num_rows = menu_get_number_of_rows,
        .get_num_sections = menu_get_number_of_sections,
        .select_click = menu_select_grocery_item,
        .select_long_click = menu_select_grocery_item,
        .selection_changed = NULL
        });

    menu_layer_set_click_config_onto_window(menu_layer, window);
    layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
}

//Destroy everything in the Window
static void window_unload(Window *window) {

    menu_layer_destroy(menu_layer);
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
    checked_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CHECKED_BOX_ICON);
    unchecked_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UNCHECKED_BOX_ICON);

    //Register AppMessage Callbacks
    app_message_register_inbox_received(in_received_handler);
    app_message_register_inbox_dropped(in_dropped_handler);
    app_message_register_outbox_sent(out_sent_handler);
    app_message_register_outbox_failed(out_failed_handler);

    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

    //Request the grocery list
    send_request_message();

    // Push the window
    window_stack_push(window, true);
}

//Destroy the window
static void deinit(void) {
    
    //Destroy anything using dynamic memory

    list_destroy();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
