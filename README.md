# ESP32 Lightsaber by MrNaif

A feature-rich, customizable lightsaber project built for ESP32 microcontrollers with RGB LED strips, motion sensing, audio effects, and wireless connectivity.

## Features

- **RGB Lighting Effects**: Multiple color modes including solid, blink, glow, ocean, color wipe, alien, blend, pulse, and party
- **Color Selection**: Choose from multiple preset colors (red, green, blue, yellow, cyan, magenta)
- **Motion-Activated Sound Effects**: Swing and clash sound effects triggered by motion sensing
- **Audio Playback**: Supports MP3 audio from SD card and internet radio
- **Power-Up/Down Animations**: Realistic blade ignition and retraction animations
- **Wireless Connectivity**: WiFi support for OTA updates and web configuration
- **Web Serial Interface**: Debug and control via browser
- **Battery Monitoring**: Check battery percentage
- **Volume Control**: Adjustable audio volume

## Hardware Requirements

- ESP32 microcontroller board (I used ESP32 DevKitC V4)
- WS2812B RGB LED strip
- SD card reader
- MPU6050
- MAX98357A Audio amplifier and speaker (3W 4 Ohm)
- Two push buttons
- Battery (3 18650 in parallel)
- TP4056 battery charger
- DC DC booster (5V output)

## Usage

### Controls

- **Button 1**:
  - Single click: Toggle lightsaber on/off
  - Double click: Change color mode
  - Triple click: Show debug info
  - Long press: Increase volume

- **Button 2**:
  - Single click: Change color
  - Double click: Change SD sound
  - Triple click: Change internet radio
  - Long press: Decrease volume

- **Misc controls**:
  - Tap both buttons: Announce device ip
  - Hold both buttons for > 1 second: Toggle sound mode (only lightsaber sounds/interleave with lightsaber effects/only custom sounds)
  - Hold both buttons for > 2 seconds: Toggle Internet radio mode
  - Hold both buttons for > 5 seconds: Reset WiFi credentials

### Initial Setup

1. Power on the device
2. Connect to the "Lightsaber_AP" WiFi network
3. Open a web browser and navigate to "router" ip address, and configure WiFi credentials
4. When done, click "Exit" to make the device connect to the configured WiFi network
5. Default web interface credentials: admin/admin (/update for OTA updates, /webserial for web serial)

## Software

This project uses PlatformIO for development. Main components include:

- NeoPixelBus library for LED control
- Adafruit MPU6050 for motion sensing
- ESP32-audioI2S for sound playback
- Button2 library for button handling
- AsyncWebServer for web interface
- ElegantOTA for over-the-air updates
- WebSerial for debugging
- NetWizard for WiFi management

## Building & Flashing

1. Clone this repository
2. Open in PlatformIO
3. Build and upload to your ESP32

## Over-the-Air Updates

Once connected to WiFi, you can update the firmware via a web browser:

1. Connect to the same WiFi network as the lightsaber
2. Navigate to the lightsaber's IP address
3. Access the /update endpoint
4. Login with admin/admin credentials
5. Upload the new firmware

## Customization

- Edit `led.h` to customize lighting effects and colors
- Modify motion triggers in `main.cpp` to adjust sensitivity
- Add your own MP3 sounds to the SD card for custom effects

## Wishlist

- Instead of ESP32-audioI2S, use [Arduino audio tools](https://github.com/pschatzmann/arduino-audio-tools) for less RAM usage. Unfortunately the library was too complex to integrate for now, so I used ESP32-audioI2S.
- A Web interface (frontend) to control the lightsaber
