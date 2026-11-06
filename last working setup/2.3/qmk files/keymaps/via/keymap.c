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
    
    // Initialize the report
    report_mouse_t combined_report = {0};

    // 1. Combine Buttons: Keep button presses from BOTH sides
    combined_report.buttons = left_report.buttons | right_report.buttons;

    // --- CURSOR LOGIC (using left_report) ---
    // Note: User confirmed left_report corresponds to the Right Hand/Mouse trackball
    
    // Static variables to store the remainder of the division
    static int16_t x_remainder = 0;
    static int16_t y_remainder = 0;

    // Handle edge case: PMW3360 sensor overflow bug
    // If ball reports -128 on x axis for left movement while moving right, treat as max right movement
    if (left_report.x == -128) 
    {
        left_report.x = 127; // Correct the input data to max positive
    } 

    // Add previous remainder to current reading BEFORE division
    int16_t total_x = left_report.x + x_remainder;
    int16_t total_y = left_report.y + y_remainder;

    // Calculate output (Integer Division)
    combined_report.x = total_x / mouseSensitivity;
    combined_report.y = total_y / mouseSensitivity;

    // Store the new remainder for the next cycle
    // e.g. If you moved 5 counts and sensitivity is 2: 
    // Output = 2, Remainder = 1. That 1 is saved for the next loop.
    x_remainder = total_x % mouseSensitivity;
    y_remainder = total_y % mouseSensitivity;


    // --- SCROLL LOGIC (using right_report) ---
    // Note: User confirmed right_report corresponds to the Left Hand/Scroll trackball

    scrollTallyX += right_report.x;
    scrollTallyY += right_report.y;

    // Horizontal Scroll Calculation
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

    // Vertical Scroll Calculation
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

