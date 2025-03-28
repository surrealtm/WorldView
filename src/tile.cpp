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
    return { (f32) (lon / 90.0), (f32) (lat / -90.0), 0 };
}

static
Vertices create_tile_vertices(Map_Mode map_mode, Bounding_Box box) {
    Vertices result;

    switch(map_mode) {
    case MAP_MODE_2D: {
        result.count     = 6;
        result.positions = (v3f *) Default_Allocator->allocate(result.count * sizeof(v3f));
        result.uvs       = (v2f *) Default_Allocator->allocate(result.count * sizeof(v2f));

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

static
void destroy_tile_vertices(Vertices *vertices) {
    Default_Allocator->deallocate(vertices->positions);
    Default_Allocator->deallocate(vertices->uvs);
    vertices->positions = null;
    vertices->uvs = null;
    vertices->count = 0;
}

void create_tile(Tile *tile, Map_Mode map_mode, Bounding_Box box) {
    Vertices vertices = create_tile_vertices(map_mode, box);

    tile->texture = create_texture(null, TILE_TEXTURE_RESOLUTION, TILE_TEXTURE_RESOLUTION, 0);
    tile->mesh    = create_mesh(vertices.positions[0].values, vertices.uvs[0].values, vertices.count);

    destroy_tile_vertices(&vertices);
}

void destroy_tile(Tile *tile) {
    destroy_texture(tile->texture);
    destroy_mesh(tile->mesh);
    tile->texture = null;
    tile->mesh = null;
}