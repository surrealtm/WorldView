#pragma once

#define FRAME_RATE 60

// --- Foundation
#include <window.h>
#include <math/v3.h>
#include <math/m4.h>

// --- App
#include "tile.h"

enum Map_Mode {
	MAP_MODE_2D,
};

struct Camera {
	f32 fov, near, far, ratio;
	v3f position, rotation;
	m4f projection_view;
};

struct App {
	Window window;
	Camera camera;
	Map_Mode map_mode;

	Resizable_Array<Tile> tiles;
};
