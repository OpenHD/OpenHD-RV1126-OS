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

#include "photo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <rkadk/rkadk_storage.h>
#include <rkadk/rkadk_param.h>

extern RKADK_MW_PTR pStorage_Handle;
static RKADK_PHOTO_ATTR_S *pstPhotoAttr = NULL;

int photo_display_start(char *photo_path, RKADK_PHOTO_DATA_ATTR_S *pDataAttr,
                RKADK_U32 photo_width, RKADK_U32 photo_height) {

    RKADK_CHECK_POINTER(pDataAttr, RKADK_FAILURE);
    memset(pDataAttr, 0, sizeof(RKADK_PHOTO_DATA_ATTR_S));
    pDataAttr->enType = RKADK_THUMB_TYPE_RGBA8888;
    pDataAttr->u32Width = photo_width;
    pDataAttr->u32Height = photo_height;

    if (!RKADK_PHOTO_GetData(photo_path, pDataAttr)) {
        RKADK_LOGD("u32Width: %d, u32Height: %d, u32BufSize: %d\n",
            pDataAttr->u32Width, pDataAttr->u32Height,
            pDataAttr->u32BufSize);
    } else {
        RKADK_LOGE("RKADK_PHOTO_GetData failed");
        return -1;
    }
    return 0;
}

int photo_display_stop(RKADK_PHOTO_DATA_ATTR_S *pDataAttr) {
    RKADK_CHECK_POINTER(pDataAttr, RKADK_FAILURE);
    RKADK_PHOTO_FreeData(pDataAttr);
    return 0;
}

static void photo_data_recv(RKADK_U8 *pu8DataBuf, RKADK_U32 u32DataLen) {
    RKADK_S32 i;
    time_t timep;
    struct tm *p;
    static RKADK_U32 photoId = 0;
    RKADK_STR_DEV_ATTR stDevAttr;
    char jpegPath[MAX_PATH_LEN];
    FILE *file = NULL;

    RKADK_LOGI("Photo_DataRecv enter\n");
    if (!pu8DataBuf || u32DataLen <= 0) {
        RKADK_LOGE("Invalid photo data, u32DataLen = %d", u32DataLen);
        return;
    }
    if (DISK_UNMOUNTED == RKADK_STORAGE_GetMountStatus(pStorage_Handle)) {
       RKADK_LOGE("Disk unmount!Create photo failed");
       return;
    }
    time(&timep);
    p = gmtime(&timep);
    stDevAttr = RKADK_STORAGE_GetDevAttr(pStorage_Handle);
    memset(jpegPath, 0, MAX_PATH_LEN);
    snprintf(jpegPath, sizeof(jpegPath), "%s%s%04d%02d%02d_%02d%02d%02d.jpg",
            stDevAttr.cMountPath, stDevAttr.pstFolderAttr[2].cFolderPath,
            (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
            p->tm_min, p->tm_sec);
    printf("Save photo path:%s\n", jpegPath);
    if (!access(jpegPath, 0)) {
        RKADK_LOGE("Take photos too frequent!");
        return;
    }
    file = fopen(jpegPath, "w");
    if (!file) {
        RKADK_LOGE("Create jpeg file(%s) failed", jpegPath);
        return;
    }
    RKADK_LOGD("Save jpeg to %s", jpegPath);
    fwrite(pu8DataBuf, 1, u32DataLen, file);
    fclose(file);
}

int photo_init(RKADK_U32 u32CamID) {
    int ret;

    if (!pstPhotoAttr) {
        pstPhotoAttr = (RKADK_PHOTO_ATTR_S *)malloc(sizeof(RKADK_PHOTO_ATTR_S));
        if (!pstPhotoAttr) {
            printf("Malloc RKADK_PHOTO_ATTR_S fail!\n");
            return -1;
        }
    }

    memset(pstPhotoAttr, 0 ,sizeof(RKADK_PHOTO_ATTR_S));
    RKADK_PARAM_THUMB_CFG_S *ptsThumbCfg = RKADK_PARAM_GetThumbCfg();
    if (!ptsThumbCfg) {
        RKADK_LOGE("RKADK_PARAM_GetThumbCfg failed");
        free(pstPhotoAttr);
        pstPhotoAttr = NULL;
        return -1;
    }

    pstPhotoAttr->u32CamID = u32CamID;
    pstPhotoAttr->enPhotoType = RKADK_PHOTO_TYPE_SINGLE;
    pstPhotoAttr->unPhotoTypeAttr.stSingleAttr.s32Time_sec = 0;
    pstPhotoAttr->pfnPhotoDataProc = photo_data_recv;
    pstPhotoAttr->stThumbAttr.bSupportDCF = RKADK_FALSE;
    pstPhotoAttr->stThumbAttr.stMPFAttr.eMode = RKADK_PHOTO_MPF_SINGLE;
    pstPhotoAttr->stThumbAttr.stMPFAttr.sCfg.u8LargeThumbNum = 1;
    pstPhotoAttr->stThumbAttr.stMPFAttr.sCfg.astLargeThumbSize[0].u32Width = ptsThumbCfg->thumb_width;
    pstPhotoAttr->stThumbAttr.stMPFAttr.sCfg.astLargeThumbSize[0].u32Height = ptsThumbCfg->thumb_height;
    RKADK_LOGI("Thumb_width:%d, Thumb_height:%d, #Camera id: %d\n",
            ptsThumbCfg->thumb_width, ptsThumbCfg->thumb_height, pstPhotoAttr->u32CamID);
    ret = RKADK_PHOTO_Init(pstPhotoAttr);
    if (ret) {
        RKADK_LOGE("RKADK_PHOTO_Init failed(%d)", ret);
        free(pstPhotoAttr);
        pstPhotoAttr = NULL;
        return -1;
    }
    return 0;
}

int photo_deinit() {
    int ret, u32CamID;

    if (pstPhotoAttr) {
        u32CamID = pstPhotoAttr->u32CamID;
        free(pstPhotoAttr);
        pstPhotoAttr = NULL;
        ret = RKADK_PHOTO_DeInit(u32CamID);
        if (ret) {
            RKADK_LOGE("Photo_Deinit failed");
            return -1;
        }
    }

    return 0;
}

int photo_take_photo() {
    int ret;

    if (pstPhotoAttr) {
        ret = RKADK_PHOTO_TakePhoto(pstPhotoAttr);
        if (ret) {
            RKADK_LOGE("RKADK_PHOTO_TakePhoto failed");
            return -1;
        }
    }
    return 0;
}

