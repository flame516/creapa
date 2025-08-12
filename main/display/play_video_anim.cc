#ifdef __cplusplus
extern "C" {
#endif

/* ---- 关键：强制 1 字节对齐，避免 lv_color_t 结构体被补齐 ---- */
#pragma pack(push, 1)

#include "play_video_anim.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(pop)
/* ---------------------------------------- */

#define FRAME_WIDTH  160
#define FRAME_HEIGHT 275

static const struct {
    const char *path_pattern;
    int frame_count;
} sd_anim_groups[] = {
    {"A:/video/group0/out%03d.bin", 10},
    {"A:/video/group1/out%03d.bin", 15},
};
static const int sd_group_count = (int)(sizeof(sd_anim_groups) / sizeof(sd_anim_groups[0]));

static void sd_video_anim_cb(lv_timer_t *timer) {
    if (!timer) return;
    sd_video_anim_t *anim = (sd_video_anim_t *)lv_timer_get_user_data(timer);
    if (!anim) return;

    char bin_path[128];
    snprintf(bin_path, sizeof(bin_path), anim->path_pattern, anim->frame_idx + 1);

    lv_fs_file_t file;
    lv_fs_res_t res = lv_fs_open(&file, bin_path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        printf("[play_video] 打开帧失败: %s (res=%d)", bin_path, res);
        anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
        return;
    }

    uint32_t to_read = (uint32_t)(FRAME_WIDTH * FRAME_HEIGHT * sizeof(lv_color_t));
    uint32_t br = 0;
    res = lv_fs_read(&file, anim->frame_buf, to_read, &br);
    lv_fs_close(&file);
    if (res != LV_FS_RES_OK || br != to_read) {
        anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
        return;
    }

    if (anim->canvas_obj) lv_obj_invalidate(anim->canvas_obj);
    anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
}

sd_video_anim_t* create_sd_video_anim(const char *path_pattern, int frame_count, uint32_t interval_ms) {
    if (!path_pattern || frame_count <= 0) {
        printf("[play_video] 参数不合法");
        return NULL;
    }

    /* 检查颜色类型大小 */
    if (sizeof(lv_color_t) != 2) {
        printf("[play_video] lv_color_t 大小异常: %u 字节\n", (unsigned)sizeof(lv_color_t));

        return NULL;
    }

    sd_video_anim_t *anim = (sd_video_anim_t *)malloc(sizeof(sd_video_anim_t));
    if (!anim) {
        printf("[play_video] 动画结构体分配失败");
        return NULL;
    }
    memset(anim, 0, sizeof(sd_video_anim_t));

    strncpy(anim->path_pattern, path_pattern, sizeof(anim->path_pattern) - 1);
    anim->frame_count = frame_count;

    size_t buf_size = (size_t)FRAME_WIDTH * (size_t)FRAME_HEIGHT * sizeof(lv_color_t);
    anim->frame_buf = (lv_color_t *)malloc(buf_size);
    if (!anim->frame_buf) {
        printf("[play_video] 帧缓冲分配失败 (%zu bytes)", buf_size);
        free(anim);
        return NULL;
    }

    lv_result_t r = lv_draw_buf_init(
        &anim->draw_buf,
        FRAME_WIDTH,
        FRAME_HEIGHT,
        LV_COLOR_FORMAT_RGB565,
        FRAME_WIDTH * (uint32_t)sizeof(lv_color_t),
        anim->frame_buf,
        0
    );
    if (r != LV_RES_OK) {
        free(anim->frame_buf);
        free(anim);
        return NULL;
    }

    anim->canvas_obj = lv_canvas_create(lv_scr_act());
    if (!anim->canvas_obj) {
        free(anim->frame_buf);
        free(anim);
        return NULL;
    }

    lv_canvas_set_draw_buf(anim->canvas_obj, &anim->draw_buf);
    lv_obj_center(anim->canvas_obj);

    char first_path[128];
    snprintf(first_path, sizeof(first_path), anim->path_pattern, 1);
    lv_fs_file_t f;
    if (lv_fs_open(&f, first_path, LV_FS_MODE_RD) == LV_FS_RES_OK) {
        uint32_t br = 0;
        uint32_t want = (uint32_t)(FRAME_WIDTH * FRAME_HEIGHT * sizeof(lv_color_t));
        if (lv_fs_read(&f, anim->frame_buf, want, &br) == LV_FS_RES_OK && br == want) {
            lv_obj_invalidate(anim->canvas_obj);
        }
        lv_fs_close(&f);
    }

    anim->timer = lv_timer_create(sd_video_anim_cb, interval_ms, anim);
    if (!anim->timer) {
        lv_obj_del(anim->canvas_obj);
        free(anim->frame_buf);
        free(anim);
        return NULL;
    }

    return anim;
}

sd_video_anim_t* create_sd_video_anim_by_group(int group_index, uint32_t interval_ms) {
    if (group_index < 0 || group_index >= sd_group_count) {
        printf("[play_video] 动画组索引越界: %d", group_index);
        return NULL;
    }
    return create_sd_video_anim(sd_anim_groups[group_index].path_pattern,
                                sd_anim_groups[group_index].frame_count,
                                interval_ms);
}

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
