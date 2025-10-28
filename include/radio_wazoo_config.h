#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// SPI Bus configuration (shared between SD card and TFT display)
// Using safe GPIOs that avoid strapping pins (GPIO0, GPIO45, GPIO46)
#define SPI_BUS_MISO GPIO_NUM_37
#define SPI_BUS_MOSI GPIO_NUM_35
#define SPI_BUS_SCLK GPIO_NUM_36

// SPI SD Card configuration
#define SD_CARD_MOUNT_POINT "/sdcard"
#define SD_CARD_CONFIG()                                                                                               \
    {                                                                                                                  \
        .cs = GPIO_NUM_34,                                                                                             \
        .max_files = 5,                                                                                                \
        .format_if_mount_failed = false,                                                                               \
    }

// TFT Display configuration
// Shares SPI bus with SD card (MOSI, SCLK)
// Supported types: ST7735, ST7789, ILI9341
// Backlight controlled via GPIO1
#define TFT_DISPLAY_TYPE_ST7735 0
#define TFT_DISPLAY_TYPE_ST7789 1
#define TFT_DISPLAY_TYPE_ILI9341 2

#define TFT_DISPLAY_CONFIG()                                                                                           \
    {                                                                                                                  \
        .type = TFT_DISPLAY_TYPE_ST7735,                                                                               \
        .width = 128,                                                                                                  \
        .height = 160,                                                                                                 \
        .mosi = SPI_BUS_MOSI,                                                                                          \
        .sclk = SPI_BUS_SCLK,                                                                                          \
        .cs = GPIO_NUM_33,                                                                                             \
        .dc = GPIO_NUM_21,                                                                                             \
        .rst = GPIO_NUM_39,                                                                                            \
        .backlight = GPIO_NUM_1,                                                                                       \
    }

// WiFi AP Configuration
#define WIFI_AP_SSID "RadioWazooAP"
#define WIFI_AP_PASSWORD "12345678"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_MAX_CONN 4

// IP address (OCTETS)
#define AP_IP_1 192
#define AP_IP_2 168
#define AP_IP_3 4
#define AP_IP_4 1

// Macro
#define SET_AP_IP(ip_info)                                                                                             \
    do {                                                                                                               \
        IP4_ADDR(&(ip_info).ip, AP_IP_1, AP_IP_2, AP_IP_3, AP_IP_4);                                                   \
        IP4_ADDR(&(ip_info).gw, AP_IP_1, AP_IP_2, AP_IP_3, AP_IP_4);                                                   \
        IP4_ADDR(&(ip_info).netmask, 255, 255, 255, 0);                                                                \
    } while (0)

// Filesystem Configuration
#define LITTLEFS_BASE_PATH "/littlefs"
#define LITTLEFS_PARTITION_LABEL "storage"

// System Configuration
#define PERIPH_INIT_DELAY_MS 2000
#define PERIPH_INIT_TASK_STACK_SIZE 4096
#define PERIPH_INIT_TASK_PRIORITY 5
#define SPI_MAX_TRANSFER_SIZE 4000
#define SD_CARD_SETTLE_DELAY_MS 500
#define TFT_DISPLAY_SETTLE_DELAY_MS 500
#define PLASMA_EFFECT_DURATION_MS 30000
#define MAIN_LOOP_DELAY_MS 1000

#ifdef __cplusplus
}
#endif

#endif // CONFIG_H
