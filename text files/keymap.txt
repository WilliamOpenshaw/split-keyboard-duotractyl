#include QMK_KEYBOARD_H

enum custom_keycodes {  CPISCROLLDOWN = SAFE_RANGE, // Scroll sensitivity down 
                        CPISCROLLUP, // Scroll sensitivity up 
                        CPICURSORUP, // Cursor sensitivity up 
                        CPICURSORDOWN };

static uint8_t mouseSensitivity = 2;  // default scale (1:1)
static uint8_t scrollSensitivity = 2;
static int8_t scrollTallyX = 0;
static int8_t scrollTallyY = 0;

// 4) Initialize from EEPROM & enable debug
void keyboard_post_init_user(void) {
    debug_enable      = true;
    debug_mouse       = true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;
    switch (keycode) {
        case CPICURSORDOWN: 
            if (mouseSensitivity > 1) mouseSensitivity--;
            printf("Mouse Sensativity decreased to : %d\n", mouseSensitivity);
            return false;
        case CPICURSORUP:
            if (mouseSensitivity < 10) mouseSensitivity++;
            printf("Mouse Sensativity increased to : %d\n", mouseSensitivity);
            return false;
        case CPISCROLLDOWN:
            if (scrollSensitivity > 1) scrollSensitivity--;
            printf("Scroll Sensativity decreased to : %d\n", scrollSensitivity);
            return false;
        case CPISCROLLUP:
            if (scrollSensitivity < 40) scrollSensitivity++;
            printf("Scroll Sensativity increased to : %d\n", scrollSensitivity);
            return false;
    }
    return true;
}

// Combine reports from both pointing devices
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {    
    
    // Start with right trackball for cursor movement
    report_mouse_t combined_report = left_report;

    // cursor movement, rounding away from zero
    
    // Handle edge case
    // If ball reports -128 on x axis for left movement while moving right, we should still go right
    // if combined report x = -128, then combined report x = 0
    if (left_report.x == -128) 
    {
        left_report.x = 128; // treat as max right movement
        combined_report.x = 64; // treat as max right movement
    } 
    else 
    {
        combined_report.x = left_report.x / mouseSensitivity;
    }
    combined_report.y = left_report.y / mouseSensitivity;

    scrollTallyX += right_report.x;
    scrollTallyY += right_report.y;

    if (scrollTallyX >= (120 / scrollSensitivity)) 
    {
        combined_report.h = 1;
        scrollTallyX = 0; // reset after applying
    }
    else if (scrollTallyX <= (-120 / scrollSensitivity)) 
    {
        combined_report.h = -1;
        scrollTallyX = 0; // reset after applying
    } 
    else 
    {
        combined_report.h = 0; // no horizontal scroll
    }

    if (scrollTallyY >= (120 / scrollSensitivity)) 
    {
        combined_report.v = 1;
        scrollTallyY = 0; // reset after applying
    }
    else if (scrollTallyY <= (-120 / scrollSensitivity)) 
    {
        combined_report.v = -1;
        scrollTallyY = 0; // reset after applying
    } 
    else 
    {
        combined_report.v = 0; // no vertical scroll
    }

    // Keep any button presses from both sides
    combined_report.buttons |= right_report.buttons;

    // Debug output for both trackballs
    if (debug_mouse) {
        if (left_report.x != 0 || left_report.y != 0) {
            printf("Left mouse trackball: x=%d, y=%d\n", left_report.x, left_report.y);
            printf("Left After Calc: x=%d, y=%d\n", (combined_report.x), (combined_report.y));
        }
        if (right_report.x != 0 || right_report.y != 0) {
            printf("Right scroll trackball: x=%d, y=%d\n", right_report.x, right_report.y);
            printf("Right After Calc: x=%d, y=%d\n", (combined_report.h), (combined_report.v));
            printf("Scroll Tally: X=%d, Y=%d\n", scrollTallyX, scrollTallyY);
        }
    }
    
    return combined_report;
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = 
{ 
    [0] = LAYOUT( 
        // Left Half - Row 0 (6 keys) 
        KC_ESC, KC_1, KC_2, KC_3, KC_4, KC_5, 
        // Right Half - Row 0 (6 keys) 
        KC_6, KC_7, CPICURSORDOWN, CPICURSORUP, CPISCROLLDOWN, CPISCROLLUP, 
        // Left Half - Row 1 (6 keys) 
        KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, 
        // Right Half - Row 1 (6 keys) 
        KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRC, 
        // Left Half - Row 2 (6 keys) 
        KC_CAPS, KC_A, KC_S, KC_D, KC_F, KC_G, 
        // Right Half - Row 2 (6 keys) 
        KC_H, KC_J, KC_K, KC_L, KC_SCLN, KC_QUOT, 
        // Left Half - Row 3 (7 keys including thumb) 
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_LBRC, 
        // Right Half - Row 3 (7 keys including thumb) 
        KC_RBRC, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT, 
        // Left Half - Row 4 (7 keys - bottom row + thumb cluster)
        KC_LCTL, KC_LGUI, KC_LALT, KC_ESC, QK_BOOT, MO(1), KC_SPC, 
        // Right Half - Row 4 (7 keys - bottom row + thumb cluster) 
        KC_ENT, KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, KC_RALT, KC_RCTL ), 

    [1] = LAYOUT( 
        // Left Half - Row 0 (6 keys) 
        KC_ESC, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, 
        // Right Half - Row 0 (6 keys) 
        KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, 
        // Left Half - Row 1 (6 keys) 
        KC_TAB, KC_EXLM, KC_AT, KC_HASH, KC_DLR, KC_PERC, 
        // Right Half - Row 1 (6 keys) 
        KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_F12, 
        // Left Half - Row 2 (6 keys) 
        KC_CAPS, KC_1, KC_2, KC_3, KC_4, KC_5, 
        // Right Half - Row 2 (6 keys) 
        KC_6, KC_7, KC_8, KC_9, KC_0, KC_QUOT, 
        // Left Half - Row 3 (7 keys including thumb) 
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_LBRC, 
        // Right Half - Row 3 (7 keys including thumb) 
        KC_RBRC, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT, 
        // Left Half - Row 4 (7 keys - bottom row + thumb cluster) 
        KC_LCTL, KC_LGUI, KC_LALT, KC_ESC, QK_BOOT, _______, KC_SPC, 
        // Right Half - Row 4 (7 keys - bottom row + thumb cluster) 
        KC_ENT, KC_HOME, KC_PGDN, KC_PGUP, KC_END, KC_RALT, KC_RCTL ), 
    
    [2] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ ), 


    [3] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______ ) 

};

