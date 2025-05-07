#pragma once

// Pins config
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18
#define I2S_DOUT 26
#define I2S_BCLK 27
#define I2S_LRC 14
#define LED_PIN 13
#define NUM_PIXELS 120
#define BTN1_PIN 33
#define BTN2_PIN 32
#define KNOCK_PIN 35
#define VOLT_PIN 34

// Voltage measuring config
#define NO_OF_SAMPLES 100
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_6
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_DEFAULT
#define DIVIDER_COEFFICIENT 11
#define VOLTAGE_FIXUP 150

// Audio config
#define AUDIOTASK_PRIO 2
#define AUDIOTASK_CORE 0

// Blink config

#define BLINK_ALLOW 1
#define BLINK_AMPL 40
#define BLINK_DELAY 30
#define FLASH_DELAY 80

#define GYRO_DEBUG 0