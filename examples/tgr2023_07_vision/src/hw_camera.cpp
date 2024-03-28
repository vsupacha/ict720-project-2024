#include "hw_camera.h"

// constants
#define TAG             "hw_camera"    

#define CAM_PWDN_PIN     -1
#define CAM_RESET_PIN    18
#define CAM_XCLK_PIN     14

#define CAM_SIOD_PIN     4
#define CAM_SIOC_PIN     5

#define CAM_Y9_PIN       15
#define CAM_Y8_PIN       16
#define CAM_Y7_PIN       17
#define CAM_Y6_PIN       12
#define CAM_Y5_PIN       10
#define CAM_Y4_PIN       8
#define CAM_Y3_PIN       9
#define CAM_Y2_PIN       11

#define CAM_VSYNC_PIN    6
#define CAM_HREF_PIN     7
#define CAM_PCLK_PIN     13

// static variables

// static function prototypes

// initialize OV2640 modeule
void hw_camera_init() {
    camera_config_t camera_config;

    // configure hw pins
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.pin_d0 = CAM_Y2_PIN;
    camera_config.pin_d1 = CAM_Y3_PIN;
    camera_config.pin_d2 = CAM_Y4_PIN;
    camera_config.pin_d3 = CAM_Y5_PIN;
    camera_config.pin_d4 = CAM_Y6_PIN;
    camera_config.pin_d5 = CAM_Y7_PIN;
    camera_config.pin_d6 = CAM_Y8_PIN;
    camera_config.pin_d7 = CAM_Y9_PIN;
    camera_config.pin_xclk = CAM_XCLK_PIN;
    camera_config.pin_pclk = CAM_PCLK_PIN;
    camera_config.pin_vsync = CAM_VSYNC_PIN;
    camera_config.pin_href = CAM_HREF_PIN;
    camera_config.pin_sscb_sda = CAM_SIOD_PIN;
    camera_config.pin_sscb_scl = CAM_SIOC_PIN;
    camera_config.pin_pwdn = CAM_PWDN_PIN;
    camera_config.pin_reset = CAM_RESET_PIN;
    camera_config.xclk_freq_hz = 20000000;
    camera_config.pixel_format = PIXFORMAT_JPEG;
    camera_config.grab_mode = CAMERA_GRAB_LATEST;

    // configure jpeg settings
    if (psramFound()) {
        camera_config.frame_size = FRAMESIZE_240X240;
        camera_config.jpeg_quality = 10;
        camera_config.fb_count = 2;
        camera_config.fb_location = CAMERA_FB_IN_PSRAM;
        ESP_LOGI(TAG, "PSRAM found, using %d frames", camera_config.fb_count);
    } else {
        camera_config.frame_size = FRAMESIZE_240X240;
        camera_config.jpeg_quality = 12;
        camera_config.fb_count = 1;
        camera_config.fb_location = CAMERA_FB_IN_DRAM;
        ESP_LOGI(TAG, "PSRAM not found, using %d frames", camera_config.fb_count);
    }

    // initialize camera
    esp_camera_deinit();
    delay(100);
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }

    // adjust parameters
    sensor_t *cam_sensor = esp_camera_sensor_get();
    cam_sensor->set_framesize(cam_sensor, FRAMESIZE_240X240);
    cam_sensor->set_brightness(cam_sensor, 1);     // -2 to 2
    cam_sensor->set_contrast(cam_sensor, 0);       // -2 to 2
    cam_sensor->set_saturation(cam_sensor, 0);     // -2 to 2
    cam_sensor->set_special_effect(cam_sensor, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    cam_sensor->set_whitebal(cam_sensor, 0);       // 0 = disable , 1 = enable
    cam_sensor->set_awb_gain(cam_sensor, 1);       // 0 = disable , 1 = enable
    cam_sensor->set_wb_mode(cam_sensor, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    cam_sensor->set_exposure_ctrl(cam_sensor, 1);  // 0 = disable , 1 = enable
    cam_sensor->set_aec2(cam_sensor, 0);           // 0 = disable , 1 = enable
    cam_sensor->set_ae_level(cam_sensor, 0);       // -2 to 2
    cam_sensor->set_aec_value(cam_sensor, 300);    // 0 to 1200
    cam_sensor->set_gain_ctrl(cam_sensor, 1);      // 0 = disable , 1 = enable
    cam_sensor->set_agc_gain(cam_sensor, 0);       // 0 to 30
    cam_sensor->set_gainceiling(cam_sensor, (gainceiling_t)0);  // 0 to 6
    cam_sensor->set_bpc(cam_sensor, 0);            // 0 = disable , 1 = enable
    cam_sensor->set_wpc(cam_sensor, 1);            // 0 = disable , 1 = enable
    cam_sensor->set_raw_gma(cam_sensor, 1);        // 0 = disable , 1 = enable
    cam_sensor->set_lenc(cam_sensor, 1);           // 0 = disable , 1 = enable
    cam_sensor->set_hmirror(cam_sensor, 0);        // 0 = disable , 1 = enable
    cam_sensor->set_vflip(cam_sensor, 0);          // 0 = disable , 1 = enable
    cam_sensor->set_dcw(cam_sensor, 1);            // 0 = disable , 1 = enable
    cam_sensor->set_colorbar(cam_sensor, 0);       // 0 = disable , 1 = enable
}

// camera snapshot in JPEG format
uint32_t hw_camera_jpg_snapshot(uint8_t *buffer) {
    camera_fb_t *fb = NULL;
    uint32_t fb_len;
    fb = esp_camera_fb_get();
    if (fb) {
        ESP_LOGI(TAG, "Camera capture success: %dx%d", fb->width, fb->height);
    } else {
        ESP_LOGE(TAG, "Camera capture failed");
    }
    memcpy(buffer, fb->buf, fb->len);
    fb_len = fb->len;
    esp_camera_fb_return(fb);
    return fb_len;
}

// camera snapshot in JPEG, then convert to BMP
void hw_camera_raw_snapshot(uint8_t *buffer, uint32_t *width, uint32_t *height) {
    camera_fb_t *fb = NULL;
    uint32_t fb_len;
    fb = esp_camera_fb_get();
    if (fb) {
        ESP_LOGI(TAG, "Camera capture success: %dx%d", fb->width, fb->height);
    } else {
        ESP_LOGE(TAG, "Camera capture failed");
    }
    bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, buffer);
    if (!converted) {
        ESP_LOGE(TAG, "BMP conversion failed");
        *width = 0;
        *height = 0;
    }
    esp_camera_fb_return(fb);
    *width = 240;
    *height = 240;
}