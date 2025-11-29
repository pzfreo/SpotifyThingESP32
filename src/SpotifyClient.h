#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "Config.h"
#include "SharedTypes.h"

// API Endpoints
const char* SPOT_PLAYER = "https://api.spotify.com/v1/me/player";
const char* SPOT_NEXT   = "https://api.spotify.com/v1/me/player/next";
const char* SPOT_PREV   = "https://api.spotify.com/v1/me/player/previous";
const char* SPOT_PLAY   = "https://api.spotify.com/v1/me/player/play";
const char* SPOT_PAUSE  = "https://api.spotify.com/v1/me/player/pause";
const char* SPOT_VOLUME = "https://api.spotify.com/v1/me/player/volume";
const char* SPOT_SEEK   = "https://api.spotify.com/v1/me/player/seek";
const char* SPOT_LIB    = "https://api.spotify.com/v1/me/tracks"; 

class SpotifyClient {
private:
    Preferences* prefs;
    char accesstoken[512];
    char deviceId[40];
    const char* authUrl = "https://spotauth-36097512380.europe-west1.run.app/";
    char lastDeviceID[64]; // For auto-waking

    // Helper for Auth Headers
    void addAuthHeaders(HTTPClient& http) {
        char auth[512];
        snprintf(auth, sizeof(auth), "Bearer %s", accesstoken);
        http.addHeader("Authorization", auth);
    }

public:
    SpotifyClient(Preferences* p) : prefs(p) {
        lastDeviceID[0] = '\0';
    }

    void init(const char* devId) {
        strlcpy(deviceId, devId, sizeof(deviceId));
        // Load last known device ID from flash
        if (prefs->isKey("savedDevId")) {
            String s = prefs->getString("savedDevId");
            strlcpy(lastDeviceID, s.c_str(), sizeof(lastDeviceID));
        }
    }

    char* getAccessToken() { return accesstoken; }
    const char* getAuthUrl() { return authUrl; }
    const char* getDeviceId() { return deviceId; }

    bool refreshAccessToken() {
        WiFiClientSecure client;
        client.setInsecure();
        client.setHandshakeTimeout(30);
        HTTPClient http;
        
        String url = String(authUrl) + "refresh?deviceId=" + String(deviceId) + "&authKey=" + String(AUTHKEY);
        http.begin(client, url);
        
        int code = http.GET();
        bool result = false;
        
        if (code == 200) {
            JsonDocument doc;
            deserializeJson(doc, http.getStream());
            const char* token = doc["access_token"];
            if (token) {
                strlcpy(accesstoken, token, 512);
                result = true;
            }
        }
        http.end();
        return result;
    }

    // Returns true if data updated, false otherwise
    bool getData(SpotifyState& state, SemaphoreHandle_t mutex) {
        if (WiFi.status() != WL_CONNECTED) return false;
        
        WiFiClientSecure client;
        client.setInsecure();
        client.setHandshakeTimeout(30);
        HTTPClient http;
        http.useHTTP10(true);

        if (!http.begin(client, SPOT_PLAYER)) return false;
        addAuthHeaders(http);

        int code = http.GET();
        bool updated = false;

        if (code == 200) {
            JsonDocument filter;
            filter["device"]["name"] = true;
            filter["device"]["id"] = true;
            filter["device"]["volume_percent"] = true;
            filter["is_playing"] = true;
            filter["progress_ms"] = true;
            filter["item"]["name"] = true;
            filter["item"]["album"]["name"] = true;
            filter["item"]["id"] = true;
            #ifdef ENABLE_ALBUM_ART
            filter["item"]["album"]["images"] = true;
            #endif
            filter["item"]["artists"][0]["name"] = true;
            filter["item"]["duration_ms"] = true;

            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));

