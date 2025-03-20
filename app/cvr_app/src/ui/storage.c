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
#include <sys/mount.h>
#include <rkadk/rkadk_storage.h>

#include "settings.h"
#include "storage.h"

#include "lvgl/lv_res.h"
#include "lvgl.h"
#include "hal/key.h"

extern lv_ft_info_t ttf_info_24;
extern lv_ft_info_t ttf_info_28;
extern lv_ft_info_t ttf_info_32;

struct cb_obj {
  void (*cb)(int);
  struct cb_obj *next;
};

static lv_obj_t *dialog_nosdcard_obj = NULL;
static lv_obj_t *dialog_noformat_obj = NULL;
static lv_obj_t *dialog_scansdcard_obj = NULL;

static RKADK_STR_DEV_ATTR stDevAttr;
RKADK_MW_PTR pStorage_Handle = NULL;
static lv_timer_t *storage_time = NULL;
static RKADK_MOUNT_STATUS sd_status = DISK_UNMOUNTED;
static struct cb_obj *sd_status_cb_list = NULL;
static pthread_mutex_t sd_status_lock;
static char format_id[8] = {'R', 'K', 'C', 'V', 'R', '1', '.', '0'};
static char volume[11] = {'R', 'K', 'C', 'V', 'R', 0};

static void storage_time_handler(lv_timer_t *timer)
{
    if (pStorage_Handle) {
        RKADK_MOUNT_STATUS mountstatus = RKADK_STORAGE_GetMountStatus(pStorage_Handle);
        //printf("%s %d, %d\n", __func__, sd_status, mountstatus);
        if (sd_status != mountstatus) {
            sd_status = mountstatus;
            pthread_mutex_lock(&sd_status_lock);
            struct cb_obj *temp = sd_status_cb_list;
            while (temp) {
                if (temp->cb)
                    temp->cb(sd_status);
                temp = temp->next;
            }
            pthread_mutex_unlock(&sd_status_lock);
        }
    }
}

static void close_event_handler(lv_event_t * e)
{
    static int mic = 0;
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        printf("%s Clicked\n", __func__);
        storage_nosdcard_dialog_destroy();
        storage_noformat_dialog_destroy();
        storage_scansdcard_dialog_destroy();
    }
}

void storage_status_cb_reg(void (*cb)(int))
{
    pthread_mutex_lock(&sd_status_lock);
    struct cb_obj *temp = sd_status_cb_list;
    while (temp) {
        if (temp->cb == cb)
            goto out;
        temp = temp->next;
    }
    struct cb_obj *obj = malloc(sizeof(struct cb_obj));
    memset(obj, 0, sizeof(struct cb_obj));
    obj->cb = cb;
    if (sd_status_cb_list) {
        temp = sd_status_cb_list;
        while (temp->next) {
            temp = temp->next;
        }
        temp->next = obj;
    } else {
        sd_status_cb_list = obj;
    }
out:
    pthread_mutex_unlock(&sd_status_lock);
}

void storage_status_cb_unreg(void (*cb)(int))
{
    pthread_mutex_lock(&sd_status_lock);
    if (sd_status_cb_list) {
        if (sd_status_cb_list->cb == cb) {
            sd_status_cb_list = sd_status_cb_list->next;
            goto out;
        }
        struct cb_obj *temp = sd_status_cb_list;
        while (temp->next) {
            if (temp->next->cb == cb) {
                struct cb_obj *temp_free = temp->next;
                temp->next = temp_free->next;
                free(temp_free);
                goto out;
            }
            temp = temp->next;
        }
    }
out:
    pthread_mutex_unlock(&sd_status_lock);
}

int storage_get_status(void)
{
    if (pStorage_Handle)
        return RKADK_STORAGE_GetMountStatus(pStorage_Handle);
    else
        return sd_status;
}

void storage_nosdcard_dialog_destroy(void)
{
    if (dialog_nosdcard_obj) {
#if USE_KEY
        lv_port_indev_group_destroy(lv_group_get_default());
#endif
        lv_obj_del(dialog_nosdcard_obj);
        dialog_nosdcard_obj = NULL;
    }
}

