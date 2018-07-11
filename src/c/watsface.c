#include <pebble.h>

//#define TEST_TIME_HOUR 13
//#define TEST_TIME_MIN 30
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define nullptr 0

static Window* s_window;
static Layer* s_layerTime = nullptr;
static TextLayer* s_layerBattery = nullptr;
static TextLayer* s_layerDay = nullptr;
static Layer* s_layerCal = nullptr;
static TextLayer* s_layerMonth = nullptr;
static bool s_bluetoothConnected = true;
static GBitmap* s_fontAtlas;
static GBitmap* s_fontImages[10];
static char s_battery[8] = "";
static char s_day[8] = "";
static char s_cal[8] = "";
static char s_month[8] = "";
static struct tm s_time;

struct FontCharacter
{
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
	unsigned int xoffset;
	unsigned int yoffset;
	unsigned int xadvance;
};

// http://kvazars.com/littera/ (txt fnt format)
static struct FontCharacter s_fontChars[10] =
{	
    {0, 0, 40, 77, 3, 0, 47},
    {40, 0, 22, 74, 2, 1, 28},
    {62, 0, 40, 76, 2, 0, 44},
    {102, 0, 40, 77, 3, 0, 45},
    {142, 0, 42, 74, 3, 1, 45},
    {184, 0, 39, 76, 4, 1, 46},
    {223, 0, 40, 77, 4, 0, 46},
    {263, 0, 32, 74, 1, 2, 34},
    {295, 0, 40, 77, 2, 0, 45},
    {335, 0, 40, 77, 3, 0, 47}
};

static void BatteryEvent(BatteryChargeState battery)
{
	char* status;
	if (battery.is_charging)
	{
		status = "\U0001F493";
	}
	else if (battery.is_plugged)
	{
		status = "\U0001F499";
	}
	else
	{
		status = "%";
	}
	
	snprintf(s_battery, ARRAY_SIZE(s_battery), "%u%s", battery.charge_percent, status);
	layer_mark_dirty((Layer*)s_layerBattery);
}

static void UpdateDayDisplay(struct tm* tickTime)
{
	strftime(s_day, ARRAY_SIZE(s_day), "%a", tickTime);
	strftime(s_cal, ARRAY_SIZE(s_cal), "%d", tickTime);
	strftime(s_month, ARRAY_SIZE(s_month), "%b", tickTime);
	if (s_layerDay)
	{
		layer_mark_dirty((Layer*)s_layerDay);
		layer_mark_dirty((Layer*)s_layerCal);
		layer_mark_dirty((Layer*)s_layerMonth);
	}	
}

static void TickEvent(struct tm* tickTime, TimeUnits unitsChanged)
{
	if (tickTime)
	{
		s_time = *tickTime;
#ifdef TEST_TIME_HOUR
		s_time.tm_hour = TEST_TIME_HOUR;
		s_time.tm_min = TEST_TIME_MIN;
#endif
	}
	if (s_layerTime)
	{
		layer_mark_dirty(s_layerTime);
	}
	
    if ((unitsChanged & DAY_UNIT) != 0)
	{
		UpdateDayDisplay(tickTime);
    }
}

static void ConnectionEvent(bool connected)
{
	s_bluetoothConnected = connected;
	if (!quiet_time_is_active())
	{
		vibes_double_pulse();
	}
}

static void LayerUpdateTime(struct Layer* layer, GContext* ctx)
{
	GRect bounds = layer_get_bounds(layer);
	const int marginX = 7;
	const int gapX = 3;
	const int marginY = 3;
	const int marginOne = 17;
	const int minY = 84 + marginY;
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	
	GBitmap* bitmap;
	GRect rect;
	GRect textSize;
	struct FontCharacter* ctr;
	
	int hour1 = s_time.tm_hour / 10;
	int hour2 = s_time.tm_hour % 10;
	int min1 = s_time.tm_min / 10;
	int min2 = s_time.tm_min % 10;
	
	ctr = &s_fontChars[hour1];
	bitmap = s_fontImages[hour1];
	textSize = gbitmap_get_bounds(bitmap);
	const bool offsetFirstOne = (hour1 == 1 && min1 != 1 && min2 != 1) || (hour1 == 1 && hour2 == 1 && (min1 != 1 || min2 != 1));
    rect = GRect(marginX + (offsetFirstOne ? marginOne : 0), marginY + ctr->yoffset, textSize.size.w, textSize.size.h);
	graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
	rect.size.w = ctr->xadvance + (offsetFirstOne ? marginOne : 0);
		
	ctr = &s_fontChars[hour2];
	bitmap = s_fontImages[hour2];
	textSize = gbitmap_get_bounds(bitmap);
    rect = GRect(rect.size.w + gapX + ctr->xoffset, marginY + ctr->yoffset, textSize.size.w, textSize.size.h);
	graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
	
	ctr = &s_fontChars[min1];
	bitmap = s_fontImages[min1];
	textSize = gbitmap_get_bounds(bitmap);
    rect = GRect(marginX + (min1 == 1 && hour1 != 1 ? marginOne : 0), minY + ctr->yoffset, textSize.size.w, textSize.size.h);
	graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
	rect.size.w = ctr->xadvance + (min1 == 1 && hour1 != 1 ? marginOne : 0);
		
	ctr = &s_fontChars[min2];
	bitmap = s_fontImages[min2];
	textSize = gbitmap_get_bounds(bitmap);
    rect = GRect(rect.size.w + gapX + ctr->xoffset, minY + ctr->yoffset, textSize.size.w, textSize.size.h);
	graphics_draw_bitmap_in_rect(ctx, bitmap, rect);
}

