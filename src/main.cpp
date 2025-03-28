// --- Foundation
#include <foundation.h>
#include <os_specific.h>
#include <math/algebra.h>

// --- App
#include "app.h"
#include "draw.h"

static
void do_one_frame(App *app) {
	update_window(&app->window);

	//
	// Update the camera
	//
	{
		app->camera.fov      = 60;
		app->camera.near     = 0.1f;
		app->camera.far      = 10.0f;
		app->camera.ratio    = (f32) app->window.w / (f32) app->window.h;
		app->camera.position = v3f(0, 0, 5);
		app->camera.rotation = v3f(0, 0, 0);
		m4f projection = make_perspective_projection_matrix_vertical_fov(app->camera.fov, app->camera.ratio, app->camera.near, app->camera.far);
		m4f view = make_view_matrix(app->camera.position, app->camera.rotation);
		app->camera.projection_view = projection * view;
	}
}

int main() {
	App app;
	os_enable_high_resolution_clock();
	os_set_working_directory(os_get_executable_directory());
	create_temp_allocator(4 * ONE_MEGABYTE);

	app.pool.create(128 * ONE_MEGABYTE);
	app.allocator = app.pool.allocator();

	create_window(&app.window, "World View"_s);
    setup_draw_data(&app);
	show_window(&app.window);

	app.map_mode = MAP_MODE_2D;

	create_tile(&app, &app.root, app.map_mode, { -90, -180, 90, 180 });

	while(!app.window.should_close) {
		Hardware_Time frame_begin = os_get_hardware_time();

		do_one_frame(&app);
		draw_one_frame(&app);

		Hardware_Time frame_end = os_get_hardware_time();
		os_sleep_to_tick_rate(frame_begin, frame_end, FRAME_RATE);
	}

	destroy_tile(&app, &app.root);

	destroy_draw_data(&app);
	destroy_window(&app.window);
	app.pool.destroy();
	destroy_temp_allocator();

	return 0;
}
