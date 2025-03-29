// --- C
#include <stdarg.h>

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
v3f get_ray_from_screen(App *app, f32 mouse_x, f32 mouse_y) {
	v4f clip  = v4f(mouse_x / app->window.w * 2.0f - 1.0f, 1.0f - mouse_y / app->window.h * 2.0f, -1.0f, 1.0f);
	v4f eye   = m4_inverse(app->camera.projection) * clip;
	v4f world = m4_inverse(app->camera.view) * v4f(eye.x, eye.y, -1.0f, 0.0f);
	return v3_normalize(v3f(world.x, world.y, world.z));
}

static
void do_one_frame(App *app) {
	update_window(&app->window);

	//
	// Update the mode
	//
	if(app->window.keys[KEY_Tab] & KEY_Pressed) {
		switch(app->map_mode) {
		case MAP_MODE_2D: app->map_mode = MAP_MODE_3D; break;
		case MAP_MODE_3D: app->map_mode = MAP_MODE_2D; break;
		}

		app->root.state = TILE_Requires_Regeneration;
	}

	maybe_regenerate_tiles(app, &app->root);

	//
	// Update the camera
	//
	{
#define ZOOM_STEP 1.13
#define INTERP_SPEED clamp(app->window.frame_time * 100, 0, 1)

		app->camera.fov   = 61.0f;
		app->camera.near  = 0.001f;
		app->camera.far   = WORLD_SCALE_3D * 2.0f;
		app->camera.ratio = (f32) app->window.w / (f32) app->window.h;

		//
		// Adjust the distance
		//
		app->camera.zoom_level -= app->window.mouse_wheel_turns / 20;
		app->camera.zoom_level  = clamp(app->camera.zoom_level, 0.001f, 1.f);
		
		f64 min_distance, max_distance;
		switch(app->map_mode) {
		case MAP_MODE_2D: min_distance = 0.001f; max_distance = WORLD_SCALE_2D; break;
		case MAP_MODE_3D: min_distance = WORLD_SCALE_3D; max_distance = app->camera.far; break;
		}

		app->camera.target_distance   = pow(ZOOM_STEP, 50 * (app->camera.zoom_level - 1)) * (max_distance - min_distance) + min_distance;

		//
		// Adjust the center
		//
		if(app->window.buttons[BUTTON_Left] & BUTTON_Down) {
			f64 dy = (f64) app->window.mouse_delta_y / (f64) app->window.h;
			f64 dx = (f64) app->window.mouse_delta_x / (f64) app->window.h;
			f64 x = (app->camera.current_distance - min_distance) / (max_distance - min_distance);
			f64 sensitivity;
			
			switch(app->map_mode) {
			case MAP_MODE_2D: sensitivity = 180.0 * x; break;
			case MAP_MODE_3D: sensitivity = 180.0 * (log(x + 1) / log(3.2)); break;
			}
			
			app->camera.target_center.lat += dy * sensitivity;
			app->camera.target_center.lon -= dx * sensitivity;
		}
		
		app->camera.target_center.lat = clamp(app->camera.target_center.lat, -90.0, 90.0);

		switch(app->map_mode) {
		case MAP_MODE_2D: app->camera.target_center.lon = clamp(app->camera.target_center.lon, -180.0, 180.0); break;
		case MAP_MODE_3D: app->camera.target_center.lon = wrap(app->camera.target_center.lon, -180.0, 180.0); break;
		}
		
		//
		// Lerp from the current to the target 
		//

		lerp(&app->camera.current_center.lat, app->camera.target_center.lat, INTERP_SPEED);
		
		switch(app->map_mode) {
		case MAP_MODE_2D: lerp(&app->camera.current_center.lon, app->camera.target_center.lon, INTERP_SPEED); break;
		case MAP_MODE_3D: lerp_with_wrap(&app->camera.current_center.lon, app->camera.target_center.lon, INTERP_SPEED, -180, 180); break;
		}
		lerp(&app->camera.current_distance, app->camera.target_distance, INTERP_SPEED);

		app->camera.current_center.lat = clamp(app->camera.current_center.lat, -90.0, 90.0);
		app->camera.current_center.lon = clamp(app->camera.current_center.lon, -180.0, 180.0);

		//
		// Set up the camera matrices
		//
		switch(app->map_mode) {
		case MAP_MODE_2D: {
			v3f position = v3f((f32) app->camera.current_center.lon / 90.0f * WORLD_SCALE_2D, (f32) app->camera.current_center.lat / 90.0f * WORLD_SCALE_2D, 0);
			v3f rotation = v3f(0, 0, 0);
			app->camera.projection = make_orthographic_projection_matrix((f32) (app->camera.current_distance * app->camera.ratio * 2.0), (f32) (app->camera.current_distance * 2.0), WORLD_SCALE_2D);
			app->camera.view = make_view_matrix(position, rotation);
		} break;

		case MAP_MODE_3D: {
			f64 theta = degrees_to_radians(app->camera.current_center.lon);
			f64 sigma = degrees_to_radians(app->camera.current_center.lat);

			v3f position = v3f((f32) (sin(theta) * cos(sigma) * app->camera.current_distance),
							   (f32) (sin(sigma) * app->camera.current_distance),
							   (f32) (cos(theta) * cos(sigma) * app->camera.current_distance));
			v3f rotation = v3f((f32) (app->camera.current_center.lat / 180.0 * 0.5), (f32) -(app->camera.current_center.lon / 180.0 * 0.5f), 0);

			app->camera.projection = make_perspective_projection_matrix_vertical_fov(app->camera.fov, app->camera.ratio, app->camera.near, app->camera.far);
			app->camera.view = make_view_matrix(position, rotation);
		} break;
		}

		app->camera.projection_view = app->camera.projection * app->camera.view;
	}
}

