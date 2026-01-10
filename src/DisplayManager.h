#pragma once

#include <Arduino.h>
#include <SPI.h>

// Include device configuration (selects device-specific settings)
#include "config/device_config.h"

// ============================================================
// === DISPLAY DRIVER INCLUDES ===
// ============================================================
#if defined(USE_M5_LIBRARY)
    #include <M5Stack.h>
    // M5Stack uses TFT_eSPI internally - we'll wrap it
#else
    // Use roo_display for custom displays
    #include "roo_display.h"
    #include "roo_display/color/color.h"
    #include "roo_display/shape/basic.h"
    #include "roo_display/shape/point.h"
    #include "roo_display/ui/text_label.h"
    #include "roo_display/font/font.h"
    #include "roo_display/core/orientation.h"

    #if defined(DISPLAY_DRIVER_ILI9488)
        #include "roo_display/driver/ili9488.h"
    #endif

    using namespace roo_display;
#endif

// ============================================================
// === COLOR DEFINITIONS ===
// ============================================================
#if defined(USE_M5_LIBRARY)
// M5Stack uses TFT_eSPI colors (16-bit RGB565)
typedef uint16_t DisplayColor;
namespace Colors {
    constexpr DisplayColor Black   = TFT_BLACK;
    constexpr DisplayColor White   = TFT_WHITE;
    constexpr DisplayColor Red     = TFT_RED;
    constexpr DisplayColor Green   = TFT_GREEN;
    constexpr DisplayColor Blue    = TFT_BLUE;
    constexpr DisplayColor Cyan    = TFT_CYAN;
    constexpr DisplayColor Magenta = TFT_MAGENTA;
    constexpr DisplayColor Orange  = TFT_ORANGE;
    constexpr DisplayColor Grey    = 0x4208;  // RGB565 grey
}
#else
// roo_display Color type
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
#endif

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

    // Text rendering
#if defined(USE_M5_LIBRARY)
    void drawText(const char* text, int16_t x, int16_t y, uint8_t size, DisplayColor color);
    void drawTextInRegion(const char* text, int16_t x, int16_t y, int16_t w, int16_t h,
                          uint8_t size, DisplayColor color, DisplayColor bgColor = Colors::Black);
#else
    void drawText(const char* text, int16_t x, int16_t y, const Font& font, DisplayColor color);
    void drawTextInRegion(const char* text, int16_t x, int16_t y, int16_t w, int16_t h,
                          const Font& font, DisplayColor color, DisplayColor bgColor = Colors::Black);
#endif

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

#if !defined(USE_M5_LIBRARY)
    // Get underlying roo_display for advanced operations
    Display& getDisplay() { return display_; }
#endif

private:
#if defined(USE_M5_LIBRARY)
    // M5Stack manages display internally via M5.Lcd
#elif defined(DISPLAY_DRIVER_ILI9488)
    // ILI9488 driver with configurable pins
    Ili9488spi<TFT_CS, TFT_DC, TFT_RST> device_;
    Display display_;
#endif
};

// ============================================================
// === FONTS ===
// ============================================================
#if defined(USE_M5_LIBRARY)
// M5Stack uses numeric font sizes
constexpr uint8_t FONT_SMALL = 1;
constexpr uint8_t FONT_MEDIUM = 2;
constexpr uint8_t FONT_LARGE = 4;
#else
// Using roo_fonts library for anti-aliased fonts
// Available families: NotoSans, NotoSerif, NotoSansMono
// Available weights: Regular, Bold, Italic, BoldItalic, Condensed, CondensedBold, CondensedItalic
// Available sizes: 8, 10, 12, 15, 18, 27, 40, 60, 90

#include "roo_fonts/NotoSans_Regular/12.h"
#include "roo_fonts/NotoSans_Regular/15.h"
#include "roo_fonts/NotoSans_Italic/18.h"
#include "roo_fonts/NotoSans_Bold/27.h"
#include "roo_fonts/NotoSans_Bold/40.h"

// Small: status text, device info, timestamps
inline const Font& fontSmall() { return font_NotoSans_Regular_12(); }

// Medium: album name, general info
inline const Font& fontMedium() { return font_NotoSans_Regular_15(); }

// Artist: italic for visual distinction
inline const Font& fontArtist() { return font_NotoSans_Italic_18(); }

// Large: track titles (prominent, bold)
inline const Font& fontLarge() { return font_NotoSans_Bold_27(); }

// Extra large: track title for displays with album art (more space)
inline const Font& fontTitle() { return font_NotoSans_Bold_40(); }
#endif
