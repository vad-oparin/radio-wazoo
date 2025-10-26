#include "settings.h"
#include "esp_log.h"

static const char *const TAG = "SETTINGS";

esp_err_t settings_init(void) {
    // esp_err_t ret;

    ESP_LOGI(TAG, "Settings storage Successfully initialized...");

    return ESP_OK;
}
