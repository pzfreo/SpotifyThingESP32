
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include "esp_random.h"
#include <FS.h>
#include <SPI.h>

// --- FIX: Undefine macros for ArduinoJson conflicts ---
#ifdef swap
#undef swap
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <ArduinoJson.h>
#include <Button2.h>
#include <WiFiManager.h>
#include <TFT_eSPI.h> 
#include <QRCode.h>
#include <JPEGDEC.h>

// ============================================================
// === CONFIGURATION ===
// ============================================================

#define ENABLE_ALBUM_ART 
#define SPOTIFY_REFRESH_RATE_MS 1000 
#define AP_NAME "SpotifySetup"
#define SLEEP_TIMEOUT_MS 300000 // 5 Minutes

// --- PINS ---
#define TFT_BL     22  
#define PIN_PREV   12
#define PIN_PLAY   13
#define PIN_NEXT   14

// --- MEMORY ---
#define JPG_BUFFER_SIZE 60000


// --- API ENDPOINTS ---
const char* SPOT_PLAYER = "https://api.spotify.com/v1/me/player";
const char* SPOT_NEXT   = "https://api.spotify.com/v1/me/player/next";
const char* SPOT_PREV   = "https://api.spotify.com/v1/me/player/previous";
const char* SPOT_PLAY   = "https://api.spotify.com/v1/me/player/play";
const char* SPOT_PAUSE  = "https://api.spotify.com/v1/me/player/pause";
const char* SPOT_VOLUME = "https://api.spotify.com/v1/me/player/volume";
const char* SPOT_SEEK   = "https://api.spotify.com/v1/me/player/seek";
const char* SPOT_LIB    = "https://api.spotify.com/v1/me/tracks"; 

// --- COLORS (Standard ILI9488/TFT_eSPI colors) ---
#define C_BLACK   TFT_BLACK
#define C_WHITE   TFT_WHITE
#define C_RED     TFT_RED
#define C_GREEN   TFT_GREEN
#define C_BLUE    TFT_BLUE
#define C_CYAN    TFT_CYAN
#define C_MAGENTA TFT_MAGENTA
#define C_ORANGE  TFT_ORANGE
#define C_GREY    0x4208 

// ============================================================
// === GLOBAL OBJECTS & VARIABLES ===
// ============================================================

Preferences prefs;
SemaphoreHandle_t dataMutex;
TaskHandle_t spotifyTaskHandle;

TFT_eSPI tft = TFT_eSPI();
JPEGDEC jpeg;
uint8_t* jpgBuffer = NULL;

Button2 btnPrev, btnPlay, btnNext;

// Authentication
char accesstoken[512] = ""; 
char deviceId[40] = "";     
const char* authurl = "https://spotauth-36097512380.europe-west1.run.app/";
char urlbuffer[1024];  
char g_lastSpotifyDeviceID[64] = ""; 

// Data State
struct SpotifyState {
    char trackName[128];
    char artistName[128];
    char albumName[128];
    char deviceName[64];
    char trackID[64]; 
    char imageUrl[256]; 
    bool isPlaying;
    int progressMS;
    int durationMS;
    int volumePercent;
    bool loggedIn;
};

SpotifyState sharedState;
bool newDataAvailable = false;

// Display Tracking
char lastTrackName[128] = ""; 
char lastDeviceName[64] = ""; 
int lastVolume = -1;          
char lastImageUrl[256] = "";
bool lastIsPlaying = false; 
int lastBarWidth = -1; // FIX: Track bar width to prevent flicker

// Logic Control
volatile bool triggerNext = false;
volatile bool triggerPrev = false;
volatile bool triggerPlay = false;
volatile bool triggerLike = false;
volatile int  triggerVolumeChange = 0;
volatile bool triggerRefresh = false; // --- FIX: Added trigger for immediate wake updates ---

unsigned long lastActivityTime = 0;
bool isSleeping = false;

// Timers
unsigned long resetComboStartTime = 0;
bool isResetting = false;
int lastResetCountdown = -1;

unsigned long logoutStartTime = 0;
bool isLoggingOut = false;

unsigned long nextPressTime = 0;
unsigned long prevPressTime = 0;
unsigned long lastVolRepeat = 0;
unsigned long playPressTime = 0;
bool isSavingTrack = false;
unsigned long feedbackMessageClearTime = 0;
bool showFeedbackMessage = false;

// ============================================================
// === FORWARD DECLARATIONS (CRITICAL) ===
// ============================================================
void updateDisplay();
void drawAlbumArt(const char* url);
int JPEGDraw(JPEGDRAW *pDraw);
void showPopup(const char* text, uint16_t color);
void showQRCode(const char* data, const char* title, const char* footer);
void clearScreen();
bool wakeUp();
void configModeCallback(WiFiManager *myWiFiManager);
void connect_to_wifi();
void gen_random_hex(char* buffer, int numBytes);

