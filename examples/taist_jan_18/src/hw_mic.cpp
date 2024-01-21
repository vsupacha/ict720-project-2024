#include "hw_mic.h"

// constants
#define TAG             "hw_mic"

const i2s_port_t I2S_PORT = I2S_NUM_0;
#define I2S_SCK_PIN     41
#define I2S_WS_PIN      42
#define I2S_DOUT_PIN    2

#define DMA_BUF_COUNT	4
#define DMA_BUF_LENGTH	256

// initialize microphone
esp_err_t hw_mic_init(unsigned int sample_rate) {
    esp_err_t err = ESP_OK;

    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // Receive, not transfer
        .sample_rate = sample_rate,                         // 11.025/16/22.05/44.1 KHz
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,       // only work with 32bits
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,        // left channel
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // Interrupt level 1
        .dma_buf_count = DMA_BUF_COUNT,                     // number of buffers
        .dma_buf_len = DMA_BUF_LENGTH,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
    };
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,      // BCKL
        .ws_io_num = I2S_WS_PIN,        // LRCL
        .data_out_num = -1,             // not used (only for speakers)
        .data_in_num = I2S_DOUT_PIN     // DOUT
    };
    err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed installing driver: %d", err);
        return err;
    }
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed setting pin: %d", err);
        return err;
    }
    i2s_zero_dma_buffer(I2S_NUM_0);     // flush DMA buffer
    return err;
}

// read microphone
esp_err_t hw_mic_read(int32_t *samples, unsigned int *num_samples) {
    esp_err_t err = ESP_OK;
    size_t bytes_read = 0;
    err = i2s_read(I2S_PORT, (char *)samples, (*num_samples)*4, &bytes_read, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed reading I2S: %d", err);
        *num_samples = 0;
        return err;
    }
    *num_samples = bytes_read/4;
    return err;
}

