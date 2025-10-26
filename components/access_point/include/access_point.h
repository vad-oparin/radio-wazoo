#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi Access Point
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t access_point(void);

#ifdef __cplusplus
}
#endif

#endif // ACCESS_POINT_H
