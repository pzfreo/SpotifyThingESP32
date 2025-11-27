#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "esp_random.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Button2.h>
#include <WiFiManager.h> // Requires "WiFiManager" by tzapu
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// --- DISPLAY LIBRARIES ---
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <QRCode.h>

// --- CONFIGURATION ---
#define SPOTIFY_REFRESH_RATE_MS 1000 
#define AP_NAME "SpotifySetup"

// --- DISPLAY PINS ---
#define TFT_CS     15
#define TFT_DC     21
#define TFT_RST    4

// --- BUTTON PINS ---
#define PIN_PREV   12
#define PIN_PLAY   13
#define PIN_NEXT   14

// --- DATA STRUCTURES ---
struct SpotifyState {
    char trackName[64];
    char artistName[64];
    char albumName[64];
    char deviceName[64];
    bool isPlaying;
    int progressMS;
    int durationMS;
    bool loggedIn;
};

// Initialize Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Initialize Buttons
Button2 btnPrev, btnPlay, btnNext;

// Globals
const char* AUTHKEY = "ohsosecret";

// API Endpoints
const char* SPOT_PLAYER = "https://api.spotify.com/v1/me/player";
const char* SPOT_NEXT   = "https://api.spotify.com/v1/me/player/next";
const char* SPOT_PREV   = "https://api.spotify.com/v1/me/player/previous";
const char* SPOT_PLAY   = "https://api.spotify.com/v1/me/player/play";
const char* SPOT_PAUSE  = "https://api.spotify.com/v1/me/player/pause";

Preferences prefs;
char accesstoken[512]; 
char deviceId[40];     
const char* authurl = "https://spotauth-36097512380.europe-west1.run.app/";
char urlbuffer[1024];  

// Threading & Synchronization
SemaphoreHandle_t dataMutex;
TaskHandle_t spotifyTaskHandle;

// Shared State (Protected by Mutex)
SpotifyState sharedState;
bool newDataAvailable = false;

// LAST KNOWN ACTIVE DEVICE (For waking up idle sessions)
char g_lastSpotifyDeviceID[64] = ""; 

// Command Triggers (Set by Main Loop, Read by Task)
volatile bool triggerNext = false;
volatile bool triggerPrev = false;
volatile bool triggerPlay = false;

// Display State (Local copy for rendering)
SpotifyState displayState;
char lastTrackName[64] = ""; 

// Factory Reset Timer
unsigned long resetComboStartTime = 0;
bool isResetting = false;

// Logout Timer
unsigned long logoutStartTime = 0;
bool isLoggingOut = false;

// Forward Declarations
void updateDisplay();
void spotifyTask(void * parameter);
boolean getSpotifyData();
void sendSpotifyCommand(const char* method, const char* endpoint);
boolean refreshAccessToken(char *targetBuffer, const char* baseurl);
void gen_random_hex(char* buffer, int numBytes);
void showQRCode(const char* data, const char* title, const char* footer);

// --- WIFI CONNECTION (WiFiManager) ---

// Callback when ESP enters AP mode (Display QR Code for WiFi Setup)
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());

    // Generate WiFi QR Code: WIFI:S:SSID;T:nopass;; (Open Network)
    String qrData = "WIFI:S:" + myWiFiManager->getConfigPortalSSID() + ";T:nopass;;";
    
    showQRCode(qrData.c_str(), "Setup WiFi", "Scan to Connect");
}

void connect_to_wifi() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 120);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.println("Connecting WiFi...");

    WiFiManager wm;
    wm.setAPCallback(configModeCallback);
    
    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name
    if (!wm.autoConnect(AP_NAME)) {
        Serial.println("failed to connect and hit timeout");
        // Reset and try again
        ESP.restart();
        delay(1000);
    }

    // Connected
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 120);
    tft.println("WiFi Connected!");
    Serial.println("WiFi Connected!");
    delay(1000);
}

