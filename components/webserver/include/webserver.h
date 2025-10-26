#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init and start HTTP web server
 *
 * @return httpd_handle_t Server handle or NULL on error
 */
httpd_handle_t webserver_init(void);

/**
 * @brief Stop HTTP web server
 *
 * @param server Server handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t webserver_stop(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif // WEBSERVER_H
