#include <Adafruit_MPU6050.h>
#include <Arduino.h>
#include <AsyncTCP.h>
#include <Audio.h>
#include <Button2.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <FS.h>
#include <NeoPixelBusLg.h>
#include <NetWizard.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>
#include <WebSerial.h>
#include <WiFiMulti.h>

#include "audioqueue.h"
#include "config.h"
#include "debug.h"
#include "led.h"
#include "voltage.h"

// timers
uint32_t blink_timer = 0, mpuTimer = 0;

void dumpHeap(const char *tag) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
  d_printf("[%s] total_free: %u, largest_free: %u, free_blocks: %u\n", tag,
           info.total_free_bytes, info.largest_free_block, info.free_blocks);
}

// main objects
NeoPixelBusLg<NeoGrbFeature, NeoWs2812xMethod> strip(NUM_PIXELS, LED_PIN);
Adafruit_MPU6050 mpu;

Button2 btn1;
Button2 btn2;

AsyncWebServer server(80);
NetWizard NW(&server);

Preferences preferences;

// MPU calculated params
float ACC = 0, GYR = 0;
const float STRIKE_LIGHT = 15.0;
const float STRIKE_STRONG = 20.0;
const float SWING_LIGHT = 2.0;
const float SWING_STRONG = 2.5;

// if updating, don't process much
bool updating = false;

// saved preferences
uint32_t currentColorMode = 0;
uint32_t currentColor = 0;
uint32_t currentVolume = 10;
uint32_t currentStation = 0;
uint32_t currentSDFile = 0;
bool internetRadioMode = false;

void onOTAStart() {
  audioStopSong();
  updating = true;
  for (int i = 0; i < NUM_PIXELS; i++) {
    strip.SetPixelColor(i, RgbColor(0, 0, 255));
  }
  strip.Show();
}

void onOTAEnd(bool success) {
  if (!success) {
    updating = false;
  }
}

String stations[] = {"http://mp3.ffh.de/radioffh/hqlivestream.mp3",
                     "http://stream.srg-ssr.ch/m/rsp/mp3_128",
                     "http://stream.radioparadise.com/mp3-128",
                     "http://nr9.newradio.it:9371/stream",
                     "http://media-ice.musicradio.com/ChillMP3"};

constexpr auto STATION_COUNT = sizeof(stations) / sizeof(stations[0]);

String SDFiles[] = {"/witcher.mp3", "/rammstein.mp3", "/imlerith.mp3"};

constexpr auto SD_FILE_COUNT = sizeof(SDFiles) / sizeof(SDFiles[0]);

// button states
volatile bool volUpActive = false;
volatile bool volDownActive = false;
uint8_t pressed_counter = 0;
uint32_t bothClickTime = 0;
bool doubleClickCandidate = false;
uint32_t lastComboActionTime = 0;

void onVolUpStart(Button2 &btn) { volUpActive = true; }
void onVolDownStart(Button2 &btn) { volDownActive = true; }

void onButtonPressed(Button2 &btn) {
  pressed_counter += 1;
  d_println("Button pressed, counter: " + String(pressed_counter));
  if (pressed_counter == 2) {
    d_println("Both buttons pressed");
    bothClickTime = millis();
    doubleClickCandidate = true;
  }
}

const uint32_t COMBO_SUPPRESS_MS = 500;
const uint32_t SINGLE_COMBO_WINDOW = 1000;
const uint32_t RADIO_LONGPRESS_MS = 2000;
const uint32_t WIFI_RESET_MS = 5000;

void announceIPAddress() {
  audioStopSong();
  audioConnecttospeech(
      ("My IP address is " + WiFi.localIP().toString()).c_str(), "en");
  while (audioIsPlaying()) {
    delay(100);
  }
}

// audio states when blade is on
// vibe plays either SD or internet radio
enum class AudioState { STATIC, VIBE, EFFECT };
AudioState currentAudioState = AudioState::STATIC,
           oldAudioState = AudioState::STATIC;