void storage_nosdcard_dialog_create(void)
{
    lv_obj_t *obj;
    lv_obj_t *obj_bg;
    lv_color_t color;
    lv_color_t text_color;
    text_color.full = 0xffffffff;
    color.full = 0xFF000000;

    storage_noformat_dialog_destroy();
    storage_scansdcard_dialog_destroy();

    if (dialog_nosdcard_obj)
        return;

    dialog_nosdcard_obj = obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES , LV_VER_RES);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    lv_obj_add_event_cb(obj, close_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_group_t *group = lv_port_indev_group_create();
    lv_group_add_obj(group, obj);
#endif
    color.full = 0xFF222D30;
    obj_bg = obj = lv_obj_create(obj);
    lv_obj_set_size(obj, 600 , 400);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
    lv_obj_add_event_cb(obj, close_event_handler, LV_EVENT_CLICKED, NULL);

    obj = lv_label_create(obj_bg);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(obj, res_str[RES_STR_SETTINGS_BOX_STORE_TITLE]);

    text_color.full = 0xff4C5457;
    obj = lv_label_create(dialog_nosdcard_obj);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 210);
    lv_label_set_text(obj, "-------------------------------------------------");

    text_color.full = 0xffffffff;
    obj = lv_label_create(obj_bg);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 200);
    lv_label_set_text(obj, res_str[RES_STR_SETTINGS_BOX_NOSDCARD]);
}

void storage_noformat_dialog_destroy(void)
{
    if (dialog_noformat_obj) {
#if USE_KEY
        lv_port_indev_group_destroy(lv_group_get_default());
#endif
        lv_obj_del(dialog_noformat_obj);
        dialog_noformat_obj = NULL;
    }
}

void storage_noformat_dialog_create(void)
{
    lv_obj_t *obj;
    lv_obj_t *obj_bg;
    lv_color_t color;
    lv_color_t text_color;
    text_color.full = 0xffffffff;
    color.full = 0xFF000000;

    storage_nosdcard_dialog_destroy();
    storage_scansdcard_dialog_destroy();

    if (dialog_noformat_obj)
        return;

    dialog_noformat_obj = obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES , LV_VER_RES);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    lv_obj_add_event_cb(obj, close_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_group_t *group = lv_port_indev_group_create();
    lv_group_add_obj(group, obj);
#endif
    color.full = 0xFF222D30;
    obj_bg = obj = lv_obj_create(obj);
    lv_obj_set_size(obj, 600 , 400);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
    lv_obj_add_event_cb(obj, close_event_handler, LV_EVENT_CLICKED, NULL);

    obj = lv_label_create(obj_bg);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(obj, res_str[RES_STR_SETTINGS_BOX_STORE_TITLE]);

    text_color.full = 0xff4C5457;
    obj = lv_label_create(dialog_noformat_obj);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 210);
    lv_label_set_text(obj, "-------------------------------------------------");

    text_color.full = 0xffffffff;
    obj = lv_label_create(obj_bg);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 200);
    lv_label_set_text(obj, res_str[RES_STR_SETTINGS_BOX_NOFORMAT]);
}

void storage_scansdcard_dialog_destroy(void)
{
    if (dialog_scansdcard_obj) {
#if USE_KEY
        lv_port_indev_group_destroy(lv_group_get_default());
#endif
        lv_obj_del(dialog_scansdcard_obj);
        dialog_scansdcard_obj = NULL;
    }
}

void storage_scansdcard_dialog_create(void)
{
    lv_obj_t *obj;
    lv_obj_t *obj_bg;
    lv_color_t color;
    lv_color_t text_color;
    text_color.full = 0xffffffff;
    color.full = 0xFF000000;

    storage_nosdcard_dialog_destroy();
    storage_noformat_dialog_destroy();

    if (dialog_scansdcard_obj)
        return;

    dialog_scansdcard_obj = obj = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj, LV_HOR_RES , LV_VER_RES);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    lv_obj_add_event_cb(obj, close_event_handler, LV_EVENT_CLICKED, NULL);
#if USE_KEY
    lv_group_t *group = lv_port_indev_group_create();
    lv_group_add_obj(group, obj);