boolean refreshAccessToken(char *targetBuffer, const char* baseurl);
boolean getSpotifyData();
void sendSpotifyCommand(const char* method, const char* endpoint);
void saveToLiked();
void setSpotifyVolume(int percent);
void spotifyTask(void * parameter);

// ============================================================
// === HELPER FUNCTIONS ===
// ============================================================

void showPopup(const char* text, uint16_t color) {
    int boxW = 300; int boxH = 100;
    int boxX = (480 - boxW) / 2;
    int boxY = (320 - boxH) / 2;
    
    tft.fillRect(boxX, boxY, boxW, boxH, C_WHITE);
    tft.drawRect(boxX, boxY, boxW, boxH, C_BLACK);
    
    tft.setCursor(boxX + 40, boxY + 40); 
    tft.setTextColor(color, C_WHITE);
    tft.setTextSize(2);
    tft.println(text);
}

void clearScreen() {
    tft.fillScreen(C_BLACK);
    lastTrackName[0] = '\0';
    lastDeviceName[0] = '\0';
    lastVolume = -1;
    lastImageUrl[0] = '\0';
    lastIsPlaying = !sharedState.isPlaying; 
    lastBarWidth = -1; // Reset bar tracker
}

// JPEG Callback
int JPEGDraw(JPEGDRAW *pDraw) {
    tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, (uint16_t *)pDraw->pPixels);
    return 1;
}

void drawAlbumArt(const char* url) {
    if (WiFi.status() != WL_CONNECTED) return;
    Serial.printf("Downloading Art: %s\n", url);
    
    WiFiClientSecure imgClient;
    imgClient.setInsecure();
    HTTPClient imgHttp;
    imgHttp.useHTTP10(true);
    
    if (imgHttp.begin(imgClient, url)) {
        int httpCode = imgHttp.GET();
        if (httpCode == 200) {
            int len = imgHttp.getSize();
            if (len > 0 && len < JPG_BUFFER_SIZE) {
                WiFiClient *stream = imgHttp.getStreamPtr();
                int totalRead = 0;
                while (imgHttp.connected() && (len > 0 || len == -1)) {
                    size_t size = stream->available();
                    if (size) {
                        int c = stream->readBytes(jpgBuffer + totalRead, ((JPG_BUFFER_SIZE - totalRead) > size ? size : (JPG_BUFFER_SIZE - totalRead)));
                        totalRead += c;
                        if (len > 0) len -= c;
                        if (totalRead >= JPG_BUFFER_SIZE) break;
                    }
                    delay(1);
                }

                if (totalRead > 0) {
                    if (jpeg.openRAM(jpgBuffer, totalRead, JPEGDraw)) {
                        // Center in Right Pane (X 240-480)
                        int scale = 0;
                        if (jpeg.getWidth() > 240) scale = JPEG_SCALE_HALF;
                        if (jpeg.getWidth() > 480) scale = JPEG_SCALE_QUARTER;
                        
                        int outputWidth = jpeg.getWidth();
                        int outputHeight = jpeg.getHeight();
                        if (scale == JPEG_SCALE_HALF) { outputWidth /= 2; outputHeight /= 2; }
                        if (scale == JPEG_SCALE_QUARTER) { outputWidth /= 4; outputHeight /= 4; }
                        
                        int xOff = 240 + (240 - outputWidth) / 2;
                        int yOff = (280 - outputHeight) / 2; 

                        jpeg.setPixelType(RGB565_BIG_ENDIAN);
                        jpeg.decode(xOff, yOff, scale); 
                        jpeg.close();
                    }
                }
            } else {
                Serial.println("Art too big for buffer");
            }
        }
        imgHttp.end();
    }
}

void showQRCode(const char* data, const char* title, const char* footer) {
    tft.fillScreen(C_BLACK);
    tft.setCursor(0, 20);
    tft.setTextColor(C_WHITE, C_BLACK);
    tft.setTextSize(2);
    tft.println(title);
    
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(10)];
    qrcode_initText(&qrcode, qrcodeData, 10, ECC_LOW, data);

    int scale = 3; 
    int border = 10;
    int startX = (480 - (qrcode.size * scale)) / 2;
    int startY = 60;

    tft.fillRect(startX - border, startY - border, (qrcode.size * scale) + (border*2), (qrcode.size * scale) + (border*2), C_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, C_BLACK);
            }
        }
    }
    
    tft.setCursor(10, 280);
    tft.setTextColor(C_GREEN, C_BLACK);
    tft.setTextSize(2);
    tft.println(footer);
}

