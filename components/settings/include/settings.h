#ifndef SETTINGS_H
#define SETTINGS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Settings storage
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t settings_init(void);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
