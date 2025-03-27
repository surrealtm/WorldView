// --- Foundation
#include <foundation.h>
#include <d3d11_layer.h>
#include <os_specific.h>

// --- App
#include "app.h"

static
void do_one_frame(App *app) {
	update_window(&app->window);
}

int main() {
	App app;
	create_window(&app.window, "World View"_s);
    create_renderer(&app);
	show_window(&app.window);

	while(!app.window.should_close) {
		Hardware_Time frame_begin = os_get_hardware_time();

		do_one_frame(&app);
		draw_one_frame(&app);

		Hardware_Time frame_end = os_get_hardware_time();
		os_sleep_to_tick_rate(frame_begin, frame_end, FRAME_RATE);
	}

	destroy_renderer(&app);
	destroy_window(&app.window);
	return 0;
}
