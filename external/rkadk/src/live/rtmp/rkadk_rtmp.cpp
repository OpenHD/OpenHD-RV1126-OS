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

#include "rkadk_rtmp.h"
#include "rkadk_log.h"
#include "rkadk_media_comm.h"
#include "rkadk_param.h"
#include "rkmedia_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  RKADK_U32 u32CamId;
  RKADK_U32 u32MuxerChnId;
} RKADK_RTMP_HANDLE_S;

static void RKADK_RTMP_AudioSetChn(MPP_CHN_S *pstAiChn, MPP_CHN_S *pstAencChn) {
  pstAiChn->enModId = RK_ID_AI;
  pstAiChn->s32DevId = 0;
  pstAiChn->s32ChnId = LIVE_AI_CHN;

  pstAencChn->enModId = RK_ID_AENC;
  pstAencChn->s32DevId = 0;
  pstAencChn->s32ChnId = LIVE_AENC_CHN;
}

static RKADK_S32 RKADK_RTMP_SetAiAttr(AI_CHN_ATTR_S *pstAiAttr,
                                      RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg) {
  RKADK_CHECK_POINTER(pstAiAttr, RKADK_FAILURE);

  pstAiAttr->pcAudioNode = pstAudioCfg->audio_node;
  pstAiAttr->enSampleFormat = pstAudioCfg->sample_format;
  pstAiAttr->u32NbSamples = pstAudioCfg->samples_per_frame;
  pstAiAttr->u32SampleRate = pstAudioCfg->samplerate;
  pstAiAttr->u32Channels = pstAudioCfg->channels;
  pstAiAttr->enAiLayout = pstAudioCfg->ai_layout;
  return 0;
}

static RKADK_S32 RKADK_RTMP_SetAencAttr(RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg,
                                        AENC_CHN_ATTR_S *pstAencAttr) {

  RKADK_CHECK_POINTER(pstAencAttr, RKADK_FAILURE);

  pstAencAttr->u32Bitrate = pstAudioCfg->bitrate;
  switch (pstAudioCfg->codec_type) {
  case RKADK_CODEC_TYPE_G711A:
    pstAencAttr->stAencG711A.u32Channels = pstAudioCfg->channels;
    pstAencAttr->stAencG711A.u32NbSample = pstAudioCfg->samples_per_frame;
    pstAencAttr->stAencG711A.u32SampleRate = pstAudioCfg->samplerate;
    break;
  case RKADK_CODEC_TYPE_G711U:
    pstAencAttr->stAencG711U.u32Channels = pstAudioCfg->channels;
    pstAencAttr->stAencG711U.u32NbSample = pstAudioCfg->samples_per_frame;
    pstAencAttr->stAencG711U.u32SampleRate = pstAudioCfg->samplerate;
    break;
  case RKADK_CODEC_TYPE_MP3:
    pstAencAttr->stAencMP3.u32Channels = pstAudioCfg->channels;
    pstAencAttr->stAencMP3.u32SampleRate = pstAudioCfg->samplerate;
    break;
  case RKADK_CODEC_TYPE_G726:
    pstAencAttr->stAencG726.u32Channels = pstAudioCfg->channels;
    pstAencAttr->stAencG726.u32SampleRate = pstAudioCfg->samplerate;
    break;
  case RKADK_CODEC_TYPE_MP2:
    pstAencAttr->stAencMP2.u32Channels = pstAudioCfg->channels;
    pstAencAttr->stAencMP2.u32SampleRate = pstAudioCfg->samplerate;
    break;

  default:
    RKADK_LOGE("Nonsupport codec type(%d)", pstAudioCfg->codec_type);
    return -1;
  }

  pstAencAttr->enCodecType =
      RKADK_MEDIA_GetRkCodecType(pstAudioCfg->codec_type);
  pstAencAttr->u32Quality = 1;
  return 0;
}

static void RKADK_RTMP_VideoSetChn(RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                   MPP_CHN_S *pstViChn, MPP_CHN_S *pstVencChn,
                                   MPP_CHN_S *pstRgaChn) {
  pstViChn->enModId = RK_ID_VI;
  pstViChn->s32DevId = 0;
  pstViChn->s32ChnId = pstLiveCfg->vi_attr.u32ViChn;

  pstRgaChn->enModId = RK_ID_RGA;
  pstRgaChn->s32DevId = 0;
  pstRgaChn->s32ChnId = pstLiveCfg->attribute.rga_chn;

  pstVencChn->enModId = RK_ID_VENC;
  pstVencChn->s32DevId = 0;
  pstVencChn->s32ChnId = pstLiveCfg->attribute.venc_chn;
}

