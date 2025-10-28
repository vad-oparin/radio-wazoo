#include "tft_display.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <string.h>

static const char *const TAG = "TFT_DISPLAY";

static spi_device_handle_t s_spi_handle = NULL;
static tft_display_config_t s_config;
static bool s_initialized = false;

// ST7735 Commands
#define ST7735_NOP 0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT 0x11
#define ST7735_NORON 0x13
#define ST7735_INVOFF 0x20
#define ST7735_INVON 0x21
#define ST7735_DISPON 0x29
#define ST7735_CASET 0x2A
#define ST7735_RASET 0x2B
#define ST7735_RAMWR 0x2C
#define ST7735_COLMOD 0x3A
#define ST7735_MADCTL 0x36
#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color conversion RGB888 to RGB565
#define COLOR_TO_RGB565(c) (((c.red & 0xF8) << 8) | ((c.green & 0xFC) << 3) | (c.blue >> 3))

static void tft_gpio_set_dc(uint8_t level) {
    gpio_set_level(s_config.dc, level);
}

static void tft_gpio_set_rst(uint8_t level) {
    if (s_config.rst != GPIO_NUM_NC) {
        gpio_set_level(s_config.rst, level);
    }
}

static esp_err_t tft_spi_write_cmd(uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    tft_gpio_set_dc(0);
    t.length = 8;
    t.tx_buffer = &cmd;
    ret = spi_device_transmit(s_spi_handle, &t);

    return ret;
}

static esp_err_t tft_spi_write_data(const uint8_t *data, size_t len) {
    if (len == 0) {
        return ESP_OK;
    }

    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    tft_gpio_set_dc(1);
    t.length = len * 8;
    t.tx_buffer = data;
    ret = spi_device_transmit(s_spi_handle, &t);

    return ret;
}

static esp_err_t tft_spi_write_data_byte(uint8_t data) {
    return tft_spi_write_data(&data, 1);
}

static esp_err_t tft_st7735_init(void) {
    ESP_LOGI(TAG, "Initializing ST7735 controller");

    tft_gpio_set_rst(1);
    vTaskDelay(pdMS_TO_TICKS(10));
    tft_gpio_set_rst(0);
    vTaskDelay(pdMS_TO_TICKS(10));
    tft_gpio_set_rst(1);
    vTaskDelay(pdMS_TO_TICKS(120));

    tft_spi_write_cmd(ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    tft_spi_write_cmd(ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));

    tft_spi_write_cmd(ST7735_FRMCTR1);
    uint8_t frmctr1[] = {0x01, 0x2C, 0x2D};
    tft_spi_write_data(frmctr1, 3);

    tft_spi_write_cmd(ST7735_FRMCTR2);
    uint8_t frmctr2[] = {0x01, 0x2C, 0x2D};
    tft_spi_write_data(frmctr2, 3);

    tft_spi_write_cmd(ST7735_FRMCTR3);
    uint8_t frmctr3[] = {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    tft_spi_write_data(frmctr3, 6);

    tft_spi_write_cmd(ST7735_INVCTR);
    tft_spi_write_data_byte(0x07);

    tft_spi_write_cmd(ST7735_PWCTR1);
    uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    tft_spi_write_data(pwctr1, 3);

    tft_spi_write_cmd(ST7735_PWCTR2);
    tft_spi_write_data_byte(0xC5);

    tft_spi_write_cmd(ST7735_PWCTR3);
    uint8_t pwctr3[] = {0x0A, 0x00};
    tft_spi_write_data(pwctr3, 2);

    tft_spi_write_cmd(ST7735_PWCTR4);
    uint8_t pwctr4[] = {0x8A, 0x2A};
    tft_spi_write_data(pwctr4, 2);

    tft_spi_write_cmd(ST7735_PWCTR5);
    uint8_t pwctr5[] = {0x8A, 0xEE};
    tft_spi_write_data(pwctr5, 2);

    tft_spi_write_cmd(ST7735_VMCTR1);
    tft_spi_write_data_byte(0x0E);

    tft_spi_write_cmd(ST7735_INVOFF);

    tft_spi_write_cmd(ST7735_MADCTL);
    tft_spi_write_data_byte(0xC8);

    tft_spi_write_cmd(ST7735_COLMOD);
    tft_spi_write_data_byte(0x05);

    tft_spi_write_cmd(ST7735_GMCTRP1);
    uint8_t gmctrp1[] = {0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                         0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10};
    tft_spi_write_data(gmctrp1, 16);

    tft_spi_write_cmd(ST7735_GMCTRN1);
    uint8_t gmctrn1[] = {0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
                         0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10};
    tft_spi_write_data(gmctrn1, 16);

    tft_spi_write_cmd(ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));

    tft_spi_write_cmd(ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(TAG, "ST7735 initialization complete");

    return ESP_OK;
}

static esp_err_t tft_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    tft_spi_write_cmd(ST7735_CASET);
    uint8_t caset[] = {(x >> 8) & 0xFF, x & 0xFF, (x_end >> 8) & 0xFF, x_end & 0xFF};
    tft_spi_write_data(caset, 4);

    tft_spi_write_cmd(ST7735_RASET);
    uint8_t raset[] = {(y >> 8) & 0xFF, y & 0xFF, (y_end >> 8) & 0xFF, y_end & 0xFF};
    tft_spi_write_data(raset, 4);

    tft_spi_write_cmd(ST7735_RAMWR);

    return ESP_OK;
}

