#ifndef __HW_CAMERA_H__
#define __HW_CAMERA_H__

// include files
#include <Arduino.h>
#include <esp_camera.h>
#include <esp_log.h>

// shared variables

// public function prototypes
void hw_camera_init(void);
uint32_t hw_camera_jpg_snapshot(uint8_t *buffer);
void hw_camera_raw_snapshot(uint8_t *buffer, uint32_t *width, uint32_t *height);

#endif // __HW_CAMERA_H__