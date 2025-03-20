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
#include <stdbool.h>
#include <unistd.h>
#include <sys/time.h>
#include "rkadk_common.h"
#include "rkadk_log.h"
#include "rkadk_player.h"

static RKADK_MW_PTR pPlayer = NULL;

int cvr_player_play_audio(char *filepath) {
    int ret;
    RKADK_MW_PTR Player = NULL;
    RKADK_PLAYER_CFG_S stPlayCfg;
    memset(&stPlayCfg, 0, sizeof(RKADK_PLAYER_CFG_S));
    stPlayCfg.bEnableAudio = true;
    stPlayCfg.bEnableVideo = false;
    stPlayCfg.pfnPlayerCallback = NULL;
    if (RKADK_PLAYER_Create(&Player, &stPlayCfg)) {
        RKADK_LOGE("RKADK_PLAYER_Create failed");
        return -1;
    }
    ret = RKADK_PLAYER_SetDataSource(Player, filepath);
    if (ret) {
        printf("RKADK_PLAYER_SetDataSource failed! ret=%d\n", ret);
        return -1;
    }
    ret = RKADK_PLAYER_Prepare(Player);
    if (ret) {
        printf("RKADK_PLAYER_Prepare failed! ret=%d\n");
        return -1;
    }
    ret = RKADK_PLAYER_Play(Player);
    if (ret) {
        printf("RKADK_PLAYER_Play failed!");
        return -1;
    }
    usleep(500 * 1000);
    RKADK_PLAYER_Destroy(Player);
    return 0;
}

static void cvr_player_param_init(RKADK_PLAYER_FRAMEINFO_S *pstFrmInfo) {
    RKADK_CHECK_POINTER_N(pstFrmInfo);
    memset(pstFrmInfo, 0, sizeof(RKADK_PLAYER_FRAMEINFO_S));

    pstFrmInfo->u32VoLayerMode = 1;
    pstFrmInfo->u32VoFormat = VO_FORMAT_RGB888;
    pstFrmInfo->u32VoDev = VO_DEV_HD0;
    pstFrmInfo->u32EnIntfType = DISPLAY_TYPE_MIPI;
    pstFrmInfo->u32DispFrmRt = 30;
    pstFrmInfo->enIntfSync = RKADK_VO_OUTPUT_DEFAULT;
    pstFrmInfo->u32EnMode = CHNN_ASPECT_RATIO_AUTO;
    pstFrmInfo->u32BorderColor = 0x0000FA;
    pstFrmInfo->u32ChnnNum = 1;
    pstFrmInfo->stSyncInfo.bIdv = RKADK_TRUE;
    pstFrmInfo->stSyncInfo.bIhs = RKADK_TRUE;
    pstFrmInfo->stSyncInfo.bIvs = RKADK_TRUE;
    pstFrmInfo->stSyncInfo.bSynm = RKADK_TRUE;
    pstFrmInfo->stSyncInfo.bIop = RKADK_TRUE;
    pstFrmInfo->stSyncInfo.u16FrameRate = 60;
    pstFrmInfo->stSyncInfo.u16PixClock = 65000;
    pstFrmInfo->stSyncInfo.u16Hact = 1200;
    pstFrmInfo->stSyncInfo.u16Hbb = 24;
    pstFrmInfo->stSyncInfo.u16Hfb = 240;
    pstFrmInfo->stSyncInfo.u16Hpw = 136;
    pstFrmInfo->stSyncInfo.u16Hmid = 0;
    pstFrmInfo->stSyncInfo.u16Vact = 1200;
    pstFrmInfo->stSyncInfo.u16Vbb = 200;
    pstFrmInfo->stSyncInfo.u16Vfb = 194;
    pstFrmInfo->stSyncInfo.u16Vpw = 6;
    pstFrmInfo->stVoAttr.u32Rotation = 90;
    return;
}

static RKADK_VOID cvr_player_event(RKADK_MW_PTR Player,
                                    RKADK_PLAYER_EVENT_E enEvent,
                                    RKADK_VOID *pData) {
    int position = 0;

    switch (enEvent) {
        case RKADK_PLAYER_EVENT_STATE_CHANGED:
            printf("RKADK_PLAYER_EVENT_STATE_CHANGED\n");
            break;
        case RKADK_PLAYER_EVENT_EOF:
            printf("RKADK_PLAYER_EVENT_EOF\n");
            break;
        case RKADK_PLAYER_EVENT_SOF:
            printf("RKADK_PLAYER_EVENT_SOF\n");
            break;
        case RKADK_PLAYER_EVENT_PROGRESS:
            if (pData)
                position = *((int *)pData);
            printf("RKADK_PLAYER_EVENT_PROGRESS(%d ms)\n", position);
            break;
        case RKADK_PLAYER_EVENT_SEEK_END:
            printf("RKADK_PLAYER_EVENT_SEEK_END\n");
            break;
        case RKADK_PLAYER_EVENT_ERROR:
            printf("RKADK_PLAYER_EVENT_ERROR\n");
            break;
        case RKADK_PLAYER_EVENT_PREPARED:
            printf("RKADK_PLAYER_EVENT_PREPARED\n");
            break;
        case RKADK_PLAYER_EVENT_STARTED:
            printf("RKADK_PLAYER_EVENT_STARTED\n");
            break;
        case RKADK_PLAYER_EVENT_PAUSED:
            printf("RKADK_PLAYER_EVENT_PAUSED\n");
            break;
        case RKADK_PLAYER_EVENT_STOPPED:
            printf("RKADK_PLAYER_EVENT_STOPPED\n");
            break;
        default:
            printf("Unknown event(%d)\n", enEvent);
            break;
    }
}

