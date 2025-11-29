#pragma once
#include <SPI.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <TFT_eSPI.h> 
#include <QRCode.h>
#include "Config.h"
#include "SharedTypes.h"

#ifdef ENABLE_ALBUM_ART
#include <JPEGDEC.h>
#endif

// --- COLOR MAPPINGS ---
#define ILI9488_BLACK   TFT_BLACK
#define ILI9488_WHITE   TFT_WHITE
#define ILI9488_RED     TFT_RED
#define ILI9488_GREEN   TFT_GREEN
#define ILI9488_BLUE    TFT_BLUE
#define ILI9488_CYAN    TFT_CYAN
#define ILI9488_MAGENTA TFT_MAGENTA
#define ILI9488_YELLOW  TFT_YELLOW
#define ILI9488_ORANGE  TFT_ORANGE
#define ILI9488_DARKGREY 0x4208 

class DisplayManager {
private:
    TFT_eSPI tft;
    char lastTrackName[64];
    char lastDeviceName[64];
    int lastVolume;

    #ifdef ENABLE_ALBUM_ART
    char lastImageUrl[256];
    #endif

public:
    DisplayManager() {
        lastTrackName[0] = '\0';
        lastDeviceName[0] = '\0';
        lastVolume = -1;
        #ifdef ENABLE_ALBUM_ART
        lastImageUrl[0] = '\0';
        #endif
    }

    void init() {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, HIGH);
        
        // Manual Reset for ILI9488 stability
        #ifdef TFT_RST
        pinMode(TFT_RST, OUTPUT);
        digitalWrite(TFT_RST, HIGH); delay(100);
        digitalWrite(TFT_RST, LOW);  delay(100);
        digitalWrite(TFT_RST, HIGH); delay(200);
        #endif

