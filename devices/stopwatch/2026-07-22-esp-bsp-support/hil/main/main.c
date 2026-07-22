/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define IMU_ACCE_BIT BIT0
#define IMU_GYRO_BIT BIT1
#define DISPLAY_CHUNK_ROWS 20
#define AUDIO_FRAMES 320
#define AUDIO_LOOPS 25

static const char *TAG = "stopwatch_hil";
static unsigned int failures;
static EventGroupHandle_t imu_events;
static float last_acce[3];
static float last_gyro[3];

static void record_check(const char *name, bool passed)
{
    ESP_LOGI(TAG, "HIL_CHECK name=%s status=%s", name, passed ? "PASS" : "FAIL");
    if (!passed) {
        ++failures;
    }
}

static bool probe_i2c(i2c_master_bus_handle_t bus, uint8_t address)
{
    esp_err_t ret = i2c_master_probe(bus, address, 100);
    ESP_LOGI(TAG, "HIL_I2C address=0x%02X status=%s", address, esp_err_to_name(ret));
    return ret == ESP_OK;
}

static void sensor_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    (void)handler_args;
    (void)base;
    sensor_data_t *data = (sensor_data_t *)event_data;
    if (id == SENSOR_ACCE_DATA_READY) {
        last_acce[0] = data->acce.x;
        last_acce[1] = data->acce.y;
        last_acce[2] = data->acce.z;
        xEventGroupSetBits(imu_events, IMU_ACCE_BIT);
    } else if (id == SENSOR_GYRO_DATA_READY) {
        last_gyro[0] = data->gyro.x;
        last_gyro[1] = data->gyro.y;
        last_gyro[2] = data->gyro.z;
        xEventGroupSetBits(imu_events, IMU_GYRO_BIT);
    }
}

