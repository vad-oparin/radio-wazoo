#include "sd_card.h"
#include "driver/sdspi_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "radio_wazoo_config.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

static const char *const TAG = "SD_CARD";

static sdmmc_card_t *s_card = NULL;

esp_err_t sd_card_init(const sd_card_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Configuration is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_card != NULL) {
        ESP_LOGW(TAG, "SD card already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Mounting SD card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI2_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = config->cs;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = config->format_if_mount_failed,
        .max_files = config->max_files,
        .allocation_unit_size = 16 * 1024,
    };

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_CARD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "SD card mounted at %s", SD_CARD_MOUNT_POINT);
    sdmmc_card_print_info(stdout, s_card);

    return ESP_OK;
}

esp_err_t sd_card_deinit(void) {
    if (s_card == NULL) {
        ESP_LOGW(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_CARD_MOUNT_POINT, s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }

    s_card = NULL;
    ESP_LOGI(TAG, "SD card unmounted successfully");

    return ESP_OK;
}

esp_err_t sd_card_get_info(uint64_t *out_size_mb, char *out_name) {
    if (s_card == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (out_size_mb != NULL) {
        *out_size_mb = ((uint64_t)s_card->csd.capacity) * s_card->csd.sector_size / (1024 * 1024);
    }

    if (out_name != NULL) {
        snprintf(out_name, 32, "%s", s_card->cid.name);
    }

    return ESP_OK;
}

esp_err_t sd_card_list_files(void) {
    if (s_card == NULL) {
        ESP_LOGE(TAG, "SD card not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    DIR *dir = opendir(SD_CARD_MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "Failed to open directory: %s", SD_CARD_MOUNT_POINT);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Listing files in %s:", SD_CARD_MOUNT_POINT);
    ESP_LOGI(TAG, "%-30s %10s", "Name", "Size");
    ESP_LOGI(TAG, "---------------------------------------------");

    struct dirent *entry;
    struct stat file_stat;
    char filepath[512];
    int file_count = 0;
    uint64_t total_size = 0;

    while ((entry = readdir(dir)) != NULL) {
        int ret = snprintf(filepath, sizeof(filepath), "%s/%s", SD_CARD_MOUNT_POINT, entry->d_name);
        if (ret < 0 || ret >= sizeof(filepath)) {
            ESP_LOGW(TAG, "Path too long, skipping: %s", entry->d_name);
            continue;
        }

        if (stat(filepath, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                ESP_LOGI(TAG, "%-30s %10s", entry->d_name, "<DIR>");
            } else {
                ESP_LOGI(TAG, "%-30s %10ld", entry->d_name, file_stat.st_size);
                total_size += file_stat.st_size;
            }
            file_count++;
        }
    }

    closedir(dir);

    ESP_LOGI(TAG, "---------------------------------------------");
    ESP_LOGI(TAG, "Total: %d items, %llu bytes", file_count, total_size);

    return ESP_OK;
}
