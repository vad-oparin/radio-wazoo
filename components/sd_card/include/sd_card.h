#ifndef SD_CARD_H
#define SD_CARD_H

#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t cs;
    int max_files;
    bool format_if_mount_failed;
} sd_card_config_t;

/**
 * @brief Initialize and mount SPI SD Card with FAT32 filesystem
 *
 * @note SPI bus must be initialized before calling this function
 *
 * @param config Configuration structure for SD card CS pin and mount options
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_init(const sd_card_config_t *config);

/**
 * @brief Unmount and deinitialize SD card
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_deinit(void);

/**
 * @brief Get SD card information
 *
 * @param out_size_mb Card size in megabytes (NULL to skip)
 * @param out_name Card name (NULL to skip, must be at least 32 bytes if provided)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_get_info(uint64_t *out_size_mb, char *out_name);

/**
 * @brief List all files in SD card root directory
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sd_card_list_files(void);

#ifdef __cplusplus
}
#endif

#endif // SD_CARD_H