            if (!err) {
                const char* remoteDevId = doc["device"]["id"];
                // Save device ID if new
                if (remoteDevId && strcmp(remoteDevId, lastDeviceID) != 0) {
                    strlcpy(lastDeviceID, remoteDevId, sizeof(lastDeviceID));
                    prefs->putString("savedDevId", lastDeviceID);
                }

                // Thread-safe update
                if (xSemaphoreTake(mutex, 100) == pdTRUE) {
                    const char* t = doc["item"]["name"];
                    const char* a = doc["item"]["artists"][0]["name"];
                    const char* al = doc["item"]["album"]["name"];
                    const char* d = doc["device"]["name"];
                    const char* tid = doc["item"]["id"];

                    if (t) strlcpy(state.trackName, t, 64);
                    if (a) strlcpy(state.artistName, a, 64);
                    if (al) strlcpy(state.albumName, al, 64);
                    if (d) strlcpy(state.deviceName, d, 64);
                    if (tid) strlcpy(state.trackID, tid, 64);

                    state.progressMS = doc["progress_ms"];
                    state.durationMS = doc["item"]["duration_ms"];
                    state.isPlaying = doc["is_playing"];
                    state.volumePercent = doc["device"]["volume_percent"];

                    #ifdef ENABLE_ALBUM_ART
                    // Logic to pick best image
                    const char* imgUrl = NULL;
                    JsonArray images = doc["item"]["album"]["images"];
                    if (!images.isNull() && images.size() > 0) {
                         if (images.size() > 1) imgUrl = images[1]["url"];
                         else imgUrl = images[0]["url"];
                    }
                    if (imgUrl && strcmp(state.imageUrl, imgUrl) != 0) {
                        strlcpy(state.imageUrl, imgUrl, 256);
                    }
                    #endif

                    updated = true;
                    xSemaphoreGive(mutex);
                }
            }
        } else if (code == 204) {
            // No Content (Idle)
            if (xSemaphoreTake(mutex, 100) == pdTRUE) {
                strlcpy(state.trackName, "No Active Device", 64);
                strlcpy(state.artistName, "Tap Play to Wake", 64);
                state.isPlaying = false;
                updated = true;
                xSemaphoreGive(mutex);
            }
        } else if (code == 401) {
            refreshAccessToken();
        }
        
        http.end();
        return updated;
    }

    void command(const char* method, const char* endpoint, String body = "") {
        if (WiFi.status() != WL_CONNECTED) return;
        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;
        
        String url = String(endpoint);
        http.begin(client, url);
        addAuthHeaders(http);
        if (body.length() > 0) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Content-Length", String(body.length()));
        } else {
            http.addHeader("Content-Length", "0");
        }

        int code = 0;
        if (strcmp(method, "POST") == 0) code = http.POST(body);
        else if (strcmp(method, "PUT") == 0) code = http.PUT(body);

        // Retry logic for 401 (Auth) or 404 (No Device)
        if (code == 401) {
            if (refreshAccessToken()) {
                // Retry once
                http.end();
                http.begin(client, url);
                addAuthHeaders(http);
                if (body.length() > 0) {
                    http.addHeader("Content-Type", "application/json");
                    http.addHeader("Content-Length", String(body.length()));
                } else {
                    http.addHeader("Content-Length", "0");
                }
                if (strcmp(method, "POST") == 0) http.POST(body);
                else if (strcmp(method, "PUT") == 0) http.PUT(body);
            }
        } 
        else if ((code == 404 || code == 403) && strlen(lastDeviceID) > 0) {
            // Retry with Device ID
            if (url.indexOf('?') == -1) url += "?device_id=";
            else url += "&device_id=";
            url += String(lastDeviceID);
            
            http.end();
            http.begin(client, url);
            addAuthHeaders(http);
             if (body.length() > 0) {
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Content-Length", String(body.length()));
            } else {
                http.addHeader("Content-Length", "0");
            }
            if (strcmp(method, "POST") == 0) http.POST(body);
            else if (strcmp(method, "PUT") == 0) http.PUT(body);
        }
        http.end();
    }

    void setVolume(int percent) {
        char url[128];
        snprintf(url, sizeof(url), "%s?volume_percent=%d", SPOT_VOLUME, percent);
        command("PUT", url);
    }

    void saveTrack(const char* trackId) {
        if (strlen(trackId) < 5) return;
        String url = String(SPOT_LIB) + "?ids=" + String(trackId);
        command("PUT", url.c_str());
    }
};