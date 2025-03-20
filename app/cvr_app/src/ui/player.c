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

#include "player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <rkadk/rkadk_photo.h>

#include "lvgl/lv_res.h"
#include "lvgl.h"
#include "media/player.h"
#include "media/photo.h"

#define THUMB_W    1280
#define THUMB_H    720

extern lv_ft_info_t ttf_info_32;
extern lv_ft_info_t ttf_info_28;
extern lv_ft_info_t ttf_info_24;

static lv_timer_t *timer = NULL;

static lv_img_dsc_t *index_bg = NULL;
static lv_img_dsc_t *icon_return_r = NULL;
static lv_img_dsc_t *icon_return_p = NULL;
static lv_img_dsc_t *img_icon_next = NULL;
static lv_img_dsc_t *img_icon_next_p = NULL;
static lv_img_dsc_t *img_icon_pre = NULL;
static lv_img_dsc_t *img_icon_pre_p = NULL;
static lv_img_dsc_t *player_icon_play = NULL;
static lv_img_dsc_t *player_icon_play_p = NULL;
static lv_img_dsc_t *player_icon_stop = NULL;
static lv_img_dsc_t *player_icon_stop_p = NULL;
static lv_img_dsc_t *player_icon_pre = NULL;
static lv_img_dsc_t *player_icon_pre_p = NULL;
static lv_img_dsc_t *player_icon_next = NULL;
static lv_img_dsc_t *player_icon_next_p = NULL;
static lv_img_dsc_t *photo_playback = NULL;


static lv_obj_t *player_main_obj = NULL;
static lv_obj_t *player_main_bg_obj = NULL;
static lv_obj_t *cur_time_obj = NULL;
static lv_obj_t *total_time_obj = NULL;
static lv_obj_t *slider_obj = NULL;
static lv_obj_t *pause_obj = NULL;
static lv_obj_t *name_obj = NULL;
static lv_obj_t *return_obj = NULL;
static lv_obj_t *photo_obj = NULL;

static int cur_time = 0;
static int total_time = 60;
static int file_position = 0;
static int file_type = 0;
static int pause_flag = PLAYER_PLAY;
static pthread_mutex_t switch_video_mutex;
static RKADK_PHOTO_DATA_ATTR_S stDataAttr;
static PLAYER_FILE_INFO *replay_info = NULL;

static void player_release_res(void)
{
    lv_res_destroy(&index_bg);
    lv_res_destroy(&icon_return_r);
    lv_res_destroy(&icon_return_p);
    lv_res_destroy(&img_icon_next);
    lv_res_destroy(&img_icon_pre);
    lv_res_destroy(&player_icon_play);
    lv_res_destroy(&player_icon_play_p);
    lv_res_destroy(&player_icon_pre);
    lv_res_destroy(&player_icon_next);
    lv_res_destroy(&player_icon_pre_p);
    lv_res_destroy(&player_icon_next_p);
}

static void player_load_res(void)
{
    player_release_res();

    index_bg = lv_res_create("/oem/cvr_res/ui_1280x720/index_bg.png");
    icon_return_r = lv_res_create("/oem/cvr_res/ui_1280x720/icon_return_01.png");
    icon_return_p = lv_res_create("/oem/cvr_res/ui_1280x720/icon_return_02.png");
    img_icon_next = lv_res_create("/oem/cvr_res/ui_1280x720/img_icon_next.png");
    img_icon_next_p = lv_res_create("/oem/cvr_res/ui_1280x720/img_icon_next_p.png");
    img_icon_pre = lv_res_create("/oem/cvr_res/ui_1280x720/img_icon_pre.png");
    img_icon_pre_p = lv_res_create("/oem/cvr_res/ui_1280x720/img_icon_pre_p.png");
    player_icon_play = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_play.png");
    player_icon_play_p = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_play_p.png");
    player_icon_stop = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_stop.png");
    player_icon_stop_p = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_stop_p.png");
    player_icon_pre = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_pre.png");
    player_icon_pre_p = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_pre_p.png");
    player_icon_next = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_next.png");
    player_icon_next_p = lv_res_create("/oem/cvr_res/ui_1280x720/player_icon_next_p.png");

}

