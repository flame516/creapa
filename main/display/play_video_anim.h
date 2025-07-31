#ifndef PLAY_VIDEO_ANIM_H
#define PLAY_VIDEO_ANIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// 动画数据结构（保留原有）
typedef struct {
    const lv_img_dsc_t **frames;
    int frame_count;
    int current_frame;
    lv_timer_t *timer;
    lv_obj_t *img_obj;
} video_anim_t;

// SD卡播放器控制结构体（新增）
typedef struct {
    lv_obj_t *img_obj;         // LVGL图片对象
    lv_timer_t *timer;         // LVGL定时器
    char path_pattern[128];    // 帧图片路径格式，如 "/sdcard/video/frame%03d.bmp"
    int frame_idx;             // 当前帧索引
    int frame_count;           // 总帧数
} sd_video_anim_t;

// 原有函数声明（保留）
void stop_video_anim(video_anim_t *anim);

// 新增SD卡播放函数声明
sd_video_anim_t* create_sd_video_anim(const char *path_pattern, int frame_count, uint32_t interval_ms);
void stop_sd_video_anim(sd_video_anim_t *anim);

// 组合接口：通过数组索引播放SD卡动画组
sd_video_anim_t* create_sd_video_anim_by_group(int group_index, uint32_t interval_ms);

#ifdef __cplusplus
}
#endif

#endif // PLAY_VIDEO_ANIM_H 