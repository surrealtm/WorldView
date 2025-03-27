#pragma once

#define FRAME_RATE 60

// --- Foundation
#include <window.h>

// --- App
#include "draw.h"

struct App {
	Window window;
    Renderer renderer;
};
