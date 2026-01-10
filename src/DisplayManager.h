#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "roo_display.h"
#include "roo_display/driver/ili9488.h"
#include "roo_display/color/color.h"
#include "roo_display/shape/basic.h"
#include "roo_display/shape/point.h"
#include "roo_display/ui/text_label.h"
#include "roo_display/font/font.h"
#include "roo_display/core/orientation.h"

#include "display_config.h"

using namespace roo_display;

// ============================================================
// === COLOR DEFINITIONS ===
// ============================================================
namespace Colors {
    constexpr Color Black   = color::Black;
    constexpr Color White   = color::White;
    constexpr Color Red     = color::Red;
    constexpr Color Green   = color::Green;
    constexpr Color Blue    = color::Blue;
    constexpr Color Cyan    = color::Cyan;
    constexpr Color Magenta = color::Magenta;
    constexpr Color Orange  = color::Orange;
    constexpr Color Grey    = Color(0x42, 0x42, 0x42);
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
    void clear(Color bg = Colors::Black);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color);

    // Text rendering
    void drawText(const char* text, int16_t x, int16_t y, const Font& font, Color color);
    void drawTextInRegion(const char* text, int16_t x, int16_t y, int16_t w, int16_t h,
                          const Font& font, Color color, Color bgColor = Colors::Black);

    // UI Components
    void drawProgressBar(int progress, int duration, int16_t y, int16_t height,
                         Color activeColor, Color inactiveColor);
    void drawPlayIcon(int16_t x, int16_t y, Color color);
    void drawPauseIcon(int16_t x, int16_t y, Color color);
    void drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                      int16_t x3, int16_t y3, Color color);

    // Popup/Modal
    void showPopup(const char* text, Color textColor, Color bgColor = Colors::White);

    // QR Code support
    void drawQRModule(int16_t x, int16_t y, int16_t size, Color color);

    // Image rendering (JPEG callback support)
    void pushImage(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data);

    // Get underlying display for advanced operations
    Display& getDisplay() { return display_; }

private:
    // ILI9488 driver with CS=15, DC=21, RST=4
    Ili9488spi<TFT_CS, TFT_DC, TFT_RST> device_;
    Display display_;
};

// ============================================================
// === FONTS FROM ROO_FONTS LIBRARY ===
// ============================================================
// Using roo_fonts library for anti-aliased fonts
// Font sizes approximate to TFT_eSPI sizes:
// Size 1 (~12px) - Small text for device info
// Size 2 (~18px) - Medium text for artist/album
// Size 3 (~27px) - Large text for track title

#include "roo_fonts/NotoSans_Regular/12.h"
#include "roo_fonts/NotoSans_Regular/18.h"
#include "roo_fonts/NotoSans_Bold/27.h"

// Font accessor functions
inline const Font& fontSmall() { return font_NotoSans_Regular_12(); }
inline const Font& fontMedium() { return font_NotoSans_Regular_18(); }
inline const Font& fontLarge() { return font_NotoSans_Bold_27(); }
