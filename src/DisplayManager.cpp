#include "DisplayManager.h"

// ============================================================
// === CONSTRUCTOR ===
// ============================================================
#if defined(USE_M5_LIBRARY)
DisplayManager::DisplayManager() {
    // M5Stack manages display internally
}
#else
DisplayManager::DisplayManager()
    : device_(),
      display_(device_) {
}
#endif

// ============================================================
// === INITIALIZATION ===
// ============================================================
void DisplayManager::init() {
#if defined(USE_M5_LIBRARY)
    // M5Stack initialization - display is already initialized by M5.begin()
    // Called in main setup(), so just configure display here
    M5.Lcd.setRotation(SCREEN_ROTATION);
    M5.Lcd.fillScreen(TFT_BLACK);
#else
    // Initialize SPI bus for roo_display
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);

    // Initialize the display
    display_.init(Colors::Black);

    // Set landscape orientation (equivalent to rotation 1)
    display_.setOrientation(Orientation().rotateRight());
#endif
}

void DisplayManager::hardwareReset() {
#if defined(USE_M5_LIBRARY)
    // M5Stack handles hardware reset internally
#else
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);
    delay(100);
    digitalWrite(TFT_RST, LOW);
    delay(100);
    digitalWrite(TFT_RST, HIGH);
    delay(200);
#endif
}

void DisplayManager::setBacklight(bool on) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.setBrightness(on ? 255 : 0);
#else
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, on ? HIGH : LOW);
#endif
}

// ============================================================
// === BASIC DRAWING ===
// ============================================================
void DisplayManager::clear(DisplayColor bg) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillScreen(bg);
#else
    DrawingContext dc(display_);
    dc.draw(FilledRect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, bg));
#endif
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, DisplayColor color) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillRect(x, y, w, h, color);
#else
    DrawingContext dc(display_);
    dc.draw(FilledRect(x, y, x + w - 1, y + h - 1, color));
#endif
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, DisplayColor color) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.drawRect(x, y, w, h, color);
#else
    DrawingContext dc(display_);
    dc.draw(Rect(x, y, x + w - 1, y + h - 1, color));
#endif
}

// ============================================================
// === TEXT RENDERING ===
// ============================================================
#if defined(USE_M5_LIBRARY)
void DisplayManager::drawText(const char* text, int16_t x, int16_t y,
                               uint8_t size, DisplayColor color) {
    M5.Lcd.setTextColor(color);
    M5.Lcd.setTextSize(size);
    M5.Lcd.setCursor(x, y);
    M5.Lcd.print(text);
}

void DisplayManager::drawTextInRegion(const char* text, int16_t x, int16_t y,
                                       int16_t w, int16_t h,
                                       uint8_t size, DisplayColor color, DisplayColor bgColor) {
    // Fill background
    M5.Lcd.fillRect(x, y, w, h, bgColor);
    // Draw text
    M5.Lcd.setTextColor(color, bgColor);
    M5.Lcd.setTextSize(size);
    M5.Lcd.setCursor(x, y);
    M5.Lcd.print(text);
}
#else
void DisplayManager::drawText(const char* text, int16_t x, int16_t y,
                               const Font& font, DisplayColor color) {
    DrawingContext dc(display_);
    dc.draw(TextLabel(text, font, color), x, y);
}

void DisplayManager::drawTextInRegion(const char* text, int16_t x, int16_t y,
                                       int16_t w, int16_t h,
                                       const Font& font, DisplayColor color, DisplayColor bgColor) {
    DrawingContext dc(display_);

    // First fill the background region
    dc.draw(FilledRect(x, y, x + w - 1, y + h - 1, bgColor));

    // Set clip box to constrain text
    dc.setClipBox(Box(x, y, x + w - 1, y + h - 1));

    // Draw text within the clipped region
    dc.draw(TextLabel(text, font, color), x, y);
}
#endif

// ============================================================
// === UI COMPONENTS ===
// ============================================================
void DisplayManager::drawProgressBar(int progress, int duration, int16_t y,
                                      int16_t height, DisplayColor activeColor,
                                      DisplayColor inactiveColor) {
    if (duration <= 0) return;

    int barWidth = (progress * SCREEN_WIDTH) / duration;
    if (barWidth < 0) barWidth = 0;
    if (barWidth > SCREEN_WIDTH) barWidth = SCREEN_WIDTH;

#if defined(USE_M5_LIBRARY)
    // Active (played) portion
    if (barWidth > 0) {
        M5.Lcd.fillRect(0, y, barWidth, height, activeColor);
    }
    // Inactive (remaining) portion
    if (barWidth < SCREEN_WIDTH) {
        M5.Lcd.fillRect(barWidth, y, SCREEN_WIDTH - barWidth, height, inactiveColor);
    }
#else
    DrawingContext dc(display_);

    // Active (played) portion
    if (barWidth > 0) {
        dc.draw(FilledRect(0, y, barWidth - 1, y + height - 1, activeColor));
    }

    // Inactive (remaining) portion
    if (barWidth < SCREEN_WIDTH) {
        dc.draw(FilledRect(barWidth, y, SCREEN_WIDTH - 1, y + height - 1, inactiveColor));
    }
#endif
}

