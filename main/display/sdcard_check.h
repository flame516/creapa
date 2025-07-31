#pragma once

#include "esp_err.h"
#include "sdmmc_cmd.h"

#ifdef __cplusplus
extern "C" {
#endif

// 初始化并挂载SD卡，成功返回ESP_OK，失败返回错误码
// out_card 可为NULL，若非NULL则返回挂载的卡对象指针
esp_err_t sdcard_init_and_mount(sdmmc_card_t **out_card);

// 文件读写函数
esp_err_t s_example_read_file(const char *path);
esp_err_t s_example_write_file(const char *path, const char *data);
esp_err_t s_example_read_png_file(const char *path);

// SD卡读写测试（可选调用）
void sdcard_test_rw(void);

// 测试读取PNG文件
void sdcard_test_read_png(const char *png_path);

// SD卡卸载和SPI释放
void sdcard_unmount_and_free(sdmmc_card_t *card);

#ifdef __cplusplus
}
#endif 