
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
#include <qrcode.h>
#include <JPEGDEC.h>

// Include roo_display-based DisplayManager
#include "DisplayManager.h"

// ============================================================
// === CONFIGURATION ===
// ============================================================

#define ENABLE_ALBUM_ART
#define SPOTIFY_REFRESH_RATE_MS 1000
#define AP_NAME "SpotifySetup"
#define SLEEP_TIMEOUT_MS 300000 // 5 Minutes

// --- PINS ---
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

// ============================================================
// === GLOBAL OBJECTS & VARIABLES ===
// ============================================================

Preferences prefs;
SemaphoreHandle_t dataMutex;
TaskHandle_t spotifyTaskHandle;

// roo_display manager
DisplayManager display;
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
int lastBarWidth = -1;

// Logic Control
volatile bool triggerNext = false;
volatile bool triggerPrev = false;
volatile bool triggerPlay = false;
volatile bool triggerLike = false;
volatile int  triggerVolumeChange = 0;
volatile bool triggerRefresh = false;

unsigned long lastActivityTime = 0;
volatile bool isSleeping = false;

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
// === FORWARD DECLARATIONS ===
// ============================================================
void updateDisplay();
void drawAlbumArt(const char* url);
int JPEGDraw(JPEGDRAW *pDraw);
void showPopup(const char* text, Color color);
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

void showPopup(const char* text, Color color) {
    display.showPopup(text, color, Colors::White);
}

void clearScreen() {
    display.clear(Colors::Black);
    lastTrackName[0] = '\0';
    lastDeviceName[0] = '\0';
    lastVolume = -1;
    lastImageUrl[0] = '\0';
    lastIsPlaying = !sharedState.isPlaying;
    lastBarWidth = -1;
}

// JPEG Callback - uses DisplayManager's pushImage
int JPEGDraw(JPEGDRAW *pDraw) {
    display.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight,
                      (uint16_t *)pDraw->pPixels);
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
                        int c = stream->readBytes(jpgBuffer + totalRead,
                            ((JPG_BUFFER_SIZE - totalRead) > size ? size : (JPG_BUFFER_SIZE - totalRead)));
                        totalRead += c;
                        if (len > 0) len -= c;
                        if (totalRead >= JPG_BUFFER_SIZE) break;
                    }
                    delay(1);
                }

                if (totalRead > 0) {
                    if (jpeg.openRAM(jpgBuffer, totalRead, JPEGDraw)) {
                        int scale = 0;
                        if (jpeg.getWidth() > 240) scale = JPEG_SCALE_HALF;
                        if (jpeg.getWidth() > 480) scale = JPEG_SCALE_QUARTER;

                        int outputWidth = jpeg.getWidth();
                        int outputHeight = jpeg.getHeight();
                        if (scale == JPEG_SCALE_HALF) { outputWidth /= 2; outputHeight /= 2; }
                        if (scale == JPEG_SCALE_QUARTER) { outputWidth /= 4; outputHeight /= 4; }

                        int xOff = RIGHT_PANE_X + (LEFT_PANE_WIDTH - outputWidth) / 2;
                        int yOff = (STATUS_BAR_Y - outputHeight) / 2;

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
    display.clear(Colors::Black);

    // Draw title
    display.drawText(title, 10, 20, fontMedium(), Colors::White);

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(10)];
    qrcode_initText(&qrcode, qrcodeData, 10, ECC_LOW, data);

    int scale = 3;
    int border = 10;
    int startX = (SCREEN_WIDTH - (qrcode.size * scale)) / 2;
    int startY = 60;

    // White background for QR code
    display.fillRect(startX - border, startY - border,
                     (qrcode.size * scale) + (border * 2),
                     (qrcode.size * scale) + (border * 2), Colors::White);

    // Draw QR code modules
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                display.drawQRModule(startX + (x * scale), startY + (y * scale),
                                     scale, Colors::Black);
            }
        }
    }

    // Draw footer
    display.drawText(footer, 10, 280, fontMedium(), Colors::Green);
}