uint32_t file_pos = 0;
String effectType = "";
enum class AudioMode { SWORD, INTERLEAVE, SOUNDS };
int currentAudioMode = static_cast<int>(AudioMode::SWORD);

constexpr auto AUDIOMODE_COUNT =
    static_cast<std::underlying_type_t<Color>>(AudioMode::SOUNDS) + 1;

void resumeCurrentSong() {
  currentAudioState = AudioState::VIBE;
  if (static_cast<AudioMode>(currentAudioMode) == AudioMode::SWORD) {
    currentAudioMode = static_cast<int>(AudioMode::INTERLEAVE);
  }
  audioStopSong();
  if (internetRadioMode) {
    audioConnecttohost(stations[currentStation].c_str());
  } else {
    audioConnecttoSD(SDFiles[currentSDFile].c_str(), file_pos);
  }
}

void toggleInternetRadio() {
  internetRadioMode = !internetRadioMode;
  preferences.putBool("internet_radio", internetRadioMode);
  resumeCurrentSong();
}

void resetWiFiBinding() {
  audioStopSong();
  audioConnecttospeech("Resetting Wi-Fi binding", "en");
  while (audioIsPlaying()) {
    delay(100);
  }
  NW.reset();
  ESP.restart();
}

void switchAudioMode() {
  currentAudioMode = (currentAudioMode + 1) % AUDIOMODE_COUNT;
}

void onButtonReleased(Button2 &btn) {
  if (pressed_counter >= 1)
    pressed_counter -= 1;
  d_println("Button released, counter: " + String(pressed_counter));
  if (pressed_counter == 0 && doubleClickCandidate) {
    auto diff = millis() - bothClickTime;
    d_println("Both buttons released, time: " + String(diff));
    if (diff > WIFI_RESET_MS) {
      resetWiFiBinding();
    } else if (diff > RADIO_LONGPRESS_MS) {
      toggleInternetRadio();
    } else if (diff < RADIO_LONGPRESS_MS && diff > SINGLE_COMBO_WINDOW) {
      switchAudioMode();
    } else if (diff < SINGLE_COMBO_WINDOW) {
      announceIPAddress();
    }
    lastComboActionTime = millis();
    doubleClickCandidate = false;
  }
  if (btn == btn1) {
    volUpActive = false;
  }
  if (btn == btn2) {
    volDownActive = false;
  }
}

void increaseVolumeStep() {
  static uint32_t lastStep = 0;
  if (millis() - lastStep > 100) {
    lastStep = millis();
    if (currentVolume == 21) {
      return;
    }
    currentVolume = min(currentVolume + 1, 21ul);
    audioSetVolume(currentVolume);
    preferences.putUInt("volume", currentVolume);
  }
}

void decreaseVolumeStep() {
  static uint32_t lastStep = 0;
  if (millis() - lastStep > 100) {
    lastStep = millis();
    if (currentVolume == 0) {
      return;
    }
    currentVolume = max(currentVolume - 1, 0ul);
    audioSetVolume(currentVolume);
    preferences.putUInt("volume", currentVolume);
  }
}

bool sword_on = false;

