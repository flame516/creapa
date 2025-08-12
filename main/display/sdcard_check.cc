/* SD card and FAT filesystem example.
   This example uses SPI peripheral to communicate with SD card.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#define EXAMPLE_MAX_CHAR_SIZE    64
#define MOUNT_POINT "/sdcard"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t s_example_write_file(const char *path, const char *data)
{
    printf("[SD] Opening file for write: %s\n", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        printf("[SD] Failed to open file for writing: %s\n", path);
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    printf("[SD] File written: %s\n", path);
    return ESP_OK;
}

esp_err_t s_example_read_file(const char *path)
{
    printf("[SD] Opening file for read: %s\n", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        printf("[SD] Failed to open file for reading: %s\n", path);
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    printf("[SD] Read from file: '%s'\n", line);
    return ESP_OK;
}

// 读取PNG文件的函数
esp_err_t s_example_read_png_file(const char *path)
{
    printf("[SD] Opening PNG file for read: %s\n", path);
    FILE *f = fopen(path, "rb");  // 使用二进制模式读取
    if (f == NULL) {
        printf("[SD] Failed to open PNG file for reading: %s\n", path);
        return ESP_FAIL;
    }
    
    // 获取文件大小
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    printf("[SD] PNG file size: %ld bytes\n", file_size);
    
    // 读取PNG文件头（前8字节）
    unsigned char png_header[8];
    size_t bytes_read = fread(png_header, 1, 8, f);
    fclose(f);
    
    if (bytes_read != 8) {
        printf("[SD] Failed to read PNG header\n");
        return ESP_FAIL;
    }
    
    // 检查PNG文件头签名
    unsigned char png_signature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    bool is_valid_png = true;
    for (int i = 0; i < 8; i++) {
        if (png_header[i] != png_signature[i]) {
            is_valid_png = false;
            break;
        }
    }
    
    if (is_valid_png) {
        printf("[SD] Valid PNG file detected\n");
    } else {
        printf("[SD] Warning: File may not be a valid PNG\n");
    }
    
    printf("[SD] PNG file read successfully: %s\n", path);
    return ESP_OK;
}

// 前向声明
void sdcard_test_rw(void);
void sdcard_test_read_png(const char *png_path);

// SD卡初始化和挂载，成功返回ESP_OK，否则返回错误码
esp_err_t sdcard_init_and_mount(sdmmc_card_t **out_card) {
    esp_err_t ret;
    printf("[SD] sdcard_init_and_mount start\n");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    static sdmmc_card_t *card = NULL;
    const char mount_point[] = MOUNT_POINT;
    printf("[SD] Initializing SD card\n");
    printf("[SD] Using SPI peripheral\n");
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        printf("[SD] Failed to create a new on-chip LDO power control driver\n");
        return ret;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = 13,
        .miso_io_num = 11,
        .sclk_io_num = 12,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    spi_host_device_t spi_host = (spi_host_device_t)host.slot;
    printf("[SD] Initializing SPI bus...\n");
    ret = spi_bus_initialize(spi_host, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        printf("[SD] Failed to initialize bus. ret=%d\n", ret);
        return ret;
    }
    printf("[SD] SPI bus initialized.\n");
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = (gpio_num_t)14;
    slot_config.host_id = spi_host;
    printf("[SD] Mounting filesystem...\n");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        printf("[SD] Failed to mount filesystem. ret=%d\n", ret);
        if (ret == ESP_FAIL) {
            printf("[SD] Mount failed: ESP_FAIL\n");
        } else {
            printf("[SD] Mount failed: %s\n", esp_err_to_name(ret));
        }
        return ret;
    }
    printf("[SD] Filesystem mounted successfully!\n");
    sdcard_test_rw();
    if (out_card) *out_card = card;
    return ESP_OK;
}

// SD卡读写测试（可选调用）
void sdcard_test_rw(void) {
    printf("[SD] Try to write file...\n");
    s_example_write_file("/sdcard/hello.txt", "hello, world!\n");
    printf("[SD] Try to read file...\n");
    s_example_read_file("/sdcard/hello.txt");
}

// 测试读取PNG文件
void sdcard_test_read_png(const char *png_path) {
    printf("[SD] Testing PNG file read...\n");
    s_example_read_png_file(png_path);
}

// SD卡卸载和SPI释放
void sdcard_unmount_and_free(sdmmc_card_t *card) {
    printf("[SD] Unmounting filesystem...\n");
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    printf("[SD] Card unmounted\n");
    // 释放SPI总线
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_host_device_t spi_host = (spi_host_device_t)host.slot;
    spi_bus_free(spi_host);
    printf("[SD] SPI bus freed\n");
}

#ifdef __cplusplus
}
#endif

