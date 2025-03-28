#pragma once

struct App;
typedef void *G_Handle; // Graphics Handle

void setup_draw_data(App *app);
void destroy_draw_data(App *app);

void draw_one_frame(App *app);

G_Handle create_texture(u8 *pixels, s64 width, s64 height, s64 channels);
void destroy_texture(G_Handle handle);
G_Handle create_mesh(f32 *positions, f32 *uvs, s64 count);
void destroy_mesh(G_Handle handle);