void reportDebugInfo() {
  auto voltage = read_voltage();
  d_printf("Battery voltage: %.2fV\n", voltage);
  d_printf("Heap size: %u bytes\n",
           heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
  d_printf("Uptime: %lu seconds\n", millis() / 1000);
  d_printf("Free heap: %lu bytes\n", ESP.getFreeHeap());
  d_printf("CPU frequency: %lu MHz\n", ESP.getCpuFreqMHz());
  audioStopSong();
  audioConnecttospeech(
      ("Battery voltage is " + String(voltage) + " volts").c_str(), "en");
}

void showBatteryPercentage() {
  static uint32_t charge_timer = 0;
  if (millis() - charge_timer > 1000) {
    charge_timer = millis();
    auto percentage = get_battery_percentage();
    auto capacity = map(percentage, 100, 0, (NUM_PIXELS / 2 - 1), 1);
    setAll(0, 0, 0);
    for (char i = 0; i <= capacity; i++) {
      setPixel(i, red, green, blue);
      setPixel((NUM_PIXELS - 1 - i), red, green, blue);
      strip.Show();
      delay(25);
      btn1.loop();
      btn2.loop();
    }
    delay(100);
  }
}

void light_up() {
  setAll(0, 0, 0);
  for (int i = 0; i <= (NUM_PIXELS / 2 - 1); i++) {
    setPixel(i, red, green, blue);
    setPixel((NUM_PIXELS - 1 - i), red, green, blue);
    strip.Show();
    delay(10);
  }
}

void light_down() {
  setAll(red, green, blue);
  for (int i = (NUM_PIXELS / 2 - 1); i >= 0; i--) {
    setPixel(i, 0, 0, 0);
    setPixel((NUM_PIXELS - 1 - i), 0, 0, 0);
    strip.Show();
    delay(10);
  }
}

void onB1Click(Button2 &btn) {
  if (doubleClickCandidate ||
      millis() - lastComboActionTime < COMBO_SUPPRESS_MS)
    return;
  file_pos = 0;
  sword_on = !sword_on;
  if (sword_on) {
    audioStopSong();
    audioConnecttoSD("/poweron.mp3");
    delay(200);
    light_up();
    delay(200);
  } else {
    audioStopSong();
    audioConnecttoSD("/poweroff.mp3");
    delay(200);
    light_down();
    delay(200);
  }
}

void onB1DoubleClick(Button2 &btn) {
  currentColorMode = (currentColorMode + 1) % COLORMODE_COUNT;
  preferences.putUInt("color_mode", currentColorMode);
}

void onB1TripleClick(Button2 &btn) { reportDebugInfo(); }

void onB2Click(Button2 &btn) {
  if (doubleClickCandidate ||
      millis() - lastComboActionTime < COMBO_SUPPRESS_MS)
    return;
  currentColor = (currentColor + 1) % COLOR_COUNT;
  preferences.putUInt("color", currentColor);
  applyColor(static_cast<Color>(currentColor));
}

void onB2DoubleClick(Button2 &btn) {
  currentSDFile = (currentSDFile + 1) % SD_FILE_COUNT;
  internetRadioMode = false;
  preferences.putUInt("sd_file", currentSDFile);
  preferences.putBool("internet_radio", internetRadioMode);
  file_pos = 0;
  resumeCurrentSong();
}

void onB2TripleClick(Button2 &btn) {
  currentStation = (currentStation + 1) % STATION_COUNT;
  internetRadioMode = true;
  preferences.putUInt("station", currentStation);
  preferences.putBool("internet_radio", internetRadioMode);
  resumeCurrentSong();
}

void setup() {
  pinMode(SD_CS, OUTPUT);
  pinMode(KNOCK_PIN, INPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  SPI.setFrequency(1000000);
  dumpHeap("setup");
  calibrate_voltage();
  Serial.begin(115200);
  esp_reset_reason_t reason = esp_reset_reason();
  d_printf("Reset reason: %d\n", reason);
  preferences.begin("lightsaber", false);
  currentColorMode = preferences.getUInt("color_mode", 0);
  currentColor = preferences.getUInt("color", 0);
  currentVolume = preferences.getUInt("volume", 10);
  currentStation = preferences.getUInt("station", 0);
  currentSDFile = preferences.getUInt("sd_file", 0);
  internetRadioMode = preferences.getBool("internet_radio", false);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card init failed");
  }
  strip.Begin();
  dumpHeap("strip.begin");
  NW.setStrategy(NetWizardStrategy::BLOCKING);
  NW.autoConnect("Lightsaber_AP", "");
  dumpHeap("wifi init");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  audioInit();
  dumpHeap("audio init");
  btn1.setPressedHandler(onButtonPressed);
  btn1.setLongClickTime(1000);
  btn1.setLongClickDetectedHandler(onVolUpStart);
  btn1.setClickHandler(onB1Click);
  btn1.setDoubleClickHandler(onB1DoubleClick);
  btn1.setTripleClickHandler(onB1TripleClick);
  btn1.setReleasedHandler(onButtonReleased);

  btn2.setPressedHandler(onButtonPressed);
  btn2.setLongClickTime(1000);
  btn2.setLongClickDetectedHandler(onVolDownStart);
  btn2.setClickHandler(onB2Click);
  btn2.setDoubleClickHandler(onB2DoubleClick);
  btn2.setTripleClickHandler(onB2TripleClick);
  btn2.setReleasedHandler(onButtonReleased);
  btn1.begin(BTN1_PIN);
  btn2.begin(BTN2_PIN);
  strip.SetLuminance(150);
  applyColor(static_cast<Color>(currentColor));
  dumpHeap("strip set");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is LightSaber by MrNaif.");
  });
  ElegantOTA.begin(&server, "admin", "admin");
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onEnd(onOTAEnd);
  WebSerial.setAuthentication("admin", "admin");
  WebSerial.begin(&server);
  dumpHeap("route config");
  WebSerial.onMessage([&](uint8_t *data, size_t len) {
    Serial.printf("Received %u bytes from WebSerial: ", len);
    Serial.write(data, len);
    Serial.println();
    WebSerial.println("Received Data...");
    String d = "";
    for (size_t i = 0; i < len; i++) {
      d += char(data[i]);
    }
    WebSerial.println(d);
  });
  if (mpu.begin()) {
    d_println("MPU6050 initialized successfully");
  } else {
    d_println("MPU6050 initialization failed");
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_1000_DEG);
  server.begin();
}