#endif
    color.full = 0xFF222D30;
    obj_bg = obj = lv_obj_create(obj);
    lv_obj_set_size(obj, 600 , 400);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_border_color(obj, color, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_80, 0);
    lv_obj_add_event_cb(obj, close_event_handler, LV_EVENT_CLICKED, NULL);

    obj = lv_label_create(obj_bg);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 10);
    lv_label_set_text(obj, res_str[RES_STR_SETTINGS_BOX_STORE_TITLE]);

    text_color.full = 0xff4C5457;
    obj = lv_label_create(dialog_scansdcard_obj);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 210);
    lv_label_set_text(obj, "-------------------------------------------------");

    text_color.full = 0xffffffff;
    obj = lv_label_create(obj_bg);
    lv_obj_set_style_text_font(obj, ttf_info_28.font, 0);
    lv_obj_set_style_text_color(obj, text_color, 0);
    lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 200);
    lv_label_set_text(obj, res_str[RES_STR_SETTINGS_BOX_SCAN]);
}

int storage_format(void)
{
    if (storage_time)
        lv_timer_del(storage_time);
    storage_time = NULL;

    if (pStorage_Handle)
        RKADK_STORAGE_Format(pStorage_Handle, "vfat");

    storage_time = lv_timer_create(storage_time_handler, 100, NULL);
}

int storage_init(void)
{
    pthread_mutex_init(&sd_status_lock, NULL);
    memset(&stDevAttr, 0, sizeof(RKADK_STR_DEV_ATTR));
    sprintf(stDevAttr.cMountPath, "/mnt/sdcard");
    sprintf(stDevAttr.cDevPath, "/dev/mmcblk2p1");
    stDevAttr.s32AutoDel = 1;
    stDevAttr.s32FreeSizeDelMin = 1200;
    stDevAttr.s32FreeSizeDelMax = 1800;
    stDevAttr.s32FolderNum = 4;
    stDevAttr.pstFolderAttr = (RKADK_STR_FOLDER_ATTR *)malloc(
        sizeof(RKADK_STR_FOLDER_ATTR) * stDevAttr.s32FolderNum);
    memcpy(stDevAttr.cFormatId, format_id, 8);
    memcpy(stDevAttr.cVolume, volume, 11);
    stDevAttr.s32CheckFormatId = 1;

    if (!stDevAttr.pstFolderAttr) {
        printf("stDevAttr.pstFolderAttr malloc failed.\n");
        return -1;
    }
    memset(stDevAttr.pstFolderAttr, 0,
            sizeof(RKADK_STR_FOLDER_ATTR) * stDevAttr.s32FolderNum);

    stDevAttr.pstFolderAttr[0].bNumLimit = RKADK_FALSE;
    stDevAttr.pstFolderAttr[0].s32Limit = 50;
    sprintf(stDevAttr.pstFolderAttr[0].cFolderPath, "/video_front/");
    stDevAttr.pstFolderAttr[1].bNumLimit = RKADK_FALSE;
    stDevAttr.pstFolderAttr[1].s32Limit = 40;
    sprintf(stDevAttr.pstFolderAttr[1].cFolderPath, "/video_back/");
    stDevAttr.pstFolderAttr[2].bNumLimit = RKADK_TRUE;
    stDevAttr.pstFolderAttr[2].s32Limit = 20000;
    sprintf(stDevAttr.pstFolderAttr[2].cFolderPath, "/photo/");
    stDevAttr.pstFolderAttr[3].bNumLimit = RKADK_FALSE;
    stDevAttr.pstFolderAttr[3].s32Limit = 10;
    sprintf(stDevAttr.pstFolderAttr[3].cFolderPath, "/video_urgent/");

    if (RKADK_STORAGE_Init(&pStorage_Handle, &stDevAttr)) {
        printf("Storage init failed.\n");
        return -1;
    }

    storage_time = lv_timer_create(storage_time_handler, 100, NULL);
    return 0;
}

int storage_deinit(void)
{
    if (storage_time)
        lv_timer_del(storage_time);
    storage_time = NULL;
    RKADK_STORAGE_Deinit(pStorage_Handle);
    pStorage_Handle = NULL;
    if (stDevAttr.pstFolderAttr) {
        free(stDevAttr.pstFolderAttr);
        stDevAttr.pstFolderAttr = NULL;
    }

    return 0;
}