void updateDisplay() {
    bool trackChanged = strcmp(sharedState.trackName, lastTrackName) != 0;

#ifdef ENABLE_ALBUM_ART
    // --- ART LAYOUT (480x320) ---
    // Left: Text (0-240). Right: Art (240-480). Bottom: Status (Y=280).
    
    if (trackChanged) {
        // Clear Left Text Area
        tft.fillRect(0, 0, 240, 276, C_BLACK); // Don't clear status bar area
        strlcpy(lastTrackName, sharedState.trackName, sizeof(lastTrackName));
        
        // Track Title
        tft.setViewport(0, 0, 240, 90);
        tft.setTextWrap(true);
        tft.setCursor(10, 20); 
        tft.setTextColor(C_WHITE);
        tft.setTextSize(3); 
        tft.println(sharedState.trackName);
        tft.resetViewport();
        
        // Artist
        tft.setViewport(0, 90, 240, 70);
        tft.setCursor(10, 10); 
        tft.setTextColor(C_CYAN);
        tft.setTextSize(2);
        tft.println(sharedState.artistName);
        tft.resetViewport();

        // Album
        tft.setViewport(0, 160, 240, 120);
        tft.setCursor(10, 0); 
        tft.setTextColor(C_WHITE); 
        tft.setTextSize(2);
        tft.println(sharedState.albumName);
        tft.resetViewport();

        tft.setTextWrap(false);
    }
    
    // Progress Bar (Full width above status bar)
    // Y=276, Height=4
    int barWidth = map(sharedState.progressMS, 0, sharedState.durationMS, 0, 480);
    
    // FIX: Anti-Flicker Logic (Only draw if width changed)
    if (barWidth != lastBarWidth) {
        lastBarWidth = barWidth;
        
        // Draw Green part (Active)
        tft.fillRect(0, 276, barWidth, 4, C_GREEN);
        
        // Draw Grey part (Remaining) - side-by-side, no layering
        if (barWidth < 480) {
             tft.fillRect(barWidth, 276, 480 - barWidth, 4, C_GREY);
        }
    }
    
    // --- STATUS BAR (Y=280 to 320) ---
    bool deviceChanged = (strcmp(sharedState.deviceName, lastDeviceName) != 0);
    bool volumeChanged = (sharedState.volumePercent != lastVolume);
    bool playStateChanged = (sharedState.isPlaying != lastIsPlaying);

    // Only redraw status bar background if track changed to clean up
    if (trackChanged) tft.fillRect(0, 280, 480, 40, C_BLACK);

    // 1. Time (Always update)
    tft.setTextSize(2);
    tft.setCursor(10, 290);
    tft.setTextColor(C_WHITE, C_BLACK);
    int curMin = sharedState.progressMS / 60000;
    int curSec = (sharedState.progressMS / 1000) % 60;
    int totMin = sharedState.durationMS / 60000;
    int totSec = (sharedState.durationMS / 1000) % 60;
    tft.printf("%02d:%02d / %02d:%02d", curMin, curSec, totMin, totSec);

    // 2. Play/Pause Icon (Center) - Only if state changed
    if (playStateChanged || trackChanged) {
        lastIsPlaying = sharedState.isPlaying;
        // Clear icon area first
        tft.fillRect(220, 280, 40, 40, C_BLACK);

        if(sharedState.isPlaying) {
            // Playing -> Show Triangle (State)
            tft.fillTriangle(230, 288, 230, 304, 245, 296, C_GREEN);
        } else {
            // Paused -> Show Bars (State)
             tft.fillRect(230, 288, 5, 16, C_WHITE);
             tft.fillRect(240, 288, 5, 16, C_WHITE);
        }
    }

    // 3. Device/Vol (Right) - Only if value changed
    if (deviceChanged || volumeChanged || trackChanged) {
        // Update trackers
        strlcpy(lastDeviceName, sharedState.deviceName, sizeof(lastDeviceName));
        lastVolume = sharedState.volumePercent;

        // Clear right area
        tft.fillRect(280, 280, 200, 40, C_BLACK);
        
        // Viewport for Device Name (300, 290) Width 170
        tft.setViewport(300, 290, 170, 30);
        tft.setCursor(0, 5); // Relative to viewport
        tft.setTextSize(1); // Small Font for Device Info
        tft.setTextColor(C_WHITE, C_BLACK);
        tft.print(sharedState.deviceName);
        tft.print(" [");
        tft.print(sharedState.volumePercent);
        tft.print("%]");
        tft.resetViewport();
        tft.setTextSize(2); // Restore standard size
    }

#else
    // --- TEXT LAYOUT ---
    // Same logic applies
    if (trackChanged) {
        tft.fillRect(0, 0, 480, 200, C_BLACK); 
        strlcpy(lastTrackName, sharedState.trackName, sizeof(lastTrackName));
        
        tft.setTextWrap(true);

        // Track Title (Size 3)
        tft.setViewport(0, 0, 480, 90);
        tft.setCursor(20, 20);
        tft.setTextColor(C_WHITE, C_BLACK);
        tft.setTextSize(3); 
        tft.println(sharedState.trackName);
        tft.resetViewport();
        
        // Artist Name (Size 2)
        tft.setViewport(0, 90, 480, 70);
        tft.setCursor(20, 10); 
        tft.setTextColor(C_CYAN, C_BLACK);
        tft.setTextSize(2);
        tft.println(sharedState.artistName);
        tft.resetViewport();
        
        // Album Name (Size 2)
        tft.setViewport(0, 160, 480, 40);
        tft.setCursor(20, 0); 
        tft.setTextColor(C_WHITE, C_BLACK);
        tft.setTextSize(2);
        tft.println(sharedState.albumName);
        tft.resetViewport();
        
        tft.setTextWrap(false); 
    }
    
    // Progress Bar
    if (sharedState.durationMS > 0) {
        int barWidth = map(sharedState.progressMS, 0, sharedState.durationMS, 0, 440);
        
        // FIX: Anti-Flicker for Text Layout
        if (barWidth != lastBarWidth) {
            lastBarWidth = barWidth;
            tft.fillRect(20, 220, barWidth, 10, C_GREEN); 
            if (barWidth < 440) {
                tft.fillRect(20 + barWidth, 220, 440 - barWidth, 10, C_GREY); 
            }
        }
    }
    
    // Status Bar Logic
    tft.setTextSize(2);
    tft.setCursor(20, 240);
    tft.setTextColor(C_WHITE, C_BLACK);
    int curMin = sharedState.progressMS / 60000;
    int curSec = (sharedState.progressMS / 1000) % 60;
    tft.printf("%02d:%02d", curMin, curSec);

    bool playStateChanged = (sharedState.isPlaying != lastIsPlaying);
    
    if (playStateChanged || trackChanged) {
        lastIsPlaying = sharedState.isPlaying;
        
        // Play/Pause Icon
        tft.fillRect(400, 230, 40, 30, C_BLACK);
        if(sharedState.isPlaying) {
             // Play Triangle
             tft.fillTriangle(400, 240, 400, 256, 415, 248, C_GREEN);
        } else {
             // Pause Bars
             tft.fillRect(400, 240, 5, 16, C_WHITE);
             tft.fillRect(410, 240, 5, 16, C_WHITE);
        }
    }
    
    // Status Line
    if (strcmp(sharedState.deviceName, lastDeviceName) != 0 || sharedState.volumePercent != lastVolume) {
        strlcpy(lastDeviceName, sharedState.deviceName, sizeof(lastDeviceName));
        lastVolume = sharedState.volumePercent;
        
        tft.fillRect(0, 270, 480, 20, C_BLACK); 
        
        // Viewport for Device (Text Layout)
        tft.setViewport(20, 270, 360, 20);
        tft.setCursor(0, 5); 
        // FIX: Font Size 1
        tft.setTextSize(1);
        tft.setTextColor(C_WHITE, C_BLACK);
        tft.print(sharedState.deviceName);
        tft.print(" [Vol ");
        tft.print(sharedState.volumePercent);
        tft.print("%]");
        tft.resetViewport();
        tft.setTextSize(2);
    }
#endif
}