void get_freq() {
  if (micros() - mpuTimer > 10000) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    float gx = g.gyro.x;
    float gy = g.gyro.y;
    float gz = g.gyro.z;

    GYR = sqrt(gx * gx + gy * gy + gz * gz);
    ACC = sqrt(ax * ax + ay * ay + az * az);

#if GYRO_DEBUG
    d_printf("GyroX: %f, GyroY: %f, GyroZ: %f, AccelX: %f, AccelY: %f, "
             "AccelZ: %f\n",
             gx, gy, gz, ax, ay, az);
    d_printf("ACC: %f, GYR: %f\n", ACC, GYR);
#endif
    mpuTimer = micros();
  }
}

void randomBlink() {
  if (BLINK_ALLOW && (millis() - blink_timer > BLINK_DELAY)) {
    blink_timer = millis();
    applyColorMode(static_cast<ColorMode>(currentColorMode));
  }
}

void strike_flash() {
  setAll(255, 255, 255);
  delay(FLASH_DELAY);
  setAll(red, blue, green);
}

constexpr unsigned long STRIKE_COOLDOWN = 300;
constexpr unsigned long SWING_COOLDOWN = 1000;

const unsigned long strikeDurations[12] = {1128, 744,  1056, 792,  1200, 1128,
                                           1128, 1008, 648,  1128, 984,  984};

const unsigned long swingDurations[12] = {576, 576, 576, 576, 576, 576,
                                          504, 504, 672, 600, 552, 672};

unsigned long lastStrikeTime = 0;
unsigned long lastSwingTime = 0;
unsigned long effectStart = 0;
unsigned long effectLength = 0;

void playEffect(const char *type, int count, unsigned long duration) {
  if (static_cast<AudioMode>(currentAudioMode) == AudioMode::SOUNDS) {
    return;
  }
  auto temp_pos = audioStopSong();
  if (currentAudioState != AudioState::EFFECT) {
    file_pos = temp_pos;
  }
  char fn[32];
  snprintf(fn, sizeof(fn), "/%s%d.mp3", type, count + 1);
  audioConnecttoSD(fn);
  effectStart = millis();
  effectLength = duration;
  effectType = type;
  if (currentAudioState != AudioState::EFFECT) {
    oldAudioState = currentAudioState;
    currentAudioState = AudioState::EFFECT;
  }
}

