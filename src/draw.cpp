// --- Foundation
#include <d3d11_layer.h>
#include <math/v2.h>
#include <math/v4.h>
#include <math/m4.h>

// --- App
#include "app.h"
#include "draw.h"

#define IMM2D_BATCH_SIZE 512
#define REDRAW_TILES_EVERY_FRAME true

Shader_Input_Specification TILE_SHADER_INPUTS[] = {
    { "POSITION", 3, 0 },
    { "UV", 2, 1 },
};

Shader_Input_Specification IMM2D_SHADER_INPUTS[] = {
    { "POSITION", 2, 0 },
    { "COLOR", 4, 1 },
};

struct Tile_Shader_Constants {
    m4f projection_view;
};

struct Render_Data {
    Frame_Buffer *default_fbo;

    // Rendering the tiles on screen
    Shader world_shader;
    Shader_Constant_Buffer world_constants_buffer;

    // Rendering the tile textures
    Frame_Buffer imm2d_fbo;
    Shader imm2d_shader;
    Vertex_Buffer_Array imm2d_mesh;
    v2f *imm2d_positions/*[IMM2D_BATCH_SIZE]*/;
    v4f *imm2d_colors/*[IMM2D_BATCH_SIZE]*/;
    s64 imm2d_count;
};

Render_Data render_data;

static
void maybe_report_error(Error_Code error) {
    if(error != Success) {
        string _string = error_string(error);
        foundation_error("%.*s", (u32) _string.count, _string.data);
    }
}

void setup_draw_data(App *app) {
    Error_Code error;
    
    create_d3d11_context(&app->window, false);
    error = create_shader_from_file(&render_data.world_shader, "data/world.hlsl"_s, TILE_SHADER_INPUTS, ARRAY_COUNT(TILE_SHADER_INPUTS));
    maybe_report_error(error);

    create_shader_constant_buffer(&render_data.world_constants_buffer, sizeof(Tile_Shader_Constants));
    
	render_data.default_fbo = get_default_frame_buffer(&app->window);
    create_frame_buffer(&render_data.imm2d_fbo, 1);
    create_frame_buffer_color_attachment(&render_data.imm2d_fbo, TILE_TEXTURE_RESOLUTION, TILE_TEXTURE_RESOLUTION);

    error = create_shader_from_file(&render_data.imm2d_shader, "data/imm2d.hlsl"_s, IMM2D_SHADER_INPUTS, ARRAY_COUNT(IMM2D_SHADER_INPUTS));
    maybe_report_error(error);

    create_vertex_buffer_array(&render_data.imm2d_mesh, VERTEX_BUFFER_Triangles);
    allocate_vertex_data(&render_data.imm2d_mesh, IMM2D_BATCH_SIZE * 2, 2);
    allocate_vertex_data(&render_data.imm2d_mesh, IMM2D_BATCH_SIZE * 4, 4);

    render_data.imm2d_positions = (v2f *) app->allocator.allocate(IMM2D_BATCH_SIZE * sizeof(v2f));
    render_data.imm2d_colors    = (v4f *) app->allocator.allocate(IMM2D_BATCH_SIZE * sizeof(v4f));
    render_data.imm2d_count     = 0;
}

void destroy_draw_data(App *app) {
    app->allocator.deallocate(render_data.imm2d_positions);
    app->allocator.deallocate(render_data.imm2d_colors);

    destroy_vertex_buffer_array(&render_data.imm2d_mesh);
    destroy_shader(&render_data.imm2d_shader);
    destroy_shader_constant_buffer(&render_data.world_constants_buffer);
    destroy_shader(&render_data.world_shader);
    destroy_frame_buffer(&render_data.imm2d_fbo);
    destroy_d3d11_context(&app->window);
}



static inline
v4f color_from_coordinate(const Coordinate &coord) {
    return v4f{ (f32) ((coord.lon + 180.0) / 360.0), (f32) ((coord.lat + 90.0) / 180.0), 0.0, 1.0 };
}