// --- FIX: Updated WakeUp to handle redraw and force refresh ---
bool wakeUp() {
    lastActivityTime = millis();
    if (isSleeping) {
        isSleeping = false;
        digitalWrite(TFT_BL, HIGH); 
        clearScreen();
        
        // 1. Force the Main Loop to redraw current memory (Text AND Art)
        // This ensures the user sees something immediately
        if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
            newDataAvailable = true; 
            xSemaphoreGive(dataMutex);
        }

        // 2. Force the Background Task to fetch new data immediately
        triggerRefresh = true;

        Serial.println("WakeUp: Requesting immediate update...");
        return true; 
    }
    return false; 
}

void gen_random_hex(char* buffer, int numBytes) {
  uint8_t rawBytes[numBytes];
  esp_fill_random(rawBytes, numBytes); 
  for (int i = 0; i < numBytes; i++) {
    sprintf(buffer + (i * 2), "%02x", rawBytes[i]);
  }
  buffer[numBytes * 2] = '\0';
}

// ============================================================
// === BUTTON CALLBACKS ===
// ============================================================
void onPrevClick(Button2& btn) { if (!wakeUp()) { Serial.println("BTN: PREV"); triggerPrev = true; } }
void onNextClick(Button2& btn) { if (!wakeUp()) { Serial.println("BTN: NEXT"); triggerNext = true; } }
void onPlayClick(Button2& btn) { 
    if (!wakeUp()) {
        if (!isSavingTrack) {
            Serial.println("BTN: PLAY");
            triggerPlay = true;
            if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
                sharedState.isPlaying = !sharedState.isPlaying;
                newDataAvailable = true;
                xSemaphoreGive(dataMutex);
            }
        }
        isSavingTrack = false; 
    }
}

// ============================================================
// === API IMPLEMENTATION ===
// ============================================================