void log(Log_Level level, const char *format, ...) {
	const char *LOG_LEVEL_STRING[] = {
		"DEBUG",
		"WARN ",
		"ERROR"
	};
	
	System_Time time = os_get_system_time();

	va_list args;
	printf("[%02d:%02d:%02d][%s] ", time.hour, time.minute, time.second, LOG_LEVEL_STRING[level]);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

int main() {
	Hardware_Time start = os_get_hardware_time();
	log(LOG_Debug, "Initialization World View...");

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

	create_tile(&app, &app.root, Bounding_Box{ -90, -180, 90, 180 });
	subdivide_tile(&app, &app.root);
	subdivide_tile(&app, app.root.children[0]);

	Hardware_Time end = os_get_hardware_time();
	log(LOG_Debug, "Initialization complete (%fms). Presenting...", os_convert_hardware_time(end - start, Milliseconds));

	Hardware_Time last_info_dump = os_get_hardware_time();

	while(!app.window.should_close) {
		Hardware_Time frame_begin = os_get_hardware_time();
		s64 temp_mark = mark_temp_allocator();

		do_one_frame(&app);
		draw_one_frame(&app);

		if(os_convert_hardware_time(frame_begin - last_info_dump, Seconds) > 5) {
			log(LOG_Debug, " - Internal: %fmb, Temp: %fmb, OS: %fmb, Frame: %fs", convert_to_memory_unit(app.allocator.stats.working_set, Megabytes), convert_to_memory_unit(mark_temp_allocator(), Megabytes), convert_to_memory_unit(os_get_working_set_size(), Megabytes), app.window.frame_time);
			last_info_dump = frame_begin;
		}

		release_temp_allocator(temp_mark);
		
		Hardware_Time frame_end = os_get_hardware_time();
		os_sleep_to_tick_rate(frame_begin, frame_end, FRAME_RATE);
	}

	destroy_tile(&app, &app.root, true);

	destroy_draw_data(&app);
	destroy_window(&app.window);
	app.pool.destroy();
	destroy_temp_allocator();

	return 0;
}
