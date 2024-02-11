#include <Arduino.h>
#include <task.h>
#include <queue.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "hw_mic.h"

// constants
#define SOUND_SAMPLE_RATE 16000
#define SOUND_BUF_SZ      160
#define SOUND_WND_SZ      100

#define MQTT_BROKER       "broker.hivemq.com"
#define MQTT_PORT         1883
#define HIVEMQ_USERNAME   "taist_aiot_???"
#define MQTT_SOUND_TOPIC  "taist/aiot/sound/dev_???"
#define MQTT_HB_TOPIC     "taist/aiot/heartbeat/dev_???"
#define MQTT_CMD_TOPIC    "taist/aiot/command/dev_???"

#define WIFI_SSID         "Your SSID"
#define WIFI_PASSWORD     "Your Password"

// global variables
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);

// queue handle
QueueHandle_t evt_queue;

void on_cmd_received(char* topic, byte* payload, unsigned int length) {
  // ignored
}

// task to detect sound from microphone
void sound_detect_task(void *pvParameter) {
  static int32_t samples[SOUND_BUF_SZ];
  static unsigned int num_samples = SOUND_BUF_SZ;
  float avg_sample, base_sample;

  // initialize microphone
  hw_mic_init(SOUND_SAMPLE_RATE);

  // prepare baseline
  base_sample = 0;
  for (int i=0; i < SOUND_WND_SZ; i++) {
    num_samples = SOUND_BUF_SZ;
    hw_mic_read(samples, &num_samples);
    avg_sample = 0;
    for (int j=0; j < num_samples; j++) {
      avg_sample += abs(samples[j]);
    }
    avg_sample /= num_samples;
    base_sample += avg_sample;
  }
  base_sample /= SOUND_WND_SZ;

  // read microphone
  while (true) {
    num_samples = SOUND_BUF_SZ;
    // sample microphone
    hw_mic_read(samples, &num_samples);
    // do average of samples
    avg_sample = 0;
    for (int j=0; j < num_samples; j++) {
      avg_sample += abs(samples[j]);
    }
    avg_sample /= num_samples;
    // normalize to baseline
    avg_sample /= base_sample;
    // send event to comm task
    xQueueSend(evt_queue, &avg_sample, portMAX_DELAY);
  }
}

// task to detect sound events and send to MQTT 
void comm_task(void *pvParameter) {
  float value;
  const int BUF_SIZE = 25;
  float buf[BUF_SIZE];
  int buf_idx = 0;
  float avg_value;
  bool detected = false;
  static uint32_t prev_ms = 0; // last ms of sending MQTT

  // initialize serial and network
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(on_cmd_received);
  mqtt_client.connect(HIVEMQ_USERNAME);

  // initialize buffer
  for (int i=0; i < BUF_SIZE; i++) {
    buf[i] = 0;
  }
  while (true) {
    // wait for event
    xQueueReceive(evt_queue, &value, portMAX_DELAY);
    // 1. update circular buffer
    buf[buf_idx] = value; 
    buf_idx = (buf_idx + 1) % BUF_SIZE;
    // 2. do moving average with circular buffer
    avg_value = 0; 
    for (int i=0; i < BUF_SIZE; i++) {
      avg_value += buf[i];
    }
    avg_value /= BUF_SIZE;
    // 3. do threshold and edge detection
    if (avg_value > 0.5) { 
      detected = true;

    } else {
      detected = false;

    }
    // communicate with MQTT
    //Serial.printf("%d, %f, %d\n", millis(), value, detected);
    if (millis() - prev_ms > 2000) {
      prev_ms = millis();
      if (mqtt_client.connected()) {
        mqtt_client.publish(MQTT_HB_TOPIC, "1");
      }
    }
  }
}

void setup() {
  // prepare WiFi and MQTT


  // initialize RTOS task
  evt_queue = xQueueCreate(10, sizeof(float));

  // create tasks
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
    8192,         // stack size of task
    NULL,         // parameter of the task
    2,            // priority of the task
    NULL          // task handle to keep track of created task
  );
}

void loop() {
  // execute MQTT loop
  if (mqtt_client.connected()) {
    mqtt_client.loop();
    Serial.print("MQTT loop\n");
  }
  delay(1000);
}
