/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <rkadk/rkadk_storage.h>

#include "rkadk_common.h"
#include "rkadk_thumb.h"
#include "rkadk_photo.h"

#include "media_lib.h"
#include "player.h"
#include "settings.h"
#include "storage.h"

#include "lvgl/lv_res.h"
#include "lvgl.h"
#include "hal/key.h"

typedef struct _MEDIA_THUMB {
    lv_img_dsc_t *thumb_img;
    lv_obj_t *thumb_obj;
    lv_obj_t *string_obj;
    pthread_mutex_t lock;
    int flag;
} MEDIA_THUMB;

#define THUMB_W    260
#define THUMB_H    180

#define LINE_NUM   40
#define ROW_NUM   4
#define FILE_NUM_LEN 16

extern lv_ft_info_t ttf_info_24;
extern lv_ft_info_t ttf_info_28;
extern lv_ft_info_t ttf_info_32;
extern RKADK_MW_PTR pStorage_Handle;

static lv_obj_t *media_lib_main_obj = NULL;
static lv_obj_t *media_lib_title_obj = NULL;
static lv_obj_t *media_lib_img_bg_obj = NULL;
static lv_obj_t *media_lib_filenum_obj[4] = {NULL};

static lv_img_dsc_t *icon_return_r = NULL;
static lv_img_dsc_t *icon_return_p = NULL;

static lv_img_dsc_t *icon_bg_r = NULL;
static lv_img_dsc_t *icon_bg_p = NULL;
static lv_img_dsc_t *icon_video_f_01 = NULL;
static lv_img_dsc_t *icon_video_p_01 = NULL;
static lv_img_dsc_t *icon_video_u_01 = NULL;
static lv_img_dsc_t *icon_photo_01 = NULL;
static lv_img_dsc_t *list_icon_play = NULL;

static lv_timer_t *media_lib_timer = NULL;
static lv_timer_t *media_lib_preview_timer = NULL;
static RKADK_FILE_LIST_ARRAY flieListArray;
static PLAYER_INFO_ARRAY replayArray;
static pthread_t thumb_tid = 0;
static int thumb_position = 0;
static int isvideo = 1;
static int type = 0;
static int thumb_thread_run = 0;
static int line_pos = 0;
static int line_update = 0;

static void (*pReturn_cb)(void) = NULL;

void media_lib_main_create(void);
void media_lib_main_destroy(void);

static void file_list_array_init(void) {
    int i;
    RKADK_STR_DEV_ATTR devAttr = RKADK_STORAGE_GetDevAttr(pStorage_Handle);

    memset(&flieListArray, 0, sizeof(RKADK_FILE_LIST_ARRAY));
    flieListArray.s32ListNum = devAttr.s32FolderNum;
    flieListArray.list = (RKADK_FILE_LIST *)malloc(sizeof(RKADK_FILE_LIST) * flieListArray.s32ListNum);
    memset(flieListArray.list, 0, sizeof(RKADK_FILE_LIST) * flieListArray.s32ListNum);

    for (i = 0; i < flieListArray.s32ListNum; i++) {
        sprintf(flieListArray.list[i].path, "%s%s", devAttr.cMountPath, devAttr.pstFolderAttr[i].cFolderPath);
        flieListArray.list[i].s32FileNum = RKADK_STORAGE_GetFileNum(flieListArray.list[i].path, pStorage_Handle);
    }
}

static void file_list_array_deinit(void) {
    if (flieListArray.list) {
        free(flieListArray.list);
        flieListArray.list = NULL;
    }
}

static void file_num_update(void) {
    int i;

    for (i = 0; i < flieListArray.s32ListNum; i++) {
        flieListArray.list[i].s32FileNum = RKADK_STORAGE_GetFileNum(flieListArray.list[i].path, pStorage_Handle);
        if (media_lib_filenum_obj[i]) {
            char fileNum[FILE_NUM_LEN];
            snprintf(fileNum, FILE_NUM_LEN, "%d", flieListArray.list[i].s32FileNum);
            lv_label_set_text(media_lib_filenum_obj[i], fileNum);
        }
    }
}

