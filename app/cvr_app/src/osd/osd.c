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

#include "osd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

int osd_venc_init(void) {
    int chnid, ret;
    unsigned int cameraId = 0;
    for (RKADK_STREAM_TYPE_E i = RKADK_STREAM_TYPE_VIDEO_SUB;
        i <= RKADK_STREAM_TYPE_SNAP; i = (RKADK_STREAM_TYPE_E)(i + 1)) {
        chnid = RKADK_PARAM_GetVencChnId(cameraId, i);
        if (ret != -1) {
            printf("vencChnId:%d\n", chnid);
        }
        ret = RK_MPI_VENC_RGN_Init(chnid, NULL);
        if (ret) {
            printf("Venc_rgn_init fail ret = %d\n", ret);
            return -1;
        }
    }
    return 0;
}

int osd_venc_deinit(void) {
    printf("osd_venc_deinit\n");
    return 0;
}

void osd_add_watermark(RKADK_STREAM_TYPE_E stream_type, OSD_INFO font_info) {

    int ret, chnid;
    time_t timep;
    struct tm *p;
    unsigned int cameraId = 0;
    unsigned char *buffer = NULL;
    BITMAP_S BitMap;
    OSD_REGION_INFO_S RngInfo;
    int buffer_width, buffer_height, buffer_size;

    wchar_t wmark_wch[MAX_FONT_LEN];
    char wmark_ch[MAX_FONT_LEN];

    time(&timep);
    p = gmtime(&timep);
    memset(wmark_wch, 0, sizeof(wmark_wch));
    memset(wmark_ch, 0, sizeof(wmark_ch));

    snprintf(wmark_ch, sizeof(wmark_ch), "%04d-%02d-%02d %02d:%02d:%02d",
                (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
                p->tm_min, p->tm_sec);

    ret = mbstowcs(wmark_wch, wmark_ch, strlen(wmark_ch));
    buffer_width = UPALIGNTO16((strlen(wmark_ch) / 2 + 2) * (font_info.osd_font_size));
    buffer_height = UPALIGNTO16(font_info.osd_font_size + 16);
    buffer_size = buffer_width * buffer_height * 4; // BGRA8888 4byte
    buffer = (unsigned char *)malloc(buffer_size);
    if (buffer == NULL) {
        printf("Add_watermark:create buffer fail!\n");
        return;
    }
    memset(buffer, 0, buffer_size);
    ret = create_font("/oem/cvr_res/font/msyh.ttf", font_info.osd_font_size);
    if (ret) {
        printf("Add_watermark:create_font fail\n");
        if (buffer)
            free(buffer);
            BitMap.pData = NULL;
            buffer = NULL;
        return;
    }

    set_font_color(ARGB32_WHITE);
    draw_argb8888_text(buffer, buffer_width, buffer_height, wmark_wch);

    memset(&BitMap, 0, sizeof(BITMAP_S));
    memset(&RngInfo, 0, sizeof(OSD_REGION_INFO_S));
    BitMap.enPixelFormat = PIXEL_FORMAT_ARGB_8888;
    BitMap.u32Width = buffer_width;
    BitMap.u32Height = buffer_height;
    BitMap.pData = buffer;

    RngInfo.enRegionId = 2;
    RngInfo.u32PosX = font_info.osd_posx;
    RngInfo.u32PosY = font_info.osd_posy;
    RngInfo.u32Width = buffer_width;
    RngInfo.u32Height = buffer_height;
    RngInfo.u8Enable = 1;
    RngInfo.u8Inverse = 0;

    chnid = RKADK_PARAM_GetVencChnId(cameraId, stream_type);
    if (chnid == -1) {
        printf("GetVencChnId fail\n");
        destroy_font();
        if (buffer) {
            free(buffer);
            BitMap.pData = NULL;
            buffer = NULL;
        }
        return;
    }

    ret = RK_MPI_VENC_RGN_SetBitMap(chnid, &RngInfo, &BitMap);
    if (ret)
        printf("ERROR: set rgn bitmap(enable) failed! ret=%d\n", ret);

    if (buffer) {
        free(buffer);
        BitMap.pData = NULL;
        buffer = NULL;
    }
    destroy_font();
}

