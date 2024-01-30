#include "main.h"
#include "hw_camera.h"
#include "openmvrpc.h"

// constants
#define TAG           "main"

#define BUTTON_PIN    0

// static variables
static uint8_t jpg_buf[20480];
static uint16_t jpg_sz = 0;
static bool read_flag = false;

openmv::rpc_scratch_buffer<256> scratch_buffer;
openmv::rpc_callback_buffer<8> callback_buffer;
openmv::rpc_hardware_serial_uart_slave rpc_slave;

// static function declarations
static void print_memory(void);

size_t button_read_callback(void *out_data);
size_t jpeg_image_snapshot_callback(void *out_data);
size_t jpeg_image_read_callback(void *out_data);

// initialize hardware
void setup() {
  Serial.begin(115200);
  hw_camera_init();
  rpc_slave.register_callback(F("button_read"), button_read_callback);
  rpc_slave.register_callback(F("jpeg_image_snapshot"), jpeg_image_snapshot_callback);
  rpc_slave.register_callback(F("jpeg_image_read"), jpeg_image_read_callback);
  rpc_slave.begin();
  ESP_LOGI(TAG, "Setup complete");
}

// main loop
void loop() {
  if (read_flag) {
    rpc_slave.put_bytes(jpg_buf, jpg_sz, 10000);
    read_flag = false;
  }
  rpc_slave.loop();
}

// Print memory information
void print_memory() {
  ESP_LOGI(TAG, "Total heap: %u", ESP.getHeapSize());
  ESP_LOGI(TAG, "Free heap: %u", ESP.getFreeHeap());
  ESP_LOGI(TAG, "Total PSRAM: %u", ESP.getPsramSize());
  ESP_LOGI(TAG, "Free PSRAM: %d", ESP.getFreePsram());
}

// callback for digital_read
size_t button_read_callback(void *out_data) {
  uint8_t state = 1;

  state = !digitalRead(BUTTON_PIN);
  memcpy(out_data, &state, sizeof(state));
  return sizeof(state);
}

// take camera snapshot
size_t jpeg_image_snapshot_callback(void *out_data) {
  jpg_sz = hw_camera_jpg_snapshot(jpg_buf);
  memcpy(out_data, &jpg_sz, sizeof(jpg_sz));
  return sizeof(jpg_sz);
}

// start reading image
size_t jpeg_image_read_callback(void *out_data) {
  read_flag = true;
  return 0;
}
