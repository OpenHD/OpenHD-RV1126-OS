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

#include "lvgl.h"

#include "media/recorder.h"
#include "main_view.h"
#include "media_lib.h"
#include "settings.h"
#include "storage.h"
#include "media/photo.h"
#include <rkadk/rkadk_photo.h>
#include <rkadk/rkadk_param.h>
#include <rkadk/rkadk_storage.h>
#include "rkmedia_venc.h"

#include "lvgl/lv_res.h"
#include "osd/osd.h"
#include "hal/key.h"
#include "lvgl/lv_msg.h"
#include "media/preview.h"
#include "media/vi_isp.h"
#include "media/player.h"

typedef enum {
    RECORD_STOPPED = 0,
    RECORD_STARTED,
} RECORD_STATUS;

typedef enum {
    MSG_PREVIEW_OPEN = 0,
    MSG_PREVIEW_CLOSE,
    MSG_PHOTO_AUDIO,
} CVR_ENUM_MSG;

extern RKADK_MW_PTR pStorage_Handle;

static lv_img_dsc_t *index_bg = NULL;
static lv_img_dsc_t *index_icon_photo_p = NULL;
static lv_img_dsc_t *index_icon_photo_r = NULL;
static lv_img_dsc_t *index_icon_sw_p = NULL;
static lv_img_dsc_t *index_icon_sw_r = NULL;
static lv_img_dsc_t *index_icon_set_p = NULL;
static lv_img_dsc_t *index_icon_set_r = NULL;
static lv_img_dsc_t *index_icon_media_p = NULL;
static lv_img_dsc_t *index_icon_media_r = NULL;
static lv_img_dsc_t *index_icon_sd_nor = NULL;
static lv_img_dsc_t *index_icon_sd = NULL;
static lv_img_dsc_t *index_icon_sd_err = NULL;
static lv_img_dsc_t *index_icon_sd_scan = NULL;
static lv_img_dsc_t *index_icon_record_nor = NULL;
static lv_img_dsc_t *index_icon_record = NULL;
static lv_img_dsc_t *index_icon_movie_p = NULL;
static lv_img_dsc_t *index_icon_movie_f = NULL;
static lv_img_dsc_t *index_icon_movie_p_nor = NULL;
static lv_img_dsc_t *index_icon_movie_f_nor = NULL;
static lv_img_dsc_t *index_icon_mic_nor = NULL;
static lv_img_dsc_t *index_icon_mic = NULL;

static lv_obj_t *main_view_obj = NULL;
static lv_obj_t *main_view_bg_obj = NULL;
static lv_obj_t *sd_obj = NULL;
static lv_obj_t *rec_obj = NULL;
static lv_obj_t *mic_obj = NULL;
static lv_obj_t *time_obj = NULL;
static lv_obj_t *p_obj = NULL;
static lv_obj_t *f_obj = NULL;
static lv_timer_t *timer = NULL;
static lv_color_t bg_color;
static lv_color_t text_color;
static lv_timer_t *osd_timer = NULL;
static int venc_init = 0;
static int rec_status = RECORD_STOPPED;

static void main_view_release_res(void)
{
    lv_res_destroy(&index_bg);
    lv_res_destroy(&index_icon_photo_p);
    lv_res_destroy(&index_icon_photo_r);
    lv_res_destroy(&index_icon_sw_p);
    lv_res_destroy(&index_icon_sw_r);
    lv_res_destroy(&index_icon_set_p);
    lv_res_destroy(&index_icon_set_r);
    lv_res_destroy(&index_icon_media_p);
    lv_res_destroy(&index_icon_media_r);
    lv_res_destroy(&index_icon_sd_nor);
    lv_res_destroy(&index_icon_sd);
    lv_res_destroy(&index_icon_sd_err);
    lv_res_destroy(&index_icon_sd_scan);
    lv_res_destroy(&index_icon_record_nor);
    lv_res_destroy(&index_icon_record);
    lv_res_destroy(&index_icon_movie_p_nor);
    lv_res_destroy(&index_icon_movie_f_nor);
    lv_res_destroy(&index_icon_mic_nor);
    lv_res_destroy(&index_icon_mic);
}