void updateDisplay() {
    bool trackChanged = strcmp(sharedState.trackName, lastTrackName) != 0;

#ifdef ENABLE_ALBUM_ART
    // --- ART LAYOUT (480x320) ---
    // Left: Text (0-240). Right: Art (240-480). Bottom: Status (Y=280).

    if (trackChanged) {
        // Clear Left Text Area
        display.fillRect(0, 0, LEFT_PANE_WIDTH, PROGRESS_BAR_Y, Colors::Black);
        strlcpy(lastTrackName, sharedState.trackName, sizeof(lastTrackName));

        // Track Title (top section)
        display.drawTextInRegion(sharedState.trackName, 10, 20,
                                  LEFT_PANE_WIDTH - 20, TRACK_TITLE_H - 20,
                                  fontLarge(), Colors::White, Colors::Black);

        // Artist (middle section)
        display.drawTextInRegion(sharedState.artistName, 10, ARTIST_Y + 10,
                                  LEFT_PANE_WIDTH - 20, ARTIST_H - 10,
                                  fontMedium(), Colors::Cyan, Colors::Black);

        // Album (below artist)
        display.drawTextInRegion(sharedState.albumName, 10, ALBUM_Y,
                                  LEFT_PANE_WIDTH - 20, ALBUM_H,
                                  fontMedium(), Colors::White, Colors::Black);
    }

    // Progress Bar (Full width above status bar)
    int barWidth = 0;
    if (sharedState.durationMS > 0) {
        barWidth = (sharedState.progressMS * SCREEN_WIDTH) / sharedState.durationMS;
    }

    // Anti-Flicker Logic (Only draw if width changed)
    if (barWidth != lastBarWidth) {
        lastBarWidth = barWidth;
        display.drawProgressBar(sharedState.progressMS, sharedState.durationMS,
                                 PROGRESS_BAR_Y, PROGRESS_BAR_H,
                                 Colors::Green, Colors::Grey);
    }

    // --- STATUS BAR (Y=280 to 320) ---
    bool deviceChanged = (strcmp(sharedState.deviceName, lastDeviceName) != 0);
    bool volumeChanged = (sharedState.volumePercent != lastVolume);
    bool playStateChanged = (sharedState.isPlaying != lastIsPlaying);

    // Only redraw status bar background if track changed
    if (trackChanged) {
        display.fillRect(0, STATUS_BAR_Y, SCREEN_WIDTH, STATUS_BAR_HEIGHT, Colors::Black);
    }

    // 1. Time (Always update)
    char timeStr[32];
    int curMin = sharedState.progressMS / 60000;
    int curSec = (sharedState.progressMS / 1000) % 60;
    int totMin = sharedState.durationMS / 60000;
    int totSec = (sharedState.durationMS / 1000) % 60;
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d / %02d:%02d", curMin, curSec, totMin, totSec);

    // Clear time area and redraw
    display.fillRect(TIME_X, TIME_Y, 150, 25, Colors::Black);
    display.drawText(timeStr, TIME_X, TIME_Y, fontMedium(), Colors::White);

    // 2. Play/Pause Icon (Center) - Only if state changed
    if (playStateChanged || trackChanged) {
        lastIsPlaying = sharedState.isPlaying;
        // Clear icon area
        display.fillRect(220, STATUS_BAR_Y, 40, STATUS_BAR_HEIGHT, Colors::Black);

        if (sharedState.isPlaying) {
            display.drawPlayIcon(PLAY_ICON_X, PLAY_ICON_Y, Colors::Green);
        } else {
            display.drawPauseIcon(PLAY_ICON_X, PLAY_ICON_Y, Colors::White);
        }
    }

    // 3. Device/Vol (Right) - Only if value changed
    if (deviceChanged || volumeChanged || trackChanged) {
        strlcpy(lastDeviceName, sharedState.deviceName, sizeof(lastDeviceName));
        lastVolume = sharedState.volumePercent;

        // Clear right area
        display.fillRect(280, STATUS_BAR_Y, 200, STATUS_BAR_HEIGHT, Colors::Black);

        // Format device info string
        char deviceInfo[128];
        snprintf(deviceInfo, sizeof(deviceInfo), "%s [%d%%]",
                 sharedState.deviceName, sharedState.volumePercent);

        display.drawText(deviceInfo, DEVICE_INFO_X, DEVICE_INFO_Y,
                          fontSmall(), Colors::White);
    }

#else
    // --- TEXT LAYOUT ---
    if (trackChanged) {
        display.fillRect(0, 0, SCREEN_WIDTH, 200, Colors::Black);
        strlcpy(lastTrackName, sharedState.trackName, sizeof(lastTrackName));

        // Track Title
        display.drawTextInRegion(sharedState.trackName, 20, 20,
                                  SCREEN_WIDTH - 40, 70,
                                  fontLarge(), Colors::White, Colors::Black);

        // Artist Name
        display.drawTextInRegion(sharedState.artistName, 20, 100,
                                  SCREEN_WIDTH - 40, 50,
                                  fontMedium(), Colors::Cyan, Colors::Black);

        // Album Name
        display.drawTextInRegion(sharedState.albumName, 20, 160,
                                  SCREEN_WIDTH - 40, 30,
                                  fontMedium(), Colors::White, Colors::Black);
    }

    // Progress Bar
    if (sharedState.durationMS > 0) {
        int barWidth = (sharedState.progressMS * 440) / sharedState.durationMS;

        if (barWidth != lastBarWidth) {
            lastBarWidth = barWidth;
            display.fillRect(20, 220, barWidth, 10, Colors::Green);
            if (barWidth < 440) {
                display.fillRect(20 + barWidth, 220, 440 - barWidth, 10, Colors::Grey);
            }
        }
    }

    // Time display
    char timeStr[16];
    int curMin = sharedState.progressMS / 60000;
    int curSec = (sharedState.progressMS / 1000) % 60;
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", curMin, curSec);
    display.fillRect(20, 240, 80, 25, Colors::Black);
    display.drawText(timeStr, 20, 240, fontMedium(), Colors::White);

    bool playStateChanged = (sharedState.isPlaying != lastIsPlaying);

    if (playStateChanged || trackChanged) {
        lastIsPlaying = sharedState.isPlaying;
        display.fillRect(400, 230, 40, 30, Colors::Black);
        if (sharedState.isPlaying) {
            display.drawPlayIcon(400, 240, Colors::Green);
        } else {
            display.drawPauseIcon(400, 240, Colors::White);
        }
    }

    // Device info
    if (strcmp(sharedState.deviceName, lastDeviceName) != 0 ||
        sharedState.volumePercent != lastVolume) {
        strlcpy(lastDeviceName, sharedState.deviceName, sizeof(lastDeviceName));
        lastVolume = sharedState.volumePercent;

        display.fillRect(0, 270, SCREEN_WIDTH, 20, Colors::Black);

        char deviceInfo[128];
        snprintf(deviceInfo, sizeof(deviceInfo), "%s [Vol %d%%]",
                 sharedState.deviceName, sharedState.volumePercent);
        display.drawText(deviceInfo, 20, 275, fontSmall(), Colors::White);
    }
#endif
}

