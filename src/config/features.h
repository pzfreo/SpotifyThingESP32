#pragma once

// ============================================================
// === FEATURE FLAGS ===
// ============================================================
// These can be overridden via build_flags in platformio.ini
// e.g., -DFEATURE_ALBUM_ART=0

// --- Album Art Display ---
// Requires PSRAM for image buffering
#ifndef FEATURE_ALBUM_ART
    #define FEATURE_ALBUM_ART 1
#endif

// --- PSRAM Support ---
// Enable for devices with external PSRAM
#ifndef FEATURE_PSRAM
    #define FEATURE_PSRAM 1
#endif

// --- QR Code Display ---
// For WiFi setup screen
#ifndef FEATURE_QRCODE
    #define FEATURE_QRCODE 1
#endif

// --- Volume Bar Display ---
#ifndef FEATURE_VOLUME_BAR
    #define FEATURE_VOLUME_BAR 1
#endif

// --- Progress Bar Display ---
#ifndef FEATURE_PROGRESS_BAR
    #define FEATURE_PROGRESS_BAR 1
#endif

// --- Hardware Buttons ---
// Physical buttons for control (vs touch)
#ifndef FEATURE_HARDWARE_BUTTONS
    #define FEATURE_HARDWARE_BUTTONS 1
#endif

// --- Rotary Encoder ---
// For volume control
#ifndef FEATURE_ROTARY_ENCODER
    #define FEATURE_ROTARY_ENCODER 0
#endif

// --- Derived Features ---
// Album art requires PSRAM due to memory requirements
#if FEATURE_ALBUM_ART && !FEATURE_PSRAM
    #undef FEATURE_ALBUM_ART
    #define FEATURE_ALBUM_ART 0
    #warning "Album art disabled: requires PSRAM"
#endif