static void LayerUpdateCal(struct Layer* layer, GContext* ctx)
{
	GRect bounds = layer_get_bounds(layer);
	graphics_context_set_fill_color(ctx, GColorRed);
	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 3, GCornersAll);
	graphics_draw_round_rect(ctx, bounds, 3);
	graphics_draw_text(ctx, s_cal, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD), grect_inset(bounds, GEdgeInsets(-3)), GTextOverflowModeWordWrap, GTextAlignmentCenter, nullptr);
}

static void WindowLoad(Window* window)
{
	Layer* windowLayer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(windowLayer);
	
	const int calY = 20;
	
	s_layerTime = layer_create(GRect(0, 0, bounds.size.w - 40, bounds.size.h));
	layer_set_update_proc(s_layerTime, LayerUpdateTime);
	layer_add_child(windowLayer, s_layerTime);
	
	s_layerBattery = text_layer_create(GRect(bounds.size.w - 40, bounds.size.h - 26, 38, 20));
	text_layer_set_text(s_layerBattery, s_battery);
	text_layer_set_text_alignment(s_layerBattery, GTextAlignmentCenter);
	text_layer_set_text_color(s_layerBattery, GColorBlue);
	text_layer_set_font(s_layerBattery, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	layer_add_child(windowLayer, (Layer*)s_layerBattery);
	
	s_layerMonth = text_layer_create(GRect(bounds.size.w - 40, bounds.size.h - calY - 50, 35, 30));
	text_layer_set_text(s_layerMonth, s_month);
	text_layer_set_text_alignment(s_layerMonth, GTextAlignmentCenter);
	text_layer_set_font(s_layerMonth, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	layer_add_child(windowLayer, (Layer*)s_layerMonth);
		
	GRect calRect = GRect(bounds.size.w - 40, bounds.size.h - calY - 76, 35, 31);
	s_layerCal = layer_create(calRect);
	layer_set_update_proc(s_layerCal, LayerUpdateCal);
	layer_add_child(windowLayer, s_layerCal);
	
	s_layerDay = text_layer_create(GRect(bounds.size.w - 40, bounds.size.h - calY - 106, 35, 30));
	text_layer_set_text(s_layerDay, s_day);
	text_layer_set_text_alignment(s_layerDay, GTextAlignmentCenter);
	text_layer_set_font(s_layerDay, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	layer_add_child(windowLayer, (Layer*)s_layerDay);
	
	layer_mark_dirty(windowLayer);
}

static void WindowUnload(Window* window)
{
	layer_destroy(s_layerTime);
	text_layer_destroy(s_layerBattery);
	text_layer_destroy(s_layerDay);
	text_layer_destroy(s_layerMonth);
	layer_destroy(s_layerCal);
}

static void Init(void)
{
	time_t localTime = time(NULL);
	s_time = *localtime(&localTime);
	UpdateDayDisplay(&s_time);

	s_fontAtlas = gbitmap_create_with_resource(RESOURCE_ID_FONT_ATLAS);
	for (uint i = 0; i < 10; ++i)
	{
		s_fontImages[i] = gbitmap_create_as_sub_bitmap(s_fontAtlas, GRect(s_fontChars[i].x, s_fontChars[i].y, s_fontChars[i].width, s_fontChars[i].height));
	}

	s_window = window_create();
	tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT, TickEvent);
	
	battery_state_service_subscribe(BatteryEvent);
	
	connection_service_subscribe((ConnectionHandlers)
	{
		.pebble_app_connection_handler = ConnectionEvent
	});
	
	window_set_window_handlers(s_window, (WindowHandlers)
	{
		.load = WindowLoad,
		.unload = WindowUnload,
	});

	window_stack_push(s_window, true);
	
	BatteryEvent(battery_state_service_peek());
}

static void Destroy(void)
{
    battery_state_service_unsubscribe();
	tick_timer_service_unsubscribe();
	connection_service_unsubscribe();

	window_destroy(s_window);
	
	for (uint i = 0; i < 10; ++i)
	{
		gbitmap_destroy(s_fontImages[i]);
	}
	gbitmap_destroy(s_fontAtlas);
}

int main(void)
{
	Init();
	app_event_loop();
	Destroy();
}
