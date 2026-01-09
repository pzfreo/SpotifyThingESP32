# Conversion Plan: TFT_eSPI to roo_display

## Executive Summary

This document outlines the plan to convert the SpotifyThingESP32 project from **TFT_eSPI** to **roo_display** library. The conversion will enable better anti-aliasing, smoother graphics, enhanced typography, and a more maintainable codebase.

---

## Current Architecture

### Display Stack
- **Library**: TFT_eSPI v2.5.43
- **Display**: ILI9488 480x320 TFT
- **Image Decoding**: JPEGDEC (bitbank2)
- **QR Codes**: QRCode library

### Key Display Features Used
1. Viewport-based clipping (`setViewport`/`resetViewport`)
2. Basic text rendering with size scaling
3. JPEG streaming and callback-based rendering
4. Filled rectangles for UI elements
5. Simple shapes (triangles, rectangles) for icons
6. Progress bar with `fillRect`

---

## Phase 1: Project Setup & Dependencies

### 1.1 Update platformio.ini

**Remove:**
```ini
lib_deps =
    bodmer/TFT_eSPI@^2.5.43
```

**Add:**
```ini
lib_deps =
    https://github.com/dejwk/roo_display.git
    https://github.com/dejwk/roo_smooth_fonts.git  ; For anti-aliased fonts
```

### 1.2 Remove TFT_eSPI Build Flags

**Remove from platformio.ini:**
```ini
build_flags =
    -DUSER_SETUP_LOADED=1
    -DILI9488_DRIVER=1
    -DTFT_MISO=19
    -DTFT_MOSI=23
    ... (all TFT_eSPI flags)
```

### 1.3 Create roo_display Configuration Header

**New file: `src/display_config.h`**
```cpp
#pragma once

#include "roo_display.h"
#include "roo_display/driver/ili9488.h"

// Pin definitions (same as current project)
#define TFT_CS    15
#define TFT_DC    21
#define TFT_RST   4
#define TFT_BL    22

// SPI pins
#define TFT_MOSI  23
#define TFT_MISO  19
#define TFT_SCLK  18

// Display dimensions
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320

// Create device driver
using DisplayDevice = roo_display::Ili9488spi<TFT_CS, TFT_DC, TFT_RST>;
```

---

## Phase 2: Core Display Initialization

### 2.1 Replace TFT Object

**Current (TFT_eSPI):**
```cpp
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

void setup() {
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
}
```

**New (roo_display):**
```cpp
#include "roo_display.h"
#include "roo_display/driver/ili9488.h"

using namespace roo_display;

Ili9488spi<TFT_CS, TFT_DC, TFT_RST> device(
    Orientation().rotateRight()  // Equivalent to rotation(1)
);
Display display(device);

void setup() {
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
    display.init(color::Black);
}
```

### 2.2 Create Display Abstraction Class

**New file: `src/DisplayManager.h`**
```cpp
#pragma once

#include "roo_display.h"
#include "roo_display/driver/ili9488.h"
#include "roo_display/ui/text_label.h"

using namespace roo_display;

class DisplayManager {
public:
    DisplayManager();
    void init();

    // High-level drawing methods
    void clear(Color bg = color::Black);
    void drawText(const char* text, int x, int y, const Font& font, Color color);
    void drawRect(int x, int y, int w, int h, Color color);
    void fillRect(int x, int y, int w, int h, Color color);
    void drawImage(const uint8_t* data, int x, int y, int w, int h);
    void drawProgressBar(int progress, int max, int y);

    // Viewport/clipping support
    void setClipRegion(int x, int y, int w, int h);
    void resetClipRegion();

    Display& getDisplay() { return display_; }

private:
    Ili9488spi<15, 21, 4> device_;
    Display display_;
    Box currentClip_;
};
```

---

## Phase 3: Color System Migration

### 3.1 Color Constant Mapping

