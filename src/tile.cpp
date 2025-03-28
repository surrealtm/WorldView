#include <math/v2.h>
#include <math/v3.h>

#include "tile.h"
#include "app.h"
#include "draw.h"

struct Vertices {
    v3f *positions;
    v2f *uvs;
    s64 count;
};

static
v3f point_to_2d_map(f64 lat, f64 lon) {
    return { (f32) (lon / 90.0) * WORLD_SCALE_2D, (f32) (lat / -90.0) * WORLD_SCALE_2D, 0 };
}

static
Vertices create_tile_vertices(Map_Mode map_mode, Bounding_Box box) {
    Vertices result;

    switch(map_mode) {
    case MAP_MODE_2D: {
        result.count     = 6;
        result.positions = (v3f *) temp.allocate(result.count * sizeof(v3f));
        result.uvs       = (v2f *) temp.allocate(result.count * sizeof(v2f));

        result.positions[0] = point_to_2d_map(box.lat0, box.lon0);
        result.positions[1] = point_to_2d_map(box.lat0, box.lon1);
        result.positions[2] = point_to_2d_map(box.lat1, box.lon0);

        result.positions[3] = point_to_2d_map(box.lat1, box.lon0);
        result.positions[4] = point_to_2d_map(box.lat0, box.lon1);
        result.positions[5] = point_to_2d_map(box.lat1, box.lon1);

        result.uvs[0] = v2f(0, 0);
        result.uvs[1] = v2f(0, 1);
        result.uvs[2] = v2f(1, 0);

        result.uvs[3] = v2f(1, 0);
        result.uvs[4] = v2f(0, 1);
        result.uvs[5] = v2f(1, 1);
    } break;
    }

    return result;
}

void create_tile(App *app, Tile *tile, Map_Mode map_mode, Bounding_Box box) {
    s64 tmp_mark = mark_temp_allocator();

    Vertices vertices = create_tile_vertices(map_mode, box);

    tile->box     = box;
    tile->texture = create_empty_texture(app, TILE_TEXTURE_RESOLUTION, TILE_TEXTURE_RESOLUTION, TILE_TEXTURE_CHANNELS);
    tile->mesh    = create_mesh(app, vertices.positions[0].values, vertices.uvs[0].values, vertices.count);
    tile->repaint = true;
    tile->leaf    = true;

    release_temp_allocator(tmp_mark);
}

void destroy_tile(App *app, Tile *tile) {
    if(!tile->leaf) {
        for(s64 i = 0; i < ARRAY_COUNT(tile->children); ++i) {
            destroy_tile(app, tile->children[i]);
            app->allocator.deallocate(tile->children[i]);
        }
    }

    destroy_texture(app, tile->texture);
    destroy_mesh(app, tile->mesh);
    tile->texture = null;
    tile->mesh = null;
}

void subdivide_tile(App *app, Tile *tile) {
    assert(tile->leaf);
    tile->leaf = false;

    f64 half_lat   = (tile->box.lat1 - tile->box.lat0) * 0.5;
    f64 half_lon   = (tile->box.lon1 - tile->box.lon0) * 0.5;

    for(s64 i = 0; i < ARRAY_COUNT(tile->children); ++i) {
        f64 lat_offset = (f64) (i / 2);
        f64 lon_offset = (f64) (i % 2);

        Bounding_Box child_box = { tile->box.lat0 + half_lat * (lat_offset + 0),
                                   tile->box.lon0 + half_lon * (lon_offset + 0),
                                   tile->box.lat0 + half_lat * (lat_offset + 1),
                                   tile->box.lon0 + half_lon * (lon_offset + 1) };

        tile->children[i] = (Tile *) app->allocator.allocate(sizeof(Tile));
        create_tile(app, tile->children[i], app->map_mode, child_box);
    }
}