// --- BUTTON CALLBACKS (Run in Main Loop - MUST BE FAST) ---
void onPrevClick(Button2& btn) {
    Serial.println("BTN: PREV");
    triggerPrev = true; // Signal the background task
}

void onNextClick(Button2& btn) {
    Serial.println("BTN: NEXT");
    triggerNext = true;
}

void onPlayClick(Button2& btn) {
    Serial.println("BTN: PLAY/PAUSE");
    triggerPlay = true;
    
    // Optimistic UI Update (Immediate feedback)
    displayState.isPlaying = !displayState.isPlaying;
    updateDisplay(); 
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    setCpuFrequencyMhz(160); 

    // 1. Init Hardware
    tft.begin();
    tft.setRotation(0); 
    tft.fillScreen(ILI9341_BLACK);
    
    dataMutex = xSemaphoreCreateMutex();

    btnPrev.begin(PIN_PREV);
    btnPrev.setTapHandler(onPrevClick);
    btnPlay.begin(PIN_PLAY);
    btnPlay.setTapHandler(onPlayClick);
    btnNext.begin(PIN_NEXT);
    btnNext.setTapHandler(onNextClick);

    // 2. Connect WiFi & Auth
    connect_to_wifi();
    prefs.begin("spothing", false);

    if (!prefs.isKey("deviceId")) {
        gen_random_hex(deviceId, 16); 
        prefs.putString("deviceId", deviceId);
    } else {
        strlcpy(deviceId, prefs.getString("deviceId").c_str(), sizeof(deviceId));
    }

    // Default State
    strlcpy(sharedState.trackName, "Loading...", 64);
    strlcpy(sharedState.artistName, "", 64);
    strlcpy(sharedState.albumName, "", 64);
    sharedState.isPlaying = false;
    sharedState.loggedIn = prefs.getBool("loggedin", false);

    // Initial Token Check (Blocking)
    if (sharedState.loggedIn) {
        // UI: Show connecting status
        tft.fillScreen(ILI9341_BLACK);
        tft.setCursor(10, 120);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.println("Connecting to");
        tft.println("Spotify...");

        if(!refreshAccessToken(accesstoken, authurl)) {
            sharedState.loggedIn = false;
            prefs.putBool("loggedin", false);
        }
    }

    // Login Flow (Blocking if needed)
    if (!sharedState.loggedIn) {
        strlcpy(urlbuffer, authurl, sizeof(urlbuffer));
        strlcat(urlbuffer, "login?deviceId=", sizeof(urlbuffer));
        strlcat(urlbuffer, deviceId, sizeof(urlbuffer));
        
        // Show Login QR
        showQRCode(urlbuffer, "Scan to Login:", "Waiting for token...");
        
        boolean haveToken = false;
        while (!haveToken) {
            delay(5000);
            haveToken = refreshAccessToken(accesstoken, authurl);
            if (haveToken) {
                prefs.putBool("loggedin", true);
                sharedState.loggedIn = true;
            }
        }
    }
    
    // 3. Start Background Task
    // Core 0 is usually for WiFi/System, Core 1 for Arduino Loop.
    // We pin this task to Core 0 to offload network operations.
    xTaskCreatePinnedToCore(
      spotifyTask,      // Function
      "SpotifyTask",    // Name
      8192,             // Stack size (8KB)
      NULL,             // Params
      1,                // Priority
      &spotifyTaskHandle,
      0                 // Core 0
    );
}