static void exit_preview(void) {
    int i;

    if (media_lib_img_bg_obj == NULL)
        return;

    if (media_lib_preview_timer)
        lv_timer_del(media_lib_preview_timer);
    media_lib_preview_timer = NULL;
    thumb_thread_run = 0;
    if (thumb_tid)
        pthread_join(thumb_tid, NULL);
    if (media_lib_img_bg_obj) {
        lv_obj_del(media_lib_img_bg_obj);
        media_lib_img_bg_obj = NULL;
    }
    if (media_lib_title_obj) {
        lv_obj_del(media_lib_title_obj);
        media_lib_title_obj = NULL;
    }
    for (i = 0; i < flieListArray.list[type].s32FileNum; i++) {
        MEDIA_THUMB *media_thumb = (MEDIA_THUMB *)flieListArray.list[type].file[i].thumb;
        if (media_thumb) {
            lv_img_dsc_t *thumb = media_thumb->thumb_img;
            if (thumb) {
                lv_img_cache_invalidate_src(thumb);
                char *data = (char *)thumb->data;
                if (data)
                    free(data);
                free(thumb);
            }
            free(media_thumb);
        }
        flieListArray.list[type].file[i].thumb = NULL;
    }
    if (replayArray.replay_list != NULL) {
        free(replayArray.replay_list);
        replayArray.replay_list = NULL;
    }
    RKADK_STORAGE_FreeFileList(&flieListArray.list[type]);
    media_lib_main_create();
}

static void media_lib_release_res(void)
{
    lv_res_destroy(&icon_return_r);
    lv_res_destroy(&icon_return_p);
    lv_res_destroy(&icon_bg_r);
    lv_res_destroy(&icon_bg_p);
    lv_res_destroy(&icon_video_f_01);
    lv_res_destroy(&icon_video_p_01);
    lv_res_destroy(&icon_video_u_01);
    lv_res_destroy(&icon_photo_01);
    lv_res_destroy(&list_icon_play);
}

static void media_lib_load_res(void)
{
    media_lib_release_res();

    icon_return_r = lv_res_create("/oem/cvr_res/ui_1280x720/icon_return_01.png");
    icon_return_p = lv_res_create("/oem/cvr_res/ui_1280x720/icon_return_02.png");
    icon_bg_r = lv_res_create("/oem/cvr_res/ui_1280x720/media_icon_bg_01.png");
    icon_bg_p = lv_res_create("/oem/cvr_res/ui_1280x720/media_icon_bg_02.png");
    icon_video_f_01 = lv_res_create("/oem/cvr_res/ui_1280x720/icon_video_f_01.png");
    icon_video_p_01 = lv_res_create("/oem/cvr_res/ui_1280x720/icon_video_p_01.png");
    icon_video_u_01 = lv_res_create("/oem/cvr_res/ui_1280x720/icon_video_u_01.png");
    icon_photo_01 = lv_res_create("/oem/cvr_res/ui_1280x720/icon_photo_01.png");
    list_icon_play = lv_res_create("/oem/cvr_res/ui_1280x720/list_icon_play.png");
}

static void return_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        media_lib_stop();
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    }
#endif
}

static void back_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        exit_preview();
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    }
#endif
}

static void video_player_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    PLAYER_FILE_INFO *val = (PLAYER_FILE_INFO *)lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        player_start(val, type);
    }
}

static void photo_player_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    PLAYER_FILE_INFO *val = (PLAYER_FILE_INFO *)lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        player_start(val, type);
    }
}

static void main_event_handler(lv_event_t * e)
{
    lv_obj_t * cont = lv_event_get_target(e);
    int line = 0;
    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);

    if (cont->spec_attr->scroll.y < 0)
        line = (cont->spec_attr->scroll.y * (-1) - 210 + 260) / 260;
    if (line != line_pos) {
        line_update = 1;
        line_pos = line;
    }
}