static void main_view_load_res(void)
{
    main_view_release_res();

    index_bg = lv_res_create("/oem/cvr_res/ui_1280x720/index_bg.png");
    index_icon_photo_p = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_photo_p.png");
    index_icon_photo_r = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_photo_r.png");
    index_icon_sw_p = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_sw_p.png");
    index_icon_sw_r = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_sw_r.png");
    index_icon_set_p = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_set_p.png");
    index_icon_set_r = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_set_r.png");
    index_icon_media_p = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_media_p.png");
    index_icon_media_r = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_media_r.png");
    index_icon_sd_nor = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_sd_nor.png");
    index_icon_sd = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_sd.png");
    index_icon_sd_err = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_sd_err.png");
    index_icon_sd_scan = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_sd_scan.png");
    index_icon_record_nor = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_record_nor.png");
    index_icon_record = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_record.png");
    index_icon_movie_p = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_movie_p.png");
    index_icon_movie_f = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_movie_f.png");
    index_icon_movie_p_nor = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_movie_p_nor.png");
    index_icon_movie_f_nor = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_movie_f_nor.png");
    index_icon_mic_nor = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_mic_nor.png");
    index_icon_mic = lv_res_create("/oem/cvr_res/ui_1280x720/index_icon_mic.png");
}

static void sw_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_sw_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_sw_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_sw_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_sw_p, NULL);
    }
#endif
}

static void photo_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        OSD_INFO font_info;
        font_info.osd_font_size = FONT_SIZE;
        font_info.osd_posx = OSD_POSX;
        font_info.osd_posy = OSD_POSY;
        osd_add_watermark(RKADK_STREAM_TYPE_SNAP, font_info);
        int status = storage_get_status();
        if (status == DISK_MOUNTED) {
            lv_post_msg((void *)main_view_start, MSG_PHOTO_AUDIO, NULL, NULL);
            if (photo_take_photo())
                printf("photo_take_photo fail\n");
        }
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_photo_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_photo_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_photo_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_photo_p, NULL);
    }
#endif
}

static void media_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        int status = storage_get_status();
        switch(status) {
            case DISK_UNMOUNTED:
                storage_nosdcard_dialog_create();
                break;
            case DISK_FORMAT_ERR:
            case DISK_NOT_FORMATTED:
                storage_noformat_dialog_create();
                break;
            case DISK_SCANNING:
                storage_scansdcard_dialog_create();
                break;
            case DISK_MOUNTED:
                main_view_stop();
                media_lib_start(&main_view_start);
                break;
        }
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_media_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_media_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_media_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_media_p, NULL);
    }
#endif
}

static void set_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        main_view_stop();
        settings_start(&main_view_start);
    }
#if USE_KEY
    else if (code == LV_EVENT_FOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_set_p, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_set_p, NULL);
    } else if (code == LV_EVENT_DEFOCUSED) {
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_set_r, NULL);
       lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_set_p, NULL);
    }
#endif
}

static void osd_time_handler(lv_timer_t *osd_timer)
{
    if (rec_status == RECORD_STOPPED)
        return;
    if (!venc_init) {
        int ret = osd_venc_init();
        if (ret)
            return;
        venc_init = 1;
    }
    OSD_INFO font_info;
    font_info.osd_font_size = FONT_SIZE;
    font_info.osd_posx = OSD_POSX;
    font_info.osd_posy = OSD_POSY;
    osd_add_watermark(RKADK_STREAM_TYPE_VIDEO_SUB, font_info);
}

static void recorder_start(void)
{
    if (rec_status == RECORD_STOPPED) {
        if (osd_timer == NULL) {
           osd_timer = lv_timer_create(osd_time_handler, 1000, NULL);
        }
        cvr_recorder_start(0);
    }
    rec_status = RECORD_STARTED;
}

static void recorder_stop(void)
{
    if (rec_status == RECORD_STARTED) {
        if (osd_timer)
            lv_timer_del(osd_timer);
        osd_timer = NULL;
        venc_init = 0;
        cvr_recorder_stop();
    }
    rec_status = RECORD_STOPPED;
}

