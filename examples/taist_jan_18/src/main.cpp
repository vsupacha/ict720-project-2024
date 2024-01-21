#include <Arduino.h>
#include <esp_log.h>
#include "hw_mic.h"

#define TAG "main"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  hw_mic_init(16000);
}

void loop() {
  static int32_t samples[160];
  static uint32_t num_samples = 160;
  hw_mic_read(samples, &num_samples);
  uint32_t avg_sound = 0;
  for (int i=0; i < num_samples; i++) {
    avg_sound += abs(samples[i]);
  }
  avg_sound /= num_samples;
  Serial.println(avg_sound);
  delay(1);
}
