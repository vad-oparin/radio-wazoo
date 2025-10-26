#include "access_point.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "filesystem.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "webserver.h"
#include <inttypes.h>
#include <stdio.h>

static const char *const TAG = "MAIN";

void app_loop(void) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    webserver_init();

    ESP_LOGI(TAG, "Initialization complete. Entering main loop...");
    app_loop(); // never returns
}