static void update_sd_status(void)
{
    int sd_status = storage_get_status();
    switch(sd_status) {
        case DISK_UNMOUNTED:
            lv_img_set_src(sd_obj, index_icon_sd_nor);
            recorder_stop();
            storage_nosdcard_dialog_create();
            break;
        case DISK_FORMAT_ERR:
        case DISK_NOT_FORMATTED:
            lv_img_set_src(sd_obj, index_icon_sd_err);
            recorder_stop();
            storage_noformat_dialog_create();
            break;
        case DISK_SCANNING:
            lv_img_set_src(sd_obj, index_icon_sd_scan);
            recorder_stop();
            storage_noformat_dialog_destroy();
            storage_nosdcard_dialog_destroy();
            break;
        case DISK_MOUNTED:
            lv_img_set_src(sd_obj, index_icon_sd);
            recorder_start();
            storage_scansdcard_dialog_destroy();
            storage_noformat_dialog_destroy();
            storage_nosdcard_dialog_destroy();
            break;
    }
}

static void sd_status_cb(int status)
{
    printf("%s\n", __func__);
    if (main_view_bg_obj)
        update_sd_status();
}

static void update_status(void)
{
    static int mic_status = 0;
    static int p_status = 0;
    static int f_status = 0;

    char str_time[32];
    time_t timep;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep);
    sprintf(str_time, "%4d-%02d-%02d %02d:%02d:%02d", 1900 + p->tm_year,
           1 + p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
    lv_label_set_text(time_obj, str_time);

    if (rec_status)
        lv_img_set_src(rec_obj, index_icon_record);
    else
        lv_img_set_src(rec_obj, index_icon_record_nor);

    mic_status = !mic_status;
    if (mic_status)
        lv_img_set_src(mic_obj, index_icon_mic);
    else
        lv_img_set_src(mic_obj, index_icon_mic_nor);

    p_status = !p_status;
    if (p_status)
        lv_img_set_src(p_obj, index_icon_movie_p);
    else
        lv_img_set_src(p_obj, index_icon_movie_p_nor);

    f_status = !f_status;
    if (f_status)
        lv_img_set_src(f_obj, index_icon_movie_f);
    else
        lv_img_set_src(f_obj, index_icon_movie_f_nor);
}

static void time_handler(lv_timer_t *timer)
{
    update_status();
}

int main_msg_handler(int msg, void *wparam, void *lparam) {
    int ret;
    switch (msg) {
    case MSG_PREVIEW_OPEN:
        cvr_vi_init();
        cvr_preview_start(0);
        ret = photo_init(0);
        if (ret)
            printf("photo init fail!\n");
        int sd_status = storage_get_status();
        if (sd_status == DISK_MOUNTED) {
            cvr_wait_vi_converge();
            recorder_start();
        }
        storage_status_cb_reg(&sd_status_cb);
        break;
    case MSG_PREVIEW_CLOSE:
        ret = photo_deinit();
        if (ret)
            printf("photo_deinit fail\n");
        recorder_stop();
        cvr_preview_stop(0);
        cvr_vi_deinit();
        break;
    case MSG_PHOTO_AUDIO:
        if (cvr_player_play_audio("/oem/cvr_res/sound/takephoto.wav"))
            printf("play audio fail\n");
        break;
    default:
        return -1;
    }
    return 0;
}