static void media_lib_thumb_create(int pos)
{
    lv_color_t text_color;
    text_color.full = 0xffffffff;
    lv_obj_t *obj;
    lv_img_dsc_t *thumb;
    MEDIA_THUMB *media_thumb = flieListArray.list[type].file[pos].thumb;
    if (media_thumb == NULL) {
        media_thumb = malloc(sizeof(MEDIA_THUMB));
        memset(media_thumb, 0, sizeof(MEDIA_THUMB));
        pthread_mutex_init(&media_thumb->lock, NULL);
    } else {
        if (media_thumb->thumb_obj)
            return;
    }
    pthread_mutex_lock(&media_thumb->lock);
    thumb = media_thumb->thumb_img;
    if (thumb == NULL) {
        thumb = malloc(sizeof(lv_img_dsc_t));
        memset(thumb, 0, sizeof(lv_img_dsc_t));
    }
    thumb->header.always_zero = 0;
    thumb->header.w = THUMB_W;
    thumb->header.h = THUMB_H;
    thumb->data_size = THUMB_W * THUMB_H * 4;
    thumb->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
    if (thumb->data == NULL)
        thumb->data = malloc(thumb->data_size);
    memset((void *)thumb->data, 0x00, thumb->data_size);

    char *file = flieListArray.list[type].file[pos].filename;
    int img_x = 40 + (pos % ROW_NUM) * 300;
    int img_y = 10 + (pos / ROW_NUM) * 260;
    int text_x = 40 + (pos % ROW_NUM) * 300;
    int text_y = 200 + (pos / ROW_NUM) * 260;

    if (isvideo) {
        media_thumb->thumb_obj = obj = lv_img_create(media_lib_img_bg_obj);
        lv_img_set_src(obj, thumb);
        //lv_obj_align(obj, LV_ALIGN_CENTER, 0, -20);
        lv_obj_set_pos(obj, img_x, img_y);
        lv_obj_set_size(obj, 260, 180);
        obj = lv_imgbtn_create(obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, list_icon_play, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, list_icon_play, NULL);
        lv_obj_add_event_cb(obj, video_player_event_handler, LV_EVENT_CLICKED, &replayArray.replay_list[pos]);
        lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(obj, 72, 72);
    } else {
        media_thumb->thumb_obj = obj = lv_imgbtn_create(media_lib_img_bg_obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, thumb, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, thumb, NULL);
        lv_obj_add_event_cb(obj, photo_player_event_handler, LV_EVENT_CLICKED, &replayArray.replay_list[pos]);
        lv_obj_set_pos(obj, img_x, img_y);
        lv_obj_set_size(obj, 260, 180);
    }
    media_thumb->string_obj = obj = lv_label_create(media_lib_img_bg_obj);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    //text_color.full = 0xff313131;
    lv_obj_set_style_text_color(obj, text_color, 0);
    //lv_obj_align(obj, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_pos(obj, text_x, text_y);
    lv_label_set_text(obj, file);
    media_thumb->flag = 0;
    pthread_mutex_unlock(&media_thumb->lock);
    media_thumb->thumb_img = thumb;
    flieListArray.list[type].file[pos].thumb = (void *)media_thumb;
}

