#pragma once

// ============================================================
// === DISPLAY CONFIGURATION FOR ROO_DISPLAY ===
// ============================================================

// --- PIN DEFINITIONS ---
// SPI Pins (ESP32 default VSPI)
#define TFT_MISO  19
#define TFT_MOSI  23
#define TFT_SCLK  18

// Display Control Pins
#define TFT_CS    15
#define TFT_DC    21
#define TFT_RST   4
#define TFT_BL    22

// --- DISPLAY DIMENSIONS ---
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320

// --- LAYOUT CONSTANTS ---
// Album art layout (two-pane)
#define LEFT_PANE_WIDTH   240
#define RIGHT_PANE_X      240
#define STATUS_BAR_Y      280
#define PROGRESS_BAR_Y    276
#define PROGRESS_BAR_H    4

// Text positions
#define TRACK_TITLE_Y     0
#define TRACK_TITLE_H     90
#define ARTIST_Y          90
#define ARTIST_H          70
#define ALBUM_Y           160
#define ALBUM_H           120

// Status bar
#define STATUS_BAR_HEIGHT 40
#define TIME_X            10
#define TIME_Y            290
#define PLAY_ICON_X       230
#define PLAY_ICON_Y       288
#define DEVICE_INFO_X     300
#define DEVICE_INFO_Y     290
