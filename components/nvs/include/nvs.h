#ifndef NVS_H
#define NVS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Non-Volatile Storage
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t nvs_init(void);

#ifdef __cplusplus
}
#endif

#endif // NVS_H