static int RKADK_RTMP_SetVencAttr(RKADK_U32 u32CamId,
                                  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                  VENC_CHN_ATTR_S *pstVencAttr) {
  int ret;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  RKADK_CHECK_POINTER(pstVencAttr, RKADK_FAILURE);
  memset(pstVencAttr, 0, sizeof(VENC_CHN_ATTR_S));

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  pstVencAttr->stRcAttr.enRcMode = RKADK_PARAM_GetRcMode(
      pstLiveCfg->attribute.rc_mode, pstLiveCfg->attribute.codec_type);

  ret = RKADK_MEDIA_SetRcAttr(&pstVencAttr->stRcAttr, pstLiveCfg->attribute.gop,
                              pstLiveCfg->attribute.bitrate,
                              pstSensorCfg->framerate, pstSensorCfg->framerate);
  if (ret) {
    RKADK_LOGE("RKADK_MEDIA_SetRcAttr failed");
    return -1;
  }

  if (pstLiveCfg->attribute.codec_type == RKADK_CODEC_TYPE_H265)
    pstVencAttr->stVencAttr.stAttrH265e.bScaleList =
        (RK_BOOL)pstLiveCfg->attribute.venc_param.scaling_list;

  pstVencAttr->stVencAttr.enType =
      RKADK_MEDIA_GetRkCodecType(pstLiveCfg->attribute.codec_type);
  pstVencAttr->stVencAttr.imageType = pstLiveCfg->vi_attr.stChnAttr.enPixFmt;
  pstVencAttr->stVencAttr.u32PicWidth = pstLiveCfg->attribute.width;
  pstVencAttr->stVencAttr.u32PicHeight = pstLiveCfg->attribute.height;
  pstVencAttr->stVencAttr.u32VirWidth = pstLiveCfg->attribute.width;
  pstVencAttr->stVencAttr.u32VirHeight = pstLiveCfg->attribute.height;
  pstVencAttr->stVencAttr.u32Profile = pstLiveCfg->attribute.profile;
  pstVencAttr->stVencAttr.bFullRange =
      (RK_BOOL)pstLiveCfg->attribute.venc_param.full_range;

  return 0;
}

static int RKADK_RTMP_SetMuxerAttr(RKADK_U32 u32CamId, char *path,
                                   RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                   RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg,
                                   MUXER_CHN_ATTR_S *pstMuxerAttr) {
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  memset(pstMuxerAttr, 0, sizeof(MUXER_CHN_ATTR_S));
  pstMuxerAttr->u32MuxerId =
      RECORD_FILE_NUM_MAX * RKADK_MAX_SENSOR_CNT + u32CamId;
  pstMuxerAttr->enMode = MUXER_MODE_SINGLE;
  pstMuxerAttr->enType = MUXER_TYPE_FLV;
  pstMuxerAttr->pcOutputFile = path;
  pstMuxerAttr->stVideoStreamParam.enCodecType =
      RKADK_MEDIA_GetRkCodecType(pstLiveCfg->attribute.codec_type);
  pstMuxerAttr->stVideoStreamParam.enImageType =
      pstLiveCfg->vi_attr.stChnAttr.enPixFmt;
  pstMuxerAttr->stVideoStreamParam.u16Fps = pstSensorCfg->framerate;
  pstMuxerAttr->stVideoStreamParam.u16Level = 41;
  pstMuxerAttr->stVideoStreamParam.u16Profile = pstLiveCfg->attribute.profile;
  pstMuxerAttr->stVideoStreamParam.u32BitRate = pstLiveCfg->attribute.bitrate;
  pstMuxerAttr->stVideoStreamParam.u32Width = pstLiveCfg->attribute.width;
  pstMuxerAttr->stVideoStreamParam.u32Height = pstLiveCfg->attribute.height;

  if (pstAudioCfg->codec_type == RKADK_CODEC_TYPE_MP3) {
    pstMuxerAttr->stAudioStreamParam.enCodecType =
        RKADK_MEDIA_GetRkCodecType(pstAudioCfg->codec_type);
    pstMuxerAttr->stAudioStreamParam.enSampFmt = pstAudioCfg->sample_format;
    pstMuxerAttr->stAudioStreamParam.u32Channels = pstAudioCfg->channels;
    pstMuxerAttr->stAudioStreamParam.u32NbSamples =
        pstAudioCfg->samples_per_frame;
    pstMuxerAttr->stAudioStreamParam.u32SampleRate = pstAudioCfg->samplerate;
  }

  return 0;
}

