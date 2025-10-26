#include "filesystem.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "radio_wazoo_config.h"
#include <sys/stat.h>

static const char *const TAG = "FILESYSTEM";

esp_err_t filesystem_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing LittleFS");
    ESP_LOGI(TAG, "First boot may take up to 15 seconds (formatting 1MB partition)...");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = LITTLEFS_BASE_PATH,
        .partition_label = LITTLEFS_PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    ESP_LOGI(TAG, "Mounting LittleFS partition '%s'...", LITTLEFS_PARTITION_LABEL);
    ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(LITTLEFS_PARTITION_LABEL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
        esp_vfs_littlefs_unregister(LITTLEFS_PARTITION_LABEL);
        return ret;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG, "LittleFS mounted successfully at %s", LITTLEFS_BASE_PATH);
    ESP_LOGI(TAG, "Web files should be uploaded to the storage partition separately");

    return ret;
}

esp_err_t filesystem_deinit(void) {
    ESP_LOGI(TAG, "Unmounting LittleFS partition '%s'...", LITTLEFS_PARTITION_LABEL);
    esp_err_t ret = esp_vfs_littlefs_unregister(LITTLEFS_PARTITION_LABEL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount LittleFS (%s)", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "LittleFS unmounted successfully");
    return ESP_OK;
}
