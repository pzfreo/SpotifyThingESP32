#pragma once

// ============================================================
// === DEVICE CONFIGURATION SELECTOR ===
// ============================================================
// This file selects the appropriate device configuration based
// on build flags defined in platformio.ini.
//
// To add a new device:
// 1. Create a new header in devices/
// 2. Add a new #elif block below
// 3. Add a new [env:xxx] in platformio.ini with -DDEVICE_XXX

// --- Include Feature Flags First ---
// Device configs can override these
#include "features.h"

// --- Select Device Configuration ---
#if defined(DEVICE_DEV1_FREENOVE)
    #include "devices/dev1_freenove.h"

#elif defined(DEVICE_M5STACK_CORE)
    #include "devices/m5stack_core.h"

#else
    // Default to Dev1 for backward compatibility
    #warning "No device specified, defaulting to DEVICE_DEV1_FREENOVE"
    #define DEVICE_DEV1_FREENOVE 1
    #include "devices/dev1_freenove.h"
#endif

// --- Re-include features to apply device overrides ---
// Device headers may have set FEATURE_* defines that need to
// override the defaults
#include "features.h"

// --- Validation ---
#ifndef DEVICE_NAME
    #error "Device configuration must define DEVICE_NAME"
#endif

#ifndef SCREEN_WIDTH
    #error "Device configuration must define SCREEN_WIDTH"
#endif

#ifndef SCREEN_HEIGHT
    #error "Device configuration must define SCREEN_HEIGHT"
#endif

// --- Computed Values ---
#define SCREEN_PIXELS (SCREEN_WIDTH * SCREEN_HEIGHT)

// Layout type detection
#if defined(LAYOUT_TWO_PANE)
    #define HAS_ALBUM_ART_AREA 1
#else
    #define HAS_ALBUM_ART_AREA 0
#endif

// Effective album art support
#if FEATURE_ALBUM_ART && HAS_ALBUM_ART_AREA
    #define SHOW_ALBUM_ART 1
#else
    #define SHOW_ALBUM_ART 0
#endif
