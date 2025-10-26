#include "filesystem.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "radio_wazoo_config.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

esp_err_t filesystem_read_file(const char *path, char *buffer, size_t buffer_size, size_t *bytes_read) {
    if (path == NULL || buffer == NULL || buffer_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file '%s' for reading: %s", path, strerror(errno));
        return ESP_ERR_NOT_FOUND;
    }

    size_t read_count = fread(buffer, 1, buffer_size - 1, file);
    buffer[read_count] = '\0';

    if (ferror(file)) {
        ESP_LOGE(TAG, "Error reading file '%s'", path);
        fclose(file);
        return ESP_FAIL;
    }

    fclose(file);

    if (bytes_read != NULL) {
        *bytes_read = read_count;
    }

    ESP_LOGD(TAG, "Read %d bytes from '%s'", read_count, path);
    return ESP_OK;
}

esp_err_t filesystem_write_file(const char *path, const char *data, size_t data_size) {
    if (path == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = fopen(path, "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file '%s' for writing: %s", path, strerror(errno));
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, data_size, file);
    if (written != data_size) {
        ESP_LOGE(TAG, "Failed to write all data to '%s' (wrote %d of %d bytes)", path, written, data_size);
        fclose(file);
        return ESP_FAIL;
    }

    fclose(file);

    ESP_LOGD(TAG, "Wrote %d bytes to '%s'", data_size, path);
    return ESP_OK;
}

bool filesystem_file_exists(const char *path) {
    if (path == NULL) {
        return false;
    }

    struct stat st;
    return (stat(path, &st) == 0);
}

esp_err_t filesystem_delete_file(const char *path) {
    if (path == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (unlink(path) != 0) {
        if (errno == ENOENT) {
            ESP_LOGD(TAG, "File '%s' does not exist", path);
            return ESP_ERR_NOT_FOUND;
        }
        ESP_LOGE(TAG, "Failed to delete file '%s': %s", path, strerror(errno));
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Deleted file '%s'", path);
    return ESP_OK;
}