**Current (TFT_eSPI):**
```cpp
#define C_BLACK   TFT_BLACK
#define C_WHITE   TFT_WHITE
#define C_RED     TFT_RED
#define C_GREEN   TFT_GREEN
#define C_CYAN    TFT_CYAN
#define C_GREY    0x4208
```

**New (roo_display):**
```cpp
// roo_display has named colors built-in
namespace Colors {
    constexpr Color Black   = color::Black;
    constexpr Color White   = color::White;
    constexpr Color Red     = color::Red;
    constexpr Color Green   = color::Green;
    constexpr Color Cyan    = color::Cyan;
    constexpr Color Grey    = color::DimGray;    // Or Color(0x42, 0x42, 0x42)
    constexpr Color Orange  = color::Orange;
    constexpr Color Magenta = color::Magenta;
}
```

### 3.2 Custom Colors

```cpp
// For custom RGB565 values like 0x4208
Color customGrey = Color(
    ((0x4208 >> 11) & 0x1F) << 3,  // R
    ((0x4208 >> 5) & 0x3F) << 2,   // G
    (0x4208 & 0x1F) << 3           // B
);
// Or simply: Color(0x42, 0x10, 0x40) for the closest match
```

---

## Phase 4: Text Rendering Migration

### 4.1 Font Selection

**Current (TFT_eSPI):**
```cpp
tft.setTextSize(1);  // Small
tft.setTextSize(2);  // Medium
tft.setTextSize(3);  // Large
```

**New (roo_display):**
```cpp
#include "roo_smooth_fonts/NotoSans_Regular/18.h"
#include "roo_smooth_fonts/NotoSans_Regular/27.h"
#include "roo_smooth_fonts/NotoSans_Bold/40.h"

// Size mapping:
// Size 1 -> font_NotoSans_Regular_18()
// Size 2 -> font_NotoSans_Regular_27()
// Size 3 -> font_NotoSans_Bold_40()
```

### 4.2 Text Drawing

**Current:**
```cpp
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.setTextSize(3);
tft.setCursor(10, 20);
tft.println("Track Name");
```

**New:**
```cpp
DrawingContext dc(display);
dc.setBackgroundColor(color::Black);
dc.draw(
    TextLabel("Track Name", font_NotoSans_Bold_40(), color::White),
    10, 20
);
```

### 4.3 Text Wrapping in Viewports

**Current:**
```cpp
tft.setViewport(0, 90, 240, 70);
tft.setTextWrap(true);
tft.println(artistName);
tft.resetViewport();
```

**New:**
```cpp
DrawingContext dc(display);
dc.setClipBox(Box(0, 90, 239, 159));  // x1, y1, x2, y2
dc.draw(
    TextLabel(artistName, font_NotoSans_Regular_27(), color::Cyan),
    0, 90
);
// Clipping automatically handles overflow
```

For true text wrapping, use `StringViewLabel` with width constraints:
```cpp
#include "roo_display/ui/string_view_label.h"

StringViewLabel label(
    StringView(artistName),
    font_NotoSans_Regular_27(),
    color::Cyan,
    ALIGN_LEFT,
    240  // Max width for wrapping
);
dc.draw(label, 0, 90);
```

---

## Phase 5: Shape and UI Element Migration

### 5.1 Filled Rectangles

**Current:**
```cpp
tft.fillRect(0, 276, barWidth, 4, C_GREEN);
```

**New:**
```cpp
#include "roo_display/shape/basic.h"

DrawingContext dc(display);
dc.draw(FilledRect(0, 276, barWidth - 1, 279, color::Green));
```

### 5.2 Progress Bar

**New implementation:**
```cpp
void drawProgressBar(int progress, int duration) {
    int barWidth = (progress * 480) / duration;

    DrawingContext dc(display);
    // Green played portion
    dc.draw(FilledRect(0, 276, barWidth - 1, 279, color::Green));
    // Grey remaining portion
    dc.draw(FilledRect(barWidth, 276, 479, 279, Colors::Grey));
}
```

