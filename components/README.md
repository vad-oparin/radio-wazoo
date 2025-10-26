# ESP-IDF Components

This directory contains custom reusable ESP-IDF components for the Radio Wazoo project. Each component is self-contained with its own `CMakeLists.txt` and can be used independently in other ESP-IDF projects.

## Component Overview

### access_point

WiFi Access Point management component.

**Features:**
- Configurable SSID and password
- Static IP address configuration
- Automatic DHCP server setup
- AP initialization and deinitialization

**API:**
```c
esp_err_t access_point_init(void);      // Initialize WiFi AP
esp_err_t access_point_deinit(void);    // Stop WiFi AP
```

**Configuration:** See `include/radio_wazoo_config.h` for SSID, password, and IP settings.

---

### webserver

HTTP web server component with static file serving.

**Features:**
- Static file serving with chunked transfer (1KB chunks)
- Wildcard URI matching for assets (`/assets/*`)
- Automatic content-type detection (HTML, CSS, JS, JSON, images)
- JSON error responses with HTTP status codes
- Memory-efficient stack-allocated buffers

**API:**
```c
httpd_handle_t webserver_init(void);              // Start HTTP server
esp_err_t webserver_stop(httpd_handle_t server);  // Stop HTTP server
```

**Supported MIME types:** HTML, CSS, JavaScript, JSON, PNG, JPG, SVG, ICO

---

### filesystem

LittleFS filesystem management component.

**Features:**
- LittleFS partition mount/unmount
- File read/write operations
- File existence checking
- File deletion

**API:**
```c
esp_err_t filesystem_init(void);                          // Mount LittleFS
esp_err_t filesystem_deinit(void);                        // Unmount LittleFS
esp_err_t filesystem_read_file(path, buffer, size, read); // Read file
esp_err_t filesystem_write_file(path, data, size);        // Write file
bool filesystem_file_exists(path);                        // Check existence
esp_err_t filesystem_delete_file(path);                   // Delete file
```

**Mount point:** `/littlefs`

---

### nvs

Non-Volatile Storage component with caching layer.

**Features:**
- Key-value storage with flash persistence
- In-memory caching for improved performance
- String and integer (i32) data types
- Batch commit operations

**API:**
```c
esp_err_t nvs_init(void);                           // Initialize NVS
esp_err_t nvs_cache_put_str(key, value);            // Store string
esp_err_t nvs_cache_get_str(key, buffer, max_len);  // Retrieve string
esp_err_t nvs_cache_put_i32(key, value);            // Store integer
esp_err_t nvs_cache_get_i32(key, value_ptr);        // Retrieve integer
esp_err_t nvs_cache_forget(key);                    // Delete key
esp_err_t nvs_cache_flush(void);                    // Commit changes
```

**Namespace:** `radio_wazoo` (configurable in implementation)

---

### settings

Application settings management component.

**Features:**
- CSV-based settings format
- Scope-based organization (Network, System, etc.)
- Type-safe value storage
- Default configuration loading

**API:**
```c
esp_err_t settings_init(void);  // Initialize settings
```

**Settings file:** `src/settings/settings.default.csv`
**Format:** `Scope, Name, Type, Value`

---

## Usage in Other Projects

To use these components in another ESP-IDF project:

1. Copy the desired component directory to your project's `components/` folder
2. Ensure dependencies are met (ESP-IDF core components)
3. Include the header file in your code:
   ```c
   #include "component_name.h"
   ```
4. The component will be automatically discovered and built by ESP-IDF

## Component Dependencies

- **access_point:** `esp_wifi`, `esp_netif`, `lwip`
- **webserver:** `esp_http_server`, `filesystem`
- **filesystem:** `esp_littlefs` (via IDF component manager)
- **nvs:** `nvs_flash`
- **settings:** `filesystem`, `nvs`

## Development Guidelines

When modifying components:

- Keep components decoupled and reusable
- Document all public APIs with doxygen comments
- Follow ESP-IDF coding conventions (4 spaces, 120 char lines)
- Place headers in `include/` directory
- Use `esp_err_t` return type for error handling
- Add `extern "C"` guards for C++ compatibility