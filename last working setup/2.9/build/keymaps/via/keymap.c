#include QMK_KEYBOARD_H

enum custom_keycodes {  CPISCROLLDOWN = SAFE_RANGE, // Scroll sensitivity down 
                        CPISCROLLUP, // Scroll sensitivity up 
                        CPICURSORUP, // Cursor sensitivity up 
                        CPICURSORDOWN };

static uint8_t mouseSensitivity = 2;  // default scale (1:1)
static uint8_t scrollSensitivity = 2;
static int16_t scrollTallyX = 0;  // int16 so fast flicks can't overflow and flip scroll direction
static int16_t scrollTallyY = 0;

// ============================================================================
// DIAGNOSTIC LOGGING
// Only active when CONSOLE_ENABLE = yes in rules.mk. View it with the
// "HID Console" in QMK Toolbox, or by running `qmk console` in QMK MSYS.
// Only the half plugged into USB (the right half) can send log lines.
//
// Every 5 seconds it prints a status line, and it prints events right away
// when the link to the left half drops or the main loop stalls.
// It does NOT log which keys you type, only how many were pressed per half.
// ============================================================================
#ifdef CONSOLE_ENABLE
#    include "split_util.h"  // is_transport_connected()
#    define DUO_LOG(...) uprintf(__VA_ARGS__)

#    define DUO_STATUS_INTERVAL_MS 5000
#    define DUO_STALL_WARN_MS 5   // report any main-loop pause longer than this

static uint32_t duo_loops        = 0;  // main loop passes in this window
static uint32_t duo_max_gap_ms   = 0;  // longest pause between loop passes
static uint32_t duo_link_drops   = 0;  // total since power-on
static uint16_t duo_keys_left    = 0;  // key presses in this window
static uint16_t duo_keys_right   = 0;
static uint16_t duo_ball_left    = 0;  // reports with movement in this window
static uint16_t duo_ball_right   = 0;

void housekeeping_task_user(void) {
    if (!is_keyboard_master()) return;  // the left half has no USB to log to

    static uint32_t last_loop_time   = 0;
    static uint32_t last_status_time = 0;
    static uint32_t link_lost_time   = 0;
    static bool     link_was_ok      = true;

    uint32_t now = timer_read32();

    // Main loop timing: long gaps here = keys and cursor feel laggy
    if (last_loop_time != 0) {
        uint32_t gap = TIMER_DIFF_32(now, last_loop_time);
        if (gap > duo_max_gap_ms) duo_max_gap_ms = gap;
        if (gap > DUO_STALL_WARN_MS) {
            DUO_LOG("[DUO] t=%lums STALL: main loop paused %lums\n", now, gap);
        }
    }
    last_loop_time = now;
    duo_loops++;

    // Split link state changes
    bool link_ok = is_transport_connected();
    if (link_ok != link_was_ok) {
        if (!link_ok) {
            duo_link_drops++;
            link_lost_time = now;
            DUO_LOG("[DUO] t=%lums LINK LOST to left half (drop #%lu)\n", now, duo_link_drops);
        } else {
            DUO_LOG("[DUO] t=%lums LINK RESTORED after %lums\n", now, TIMER_DIFF_32(now, link_lost_time));
        }
        link_was_ok = link_ok;
    }

    // Periodic status line
    uint32_t window = TIMER_DIFF_32(now, last_status_time);
    if (window >= DUO_STATUS_INTERVAL_MS) {
        DUO_LOG("[DUO] t=%lus side=%s link=%s drops=%lu loops/s=%lu maxgap=%lums keysL=%u keysR=%u ballL=%u ballR=%u cursorDiv=%u scrollSens=%u\n",
                now / 1000,
                is_keyboard_left() ? "LEFT(wrong!)" : "RIGHT",
                link_ok ? "OK" : "DOWN",
                duo_link_drops,
                duo_loops * 1000 / window,
                duo_max_gap_ms,
                duo_keys_left, duo_keys_right,
                duo_ball_left, duo_ball_right,
                mouseSensitivity, scrollSensitivity);
        duo_loops = 0;
        duo_max_gap_ms = 0;
        duo_keys_left = duo_keys_right = 0;
        duo_ball_left = duo_ball_right = 0;
        last_status_time = now;
    }
}
#else
#    define DUO_LOG(...) do {} while (0)
#endif

void keyboard_post_init_user(void) {
    debug_enable      = false;
    debug_mouse       = false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef CONSOLE_ENABLE
    // Count presses per half (rows 0-4 = left half, 5-9 = right half)
    if (record->event.pressed) {
        if (record->event.key.row < MATRIX_ROWS / 2) duo_keys_left++;
        else                                        duo_keys_right++;
    }
#endif
    if (!record->event.pressed) return true;
    switch (keycode) {
        case CPICURSORDOWN:
            if (mouseSensitivity > 1) mouseSensitivity--;
            DUO_LOG("[DUO] cursor divider -> %u\n", mouseSensitivity);
            return false;
        case CPICURSORUP:
            if (mouseSensitivity < 10) mouseSensitivity++;
            DUO_LOG("[DUO] cursor divider -> %u\n", mouseSensitivity);
            return false;
        case CPISCROLLDOWN:
            if (scrollSensitivity > 1) scrollSensitivity--;
            DUO_LOG("[DUO] scroll sensitivity -> %u\n", scrollSensitivity);
            return false;
        case CPISCROLLUP:
            if (scrollSensitivity < 40) scrollSensitivity++;
            DUO_LOG("[DUO] scroll sensitivity -> %u\n", scrollSensitivity);
            return false;
    }
    return true;
}

// Combine reports from both pointing devices
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {    
    
#ifdef CONSOLE_ENABLE
    if (left_report.x || left_report.y) duo_ball_left++;
    if (right_report.x || right_report.y) duo_ball_right++;
#endif

    // Initialize the report
    report_mouse_t combined_report = {0};

    // 1. Combine Buttons: Keep button presses from BOTH sides
    combined_report.buttons = left_report.buttons | right_report.buttons;

    // --- CURSOR LOGIC (using right_report = right-hand trackball) ---

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


    // --- SCROLL LOGIC (using left_report = left-hand trackball) ---

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

