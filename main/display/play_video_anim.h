#ifndef PLAY_VIDEO_ANIM_H
#define PLAY_VIDEO_ANIM_H

#include "lvgl.h"

/* 动画控制结构体 */
typedef struct {
    char path_pattern[128];
    int frame_count;
    int frame_idx;
    lv_timer_t *timer;
    lv_obj_t *canvas_obj;   /* 兼容保留，当前未使用 */
    lv_draw_buf_t draw_buf; /* 兼容保留，当前未使用 */
    lv_color_t *frame_buf;
    /* 使用 lv_image 显示帧数据 */
    lv_obj_t *image_obj;
    lv_image_dsc_t img_dsc;
} sd_video_anim_t;

#ifdef __cplusplus
extern "C" {
#endif

sd_video_anim_t* create_sd_video_anim(const char *path_pattern, int frame_count, uint32_t interval_ms);
sd_video_anim_t* create_sd_video_anim_by_group(int group_index, uint32_t interval_ms);
void stop_sd_video_anim(sd_video_anim_t *anim);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PLAY_VIDEO_ANIM_H */