static inline
v2f tile_from_coordinate_space(Tile *tile, const Coordinate &coord) {
    return v2f(
        (f32) ((coord.lon - tile->box.lon0) / (tile->box.lon1 - tile->box.lon0) * 2.0 - 1.0),
        (f32) (1.0 - (coord.lat - tile->box.lat0) / (tile->box.lat1 - tile->box.lat0) * 2.0));
}

static
void flush_imm2d() {
    update_vertex_data(&render_data.imm2d_mesh, 0, render_data.imm2d_positions[0].values, render_data.imm2d_count * 2);
    update_vertex_data(&render_data.imm2d_mesh, 1, render_data.imm2d_colors[0].values, render_data.imm2d_count * 4);

    bind_shader(&render_data.imm2d_shader);
    bind_vertex_buffer_array(&render_data.imm2d_mesh);
    draw_vertex_buffer_array(&render_data.imm2d_mesh);

    render_data.imm2d_count = 0;
}

static
void imm2d_tile_space(const v2f &p0, const v2f &p1, const v2f &p2, const v4f &c0, const v4f &c1, const v4f &c2) {
    if(render_data.imm2d_count + 3 > IMM2D_BATCH_SIZE) flush_imm2d();

    render_data.imm2d_positions[render_data.imm2d_count + 0] = p0;
    render_data.imm2d_positions[render_data.imm2d_count + 1] = p1;
    render_data.imm2d_positions[render_data.imm2d_count + 2] = p2;

    render_data.imm2d_colors[render_data.imm2d_count + 0] = c0;
    render_data.imm2d_colors[render_data.imm2d_count + 1] = c1;
    render_data.imm2d_colors[render_data.imm2d_count + 2] = c2;

    render_data.imm2d_count += 3;
}

static
void imm2d_tile_space(const v2f &p0, const v2f &p1, const v2f &p2, const v4f &color) {
    if(render_data.imm2d_count + 3 > IMM2D_BATCH_SIZE) flush_imm2d();

    render_data.imm2d_positions[render_data.imm2d_count + 0] = p0;
    render_data.imm2d_positions[render_data.imm2d_count + 1] = p1;
    render_data.imm2d_positions[render_data.imm2d_count + 2] = p2;

    render_data.imm2d_colors[render_data.imm2d_count + 0] = color;
    render_data.imm2d_colors[render_data.imm2d_count + 1] = color;
    render_data.imm2d_colors[render_data.imm2d_count + 2] = color;

    render_data.imm2d_count += 3;
}

static inline
void imm2d_coordinate_space(Tile *tile, const Coordinate &p0, const Coordinate &p1, const Coordinate &p2, const v4f &color) {
    imm2d_tile_space(tile_from_coordinate_space(tile, p0), tile_from_coordinate_space(tile, p1), tile_from_coordinate_space(tile, p2), color);
}

static inline
void imm2d_coordinate_space(Tile *tile, const Coordinate &p0, const Coordinate &p1, const Coordinate &p2, const v4f &c0, const v4f &c1, const v4f &c2) {
    imm2d_tile_space(tile_from_coordinate_space(tile, p0), tile_from_coordinate_space(tile, p1), tile_from_coordinate_space(tile, p2), c0, c1, c2);
}