// --- MAIN LOOP (UI & BUTTONS ONLY) ---
void loop() {
    // 1. Check Buttons (Super Fast)
    btnPrev.loop();
    btnPlay.loop();
    btnNext.loop();

    // 2. Factory Reset Logic (Hold Prev + Next for 10s)
    if (btnPrev.isPressed() && btnNext.isPressed()) {
        if (!isResetting) {
            resetComboStartTime = millis();
            isResetting = true;
            Serial.println("Reset Combo Started...");
        } else {
            unsigned long heldTime = millis() - resetComboStartTime;
            
            // UI Feedback (Warning)
            if (heldTime > 2000) {
                 tft.setCursor(30, 240); // Centered-ish at bottom
                 tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
                 tft.setTextSize(1);
                 tft.printf("FACTORY RESET IN %lu...", (10000 - heldTime)/1000);
            }

            if (heldTime > 10000) {
                tft.fillScreen(ILI9341_RED);
                tft.setCursor(20, 120);
                tft.setTextColor(ILI9341_WHITE);
                tft.setTextSize(2);
                tft.println("FACTORY RESET!");
                Serial.println("FACTORY RESET TRIGGERED!");
                
                // Clear Preferences
                prefs.clear(); 
                
                // Clear WiFi Settings
                WiFiManager wm;
                wm.resetSettings();

                delay(2000);
                ESP.restart();
            }
        }
    } else {
        // If user releases buttons early, clear the warning text
        if (isResetting) {
            tft.fillRect(0, 240, 240, 20, ILI9341_BLACK);
        }
        isResetting = false;
    }

    // 3. Logout Logic (Hold Play for 10s)
    // We check that Prev and Next are NOT pressed to ensure we don't conflict with Factory Reset
    if (btnPlay.isPressed() && !btnPrev.isPressed() && !btnNext.isPressed()) {
        if (!isLoggingOut) {
            logoutStartTime = millis();
            isLoggingOut = true;
            Serial.println("Logout Timer Started...");
        } else {
            unsigned long heldTime = millis() - logoutStartTime;
            
            // UI Feedback (Warning)
            if (heldTime > 2000) {
                 tft.setCursor(60, 240); // Same Y position as Reset
                 tft.setTextColor(ILI9341_ORANGE, ILI9341_BLACK);
                 tft.setTextSize(1);
                 tft.printf("LOGOUT IN %lu...", (10000 - heldTime)/1000);
            }

            if (heldTime > 10000) {
                tft.fillScreen(ILI9341_ORANGE);
                tft.setCursor(40, 120);
                tft.setTextColor(ILI9341_BLACK);
                tft.setTextSize(2);
                tft.println("LOGGING OUT...");
                Serial.println("LOGOUT TRIGGERED!");
                
                // Clear Login Only
                prefs.putBool("loggedin", false);
                delay(2000);
                ESP.restart();
            }
        }
    } else {
        // If user releases buttons early, clear the warning text
        if (isLoggingOut) {
            tft.fillRect(0, 240, 240, 20, ILI9341_BLACK);
        }
        isLoggingOut = false;
    }

    // 4. Check for Data Updates from Background Task
    bool shouldRedraw = false;
    
    // Try to take the mutex to read shared data
    // We use a short timeout (0) so we don't block the UI loop if the task is writing
    if (xSemaphoreTake(dataMutex, 0) == pdTRUE) {
        if (newDataAvailable) {
            // Copy shared state to local display state
            memcpy(&displayState, &sharedState, sizeof(SpotifyState));
            newDataAvailable = false;
            shouldRedraw = true;
        }
        xSemaphoreGive(dataMutex);
    }

    if (shouldRedraw) {
        updateDisplay();
    }
}