static bool RKADK_RTMP_IsUseRga(RKADK_U32 u32CamId,
                                RKADK_PARAM_STREAM_CFG_S *pstLiveCfg) {
  bool bUseRga = false;
  RKADK_U32 u32SrcWidth = pstLiveCfg->vi_attr.stChnAttr.u32Width;
  RKADK_U32 u32SrcHeight = pstLiveCfg->vi_attr.stChnAttr.u32Height;
  RKADK_U32 u32DstWidth = pstLiveCfg->attribute.width;
  RKADK_U32 u32DstHeight = pstLiveCfg->attribute.height;

  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return false;
  }

  if (u32DstWidth != u32SrcWidth || u32DstHeight != u32SrcHeight) {
    RKADK_LOGD("In[%d, %d], Out[%d, %d]", u32SrcWidth, u32SrcHeight,
               u32DstWidth, u32DstHeight);
    bUseRga = true;
  }

  if (!pstSensorCfg->used_isp) {
    if (pstSensorCfg->flip || pstSensorCfg->mirror)
      bUseRga = true;
  }

  return bUseRga;
}

static RKADK_S32 RKADK_RTMP_EnableVideo(RKADK_U32 u32CamId, MPP_CHN_S stViChn,
                                        MPP_CHN_S stVencChn, MPP_CHN_S stRgaChn,
                                        RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                        bool bUseRga) {
  int ret = 0;
  VENC_RC_PARAM_S stVencRcParam;
  VENC_CHN_ATTR_S stVencChnAttr;
  RGA_ATTR_S stRgaAttr;

  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = RKADK_PARAM_GetSensorCfg(u32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  // Create VI
  ret = RKADK_MPI_VI_Init(u32CamId, stViChn.s32ChnId,
                          &(pstLiveCfg->vi_attr.stChnAttr));
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_Init faled %d", ret);
    return ret;
  }

  // Create RGA
  if (bUseRga) {
    memset(&stRgaAttr, 0, sizeof(RGA_ATTR_S));

    if (!pstSensorCfg->used_isp) {
      if (pstSensorCfg->flip && pstSensorCfg->mirror)
        stRgaAttr.enFlip = RGA_FLIP_HV;
      else if (pstSensorCfg->flip)
        stRgaAttr.enFlip = RGA_FLIP_V;
      else if (pstSensorCfg->mirror)
        stRgaAttr.enFlip = RGA_FLIP_H;
    }

    stRgaAttr.bEnBufPool = RK_TRUE;
    stRgaAttr.u16BufPoolCnt = 3;
    stRgaAttr.stImgIn.imgType = pstLiveCfg->vi_attr.stChnAttr.enPixFmt;
    stRgaAttr.stImgIn.u32Width = pstLiveCfg->vi_attr.stChnAttr.u32Width;
    stRgaAttr.stImgIn.u32Height = pstLiveCfg->vi_attr.stChnAttr.u32Height;
    stRgaAttr.stImgIn.u32HorStride = pstLiveCfg->vi_attr.stChnAttr.u32Width;
    stRgaAttr.stImgIn.u32VirStride = pstLiveCfg->vi_attr.stChnAttr.u32Height;
    stRgaAttr.stImgOut.imgType = stRgaAttr.stImgIn.imgType;
    stRgaAttr.stImgOut.u32Width = pstLiveCfg->attribute.width;
    stRgaAttr.stImgOut.u32Height = pstLiveCfg->attribute.height;
    stRgaAttr.stImgOut.u32HorStride = pstLiveCfg->attribute.width;
    stRgaAttr.stImgOut.u32VirStride = pstLiveCfg->attribute.height;
    ret = RKADK_MPI_RGA_Init(stRgaChn.s32ChnId, &stRgaAttr);
    if (ret) {
      RKADK_LOGE("Init Rga[%d] falied[%d]", stRgaChn.s32ChnId, ret);
      goto failed;
    }
  }

  // Create VENC
  ret = RKADK_RTMP_SetVencAttr(u32CamId, pstLiveCfg, &stVencChnAttr);
  if (ret) {
    RKADK_LOGE("RKADK_RTMP_SetVencAttr failed");
    goto failed;
  }

  ret = RKADK_MPI_VENC_Init(stVencChn.s32ChnId, &stVencChnAttr);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VENC_Init failed(%d)", ret);
    goto failed;
  }

  ret = RKADK_PARAM_GetRcParam(pstLiveCfg->attribute, &stVencRcParam);
  if (!ret) {
    ret = RK_MPI_VENC_SetRcParam(stVencChn.s32ChnId, &stVencRcParam);
    if (ret)
      RKADK_LOGW("RK_MPI_VENC_SetRcParam failed(%d), use default cfg", ret);
  }

  return 0;