static void return_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        player_stop();
    }
}

static void pause_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        pause_flag = cvr_player_pause();
        if (pause_flag == PLAYER_PAUSE) {
            lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_play, NULL);
            lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_play_p, NULL);
        } else if (pause_flag == PLAYER_PLAY) {
            lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_stop, NULL);
            lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_stop_p, NULL);
        }
    }
}

static void replay_autoswitch(void) {
    int ret;
    char video_path[MAX_PATH_LEN];
    int replay_totaltime;

    if (file_position == replay_info->list[file_type].s32FileNum-1) {
        printf("Last video\n");
        file_position = 0;
    } else {
        file_position++;
    }
    memset(video_path, 0, sizeof(video_path));
    snprintf(video_path, sizeof(video_path), "/mnt/sdcard/video_front/%s", replay_info->list[file_type].file[file_position].filename);
    pthread_mutex_lock(&switch_video_mutex);
    ret = cvr_player_switch(video_path);
    lv_label_set_text(name_obj, replay_info->list[file_type].file[file_position].filename);
    cvr_player_get_duration(&replay_totaltime);
    if (!ret) {
        cur_time = 0;
        total_time = replay_totaltime / 1000;
        lv_slider_set_range(slider_obj, 0, total_time);
        pause_flag = PLAYER_PLAY;
        lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_stop, NULL);
        lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_stop_p, NULL);
    }
    pthread_mutex_unlock(&switch_video_mutex);
}

static void update_time(void)
{
    char str_time[32];

    lv_slider_set_value(slider_obj, cur_time, LV_ANIM_OFF);
    sprintf(str_time, "%02d:%02d", cur_time / 60, cur_time % 60);
    lv_label_set_text(cur_time_obj, str_time);
    sprintf(str_time, "%02d:%02d", total_time / 60, total_time % 60);
    lv_label_set_text(total_time_obj, str_time);
}

static void time_handler(lv_timer_t *timer)
{
    if (pause_flag == PLAYER_PLAY) {
        cur_time++;
        if (cur_time > total_time)
            replay_autoswitch();
        update_time();
    }
}

static void switch_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int switch_type = (int)lv_event_get_user_data(e);
    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        int ret;
        char video_path[MAX_PATH_LEN];
        int replay_totaltime;

        if (switch_type == 0) {
            if (file_position == 0) {
                printf("First video\n");
                return;
            } else {
                file_position--;
            }
        } else {
            if (file_position == replay_info->list[file_type].s32FileNum - 1) {
                printf("Last video\n");
                return;
            } else {
                file_position++;
            }
        }
        memset(video_path, 0, sizeof(video_path));
        snprintf(video_path, sizeof(video_path), "/mnt/sdcard/video_front/%s", replay_info->list[file_type].file[file_position].filename);
        pthread_mutex_lock(&switch_video_mutex);
        ret = cvr_player_switch(video_path);
        if (!ret) {
            printf("Switch_video_path :%s\n", video_path);
            lv_label_set_text(name_obj, replay_info->list[file_type].file[file_position].filename);
            pause_flag = PLAYER_PLAY;
            lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_stop, NULL);
            lv_imgbtn_set_src(pause_obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_stop_p, NULL);

            cvr_player_get_duration(&replay_totaltime);
            cur_time = 0;
            total_time = replay_totaltime / 1000;
            lv_slider_set_range(slider_obj, 0, total_time);
            update_time();
        } else {
            printf("Switch video failed\n");
        }
        pthread_mutex_unlock(&switch_video_mutex);
    }
}