static void test_i2c_and_power(void)
{
    esp_err_t ret = bsp_control_init();
    record_check("CONTROL_INIT", ret == ESP_OK);
    if (ret != ESP_OK) {
        return;
    }

    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    record_check("I2C_BUS", bus != NULL);
    if (bus == NULL) {
        return;
    }

    record_check("I2C_M5PM1", probe_i2c(bus, BSP_I2C_M5PM1_ADDRESS));
    bool ioe_found = probe_i2c(bus, BSP_I2C_M5IOE1_ADDRESS);
    if (!ioe_found) {
        ioe_found = probe_i2c(bus, BSP_I2C_M5IOE1_ADDRESS_ALT);
    }
    record_check("I2C_M5IOE1", ioe_found);
    record_check("I2C_TOUCH", probe_i2c(bus, BSP_I2C_TOUCH_ADDRESS));
    record_check("I2C_ES8311", probe_i2c(bus, BSP_I2C_AUDIO_ADDRESS));
    record_check("I2C_RTC", probe_i2c(bus, BSP_I2C_RTC_ADDRESS));
    record_check("I2C_BMI270", probe_i2c(bus, BSP_I2C_IMU_ADDRESS));

    uint16_t battery_mv = 0;
    uint16_t input_mv = 0;
    bsp_stopwatch_power_source_t source = BSP_STOPWATCH_POWER_SOURCE_UNKNOWN;
    esp_err_t battery_ret = ESP_FAIL;
    esp_err_t input_ret = ESP_FAIL;
    esp_err_t source_ret = ESP_FAIL;
    for (int attempt = 0; attempt < 10; ++attempt) {
        battery_ret = bsp_stopwatch_get_battery_voltage(&battery_mv);
        input_ret = bsp_stopwatch_get_input_voltage(&input_mv);
        source_ret = bsp_stopwatch_get_power_source(&source);
        if (battery_ret == ESP_OK && battery_mv >= 2500 && battery_mv <= 5000 &&
                input_ret == ESP_OK && input_mv <= 6000 &&
                source_ret == ESP_OK && source != BSP_STOPWATCH_POWER_SOURCE_NONE) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGI(TAG, "HIL_POWER battery_mv=%u input_mv=%u source=%u",
             battery_mv, input_mv, (unsigned int)source);
    record_check("BATTERY_VOLTAGE_READ", battery_ret == ESP_OK && battery_mv <= 6000);
    ESP_LOGI(TAG, "HIL_OBSERVATION battery_voltage_range=%s",
             battery_mv >= 2500 && battery_mv <= 5000 ? "EXPECTED" : "OUT_OF_RANGE");
    record_check("INPUT_VOLTAGE", input_ret == ESP_OK && input_mv <= 6000);
    record_check("POWER_SOURCE", source_ret == ESP_OK && source != BSP_STOPWATCH_POWER_SOURCE_NONE);
}

static void test_buttons(void)
{
    button_handle_t buttons[BSP_BUTTON_NUM] = {0};
    esp_err_t ret = bsp_iot_button_create(buttons, NULL, BSP_BUTTON_NUM);
    bool handles_valid = ret == ESP_OK;
    for (size_t i = 0; i < BSP_BUTTON_NUM; ++i) {
        handles_valid = handles_valid && buttons[i] != NULL;
    }
    record_check("BUTTON_HANDLES", handles_valid);

    int level_a = gpio_get_level(BSP_BUTTON_A_IO);
    int level_b = gpio_get_level(BSP_BUTTON_B_IO);
    ESP_LOGI(TAG, "HIL_BUTTON_IDLE a=%d b=%d", level_a, level_b);
    record_check("BUTTON_IDLE_LEVELS", level_a == 1 && level_b == 1);
}

static uint16_t panel_color(uint16_t rgb565)
{
    return (uint16_t)((rgb565 << 8) | (rgb565 >> 8));
}

static void test_display_and_touch(void)
{
    bsp_display_config_t config = {
        .max_transfer_sz = BSP_LCD_H_RES * DISPLAY_CHUNK_ROWS * sizeof(uint16_t),
    };
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_panel_io_handle_t io = NULL;
    esp_err_t ret = bsp_display_new(&config, &panel, &io);
    record_check("DISPLAY_INIT", ret == ESP_OK && panel != NULL && io != NULL);
    if (ret == ESP_OK) {
        ret = esp_lcd_panel_disp_on_off(panel, true);
        uint16_t *pixels = heap_caps_malloc(config.max_transfer_sz, MALLOC_CAP_DMA);
        bool draw_ok = ret == ESP_OK && pixels != NULL;
        for (int y = 0; draw_ok && y < BSP_LCD_V_RES; y += DISPLAY_CHUNK_ROWS) {
            int rows = BSP_LCD_V_RES - y;
            if (rows > DISPLAY_CHUNK_ROWS) {
                rows = DISPLAY_CHUNK_ROWS;
            }
            uint16_t color = y < BSP_LCD_V_RES / 3 ? 0xF800 :
                             y < (BSP_LCD_V_RES * 2) / 3 ? 0x07E0 : 0x001F;
            color = panel_color(color);
            for (int i = 0; i < BSP_LCD_H_RES * rows; ++i) {
                pixels[i] = color;
            }
            draw_ok = esp_lcd_panel_draw_bitmap(panel, 0, y, BSP_LCD_H_RES, y + rows, pixels) == ESP_OK;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        free(pixels);
        draw_ok = draw_ok && bsp_display_brightness_set(80) == ESP_OK;
        record_check("DISPLAY_DRAW", draw_ok);
    }

    esp_lcd_touch_handle_t touch = NULL;
    ret = bsp_touch_new(NULL, &touch);
    record_check("TOUCH_INIT", ret == ESP_OK && touch != NULL);
    if (ret == ESP_OK) {
        esp_lcd_touch_point_data_t point = {0};
        uint8_t point_count = 0;
        ret = esp_lcd_touch_read_data(touch);
        if (ret == ESP_OK) {
            ret = esp_lcd_touch_get_data(touch, &point, &point_count, 1);
        }
        ESP_LOGI(TAG, "HIL_TOUCH_SAMPLE points=%u x=%u y=%u", point_count, point.x, point.y);
        record_check("TOUCH_READ", ret == ESP_OK);
    }
}

static void test_imu_and_vibration(void)
{
    imu_events = xEventGroupCreate();
    record_check("IMU_EVENT_GROUP", imu_events != NULL);
    if (imu_events == NULL) {
        return;
    }

    bsp_sensor_config_t config = {
        .type = IMU_ID,
        .mode = MODE_POLLING,
        .period = 100,
    };
    sensor_handle_t sensor = NULL;
    esp_err_t ret = bsp_sensor_init(&config, &sensor);
    bool started = ret == ESP_OK && sensor != NULL;
    if (started) {
        started = iot_sensor_handler_register(sensor, sensor_event_handler, NULL) == ESP_OK;
    }
    if (started) {
        started = iot_sensor_start(sensor) == ESP_OK;
    }
    record_check("IMU_START", started);
    if (started) {
        EventBits_t bits = xEventGroupWaitBits(imu_events, IMU_ACCE_BIT | IMU_GYRO_BIT,
                                               pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG, "HIL_IMU acce=%.3f,%.3f,%.3f gyro=%.3f,%.3f,%.3f bits=0x%02X",
                 last_acce[0], last_acce[1], last_acce[2],
                 last_gyro[0], last_gyro[1], last_gyro[2], (unsigned int)bits);
        record_check("IMU_DATA", (bits & (IMU_ACCE_BIT | IMU_GYRO_BIT)) == (IMU_ACCE_BIT | IMU_GYRO_BIT));
    }

    ret = bsp_stopwatch_vibration_set(60);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_err_t stop_ret = bsp_stopwatch_vibration_set(0);
    record_check("VIBRATION_COMMAND", ret == ESP_OK && stop_ret == ESP_OK);
}

static void test_audio(void)
{
    esp_codec_dev_handle_t speaker = bsp_audio_codec_speaker_init();
    esp_codec_dev_handle_t microphone = bsp_audio_codec_microphone_init();
    record_check("AUDIO_CODEC", speaker != NULL && microphone != NULL && speaker == microphone);
    if (speaker == NULL || microphone == NULL) {
        return;
    }

    esp_codec_dev_sample_info_t format = {
        .sample_rate = 44100,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_err_t ret = esp_codec_dev_open(speaker, &format);
    ret = ret == ESP_OK ? esp_codec_dev_set_out_vol(speaker, 55) : ret;
    ret = ret == ESP_OK ? esp_codec_dev_set_in_gain(microphone, 30.0f) : ret;
    record_check("AUDIO_OPEN", ret == ESP_OK);
    if (ret != ESP_OK) {
        return;
    }

    int16_t tone[AUDIO_FRAMES];
    int16_t capture[AUDIO_FRAMES];
    for (int i = 0; i < AUDIO_FRAMES; ++i) {
        tone[i] = (i % 16) < 8 ? 3000 : -3000;
    }

    bool write_ok = true;
    bool read_ok = true;
    int peak = 0;
    uint64_t magnitude_sum = 0;
    for (int loop = 0; loop < AUDIO_LOOPS; ++loop) {
        write_ok = write_ok && esp_codec_dev_write(speaker, tone, sizeof(tone)) == ESP_OK;
        memset(capture, 0, sizeof(capture));
        esp_err_t read_ret = esp_codec_dev_read(microphone, capture, sizeof(capture));
        read_ok = read_ok && read_ret == ESP_OK;
        if (read_ret == ESP_OK) {
            for (int i = 0; i < AUDIO_FRAMES; ++i) {
                int magnitude = abs((int)capture[i]);
                if (magnitude > peak) {
                    peak = magnitude;
                }
                magnitude_sum += (uint32_t)magnitude;
            }
        }
    }
    uint32_t mean_magnitude = (uint32_t)(magnitude_sum / (AUDIO_FRAMES * AUDIO_LOOPS));
    ESP_LOGI(TAG, "HIL_AUDIO peak=%d mean_magnitude=%" PRIu32, peak, mean_magnitude);
    record_check("AUDIO_PLAYBACK_CALL", write_ok);
    record_check("AUDIO_MIC_CAPTURE", read_ok && peak > 50 && mean_magnitude > 2);
    record_check("AUDIO_CLOSE", esp_codec_dev_close(speaker) == ESP_OK);
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "STOPWATCH_HIL_BEGIN");
    test_i2c_and_power();
    test_buttons();
    test_display_and_touch();
    test_imu_and_vibration();
    test_audio();

    ESP_LOGI(TAG, "STOPWATCH_HIL_DONE status=%s failures=%u", failures == 0 ? "PASS" : "FAIL", failures);
    while (true) {
        ESP_LOGI(TAG, "STOPWATCH_HIL_HEARTBEAT failures=%u button_a=%d button_b=%d",
                 failures, gpio_get_level(BSP_BUTTON_A_IO), gpio_get_level(BSP_BUTTON_B_IO));
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
