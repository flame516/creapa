#ifndef PLAY_VIDEO_ANIM_H
#define PLAY_VIDEO_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* 播放视频动画结构体 */
typedef struct {
    char path_pattern[128];    /* 文件路径模式，例如 "A:/video/group0/out%03d.bin" */
    int frame_count;           /* 总帧数 */
    int frame_idx;             /* 当前帧索引 */

    lv_obj_t *canvas_obj;      /* LVGL Canvas 对象 */
    lv_color_t *frame_buf;     /* 帧缓冲（RGB565） */

    lv_draw_buf_t draw_buf;    /* LVGL 9 draw buffer，和 frame_buf 绑定在一起 */
    lv_timer_t *timer;         /* 播放定时器 */
} sd_video_anim_t;

/**
 * @brief 创建 SD 卡视频动画
 * @param path_pattern 文件路径模式，例如 "A:/video/group0/out%03d.bin"
 * @param frame_count  帧总数
 * @param interval_ms  每帧间隔（毫秒）
 * @return sd_video_anim_t* 动画对象指针，失败返回 NULL
 */
sd_video_anim_t* create_sd_video_anim(const char *path_pattern,
                                      int frame_count,
                                      uint32_t interval_ms);

/**
 * @brief 通过预定义组创建动画
 * @param group_index 动画组索引
 * @param interval_ms 每帧间隔（毫秒）
 * @return sd_video_anim_t* 动画对象指针，失败返回 NULL
 */
sd_video_anim_t* create_sd_video_anim_by_group(int group_index,
                                               uint32_t interval_ms);

/**
 * @brief 停止并释放动画
 * @param anim 动画对象指针
 */
void stop_sd_video_anim(sd_video_anim_t *anim);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PLAY_VIDEO_ANIM_H */
