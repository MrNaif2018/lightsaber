#include "audioqueue.h"

Audio audio;
audioMessage audioTxMessage, audioRxMessage;
QueueHandle_t audioSetQueue = NULL;
QueueHandle_t audioGetQueue = NULL;

void CreateQueues() {
  audioSetQueue = xQueueCreate(10, sizeof(struct audioMessage));
  audioGetQueue = xQueueCreate(10, sizeof(struct audioMessage));
}

void audioTask(void *parameter) {
  if (!audioSetQueue || !audioGetQueue) {
    Serial.println("Error: queues are not initialized");
    while (true) {
      ;
    }
  }

  struct audioMessage audioRxTaskMessage;
  struct audioMessage audioTxTaskMessage;

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(currentVolume); // 0...21

  while (true) {
    if (xQueueReceive(audioSetQueue, &audioRxTaskMessage, 1) == pdPASS) {
      if (audioRxTaskMessage.cmd == SET_VOLUME) {
        audioTxTaskMessage.cmd = SET_VOLUME;
        audio.setVolume(audioRxTaskMessage.value1);
        audioTxTaskMessage.ret = 1;
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == IS_PLAYING) {
        audioTxTaskMessage.cmd = IS_PLAYING;
        audioTxTaskMessage.ret = audio.isRunning();
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == CONNECTTOHOST) {
        audioTxTaskMessage.cmd = CONNECTTOHOST;
        const char *host = audioRxTaskMessage.txt1;
        const char *user = audioRxTaskMessage.txt2;
        const char *pwd = audioRxTaskMessage.txt3;
        audioTxTaskMessage.ret = audio.connecttohost(host, user, pwd);
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == CONNECTTOSD) {
        audioTxTaskMessage.cmd = CONNECTTOSD;
        audioTxTaskMessage.ret = audio.connecttoFS(SD, audioRxTaskMessage.txt1,
                                                   audioRxTaskMessage.value1);
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == CONNECTTOSPEECH) {
        audioTxTaskMessage.cmd = CONNECTTOSPEECH;
        audioTxTaskMessage.ret = audio.connecttospeech(audioRxTaskMessage.txt1,
                                                       audioRxTaskMessage.txt2);
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else if (audioRxTaskMessage.cmd == STOPSONG) {
        audioTxTaskMessage.cmd = STOPSONG;
        audioTxTaskMessage.ret = audio.stopSong();
        xQueueSend(audioGetQueue, &audioTxTaskMessage, portMAX_DELAY);
      } else {
        Serial.println("Error: unknown audioTaskMessage");
      }
    }
    audio.loop();
    vTaskDelay(1);
  }
}

void audioInit() {
  CreateQueues();
  xTaskCreatePinnedToCore(audioTask,      /* Function to implement the task
                                           */
                          "audioplay",    /* Name of the task */
                          8000,           /* Stack size in words */
                          NULL,           /* Task input parameter */
                          AUDIOTASK_PRIO, /* Priority of the task */
                          NULL,           /* Task handle. */
                          AUDIOTASK_CORE  /* Core where the task should run
                                           */
  );
}

audioMessage transmitReceive(audioMessage msg) {
  xQueueSend(audioSetQueue, &msg, portMAX_DELAY);
  if (xQueueReceive(audioGetQueue, &audioRxMessage, portMAX_DELAY) == pdPASS) {
    if (msg.cmd != audioRxMessage.cmd) {
      Serial.println("Error: wrong reply from message queue");
    }
  }
  return audioRxMessage;
}

void audioSetVolume(uint8_t vol) {
  audioTxMessage.cmd = SET_VOLUME;
  audioTxMessage.value1 = vol;
  audioMessage RX = transmitReceive(audioTxMessage);
  (void)RX;
}

bool audioIsPlaying() {
  audioTxMessage.cmd = IS_PLAYING;
  audioMessage RX = transmitReceive(audioTxMessage);
  return RX.ret;
}

bool audioConnecttohost(const char *host, const char *user, const char *pwd) {
  audioTxMessage.cmd = CONNECTTOHOST;
  audioTxMessage.txt1 = host;
  audioTxMessage.txt2 = user;
  audioTxMessage.txt3 = pwd;
  audioMessage RX = transmitReceive(audioTxMessage);
  return RX.ret;
}

bool audioConnecttoSD(const char *filename, uint32_t resumeFilePos) {
  audioTxMessage.cmd = CONNECTTOSD;
  audioTxMessage.txt1 = filename;
  audioTxMessage.value1 = resumeFilePos;
  audioMessage RX = transmitReceive(audioTxMessage);
  return RX.ret;
}

bool audioConnecttospeech(const char *speech, const char *lang) {
  audioTxMessage.cmd = CONNECTTOSPEECH;
  audioTxMessage.txt1 = speech;
  audioTxMessage.txt2 = lang;
  audioMessage RX = transmitReceive(audioTxMessage);
  return RX.ret;
}

uint32_t audioStopSong() {
  audioTxMessage.cmd = STOPSONG;
  audioMessage RX = transmitReceive(audioTxMessage);
  return RX.ret;
}