static void *thumb_thread(void *arg)
{
    char filepath[128];
    int i;

    RKADK_JPG_THUMB_TYPE_E eJpgThumbType = RKADK_JPG_THUMB_TYPE_MFP1;
    RKADK_THUMB_ATTR_S stThumbAttr;
    memset(&stThumbAttr, 0, sizeof(RKADK_THUMB_ATTR_S));
    stThumbAttr.u32Width = THUMB_W;
    stThumbAttr.u32Height = THUMB_H;
    stThumbAttr.enType = RKADK_THUMB_TYPE_RGB888;
    while (thumb_thread_run) {
        for (i = 0; i < flieListArray.list[type].s32FileNum; i++) {
            int ret = -1;
            if (line_update) {
                int j = line_pos * ROW_NUM;
                line_update = 0;
                if (j < flieListArray.list[type].s32FileNum)
                    i = j;
            }

            MEDIA_THUMB *media_thumb = (MEDIA_THUMB *)flieListArray.list[type].file[i].thumb;
            if (thumb_thread_run && media_thumb) {
                lv_img_dsc_t *thumb = media_thumb->thumb_img;
                if (thumb == NULL)
                    continue;
                if (media_thumb->thumb_obj == NULL) {
                    pthread_mutex_lock(&media_thumb->lock);
                    lv_img_cache_invalidate_src(thumb);
                    char *data = (char *)thumb->data;
                    if (data)
                        free(data);
                    free(thumb);
                    media_thumb->thumb_img = NULL;
                    pthread_mutex_unlock(&media_thumb->lock);
                }
                if (media_thumb->flag == 0) {
                    sprintf(filepath, "%s%s", flieListArray.list[type].path, flieListArray.list[type].file[i].filename);
                    if (strstr(flieListArray.list[type].file[i].filename, "jpg") || strstr(flieListArray.list[type].file[i].filename, "jpeg")) {
                        ret = RKADK_PHOTO_GetThmInJpgEx(filepath, eJpgThumbType, &stThumbAttr);
                        if (ret)
                            RKADK_LOGE("RKADK_PHOTO_GetThmInJpg failed");
                    } else if (strstr(flieListArray.list[type].file[i].filename, "mp4")) {
                        ret = RKADK_GetThmInMp4Ex(filepath, &stThumbAttr);
                        if (ret)
                            printf("RKADK_GetThmInMp4Ex failed\n");
                    }

                    if (ret == 0) {
                        int k, j;

                        for (k = 0; k < thumb->header.h; k++) {
                            char* img_data = (char*)&thumb->data[thumb->header.w * k * 4];
                            char* row_data = (char*)&(stThumbAttr.pu8Buf[thumb->header.w * k * 3]);
                            //memcpy(img_data, row_data, thumb->header.w * 3);
                            for (j = 0; j < thumb->header.w * 3; j += 3) {
                                *img_data++ = row_data[j + 2];
                                *img_data++ = row_data[j + 1];
                                *img_data++ = row_data[j + 0];
                               *img_data++ = 0xff;
                            }
                        }
                    }
                    RKADK_ThmBufFree(&stThumbAttr);
                    media_thumb->flag = 1;
                }
            }

        }
        usleep(10000);
    }
    thumb_tid = 0;
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

static void main_time_handler(lv_timer_t *media_lib_timer)
{
    int status = storage_get_status();
    if (status != DISK_MOUNTED) {
        player_stop();
        exit_preview();
        media_lib_stop();
    }
}

static void preview_time_handler(lv_timer_t *media_lib_timer)
{
    lv_color_t text_color;
    int i;
    static int start_p = 0;
    int free_count = 0;
    int create_count = 0;
    int start = 0;
    int end = LINE_NUM;
    text_color.full = 0xffffffff;

    if (line_pos > (LINE_NUM) / 2) {
        start = line_pos - (LINE_NUM) / 2;
        end = line_pos + (LINE_NUM) / 2;
    }
    if (start_p != start)
        start_p = start;

    for (i = 0; i < flieListArray.list[type].s32FileNum; i++) {
        int cur = i / ROW_NUM;
        MEDIA_THUMB *media_thumb = flieListArray.list[type].file[i].thumb;
        if (free_count < 8) {
            if ((cur < start) || (cur > end)) {
                if (media_thumb && media_thumb->thumb_obj) {
                    lv_obj_del(media_thumb->thumb_obj);
                    lv_obj_del(media_thumb->string_obj);
                    media_thumb->thumb_obj = NULL;
                    media_thumb->string_obj = NULL;
                    free_count++;
                }
            }
        }
        if (create_count < 8) {
            if ((cur >= start) && (cur <= end)) {
                if (!(media_thumb && media_thumb->thumb_obj)) {
                    media_lib_thumb_create(i);
                    create_count++;
                }
            }
        }

        if (media_thumb && media_thumb->thumb_obj && media_thumb->flag == 1) {
            media_thumb->flag = 2;
            lv_img_dsc_t *thumb = media_thumb->thumb_img;
            if (thumb) {
                if (isvideo) {
                    lv_img_set_src(media_thumb->thumb_obj, thumb);
                } else {
                    lv_imgbtn_set_src(media_thumb->thumb_obj, LV_IMGBTN_STATE_RELEASED, NULL, thumb, NULL);
                    lv_imgbtn_set_src(media_thumb->thumb_obj, LV_IMGBTN_STATE_PRESSED, NULL, thumb, NULL);
                }
            }
        }
    }
}

static void media_lib_preview(void)
{
    lv_obj_t *obj;
    lv_color_t bg_color;
    lv_color_t text_color;
    int i;
#if USE_KEY
    lv_group_t *group = lv_port_indev_group_create();
#endif

    isvideo = 1;
    line_pos = 0;
    bg_color.full = 0xFF04171D;
    text_color.full = 0xffffffff;

    media_lib_title_obj = obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES, 200);
    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(obj, bg_color, 0);
    lv_obj_set_style_border_color(obj, bg_color, 0);
    lv_obj_set_style_radius(obj, 0, 0);

    obj = lv_imgbtn_create(media_lib_title_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    lv_obj_add_event_cb(obj, back_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_obj_add_event_cb(obj, back_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, back_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj);
#endif
    lv_obj_set_pos(obj, 40, 20);
    lv_obj_set_size(obj, 96, 96);

    obj = lv_label_create(media_lib_title_obj);
    lv_obj_set_style_text_font(obj, ttf_info_32.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 38);

    if (type == 0) {
        lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_VIDEO_F]);
    } else if (type == 1) {
        lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_VIDEO_P]);
    } else if (type == 2) {
        lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_PHOTO]);
        isvideo = 0;
    } else if (type == 3) {
        lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_VIDEO_U]);
    }

    media_lib_img_bg_obj = obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES , LV_VER_RES - 130);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_bg_color(obj, bg_color, 0);
    lv_obj_set_style_border_color(obj, bg_color, 0);
    lv_obj_set_style_radius(obj, 0, 0);

    lv_obj_add_event_cb(media_lib_img_bg_obj, main_event_handler, LV_EVENT_SCROLL, NULL);

    memset(&replayArray, 0, sizeof(PLAYER_INFO_ARRAY));
    replayArray.totallnum = flieListArray.list[type].s32FileNum;
    replayArray.replay_list = (PLAYER_FILE_INFO *)malloc(sizeof(PLAYER_FILE_INFO) * replayArray.totallnum);
    memset(replayArray.replay_list, 0, sizeof(PLAYER_FILE_INFO) * replayArray.totallnum);

    if (!RKADK_STORAGE_GetFileList(&flieListArray.list[type], pStorage_Handle, LIST_ASCENDING)) {
        for (i = 0; (i < flieListArray.list[type].s32FileNum)/* && (i < 64)*/ && ((i / ROW_NUM) < LINE_NUM); i++) {
            replayArray.replay_list[i].curposition = i;
            replayArray.replay_list[i].list = flieListArray.list;
            media_lib_thumb_create(i);
        }
        thumb_position = i;
        int x = 52;
        int y = 200 + ((flieListArray.list[type].s32FileNum - 1) / ROW_NUM) * 260;
        obj = lv_obj_create(media_lib_img_bg_obj);
        lv_obj_set_size(obj, 1 , 1);
        lv_obj_set_pos(obj, x, y);
        lv_obj_set_style_bg_color(obj, bg_color, 0);
        lv_obj_set_style_border_color(obj, bg_color, 0);
        lv_obj_set_style_radius(obj, 0, 0);
    }
    thumb_thread_run = 1;
    pthread_create(&thumb_tid, NULL, thumb_thread, NULL);
    media_lib_preview_timer = lv_timer_create(preview_time_handler, 20, NULL);
}