boolean refreshAccessToken(char *targetBuffer, const char* baseurl) {
    WiFiClientSecure client;
    client.setInsecure(); 
    client.setHandshakeTimeout(30); 
    HTTPClient http;
    JsonDocument jsonDoc;
    strlcpy(urlbuffer, authurl, sizeof(urlbuffer));
    strlcat(urlbuffer, "refresh?deviceId=", sizeof(urlbuffer));
    strlcat(urlbuffer, deviceId, sizeof(urlbuffer)); 
    strlcat(urlbuffer, "&authKey=", sizeof(urlbuffer)); 
    strlcat(urlbuffer, AUTHKEY, sizeof(urlbuffer)); 
    
    // DEBUG: Print URL to ensure keys match (Careful with sharing this log)
    // Serial.printf("Polling Auth: %s\n", urlbuffer); 
    Serial.printf("Polling Device ID: %s\n", deviceId);

    if (!http.begin(client, urlbuffer)) return false;
    
    int httpResponseCode = http.GET();
    boolean result = false;
    if (httpResponseCode == 200) {   
        DeserializationError error = deserializeJson(jsonDoc, http.getStream());
        if (!error) {
             const char *newToken = jsonDoc["access_token"];
             if (newToken) {
                strlcpy(targetBuffer, newToken, 512); 
                result = true;
             }
        }
    }
    http.end();
    return result;
}

boolean getSpotifyData() {
    if (WiFi.status() != WL_CONNECTED) return false;
    WiFiClientSecure client;
    client.setInsecure();
    client.setHandshakeTimeout(30);
    HTTPClient http;
    http.useHTTP10(true);

    if (!http.begin(client, SPOT_PLAYER)) return false;
    
    char auth[512];
    snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
    http.addHeader("Authorization", auth);

    int httpCode = http.GET();
    
    if (httpCode == 200) {
        Stream& responseStream = http.getStream();
        JsonDocument filter;
        filter["device"]["name"] = true;
        filter["device"]["id"] = true;
        filter["device"]["volume_percent"] = true;
        filter["is_playing"] = true;
        filter["progress_ms"] = true;
        filter["item"]["name"] = true;
        filter["item"]["album"]["name"] = true;
        filter["item"]["id"] = true;
        filter["item"]["album"]["images"] = true; 
        filter["item"]["artists"][0]["name"] = true;
        filter["item"]["duration_ms"] = true;

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, responseStream, DeserializationOption::Filter(filter));

        if (!error) {
            const char* spDevId = doc["device"]["id"];
            if (spDevId && strlen(spDevId) > 0) {
                 if (strcmp(spDevId, g_lastSpotifyDeviceID) != 0) {
                      strlcpy(g_lastSpotifyDeviceID, spDevId, sizeof(g_lastSpotifyDeviceID));
                      prefs.putString("savedDevId", g_lastSpotifyDeviceID);
                 }
            }

            if (xSemaphoreTake(dataMutex, 100) == pdTRUE) {
                const char* tName = doc["item"]["name"];
                const char* aName = doc["item"]["artists"][0]["name"];
                const char* alName = doc["item"]["album"]["name"];
                const char* dName = doc["device"]["name"];
                const char* tId = doc["item"]["id"];
                
                if (tName) strlcpy(sharedState.trackName, tName, 64);
                if (aName) strlcpy(sharedState.artistName, aName, 64);
                if (alName) strlcpy(sharedState.albumName, alName, 64);
                if (dName) strlcpy(sharedState.deviceName, dName, 64);
                if (tId) strlcpy(sharedState.trackID, tId, 64);

                // Image Logic
                const char* imgUrl = NULL;
                JsonArray images = doc["item"]["album"]["images"];
                if (!images.isNull() && images.size() > 0) {
                    if (images.size() > 1) imgUrl = images[1]["url"];
                    else imgUrl = images[0]["url"];
                }
                if (imgUrl && strcmp(sharedState.imageUrl, imgUrl) != 0) {
                    strlcpy(sharedState.imageUrl, imgUrl, 256);
                }

                sharedState.progressMS = doc["progress_ms"];
                sharedState.durationMS = doc["item"]["duration_ms"];
                sharedState.isPlaying = doc["is_playing"];
                sharedState.volumePercent = doc["device"]["volume_percent"];
                
                newDataAvailable = true;
                xSemaphoreGive(dataMutex);
            }
            http.end();
            return true;
        }
    } else if (httpCode == 204) {
        // No Active Device
        if (xSemaphoreTake(dataMutex, 100) == pdTRUE) {
            strlcpy(sharedState.trackName, "No Active Device", 64);
            strlcpy(sharedState.artistName, "Tap Play to Wake", 64);
            sharedState.isPlaying = false;
            newDataAvailable = true;
            xSemaphoreGive(dataMutex);
        }
    } else if (httpCode == 401) {
        refreshAccessToken(accesstoken, authurl);
    }
    http.end();
    return false;
}

void setSpotifyVolume(int percent) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    char url[128];
    snprintf(url, sizeof(url), "%s?volume_percent=%d", SPOT_VOLUME, percent);
    http.begin(client, url);
    char auth[512];
    snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Length", "0");
    int code = http.PUT("");
    if (code == 401) refreshAccessToken(accesstoken, authurl);
    http.end();
}

