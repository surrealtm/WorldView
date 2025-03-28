#pragma once

#include <foundation.h>

#define TILE_TEXTURE_RESOLUTION 512
#define TILE_TEXTURE_CHANNELS 4 // D3D11 doesn't support actual RBG, only RGBA

typedef void *G_Handle;
struct App;
enum Map_Mode;

struct Coordinate {
    f64 lat;
    f64 lon;
};

struct Bounding_Box {
    f64 lat0, lon0;
    f64 lat1, lon1;  
};

struct Tile {
    Bounding_Box box;
    Tile *children[4];

    G_Handle texture;
    G_Handle mesh;

    b8 repaint;
    b8 leaf;
};

void create_tile(App *app, Tile *tile, Map_Mode map_mode, Bounding_Box box);
void destroy_tile(App *app, Tile *tile);
