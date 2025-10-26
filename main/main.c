#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"
#include <inttypes.h>
#include <stdio.h>

static const char *const TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "=== Radio Wazoo ===");
    ESP_LOGI(TAG, "Free heap: %" PRIu32 " bytes", esp_get_free_heap_size());

    ESP_LOGI(TAG, "Initializing non-volatile storage...");
    ESP_ERROR_CHECK(nvs_init());
}