void sendSpotifyCommand(const char* method, const char* endpoint) {
    if (WiFi.status() != WL_CONNECTED) return;
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    String requestUrl = String(endpoint);
    http.begin(client, requestUrl);
    char auth[512];
    snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Length", "0");
    
    int httpCode = 0;
    if (strcmp(method, "POST") == 0) httpCode = http.POST("");
    else if (strcmp(method, "PUT") == 0) httpCode = http.PUT("");

    if (httpCode == 401) {
        if (refreshAccessToken(accesstoken, authurl)) {
            http.end(); 
            http.begin(client, requestUrl);
            snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
            http.addHeader("Authorization", auth);
            http.addHeader("Content-Length", "0");
            if (strcmp(method, "POST") == 0) httpCode = http.POST("");
            else if (strcmp(method, "PUT") == 0) httpCode = http.PUT("");
        }
    } else if ((httpCode == 404 || httpCode == 403) && strlen(g_lastSpotifyDeviceID) > 0) {
        // Retry with Device ID
        if (requestUrl.indexOf('?') == -1) requestUrl += "?device_id=";
        else requestUrl += "&device_id=";
        requestUrl += String(g_lastSpotifyDeviceID);
        http.end();
        http.begin(client, requestUrl);
        snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
        http.addHeader("Authorization", auth);
        http.addHeader("Content-Length", "0");
        if (strcmp(method, "POST") == 0) httpCode = http.POST("");
        else if (strcmp(method, "PUT") == 0) httpCode = http.PUT("");
    }
    http.end();
}

void saveToLiked() {
    // 1. Check ID
    char tid[64];
    if (xSemaphoreTake(dataMutex, 100) == pdTRUE) {
        strlcpy(tid, sharedState.trackID, 64);
        xSemaphoreGive(dataMutex);
    }
    
    if (strlen(tid) < 5) return; 

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    // PUT /v1/me/tracks?ids={id}
    String url = String(SPOT_LIB) + "?ids=" + String(tid);
    
    http.begin(client, url);
    char auth[512];
    snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Length", "0"); 
    
    int httpCode = http.PUT("");
    if (httpCode == 200) {
        Serial.println("Saved to Liked Songs!");
    } else {
        Serial.printf("Save Error: %d\n", httpCode);
        if (httpCode == 401) refreshAccessToken(accesstoken, authurl);
    }
    http.end();
}

// ============================================================
// ============================================================

// --- WIFI CALLBACK ---
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Status: Entered Config Mode");
    String qrData = "WIFI:S:" + myWiFiManager->getConfigPortalSSID() + ";T:nopass;;";
    showQRCode(qrData.c_str(), "Setup WiFi", "Scan to Connect");
}

void connect_to_wifi() {
    tft.fillScreen(C_BLACK);
    tft.setCursor(10, 100);
    tft.setTextColor(C_WHITE, C_BLACK); // Set BG Color!
    tft.setTextSize(2);
    tft.println("Connecting WiFi...");

    WiFiManager wm;
    wm.setAPCallback(configModeCallback);
    if (!wm.autoConnect(AP_NAME)) {
        ESP.restart();
        delay(1000);
    }
    
    tft.fillScreen(C_BLACK);
    tft.setCursor(10, 100);
    tft.println("WiFi Connected!");
    delay(1000);
}

