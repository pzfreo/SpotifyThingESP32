#include <Arduino.h>
#include <Button2.h>
#include <WiFiManager.h>
#include "Config.h"
#include "SharedTypes.h"
#include "SpotifyClient.h"
#include "DisplayManager.h"

// --- GLOBAL OBJECTS ---
Preferences prefs;
SemaphoreHandle_t dataMutex;
TaskHandle_t spotifyTaskHandle;

SpotifyClient spotClient(&prefs);
DisplayManager display;
Button2 btnPrev, btnPlay, btnNext;

// --- STATE ---
SpotifyState sharedState;
bool newDataAvailable = false;

// --- LOGIC VARIABLES ---
volatile bool triggerNext = false;
volatile bool triggerPrev = false;
volatile bool triggerPlay = false;
volatile bool triggerLike = false;
volatile int  triggerVolumeChange = 0;

unsigned long lastActivityTime = 0;
bool isSleeping = false;

// Timers for Long Press Logic
unsigned long resetComboStartTime = 0;
bool isResetting = false;
int lastResetCountdown = -1;

unsigned long logoutStartTime = 0;
bool isLoggingOut = false;

// Timers for Volume/Like
unsigned long nextPressTime = 0;
unsigned long prevPressTime = 0;
unsigned long lastVolRepeat = 0;
unsigned long playPressTime = 0;
bool isSavingTrack = false;
unsigned long feedbackMessageClearTime = 0;
bool showFeedbackMessage = false;

// --- HELPER FUNCTIONS ---

// JPEG Callback for Album Art (Must be global/static to work with C-style library)
#ifdef ENABLE_ALBUM_ART
int JPEGDraw(JPEGDRAW *pDraw) {
    // Forward to display object
    // We get the TFT pointer to call pushImage directly for speed
    display.getTFT()->pushImage(pDraw->x, pDraw->y + 40, pDraw->iWidth, pDraw->iHeight, (uint16_t *)pDraw->pPixels);
    return 1;
}
#endif

bool wakeUp() {
    lastActivityTime = millis();
    if (isSleeping) {
        isSleeping = false;
        display.setBacklight(true);
        // Force redraw
        display.clearScreen();
        return true; 
    }
    return false; 
}

// --- BUTTON CALLBACKS ---

void onPrevClick(Button2& btn) {
    if (wakeUp()) return;
    Serial.println("BTN: PREV");
    triggerPrev = true; 
}

void onNextClick(Button2& btn) {
    if (wakeUp()) return;
    Serial.println("BTN: NEXT");
    triggerNext = true;
}

void onPlayClick(Button2& btn) {
    if (wakeUp()) return;
    
    if (!isSavingTrack) {
        Serial.println("BTN: PLAY/PAUSE");
        triggerPlay = true;
        // Optimistic update for UI responsiveness
        if (xSemaphoreTake(dataMutex, 10) == pdTRUE) {
            sharedState.isPlaying = !sharedState.isPlaying;
            newDataAvailable = true;
            xSemaphoreGive(dataMutex);
        }
    }
    isSavingTrack = false; 
}

