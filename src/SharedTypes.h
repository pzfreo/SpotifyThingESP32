#pragma once
#include <Arduino.h>

struct SpotifyState {
    char trackName[64];
    char artistName[64];
    char albumName[64];
    char deviceName[64];
    char trackID[64]; 
    
    #ifdef ENABLE_ALBUM_ART
    char imageUrl[256]; 
    #endif
    
    bool isPlaying;
    int progressMS;
    int durationMS;
    int volumePercent;
    bool loggedIn;
};