// --- BACKGROUND TASK ---
void spotifyTask(void * parameter) {
    Serial.println("Status: Spotify Task Started (Core 0)");
    unsigned long lastUpdate = 0;
    bool forceUpdate = true;

    for(;;) {
        // 1. Handle Commands
        if (triggerNext) {
            sendSpotifyCommand("POST", SPOT_NEXT);
            triggerNext = false; forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerPrev) {
            // Smart Previous Logic
            long estimatedProgress = sharedState.progressMS;
            if (sharedState.isPlaying) estimatedProgress += (millis() - lastUpdate);

            if (estimatedProgress > 10000) { 
                char seekUrl[128];
                snprintf(seekUrl, sizeof(seekUrl), "%s?position_ms=0", SPOT_SEEK);
                sendSpotifyCommand("PUT", seekUrl);
            } else {
                sendSpotifyCommand("POST", SPOT_PREV);
            }
            triggerPrev = false; forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerPlay) {
            if (sharedState.isPlaying) sendSpotifyCommand("PUT", SPOT_PAUSE);
            else sendSpotifyCommand("PUT", SPOT_PLAY);
            triggerPlay = false; forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerVolumeChange != 0) {
            int newVol = sharedState.volumePercent + triggerVolumeChange;
            if (newVol > 100) newVol = 100;
            if (newVol < 0) newVol = 0;
            setSpotifyVolume(newVol);
            triggerVolumeChange = 0;
        }
        if (triggerLike) {
            saveToLiked();
            triggerLike = false;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        // --- FIX: Check for Wake Up Trigger ---
        if (triggerRefresh) {
            forceUpdate = true;
            triggerRefresh = false;
        }

        // 2. Poll Data
        unsigned long now = millis();
        if (forceUpdate || (now - lastUpdate > SPOTIFY_REFRESH_RATE_MS)) {
            // DEBUG: Log before call
            Serial.println("DEBUG: Updating Data..."); 
            if (getSpotifyData()) {
                newDataAvailable = true;
                Serial.println("DEBUG: Data Updated OK");
            } else {
                Serial.println("DEBUG: Data Update Failed");
            }
            lastUpdate = now;
            forceUpdate = false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

// --- MAIN SETUP ---
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- BOOT ---");
    
    #ifdef ENABLE_ALBUM_ART
    setCpuFrequencyMhz(240);
    #else
    setCpuFrequencyMhz(160);
    #endif

    // 1. Init Hardware
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH); 

    // MANUAL RESET
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, HIGH);
    delay(100);
    digitalWrite(TFT_RST, LOW);
    delay(100);
    digitalWrite(TFT_RST, HIGH);
    delay(200);

    // Init TFT_eSPI
    tft.init();
    tft.setRotation(1); // LANDSCAPE 480x320
    
    // STARTUP DIAGNOSTICS: Color Cycle & Text Test
    tft.fillScreen(C_RED);
    delay(250);
    tft.fillScreen(C_GREEN);
    delay(250);
    tft.fillScreen(C_BLUE);
    delay(250);
    
    tft.fillScreen(C_BLACK);
    tft.setCursor(10, 50);
    tft.setTextColor(C_WHITE, C_BLACK); // Set BG Color!
    tft.setTextSize(3);
    tft.println("System Starting...");
    delay(500); 
    
    tft.setTextWrap(false); 
    
#ifdef ENABLE_ALBUM_ART
    jpgBuffer = (uint8_t*)malloc(JPG_BUFFER_SIZE);
    if (!jpgBuffer) {
        tft.setCursor(10, 100);
        tft.setTextColor(C_RED, C_BLACK);
        tft.println("RAM FAIL: No JPEG Buffer");
        delay(2000);
    } else {
        tft.setCursor(10, 100);
        tft.setTextColor(C_GREEN, C_BLACK);
        tft.println("RAM OK");
        delay(500);
    }
#endif

    dataMutex = xSemaphoreCreateMutex();

    // Setup Buttons
    btnPrev.begin(PIN_PREV); btnPrev.setTapHandler(onPrevClick); btnPrev.setLongClickTime(500); 
    btnPlay.begin(PIN_PLAY); btnPlay.setTapHandler(onPlayClick); btnPlay.setLongClickTime(1000); 
    btnNext.begin(PIN_NEXT); btnNext.setTapHandler(onNextClick); btnNext.setLongClickTime(500);

    // 2. Connect WiFi
    connect_to_wifi();
    WiFi.setSleep(false); 

    prefs.begin("spothing", false);

    if (prefs.isKey("savedDevId")) {
        String savedId = prefs.getString("savedDevId");
        if (savedId.length() > 0) {
            strlcpy(g_lastSpotifyDeviceID, savedId.c_str(), sizeof(g_lastSpotifyDeviceID));
            Serial.printf("Loaded Device ID: %s\n", g_lastSpotifyDeviceID);
        }
    }

    if (!prefs.isKey("deviceId")) {
        gen_random_hex(deviceId, 16); 
        prefs.putString("deviceId", deviceId);
    } else {
        strlcpy(deviceId, prefs.getString("deviceId").c_str(), sizeof(deviceId));
    }

    // Login Flow
    if (!prefs.getBool("loggedin", false)) {
        Serial.println("Status: Starting Login Flow");
        
        // FIX: REMOVED SHADOW VARIABLE 'deviceId' that was here
        // Now uses the global 'deviceId' populated above
        
        char url[512];
        strcpy(url, authurl);
        strcat(url, "login?deviceId=");
        strcat(url, deviceId);
        
        Serial.printf("QR Device ID: %s\n", deviceId); // Verify matches Polling ID
        showQRCode(url, "Scan to Login:", "Waiting for token...");
        
        int counter = 0;
        while (!refreshAccessToken(accesstoken, authurl)) {
            delay(5000);
            Serial.print(".");
            // Visual Alive Check
            showQRCode(url, "Scan to Login:", String("Polling " + String(counter++)).c_str());
        }
        prefs.putBool("loggedin", true);
        clearScreen();
    } else {
        // Refresh token on boot
        Serial.println("Status: Refreshing Token...");
        if (!refreshAccessToken(accesstoken, authurl)) {
             Serial.println("Status: Refresh Failed, requiring login.");
             prefs.putBool("loggedin", false);
             ESP.restart();
        }
    }

    // 3. Start Background Task
    xTaskCreatePinnedToCore(spotifyTask, "SpotifyTask", 32768, NULL, 1, &spotifyTaskHandle, 0);
    
    Serial.println("Status: Setup Complete. Loop Starting.");
    lastActivityTime = millis();
}

// --- MAIN LOOP ---
void loop() {
    btnPrev.loop();
    btnPlay.loop();
    btnNext.loop();

    unsigned long now = millis();

    // 1. Sleep Logic
    // --- FIX: Reset sleep timer if playing ---
    // This ensures that even if you listen for 30 mins (no buttons pressed),
    // and then pause, the screen stays on for another 5 mins before sleeping.
    // It also prevents accidental sleep if "isPlaying" flickers.
    if (sharedState.isPlaying) {
        lastActivityTime = now;
    }

    // Standard Sleep Check
    if (!isSleeping && (now - lastActivityTime > SLEEP_TIMEOUT_MS)) {
        isSleeping = true;
        digitalWrite(TFT_BL, LOW); 
        tft.fillScreen(C_BLACK);
        Serial.println("Entering Sleep Mode...");
    }

    // 2. Combo Logic (Reset / Logout)
    if (btnPrev.isPressed() && btnNext.isPressed()) {
        wakeUp();
        if (!isResetting) {
            resetComboStartTime = now;
            isResetting = true;
            lastResetCountdown = -1;
        } else {
            unsigned long heldTime = now - resetComboStartTime;
            int currentCountdown = (int)((20000 - heldTime)/1000);
            
            if (currentCountdown != lastResetCountdown) {
                lastResetCountdown = currentCountdown;
                
                // Update Popup based on time
                if (heldTime > 2000 && heldTime < 10000) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "LOGOUT: %lu", (10000 - heldTime)/1000);
                    showPopup(buf, C_ORANGE);
                }
                else if (heldTime >= 10000 && heldTime < 20000) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "RESET: %lu", (20000 - heldTime)/1000);
                    showPopup(buf, C_RED); 
                }
                else if (heldTime >= 20000) {
                    showPopup("FACTORY RESET!", C_RED);
                    prefs.clear(); 
                    WiFiManager wm;
                    wm.resetSettings();
                    delay(2000);
                    ESP.restart();
                }
            }
        }
        return; // Skip other logic
    } else {
        if (isResetting) {
             // Released: Check for action
             unsigned long totalHold = now - resetComboStartTime;
             if (totalHold >= 10000 && totalHold < 20000) {
                showPopup("LOGGING OUT...", C_ORANGE);
                prefs.putBool("loggedin", false);
                delay(2000);
                ESP.restart();
             } else {
                 clearScreen(); // Clear popup
                 // Force full redraw
                 if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
                    newDataAvailable = true;
                    xSemaphoreGive(dataMutex);
                 }
             }
        }
        isResetting = false;
    }

    // 3. Like Track (Play Long Press > 3s)
    if (!isResetting && btnPlay.isPressed() && !btnPrev.isPressed() && !btnNext.isPressed()) {
        if (playPressTime == 0) playPressTime = now;
        
        if (!isSavingTrack && (now - playPressTime > 3000)) {
            isSavingTrack = true; 
            wakeUp();
            showPopup("SAVED TO LIKED", C_MAGENTA); // Reusing popup style
            showFeedbackMessage = true;
            feedbackMessageClearTime = now + 3000;
            triggerLike = true; 
        }
    } else {
        playPressTime = 0;
    }
    
    // Clear Feedback Message
    if (showFeedbackMessage && now > feedbackMessageClearTime) {
        showFeedbackMessage = false;
        clearScreen();
        if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
            newDataAvailable = true;
            xSemaphoreGive(dataMutex);
        }
    }

    // 4. Volume Control
    if (!isResetting && !btnPlay.isPressed()) {
        // NEXT (Vol Up)
        if (btnNext.isPressed()) {
            if (nextPressTime == 0) nextPressTime = now;
            if (now - nextPressTime > 800) {
                if (now - lastVolRepeat > 500) {
                    triggerVolumeChange = 10;
                    lastVolRepeat = now;
                    wakeUp(); 
                }
            }
        } else { nextPressTime = 0; }

        // PREV (Vol Down)
        if (btnPrev.isPressed()) {
            if (prevPressTime == 0) prevPressTime = now;
            if (now - prevPressTime > 800) {
                if (now - lastVolRepeat > 500) {
                    triggerVolumeChange = -10;
                    lastVolRepeat = now;
                    wakeUp();
                }
            }
        } else { prevPressTime = 0; }
    }

    // 5. Update Display
    if (xSemaphoreTake(dataMutex, 0) == pdTRUE) {
        if (newDataAvailable) {
            #ifdef ENABLE_ALBUM_ART
            updateDisplay();
            // Draw Art if changed
            if (strlen(sharedState.imageUrl) > 5 && strcmp(sharedState.imageUrl, lastImageUrl) != 0) {
                strlcpy(lastImageUrl, sharedState.imageUrl, 256);
                tft.fillRect(240, 40, 240, 240, C_BLACK);
                drawAlbumArt(sharedState.imageUrl);
            }
            newDataAvailable = false;
            #else
            updateDisplay();
            newDataAvailable = false;
            #endif
        }
        xSemaphoreGive(dataMutex);
    }
}
}
