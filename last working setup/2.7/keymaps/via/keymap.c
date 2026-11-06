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
    debug_enable      = false;
    debug_mouse       = false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;
    switch (keycode) {
        case CPICURSORDOWN: 
            if (mouseSensitivity > 1) mouseSensitivity--;
            //printf("Mouse Sensativity decreased to : %d\n", mouseSensitivity);
            return false;
        case CPICURSORUP:
            if (mouseSensitivity < 10) mouseSensitivity++;
            //printf("Mouse Sensativity increased to : %d\n", mouseSensitivity);
            return false;
        case CPISCROLLDOWN:
            if (scrollSensitivity > 1) scrollSensitivity--;
            //printf("Scroll Sensativity decreased to : %d\n", scrollSensitivity);
            return false;
        case CPISCROLLUP:
            if (scrollSensitivity < 40) scrollSensitivity++;
            //printf("Scroll Sensativity increased to : %d\n", scrollSensitivity);
            return false;
    }
    return true;
}

// Combine reports from both pointing devices
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {    
    
    // Initialize the report
    report_mouse_t combined_report = {0};

    // 1. Combine Buttons: Keep button presses from BOTH sides
    combined_report.buttons = left_report.buttons | right_report.buttons;

    // --- CURSOR LOGIC (using left_report) ---
    // User Note: "Cursor needs to be left_report" (Right Hand)

    // Static variables to store the remainder (lost precision) between cycles
    static int16_t x_remainder = 0;
    static int16_t y_remainder = 0;

    // Handle edge case: PMW3360 sensor overflow bug
    // If sensor reports -128 (min int8) it's often a glitch during fast positive swipes.
    // We cap it at 127 (max int8) to feed positive data into the math below.
    if (right_report.x == -128) {
        right_report.x = 127; 
    } 

    // SMOOTHING ALGORITHM:
    // 1. Add the remainder from the LAST cycle to the CURRENT raw reading
    int16_t total_x = right_report.x + x_remainder;
    int16_t total_y = right_report.y + y_remainder;

    // 2. Calculate the output to send to the OS
    combined_report.x = total_x / mouseSensitivity;
    combined_report.y = total_y / mouseSensitivity;

    // 3. Store the NEW remainder for the NEXT cycle
    // (This ensures no movement data is ever lost, just delayed until it adds up to a pixel)
    x_remainder = total_x % mouseSensitivity;
    y_remainder = total_y % mouseSensitivity;


    // --- SCROLL LOGIC (using right_report) ---
    // User Note: "Scrolling needs to be right_report" (Left Hand)

    scrollTallyX += left_report.x;
    scrollTallyY += left_report.y;

    // Horizontal Scroll
    if (scrollTallyX >= (120 / scrollSensitivity)) 
    {
        combined_report.h = 1;
        scrollTallyX = 0; 
    }
    else if (scrollTallyX <= (-120 / scrollSensitivity)) 
    {
        combined_report.h = -1;
        scrollTallyX = 0; 
    } 
    else 
    {
        combined_report.h = 0; 
    }

    // Vertical Scroll
    if (scrollTallyY >= (120 / scrollSensitivity)) 
    {
        combined_report.v = 1;
        scrollTallyY = 0; 
    }
    else if (scrollTallyY <= (-120 / scrollSensitivity)) 
    {
        combined_report.v = -1;
        scrollTallyY = 0; 
    } 
    else 
    {
        combined_report.v = 0; 
    }

    /*
    // --- DEBUGGING ---
    if (debug_mouse) {
        // Debug Cursor (left_report)
        if (left_report.x != 0 || left_report.y != 0) {
            printf("Cursor (Left_Rep): Raw=[%d, %d] | Remainder=[%d, %d] | Output=[%d, %d]\n", 
                   left_report.x, left_report.y, x_remainder, y_remainder, combined_report.x, combined_report.y);
        }
        // Debug Scroll (right_report)
        if (right_report.x != 0 || right_report.y != 0) {
            printf("Scroll (Right_Rep): Raw=[%d, %d] | Tally=[%d, %d] | Output=[%d, %d]\n", 
                   right_report.x, right_report.y, scrollTallyX, scrollTallyY, combined_report.h, combined_report.v);
        }
    }
    */
    
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