bool wakeUp() {
    lastActivityTime = millis();
    if (isSleeping) {
        isSleeping = false;
        display.setBacklight(true);
        clearScreen();

        if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
            newDataAvailable = true;
            xSemaphoreGive(dataMutex);
        }

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
void onPrevClick(Button2& btn) {
    if (!wakeUp()) {
        Serial.println("BTN: PREV");
        triggerPrev = true;
    }
}

void onNextClick(Button2& btn) {
    if (!wakeUp()) {
        Serial.println("BTN: NEXT");
        triggerNext = true;
    }
}

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
        DeserializationError error = deserializeJson(doc, responseStream,
            DeserializationOption::Filter(filter));

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
    char tid[64];
    if (xSemaphoreTake(dataMutex, 100) == pdTRUE) {
        strlcpy(tid, sharedState.trackID, 64);
        xSemaphoreGive(dataMutex);
    }

    if (strlen(tid) < 5) return;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

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
// === WIFI & BACKGROUND TASK ===
// ============================================================

void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Status: Entered Config Mode");
    String qrData = "WIFI:S:" + myWiFiManager->getConfigPortalSSID() + ";T:nopass;;";
    showQRCode(qrData.c_str(), "Setup WiFi", "Scan to Connect");
}

void connect_to_wifi() {
    display.clear(Colors::Black);
    display.drawText("Connecting WiFi...", 10, 100, fontMedium(), Colors::White);

    WiFiManager wm;
    wm.setAPCallback(configModeCallback);
    if (!wm.autoConnect(AP_NAME)) {
        ESP.restart();
        delay(1000);
    }

    display.clear(Colors::Black);
    display.drawText("WiFi Connected!", 10, 100, fontMedium(), Colors::White);
    delay(1000);
}

