#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // 用于文件存在性检查
#include "sdcard_check.h" // 包含SD卡相关函数声明
// 只保留SD卡动画数据结构
typedef struct {
    char path_pattern[128];
    int frame_count;
    int frame_idx;
    lv_timer_t *timer;
    lv_obj_t *img_obj;
} sd_video_anim_t;

// SD卡动画组配置
static const struct {
    const char *path_pattern;
    int frame_count;
} sd_anim_groups[] = {
    {"/sdcard/video/group0/out%03d.png", 10},  // 组0：10帧
    {"/sdcard/video/group1/out%03d.png", 15},  // 组1：15帧
    //{"/sdcard/video/group2/output_%03d.png", 12},  // 组2：12帧
    // 可以继续添加更多组
};

static const int sd_group_count = sizeof(sd_anim_groups) / sizeof(sd_anim_groups[0]);

// 检查文件是否存在
static bool file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

// SD卡动画回调
static void sd_video_anim_cb(lv_timer_t *timer) {
    sd_video_anim_t *anim = (sd_video_anim_t *)lv_timer_get_user_data(timer);
    if (!anim) return;
    char img_path[128];
    snprintf(img_path, sizeof(img_path), anim->path_pattern, anim->frame_idx + 1);
    if (!file_exists(img_path)) {
        printf("动画帧图片不存在: %s", img_path);
        return;
    }
    lv_img_set_src(anim->img_obj, img_path);
    anim->frame_idx = (anim->frame_idx + 1) % anim->frame_count;
}

// SD卡播放函数（改进版）
sd_video_anim_t* create_sd_video_anim(const char *path_pattern, int frame_count, uint32_t interval_ms) {
    sd_video_anim_t *anim = (sd_video_anim_t *)malloc(sizeof(sd_video_anim_t));
    if (!anim) {
        printf("分配动画结构体失败");
        return NULL;
    }
    printf("第60行");
    strncpy(anim->path_pattern, path_pattern, sizeof(anim->path_pattern) - 1);
    anim->path_pattern[sizeof(anim->path_pattern) - 1] = '\0';
    anim->frame_count = frame_count;
    anim->frame_idx = 0;
    anim->img_obj = lv_img_create(lv_scr_act());
    if (!anim->img_obj) {
        printf("创建LVGL图片对象失败");
        free(anim);
        return NULL;
    }
    char img_path[128];
    snprintf(img_path, sizeof(img_path), anim->path_pattern, 1);
    if (!file_exists(img_path)) {
        printf("动画首帧图片不存在: %s", img_path);
        lv_obj_del(anim->img_obj);
        free(anim);
        return NULL;
    }
    lv_img_set_src(anim->img_obj, img_path);
    lv_obj_center(anim->img_obj);
    anim->timer = lv_timer_create(sd_video_anim_cb, interval_ms, anim);
    if (!anim->timer) {
        printf("创建LVGL定时器失败");
        lv_obj_del(anim->img_obj);
        free(anim);
        return NULL;
    }
    printf("88");
    return anim;
}

void stop_sd_video_anim(sd_video_anim_t *anim) {
    if (!anim) return;
    if (anim->timer) lv_timer_del(anim->timer);
    if (anim->img_obj) lv_obj_del(anim->img_obj);
    free(anim);
}

// 通过数组索引播放SD卡动画组（改进版）
sd_video_anim_t* create_sd_video_anim_by_group(int group_index, uint32_t interval_ms) {
    if (group_index < 0 || group_index >= sd_group_count) {
        printf("动画组索引越界: %d", group_index);
        return NULL;
    }
    const char* pattern = sd_anim_groups[group_index].path_pattern;
    int frame_count = sd_anim_groups[group_index].frame_count;
    // 检查首帧图片是否存在
    char img_path[128];
    snprintf(img_path, sizeof(img_path), pattern, 1);
    if (!file_exists(img_path)) {
        printf("动画组%d首帧图片不存在: %s", group_index, img_path);
        return NULL;
    }
    return create_sd_video_anim(pattern, frame_count, interval_ms);
}

#ifdef __cplusplus
}
#endif
