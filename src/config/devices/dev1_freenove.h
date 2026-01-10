#pragma once

// ============================================================
// === DEV1: Freenove ESP32-WROVER with ILI9488 Display ===
// ============================================================
// Custom build with external ILI9488 480x320 TFT display
// Features: PSRAM, album art, hardware buttons

#define DEVICE_NAME "Dev1-Freenove"

// --- Display Driver ---
#define DISPLAY_DRIVER_ILI9488  1
#define USE_ROO_DISPLAY         1

// --- Display Dimensions ---
#define SCREEN_WIDTH            480
#define SCREEN_HEIGHT           320
#define SCREEN_ROTATION         1   // Landscape

// --- SPI Pins (ESP32 VSPI) ---
#define TFT_MISO                19
#define TFT_MOSI                23
#define TFT_SCLK                18

// --- Display Control Pins ---
#define TFT_CS                  15
#define TFT_DC                  21
#define TFT_RST                 4
#define TFT_BL                  22

// --- Button Pins ---
#define BTN_VOL_UP_PIN          13
#define BTN_VOL_DOWN_PIN        12
#define BTN_PLAY_PAUSE_PIN      14

// --- Layout: Two-Pane (Album Art + Info) ---
#define LAYOUT_TWO_PANE         1

// Album art on left
#define ALBUM_ART_X             0
#define ALBUM_ART_Y             0
#define ALBUM_ART_SIZE          240

// Track info on right
#define INFO_PANE_X             240
#define INFO_PANE_Y             0
#define INFO_PANE_WIDTH         240
#define INFO_PANE_HEIGHT        280

// Text positions (relative to info pane)
#define TRACK_TITLE_Y           0
#define TRACK_TITLE_H           90
#define ARTIST_Y                90
#define ARTIST_H                70
#define ALBUM_Y                 160
#define ALBUM_H                 120

// Status bar at bottom
#define STATUS_BAR_Y            280
#define STATUS_BAR_HEIGHT       40
#define PROGRESS_BAR_Y          276
#define PROGRESS_BAR_H          4

// Status bar elements
#define TIME_X                  10
#define TIME_Y                  290
#define PLAY_ICON_X             230
#define PLAY_ICON_Y             288
#define DEVICE_INFO_X           300
#define DEVICE_INFO_Y           290

// --- Feature Overrides ---
// (defaults from features.h are fine for this device)