void spotifyTask(void * parameter) {
    Serial.println("Status: Spotify Task Started (Core 0)");
    unsigned long lastUpdate = 0;
    bool forceUpdate = true;

    for(;;) {
        if (triggerNext) {
            sendSpotifyCommand("POST", SPOT_NEXT);
            triggerNext = false;
            forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerPrev) {
            long estimatedProgress = sharedState.progressMS;
            if (sharedState.isPlaying) estimatedProgress += (millis() - lastUpdate);

            if (estimatedProgress > 10000) {
                char seekUrl[128];
                snprintf(seekUrl, sizeof(seekUrl), "%s?position_ms=0", SPOT_SEEK);
                sendSpotifyCommand("PUT", seekUrl);
            } else {
                sendSpotifyCommand("POST", SPOT_PREV);
            }
            triggerPrev = false;
            forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerPlay) {
            if (sharedState.isPlaying) sendSpotifyCommand("PUT", SPOT_PAUSE);
            else sendSpotifyCommand("PUT", SPOT_PLAY);
            triggerPlay = false;
            forceUpdate = true;
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

        if (triggerRefresh) {
            forceUpdate = true;
            triggerRefresh = false;
        }

        unsigned long now = millis();
        if (forceUpdate || (!isSleeping && (now - lastUpdate > SPOTIFY_REFRESH_RATE_MS))) {
            if (getSpotifyData()) {
                newDataAvailable = true;
            }
            lastUpdate = now;
            forceUpdate = false;
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

// ============================================================
// === MAIN SETUP ===
// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n--- BOOT ---");

#ifdef ENABLE_ALBUM_ART
    setCpuFrequencyMhz(240);
#else
    setCpuFrequencyMhz(160);
#endif

    // 1. Init Hardware
    display.setBacklight(true);
    display.hardwareReset();
    display.init();

    // STARTUP DIAGNOSTICS: Color Cycle
    display.clear(Colors::Red);
    delay(250);
    display.clear(Colors::Green);
    delay(250);
    display.clear(Colors::Blue);
    delay(250);

    display.clear(Colors::Black);
    display.drawText("System Starting...", 10, 50, fontLarge(), Colors::White);
    delay(500);

#ifdef ENABLE_ALBUM_ART
    jpgBuffer = (uint8_t*)malloc(JPG_BUFFER_SIZE);
    if (!jpgBuffer) {
        display.drawText("RAM FAIL: No JPEG Buffer", 10, 100, fontMedium(), Colors::Red);
        delay(2000);
    } else {
        display.drawText("RAM OK", 10, 100, fontMedium(), Colors::Green);
        delay(500);
    }
#endif

    dataMutex = xSemaphoreCreateMutex();

    // Setup Buttons
    btnPrev.begin(PIN_PREV);
    btnPrev.setTapHandler(onPrevClick);
    btnPrev.setLongClickTime(500);
    btnPlay.begin(PIN_PLAY);
    btnPlay.setTapHandler(onPlayClick);
    btnPlay.setLongClickTime(1000);
    btnNext.begin(PIN_NEXT);
    btnNext.setTapHandler(onNextClick);
    btnNext.setLongClickTime(500);

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

        char url[512];
        strcpy(url, authurl);
        strcat(url, "login?deviceId=");
        strcat(url, deviceId);

        Serial.printf("QR Device ID: %s\n", deviceId);
        showQRCode(url, "Scan to Login:", "Waiting for token...");

        int counter = 0;
        while (!refreshAccessToken(accesstoken, authurl)) {
            delay(5000);
            Serial.print(".");
            showQRCode(url, "Scan to Login:", String("Polling " + String(counter++)).c_str());
        }
        prefs.putBool("loggedin", true);
        clearScreen();
    } else {
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

// ============================================================
// === MAIN LOOP ===
// ============================================================
void loop() {
    btnPrev.loop();
    btnPlay.loop();
    btnNext.loop();

    unsigned long now = millis();

    // 1. Sleep Logic
    if (sharedState.isPlaying) {
        lastActivityTime = now;
    }

    if (!isSleeping && (now - lastActivityTime > SLEEP_TIMEOUT_MS)) {
        isSleeping = true;
        display.setBacklight(false);
        display.clear(Colors::Black);
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
            int currentCountdown = (int)((20000 - heldTime) / 1000);

            if (currentCountdown != lastResetCountdown) {
                lastResetCountdown = currentCountdown;

                if (heldTime > 2000 && heldTime < 10000) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "LOGOUT: %lu", (10000 - heldTime) / 1000);
                    showPopup(buf, Colors::Orange);
                } else if (heldTime >= 10000 && heldTime < 20000) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "RESET: %lu", (20000 - heldTime) / 1000);
                    showPopup(buf, Colors::Red);
                } else if (heldTime >= 20000) {
                    showPopup("FACTORY RESET!", Colors::Red);
                    prefs.clear();
                    WiFiManager wm;
                    wm.resetSettings();
                    delay(2000);
                    ESP.restart();
                }
            }
        }
        return;
    } else {
        if (isResetting) {
            unsigned long totalHold = now - resetComboStartTime;
            if (totalHold >= 10000 && totalHold < 20000) {
                showPopup("LOGGING OUT...", Colors::Orange);
                prefs.putBool("loggedin", false);
                delay(2000);
                ESP.restart();
            } else {
                clearScreen();
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
            showPopup("SAVED TO LIKED", Colors::Magenta);
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
        if (btnNext.isPressed()) {
            if (nextPressTime == 0) nextPressTime = now;
            if (now - nextPressTime > 800) {
                if (now - lastVolRepeat > 500) {
                    triggerVolumeChange = 10;
                    lastVolRepeat = now;
                    wakeUp();
                }
            }
        } else {
            nextPressTime = 0;
        }

        if (btnPrev.isPressed()) {
            if (prevPressTime == 0) prevPressTime = now;
            if (now - prevPressTime > 800) {
                if (now - lastVolRepeat > 500) {
                    triggerVolumeChange = -10;
                    lastVolRepeat = now;
                    wakeUp();
                }
            }
        } else {
            prevPressTime = 0;
        }
    }

    // 5. Update Display
    if (xSemaphoreTake(dataMutex, 0) == pdTRUE) {
        if (newDataAvailable) {
#ifdef ENABLE_ALBUM_ART
            updateDisplay();
            if (strlen(sharedState.imageUrl) > 5 &&
                strcmp(sharedState.imageUrl, lastImageUrl) != 0) {
                strlcpy(lastImageUrl, sharedState.imageUrl, 256);
                display.fillRect(RIGHT_PANE_X, 40, LEFT_PANE_WIDTH, LEFT_PANE_WIDTH, Colors::Black);
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
