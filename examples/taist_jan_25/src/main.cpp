#include <Arduino.h>
#include <task.h>
#include <queue.h>
#include "hw_mic.h"

// queue handle
QueueHandle_t evt_queue;

// task function
void sound_detect_task(void *pvParameter) {
  static int32_t samples[160];
  static unsigned int num_samples = 160;
  float avg_sample, base_sample;

  // initialize microphone
  hw_mic_init(16000);

  // prepare baseline with 100 rounds
  base_sample = 0;
  for (int i=0; i < 100; i++) {
    hw_mic_read(samples, &num_samples);
    avg_sample = 0;
    for (int j=0; j < num_samples; j++) {
      avg_sample += abs(samples[j]);
    }
    avg_sample /= num_samples;
    base_sample += avg_sample;
  }
  base_sample /= 100;

  // read microphone
  while (true) {
    hw_mic_read(samples, &num_samples);
    // do something with samples

    // send event to comm task

  }
}

// comm task
void comm_task(void *pvParameter) {
  float value;
  Serial.begin(115200);
  while (true) {
    // wait for event

    // do something with event

  }
}

void setup() {
  // initialize RTOS task
  evt_queue = xQueueCreate(10, sizeof(float));

  xTaskCreate(
    sound_detect_task,    // task function
    "hw_mic_task",  // name of task
    4096,           // stack size of task
    NULL,           // parameter of the task
    3,              // priority of the task
    NULL           // task handle to keep track of created task
  );

  xTaskCreate(
    comm_task,    // task function
    "comm_task",  // name of task
    4096,         // stack size of task
    NULL,         // parameter of the task
    2,            // priority of the task
    NULL          // task handle to keep track of created task
  );
}

void loop() {
  delay(100);
}