// --- BACKGROUND TASK (NETWORKING) ---
void spotifyTask(void * parameter) {
    unsigned long lastUpdate = 0;
    bool forceUpdate = true;

    for(;;) {
        // 1. Handle Commands
        if (triggerNext) {
            spotClient.command("POST", SPOT_NEXT);
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
                spotClient.command("PUT", seekUrl);
            } else {
                spotClient.command("POST", SPOT_PREV);
            }
            triggerPrev = false; forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerPlay) {
            if (sharedState.isPlaying) spotClient.command("PUT", SPOT_PAUSE);
            else spotClient.command("PUT", SPOT_PLAY);
            triggerPlay = false; forceUpdate = true;
            vTaskDelay(200 / portTICK_PERIOD_MS);
        }
        if (triggerVolumeChange != 0) {
            int newVol = sharedState.volumePercent + triggerVolumeChange;
            if (newVol > 100) newVol = 100;
            if (newVol < 0) newVol = 0;
            spotClient.setVolume(newVol);
            triggerVolumeChange = 0;
        }
        if (triggerLike) {
            spotClient.saveTrack(sharedState.trackID);
            triggerLike = false;
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        // 2. Poll Data
        unsigned long now = millis();
        if (forceUpdate || (now - lastUpdate > SPOTIFY_REFRESH_RATE_MS)) {
            if (spotClient.getData(sharedState, dataMutex)) {
                newDataAvailable = true;
            }
            lastUpdate = now;
            forceUpdate = false;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

// --- WIFI CALLBACK ---
void configModeCallback(WiFiManager *myWiFiManager) {
    String qrData = "WIFI:S:" + myWiFiManager->getConfigPortalSSID() + ";T:nopass;;";
    display.showQR(qrData.c_str(), "Setup WiFi", "Scan to Connect");
}

// --- MAIN SETUP ---
void setup() {
    Serial.begin(115200);
    
    #ifdef ENABLE_ALBUM_ART
    setCpuFrequencyMhz(240);
    #else
    setCpuFrequencyMhz(160);
    #endif

    // 1. Init Display
    display.init();
    display.showSplash();

    dataMutex = xSemaphoreCreateMutex();

    // 2. Init Buttons
    btnPrev.begin(PIN_PREV); btnPrev.setTapHandler(onPrevClick); btnPrev.setLongClickTime(500);
    btnPlay.begin(PIN_PLAY); btnPlay.setTapHandler(onPlayClick); btnPlay.setLongClickTime(1000);
    btnNext.begin(PIN_NEXT); btnNext.setTapHandler(onNextClick); btnNext.setLongClickTime(500);

    // 3. Connect WiFi
    display.showConnecting();
    WiFiManager wm;
    wm.setAPCallback(configModeCallback);
    if (!wm.autoConnect(AP_NAME)) {
        ESP.restart();
        delay(1000);
    }
    WiFi.setSleep(false); // Performance fix

    // 4. Init Spotify
    prefs.begin("spothing", false);
    
    // Generate random device ID if missing
    if (!prefs.isKey("deviceId")) {
        uint8_t rawBytes[16];
        esp_fill_random(rawBytes, 16);
        char buf[33];
        for (int i = 0; i < 16; i++) sprintf(buf + (i * 2), "%02x", rawBytes[i]);
        prefs.putString("deviceId", buf);
    }
    
    spotClient.init(prefs.getString("deviceId").c_str());

    // Login Flow if needed
    if (!prefs.getBool("loggedin", false)) {
        char url[512];
        strcpy(url, spotClient.getAuthUrl());
        strcat(url, "login?deviceId=");
        strcat(url, spotClient.getDeviceId());
        
        display.showQR(url, "Scan to Login:", "Waiting for token...");
        
        while (!spotClient.refreshAccessToken()) {
            delay(5000);
        }
        prefs.putBool("loggedin", true);
        display.clearScreen();
    }

    // 5. Start Task
    xTaskCreatePinnedToCore(spotifyTask, "SpotifyTask", 8192, NULL, 1, &spotifyTaskHandle, 0);
    lastActivityTime = millis();
}

// --- MAIN LOOP ---
void loop() {
    btnPrev.loop();
    btnPlay.loop();
    btnNext.loop();

    unsigned long now = millis();

    // 1. Sleep Logic
    if (!isSleeping && !sharedState.isPlaying && (now - lastActivityTime > SLEEP_TIMEOUT_MS)) {
        isSleeping = true;
        display.setBacklight(false);
        display.clearScreen(); // Turn black
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
                    display.showPopup(buf, ILI9488_ORANGE);
                }
                else if (heldTime >= 10000 && heldTime < 20000) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "RESET: %lu", (20000 - heldTime)/1000);
                    display.showPopup(buf, ILI9488_RED); // Reusing popup, text might differ slightly but functionally same
                }
                else if (heldTime >= 20000) {
                    display.showPopup("FACTORY RESET!", ILI9488_RED);
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
                display.showPopup("LOGGING OUT...", ILI9488_ORANGE);
                prefs.putBool("loggedin", false);
                delay(2000);
                ESP.restart();
             } else {
                 display.clearScreen(); // Clear popup
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
            display.showPopup("SAVED TO LIKED", ILI9488_MAGENTA); // Reusing popup style
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
        display.clearScreen();
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
            display.update(sharedState, JPEGDraw);
            #else
            display.update(sharedState, NULL);
            #endif
            newDataAvailable = false;
        }
        xSemaphoreGive(dataMutex);
    }
}