bool tryTrigger(float value, float lightTh, float strongTh,
                unsigned long &lastTime, unsigned long cooldown,
                const char *type, int countMax,
                const unsigned long *durations) {
  unsigned long now = millis();
  if (now - lastTime < cooldown)
    return false;
  if (value < lightTh)
    return false;
  int idx = random(countMax);
  unsigned long dur = (value > strongTh) ? durations[idx] : durations[idx];
  playEffect(type, idx, dur);
  if (strcmp(type, "clash") == 0) {
    strike_flash();
  }
  lastTime = now;
  return true;
}

void updateStatic() {
  if (currentAudioState == AudioState::VIBE &&
      static_cast<AudioMode>(currentAudioMode) == AudioMode::SWORD) {
    audioStopSong();
    currentAudioState = AudioState::STATIC;
  }
  if (currentAudioState == AudioState::STATIC) {
    if (!audioIsPlaying()) {
      audioConnecttoSD("/hum.mp3");
    }
  } else if (currentAudioState == AudioState::EFFECT) {
    if (!audioIsPlaying() || millis() - effectStart >= effectLength) {
      audioStopSong();
      currentAudioState = oldAudioState;
      if (currentAudioState == AudioState::VIBE) {
        resumeCurrentSong();
      }
    }
  } else if (currentAudioState == AudioState::VIBE && !audioIsPlaying()) {
    resumeCurrentSong();
  }
}

void loop() {
  static unsigned long last_print_time = millis();
  ElegantOTA.loop();
  if ((unsigned long)(millis() - last_print_time) > 2000) {
    dumpHeap("loop");
    d_printf("Free: %lu, MinFree: %lu\n", ESP.getFreeHeap(),
             ESP.getMinFreeHeap());
    last_print_time = millis();
  }
  WebSerial.loop();
  if (updating) {
    return;
  }
  btn1.loop();
  btn2.loop();
  if (volUpActive) {
    increaseVolumeStep();
  }
  if (volDownActive) {
    decreaseVolumeStep();
  }
  if (!sword_on) {
    showBatteryPercentage();
    return;
  }
  get_freq();
  randomBlink();
  bool canTrigger =
      currentAudioState != AudioState::EFFECT ||
      (currentAudioState == AudioState::EFFECT && effectType == "swing");
  if (canTrigger &&
      !tryTrigger(ACC, STRIKE_LIGHT, STRIKE_STRONG, lastStrikeTime,
                  STRIKE_COOLDOWN, "clash", 12, strikeDurations)) {
    tryTrigger(GYR, SWING_LIGHT, SWING_STRONG, lastSwingTime, SWING_COOLDOWN,
               "swing", 12, swingDurations);
  }
  updateStatic();
}

void audio_info(const char *info) {
  d_print("info        ");
  d_println(info);
}
void audio_id3data(const char *info) {
  d_print("id3data     ");
  d_println(info);
}
void audio_eof_mp3(const char *info) {
  d_print("eof_mp3     ");
  d_println(info);
}
void audio_showstation(const char *info) {
  d_print("station     ");
  d_println(info);
}
void audio_showstreamtitle(const char *info) {
  d_print("streamtitle ");
  d_println(info);
}
void audio_bitrate(const char *info) {
  d_print("bitrate     ");
  d_println(info);
}
void audio_commercial(const char *info) {
  d_print("commercial  ");
  d_println(info);
}
void audio_icyurl(const char *info) {
  d_print("icyurl      ");
  d_println(info);
}
void audio_lasthost(const char *info) {
  d_print("lasthost    ");
  d_println(info);
}
