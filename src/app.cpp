// --- Foundation
#include <foundation.h>
#include <os_specific.h>
#include <math/maths.h>
#include <math/algebra.h>

// --- App
#include "app.h"
#include "draw.h"

static
void lerp(f64 *value, f64 target, f64 speed) {
	*value += (target - *value) * speed;
}

static
f64 wrap(f64 value, f64 low, f64 high) {
	if(value >= 0 || low >= 0) {
		return fmod(value - low, high - low) + low;
	} else {
		return high - fmod(-value - low, high - low);
	}
}

static
void lerp_with_wrap(f64 *value, f64 target, f64 speed, f64 low, f64 high) {
	f64 direct_distance    = fabs(target - *value);
	f64 wrap_high_distance = (high - *value) + (target - low);
	f64 wrap_low_distance  = (high - target) + (*value - low);

	if(direct_distance < wrap_high_distance && direct_distance < wrap_low_distance) {
		*value += (target - *value) * speed;
	} else if(wrap_high_distance < wrap_low_distance) {
		*value += wrap_high_distance * speed;	
	} else {
		*value -= wrap_low_distance * speed;	
	}

	*value = wrap(*value, low, high);
}

static
void do_one_frame(App *app) {
	update_window(&app->window);

	//
	// Update the camera
	//
	{
#define ZOOM_STEP 1.13
#define INTERP_SPEED clamp(app->window.frame_time * 100, 0, 1)

		app->camera.fov   = 90.0f;
		app->camera.near  = 0.001f;
		app->camera.far   = WORLD_SCALE_3D * 2.0f;
		app->camera.ratio = (f32) app->window.w / (f32) app->window.h;

		app->camera.zoom_level -= app->window.mouse_wheel_turns / 20;
		
		if(app->window.buttons[BUTTON_Left] & BUTTON_Down) {
			f64 sensitivity;

			switch(app->map_mode) {
			case MAP_MODE_2D: sensitivity = 180.0 * (app->camera.current_distance / WORLD_SCALE_2D); break;
			case MAP_MODE_3D: sensitivity = 180.0 * pow(app->camera.current_distance / app->camera.far, 5); break;
			}

			app->camera.target_center.lat += ((f64) app->window.mouse_delta_y / (f64) app->window.h) * sensitivity;
			app->camera.target_center.lon -= ((f64) app->window.mouse_delta_x / (f64) app->window.h) * sensitivity;
		}
		
		f64 min_distance, max_distance;
		switch(app->map_mode) {
		case MAP_MODE_2D: min_distance = 0.001f; max_distance = WORLD_SCALE_2D; break;
		case MAP_MODE_3D: min_distance = WORLD_SCALE_3D; max_distance = app->camera.far; break;
		}

		app->camera.zoom_level        = clamp(app->camera.zoom_level, 0.001f, 1.f);
		app->camera.target_distance   = pow(ZOOM_STEP, 50 * (app->camera.zoom_level - 1)) * (max_distance - min_distance) + min_distance;
		app->camera.target_center.lat = clamp(app->camera.target_center.lat, -90.0, 90.0);

		switch(app->map_mode) {
		case MAP_MODE_2D: app->camera.target_center.lon = clamp(app->camera.target_center.lon, -180.0, 180.0); break;
		case MAP_MODE_3D: app->camera.target_center.lon = wrap(app->camera.target_center.lon, -180.0, 180.0); break;
		}
		
		lerp(&app->camera.current_center.lat, app->camera.target_center.lat, INTERP_SPEED);
		lerp_with_wrap(&app->camera.current_center.lon, app->camera.target_center.lon, INTERP_SPEED, -180, 180);
		lerp(&app->camera.current_distance,   app->camera.target_distance, INTERP_SPEED);

		app->camera.current_center.lat = clamp(app->camera.current_center.lat, -90.0, 90.0);
		app->camera.current_center.lon = clamp(app->camera.current_center.lon, -180.0, 180.0);

		m4f projection, view;

		switch(app->map_mode) {
		case MAP_MODE_2D: {
			v3f position = v3f((f32) app->camera.current_center.lon / 90.0f * WORLD_SCALE_2D, (f32) app->camera.current_center.lat / 90.0f * WORLD_SCALE_2D, 0);
			v3f rotation = v3f(0, 0, 0);
			projection = make_orthographic_projection_matrix((f32) (app->camera.current_distance * app->camera.ratio * 2.0), (f32) (app->camera.current_distance * 2.0), WORLD_SCALE_2D);
			view = make_view_matrix(position, rotation);
		} break;

		case MAP_MODE_3D: {
			f64 theta = degrees_to_radians(app->camera.current_center.lon);
			f64 sigma = degrees_to_radians(app->camera.current_center.lat);

			v3f position = v3f((f32) (sin(theta) * cos(sigma) * app->camera.current_distance),
							   (f32) (sin(sigma) * app->camera.current_distance),
							   (f32) (cos(theta) * cos(sigma) * app->camera.current_distance));
			v3f rotation = v3f((f32) (app->camera.current_center.lat / 180.0 * 0.5), (f32) -(app->camera.current_center.lon / 180.0 * 0.5f), 0);

			projection = make_perspective_projection_matrix_vertical_fov(app->camera.fov, app->camera.ratio, app->camera.near, app->camera.far);
			view = make_view_matrix(position, rotation);
		} break;
		}

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

	app.map_mode = MAP_MODE_3D;
	app.camera.target_center    = Coordinate{ 0, 0 };
	app.camera.zoom_level       = 0.5;
	app.camera.target_distance  = 0;
	app.camera.current_center   = app.camera.target_center;
	app.camera.current_distance = app.camera.target_distance;

	create_tile(&app, &app.root, app.map_mode, { -90, -180, 90, 180 });
	//subdivide_tile(&app, &app.root);

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