### 5.3 Play/Pause Icons

**Current (manual triangle):**
```cpp
// Play triangle
tft.fillTriangle(x, y, x, y+20, x+15, y+10, C_GREEN);

// Pause bars
tft.fillRect(x, y, 6, 20, C_WHITE);
tft.fillRect(x+10, y, 6, 20, C_WHITE);
```

**New (smooth shapes):**
```cpp
#include "roo_display/shape/smooth.h"

// Play triangle with anti-aliasing
SmoothThickTriangle playIcon(
    FpPoint(x, y),
    FpPoint(x, y + 20),
    FpPoint(x + 15, y + 10),
    color::Green
);
dc.draw(playIcon);

// Pause bars
dc.draw(FilledRect(x, y, x + 5, y + 19, color::White));
dc.draw(FilledRect(x + 10, y, x + 15, y + 19, color::White));
```

### 5.4 Popup Messages

**New implementation:**
```cpp
void showPopup(const char* text, Color textColor) {
    DrawingContext dc(display);

    // White background box
    int boxX = (480 - 300) / 2;
    int boxY = (320 - 100) / 2;

    dc.draw(FilledRect(boxX, boxY, boxX + 299, boxY + 99, color::White));
    dc.draw(Rect(boxX, boxY, boxX + 299, boxY + 99, color::Black));  // Border

    // Centered text
    dc.draw(
        TextLabel(text, font_NotoSans_Bold_40(), textColor),
        kCenter | kMiddle  // Alignment
    );
}
```

---

## Phase 6: Image/Album Art Migration

### 6.1 JPEG Handling Options

**Option A: Keep JPEGDEC with roo_display integration**
```cpp
#include <JPEGDEC.h>

// Callback writes to roo_display
int JPEGDraw(JPEGDRAW *pDraw) {
    // Create a raster from the decoded pixels
    Raster<Rgb565> raster(
        pDraw->iWidth,
        pDraw->iHeight,
        (const uint8_t*)pDraw->pPixels,
        Rgb565()
    );

    DrawingContext dc(display);
    dc.draw(raster, pDraw->x, pDraw->y);
    return 1;
}
```

**Option B: Use roo_display's built-in JPEG decoder**
```cpp
#include "roo_display/image/jpeg/jpeg_decoder.h"

void drawAlbumArt(const uint8_t* jpegData, size_t size, int x, int y) {
    JpegDecoder decoder;
    auto image = decoder.decode(jpegData, size);

    if (image.ok()) {
        DrawingContext dc(display);
        dc.draw(*image, x, y);
    }
}
```

### 6.2 Album Art with Scaling

```cpp
#include "roo_display/filter/transformation.h"

void drawScaledAlbumArt(const uint8_t* jpegData, size_t size) {
    JpegDecoder decoder;
    auto image = decoder.decode(jpegData, size);

    if (image.ok()) {
        DrawingContext dc(display);

        // Calculate scale factor to fit 240x320 area
        float scale = min(240.0f / image->width(), 320.0f / image->height());

        dc.setTransformation(Transformation().scale(scale, scale));
        dc.draw(*image, 240, 0);  // Right pane
    }
}
```

---

## Phase 7: Layout System Refactor

### 7.1 Create UI Component Classes

**New file: `src/ui/TrackInfoPanel.h`**
```cpp
#pragma once

#include "roo_display.h"
#include "roo_display/ui/text_label.h"

using namespace roo_display;

class TrackInfoPanel : public Drawable {
public:
    void setTrackName(const char* name);
    void setArtistName(const char* name);
    void setAlbumName(const char* name);

    Box extents() const override;
    void drawTo(const Surface& s) const override;

private:
    String trackName_;
    String artistName_;
    String albumName_;
};
```

### 7.2 Create Status Bar Component

