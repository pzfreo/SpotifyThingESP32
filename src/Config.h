#pragma once

// --- SYSTEM SETTINGS ---
#define ENABLE_ALBUM_ART      // Comment out to disable Album Art
#define SPOTIFY_REFRESH_RATE_MS 1000 
#define AP_NAME "SpotifySetup"
#define SLEEP_TIMEOUT_MS 300000 // 5 Minutes

// --- PINS ---
// SPI Pins are defined in platformio.ini for TFT_eSPI
#define TFT_BL     22  // Backlight
#define PIN_PREV   12
#define PIN_PLAY   13
#define PIN_NEXT   14

// --- MEMORY ---
#ifdef ENABLE_ALBUM_ART
#define JPG_BUFFER_SIZE 40000
#endif

// --- AUTH ---
// Changed to #define to avoid linker "undefined reference" errors
#define AUTHKEY "ohsosecret"