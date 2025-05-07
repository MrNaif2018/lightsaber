#pragma once
#include <Arduino.h>
#include <Audio.h>
#include <SD.h>

#include "config.h"

extern Audio audio;
extern uint32_t currentVolume;

enum : uint8_t {
  SET_VOLUME,
  IS_PLAYING,
  CONNECTTOHOST,
  CONNECTTOSD,
  CONNECTTOSPEECH,
  STOPSONG,
};

struct audioMessage {
  uint8_t cmd;
  const char *txt1;
  const char *txt2;
  const char *txt3;
  uint32_t value1;
  uint32_t value2;
  uint32_t ret;
};

extern audioMessage audioTxMessage, audioRxMessage;
extern QueueHandle_t audioSetQueue;
extern QueueHandle_t audioGetQueue;

void CreateQueues();

void audioTask(void *parameter);

void audioInit();

audioMessage transmitReceive(audioMessage msg);

void audioSetVolume(uint8_t vol);

bool audioIsPlaying();

bool audioConnecttohost(const char *host, const char *user = "",
                        const char *pwd = "");

bool audioConnecttoSD(const char *filename, uint32_t resumeFilePos = 0);

bool audioConnecttospeech(const char *speech, const char *lang = "en");

uint32_t audioStopSong();