static void media_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    type = (int)lv_event_get_user_data(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked %d\n", __func__, type);
        media_lib_main_destroy();
        media_lib_preview();
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_bg_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_bg_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_bg_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_bg_p, NULL);
    }
#endif
}

void media_lib_main_create(void)
{
    lv_obj_t *obj;
    lv_obj_t *obj_b;
    lv_color_t bg_color;
    lv_color_t text_color;
    char fileNum[FILE_NUM_LEN];
#if USE_KEY
    lv_group_t *group = lv_port_indev_group_create();
#endif

    bg_color.full = 0xFF04171D;
    text_color.full = 0xffffffff;

    media_lib_main_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(media_lib_main_obj, LV_HOR_RES , LV_VER_RES);
    lv_obj_align(media_lib_main_obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(media_lib_main_obj, bg_color, 0);
    lv_obj_set_style_border_color(media_lib_main_obj, bg_color, 0);
    lv_obj_set_style_radius(media_lib_main_obj, 0, 0);

    snprintf(fileNum, FILE_NUM_LEN, "%d", flieListArray.list[0].s32FileNum);
    obj_b = lv_imgbtn_create(media_lib_main_obj);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_RELEASED, NULL, icon_bg_r, NULL);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_PRESSED, NULL, icon_bg_p, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_CLICKED, (void *)0);
