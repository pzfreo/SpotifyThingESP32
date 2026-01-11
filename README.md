# SpotifyThing ESP32

A Spotify player display for ESP32 devices with hardware button controls. Shows now playing information including track, artist, album, playback progress, and optionally album art.

## Supported Devices

### M5Stack Core
- **Display**: ILI9342C 320x240 (using ILI9341 driver)
- **Features**: Built-in buttons, speaker, larger fonts
- **No**: PSRAM, album art
- **Flash usage**: 97.8%

### dev1 (Freenove ESP32-WROVER)
- **Display**: ILI9488 480x320
- **Features**: Hardware buttons, album art, PSRAM
- **Flash usage**: 97.2%

## Features

- 🎵 Real-time Spotify playback information
- ⏯️ Hardware button controls (play/pause, next, previous, volume)
- 🖼️ Album art display (dev1 only)
- 📊 Progress bar with time display
- 🎨 Anti-aliased fonts (roo_display library)
- 📱 WiFi configuration via captive portal
- 🔐 Secure authentication via QR code
- 💤 Auto-sleep after 5 minutes of inactivity

## Setup

### Prerequisites

1. **PlatformIO** - Install via VS Code extension or CLI
2. **Spotify Auth Key** - Set as environment variable:
   ```bash
   export SPOTIFY_AUTH_KEY="your_auth_key_here"
   ```

### Building

For M5Stack Core:
```bash
pio run -e m5stack_core -t upload
```

For dev1 (Freenove ESP32-WROVER):
```bash
pio run -e dev1_freenove -t upload
```

### First Time Setup

1. Device creates WiFi access point `SpotifySetup`
2. Connect to it and configure your WiFi credentials
3. Device displays QR code for Spotify authentication
4. Scan QR code and authorize the application
5. Device connects and starts displaying playback info

## Button Controls

### Single Press
- **Left Button**: Previous track
- **Middle Button**: Play/Pause
- **Right Button**: Next track

### Long Press
- **Left Button**: Volume down (hold to repeat)
- **Middle Button**: Save track to Liked Songs (2+ seconds)
- **Right Button**: Volume up (hold to repeat)

### Reset & Logout
- **Both outer buttons**: Hold for 10 seconds to reset WiFi/auth
- **All three buttons**: Hold for 10 seconds to logout only

## Technical Details

### Display Configuration

**M5Stack Core:**
- Orientation: LeftDown (landscape)
- Fonts: 15pt status, 27pt content, 27pt Bold titles
- Text rendering: FILL_MODE_RECTANGLE (no anti-aliasing)
- Color fix: INVON command for ILI9342C compatibility

**dev1 (Freenove):**
- Orientation: DownRight (landscape)
- Fonts: 12pt status, 15pt content, 18pt Bold titles
- Text rendering: Anti-aliased with clip box
- Album art: 300x300 JPEG decoder with scaling

### Memory Usage

**M5Stack Core:**
- RAM: 15.8% (51,704 / 327,680 bytes)
- Flash: 97.8% (1,281,309 / 1,310,720 bytes)

**dev1 (Freenove):**
- RAM: 21.3% (69,680 / 327,680 bytes)
- Flash: 97.2% (1,273,749 / 1,310,720 bytes)

### Libraries Used

- [roo_display](https://github.com/dejwk/roo_display) - High-quality anti-aliased fonts
- [ArduinoJson](https://arduinojson.org/) - JSON parsing
- [WiFiManager](https://github.com/tzapu/WiFiManager) - WiFi configuration
- [Button2](https://github.com/LennartHennigs/Button2) - Button handling
- [QRCode](https://github.com/ricmoo/QRCode) - QR code generation
- [JPEGDEC](https://github.com/bitbank2/JPEGDEC) - Album art decoding (dev1 only)

## Known Issues

- SSL close_notify error logged every second (harmless, doesn't affect functionality)
  - Error: `[ssl_client.cpp:37] _handle_error(): [data_to_read():361]: (-76) UNKNOWN ERROR CODE (004C)`
  - This is the Spotify server properly closing SSL connections
  - Device works perfectly despite this message

## Development

### Project Structure
```
SpotifyThingESP32/
├── src/
│   ├── SpotifyThing.cpp          # Main application logic
│   ├── DisplayManager.h          # Display abstraction layer
│   ├── DisplayManager.cpp        # Display implementation
│   └── config/
│       └── devices/
│           ├── m5stack_core.h    # M5Stack configuration
│           └── dev1_freenove.h   # dev1 configuration
├── platformio.ini                # Build configuration
├── pre_build_patch.py            # Pre-build SD card library patches
└── ccache_config.py              # Compiler cache configuration
```

### Adding a New Device

1. Create device config in `src/config/devices/your_device.h`
2. Define display pins, dimensions, and features
3. Add environment to `platformio.ini`
4. Set appropriate `DISPLAY_DRIVER_*` define

### Font Customization

Fonts are device-specific in `src/DisplayManager.h`:
- Available sizes: 8, 10, 12, 15, 18, 27, 40, 60, 90
- Families: NotoSans, NotoSerif, NotoSansMono
- Weights: Regular, Bold, Italic, BoldItalic

Balance font sizes with flash constraints!

## License

[Add your license here]

## Credits

Built with ❤️ using PlatformIO and ESP32

Co-developed with Claude Sonnet 4.5