static void photo_switch_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int switch_type = (int)lv_event_get_user_data(e);
    int k, j, ret;
    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        char photo_path[MAX_PATH_LEN];
        if (switch_type == 0) {
            if (file_position == 0) {
                printf("First photo\n");
                return;
            } else {
                file_position--;
            }
        } else {
            if (file_position == replay_info->list[file_type].s32FileNum-1) {
                printf("Last photo\n");
                return;
            } else {
                file_position++;
            }
        }

        memset((void *)photo_playback->data, 0x00, photo_playback->data_size);
        memset(&stDataAttr, 0, sizeof(RKADK_PHOTO_DATA_ATTR_S));
        memset(photo_path, 0, sizeof(photo_path));
        snprintf(photo_path, sizeof(photo_path), "/mnt/sdcard/photo/%s",
              replay_info->list[file_type].file[file_position].filename);
        ret = photo_display_start(photo_path, &stDataAttr, THUMB_W, THUMB_H);
        if (ret) {
            printf("Photo replay fail!\n");
        }
        for (k = 0; k < photo_playback->header.h; k++) {
            char* img_data = (char*)&photo_playback->data[photo_playback->header.w * k * 4];
            char* row_data = (char*)&(stDataAttr.pu8Buf[photo_playback->header.w * k * 4]);
            for (j = 0; j < photo_playback->header.w * 4; j += 4) {
                *img_data++ = row_data[j + 2];
                *img_data++ = row_data[j + 1];
                *img_data++ = row_data[j + 0];
                *img_data++ = row_data[j + 3];
            }
        }
        lv_img_set_src(photo_obj, photo_playback);
        lv_label_set_text(name_obj, replay_info->list[file_type].file[file_position].filename);
    }
}