#if USE_KEY
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj_b);
#endif
    lv_obj_set_pos(obj_b, 30, 150);
    lv_obj_set_size(obj_b, 276 , 366);

    obj = lv_img_create(obj_b);
    lv_img_set_src(obj, icon_video_f_01);
    lv_obj_set_pos(obj, 76, 90);

    obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 250);
    lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_VIDEO_F]);

    media_lib_filenum_obj[0] = obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 290);
    lv_label_set_text(obj, fileNum);

    snprintf(fileNum, FILE_NUM_LEN, "%d", flieListArray.list[1].s32FileNum);
    obj_b = lv_imgbtn_create(media_lib_main_obj);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_RELEASED, NULL, icon_bg_r, NULL);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_PRESSED, NULL, icon_bg_p, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_CLICKED, (void *)1);
#if USE_KEY
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj_b);
#endif
    lv_obj_set_pos(obj_b, 330, 150);
    lv_obj_set_size(obj_b, 276 , 366);
    obj = lv_img_create(obj_b);
    lv_img_set_src(obj, icon_video_p_01);
    lv_obj_set_pos(obj, 76, 90);
    obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 250);
    lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_VIDEO_P]);
    media_lib_filenum_obj[1] = obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 290);
    lv_label_set_text(obj, fileNum);

    snprintf(fileNum, FILE_NUM_LEN, "%d", flieListArray.list[2].s32FileNum);
    obj_b = lv_imgbtn_create(media_lib_main_obj);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_RELEASED, NULL, icon_bg_r, NULL);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_PRESSED, NULL, icon_bg_p, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_CLICKED, (void *)2);
#if USE_KEY
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj_b);
#endif
    lv_obj_set_pos(obj_b, 630, 150);
    lv_obj_set_size(obj_b, 276 , 366);
    obj = lv_img_create(obj_b);
    lv_img_set_src(obj, icon_photo_01);
    lv_obj_set_pos(obj, 76, 90);
    obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 250);
    lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_PHOTO]);
    media_lib_filenum_obj[2] = obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 290);
    lv_label_set_text(obj, fileNum);

    snprintf(fileNum, FILE_NUM_LEN, "%d", flieListArray.list[3].s32FileNum);
    obj_b = lv_imgbtn_create(media_lib_main_obj);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_RELEASED, NULL, icon_bg_r, NULL);
    lv_imgbtn_set_src(obj_b, LV_IMGBTN_STATE_PRESSED, NULL, icon_bg_p, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_CLICKED, (void *)3);
#if USE_KEY
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj_b, media_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj_b);
#endif
    lv_obj_set_pos(obj_b, 930, 150);
    lv_obj_set_size(obj_b, 276 , 366);
    obj = lv_img_create(obj_b);
    lv_img_set_src(obj, icon_video_u_01);
    lv_obj_set_pos(obj, 76, 90);
    obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 250);
    lv_label_set_text(obj, res_str[RES_STR_MEDIA_LIB_VIDEO_U]);
    media_lib_filenum_obj[3] = obj = lv_label_create(obj_b);
    lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 290);
    lv_label_set_text(obj, fileNum);

    obj = lv_imgbtn_create(media_lib_main_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    lv_obj_add_event_cb(obj, return_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_obj_add_event_cb(obj, return_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, return_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj);
#endif
    lv_obj_set_pos(obj, 40, 20);
    lv_obj_set_size(obj, 96, 96);
}

void media_lib_main_destroy(void)
{
    if (media_lib_main_obj) {
        int i;
#if USE_KEY
        lv_port_indev_group_destroy(lv_group_get_default());
#endif
        lv_obj_del(media_lib_main_obj);
        media_lib_main_obj = NULL;
        for (i = 0; i < 4; i++)
            media_lib_filenum_obj[i] = NULL;
    }
}

void media_lib_start(void (*return_cb)(void))
{
    pReturn_cb = return_cb;
    file_list_array_init();
    media_lib_load_res();
    media_lib_main_create();
    media_lib_timer = lv_timer_create(main_time_handler, 200, NULL);
}

void media_lib_stop(void)
{
    if (media_lib_timer)
        lv_timer_del(media_lib_timer);
    media_lib_timer = NULL;
    media_lib_main_destroy();
    media_lib_release_res();
    file_list_array_deinit();
    if (pReturn_cb) {
        pReturn_cb();
    }
}
