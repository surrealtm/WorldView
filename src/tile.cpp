#include <math/maths.h>
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
v3f d2_world_from_coordinate_space(f64 lat, f64 lon) {
    return { (f32) (lon / 90.0) * WORLD_SCALE_2D, (f32) (lat / 90.0) * WORLD_SCALE_2D, 0 };
}

static
v3f d3_world_from_coordinate_space(f64 lat, f64 lon) {
    f64 theta = degrees_to_radians(lon);
    f64 sigma = degrees_to_radians(lat);
    return { (f32) (sin(theta) * cos(sigma) * WORLD_SCALE_3D), (f32) (sin(sigma) * WORLD_SCALE_3D), (f32) (cos(theta) * cos(sigma) * WORLD_SCALE_3D) };
}

static
Vertices create_tile_vertices(Map_Mode map_mode, Bounding_Box box) {
    Vertices result;

    switch(map_mode) {
    case MAP_MODE_2D: {
        result.count     = 6;
        result.positions = (v3f *) temp.allocate(result.count * sizeof(v3f));
        result.uvs       = (v2f *) temp.allocate(result.count * sizeof(v2f));

        v3f p0, p1, p2, p3;
        p0 = d2_world_from_coordinate_space(box.lat0, box.lon0);
        p1 = d2_world_from_coordinate_space(box.lat0, box.lon1);
        p2 = d2_world_from_coordinate_space(box.lat1, box.lon0);
        p3 = d2_world_from_coordinate_space(box.lat1, box.lon1);

        result.positions[0] = p0;
        result.positions[1] = p2;
        result.positions[2] = p1;

        result.positions[3] = p1;
        result.positions[4] = p2;
        result.positions[5] = p3;

        result.uvs[0] = v2f(0, 0);
        result.uvs[1] = v2f(0, 1);
        result.uvs[2] = v2f(1, 0);

        result.uvs[3] = v2f(1, 0);
        result.uvs[4] = v2f(0, 1);
        result.uvs[5] = v2f(1, 1);
    } break;

    case MAP_MODE_3D: {
        const s64 SEGMENTS = 32;

        result.count     = SEGMENTS * SEGMENTS * 6;
        result.positions = (v3f *) temp.allocate(result.count * sizeof(v3f));
        result.uvs       = (v2f *) temp.allocate(result.count * sizeof(v2f));

        for(s64 i0 = 0; i0 < SEGMENTS; ++i0) {
            s64 i1 = i0 + 1;

            f64 t0 = (f64) i0 / (f64) SEGMENTS;
            f64 t1 = (f64) i1 / (f64) SEGMENTS;

            for(s64 j0 = 0; j0 < SEGMENTS; ++j0) {
                s64 j1 = j0 + 1;

                s64 idx = (i0 * SEGMENTS + j0) * 6;

                f64 u0 = (f64) j0 / (f64) SEGMENTS;
                f64 u1 = (f64) j1 / (f64) SEGMENTS;

                f64 lat0 = box.lat0 + t0 * (box.lat1 - box.lat0);
                f64 lat1 = box.lat0 + t1 * (box.lat1 - box.lat0);
                f64 lon0 = box.lon0 + u0 * (box.lon1 - box.lon0);
                f64 lon1 = box.lon0 + u1 * (box.lon1 - box.lon0);

                v3f p0, p1, p2, p3;
                p0 = d3_world_from_coordinate_space(lat0, lon0);
                p1 = d3_world_from_coordinate_space(lat0, lon1);
                p2 = d3_world_from_coordinate_space(lat1, lon0);
                p3 = d3_world_from_coordinate_space(lat1, lon1);

                result.positions[idx + 0] = p0;
                result.positions[idx + 1] = p2;
                result.positions[idx + 2] = p1;

                result.positions[idx + 3] = p1;
                result.positions[idx + 4] = p2;
                result.positions[idx + 5] = p3;

                v2f uv0, uv1, uv2, uv3; // @Incomplete: Adjust these UVS according to the equirectangular projection
                uv0 = v2f((f32) t0, (f32) u0);
                uv1 = v2f((f32) t0, (f32) u1);
                uv2 = v2f((f32) t1, (f32) u0);
                uv3 = v2f((f32) t1, (f32) u1);

                result.uvs[idx + 0] = uv0;
                result.uvs[idx + 1] = uv2;
                result.uvs[idx + 2] = uv1;
                
                result.uvs[idx + 3] = uv1;
                result.uvs[idx + 4] = uv2;
                result.uvs[idx + 5] = uv3;
            }
        }
    } break;
    }

    return result;
}

void create_tile(App *app, Tile *tile, Map_Mode map_mode, Bounding_Box box) {
    printf("Creating tile: %f, %f -> %f, %f\n", box.lat0, box.lon0, box.lat1, box.lon1);

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
