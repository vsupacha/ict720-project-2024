#include <Arduino.h>
#include <task.h>
#include <queue.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "hw_mic.h"

// constants
#define SOUND_SAMPLE_RATE 16000
#define SOUND_BUF_SZ      160
#define SOUND_WND_SZ      100

#define MQTT_BROKER       "192.168.137.60"
#define MQTT_PORT         1883
#define HA_USER           "ha_device"
#define HA_PASSWD         "ha_passwd"

#define DEV_NAME                "t-simcam-xxxx"
#define MQTT_DISCOVERY_TOPIC    "homeassistant/binary_sensor/%s/config"
#define MQTT_AVAILABLE_TOPIC    "homeassistant/binary_sensor/%s/available"
#define MQTT_STATE_TOPIC        "homeassistant/binary_sensor/%s/state"

#define WIFI_SSID         "HA_demo"
#define WIFI_PASSWORD     "home_assistant"

#define SOUND_DETECT_THRESHOLD   0.5
#define SOUND_DETECT_COUNT       5

// global variables
WiFiClient wifi_client;
PubSubClient mqtt_client(wifi_client);
JsonDocument json_doc;

// queue handle
QueueHandle_t evt_queue;

// callback function when command is received
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
  int detect_count = 0;
  bool detected = false; 
  uint32_t prev_ms = 0; // last ms of sending MQTT
  uint32_t prev_detect_ms = 0; // last ms of detecting sound event
  char topic_buf[256];

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
  mqtt_client.connect(DEV_NAME, HA_USER, HA_PASSWD);
  mqtt_client.setBufferSize(512);
  
  // MQTT discovery
  json_doc.clear();
  json_doc["name"] = DEV_NAME;
  json_doc["device_class"] = "motion";
  sprintf(topic_buf, MQTT_STATE_TOPIC, DEV_NAME);
  json_doc["state_topic"] = topic_buf;
  json_doc["unique_id"] = DEV_NAME;
  JsonObject dev_obj = json_doc["device"].to<JsonObject>();
  dev_obj["name"] = DEV_NAME;
  JsonArray iden_arr = dev_obj["identifiers"].to<JsonArray>();
  iden_arr.add(DEV_NAME);
  char config_topic[256];
  sprintf(config_topic, MQTT_DISCOVERY_TOPIC, DEV_NAME);
  char payload_buf[256];
  serializeJson(json_doc, payload_buf);
  boolean status = mqtt_client.publish(config_topic, payload_buf, strlen(payload_buf));

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
    if (avg_value > SOUND_DETECT_THRESHOLD) { 
      detect_count++;
      if (detect_count > SOUND_DETECT_COUNT) {
        detect_count = SOUND_DETECT_COUNT;
      }
    } else {
      detect_count--;
      if (detect_count < 0) {
        detect_count = 0;
      }
    }
    if (!detected) {
      if (detect_count > int(SOUND_DETECT_COUNT/2)) {
        detected = true;
        prev_detect_ms = millis();
        if (mqtt_client.connected()) {
          sprintf(topic_buf, MQTT_STATE_TOPIC, DEV_NAME);
          mqtt_client.publish(topic_buf, "ON");
        }
        Serial.println("Sound detected");
      }
    } else {
      if (detect_count == 0) {
        detected = false;
        if (mqtt_client.connected()) {
          sprintf(topic_buf, MQTT_STATE_TOPIC, DEV_NAME);
          mqtt_client.publish(topic_buf, "OFF");
        }
        Serial.println("Sound silent");
      }
    }
    // communicate with MQTT
    //Serial.printf("%d, %f, %d\n", millis(), value, detected);
    if (millis() - prev_ms > 2000) {
      prev_ms = millis();
      // MQTT availability
      if (mqtt_client.connected()) {
        sprintf(topic_buf, MQTT_AVAILABLE_TOPIC, DEV_NAME);
        mqtt_client.publish(topic_buf, "online");
      }
    }
  }
}

void setup() {
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
    4,            // priority of the task
    NULL          // task handle to keep track of created task
  );
}

void loop() {
  // execute MQTT loop
  if (mqtt_client.connected()) {
    mqtt_client.loop();
    Serial.println("MQTT loop");
  } else {
    Serial.println("MQTT disconnected");
  }
  delay(2000);
}