void DisplayManager::drawPlayIcon(int16_t x, int16_t y, DisplayColor color) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillTriangle(x, y, x, y + 16, x + 15, y + 8, color);
#else
    DrawingContext dc(display_);
    // Play triangle pointing right - cast arithmetic to avoid narrowing warnings
    dc.draw(FilledTriangle({x, y},
                           {x, static_cast<int16_t>(y + 16)},
                           {static_cast<int16_t>(x + 15), static_cast<int16_t>(y + 8)}, color));
#endif
}

void DisplayManager::drawPauseIcon(int16_t x, int16_t y, DisplayColor color) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillRect(x, y, 5, 16, color);
    M5.Lcd.fillRect(x + 10, y, 5, 16, color);
#else
    DrawingContext dc(display_);
    // Two vertical bars
    dc.draw(FilledRect(x, y, x + 4, y + 15, color));
    dc.draw(FilledRect(x + 10, y, x + 14, y + 15, color));
#endif
}

void DisplayManager::drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                                   int16_t x3, int16_t y3, DisplayColor color) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillTriangle(x1, y1, x2, y2, x3, y3, color);
#else
    DrawingContext dc(display_);
    dc.draw(FilledTriangle({x1, y1}, {x2, y2}, {x3, y3}, color));
#endif
}

// ============================================================
// === POPUP/MODAL ===
// ============================================================
void DisplayManager::showPopup(const char* text, DisplayColor textColor, DisplayColor bgColor) {
    int16_t boxW = (SCREEN_WIDTH > 320) ? 300 : 200;
    int16_t boxH = (SCREEN_HEIGHT > 240) ? 100 : 60;
    int16_t boxX = (SCREEN_WIDTH - boxW) / 2;
    int16_t boxY = (SCREEN_HEIGHT - boxH) / 2;

#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillRect(boxX, boxY, boxW, boxH, bgColor);
    M5.Lcd.drawRect(boxX, boxY, boxW, boxH, TFT_BLACK);
    M5.Lcd.setTextColor(textColor);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(boxX + 20, boxY + 20);
    M5.Lcd.print(text);
#else
    DrawingContext dc(display_);

    // White background box
    dc.draw(FilledRect(boxX, boxY, boxX + boxW - 1, boxY + boxH - 1, bgColor));

    // Black border
    dc.draw(Rect(boxX, boxY, boxX + boxW - 1, boxY + boxH - 1, Colors::Black));

    // Centered text (approximate centering)
    int16_t textX = boxX + 40;
    int16_t textY = boxY + 40;
    dc.draw(TextLabel(text, fontMedium(), textColor), textX, textY);
#endif
}

// ============================================================
// === QR CODE SUPPORT ===
// ============================================================
void DisplayManager::drawQRModule(int16_t x, int16_t y, int16_t size, DisplayColor color) {
#if defined(USE_M5_LIBRARY)
    M5.Lcd.fillRect(x, y, size, size, color);
#else
    DrawingContext dc(display_);
    dc.draw(FilledRect(x, y, x + size - 1, y + size - 1, color));
#endif
}

// ============================================================
// === IMAGE RENDERING ===
// ============================================================
#if FEATURE_ALBUM_ART
void DisplayManager::pushImage(int16_t x, int16_t y, int16_t w, int16_t h,
                                const uint16_t* data) {
#if defined(USE_M5_LIBRARY)
    // M5Stack has direct pushImage support
    M5.Lcd.pushImage(x, y, w, h, data);
#else
    // JPEGDEC callback provides RGB565 big-endian data in MCU blocks
    // (typically 8x8 or 16x16 pixels per callback)
    //
    // TODO: Optimize using roo_display's batch pixel operations or
    // raster rendering for better performance. Current implementation
    // draws pixel-by-pixel which works but is slower.

    DrawingContext dc(display_);

    for (int16_t row = 0; row < h; row++) {
        for (int16_t col = 0; col < w; col++) {
            uint16_t pixel = data[row * w + col];
            // Convert RGB565 big-endian to Color
            // RGB565: RRRRRGGGGGGBBBBB
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            // Draw single pixel as 1x1 rect
            dc.draw(FilledRect(x + col, y + row, x + col, y + row, Color(r, g, b)));
        }
    }
#endif
}
#endif  // FEATURE_ALBUM_ART
