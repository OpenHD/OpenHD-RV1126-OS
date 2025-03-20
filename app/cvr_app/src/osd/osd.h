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

#ifndef _CVR_OSD_
#define _CVR_OSD_

#include "font_factory.h"
#include <wchar.h>
#include "rkmedia_api.h"
#include <rkadk/rkadk_param.h>

#define FONT_SIZE 80
#define OSD_POSX  1728
#define OSD_POSY  1360
#define MAX_FONT_LEN 128
#define ARGB32_WHITE 0xFFFFFFFF

#define UPALIGNTO(value, align) ((value + align - 1) & (~(align - 1)))
#define UPALIGNTO16(value) UPALIGNTO(value, 16)

typedef struct osd_info {
   unsigned int osd_posx;
   unsigned int osd_posy;
   unsigned int osd_font_size;
} OSD_INFO;

int osd_venc_init(void);
int osd_venc_deinit(void);
void osd_add_watermark(RKADK_STREAM_TYPE_E stream_type, OSD_INFO font_info);

#endif //_CVR_OSD_