// --- BACKGROUND TASK (NETWORK OPERATIONS) ---
void spotifyTask(void * parameter) {
    unsigned long lastUpdate = 0;
    bool forceUpdate = true;

    for(;;) {
        // 1. Handle Commands
        if (triggerNext) {
            sendSpotifyCommand("POST", SPOT_NEXT);
            triggerNext = false;
            forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS); // Brief pause before refresh
        }
        if (triggerPrev) {
            sendSpotifyCommand("POST", SPOT_PREV);
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

        // 2. Poll Data
        unsigned long now = millis();
        if (forceUpdate || (now - lastUpdate > SPOTIFY_REFRESH_RATE_MS)) {
            if (getSpotifyData()) {
                // Success
            }
            lastUpdate = now;
            forceUpdate = false;
        }

        // 3. Yield to prevent watchdog triggers
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// --- SPOTIFY API FUNCTIONS (RUN IN TASK) ---
boolean getSpotifyData() {
  if (WiFi.status() != WL_CONNECTED) return false;
    
  WiFiClientSecure client;
  client.setInsecure(); 
  client.setHandshakeTimeout(30); 

  HTTPClient http;
  http.useHTTP10(true); 
  
  if (http.begin(client, SPOT_PLAYER)) {
      char auth[512];
      snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
      http.addHeader("Authorization", auth);
      
      int httpCode = http.GET();
      
      if (httpCode == 200) {
          Stream& responseStream = http.getStream();
          
          JsonDocument filter;
          filter["device"]["name"] = true;
          filter["device"]["id"] = true; // Capture Device ID
          filter["is_playing"] = true;
          filter["progress_ms"] = true;
          filter["item"]["name"] = true;
          filter["item"]["album"]["name"] = true;
          filter["item"]["artists"][0]["name"] = true;
          filter["item"]["duration_ms"] = true;
          
          JsonDocument doc;
          DeserializationError error = deserializeJson(doc, responseStream, DeserializationOption::Filter(filter));
          
          if (!error) {
              // CAPTURE DEVICE ID FOR RECOVERY
              const char* spDevId = doc["device"]["id"];
              if (spDevId && strlen(spDevId) > 0) {
                  strlcpy(g_lastSpotifyDeviceID, spDevId, sizeof(g_lastSpotifyDeviceID));
              }

              // WRITE TO SHARED STATE
              if (xSemaphoreTake(dataMutex, 100 / portTICK_PERIOD_MS) == pdTRUE) {
                  const char* tName = doc["item"]["name"];
                  const char* aName = doc["item"]["artists"][0]["name"];
                  const char* alName = doc["item"]["album"]["name"];
                  const char* dName = doc["device"]["name"];
                  
                  if (tName) strlcpy(sharedState.trackName, tName, 64);
                  else strlcpy(sharedState.trackName, "Unknown", 64);
                  
                  if (aName) strlcpy(sharedState.artistName, aName, 64);
                  if (alName) strlcpy(sharedState.albumName, alName, 64);
                  if (dName) strlcpy(sharedState.deviceName, dName, 64);

                  sharedState.progressMS = doc["progress_ms"];
                  sharedState.durationMS = doc["item"]["duration_ms"];
                  sharedState.isPlaying = doc["is_playing"];
                  
                  newDataAvailable = true;
                  xSemaphoreGive(dataMutex);
              }
              http.end();
              return true;
          } 
      } else if (httpCode == 401) {
          Serial.println("Token expired.");
          refreshAccessToken(accesstoken, authurl);
      }
      http.end();
  } 
  return false;
}

void sendSpotifyCommand(const char* method, const char* endpoint) {
    if (WiFi.status() != WL_CONNECTED) return;

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    // Initial Request URL
    String requestUrl = String(endpoint);
    
    http.begin(client, requestUrl);
    
    char auth[512];
    snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
    http.addHeader("Authorization", auth);
    http.addHeader("Content-Length", "0");

    int httpCode = 0;
    if (strcmp(method, "POST") == 0) httpCode = http.POST("");
    else if (strcmp(method, "PUT") == 0) httpCode = http.PUT("");

    // --- RECOVERY LOGIC ---
    bool needsRetry = false;

    // 1. Expired Token?
    if (httpCode == 401) {
        Serial.println("401 Unauthorized -> Refreshing Token");
        if (refreshAccessToken(accesstoken, authurl)) {
            needsRetry = true;
        }
    }
    // 2. No Active Device? (Device fell asleep/disconnected)
    else if ((httpCode == 404 || httpCode == 403) && strlen(g_lastSpotifyDeviceID) > 0) {
        Serial.printf("%d No Active Device -> Retrying with specific Device ID\n", httpCode);
        
        // Append device_id to query string
        if (requestUrl.indexOf('?') == -1) requestUrl += "?device_id=";
        else requestUrl += "&device_id=";
        requestUrl += String(g_lastSpotifyDeviceID);
        
        needsRetry = true;
    }

    if (needsRetry) {
        http.end(); // Close old connection
        
        Serial.print("Retrying: "); Serial.println(requestUrl);
        http.begin(client, requestUrl);
        
        snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
        http.addHeader("Authorization", auth);
        http.addHeader("Content-Length", "0");
        
        if (strcmp(method, "POST") == 0) httpCode = http.POST("");
        else if (strcmp(method, "PUT") == 0) httpCode = http.PUT("");
        
        Serial.printf("Retry Result: %d\n", httpCode);
    }

    http.end();
}

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

    http.begin(client, urlbuffer);
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

// --- DISPLAY UPDATE (USES LOCAL STATE) ---
void updateDisplay() {
    bool trackChanged = strcmp(displayState.trackName, lastTrackName) != 0;
    
    if (trackChanged) {
        tft.fillScreen(ILI9341_BLACK);
        strlcpy(lastTrackName, displayState.trackName, sizeof(lastTrackName));
        
        // Header
        tft.fillRect(0, 0, 240, 30, 0x1DB9); 
        tft.setCursor(10, 8);
        tft.setTextColor(ILI9341_BLACK);
        tft.setTextSize(2);
        tft.println("NOW PLAYING");
        
        // Track
        tft.setCursor(10, 50);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(2);
        tft.println(displayState.trackName);
        
        // Artist
        tft.setCursor(10, 100);
        tft.setTextColor(ILI9341_LIGHTGREY);
        tft.setTextSize(2);
        tft.println(displayState.artistName);
        
        // Album
        tft.setCursor(10, 140);
        tft.setTextColor(ILI9341_DARKGREY);
        tft.setTextSize(1);
        tft.println(displayState.albumName);

        // Device
        tft.setCursor(10, 280);
        tft.setTextColor(0x1DB9); 
        tft.setTextSize(1);
        tft.print("Device: ");
        tft.println(displayState.deviceName);
    }
    
    // Progress Bar
    tft.fillRect(10, 200, 220, 10, ILI9341_DARKGREY);
    if (displayState.durationMS > 0) {
        int width = map(displayState.progressMS, 0, displayState.durationMS, 0, 220);
        tft.fillRect(10, 200, width, 10, 0x1DB9); 
    }
    
    // Time Text
    tft.setCursor(10, 220);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(2);
    
    int curMin = displayState.progressMS / 60000;
    int curSec = (displayState.progressMS / 1000) % 60;
    int totMin = displayState.durationMS / 60000;
    int totSec = (displayState.durationMS / 1000) % 60;
    
    tft.printf("%02d:%02d / %02d:%02d", curMin, curSec, totMin, totSec);

    // Play/Pause Icon
    tft.setCursor(180, 220);
    if(displayState.isPlaying) {
         tft.print(" > "); 
    } else {
         tft.print(" || ");
    }
}

// --- HELPERS ---
void gen_random_hex(char* buffer, int numBytes) {
  uint8_t rawBytes[numBytes];
  esp_fill_random(rawBytes, numBytes); 
  for (int i = 0; i < numBytes; i++) {
    sprintf(buffer + (i * 2), "%02x", rawBytes[i]);
  }
  buffer[numBytes * 2] = '\0';
}

void showQRCode(const char* data, const char* title, const char* footer) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 10);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.println(title);
    
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(10)];
    qrcode_initText(&qrcode, qrcodeData, 10, ECC_LOW, data);

    int scale = 3; 
    int border = 10;
    int startX = (240 - (qrcode.size * scale)) / 2;
    int startY = 50;

    tft.fillRect(startX - border, startY - border, (qrcode.size * scale) + (border*2), (qrcode.size * scale) + (border*2), ILI9341_WHITE);

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, ILI9341_BLACK);
            }
        }
    }
    
    tft.setCursor(10, 260);
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(1);
    tft.println(footer);
}