esp_err_t tft_display_init(const tft_display_config_t *config) {
    esp_err_t ret;

    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_initialized) {
        ESP_LOGW(TAG, "Display already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    memcpy(&s_config, config, sizeof(tft_display_config_t));

    ESP_LOGI(TAG, "Initializing TFT display (type=%d, %dx%d)", config->type, config->width, config->height);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->dc),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    if (config->rst != GPIO_NUM_NC) {
        io_conf.pin_bit_mask = (1ULL << config->rst);
        gpio_config(&io_conf);
    }

    if (config->backlight != GPIO_NUM_NC) {
        io_conf.pin_bit_mask = (1ULL << config->backlight);
        gpio_config(&io_conf);
        gpio_set_level(config->backlight, 1);
    }

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 26 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = config->cs,
        .queue_size = 7,
        .flags = SPI_DEVICE_NO_DUMMY,
    };

    ret = spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    switch (config->type) {
    case TFT_TYPE_ST7735:
        ret = tft_st7735_init();
        break;
    case TFT_TYPE_ST7789:
        ESP_LOGE(TAG, "ST7789 not yet implemented");
        ret = ESP_ERR_NOT_SUPPORTED;
        break;
    case TFT_TYPE_ILI9341:
        ESP_LOGE(TAG, "ILI9341 not yet implemented");
        ret = ESP_ERR_NOT_SUPPORTED;
        break;
    default:
        ESP_LOGE(TAG, "Unknown display type: %d", config->type);
        ret = ESP_ERR_INVALID_ARG;
        break;
    }

    if (ret == ESP_OK) {
        s_initialized = true;
        ESP_LOGI(TAG, "TFT display initialized successfully");
    }

    return ret;
}

esp_err_t tft_display_deinit(void) {
    if (!s_initialized) {
        ESP_LOGW(TAG, "Display not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_config.backlight != GPIO_NUM_NC) {
        gpio_set_level(s_config.backlight, 0);
    }

    spi_bus_remove_device(s_spi_handle);
    s_spi_handle = NULL;
    s_initialized = false;

    ESP_LOGI(TAG, "TFT display deinitialized");

    return ESP_OK;
}

esp_err_t tft_display_fill(tft_color_t color) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t rgb565 = COLOR_TO_RGB565(color);
    uint8_t hi = rgb565 >> 8;
    uint8_t lo = rgb565 & 0xFF;

    tft_set_addr_window(0, 0, s_config.width, s_config.height);

    uint8_t line_buffer[s_config.width * 2];
    for (int i = 0; i < s_config.width; i++) {
        line_buffer[i * 2] = hi;
        line_buffer[i * 2 + 1] = lo;
    }

    tft_gpio_set_dc(1);
    for (int y = 0; y < s_config.height; y++) {
        tft_spi_write_data(line_buffer, sizeof(line_buffer));
    }

    return ESP_OK;
}

esp_err_t tft_display_draw_pixel(uint16_t x, uint16_t y, tft_color_t color) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (x >= s_config.width || y >= s_config.height) {
        return ESP_ERR_INVALID_ARG;
    }

    tft_set_addr_window(x, y, 1, 1);

    uint16_t rgb565 = COLOR_TO_RGB565(color);
    uint8_t data[] = {rgb565 >> 8, rgb565 & 0xFF};
    tft_spi_write_data(data, 2);

    return ESP_OK;
}

