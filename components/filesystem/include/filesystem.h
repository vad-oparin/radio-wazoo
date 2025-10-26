#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LittleFS filesystem
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t filesystem_init(void);

/**
 * @brief Unmount LittleFS partition
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t filesystem_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_H