void player_start(PLAYER_FILE_INFO *val, int type)
{
    printf("%s %s\n", __func__, val->list[type].file[val->curposition].filename);
    lv_obj_t *obj;
    lv_color_t bg_color;
    lv_color_t text_color;
    bg_color.full = 0x00000000;
    text_color.full = 0xffffffff;
    char video_path[MAX_PATH_LEN];
    replay_info = val;
    file_type = type;
    file_position = replay_info->curposition;

    pthread_mutex_init(&switch_video_mutex,NULL);
    player_load_res();

    player_main_obj = obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES , LV_VER_RES);
    lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(obj, bg_color, 0);
    lv_obj_set_style_border_color(obj, bg_color, 0);
    lv_obj_set_style_radius(obj, 0, 0);

    player_main_bg_obj = obj = lv_img_create(lv_scr_act());
    lv_img_set_src(obj, index_bg);

    photo_obj = obj = lv_img_create(player_main_bg_obj);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 1280, 720);

    return_obj = obj = lv_imgbtn_create(player_main_bg_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);
    lv_obj_add_event_cb(obj, return_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_pos(obj, 40, 20);
    lv_obj_set_size(obj, 96, 96);

    name_obj = obj = lv_label_create(player_main_bg_obj);
    lv_obj_set_style_text_font(obj, ttf_info_32.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 38);

    lv_label_set_text(obj, val->list[type].file[val->curposition].filename);
    if (type == 2) {
        int k, j, ret;
        char photo_path[MAX_PATH_LEN];

        memset(photo_path, 0, sizeof(photo_path));
        snprintf(photo_path, sizeof(photo_path), "/mnt/sdcard/photo/%s",
                val->list[type].file[val->curposition].filename);

        memset(&stDataAttr, 0, sizeof(RKADK_PHOTO_DATA_ATTR_S));
        ret = photo_display_start(photo_path, &stDataAttr, THUMB_W, THUMB_H);
        if (ret) {
            printf("Photo replay fail!\n");
        }

        if (photo_playback == NULL) {
            photo_playback = malloc(sizeof(lv_img_dsc_t));
                if(photo_playback == NULL) {
                    printf("Malloc photo_playback fail!\n");
                }
            memset(photo_playback, 0, sizeof(lv_img_dsc_t));
        }
        photo_playback->header.always_zero = 0;
        photo_playback->header.w = THUMB_W;
        photo_playback->header.h = THUMB_H;
        photo_playback->data_size = THUMB_W * THUMB_H * 4;
        photo_playback->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        if (photo_playback->data == NULL)
           photo_playback->data = malloc(photo_playback->data_size);
             if(photo_playback->data == NULL) {
                printf("Malloc photo_playback data fail!\n");
            }
        memset((void *)photo_playback->data, 0x00, photo_playback->data_size);

        for (k = 0; k < photo_playback->header.h; k++) {
            char* img_data = (char*)&photo_playback->data[photo_playback->header.w * k * 4];
            char* row_data = (char*)&(stDataAttr.pu8Buf[photo_playback->header.w * k * 4]);
            for (j = 0; j < photo_playback->header.w * 4; j += 4) {
                *img_data++ = row_data[j + 2];
                *img_data++ = row_data[j + 1];
                *img_data++ = row_data[j + 0];
                *img_data++ = row_data[j + 3];
            }
        }

        lv_img_set_src(photo_obj, photo_playback);

        lv_label_set_text(name_obj, val->list[type].file[val->curposition].filename);
        lv_imgbtn_set_src(return_obj, LV_IMGBTN_STATE_RELEASED, NULL, icon_return_r, NULL);
        lv_imgbtn_set_src(return_obj, LV_IMGBTN_STATE_PRESSED, NULL, icon_return_p, NULL);

        obj = lv_imgbtn_create(player_main_bg_obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, img_icon_pre, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, img_icon_pre_p, NULL);
        lv_obj_add_event_cb(obj, photo_switch_handler, LV_EVENT_CLICKED, (void *)0);
        lv_obj_align(obj, LV_ALIGN_LEFT_MID, 50, 0);
        lv_obj_set_size(obj, 72, 72);
        obj = lv_imgbtn_create(player_main_bg_obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, img_icon_next, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, img_icon_next_p, NULL);
        lv_obj_add_event_cb(obj, photo_switch_handler, LV_EVENT_CLICKED, (void *)1);
        lv_obj_align(obj, LV_ALIGN_RIGHT_MID, -50, 0);
        lv_obj_set_size(obj, 72, 72);
    } else {
        pause_obj = obj = lv_imgbtn_create(player_main_bg_obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_stop, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_stop_p, NULL);
        lv_obj_add_event_cb(obj, pause_event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(obj, 128, 128);
        obj = lv_imgbtn_create(player_main_bg_obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_pre, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_pre_p, NULL);
        lv_obj_add_event_cb(obj, switch_event_handler, LV_EVENT_CLICKED, (void *)0);
        lv_obj_align(obj, LV_ALIGN_CENTER, -200, 0);
        lv_obj_set_size(obj, 72, 72);
        obj = lv_imgbtn_create(player_main_bg_obj);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, player_icon_next, NULL);
        lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, player_icon_next_p, NULL);
        lv_obj_add_event_cb(obj, switch_event_handler, LV_EVENT_CLICKED, (void *)1);
        lv_obj_align(obj, LV_ALIGN_CENTER, 200, 0);
        lv_obj_set_size(obj, 72, 72);

        cur_time_obj = obj = lv_label_create(player_main_bg_obj);
        lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
        lv_obj_set_style_text_color(obj, text_color, 0);
        lv_obj_align(obj, LV_ALIGN_BOTTOM_LEFT, 16, -30);
        //lv_label_set_text(obj, "00:00");

        total_time_obj = obj = lv_label_create(player_main_bg_obj);
        lv_obj_set_style_text_font(obj, ttf_info_24.font, 0);
        lv_obj_set_style_text_color(obj, text_color, 0);
        lv_obj_align(obj, LV_ALIGN_BOTTOM_RIGHT, -16, -30);
        //lv_label_set_text(obj, "01:00");

        /* Create a transition */
        static const lv_style_prop_t props[] = {LV_STYLE_BG_COLOR, 0};
        static lv_style_transition_dsc_t transition_dsc;
        lv_style_transition_dsc_init(&transition_dsc, props, lv_anim_path_linear, 300, 0, NULL);

        static lv_style_t style_main;
        static lv_style_t style_indicator;
        static lv_style_t style_knob;
        static lv_style_t style_pressed_color;
        lv_style_init(&style_main);
        lv_style_set_bg_opa(&style_main, LV_OPA_50);
        lv_style_set_bg_color(&style_main, lv_color_hex(0x4c5457));
        lv_style_set_radius(&style_main, LV_RADIUS_CIRCLE);
        lv_style_set_pad_ver(&style_main, -2); /*Makes the indicator larger*/

        lv_style_init(&style_indicator);
        lv_style_set_bg_opa(&style_indicator, LV_OPA_COVER);
        lv_style_set_bg_color(&style_indicator, lv_color_hex(0x00eaeb));
        lv_style_set_radius(&style_indicator, LV_RADIUS_CIRCLE);
        lv_style_set_transition(&style_indicator, &transition_dsc);

        lv_style_init(&style_knob);
        lv_style_set_bg_opa(&style_knob, LV_OPA_COVER);
        lv_style_set_bg_color(&style_knob, lv_color_hex(0xffffff));
        lv_style_set_border_color(&style_knob, lv_color_hex(0xffffff));
        lv_style_set_border_width(&style_knob, 10);
        lv_style_set_radius(&style_knob, LV_RADIUS_CIRCLE);
        lv_style_set_pad_all(&style_knob, 6); /*Makes the knob larger*/
        lv_style_set_transition(&style_knob, &transition_dsc);

        lv_style_init(&style_pressed_color);
        lv_style_set_bg_color(&style_pressed_color, lv_color_hex(0x00eaeb));

        /* Create a slider and add the style */
        slider_obj = lv_slider_create(player_main_bg_obj);

        lv_obj_remove_style_all(slider_obj);        /*Remove the styles coming from the theme*/

        lv_obj_add_style(slider_obj, &style_main, LV_PART_MAIN);
        lv_obj_add_style(slider_obj, &style_indicator, LV_PART_INDICATOR);
        lv_obj_add_style(slider_obj, &style_pressed_color, LV_PART_INDICATOR | LV_STATE_PRESSED);
        lv_obj_add_style(slider_obj, &style_knob, LV_PART_KNOB);
        lv_obj_add_style(slider_obj, &style_pressed_color, LV_PART_KNOB | LV_STATE_PRESSED);
        lv_obj_set_size(slider_obj, LV_HOR_RES - 200, 10);
        lv_obj_align(slider_obj, LV_ALIGN_BOTTOM_MID, 0, -40);
        int replay_totaltime = 0;
        switch (type) {
        case 0:
            memset(video_path, 0, sizeof(video_path));
            snprintf(video_path, sizeof(video_path), "/mnt/sdcard/video_front/%s",
                val->list[type].file[val->curposition].filename);
            cvr_player_start(video_path);
            cvr_player_get_duration(&replay_totaltime);
            printf("Replay video duration：%d\n", replay_totaltime);
            cur_time=0;
            total_time = replay_totaltime / 1000;
            lv_slider_set_range(slider_obj, 0, total_time);
            break;
        case 1:
            memset(video_path, 0, sizeof(video_path));
            snprintf(video_path, sizeof(video_path), "/mnt/sdcard/video_back/%s",
                val->list[type].file[val->curposition].filename);
            break;
        case 3:
            memset(video_path, 0, sizeof(video_path));
            snprintf(video_path, sizeof(video_path), "/mnt/sdcard/video_urgent/%s",
                val->list[type].file[val->curposition].filename);
        default:
            printf("Replay video fail!");
            break;  
        } 
        printf("Replay video_path :%s", video_path);
        update_time();
        timer = lv_timer_create(time_handler, 1000, NULL);
    }
}

static void photo_playback_stop(void) {
    if(photo_playback->data) {
        free((void*)photo_playback->data);
        photo_playback->data=NULL;
    }
    if(photo_playback) {
        free((void*)photo_playback);
        photo_playback = NULL;
    }
    photo_display_stop(&stDataAttr);
}

void player_stop(void)
{
    if (timer)
        lv_timer_del(timer);
    timer = NULL;
    if (player_main_bg_obj) {
        lv_obj_del(player_main_bg_obj);
        player_main_bg_obj = NULL;
    }
    if (player_main_obj) {
        lv_obj_del(player_main_obj);
        player_main_obj = NULL;
    }
    if (file_type == 2) {
       photo_playback_stop();
    } else {
        cvr_player_stop();
        pause_flag = PLAYER_PLAY;
    }
    player_release_res();
}
