#include "nvs.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

static const char *const TAG = "NVS";
static const char *const NVS_NAMESPACE = "cache";

esp_err_t nvs_init(void) {
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Successfully initialized...");

    return ESP_OK;
}

esp_err_t nvs_cache_put_str(const char *key, const char *value) {
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_str(handle, key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set string key '%s': %s", key, esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Stored string key '%s'", key);
    }

    return ret;
}

esp_err_t nvs_cache_get_str(const char *key, char *value, size_t max_len) {
    nvs_handle_t handle;
    esp_err_t ret;
    size_t required_size = max_len;

    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_get_str(handle, key, value, &required_size);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Retrieved string key '%s'", key);
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "Key '%s' not found", key);
    } else {
        ESP_LOGE(TAG, "Failed to get string key '%s': %s", key, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t nvs_cache_put_i32(const char *key, int32_t value) {
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_set_i32(handle, key, value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set i32 key '%s': %s", key, esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Stored i32 key '%s' = %ld", key, (long)value);
    }

    return ret;
}

esp_err_t nvs_cache_get_i32(const char *key, int32_t *value) {
    nvs_handle_t handle;
    esp_err_t ret;

    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_get_i32(handle, key, value);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Retrieved i32 key '%s' = %ld", key, (long)*value);
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "Key '%s' not found", key);
    } else {
        ESP_LOGE(TAG, "Failed to get i32 key '%s': %s", key, esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t nvs_cache_forget(const char *key) {
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_erase_key(handle, key);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to erase key '%s': %s", key, esp_err_to_name(ret));
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Erased key '%s'", key);
    }

    return ret;
}

esp_err_t nvs_cache_flush(void) {
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Flushed NVS cache");
    }

    return ret;
}
