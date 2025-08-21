#ifdef __cplusplus
extern "C" {
#endif

#include "play_video_anim.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>

/* 分辨率 */
#define FRAME_WIDTH  160
#define FRAME_HEIGHT 275

/* 动画组路径（直接使用 ESP-IDF VFS 的挂载点） */
static const struct {
    const char *path_pattern;
    int frame_count;
} sd_anim_groups[] = {
    {"/sdcard/video/group0/out%03d.bin", 142},
    {"/sdcard/video/group1/out%03d.bin", 142},
};
static const int sd_group_count = (int)(sizeof(sd_anim_groups) / sizeof(sd_anim_groups[0]));

/* 全局保存当前动画，防止立即丢失 */
static sd_video_anim_t *g_anim = NULL;
static const char *TAG_PLAY = "play_video";

/* 定时器回调 */
static void sd_video_anim_cb(lv_timer_t *timer) {
    

    if (!timer) return;
    sd_video_anim_t *anim = (sd_video_anim_t *)lv_timer_get_user_data(timer);
    if (!anim) return;

    ESP_LOGI(TAG_PLAY, "timer fired, frame=%d", anim->frame_idx);
    char bin_path[128];
    snprintf(bin_path, sizeof(bin_path), anim->path_pattern, anim->frame_idx + 1);

    FILE *file = fopen(bin_path, "rb");
    if (!file) {
        ESP_LOGW(TAG_PLAY, "open frame failed: %s", bin_path);
        anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
        return;
    }

    uint32_t px_size;
#if LV_COLOR_DEPTH == 16
    px_size = 2;
#elif LV_COLOR_DEPTH == 24
    px_size = 3;
#elif LV_COLOR_DEPTH == 32
    px_size = 4;
#else
    px_size = 2;
#endif

    uint32_t to_read = (uint32_t)(FRAME_WIDTH * FRAME_HEIGHT * px_size);
    size_t br = fread(anim->frame_buf, 1, to_read, file);
    fclose(file);
    if (br != to_read) {
        anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
        return;
    }

    /* 更新 image 的数据源或请求重绘 */
    if (anim->image_obj) {
        lv_image_set_src(anim->image_obj, &anim->img_dsc);
        lv_obj_invalidate(anim->image_obj);
    }

    anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
}


/* 创建动画（单一路径模式） */
sd_video_anim_t* create_sd_video_anim(const char *path_pattern, int frame_count, uint32_t interval_ms) {
    printf("[play_video] create_sd_video_anim path=%s frames=%d interval=%u\n",
           path_pattern ? path_pattern : "(null)", frame_count, (unsigned)interval_ms);
    if (!path_pattern || frame_count <= 0) {
        ESP_LOGE(TAG_PLAY, "invalid params: pattern=%p, frame_count=%d", (void*)path_pattern, frame_count);
        return NULL;
    }

    /* 根据 LV_COLOR_DEPTH 自动选择颜色格式和像素大小 */
    lv_color_format_t cf;
    uint32_t px_size;
#if LV_COLOR_DEPTH == 16
    cf = LV_COLOR_FORMAT_RGB565;
    px_size = 2;
#elif LV_COLOR_DEPTH == 24
    cf = LV_COLOR_FORMAT_RGB888;
    px_size = 3;
#elif LV_COLOR_DEPTH == 32
    cf = LV_COLOR_FORMAT_XRGB8888;
    px_size = 4;
#else
    ESP_LOGE(TAG_PLAY, "unsupported LV_COLOR_DEPTH=%d", LV_COLOR_DEPTH);
    return NULL;
#endif

    /* 分配动画结构体 */
    sd_video_anim_t *anim = (sd_video_anim_t *)malloc(sizeof(sd_video_anim_t));
    if (!anim) {
        ESP_LOGE(TAG_PLAY, "alloc anim failed");
        return NULL;
    }
    memset(anim, 0, sizeof(sd_video_anim_t));

    strncpy(anim->path_pattern, path_pattern, sizeof(anim->path_pattern) - 1);
    anim->frame_count = frame_count;

    /* 分配帧缓冲 */
    size_t buf_size = (size_t)FRAME_WIDTH * (size_t)FRAME_HEIGHT * px_size;
    anim->frame_buf = (lv_color_t *)malloc(buf_size);
    if (!anim->frame_buf) {
        ESP_LOGE(TAG_PLAY, "alloc frame buf failed (%zu bytes)", buf_size);
        free(anim);
        return NULL;
    }

    /* 创建 image 对象显示帧 */
    anim->image_obj = lv_image_create(lv_scr_act());
    if (!anim->image_obj) {
        ESP_LOGE(TAG_PLAY, "lv_image_create failed");
        free(anim->frame_buf);
        free(anim);
        return NULL;
    }
    lv_obj_set_size(anim->image_obj, FRAME_WIDTH, FRAME_HEIGHT);
    lv_obj_center(anim->image_obj);
    /* 初始化图片描述符 */
    memset(&anim->img_dsc, 0, sizeof(anim->img_dsc));
    anim->img_dsc.header.w = FRAME_WIDTH;
    anim->img_dsc.header.h = FRAME_HEIGHT;
    anim->img_dsc.header.cf = cf;
    anim->img_dsc.data_size = buf_size;
    anim->img_dsc.data = (const uint8_t *)anim->frame_buf;
    ESP_LOGI(TAG_PLAY, "image created size=%dx%d, pattern=%s, frames=%d",
             FRAME_WIDTH, FRAME_HEIGHT, path_pattern, frame_count);
    printf("[play_video] image created and centered (%dx%d)\n", FRAME_WIDTH, FRAME_HEIGHT);

    /* 预加载第一帧 */
    char first_path[128];
    snprintf(first_path, sizeof(first_path), anim->path_pattern, 1);
    FILE *f = fopen(first_path, "rb");
    if (f) {
        uint32_t want = (uint32_t)(FRAME_WIDTH * FRAME_HEIGHT * px_size);
        size_t br = fread(anim->frame_buf, 1, want, f);
        if (br == want) {
            lv_image_set_src(anim->image_obj, &anim->img_dsc);
            lv_obj_invalidate(anim->image_obj);
        }
        fclose(f);
    }

    anim->timer = lv_timer_create(sd_video_anim_cb, interval_ms, anim);
    if (!anim->timer) {
        ESP_LOGE(TAG_PLAY, "lv_timer_create failed");
        if (anim->image_obj) lv_obj_del(anim->image_obj);
        free(anim->frame_buf);
        free(anim);
        return NULL;
    }
    ESP_LOGI(TAG_PLAY, "timer created, period=%u ms", (unsigned)interval_ms);
    printf("[play_video] timer created period=%u ms\n", (unsigned)interval_ms);

    return anim;
}

/* 创建动画（按 group 选择） */
sd_video_anim_t* create_sd_video_anim_by_group(int group_index, uint32_t interval_ms) {
    printf("[play_video] create_by_group group=%d interval=%u (group_count=%d)\n",
           group_index, (unsigned)interval_ms, sd_group_count);
    if (group_index < 0 || group_index >= sd_group_count) {
        ESP_LOGE(TAG_PLAY, "group index out of range: %d (max=%d)", group_index, sd_group_count - 1);
        printf("[play_video] group index out of range: %d\n", group_index);
        return NULL;
    }

    /* 如果有旧动画，先释放 */
    if (g_anim) {
        ESP_LOGI(TAG_PLAY, "stopping previous animation");
        stop_sd_video_anim(g_anim);
        g_anim = NULL;
    }

    g_anim = create_sd_video_anim(sd_anim_groups[group_index].path_pattern,
                                  sd_anim_groups[group_index].frame_count,
                                  interval_ms);
    if (g_anim) {
        ESP_LOGI(TAG_PLAY, "animation started: group=%d, period=%u ms", group_index, (unsigned)interval_ms);
        printf("[play_video] animation started group=%d period=%u ms\n", group_index, (unsigned)interval_ms);
    } else {
        ESP_LOGE(TAG_PLAY, "animation start failed: group=%d", group_index);
        printf("[play_video] animation start failed group=%d\n", group_index);
    }
    return g_anim; // 即使你不接收，也能保证动画活着
}

/* 停止动画 */
void stop_sd_video_anim(sd_video_anim_t *anim) {
    if (!anim) return;
    if (anim->timer) lv_timer_del(anim->timer);
    if (anim->canvas_obj) lv_obj_del(anim->canvas_obj);
    if (anim->frame_buf) free(anim->frame_buf);
    free(anim);
}

#ifdef __cplusplus
}
#endif