esp_err_t tft_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (x + w > s_config.width || y + h > s_config.height) {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t rgb565 = COLOR_TO_RGB565(color);
    uint8_t hi = rgb565 >> 8;
    uint8_t lo = rgb565 & 0xFF;

    tft_set_addr_window(x, y, w, h);

    uint8_t line_buffer[w * 2];
    for (int i = 0; i < w; i++) {
        line_buffer[i * 2] = hi;
        line_buffer[i * 2 + 1] = lo;
    }

    tft_gpio_set_dc(1);
    for (int row = 0; row < h; row++) {
        tft_spi_write_data(line_buffer, sizeof(line_buffer));
    }

    return ESP_OK;
}

esp_err_t tft_display_backlight(bool state) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_config.backlight != GPIO_NUM_NC) {
        gpio_set_level(s_config.backlight, state ? 1 : 0);
    }

    return ESP_OK;
}

esp_err_t tft_display_test_pattern(void) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Drawing test pattern");

    uint16_t bar_height = s_config.height / 8;

    tft_display_fill_rect(0, bar_height * 0, s_config.width, bar_height, TFT_COLOR_RED);
    tft_display_fill_rect(0, bar_height * 1, s_config.width, bar_height, TFT_COLOR_GREEN);
    tft_display_fill_rect(0, bar_height * 2, s_config.width, bar_height, TFT_COLOR_BLUE);
    tft_display_fill_rect(0, bar_height * 3, s_config.width, bar_height, TFT_COLOR_YELLOW);
    tft_display_fill_rect(0, bar_height * 4, s_config.width, bar_height, TFT_COLOR_CYAN);
    tft_display_fill_rect(0, bar_height * 5, s_config.width, bar_height, TFT_COLOR_MAGENTA);
    tft_display_fill_rect(0, bar_height * 6, s_config.width, bar_height, TFT_COLOR_WHITE);
    tft_display_fill_rect(0, bar_height * 7, s_config.width, bar_height, TFT_COLOR_BLACK);

    ESP_LOGI(TAG, "Test pattern complete");

    return ESP_OK;
}

static inline float plasma_dist(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    return sqrtf(dx * dx + dy * dy);
}

static inline uint16_t plasma_color_map(float value) {
    int v = ((int)value) % 256;
    int r, g, b;

    if (v < 85) {
        r = v * 3;
        g = 255 - v * 3;
        b = 0;
    } else if (v < 170) {
        v -= 85;
        r = 255 - v * 3;
        g = 0;
        b = v * 3;
    } else {
        v -= 170;
        r = 0;
        g = v * 3;
        b = 255 - v * 3;
    }

    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

esp_err_t tft_display_plasma_effect(uint32_t duration_ms) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting plasma effect (duration=%lu ms)", duration_ms);

    float time = 0;
    float palette_shift = 0;
    uint32_t start_time = xTaskGetTickCount();
    uint32_t frame_count = 0;

    uint16_t *line_buffer = heap_caps_malloc(s_config.width * 2, MALLOC_CAP_DMA);
    if (!line_buffer) {
        ESP_LOGE(TAG, "Failed to allocate line buffer (%d bytes)", s_config.width * 2);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Line buffer allocated (%d bytes)", s_config.width * 2);

    do {
        tft_set_addr_window(0, 0, s_config.width, s_config.height);
        tft_gpio_set_dc(1);

        for (int y = 0; y < s_config.height; y++) {
            for (int x = 0; x < s_config.width; x++) {
                float value = sinf(plasma_dist(x + time, y, 128.0f, 128.0f) / 8.0f) +
                              sinf(plasma_dist(x, y, 64.0f, 64.0f) / 8.0f) +
                              sinf(plasma_dist(x, y + time / 7.0f, 192.0f, 64.0f) / 7.0f) +
                              sinf(plasma_dist(x, y, 192.0f, 100.0f) / 8.0f);

                value = (value + 4.0f) * 32.0f;
                uint16_t color = plasma_color_map(value + palette_shift);

                line_buffer[x] = (color >> 8) | (color << 8);
            }
            tft_spi_write_data((uint8_t *)line_buffer, s_config.width * 2);
        }

        time += 0.08f;
        palette_shift += 0.5f;
        frame_count++;

        if (duration_ms > 0) {
            uint32_t elapsed = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
            if (elapsed >= duration_ms) {
                break;
            }
        } else {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(20));

    } while (duration_ms > 0);

    heap_caps_free(line_buffer);

    uint32_t total_time = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
    ESP_LOGI(TAG, "Plasma effect complete (%lu frames, %.1f FPS)", frame_count,
             (float)frame_count * 1000.0f / total_time);

    return ESP_OK;
}
