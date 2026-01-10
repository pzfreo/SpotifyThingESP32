#pragma once

// ============================================================
// === DEVICE TEMPLATE ===
// ============================================================
// Copy this file and rename for new devices.
// Update platformio.ini with a new [env:xxx] section.
//
// Required steps:
// 1. Copy this file to devices/my_device.h
// 2. Define DEVICE_MY_DEVICE in platformio.ini build_flags
// 3. Add #elif in device_config.h to include your file
// 4. Set all required defines below

#define DEVICE_NAME "MyDevice"

// ============================================================
// DISPLAY CONFIGURATION (Required)
// ============================================================

// --- Choose ONE display driver ---
// #define DISPLAY_DRIVER_ILI9488  1   // 480x320 TFT
// #define DISPLAY_DRIVER_ILI9341  1   // 320x240 TFT
// #define DISPLAY_DRIVER_M5STACK  1   // M5Stack integrated
// #define DISPLAY_DRIVER_ST7789   1   // 240x240 or 320x240

// --- Display library ---
// #define USE_ROO_DISPLAY         1   // Use roo_display library
// #define USE_M5_LIBRARY          1   // Use M5Stack library
// #define USE_TFT_ESPI            1   // Use TFT_eSPI library

// --- Display Dimensions (Required) ---
#define SCREEN_WIDTH            320
#define SCREEN_HEIGHT           240
#define SCREEN_ROTATION         1   // 0-3

// ============================================================
// PIN DEFINITIONS
// ============================================================

// --- SPI Pins (if using external display) ---
// #define TFT_MISO                19
// #define TFT_MOSI                23
// #define TFT_SCLK                18

// --- Display Control Pins (if using external display) ---
// #define TFT_CS                  15
// #define TFT_DC                  21
// #define TFT_RST                 4
// #define TFT_BL                  22

// --- Button Pins (Required) ---
#define BTN_VOL_UP_PIN          0   // Change to your pin
#define BTN_VOL_DOWN_PIN        0   // Change to your pin
#define BTN_PLAY_PAUSE_PIN      0   // Change to your pin

// ============================================================
// LAYOUT CONFIGURATION
// ============================================================

// --- Choose ONE layout ---
// #define LAYOUT_TWO_PANE         1   // Album art + info side by side
// #define LAYOUT_SINGLE_PANE      1   // Info only, no album art

// --- Album Art (if LAYOUT_TWO_PANE) ---
// #define ALBUM_ART_X             0
// #define ALBUM_ART_Y             0
// #define ALBUM_ART_SIZE          200

// --- Info Pane (Required) ---
#define INFO_PANE_X             0
#define INFO_PANE_Y             0
#define INFO_PANE_WIDTH         SCREEN_WIDTH
#define INFO_PANE_HEIGHT        (SCREEN_HEIGHT - 40)

// --- Text Positions (Required) ---
#define TRACK_TITLE_Y           10
#define TRACK_TITLE_H           60
#define ARTIST_Y                70
#define ARTIST_H                50
#define ALBUM_Y                 120
#define ALBUM_H                 40

// --- Status Bar (Required) ---
#define STATUS_BAR_Y            (SCREEN_HEIGHT - 40)
#define STATUS_BAR_HEIGHT       40
#define PROGRESS_BAR_Y          (STATUS_BAR_Y - 4)
#define PROGRESS_BAR_H          4

// --- Status Bar Elements (Required) ---
#define TIME_X                  10
#define TIME_Y                  (STATUS_BAR_Y + 10)
#define PLAY_ICON_X             (SCREEN_WIDTH / 2)
#define PLAY_ICON_Y             (STATUS_BAR_Y + 8)
#define DEVICE_INFO_X           (SCREEN_WIDTH - 100)
#define DEVICE_INFO_Y           (STATUS_BAR_Y + 10)

// ============================================================
// FEATURE OVERRIDES
// ============================================================
// Uncomment to override defaults from features.h

// #define FEATURE_PSRAM           0   // No external PSRAM
// #define FEATURE_ALBUM_ART       0   // Disable album art
// #define FEATURE_QRCODE          1   // Enable QR code
// #define FEATURE_VOLUME_BAR      1   // Enable volume display
// #define FEATURE_PROGRESS_BAR    1   // Enable progress bar
// #define FEATURE_HARDWARE_BUTTONS 1  // Physical buttons
// #define FEATURE_ROTARY_ENCODER  0   // Rotary encoder for volume
// #define FEATURE_SPEAKER         0   // Audio feedback