void main_view_start(void)
{
    lv_obj_t *obj;
#if USE_KEY
    lv_group_t *group = lv_port_indev_group_create();
#endif

    lv_msg_register_handler(main_msg_handler, (void *)main_view_start);
    lv_post_msg((void *)main_view_start, MSG_PREVIEW_OPEN, NULL, NULL);
    main_view_load_res();

    bg_color.full = 0x00000000;
    text_color.full = 0xffffffff;

    main_view_bg_obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_view_bg_obj, LV_HOR_RES , LV_VER_RES);
    lv_obj_align(main_view_bg_obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(main_view_bg_obj, bg_color, 0);
    lv_obj_set_style_border_color(main_view_bg_obj, bg_color, 0);
    lv_obj_set_style_radius(main_view_bg_obj, 0, 0);

    main_view_obj = lv_img_create(lv_scr_act());
    lv_img_set_src(main_view_obj, index_bg);

    f_obj = obj = lv_img_create(main_view_obj);
    lv_img_set_src(obj, index_icon_movie_f_nor);
    lv_obj_set_pos(obj, LV_HOR_RES - 300, 32);

    p_obj = obj = lv_img_create(main_view_obj);
    lv_img_set_src(obj, index_icon_movie_p_nor);
    lv_obj_set_pos(obj, LV_HOR_RES - 240, 32);

    mic_obj = lv_img_create(main_view_obj);
    lv_img_set_src(mic_obj, index_icon_mic);
    lv_obj_set_pos(mic_obj, LV_HOR_RES - 180, 32);

    sd_obj = lv_img_create(main_view_obj);
    lv_img_set_src(sd_obj, index_icon_sd);
    lv_obj_set_pos(sd_obj, LV_HOR_RES - 120, 32);

    rec_obj = lv_img_create(main_view_obj);
    lv_img_set_src(rec_obj, index_icon_record);
    lv_obj_set_pos(rec_obj, LV_HOR_RES - 66, 40);

    obj = lv_imgbtn_create(main_view_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_sw_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_sw_p, NULL);
    lv_obj_add_event_cb(obj, sw_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_obj_add_event_cb(obj, sw_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, sw_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj);
#endif
    lv_obj_set_pos(obj, LV_HOR_RES - 800, LV_VER_RES - 148);
    lv_obj_set_size(obj, 96 , 96);

    obj = lv_imgbtn_create(main_view_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_photo_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_photo_p, NULL);
    lv_obj_add_event_cb(obj, photo_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_obj_add_event_cb(obj, photo_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, photo_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj);
#endif
    lv_obj_set_pos(obj, LV_HOR_RES - 676, LV_VER_RES - 164);
    lv_obj_set_size(obj, 128 , 128);

    obj = lv_imgbtn_create(main_view_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_media_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_media_p, NULL);
    lv_obj_add_event_cb(obj, media_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_obj_add_event_cb(obj, media_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, media_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj);
#endif
    lv_obj_set_pos(obj, LV_HOR_RES - 520, LV_VER_RES - 148);
    lv_obj_set_size(obj, 96 , 96);

    obj = lv_imgbtn_create(main_view_obj);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, index_icon_set_r, NULL);
    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, index_icon_set_p, NULL);
    lv_obj_add_event_cb(obj, set_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_obj_add_event_cb(obj, set_event_handler, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(obj, set_event_handler, LV_EVENT_DEFOCUSED, NULL);
    lv_group_add_obj(group, obj);
#endif
    lv_obj_set_pos(obj, LV_HOR_RES - 380, LV_VER_RES - 148);
    lv_obj_set_size(obj, 96 , 96);

    time_obj = lv_label_create(main_view_obj);
    lv_obj_set_pos(time_obj, LV_HOR_RES - 260, LV_VER_RES - 116);
    //lv_label_set_recolor(time_obj, true);
    lv_obj_set_style_text_font(time_obj, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(time_obj, text_color, 0);

    update_status();
    timer = lv_timer_create(time_handler, 200, NULL);
}

void main_view_stop(void)
{
    if (osd_timer)
        lv_timer_del(osd_timer);
    osd_timer = NULL;
    storage_status_cb_unreg(&sd_status_cb);
    if (timer)
        lv_timer_del(timer);
    timer = NULL;
    if (main_view_obj) {
        lv_obj_del(main_view_obj);
        main_view_obj = NULL;
    }
    if (main_view_bg_obj) {
        lv_obj_del(main_view_bg_obj);
        main_view_bg_obj = NULL;
    }
    main_view_release_res();
#if USE_KEY
    lv_port_indev_group_destroy(lv_group_get_default());
#endif
    lv_send_msg((void *)main_view_start, MSG_PREVIEW_CLOSE, NULL, NULL);
}
