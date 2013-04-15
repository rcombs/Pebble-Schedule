#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
 
#include <stdbool.h>
 
#include "schedule.h"
 
//! Generate your own UUID using `uuidgen` and replace it:
#define APP_UUID { 0x20, 0x01, 0x04, 0x70, 0xC1, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x01 }
PBL_APP_INFO(APP_UUID, "Schedule", "Rodger Combs", 0, 1, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);
 
Window s_window;
TextLayer time_layer, text_layer, name_layer;
 
#include <string.h>
 
static event events[40];
static uint16_t event_count = 0;

static char time_buffer[16] = "Loading...";

/* REVERSE: REVERSE STRING s IN PLACE  */
void reverse(char s[])
{
	int c, i , j;

	for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

int int_pow(int x, int p)
{
  if (p == 0) return 1;
  if (p == 1) return x;

  int tmp = int_pow(x, p/2);
  if (p%2 == 0) return tmp * tmp;
  else return x * tmp * tmp;
}

bool get_bit_from_mask(unsigned int mask, uint8_t bit){
	return !!(mask & int_pow(2, bit));
}

/* ITOA: CONVERT n TO CHARACTERS IN s							 */
/*		 MODIFIED VERSION TO HANDLE THE LARGEST NEGATIVE NUMBER	 */
/*		 FURTHER MODIFIED TO LEFT PAD WITH BLANKS				 */
void itoa(int n, char s[], int maxlen)
{
	int i;
	int sign = (n < 0) ? -1 : 1;

	i = 0;
	do {
		s[i++] = sign * (n % 10) + '0';
	} while ((n /= 10) != 0);
	if (sign < 0)
		s[i++] = '-';
	while (i < maxlen - 1)		/* PAD WITH BLANKS */
		s[i++] = '0';
	s[i] = '\0';
	reverse(s);
}

void redraw(PblTm *tick_time) {
	uint16_t current_time_minutes = tick_time->tm_min + tick_time->tm_hour * 60;
	
	uint16_t current = 0;
	time_type current_type = AFTER;
	
	for(; current < event_count; current++){
		if(!get_bit_from_mask(events[current].flags, tick_time->tm_wday)){
			// Not enabled for today; skip.
			continue;
		}
		// Assume events are ordered by time.
		if(events[current].start_time <= current_time_minutes && events[current].end_time > current_time_minutes) {
			current_type = DURING;
			break;
		}else if(events[current].start_time > current_time_minutes){
			current_type = BEFORE;
			break;
		}
	}
	
	uint16_t remaining;

	if(current_type == BEFORE){
		remaining = events[current].start_time - current_time_minutes - 1;
		
		text_layer_set_text(&text_layer, "UNTIL");

		text_layer_set_text(&name_layer, events[current].name);
	} else if(current_type == DURING) {
		remaining = events[current].end_time - current_time_minutes - 1;
		
		text_layer_set_text(&text_layer, get_bit_from_mask(events[current].flags, 7) ? "LEFT IN" : "LEFT FOR");

		text_layer_set_text(&name_layer, events[current].name);
	} else {
		//The remaining time in the day plus the time in the next day until class starts.
		remaining = ((24*60) - current_time_minutes) + events[0].start_time - 1;
		
		text_layer_set_text(&text_layer, "UNTIL");

		text_layer_set_text(&name_layer, events[0].name);
	}
	
	itoa(remaining, time_buffer, 3);
	
	char seconds_buffer[4];
	itoa(59 - tick_time->tm_sec, seconds_buffer, 3);
	strncat(time_buffer, ":", 3);
	strncat(time_buffer, seconds_buffer, 3);
	
	if(remaining == 0 && tick_time->tm_sec == 59){
		vibes_long_pulse();
	}

	text_layer_set_text(&time_layer, time_buffer);
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *event) {
	redraw(event->tick_time);
}
 
void handle_init(AppContextRef ctx) {
	(void)ctx;
	
	window_init(&s_window, "School Schedule");
	window_set_background_color(&s_window, GColorBlack);
	window_stack_push(&s_window, true /* Animated */);
	
	resource_init_current_app(&SCHEDULE);
	
	text_layer_init(&time_layer, GRect(0, 12, 144, 50));
	layer_add_child(&s_window.layer, &time_layer.layer);
	text_layer_init(&text_layer, GRect(0, 60, 144, 60));
	layer_add_child(&s_window.layer, &text_layer.layer);
	text_layer_init(&name_layer, GRect(0, 99, 144, 90));
	layer_add_child(&s_window.layer, &name_layer.layer);
	
	text_layer_set_text_color(&time_layer, GColorWhite);
	text_layer_set_background_color(&time_layer, GColorClear);
	text_layer_set_font(&time_layer, fonts_get_system_font(FONT_KEY_GOTHAM_42_MEDIUM_NUMBERS));
	text_layer_set_text_alignment(&time_layer, GTextAlignmentCenter);
	
	text_layer_set_text_color(&text_layer, GColorWhite);
	text_layer_set_background_color(&text_layer, GColorClear);
	text_layer_set_font(&text_layer, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
	text_layer_set_text_alignment(&text_layer, GTextAlignmentCenter);
	
	text_layer_set_text_color(&name_layer, GColorWhite);
	text_layer_set_background_color(&name_layer, GColorClear);
	text_layer_set_font(&name_layer, fonts_get_system_font(FONT_KEY_GOTHAM_30_BLACK));
	text_layer_set_text_alignment(&name_layer, GTextAlignmentCenter);
	
	ResHandle config_file = resource_get_handle(RESOURCE_ID_EVENT_LIST);
	size_t config_size = resource_size(config_file);
	event_count = config_size / sizeof(event);
	resource_load(config_file, (uint8_t*)events, config_size);
	
	PblTm tick_time;
	get_time(&tick_time);
	redraw(&tick_time);
}
 
void pbl_main(void *params) {
	AppContextRef ctx = (AppContextRef) params;
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.tick_info = {
			.tick_handler = &handle_tick,
			.tick_units = SECOND_UNIT
		}
	};
	app_event_loop(ctx, &handlers);
}