int cvr_player_pause(void) {
    int ret;
    RKADK_PLAYER_STATE_E replay_state;

    RKADK_PLAYER_GetPlayStatus(pPlayer, &replay_state);
    if (replay_state == RKADK_PLAYER_STATE_PLAY) {
        ret = RKADK_PLAYER_Pause(pPlayer);
        if (ret) {
            printf("Replay pause fail!\n");
            return -1;
        }
        return PLAYER_PAUSE;
    } else if (replay_state == RKADK_PLAYER_STATE_PAUSE) {
        ret = RKADK_PLAYER_Play(pPlayer);
        if (ret) {
            printf("RKADK_PLAYER_Play failed!");
            return -1;
        }
        return PLAYER_PLAY;
    } else {
        RKADK_LOGE("Replay getplaystatus fail!\n");
    }
    return 0;
}

int cvr_player_start(char *filepath) {
    int ret;
    RKADK_PLAYER_FRAMEINFO_S stFrmInfo;
    RKADK_BOOL bVideoEnable = true;

    RKADK_LOGD("#play file: %s", filepath);
    cvr_player_param_init(&stFrmInfo);
    RKADK_PLAYER_CFG_S stPlayCfg;
    memset(&stPlayCfg, 0, sizeof(RKADK_PLAYER_CFG_S));
    stPlayCfg.bEnableAudio = true;
    stPlayCfg.bEnableVideo = true;
    stPlayCfg.pfnPlayerCallback = cvr_player_event;
    if (RKADK_PLAYER_Create(&pPlayer, &stPlayCfg)) {
        RKADK_LOGE("RKADK_PLAYER_Create failed");
        return -1;
    }
    ret = RKADK_PLAYER_SetDataSource(pPlayer, filepath);
    if (ret) {
        printf("RKADK_PLAYER_SetDataSource failed! ret=%d\n", ret);
        return -1;
    }
    ret = RKADK_PLAYER_SetVideoSink(pPlayer, &stFrmInfo);
    if (ret) {
        printf("RKADK_PLAYER_SetVideoSink failed! ret=%d\n", ret);
        return -1;
    }
    ret = RKADK_PLAYER_Prepare(pPlayer);
    if (ret) {
        printf("RKADK_PLAYER_Prepare failed! ret=%d\n");
        return -1;
    }
    ret = RKADK_PLAYER_Play(pPlayer);
    if (ret) {
        printf("RKADK_PLAYER_Play failed!");
        return -1;
    }
    return 0;
}

int cvr_player_stop(void) {
    //RKADK_PLAYER_Stop(pPlayer);
    RKADK_PLAYER_Destroy(pPlayer);
    pPlayer = NULL;
    return 0;
}

int cvr_player_switch(char *filepath) {
    RKADK_PLAYER_STATE_E replay_state;
    RKADK_PLAYER_GetPlayStatus(pPlayer, &replay_state);
    if (replay_state != RKADK_PLAYER_STATE_IDLE ||
        replay_state != RKADK_PLAYER_STATE_INIT ||
        replay_state != RKADK_PLAYER_STATE_PREPARED) {

        int ret = RKADK_PLAYER_Stop(pPlayer);
        if (ret) {
            RKADK_LOGE("Replay_switch:stop failed, ret = %d", ret);
            return -1;
        }

        ret = RKADK_PLAYER_SetDataSource(pPlayer, filepath);
        if (ret) {
            RKADK_LOGE("Replay_switch:SetDataSource failed, ret = %d", ret);
            return -1;
        }

        ret = RKADK_PLAYER_Prepare(pPlayer);
        if (ret) {
            RKADK_LOGE("Replay_switch:Prepare failed, ret = %d", ret);
            return -1;
        }

        ret = RKADK_PLAYER_Play(pPlayer);
        if (ret) {
            RKADK_LOGE("Replay_switch:Play failed, ret = %d", ret);
            return -1;
        }
        return 0;
   }

   return -1;
}

int cvr_player_get_duration(int *pDuration) {
    int ret;

    if (pPlayer != NULL) {
        printf("replay_get_duration enter\n");
        ret = RKADK_PLAYER_GetDuration(pPlayer, pDuration);
        if (ret) {
            printf("replay_get_duration failed\n");
        }
    }
    return ret;
}