**New file: `src/ui/StatusBar.h`**
```cpp
#pragma once

#include "roo_display.h"

class StatusBar : public Drawable {
public:
    void setPlaying(bool playing);
    void setProgress(int current, int total);
    void setDevice(const char* name);
    void setVolume(int percent);

    Box extents() const override;
    void drawTo(const Surface& s) const override;

private:
    bool isPlaying_ = false;
    int progressMs_ = 0;
    int durationMs_ = 0;
    String deviceName_;
    int volumePercent_ = 0;
};
```

### 7.3 Main Display Composition

```cpp
class MainDisplay {
public:
    void update(const SpotifyState& state) {
        trackPanel_.setTrackName(state.trackName);
        trackPanel_.setArtistName(state.artistName);
        trackPanel_.setAlbumName(state.albumName);

        statusBar_.setPlaying(state.isPlaying);
        statusBar_.setProgress(state.progressMS, state.durationMS);
        statusBar_.setDevice(state.deviceName);
        statusBar_.setVolume(state.volumePercent);

        redraw();
    }

    void redraw() {
        DrawingContext dc(display_);
        dc.draw(trackPanel_, 0, 0);
        dc.draw(albumArt_, 240, 0);
        dc.draw(statusBar_, 0, 280);
    }

private:
    Display& display_;
    TrackInfoPanel trackPanel_;
    StatusBar statusBar_;
    // Album art handled separately
};
```

---

## Phase 8: QR Code Integration

### 8.1 QR Code Rendering

```cpp
#include <QRCode.h>
#include "roo_display/shape/basic.h"

void showQRCode(const char* data, const char* title) {
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, data);

    DrawingContext dc(display);
    dc.draw(FilledRect(0, 0, 479, 319, color::White));  // Clear

    // Draw title
    dc.draw(
        TextLabel(title, font_NotoSans_Bold_40(), color::Black),
        kCenter | kTop.shiftBy(20)
    );

    // Draw QR code pixels
    int scale = 3;
    int offsetX = (480 - qrcode.size * scale) / 2;
    int offsetY = (320 - qrcode.size * scale) / 2;

    for (int y = 0; y < qrcode.size; y++) {
        for (int x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                dc.draw(FilledRect(
                    offsetX + x * scale,
                    offsetY + y * scale,
                    offsetX + (x + 1) * scale - 1,
                    offsetY + (y + 1) * scale - 1,
                    color::Black
                ));
            }
        }
    }
}
```

---

## Phase 9: Anti-Flicker Optimization

### 9.1 Using Offscreen Buffers

For flicker-free updates, use offscreen rendering:

```cpp
#include "roo_display/core/offscreen.h"

// Create offscreen buffer for track info panel (240x280)
Offscreen<Rgb565> trackPanelBuffer(240, 280);

void updateTrackPanel(const SpotifyState& state) {
    // Draw to offscreen buffer
    DrawingContext dc(trackPanelBuffer);
    dc.clear();

    dc.draw(TextLabel(state.trackName, font_NotoSans_Bold_40(), color::White), 10, 10);
    dc.draw(TextLabel(state.artistName, font_NotoSans_Regular_27(), color::Cyan), 10, 60);
    dc.draw(TextLabel(state.albumName, font_NotoSans_Regular_27(), color::White), 10, 100);

    // Blit to display in one operation
    DrawingContext displayDc(display);
    displayDc.draw(trackPanelBuffer, 0, 0);
}
```

### 9.2 Selective Redraw Strategy

