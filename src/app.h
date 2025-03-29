#pragma once

#define FRAME_RATE 60

// --- Foundation
#include <window.h>
#include <math/v3.h>
#include <math/m4.h>
#include <memutils.h>

// --- App
#include "tile.h"

#define WORLD_SCALE_2D 100
#define WORLD_SCALE_3D 10

enum Map_Mode {
	MAP_MODE_2D,
	MAP_MODE_3D,
};

struct Camera {
	// Projection
	f32 fov, near, far, ratio;

	// View
	Coordinate current_center, target_center; // The central coordinates that the camera is looking at
	f64 zoom_level;
	f64 current_distance, target_distance;

	// Matrix
	m4f projection;
	m4f view;
	m4f projection_view;
};

struct App {
	Memory_Pool pool;
	Allocator allocator;

	Window window;

	Camera camera;
	Map_Mode map_mode;

	Tile root;
};
