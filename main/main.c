#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== Radio Wazoo ===");
    ESP_LOGI(TAG, "Free heap: %"PRIu32" bytes", esp_get_free_heap_size());
}
