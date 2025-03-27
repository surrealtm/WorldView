#pragma once

#include <d3d11_layer.h>
#include <math/m4.h>

struct App;

struct Renderer {
    Frame_Buffer *fbo;
    m4f projection;
    m4f view;
};

void create_renderer(App *app);
void destroy_renderer(App *app);

void draw_one_frame(App *app);
