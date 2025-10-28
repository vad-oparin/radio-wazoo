#include "access_point.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "filesystem.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "radio_wazoo_config.h"
#include "sd_card.h"
#include "tft_display.h"
#include "webserver.h"
#include <inttypes.h>
#include <stdio.h>

static const char *const TAG = "MAIN";

static spi_device_handle_t s_tft_spi_dummy = NULL;

void peripherals_init_task(void *pvParameters) {
    ESP_LOGI(TAG, "Peripherals initialization task started");
    vTaskDelay(pdMS_TO_TICKS(PERIPH_INIT_DELAY_MS));

    ESP_LOGI(TAG, "Initializing shared SPI bus (SPI2_HOST)");
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_BUS_MOSI,
        .miso_io_num = SPI_BUS_MISO,
        .sclk_io_num = SPI_BUS_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = SPI_MAX_TRANSFER_SIZE,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "SPI bus initialized");

    tft_display_config_t tft_config = TFT_DISPLAY_CONFIG();
    ESP_LOGI(TAG, "Adding TFT device to SPI bus (CS will be set HIGH)");
    spi_device_interface_config_t tft_dev_cfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .spics_io_num = tft_config.cs,
        .queue_size = 1,
    };
    ret = spi_bus_add_device(SPI2_HOST, &tft_dev_cfg, &s_tft_spi_dummy);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add TFT device: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "TFT CS line tied to idle (HIGH)");

    sd_card_config_t sd_config = SD_CARD_CONFIG();
    ret = sd_card_init(&sd_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "SD card initialization failed: %s", esp_err_to_name(ret));
    } else {
        vTaskDelay(pdMS_TO_TICKS(SD_CARD_SETTLE_DELAY_MS));
        sd_card_list_files();
    }

    ret = tft_display_init(&tft_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "TFT display initialization failed: %s", esp_err_to_name(ret));
    } else {
        vTaskDelay(pdMS_TO_TICKS(TFT_DISPLAY_SETTLE_DELAY_MS));
        ESP_LOGI(TAG, "Starting plasma effect");
        tft_display_plasma_effect(PLASMA_EFFECT_DURATION_MS);
    }

    vTaskDelete(NULL);
}

void app_loop(void) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== Radio Wazoo ===");
    ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());

    ESP_LOGI(TAG, "Initializing non-volatile storage...");
    ESP_ERROR_CHECK(nvs_init());

    ESP_LOGI(TAG, "Starting WiFi Access Point...");
    ESP_ERROR_CHECK(access_point_init());

    ESP_LOGI(TAG, "Initializing filesystem...");
    ESP_ERROR_CHECK(filesystem_init());

    ESP_LOGI(TAG, "Starting web server...");
    httpd_handle_t server = webserver_init();
    if (server == NULL) {
        ESP_LOGW(TAG, "Web server initialization failed");
    }

    ESP_LOGI(TAG, "Creating peripherals initialization task...");
    xTaskCreate(peripherals_init_task, "periph_init", PERIPH_INIT_TASK_STACK_SIZE, NULL, PERIPH_INIT_TASK_PRIORITY,
                NULL);

    ESP_LOGI(TAG, "Initialization complete. Entering main loop...");
    app_loop();
}
