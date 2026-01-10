#include "DisplayManager.h"

// ============================================================
// === CONSTRUCTOR ===
// ============================================================
DisplayManager::DisplayManager()
#if defined(DISPLAY_DRIVER_ILI9341)
    // M5Stack: Initialize with landscape dimensions (320x240)
    : device_(ILI9341_Target(ILI9341_Transport(), SCREEN_WIDTH, SCREEN_HEIGHT)),
      display_(device_) {
}
#else
    : device_(),
      display_(device_) {
}
#endif

// ============================================================
// === INITIALIZATION ===
// ============================================================
void DisplayManager::init() {
    // Initialize SPI bus for roo_display
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);

    // Initialize the display
    display_.init(Colors::Black);

    // Set landscape orientation
#if defined(DISPLAY_DRIVER_ILI9341)
    // M5Stack: Testing different orientations
    // Available: RightDown(0), DownRight(1), LeftDown(2), DownLeft(3),
    //            RightUp(4), UpRight(5), LeftUp(6), UpLeft(7)
    // Try orientation 2: LeftDown (DownRight rotated 90° CCW)
    display_.setOrientation(Orientation::LeftDown());
#else
    // dev1 ILI9488: DownRight landscape (rotateRight from default)
    display_.setOrientation(Orientation().rotateRight());
#endif
}

void DisplayManager::hardwareReset() {
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);
    delay(100);
    digitalWrite(TFT_RST, LOW);
    delay(100);
    digitalWrite(TFT_RST, HIGH);
    delay(200);
}

void DisplayManager::setBacklight(bool on) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, on ? HIGH : LOW);
}

// ============================================================
// === BASIC DRAWING ===
// ============================================================
void DisplayManager::clear(DisplayColor bg) {
    DrawingContext dc(display_);
    dc.draw(FilledRect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, bg));
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, DisplayColor color) {
    DrawingContext dc(display_);
    dc.draw(FilledRect(x, y, x + w - 1, y + h - 1, color));
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, DisplayColor color) {
    DrawingContext dc(display_);
    dc.draw(Rect(x, y, x + w - 1, y + h - 1, color));
}

// ============================================================
// === TEXT RENDERING ===
// ============================================================
void DisplayManager::drawText(const char* text, int16_t x, int16_t y,
                               const Font& font, DisplayColor color) {
    DrawingContext dc(display_);
    dc.setBackgroundColor(Colors::Black);  // Required for smooth font anti-aliasing
    dc.draw(TextLabel(text, font, color), x, y);
}

void DisplayManager::drawTextInRegion(const char* text, int16_t x, int16_t y,
                                       int16_t w, int16_t h,
                                       const Font& font, DisplayColor color, DisplayColor bgColor) {
    DrawingContext dc(display_);
    dc.setBackgroundColor(bgColor);  // Required for smooth font anti-aliasing

    // First fill the background region
    dc.draw(FilledRect(x, y, x + w - 1, y + h - 1, bgColor));

    // Set clip box to constrain text
    dc.setClipBox(Box(x, y, x + w - 1, y + h - 1));

    // Draw text within the clipped region
    dc.draw(TextLabel(text, font, color), x, y);
}

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

    DrawingContext dc(display_);

    // Active (played) portion
    if (barWidth > 0) {
        dc.draw(FilledRect(0, y, barWidth - 1, y + height - 1, activeColor));
    }

    // Inactive (remaining) portion
    if (barWidth < SCREEN_WIDTH) {
        dc.draw(FilledRect(barWidth, y, SCREEN_WIDTH - 1, y + height - 1, inactiveColor));
    }
}

void DisplayManager::drawPlayIcon(int16_t x, int16_t y, DisplayColor color) {
    DrawingContext dc(display_);
    // Play triangle pointing right - cast arithmetic to avoid narrowing warnings
    dc.draw(FilledTriangle({x, y},
                           {x, static_cast<int16_t>(y + 16)},
                           {static_cast<int16_t>(x + 15), static_cast<int16_t>(y + 8)}, color));
}

void DisplayManager::drawPauseIcon(int16_t x, int16_t y, DisplayColor color) {
    DrawingContext dc(display_);
    // Two vertical bars
    dc.draw(FilledRect(x, y, x + 4, y + 15, color));
    dc.draw(FilledRect(x + 10, y, x + 14, y + 15, color));
}

void DisplayManager::drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                                   int16_t x3, int16_t y3, DisplayColor color) {
    DrawingContext dc(display_);
    dc.draw(FilledTriangle({x1, y1}, {x2, y2}, {x3, y3}, color));
}

// ============================================================
// === POPUP/MODAL ===
// ============================================================
void DisplayManager::showPopup(const char* text, DisplayColor textColor, DisplayColor bgColor) {
    int16_t boxW = (SCREEN_WIDTH > 320) ? 300 : 200;
    int16_t boxH = (SCREEN_HEIGHT > 240) ? 100 : 60;
    int16_t boxX = (SCREEN_WIDTH - boxW) / 2;
    int16_t boxY = (SCREEN_HEIGHT - boxH) / 2;

    DrawingContext dc(display_);

    // White background box
    dc.draw(FilledRect(boxX, boxY, boxX + boxW - 1, boxY + boxH - 1, bgColor));

    // Black border
    dc.draw(Rect(boxX, boxY, boxX + boxW - 1, boxY + boxH - 1, Colors::Black));

    // Set background for anti-aliased text
    dc.setBackgroundColor(bgColor);

    // Centered text (approximate centering)
    int16_t textX = boxX + 40;
    int16_t textY = boxY + 40;
    dc.draw(TextLabel(text, fontMedium(), textColor), textX, textY);
}

// ============================================================
// === QR CODE SUPPORT ===
// ============================================================
void DisplayManager::drawQRModule(int16_t x, int16_t y, int16_t size, DisplayColor color) {
    DrawingContext dc(display_);
    dc.draw(FilledRect(x, y, x + size - 1, y + size - 1, color));
}

// ============================================================
// === IMAGE RENDERING ===
// ============================================================
#if FEATURE_ALBUM_ART
void DisplayManager::pushImage(int16_t x, int16_t y, int16_t w, int16_t h,
                                const uint16_t* data) {
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
}
#endif  // FEATURE_ALBUM_ART