```cpp
class SmartRedraw {
public:
    void update(const SpotifyState& newState) {
        bool trackChanged = strcmp(newState.trackName, lastState_.trackName) != 0;
        bool progressChanged = newState.progressMS != lastState_.progressMS;
        bool volumeChanged = newState.volumePercent != lastState_.volumePercent;

        DrawingContext dc(display_);

        if (trackChanged) {
            redrawTrackPanel(dc, newState);
            redrawAlbumArt(newState.imageUrl);
        }

        if (progressChanged) {
            redrawProgressBar(dc, newState);
        }

        if (volumeChanged || trackChanged) {
            redrawStatusBar(dc, newState);
        }

        lastState_ = newState;
    }

private:
    SpotifyState lastState_;
    Display& display_;
};
```

---

## Phase 10: Implementation Steps

### Step-by-Step Migration Order

1. **Setup (Day 1)**
   - [ ] Create feature branch
   - [ ] Update platformio.ini with roo_display dependency
   - [ ] Create display_config.h
   - [ ] Verify compilation with empty display init

2. **Core Display (Day 2)**
   - [ ] Create DisplayManager class
   - [ ] Implement init() with roo_display
   - [ ] Test basic screen clear
   - [ ] Implement color constants

3. **Text Rendering (Day 3)**
   - [ ] Add roo_smooth_fonts dependency
   - [ ] Map font sizes to actual fonts
   - [ ] Implement drawText() method
   - [ ] Test text rendering

4. **Shapes & UI (Day 4)**
   - [ ] Implement fillRect/drawRect
   - [ ] Create progress bar
   - [ ] Create play/pause icons
   - [ ] Test UI elements

5. **Album Art (Day 5)**
   - [ ] Integrate JPEG decoder
   - [ ] Implement scaling
   - [ ] Test image rendering
   - [ ] Handle download + decode pipeline

6. **QR Code (Day 6)**
   - [ ] Port QR code rendering
   - [ ] Test WiFi setup flow
   - [ ] Test Spotify login flow

7. **Anti-Flicker (Day 7)**
   - [ ] Implement offscreen buffers
   - [ ] Add selective redraw logic
   - [ ] Performance testing

8. **Integration & Testing (Day 8-9)**
   - [ ] Full system integration
   - [ ] Memory usage analysis
   - [ ] Performance benchmarks
   - [ ] Bug fixes

9. **Cleanup (Day 10)**
   - [ ] Remove TFT_eSPI references
   - [ ] Code review
   - [ ] Documentation update

---

## Memory Considerations

### Current Memory Usage (TFT_eSPI)
- JPEG Buffer: 60KB
- Task Stack: 32KB
- Misc Buffers: ~5KB

### Projected Memory Usage (roo_display)
- JPEG Buffer: 40-60KB (depending on decoder)
- Offscreen Buffer (240x280 RGB565): ~134KB
- Smooth Fonts: ~20-50KB per font (PROGMEM)
- Task Stack: 32KB

**Recommendation:** Use ESP32's PSRAM for offscreen buffers:
```cpp
Offscreen<Rgb565> buffer(240, 280, ps_malloc);
```

---

## Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Font memory usage | High | Use PROGMEM, limit font variants |
| JPEG decoder compatibility | Medium | Keep JPEGDEC as fallback |
| Anti-aliasing performance | Medium | Profile and optimize hot paths |
| Learning curve | Low | Incremental migration |
| Display driver issues | Low | ILI9488 well-supported |

---

## Benefits After Conversion

1. **Visual Quality**
   - Anti-aliased text
   - Smooth shapes and icons
   - Better color handling with transparency

2. **Maintainability**
   - Component-based UI architecture
   - Clean separation of concerns
   - Reusable drawable classes

3. **Performance**
   - Optimized SPI transport
   - Offscreen buffer compositing
   - Better memory management

4. **Features**
   - Material Icons support (34,000+ icons)
   - Gradient support for backgrounds
   - True alpha blending

---

## References

- [roo_display GitHub](https://github.com/dejwk/roo_display)
- [roo_smooth_fonts](https://github.com/dejwk/roo_smooth_fonts)
- [Programming Guide](https://github.com/dejwk/roo_display/blob/master/doc/programming_guide.md)