failed:
  if (bUseRga)
    RKADK_MPI_RGA_DeInit(stRgaChn.s32ChnId);

  RKADK_MPI_VI_DeInit(u32CamId, stViChn.s32ChnId);
  return ret;
}

static RKADK_S32 RKADK_RTMP_DisableVideo(RKADK_U32 u32CamId, MPP_CHN_S stViChn,
                                         MPP_CHN_S stVencChn,
                                         MPP_CHN_S stRgaChn, bool bUseRga) {
  int ret;

  // Destroy VENC before VI
  ret = RKADK_MPI_VENC_DeInit(stVencChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VENC_DeInit failed(%d)", ret);
    return ret;
  }

  // Destory RGA
  if (bUseRga) {
    ret = RKADK_MPI_RGA_DeInit(stRgaChn.s32ChnId);
    if (ret) {
      RKADK_LOGE("DeInit RGA[%d] failed[%d]", stRgaChn.s32ChnId, ret);
      return ret;
    }
  }

  // Destroy VI
  ret = RKADK_MPI_VI_DeInit(u32CamId, stViChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_VI_DeInit failed %d", ret);
    return ret;
  }

  return 0;
}

static RKADK_S32 RKADK_RTMP_EnableAudio(MPP_CHN_S stAiChn, MPP_CHN_S stAencChn,
                                        RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg) {
  int ret;
  AI_CHN_ATTR_S stAiAttr;
  AENC_CHN_ATTR_S stAencAttr;

  // Create AI and AENC
  ret = RKADK_RTMP_SetAiAttr(&stAiAttr, pstAudioCfg);
  if (ret) {
    RKADK_LOGE("RKADK_RTMP_SetAiAttr failed");
    return ret;
  }

  ret = RKADK_MPI_AI_Init(stAiChn.s32ChnId, &stAiAttr, pstAudioCfg->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_Init faile(%d)", ret);
    return ret;
  }

  if (RKADK_RTMP_SetAencAttr(pstAudioCfg, &stAencAttr)) {
    RKADK_LOGE("RKADK_RTMP_SetAencAttr error");
    goto failed;
  }

  ret = RKADK_MPI_AENC_Init(stAencChn.s32ChnId, &stAencAttr);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AENC_Init error %d", ret);
    goto failed;
  }

  return 0;
failed:
  RKADK_MPI_AI_DeInit(stAiChn.s32ChnId, pstAudioCfg->vqe_mode);
  return ret;
}

static RKADK_S32 RKADK_RTMP_DisableAudio(MPP_CHN_S stAiChn, MPP_CHN_S stAencChn,
                                         RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg) {
  int ret;

  // Destroy AENC before AI
  ret = RKADK_MPI_AENC_DeInit(stAencChn.s32ChnId);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AENC_DeInit failed(%d)", ret);
    return ret;
  }

  // Destroy AI
  ret = RKADK_MPI_AI_DeInit(stAiChn.s32ChnId, pstAudioCfg->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_DeInit failed %d", ret);
    return ret;
  }

  return 0;
}