        tft.init();
        tft.setRotation(1); // Landscape (480x320)
        tft.fillScreen(ILI9488_BLACK);
        tft.setTextWrap(false);
    }

    void showSplash() {
        // Startup Color Cycle Test
        tft.fillScreen(TFT_RED);   delay(200);
        tft.fillScreen(TFT_GREEN); delay(200);
        tft.fillScreen(TFT_BLUE);  delay(200);

        tft.fillScreen(ILI9488_BLACK);
        tft.setCursor(10, 50);
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        tft.setTextSize(3);
        tft.println("System Starting...");
    }
    
    void showConnecting() {
        tft.fillScreen(ILI9488_BLACK);
        tft.setCursor(10, 100);
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        tft.setTextSize(2);
        tft.println("Connecting WiFi...");
    }
    
    void showQR(const char* data, const char* title, const char* footer) {
        tft.fillScreen(ILI9488_BLACK);
        tft.setCursor(0, 20);
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        tft.setTextSize(2);
        tft.println(title);
        
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(10)];
        qrcode_initText(&qrcode, qrcodeData, 10, ECC_LOW, data);
    
        int scale = 3; 
        int border = 10;
        int startX = (480 - (qrcode.size * scale)) / 2; // Center for 480 width
        int startY = 60;
    
        tft.fillRect(startX - border, startY - border, (qrcode.size * scale) + (border*2), (qrcode.size * scale) + (border*2), ILI9488_WHITE);
    
        for (uint8_t y = 0; y < qrcode.size; y++) {
            for (uint8_t x = 0; x < qrcode.size; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, ILI9488_BLACK);
                }
            }
        }
        
        tft.setCursor(10, 280);
        tft.setTextColor(ILI9488_GREEN, ILI9488_BLACK);
        tft.println(footer);
    }

    void clearScreen() {
        tft.fillScreen(ILI9488_BLACK);
        // Force full redraw next update
        lastTrackName[0] = '\0';
        lastDeviceName[0] = '\0';
        lastVolume = -1;
        #ifdef ENABLE_ALBUM_ART
        lastImageUrl[0] = '\0';
        #endif
    }
    
    void setBacklight(bool on) {
        digitalWrite(TFT_BL, on ? HIGH : LOW);
    }
    
    void update(SpotifyState& state, JPEG_DRAW_CALLBACK* drawCallback) {
        bool trackChanged = strcmp(state.trackName, lastTrackName) != 0;

        #ifdef ENABLE_ALBUM_ART
        // --- ART LAYOUT (480x320) ---
        // Left: Text (0-240). Right: Art (240-480). Bottom: Status (Y=280).
        
        if (trackChanged) {
            // Clear Left Text Area
            tft.fillRect(0, 0, 240, 280, ILI9488_BLACK);
            strlcpy(lastTrackName, state.trackName, sizeof(lastTrackName));
            
            // USE VIEWPORT TO CONTAIN TEXT WRAPPING
            
            // Track Title (Size 3 - Big)
            tft.setViewport(0, 0, 240, 90);
            tft.setTextWrap(true);
            tft.setCursor(10, 20);
            tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
            tft.setTextSize(3); 
            tft.println(state.trackName);
            tft.resetViewport();
            
            // Artist
            tft.setViewport(0, 90, 240, 70);
            tft.setCursor(10, 10); 
            tft.setTextColor(ILI9488_CYAN, ILI9488_BLACK);
            tft.setTextSize(2);
            tft.println(state.artistName);
            tft.resetViewport();
    
            // Album
            tft.setViewport(0, 160, 240, 120);
            tft.setCursor(10, 0); 
            tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
            tft.setTextSize(2);
            tft.println(state.albumName);
            tft.resetViewport();
    
            tft.setTextWrap(false);
        }

        // Album Art Logic (Managed by Main to handle network/callback)
        // Check if URL changed in Main and call drawAlbumArt
        
        // Progress Bar
        int barWidth = map(state.progressMS, 0, state.durationMS, 0, 480);
        tft.fillRect(0, 276, 480, 4, ILI9488_DARKGREY); // BG
        tft.fillRect(0, 276, barWidth, 4, 0x1DB9); // Active
        
        // Status Bar Background
        if (trackChanged) tft.fillRect(0, 280, 480, 40, ILI9488_BLACK);

        tft.setTextSize(2);
        tft.setCursor(10, 290);
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        int curMin = state.progressMS / 60000;
        int curSec = (state.progressMS / 1000) % 60;
        tft.printf("%02d:%02d", curMin, curSec);

        tft.setCursor(230, 290);
        if(state.isPlaying) tft.print(" || "); else tft.print("  >  ");

        tft.setCursor(300, 290);
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        tft.print(state.deviceName);

        #else
        // --- TEXT ONLY LAYOUT ---
        if (trackChanged) {
            tft.fillRect(0, 0, 480, 200, ILI9488_BLACK); 
            strlcpy(lastTrackName, state.trackName, sizeof(lastTrackName));
            
            tft.setTextWrap(true);
            
            // Track
            tft.setViewport(0, 0, 480, 90);
            tft.setCursor(20, 20);
            tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
            tft.setTextSize(3); 
            tft.println(state.trackName);
            tft.resetViewport();
            
            // Artist
            tft.setViewport(0, 90, 480, 70);
            tft.setCursor(20, 10); 
            tft.setTextColor(ILI9488_CYAN, ILI9488_BLACK);
            tft.setTextSize(2);
            tft.println(state.artistName);
            tft.resetViewport();
            
            // Album
            tft.setViewport(0, 160, 480, 40);
            tft.setCursor(20, 0); 
            tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
            tft.setTextSize(2);
            tft.println(state.albumName);
            tft.resetViewport();
            tft.setTextWrap(false); 
        }
        
        // Progress Bar
        if (state.durationMS > 0) {
            int width = map(state.progressMS, 0, state.durationMS, 0, 440);
            tft.fillRect(20, 220, 440, 10, ILI9488_DARKGREY);
            tft.fillRect(20, 220, width, 10, 0x1DB9); 
        }
        
        // Time Text
        tft.setCursor(20, 240);
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        tft.setTextSize(2);
        int curMin = state.progressMS / 60000;
        int curSec = (state.progressMS / 1000) % 60;
        tft.printf("%02d:%02d", curMin, curSec);

        // Play Icon
        tft.setCursor(400, 240); 
        tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
        if(state.isPlaying) tft.print(" > "); else tft.print(" || ");
        
        // Status
        if (strcmp(state.deviceName, lastDeviceName) != 0 || state.volumePercent != lastVolume) {
            strlcpy(lastDeviceName, state.deviceName, sizeof(lastDeviceName));
            lastVolume = state.volumePercent;
            tft.fillRect(0, 270, 480, 20, ILI9488_BLACK); 
            tft.setCursor(20, 270);
            tft.setTextColor(ILI9488_WHITE, ILI9488_BLACK);
            tft.setTextSize(2);
            tft.print(state.deviceName);
            tft.print(" [Vol ");
            tft.print(state.volumePercent);
            tft.print("%]");
        }
        #endif
    }
    
    // Popup Helper (Centered Box)
    void showPopup(const char* text, uint16_t color) {
        int boxW = 300; int boxH = 100;
        int boxX = (480 - boxW) / 2;
        int boxY = (320 - boxH) / 2; // Centered vertically
        
        tft.fillRect(boxX, boxY, boxW, boxH, ILI9488_WHITE);
        tft.drawRect(boxX, boxY, boxW, boxH, ILI9488_BLACK);
        
        // Simple centering estimate for text
        tft.setCursor(boxX + 40, boxY + 40); 
        tft.setTextColor(color, ILI9488_WHITE);
        tft.setTextSize(2);
        tft.println(text);
    }
    
    TFT_eSPI* getTFT() { return &tft; } 
};