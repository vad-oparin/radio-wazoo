#include "webserver.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "filesystem.h"
#include <string.h>

static const char *const TAG = "WEBSERVER";

#define HTML_BUFFER_SIZE 4096

static esp_err_t root_handler(httpd_req_t *req) {
    char html_buffer[HTML_BUFFER_SIZE];
    size_t bytes_read = 0;

    esp_err_t ret = filesystem_read_file("/littlefs/www/index.html", html_buffer, sizeof(html_buffer), &bytes_read);

    if (ret != ESP_OK) {
        const char *error_msg = "<html><body><h1>404 Not Found</h1><p>index.html not found on filesystem</p></body></html>";
        httpd_resp_send_404(req);
        httpd_resp_send(req, error_msg, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_buffer, bytes_read);
    return ESP_OK;
}

httpd_handle_t webserver_init(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Web server started successfully");

        httpd_uri_t root_uri = {.uri = "/", .method = HTTP_GET, .handler = root_handler, .user_ctx = NULL};
        httpd_register_uri_handler(server, &root_uri);

        ESP_LOGI(TAG, "Registered URI handler: GET /");

        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap_netif != NULL) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
                ESP_LOGI(TAG, "Open http://" IPSTR " in your browser", IP2STR(&ip_info.ip));
            }
        }
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
