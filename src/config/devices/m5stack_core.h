#pragma once

// ============================================================
// === M5Stack Core Basic 2.7 ===
// ============================================================
// Integrated ESP32 device with 320x240 ILI9342C display
// Features: Built-in buttons, speaker, SD card slot
// No PSRAM - album art disabled

#define DEVICE_NAME "M5Stack-Core"

// --- Display Driver ---
#define DISPLAY_DRIVER_M5STACK  1
#define USE_M5_LIBRARY          1

// --- Display Dimensions ---
#define SCREEN_WIDTH            320
#define SCREEN_HEIGHT           240
#define SCREEN_ROTATION         1   // Landscape

// --- M5Stack handles pins internally ---
// No need to define TFT pins - M5 library manages them

// --- Button Mapping ---
// M5Stack has 3 front buttons: A (left), B (center), C (right)
#define BTN_A_PIN               39  // GPIO39 - Button A
#define BTN_B_PIN               38  // GPIO38 - Button B
#define BTN_C_PIN               37  // GPIO37 - Button C

// Map to functions
#define BTN_VOL_DOWN_PIN        BTN_A_PIN
#define BTN_PLAY_PAUSE_PIN      BTN_B_PIN
#define BTN_VOL_UP_PIN          BTN_C_PIN

// --- Layout: Single Pane (No Album Art) ---
#define LAYOUT_SINGLE_PANE      1

// Full screen for track info
#define INFO_PANE_X             0
#define INFO_PANE_Y             0
#define INFO_PANE_WIDTH         320
#define INFO_PANE_HEIGHT        200

// Text positions (centered, larger for readability)
#define TRACK_TITLE_Y           10
#define TRACK_TITLE_H           60
#define ARTIST_Y                70
#define ARTIST_H                50
#define ALBUM_Y                 120
#define ALBUM_H                 40

// Status bar at bottom
#define STATUS_BAR_Y            200
#define STATUS_BAR_HEIGHT       40
#define PROGRESS_BAR_Y          196
#define PROGRESS_BAR_H          4

// Status bar elements
#define TIME_X                  10
#define TIME_Y                  210
#define PLAY_ICON_X             150
#define PLAY_ICON_Y             208
#define DEVICE_INFO_X           200
#define DEVICE_INFO_Y           210

// --- Feature Overrides ---
// M5Stack Core Basic has no PSRAM
#define FEATURE_PSRAM           0
#define FEATURE_ALBUM_ART       0

// M5Stack has built-in speaker for feedback
#define FEATURE_SPEAKER         1
