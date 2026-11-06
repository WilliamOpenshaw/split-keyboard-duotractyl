#pragma once
#include QMK_KEYBOARD_H       // defines SAFE_RANGE
#include "quantum/via.h"      // defines USER00…USER99

#define DYNAMIC_KEYMAP_LAYER_COUNT 4
#define DYNAMIC_KEYMAP_CUSTOM_COUNT 4

enum via_custom_keycodes {
    CPISCROLLDOWN = USER00,   // Scroll sensitivity down
    CPISCROLLUP,              // Scroll sensitivity up
    CPICURSORUP,              // Cursor sensitivity up
    CPICURSORDOWN,            // Cursor sensitivity down
    VIA_KEY_LAST = CPICURSORDOWN
};