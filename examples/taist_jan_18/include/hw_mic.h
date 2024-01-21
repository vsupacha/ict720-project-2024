#ifndef __HW_MIC_H__
#define __HW_MIC_H__

// include files
#include <Arduino.h>
#include <driver/i2s.h>
#include <esp_log.h>

// shared variables

// public function prototypes
esp_err_t hw_mic_init(unsigned int sample_rate);
esp_err_t hw_mic_read(int32_t *samples, unsigned int *num_samples);

#endif  // __HW_MIC_H__