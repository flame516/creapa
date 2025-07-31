/**
 * @file lv_conf.h
 * 用户自定义LVGL配置文件，适用于SD卡图片播放
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   基本配置
 *====================*/
#define LV_COLOR_DEPTH 16
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN
#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/*====================
   文件系统支持
 *====================*/
#define LV_USE_FS_FATFS 1
#if LV_USE_FS_FATFS
    #define LV_FS_FATFS_LETTER '\0'   /* 默认盘符，留空表示路径直接用/sdcard/... */
    #define LV_FS_FATFS_CACHE_SIZE 0
#endif

/*====================
   常用控件
 *====================*/
#define LV_USE_IMAGE      1
#define LV_USE_LABEL      1
#define LV_USE_ANIMIMG    1
#define LV_USE_CANVAS     1
#define LV_USE_LIST       1
#define LV_USE_TABVIEW    1
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_FS_STDIO 1

#endif /*LV_CONF_H*/ 