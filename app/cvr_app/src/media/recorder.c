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

#include "recorder.h"

#include <stdint.h>
#include <string.h>
#include <rkadk/rkadk_record.h>
#include <rkadk/rkadk_storage.h>

extern RKADK_MW_PTR pStorage_Handle;

typedef struct sRecorder {
    RecorderState state;
    RKADK_MW_PTR adk_rec;
} Recorder;

RKADK_MW_PTR g_recorder = NULL;

static int cvr_recorder_make_filename(RKADK_MW_PTR pRecorder, RKADK_U32 u32FileCnt,
                                      RKADK_CHAR (*paszFilename)[RKADK_MAX_FILE_PATH_LEN])
{
    RKADK_STR_DEV_ATTR ostDevAttr;
    time_t timep;
    struct tm *p;
    char file_name_buf[RKADK_MAX_FILE_PATH_LEN] = { 0 };

    RKADK_LOGD("u32FileCnt:%d, pRecorder:%p", u32FileCnt, pRecorder);
    time(&timep);
    p = gmtime(&timep);
    ostDevAttr = RKADK_STORAGE_GetDevAttr(pStorage_Handle);
    for(int i = 0; i < u32FileCnt; i++) {
        if (i == 0) {
            snprintf(file_name_buf, sizeof(file_name_buf), "%s%s%04d%02d%02d_%02d%02d%02d.mp4",
                     ostDevAttr.cMountPath, ostDevAttr.pstFolderAttr[0].cFolderPath,
                     (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
                     p->tm_min, p->tm_sec);
        } else {
            snprintf(file_name_buf, sizeof(file_name_buf), "%s%s%04d%02d%02d_%02d%02d%02d_s.mp4",
                     ostDevAttr.cMountPath, ostDevAttr.pstFolderAttr[0].cFolderPath,
                     (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour,
                     p->tm_min, p->tm_sec);
        }
        strncpy(paszFilename[i], file_name_buf, strlen(file_name_buf) + 1);
        printf("Record[%d] save to path:%s\n", i, paszFilename[i]);
    }
    return 0;
}

int cvr_recorder_start(int cam_id)
{
    RKADK_RECORD_ATTR_S rec_attr;

    memset(&rec_attr, 0, sizeof(rec_attr));

    // set default value
    rec_attr.s32CamID = cam_id;
    rec_attr.enRecType = RKADK_REC_TYPE_NORMAL;
    rec_attr.pfnRequestFileNames = cvr_recorder_make_filename;

    RKADK_RECORD_Create(&rec_attr, &g_recorder);
    RKADK_RECORD_Start(g_recorder);

    return 0;
}

int cvr_recorder_stop(void)
{
    RKADK_RECORD_Destroy(g_recorder);
    g_recorder = NULL;
    return 0;
}
