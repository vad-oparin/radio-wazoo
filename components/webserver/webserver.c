#include "webserver.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *const TAG = "WEBSERVER";

httpd_handle_t webserver_init(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Web server started successfully");
        ESP_LOGI(TAG, "Open http://192.168.4.1 in your browser");
    } else {
        ESP_LOGE(TAG, "Failed to start web server");
    }

    return server;
}

esp_err_t webserver_stop(httpd_handle_t server) {
    if (server) {
        return httpd_stop(server);
    }

    return ESP_OK;
}
