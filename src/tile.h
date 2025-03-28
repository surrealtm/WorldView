#pragma once

#include <foundation.h>

#define TILE_TEXTURE_RESOLUTION 512

typedef void *G_Handle;
enum Map_Mode;

struct Bounding_Box {
    f64 lat0, lon0;
    f64 lat1, lon1;  
};

struct Tile {
    G_Handle texture;
    G_Handle mesh;  
};

void create_tile(Tile *tile, Map_Mode map_mode, Bounding_Box box);
void destroy_tile(Tile *tile);
