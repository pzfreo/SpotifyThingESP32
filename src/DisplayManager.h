#pragma once

#include <Arduino.h>
#include <SPI.h>

// Include device configuration (selects device-specific settings)
#include "config/device_config.h"

// ============================================================
// === DISPLAY DRIVER INCLUDES ===
// ============================================================
// Use roo_display for all displays with custom anti-aliased fonts
#include "roo_display.h"
#include "roo_display/color/color.h"
#include "roo_display/shape/basic.h"
#include "roo_display/shape/point.h"
#include "roo_display/ui/text_label.h"
#include "roo_display/font/font.h"
#include "roo_display/core/orientation.h"

#if defined(DISPLAY_DRIVER_ILI9488)
    #include "roo_display/driver/ili9488.h"
#elif defined(DISPLAY_DRIVER_ILI9341)
    #include "roo_display/driver/ili9341.h"
#endif

using namespace roo_display;

// ============================================================
// === COLOR DEFINITIONS ===
// ============================================================
// roo_display Color type (used by all devices)
typedef Color DisplayColor;
namespace Colors {
    constexpr DisplayColor Black   = color::Black;
    constexpr DisplayColor White   = color::White;
    constexpr DisplayColor Red     = color::Red;
    constexpr DisplayColor Green   = color::Green;
    constexpr DisplayColor Blue    = color::Blue;
    constexpr DisplayColor Cyan    = color::Cyan;
    constexpr DisplayColor Magenta = color::Magenta;
    constexpr DisplayColor Orange  = color::Orange;
    constexpr DisplayColor Grey    = Color(0x42, 0x42, 0x42);
}

// ============================================================
// === DISPLAY MANAGER CLASS ===
// ============================================================
class DisplayManager {
public:
    DisplayManager();

    // Initialization
    void init();
    void hardwareReset();
    void setBacklight(bool on);

    // Basic drawing operations
    void clear(DisplayColor bg = Colors::Black);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, DisplayColor color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, DisplayColor color);

    // Text rendering (using roo_fonts for all devices)
    void drawText(const char* text, int16_t x, int16_t y, const Font& font, DisplayColor color);
    void drawTextInRegion(const char* text, int16_t x, int16_t y, int16_t w, int16_t h,
                          const Font& font, DisplayColor color, DisplayColor bgColor = Colors::Black);

    // UI Components
    void drawProgressBar(int progress, int duration, int16_t y, int16_t height,
                         DisplayColor activeColor, DisplayColor inactiveColor);
    void drawPlayIcon(int16_t x, int16_t y, DisplayColor color);
    void drawPauseIcon(int16_t x, int16_t y, DisplayColor color);
    void drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      int16_t x3, int16_t y3, DisplayColor color);

    // Popup/Modal
    void showPopup(const char* text, DisplayColor textColor, DisplayColor bgColor = Colors::White);

    // QR Code support
    void drawQRModule(int16_t x, int16_t y, int16_t size, DisplayColor color);

#if FEATURE_ALBUM_ART
    // Image rendering (JPEG callback support) - only for devices with album art
    void pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);
#endif

    // Get underlying roo_display for advanced operations
    Display& getDisplay() { return display_; }

private:
#if defined(DISPLAY_DRIVER_ILI9488)
    // ILI9488 driver with configurable pins
    Ili9488spi<TFT_CS, TFT_DC, TFT_RST> device_;
    Display display_;
#elif defined(DISPLAY_DRIVER_ILI9341)
    // ILI9341 driver with configurable pins (M5Stack Core)
    Ili9341spi<TFT_CS, TFT_DC, TFT_RST> device_;
    Display display_;
#endif
};

// ============================================================
// === FONTS ===
// ============================================================
// Using roo_fonts library for anti-aliased fonts on all devices
// Reduced to 3 fonts to minimize flash usage
// Available families: NotoSans, NotoSerif, NotoSansMono
// Available weights: Regular, Bold, Italic, BoldItalic, Condensed, CondensedBold, CondensedItalic
// Available sizes: 8, 10, 12, 15, 18, 27, 40, 60, 90

#include "roo_fonts/NotoSans_Regular/12.h"
#include "roo_fonts/NotoSans_Regular/15.h"
#include "roo_fonts/NotoSans_Bold/18.h"

// Small: status text, device info, timestamps
inline const Font& fontSmall() { return font_NotoSans_Regular_12(); }

// Medium: album name, general info, artist names
inline const Font& fontMedium() { return font_NotoSans_Regular_15(); }

// Large: track titles, prominent text (18pt bold saves flash vs 27pt/40pt)
inline const Font& fontLarge() { return font_NotoSans_Bold_18(); }
