// --- Foundation
#include <d3d11_layer.h>
#include <math/m4.h>

// --- App
#include "app.h"
#include "draw.h"

Shader_Input_Specification TILE_SHADER_INPUTS[] = {
    { "POSITION", 3, 0 },
    { "UV", 2, 1 },
};

struct Tile_Shader_Constants {
    m4f projection_view;
};

struct Render_Data {
    Frame_Buffer *default_fbo;
    Frame_Buffer tile_fbo;
    Shader tile_shader;
    Shader_Constant_Buffer tile_constants_buffer;
};

Render_Data render_data;

static
void maybe_report_error(Error_Code error) {
    if(error != Success) {
        string _string = error_string(error);
        printf("[ERROR]: %.*s\n", (u32) _string.count, _string.data);
    }
}

void setup_draw_data(App *app) {
    Error_Code error;
    
    create_d3d11_context(&app->window, false);
    error = create_shader_from_file(&render_data.tile_shader, "data/tile.hlsl"_s, TILE_SHADER_INPUTS, ARRAY_COUNT(TILE_SHADER_INPUTS));
    maybe_report_error(error);

    create_shader_constant_buffer(&render_data.tile_constants_buffer, sizeof(Tile_Shader_Constants));
    
	render_data.default_fbo = get_default_frame_buffer(&app->window);
    create_frame_buffer(&render_data.tile_fbo, 1);
    create_frame_buffer_color_attachment(&render_data.tile_fbo, TILE_TEXTURE_RESOLUTION, TILE_TEXTURE_RESOLUTION);
}

void destroy_draw_data(App *app) {
    destroy_shader_constant_buffer(&render_data.tile_constants_buffer);
    destroy_shader(&render_data.tile_shader);
    destroy_frame_buffer(&render_data.tile_fbo);
    destroy_d3d11_context(&app->window);
}



void draw_one_frame(App *app) {
    //
    // Redraw all required tiles
    //
    for(s64 i = 0; i < app->tiles.count; ++i) {
        Tile *tile = &app->tiles[i];
        if(!tile->redraw_texture) continue;

        bind_frame_buffer(&render_data.tile_fbo);
        clear_frame_buffer(&render_data.tile_fbo, 255 / 255.0f, 0 / 255.0f, 0 / 255.0f);
        blit_frame_buffer((Texture *) tile->texture, &render_data.tile_fbo);

        tile->redraw_texture = false;
    }

    //
    // Draw all tiles
    //
    Tile_Shader_Constants tile_shader_constants;
    tile_shader_constants.projection_view = app->camera.projection_view;
    update_shader_constant_buffer(&render_data.tile_constants_buffer, &tile_shader_constants);
    bind_frame_buffer(render_data.default_fbo);
    clear_frame_buffer(render_data.default_fbo, 50 / 255.0f, 96 / 255.0f, 140 / 255.0f);
    bind_shader_constant_buffer(&render_data.tile_constants_buffer, 0, SHADER_Vertex);
    bind_shader(&render_data.tile_shader);

    for(s64 i = 0; i < app->tiles.count; ++i) {
        Tile *tile = &app->tiles[i];

        bind_texture((Texture *) tile->texture, 0);
        bind_vertex_buffer_array((Vertex_Buffer_Array *) tile->mesh);
        draw_vertex_buffer_array((Vertex_Buffer_Array *) tile->mesh);
    }

	swap_d3d11_buffers(&app->window);
}



G_Handle create_texture(u8 *pixels, s64 width, s64 height, s64 channels) {
    Texture *texture = Default_Allocator->New<Texture>();
    Error_Code error = create_texture_from_memory(texture, pixels, (s32) width, (s32) height, (u8) channels, TEXTURE_FILTER_Linear | TEXTURE_WRAP_Edge);
    foundation_assert(error == Success);
    return texture;
}

void destroy_texture(G_Handle handle) {
    destroy_texture((Texture *) handle);
}

G_Handle create_mesh(f32 *positions, f32 *uvs, s64 vertex_count) {
    Vertex_Buffer_Array *mesh = Default_Allocator->New<Vertex_Buffer_Array>();
    create_vertex_buffer_array(mesh, VERTEX_BUFFER_Triangles);
    add_vertex_data(mesh, positions, vertex_count * 3, 3, false);
    add_vertex_data(mesh, uvs, vertex_count * 2, 2, false);
    return mesh;
}

void destroy_mesh(G_Handle handle) {
    destroy_vertex_buffer_array((Vertex_Buffer_Array *) handle);
}
