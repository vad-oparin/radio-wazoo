#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TFT_TYPE_ST7735 = 0,
    TFT_TYPE_ST7789 = 1,
    TFT_TYPE_ILI9341 = 2,
} tft_display_type_t;

typedef struct {
    tft_display_type_t type;
    uint16_t width;
    uint16_t height;
    gpio_num_t mosi;
    gpio_num_t sclk;
    gpio_num_t cs;
    gpio_num_t dc;
    gpio_num_t rst;
    gpio_num_t backlight;
} tft_display_config_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} tft_color_t;

#define TFT_COLOR_RGB(r, g, b) ((tft_color_t){.red = (r), .green = (g), .blue = (b)})
#define TFT_COLOR_BLACK TFT_COLOR_RGB(0, 0, 0)
#define TFT_COLOR_WHITE TFT_COLOR_RGB(255, 255, 255)
#define TFT_COLOR_RED TFT_COLOR_RGB(255, 0, 0)
#define TFT_COLOR_GREEN TFT_COLOR_RGB(0, 255, 0)
#define TFT_COLOR_BLUE TFT_COLOR_RGB(0, 0, 255)
#define TFT_COLOR_YELLOW TFT_COLOR_RGB(255, 255, 0)
#define TFT_COLOR_CYAN TFT_COLOR_RGB(0, 255, 255)
#define TFT_COLOR_MAGENTA TFT_COLOR_RGB(255, 0, 255)

/**
 * @brief Initialize TFT display
 *
 * @param config Display configuration
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_init(const tft_display_config_t *config);

/**
 * @brief Deinitialize TFT display
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_deinit(void);

/**
 * @brief Fill entire screen with a color
 *
 * @param color Color to fill
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_fill(tft_color_t color);

/**
 * @brief Draw a pixel
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Pixel color
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_draw_pixel(uint16_t x, uint16_t y, tft_color_t color);

/**
 * @brief Draw a filled rectangle
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @param color Rectangle color
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, tft_color_t color);

/**
 * @brief Set backlight state
 *
 * @param state true = on, false = off
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_backlight(bool state);

/**
 * @brief Display test pattern
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_test_pattern(void);

/**
 * @brief Display animated plasma effect
 *
 * @param duration_ms Duration to run animation (0 = run once)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t tft_display_plasma_effect(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // TFT_DISPLAY_H