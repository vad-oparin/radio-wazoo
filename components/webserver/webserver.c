#include "webserver.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "filesystem.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *const TAG = "WEBSERVER";

#define CHUNK_SIZE 1024

static esp_err_t send_error_response(httpd_req_t *req, int status_code, const char *message) {
    char json_response[256];
    snprintf(json_response, sizeof(json_response), "{\"status\":%d,\"message\":\"%s\"}", status_code, message);

    switch (status_code) {
    case 404:
        httpd_resp_set_status(req, "404 Not Found");
        break;
    case 500:
        httpd_resp_set_status(req, "500 Internal Server Error");
        break;
    default:
        httpd_resp_set_status(req, "400 Bad Request");
        break;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    return ESP_FAIL;
}

static const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (ext == NULL)
        return "application/octet-stream";

    if (strcmp(ext, ".html") == 0)
        return "text/html";
    if (strcmp(ext, ".css") == 0)
        return "text/css";
    if (strcmp(ext, ".js") == 0)
        return "application/javascript";
    if (strcmp(ext, ".json") == 0)
        return "application/json";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".svg") == 0)
        return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0)
        return "image/x-icon";

    return "application/octet-stream";
}

static esp_err_t serve_static_file(httpd_req_t *req, const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return send_error_response(req, 404, "File not found");
    }

    const char *content_type = get_content_type(filepath);
    httpd_resp_set_type(req, content_type);

    char *chunk = malloc(CHUNK_SIZE);
    if (chunk == NULL) {
        ESP_LOGE(TAG, "Failed to allocate chunk buffer");
        fclose(file);
        return send_error_response(req, 500, "Out of memory");
    }

    size_t read_bytes;
    do {
        read_bytes = fread(chunk, 1, CHUNK_SIZE, file);
        if (read_bytes > 0) {
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send chunk");
                break;
            }
        }
    } while (read_bytes == CHUNK_SIZE);

    httpd_resp_send_chunk(req, NULL, 0);

    free(chunk);
    fclose(file);
    return ESP_OK;
}

static esp_err_t root_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET / request received");
    return serve_static_file(req, "/littlefs/www/index.html");
}

static esp_err_t static_handler(httpd_req_t *req) {
    char filepath[535];
    snprintf(filepath, sizeof(filepath), "/littlefs/www%s", req->uri);
    ESP_LOGI(TAG, "Static file request: %s", filepath);
    return serve_static_file(req, filepath);
}

// clang-format off
static const httpd_uri_t root_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL
};
static const httpd_uri_t static_uri = {
    .uri = "/assets/*",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};
// clang-format on

httpd_handle_t webserver_init(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);

    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Web server started successfully");

        if (httpd_register_uri_handler(server, &root_uri) != ESP_OK) {
            ESP_LOGI(TAG, "Failed to register URI handler: GET /");
        }
        ESP_LOGI(TAG, "Registered URI handler: GET /");

        if (httpd_register_uri_handler(server, &static_uri) != ESP_OK) {
            ESP_LOGI(TAG, "Failed to register URI handler: GET /assets/*");
        }
        ESP_LOGI(TAG, "Registered URI handler: GET /assets/*");

        esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap_netif != NULL) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK) {
                // clang-format off
                ESP_LOGI(TAG, "Open http://"IPSTR" in your browser", IP2STR(&ip_info.ip));
                // clang-format on
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