static RKADK_S32 RKADK_RTMP_EnableMuxer(RKADK_U32 u32CamId, char *path,
                                        RKADK_PARAM_STREAM_CFG_S *pstLiveCfg,
                                        RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg,
                                        MUXER_CHN_ATTR_S *pstMuxerAttr) {
  int ret = 0;

  ret = RKADK_RTMP_SetMuxerAttr(u32CamId, (char *)path, pstLiveCfg, pstAudioCfg,
                                pstMuxerAttr);
  if (ret) {
    RKADK_LOGE("RKADK_RTMP_SetMuxerAttr failed");
    return ret;
  }

  ret = RK_MPI_MUXER_EnableChn(pstMuxerAttr->u32MuxerId, pstMuxerAttr);
  if (ret) {
    RKADK_LOGE("Create MUXER[%d] failed[%d]", pstMuxerAttr->u32MuxerId, ret);
    return ret;
  }

  ret = RK_MPI_MUXER_StreamStart(pstMuxerAttr->u32MuxerId);
  if (ret) {
    RKADK_LOGE("Muxer start stream failed[%d]", ret);
    return ret;
  }

  return 0;
}

RKADK_S32 RKADK_RTMP_Init(RKADK_U32 u32CamId, const char *path,
                          RKADK_MW_PTR *ppHandle) {
  int ret = 0;
  bool bEnableAudio, bUseRga = false;
  MPP_CHN_S stAiChn, stAencChn;
  MPP_CHN_S stViChn, stVencChn, stRgaChn;
  MUXER_CHN_S stMuxerChn;
  MUXER_CHN_ATTR_S stMuxerAttr;
  RKADK_RTMP_HANDLE_S *pHandle;

  RKADK_CHECK_CAMERAID(u32CamId, RKADK_FAILURE);
  RKADK_CHECK_POINTER(path, RKADK_FAILURE);

  RKADK_LOGI("Rtmp[%d, %s] Init...", u32CamId, path);

  if (*ppHandle) {
    RKADK_LOGE("rtmp handle has been created");
    return -1;
  }

  RK_MPI_SYS_Init();
  RKADK_PARAM_Init(NULL, NULL);

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(u32CamId, RKADK_STREAM_TYPE_LIVE);
  if (!pstLiveCfg) {
    RKADK_LOGE("Live RKADK_PARAM_GetStreamCfg Live failed");
    return -1;
  }

  bUseRga = RKADK_RTMP_IsUseRga(u32CamId, pstLiveCfg);

  RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioCfg) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  /* mp2 not compatible with flv */
  bEnableAudio = pstAudioCfg->codec_type == RKADK_CODEC_TYPE_MP3 ? true : false;

  pHandle = (RKADK_RTMP_HANDLE_S *)malloc(sizeof(RKADK_RTMP_HANDLE_S));
  if (!pHandle) {
    RKADK_LOGE("malloc pHandle failed");
    return -1;
  }
  memset(pHandle, 0, sizeof(RKADK_RTMP_HANDLE_S));
  pHandle->u32CamId = u32CamId;

  RKADK_RTMP_VideoSetChn(pstLiveCfg, &stViChn, &stVencChn, &stRgaChn);
  ret = RKADK_RTMP_EnableVideo(u32CamId, stViChn, stVencChn, stRgaChn,
                               pstLiveCfg, bUseRga);
  if (ret) {
    RKADK_LOGE("RKADK_RTMP_EnableVideo failed[%d]", ret);
    free(pHandle);
    return ret;
  }

  if (bEnableAudio) {
    RKADK_RTMP_AudioSetChn(&stAiChn, &stAencChn);
    ret = RKADK_RTMP_EnableAudio(stAiChn, stAencChn, pstAudioCfg);
    if (ret) {
      RKADK_LOGE("RKADK_RTMP_EnableAudio failed[%d]", ret);
      RKADK_RTMP_DisableVideo(u32CamId, stViChn, stVencChn, stRgaChn, bUseRga);
      free(pHandle);
      return ret;
    }
  }

  // Create Muxer
  ret = RKADK_RTMP_EnableMuxer(u32CamId, (char *)path, pstLiveCfg, pstAudioCfg,
                               &stMuxerAttr);
  if (ret) {
    RKADK_LOGE("RKADK_RTMP_EnableMuxer failed");
    goto failed;
  }
  pHandle->u32MuxerChnId = stMuxerAttr.u32MuxerId;

  // Bind VENC to MUXER:VIDEO
  stMuxerChn.enModId = RK_ID_MUXER;
  stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
  stMuxerChn.s32ChnId = stMuxerAttr.u32MuxerId;
  ret = RK_MPI_MUXER_Bind(&stVencChn, &stMuxerChn);
  if (ret) {
    RKADK_LOGE("Bind VENC[%d] and MUXER[%d]:VIDEO failed[%d]",
               stVencChn.s32ChnId, stMuxerChn.s32ChnId, ret);
    goto disable_muxer;
  }

  // Bind AENC[0] to MUXER[0]:AUDIO
  if (bEnableAudio) {
    stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
    ret = RK_MPI_MUXER_Bind(&stAencChn, &stMuxerChn);
    if (ret) {
      RKADK_LOGE("Bind AENC[%d] and MUXER[%d]:AUDIO failed[%d]",
                 stAencChn.s32ChnId, stMuxerChn.s32ChnId, ret);
      goto unbind_muxer_v;
    }
  }

  if (bUseRga) {
    // RGA Bind VENC
    ret = RKADK_MPI_SYS_Bind(&stRgaChn, &stVencChn);
    if (ret) {
      RKADK_LOGE("Bind RGA[%d] to VENC[%d] failed[%d]", stRgaChn.s32ChnId,
                 stVencChn.s32ChnId, ret);
      goto unbind_muxer;
    }

    // VI Bind RGA
    ret = RKADK_MPI_SYS_Bind(&stViChn, &stRgaChn);
    if (ret) {
      RKADK_LOGE("Bind VI[%d] to RGA[%d] failed[%d]", stViChn.s32ChnId,
                 stRgaChn.s32ChnId, ret);
      RKADK_MPI_SYS_UnBind(&stRgaChn, &stVencChn);
      goto unbind_muxer;
    }
  } else {
    // Bind VI to VENC
    ret = RKADK_MPI_SYS_Bind(&stViChn, &stVencChn);
    if (ret) {
      RKADK_LOGE("Bind VI[%d] and VENC[%d] failed[%d]", stViChn.s32ChnId,
                 stVencChn.s32ChnId, ret);
      goto unbind_muxer;
    }
  }

  // Bind AI to AENC
  if (bEnableAudio) {
    ret = RKADK_MPI_SYS_Bind(&stAiChn, &stAencChn);
    if (ret) {
      RKADK_LOGE("Bind AI[%d] and AENC[%d] failed[%d]", stAiChn.s32ChnId,
                 stAencChn.s32ChnId, ret);
      goto unbind;
    }
  }

  *ppHandle = (RKADK_MW_PTR)pHandle;
  RKADK_LOGI("Rtmp[%d, %s] Init End...", u32CamId, path);
  return 0;

