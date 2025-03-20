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

#ifndef __RKADK_MEDIA_COMM_H__
#define __RKADK_MEDIA_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rkadk_common.h"
#include "rkmedia_api.h"

/* audio capture maximum count */
#define RKADK_MEDIA_AI_MAX_CNT (1)

/* audio encoder maximum count */
#define RKADK_MEDIA_AENC_MAX_CNT (3 * RKADK_MAX_SENSOR_CNT)

/* video capture maximum count */
#define RKADK_MEDIA_VI_MAX_CNT (4 * RKADK_MAX_SENSOR_CNT)

/* video encoder maximum count */
#define RKADK_MEDIA_VENC_MAX_CNT RKADK_MEDIA_VI_MAX_CNT

/* rga maximum count */
#define RKADK_MEDIA_RGA_MAX_CNT (6 * RKADK_MAX_SENSOR_CNT)

/* ai aenc maximum bind count */
#define RKADK_AI_AENC_MAX_BIND_CNT RKADK_MEDIA_AENC_MAX_CNT

/* vi venc maximum bind count */
#define RKADK_VI_VENC_MAX_BIND_CNT RKADK_MEDIA_VENC_MAX_CNT

/* vi rga maximum bind count */
#define RKADK_VI_RGA_MAX_BIND_CNT RKADK_MEDIA_RGA_MAX_CNT

/* rga venc maximum bind count */
#define RKADK_RGA_VENC_MAX_BIND_CNT RKADK_MEDIA_RGA_MAX_CNT

RKADK_S32 RKADK_MPI_AI_Init(RKADK_S32 s32AiChnId, AI_CHN_ATTR_S *pstAiChnAttr,
                            RKADK_VQE_MODE_E enMode);
RKADK_S32 RKADK_MPI_AI_DeInit(RKADK_S32 s32AiChnId, RKADK_VQE_MODE_E enMode);

RKADK_S32 RKADK_MPI_AENC_Init(RKADK_S32 s32AencChnId,
                              AENC_CHN_ATTR_S *pstAencChnAttr);
RKADK_S32 RKADK_MPI_AENC_DeInit(RKADK_S32 s32AencChnId);

RKADK_S32 RKADK_MPI_VI_Init(RKADK_U32 u32CamId, RKADK_S32 s32ChnId,
                            VI_CHN_ATTR_S *pstViChnAttr);
RKADK_S32 RKADK_MPI_VI_DeInit(RKADK_U32 u32CamId, RKADK_S32 s32ChnId);

RKADK_S32 RKADK_MPI_VENC_Init(RKADK_S32 s32ChnId,
                              VENC_CHN_ATTR_S *pstVencChnAttr);
RKADK_S32 RKADK_MPI_VENC_DeInit(RKADK_S32 s32ChnId);

RK_S32 RKADK_MPI_RGA_Init(RKADK_S32 s32ChnId, RGA_ATTR_S *pstRgaAttr);

RK_S32 RKADK_MPI_RGA_DeInit(RKADK_S32 s32ChnId);

RKADK_S32 RKADK_MPI_SYS_Bind(const MPP_CHN_S *pstSrcChn,
                             const MPP_CHN_S *pstDestChn);
RKADK_S32 RKADK_MPI_SYS_UnBind(const MPP_CHN_S *pstSrcChn,
                               const MPP_CHN_S *pstDestChn);

RKADK_CODEC_TYPE_E RKADK_MEDIA_GetCodecType(CODEC_TYPE_E enType);
CODEC_TYPE_E RKADK_MEDIA_GetRkCodecType(RKADK_CODEC_TYPE_E enType);

RKADK_S32 RKADK_MEDIA_SetRcAttr(VENC_RC_ATTR_S *pstRcAttr, RKADK_U32 u32Gop,
                                RKADK_U32 u32BitRate, RKADK_U32 u32SrcFrameRate,
                                RKADK_U32 u32DstFrameRate);

RKADK_S32 RKADK_MEDIA_GetMediaBuffer(MPP_CHN_S *pstChn, OutCbFuncEx pfnDataCB,
                                     RKADK_VOID *pHandle);

RKADK_S32 RKADK_MEDIA_StopGetMediaBuffer(MPP_CHN_S *pstChn,
                                         OutCbFuncEx pfnDataCB);

RKADK_S32 RKADK_MEDIA_FrameBufMalloc(RKADK_FRAME_ATTR_S *pstFrameAttr);

RKADK_S32 RKADK_MEDIA_FrameFree(RKADK_FRAME_ATTR_S *pstFrameAttr);

bool RKADK_MEDIA_CheckFrameAttr(RKADK_FRAME_ATTR_S *pstFrameAttr);

#ifdef __cplusplus
}
#endif
#endif
