#include "nvs.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *const TAG = "nvs";

esp_err_t nvs_init(void) {
    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Successfully initialized...");

    return ESP_OK;
}
