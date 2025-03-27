#include "app.h"
#include "draw.h"

void create_renderer(App *app) {
    create_d3d11_context(&app->window, false);
	app->renderer.fbo = get_default_frame_buffer(&app->window);
}

void destroy_renderer(App *app) {
    destroy_d3d11_context(&app->window);
}



void draw_one_frame(App *app) {
    clear_frame_buffer(app->renderer.fbo, 50 / 255.0f, 96 / 255.0f, 140 / 255.0f);
	swap_d3d11_buffers(&app->window);
}
