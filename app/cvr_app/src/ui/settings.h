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

#ifndef SETTINGS_H_
#define SETTINGS_H_

enum RES_STR_ID
{
    RES_STR_MEDIA_LIB_VIDEO_F = 0,
    RES_STR_MEDIA_LIB_VIDEO_P,
    RES_STR_MEDIA_LIB_VIDEO_U,
    RES_STR_MEDIA_LIB_PHOTO,
    RES_STR_SETTINGS_SET,
    RES_STR_SETTINGS_MOVIE,
    RES_STR_SETTINGS_RES,
    RES_STR_SETTINGS_CODE,
    RES_STR_SETTINGS_VOL,
    RES_STR_SETTINGS_MIC,
    RES_STR_SETTINGS_LANG,
    RES_STR_SETTINGS_STORE,
    RES_STR_SETTINGS_FORMAT,
    RES_STR_SETTINGS_BOX_LANG_TITLE,
    RES_STR_SETTINGS_BOX_LANG_SN,
    RES_STR_SETTINGS_BOX_LANG_TN,
    RES_STR_SETTINGS_BOX_LANG_EN,
    RES_STR_SETTINGS_BOX_LANG_JA,
    RES_STR_SETTINGS_BOX_STORE_TITLE,
    RES_STR_SETTINGS_BOX_STORE_INFO,
    RES_STR_SETTINGS_BOX_FORMAT_TITLE,
    RES_STR_SETTINGS_BOX_FORMAT_INFO,
    RES_STR_SETTINGS_BOX_FORMAT_OK,
    RES_STR_SETTINGS_BOX_FORMAT_CANCEL,
    RES_STR_SETTINGS_BOX_STORE_TOTAL,
    RES_STR_SETTINGS_BOX_NOSDCARD,
    RES_STR_SETTINGS_BOX_SCAN,
    RES_STR_SETTINGS_BOX_NOFORMAT,
    RES_STR_MAX
};
extern char *res_str[RES_STR_MAX];

extern void settings_start(void (*return_cb)(void));
extern void settings_stop(void);
extern int loading_string_res(void);
extern void unloading_string_res(void);
extern void loading_font(void);

#endif // SETTINGS_H_