static
void maybe_repaint_tiles(Tile *tile) {
    if((REDRAW_TILES_EVERY_FRAME && tile->leaf)|| tile->state == TILE_Requires_Repainting) {
        bind_frame_buffer(&render_data.imm2d_fbo);
        clear_frame_buffer(&render_data.imm2d_fbo, 255 / 255.0f, 0 / 255.0f, 0 / 255.0f);
        
        Coordinate p0{ tile->box.lat0, tile->box.lon0 };
        Coordinate p1{ tile->box.lat0, tile->box.lon1 };
        Coordinate p2{ tile->box.lat1, tile->box.lon0 };
        Coordinate p3{ tile->box.lat1, tile->box.lon1 };

        v4f c0 = color_from_coordinate(p0);
        v4f c1 = color_from_coordinate(p1);
        v4f c2 = color_from_coordinate(p2);
        v4f c3 = color_from_coordinate(p3);

        imm2d_coordinate_space(tile, p0, p1, p2, c0, c1, c2);
        imm2d_coordinate_space(tile, p1, p3, p2, c1, c3, c2);
        flush_imm2d();
        
        blit_frame_buffer((Texture *) tile->texture, &render_data.imm2d_fbo);

        tile->state = TILE_Valid;
    }

    if(!tile->leaf) {
        for(s64 i = 0; i < ARRAY_COUNT(tile->children); ++i) {
            maybe_repaint_tiles(tile->children[i]);
        }
    }
}

static
void draw_tiles(Tile *tile) {
    if(tile->leaf) {
        bind_texture((Texture *) tile->texture, 0);
        bind_vertex_buffer_array((Vertex_Buffer_Array *) tile->mesh);
        draw_vertex_buffer_array((Vertex_Buffer_Array *) tile->mesh);
    } else {
        for(s64 i = 0; i < ARRAY_COUNT(tile->children); ++i) {
            draw_tiles(tile->children[i]);
        }
    }
}

void draw_one_frame(App *app) {
    //
    // Redraw all required tiles
    //
    maybe_repaint_tiles(&app->root);

    //
    // Draw all tiles
    //
    Tile_Shader_Constants tile_shader_constants;
    tile_shader_constants.projection_view = app->camera.projection_view;
    update_shader_constant_buffer(&render_data.world_constants_buffer, &tile_shader_constants);
    bind_frame_buffer(render_data.default_fbo);
    clear_frame_buffer(render_data.default_fbo, 50 / 255.0f, 96 / 255.0f, 140 / 255.0f);
    bind_shader_constant_buffer(&render_data.world_constants_buffer, 0, SHADER_Vertex);
    bind_shader(&render_data.world_shader);

    draw_tiles(&app->root);

	swap_d3d11_buffers(&app->window);
}



G_Handle create_texture(App *app, u8 *pixels, s64 width, s64 height, s64 channels) {
    Texture *texture = app->allocator.New<Texture>();

    //
    // Choosing the appropriate texture filter here wilmight be a bit tough.
    // When having interpolated data across the entire sphere, we'd get visible seems with linear interpolation
    //   because the texture doesn't know to interpolate to the nearest pixel of the other texture obviously.
    // However, nearest interpolation doesn't look all that great either...
    //
    Error_Code error = create_texture_from_memory(texture, pixels, (s32) width, (s32) height, (u8) channels, TEXTURE_FILTER_Nearest | TEXTURE_WRAP_Edge);
    maybe_report_error(error);
    return texture;
}

G_Handle create_empty_texture(App *app, s64 width, s64 height, s64 channels) {
    return create_texture(app, null, width, height, channels);
}

void destroy_texture(App *app, G_Handle handle) {
    destroy_texture((Texture *) handle);
    app->allocator.deallocate(handle);
}

G_Handle create_mesh(App *app, f32 *positions, f32 *uvs, s64 vertex_count) {
    Vertex_Buffer_Array *mesh = app->allocator.New<Vertex_Buffer_Array>();
    create_vertex_buffer_array(mesh, VERTEX_BUFFER_Triangles);
    add_vertex_data(mesh, positions, vertex_count * 3, 3, false);
    add_vertex_data(mesh, uvs, vertex_count * 2, 2, false);
    return mesh;
}

void destroy_mesh(App *app, G_Handle handle) {
    destroy_vertex_buffer_array((Vertex_Buffer_Array *) handle);
    app->allocator.deallocate(handle);
}
