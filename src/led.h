#pragma once
#include <NeoPixelBusLg.h>

#include "config.h"

extern NeoPixelBusLg<NeoGrbFeature, NeoWs2812xMethod> strip;

void setPixel(int pixel, uint8_t red, uint8_t green, uint8_t blue) {
  strip.SetPixelColor(pixel, RgbColor(red, green, blue));
}

void setAll(uint8_t red, uint8_t green, uint8_t blue) {
  for (int i = 0; i < NUM_PIXELS; i++) {
    setPixel(i, red, green, blue);
  }
  strip.Show();
}

RgbColor paletteLookup(const RgbColor *p, uint8_t size, uint8_t idx) {
  return p[map(idx, 0, 255, 0, size - 1)];
}

RgbColor blend(RgbColor a, RgbColor b, float t) {
  return RgbColor((uint8_t)(a.R + (b.R - a.R) * t),
                  (uint8_t)(a.G + (b.G - a.G) * t),
                  (uint8_t)(a.B + (b.B - a.B) * t));
}

enum class ColorMode {
  SOLID,
  BLINK,
  GLOW,
  OCEAN,
  COLOR_WIPE,
  ALIEN,
  BLEND,
  PULSE,
  PARTY,
};

constexpr auto COLORMODE_COUNT =
    static_cast<std::underlying_type_t<ColorMode>>(ColorMode::PARTY) + 1;

enum class Color {
  RED,
  GREEN,
  BLUE,
  YELLOW,
  CYAN,
  MAGENTA,
};
constexpr auto COLOR_COUNT =
    static_cast<std::underlying_type_t<Color>>(Color::MAGENTA) + 1;

// color presets

const RgbColor firePalette[] = {RgbColor(0, 0, 0), RgbColor(255, 0, 0),
                                RgbColor(255, 165, 0), RgbColor(255, 255, 0)};

const RgbColor oceanPalette[] = {RgbColor(0, 128, 128), RgbColor(64, 224, 208),
                                 RgbColor(173, 216, 230),
                                 RgbColor(255, 255, 255)};

const RgbColor partyCols[] = {RgbColor(255, 0, 255), RgbColor(0, 255, 255),
                              RgbColor(128, 255, 0)};

const RgbColor pastelA(189, 252, 201);
const RgbColor pastelB(230, 230, 250);
const RgbColor pastelC(255, 218, 185);

uint8_t red = 255, green = 0, blue = 0;
float k = 0.2;

constexpr float MIN_BRIGHTNESS = 0.1f;
const uint16_t BreathPeriod = 2000;

static RgbColor FadeColor(const RgbColor &baseColor, float brightness) {
  HsbColor hsb = baseColor;
  hsb.B = MIN_BRIGHTNESS + brightness * (1.0f - MIN_BRIGHTNESS);
  return hsb;
}

void UpdateBlink(uint32_t nowMillis, const RgbColor &blinkColor,
                 uint32_t periodMs = 1000) {
  uint32_t t = nowMillis % periodMs;
  float half = periodMs * 0.5f;
  float brightness = (t < half) ? (t / half) : ((periodMs - t) / half);
  for (int i = 0; i < NUM_PIXELS; i++) {
    auto fadeColor = FadeColor(blinkColor, brightness);
    setPixel(i, fadeColor.R, fadeColor.G, fadeColor.B);
  }
}

void UpdateGlow(uint32_t nowMillis, const RgbColor &glowColor,
                uint32_t periodMs = 1000) {
  uint32_t t = nowMillis % periodMs;
  float phase = float(t) / float(periodMs);
  float brightness = 0.5f * (1.0f + sinf(2.0f * PI * phase - PI / 2.0f));
  for (int i = 0; i < NUM_PIXELS; i++) {
    auto fadeColor = FadeColor(glowColor, brightness);
    setPixel(i, fadeColor.R, fadeColor.G, fadeColor.B);
  }
}

void applyColorMode(ColorMode mode) {
  switch (mode) {
  case ColorMode::SOLID:
    setAll(red, green, blue);
    break;
  case ColorMode::BLINK: {
    auto now = millis();
    UpdateBlink(now, RgbColor(red, green, blue), 1000);
    break;
  }
  case ColorMode::GLOW: {
    auto now = millis();
    UpdateGlow(now, RgbColor(red, green, blue), 1000);
    break;
  }
  case ColorMode::OCEAN: {
    static uint8_t offset = 0;
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      uint8_t idx = (i * 256 / NUM_PIXELS + offset) & 0xFF;
      auto color = paletteLookup(oceanPalette, 4, idx);
      setPixel(i, color.R, color.G, color.B);
    }
    offset++;
    break;
  }
  case ColorMode::COLOR_WIPE: {
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      uint8_t flick = random(0, 256);
      auto color = paletteLookup(firePalette, 4, flick);
      setPixel(i, color.R, color.G, color.B);
    }
    break;
  }
  case ColorMode::ALIEN: {
    for (uint16_t i = 0; i < NUM_PIXELS; i++) {
      setPixel(i, (i + xTaskGetTickCount() / 5) & 0xFF,
               255 - ((i + xTaskGetTickCount() / 3) & 0xFF),
               ((i * 2 + xTaskGetTickCount() / 7) & 0xFF));
    }
    break;
  }
  case ColorMode::BLEND: {
    float t = (sin(millis() / 2000.0) + 1) / 2;
    RgbColor mid = blend(pastelA, pastelC, t);
    for (int i = 0; i < NUM_PIXELS; i++) {
      float fi = float(i) / NUM_PIXELS;
      RgbColor c = (fi < 0.5) ? blend(pastelA, pastelB, fi * 2)
                              : blend(pastelB, mid, (fi - 0.5) * 2);
      setPixel(i, c.R, c.G, c.B);
    }
    break;
  }
  case ColorMode::PULSE: {
    static float blinkOffset = 0;
    blinkOffset =
        blinkOffset * k + float(random(-BLINK_AMPL, BLINK_AMPL)) * (1.0 - k);
    if (red == 255 && green == 0 && blue == 0) {
      blinkOffset = constrain(blinkOffset, -30.0f, 30.0f);
    }
    auto redOffset = constrain(red + blinkOffset, 0, 255);
    auto greenOffset = constrain(green + blinkOffset, 0, 255);
    auto blueOffset = constrain(blue + blinkOffset, 0, 255);
    for (int i = 0; i < NUM_PIXELS; i++) {
      setPixel(i, redOffset, greenOffset, blueOffset);
    }
    break;
  }
  case ColorMode::PARTY: {
    static int idx = 0;
    for (int i = 0; i < NUM_PIXELS; i++) {
      auto color = partyCols[(idx + i) % 3];
      setPixel(i, color.R, color.G, color.B);
    }
    strip.Show();
    idx = (idx + 1) % 3;
    break;
  }
  }
  strip.Show();
}

void applyColor(Color color) {
  switch (color) {
  case Color::RED:
    (red = 255, green = 0, blue = 0);
    break;
  case Color::GREEN:
    (red = 0, green = 255, blue = 0);
    break;
  case Color::BLUE:
    (red = 0, green = 0, blue = 255);
    break;
  case Color::YELLOW:
    (red = 255, green = 255, blue = 0);
    break;
  case Color::CYAN:
    (red = 0, green = 255, blue = 255);
    break;
  case Color::MAGENTA:
    (red = 255, green = 0, blue = 255);
    break;
  }
  setAll(red, green, blue);
}