unbind:
  if (bUseRga) {
    RKADK_MPI_SYS_UnBind(&stRgaChn, &stVencChn);
    RKADK_MPI_SYS_UnBind(&stViChn, &stRgaChn);
  } else {
    RKADK_MPI_SYS_UnBind(&stViChn, &stVencChn);
  }

unbind_muxer:
  if (bEnableAudio) {
    stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
    RK_MPI_MUXER_UnBind(&stAencChn, &stMuxerChn);
  }

unbind_muxer_v:
  stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
  RK_MPI_MUXER_UnBind(&stVencChn, &stMuxerChn);

disable_muxer:
  RK_MPI_MUXER_DisableChn(stMuxerChn.s32ChnId);

failed:
  RKADK_RTMP_DisableVideo(u32CamId, stViChn, stVencChn, stRgaChn, bUseRga);

  if (bEnableAudio)
    RKADK_RTMP_DisableAudio(stAiChn, stAencChn, pstAudioCfg);

  if (pHandle)
    free(pHandle);

  RKADK_LOGE("failed");
  return ret;
}

RKADK_S32 RKADK_RTMP_DeInit(RKADK_MW_PTR pHandle) {
  int ret = 0;
  bool bDisableAudio, bUseRga = false;
  MPP_CHN_S stAiChn, stAencChn;
  MPP_CHN_S stViChn, stVencChn, stRgaChn;
  MUXER_CHN_S stMuxerChn;

  RKADK_CHECK_POINTER(pHandle, RKADK_FAILURE);
  RKADK_RTMP_HANDLE_S *pstHandle = (RKADK_RTMP_HANDLE_S *)pHandle;

  RKADK_LOGI("Rtmp[%d] DeInit...", pstHandle->u32CamId);

  RKADK_PARAM_STREAM_CFG_S *pstLiveCfg =
      RKADK_PARAM_GetStreamCfg(pstHandle->u32CamId, RKADK_STREAM_TYPE_LIVE);
  if (!pstLiveCfg) {
    RKADK_LOGE("RKADK_PARAM_GetStreamCfg failed");
    return -1;
  }

  RKADK_PARAM_AUDIO_CFG_S *pstAudioCfg = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioCfg) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return ret;
  }

  bDisableAudio =
      pstAudioCfg->codec_type == RKADK_CODEC_TYPE_MP3 ? true : false;

  RKADK_RTMP_VideoSetChn(pstLiveCfg, &stViChn, &stVencChn, &stRgaChn);

  if (bDisableAudio)
    RKADK_RTMP_AudioSetChn(&stAiChn, &stAencChn);

  // UnBind VENC to MUXER:VIDEO
  stMuxerChn.enModId = RK_ID_MUXER;
  stMuxerChn.enChnType = MUXER_CHN_TYPE_VIDEO;
  stMuxerChn.s32ChnId = pstHandle->u32MuxerChnId;
  ret = RK_MPI_MUXER_UnBind(&stVencChn, &stMuxerChn);
  if (ret) {
    RKADK_LOGE("UnBind VENC[%d] and MUXER[%d]:VIDEO failed[%d]",
               stVencChn.s32ChnId, stMuxerChn.s32ChnId, ret);
    return ret;
  }

  // UnBind AENC to MUXER:VIDEO
  if (bDisableAudio) {
    stMuxerChn.enChnType = MUXER_CHN_TYPE_AUDIO;
    ret = RK_MPI_MUXER_UnBind(&stAencChn, &stMuxerChn);
    if (ret) {
      RKADK_LOGE("UnBind AENC[%d] and MUXER[%d]:VIDEO failed[%d]",
                 stAencChn.s32ChnId, stMuxerChn.s32ChnId, ret);
      return ret;
    }
  }

  bUseRga = RKADK_RTMP_IsUseRga(pstHandle->u32CamId, pstLiveCfg);
  if (bUseRga) {
    // RGA UnBind VENC
    ret = RKADK_MPI_SYS_UnBind(&stRgaChn, &stVencChn);
    if (ret) {
      RKADK_LOGE("UnBind RGA[%d] to VENC[%d] failed[%d]", stRgaChn.s32ChnId,
                 stVencChn.s32ChnId, ret);
      return ret;
    }

    // VI UnBind RGA
    ret = RKADK_MPI_SYS_UnBind(&stViChn, &stRgaChn);
    if (ret) {
      RKADK_LOGE("UnBind VI[%d] to RGA[%d] failed[%d]", stViChn.s32ChnId,
                 stRgaChn.s32ChnId, ret);
      return ret;
    }
  } else {
    // UnBind VI and VENC
    ret = RKADK_MPI_SYS_UnBind(&stViChn, &stVencChn);
    if (ret) {
      RKADK_LOGE("UnBind VI[%d] and VENC[%d] failed[%d]", stViChn.s32ChnId,
                 stVencChn.s32ChnId, ret);
      return ret;
    }
  }

  // UnBind AI and AENC
  if (bDisableAudio) {
    ret = RKADK_MPI_SYS_UnBind(&stAiChn, &stAencChn);
    if (ret) {
      RKADK_LOGE("UnBind AI[%d] and AENC[%d] failed[%d]", stAiChn.s32ChnId,
                 stAencChn.s32ChnId, ret);
      return ret;
    }
  }

  // Destroy MUXER
  ret = RK_MPI_MUXER_DisableChn(pstHandle->u32MuxerChnId);
  if (ret) {
    RKADK_LOGE("Destroy MUXER[%d] failed[%d]", pstHandle->u32MuxerChnId, ret);
    return ret;
  }

  // Disable Video
  ret = RKADK_RTMP_DisableVideo(pstHandle->u32CamId, stViChn, stVencChn,
                                stRgaChn, bUseRga);
  if (ret) {
    RKADK_LOGE("RKADK_RTMP_DisableVideo failed(%d)", ret);
    return ret;
  }

  // Disable Audio
  if (bDisableAudio) {
    ret = RKADK_RTMP_DisableAudio(stAiChn, stAencChn, pstAudioCfg);
    if (ret) {
      RKADK_LOGE("RKADK_RTMP_DisableAudio failed(%d)", ret);
      return ret;
    }
  }

  RKADK_LOGI("Rtmp[%d] DeInit End...", pstHandle->u32CamId);
  free(pHandle);
  return 0;
}
