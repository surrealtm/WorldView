#include <foundation.h>
#include <window.h>
#include <d3d11_layer.h>
#include <os_specific.h>

#define FRAME_RATE 60

struct App {
	Window window;
	Frame_Buffer *fbo;
};

static
void do_one_frame(App *app) {
	update_window(&app->window);
}

static
void draw_one_frame(App *app) {
	clear_frame_buffer(app->fbo, 50 / 255.0f, 96 / 255.0f, 140 / 255.0f);
	swap_d3d11_buffers(&app->window);
}

int main() {
	App app;
	create_window(&app.window, "World View"_s);
	create_d3d11_context(&app.window, false);
	app.fbo = get_default_frame_buffer(&app.window);
	show_window(&app.window);

	while(!app.window.should_close) {
		Hardware_Time frame_begin = os_get_hardware_time();

		do_one_frame(&app);
		draw_one_frame(&app);

		Hardware_Time frame_end = os_get_hardware_time();
		os_sleep_to_tick_rate(frame_begin, frame_end, FRAME_RATE);
	}

	destroy_d3d11_context(&app.window);
	destroy_window(&app.window);
	return 0;
}
