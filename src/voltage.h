#pragma once
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_adc/adc_oneshot.h>

#include "config.h"

adc_oneshot_unit_handle_t adc_unit_handle;
adc_cali_handle_t cali_handle = NULL;

void calibrate_voltage() {
  adc_oneshot_unit_init_cfg_t init_cfg = {
      .unit_id = ADC_UNIT,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_unit_handle));
  adc_oneshot_chan_cfg_t chan_cfg = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_unit_handle, ADC_CHANNEL, &chan_cfg));
  adc_cali_line_fitting_config_t cfg = {
      .unit_id = ADC_UNIT,
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cfg, &cali_handle));
}

float read_voltage() {
  int adc_reading = 0;
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    int temp;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_unit_handle, ADC_CHANNEL, &temp));
    adc_reading += temp;
  }
  adc_reading /= NO_OF_SAMPLES;
  int voltage_mv = 0;
  ESP_ERROR_CHECK(
      adc_cali_raw_to_voltage(cali_handle, adc_reading, &voltage_mv));
  voltage_mv *= DIVIDER_COEFFICIENT;
  voltage_mv += VOLTAGE_FIXUP;
  return voltage_mv / 1000.0;
}

const int N_POINTS = 22;
const float voltageTable[N_POINTS] = {
    4.20, 4.15, 4.11, 4.08, 4.02, 3.98, 3.95, 3.91, 3.87, 3.85, 3.84,
    3.82, 3.80, 3.79, 3.77, 3.75, 3.73, 3.71, 3.69, 3.61, 3.27, 3.02};
const int percentTable[N_POINTS] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50,
                                    45,  40, 35, 30, 25, 20, 15, 10, 5,  2,  0};

int voltageToPercent(float v) {
  if (v >= voltageTable[0])
    return 100;
  if (v <= voltageTable[N_POINTS - 1])
    return 0;
  for (int i = 0; i < N_POINTS - 1; i++) {
    float vHigh = voltageTable[i];
    float vLow = voltageTable[i + 1];
    if (v <= vHigh && v >= vLow) {
      int pHigh = percentTable[i];
      int pLow = percentTable[i + 1];
      float frac = (v - vLow) / (vHigh - vLow);
      return round(pLow + frac * (pHigh - pLow));
    }
  }
  return 0;
}

uint8_t get_battery_percentage() {
  float voltage = read_voltage();
  float perc = voltageToPercent(voltage);
  perc = constrain(perc, 0.0, 100.0);
  return perc;
}