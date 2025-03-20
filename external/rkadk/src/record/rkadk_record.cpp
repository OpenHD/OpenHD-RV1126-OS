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

#include "rkadk_record.h"
#include "rkadk_log.h"
#include "rkadk_media_comm.h"
#include "rkadk_param.h"
#include "rkadk_recorder_pro.h"
#include "rkadk_track_source.h"
#include "rkmedia_api.h"
#include <deque>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static RKADK_REC_REQUEST_FILE_NAMES_FN
    g_pfnRequestFileNames[RKADK_MAX_SENSOR_CNT] = {NULL};
static std::deque<char *>
    g_fileNameDeque[RKADK_REC_STREAM_MAX_CNT * RKADK_MAX_SENSOR_CNT];
static pthread_mutex_t g_fileNameMutexLock = PTHREAD_MUTEX_INITIALIZER;

static int GetRecordFileName(RKADK_VOID *pHandle, RKADK_CHAR *pcFileName,
                             RKADK_U32 muxerId) {
  int index, ret;
  RKADK_RECORDER_HANDLE_S *pstRecorder;

  RKADK_MUTEX_LOCK(g_fileNameMutexLock);

  if (muxerId >= RKADK_REC_STREAM_MAX_CNT * RKADK_MAX_SENSOR_CNT) {
    RKADK_LOGE("Incorrect file index: %d", muxerId);
    RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
    return -1;
  }

  pstRecorder = (RKADK_RECORDER_HANDLE_S *)pHandle;
  if (!pstRecorder) {
    RKADK_LOGE("pstRecorder is null");
    RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
    return -1;
  }

  if (!g_pfnRequestFileNames[pstRecorder->s32CamId]) {
    RKADK_LOGE("Not Registered request name callback");
    RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
    return -1;
  }

  if (g_fileNameDeque[muxerId].empty()) {
    ARRAY_FILE_NAME fileName;
    fileName = (ARRAY_FILE_NAME)malloc(pstRecorder->u32StreamCnt *
                                       RKADK_MAX_FILE_PATH_LEN);
    ret = g_pfnRequestFileNames[pstRecorder->s32CamId](
        pHandle, pstRecorder->u32StreamCnt, fileName);
    if (ret) {
      RKADK_LOGE("get file name failed(%d)", ret);
      RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
      return -1;
    }

    char *newName[pstRecorder->u32StreamCnt];
    for (int i = 0; i < (int)pstRecorder->u32StreamCnt; i++) {
      newName[i] = strdup(fileName[i]);
      index = i + (RKADK_REC_STREAM_MAX_CNT * pstRecorder->s32CamId);
      g_fileNameDeque[index].push_back(newName[i]);
    }
    free(fileName);
  }

  // Always use the newest file name, because there's
  // not chance that one recording stream is far behind
  // than others. If so, yell a warning and shall be fixed.
  // But, A special case still happens if:
  //   1.  Request stop stream A and B async
  //   2.  stream A stopped
  //   3.  stream B is still writing and get a new file name for A and B
  //   4.  stream B stopped
  //   5.  stream A has one more file name now!
  if (g_fileNameDeque[muxerId].size() > 1)
    RKADK_LOGE("FIXME: A record stream progress is far behind that others\n");
  auto front = g_fileNameDeque[muxerId].back();
  memcpy(pcFileName, front, strlen(front));
  g_fileNameDeque[muxerId].clear();
  free(front);

  RKADK_MUTEX_UNLOCK(g_fileNameMutexLock);
  return 0;
}

static void RKADK_RECORD_SetVideoChn(int index,
                                     RKADK_PARAM_REC_CFG_S *pstRecCfg,
                                     MPP_CHN_S *pstViChn, MPP_CHN_S *pstVencChn,
                                     MPP_CHN_S *pstRgaChn) {
  pstViChn->enModId = RK_ID_VI;
  pstViChn->s32DevId = 0;
  pstViChn->s32ChnId = pstRecCfg->vi_attr[index].u32ViChn;

  pstRgaChn->enModId = RK_ID_RGA;
  pstRgaChn->s32DevId = 0;
  pstRgaChn->s32ChnId = pstRecCfg->attribute[index].rga_chn;

  pstVencChn->enModId = RK_ID_VENC;
  pstVencChn->s32DevId = 0;
  pstVencChn->s32ChnId = pstRecCfg->attribute[index].venc_chn;
}

static void RKADK_RECORD_SetAudioChn(MPP_CHN_S *pstAiChn,
                                     MPP_CHN_S *pstAencChn) {
  pstAiChn->enModId = RK_ID_AI;
  pstAiChn->s32DevId = 0;
  pstAiChn->s32ChnId = RECORD_AI_CHN;

  pstAencChn->enModId = RK_ID_AENC;
  pstAencChn->s32DevId = 0;
  pstAencChn->s32ChnId = RECORD_AENC_CHN;
}

static int RKADK_RECORD_SetVideoAttr(int index, RKADK_S32 s32CamId,
                                     RKADK_PARAM_REC_CFG_S *pstRecCfg,
                                     VENC_CHN_ATTR_S *pstVencAttr) {
  int ret;
  RKADK_U32 u32Gop;
  RKADK_U32 u32DstFrameRateNum = 0;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;
  RKADK_U32 bitrate;

  RKADK_CHECK_POINTER(pstVencAttr, RKADK_FAILURE);

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(s32CamId);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  memset(pstVencAttr, 0, sizeof(VENC_CHN_ATTR_S));

  if (pstRecCfg->record_type == RKADK_REC_TYPE_LAPSE) {
    bitrate = pstRecCfg->attribute[index].bitrate / pstRecCfg->lapse_multiple;
    u32DstFrameRateNum = pstSensorCfg->framerate / pstRecCfg->lapse_multiple;
    if (u32DstFrameRateNum < 1)
      u32DstFrameRateNum = 1;
    else if (u32DstFrameRateNum > pstSensorCfg->framerate)
      u32DstFrameRateNum = pstSensorCfg->framerate;

    u32Gop = pstRecCfg->attribute[index].gop;
  } else {
    bitrate = pstRecCfg->attribute[index].bitrate;
    u32DstFrameRateNum = pstSensorCfg->framerate;
    u32Gop = pstRecCfg->attribute[index].gop;
  }

  pstVencAttr->stRcAttr.enRcMode =
      RKADK_PARAM_GetRcMode(pstRecCfg->attribute[index].rc_mode,
                            pstRecCfg->attribute[index].codec_type);

  ret = RKADK_MEDIA_SetRcAttr(&pstVencAttr->stRcAttr, u32Gop, bitrate,
                              pstSensorCfg->framerate, u32DstFrameRateNum);
  if (ret) {
    RKADK_LOGE("RKADK_MEDIA_SetRcAttr failed");
    return -1;
  }

  if (pstRecCfg->attribute[index].codec_type == RKADK_CODEC_TYPE_H265)
    pstVencAttr->stVencAttr.stAttrH265e.bScaleList =
        (RK_BOOL)pstRecCfg->attribute[index].venc_param.scaling_list;

  pstVencAttr->stVencAttr.enType =
      RKADK_MEDIA_GetRkCodecType(pstRecCfg->attribute[index].codec_type);
  pstVencAttr->stVencAttr.imageType =
      pstRecCfg->vi_attr[index].stChnAttr.enPixFmt;
  pstVencAttr->stVencAttr.u32PicWidth = pstRecCfg->attribute[index].width;
  pstVencAttr->stVencAttr.u32PicHeight = pstRecCfg->attribute[index].height;
  pstVencAttr->stVencAttr.u32VirWidth = pstRecCfg->attribute[index].width;
  pstVencAttr->stVencAttr.u32VirHeight = pstRecCfg->attribute[index].height;
  pstVencAttr->stVencAttr.u32Profile = pstRecCfg->attribute[index].profile;
  pstVencAttr->stVencAttr.bFullRange =
      (RK_BOOL)pstRecCfg->attribute[index].venc_param.full_range;
  return 0;
}

static bool RKADK_RECORD_IsUseRga(RKADK_U32 u32CamId, int index,
                                  RKADK_PARAM_REC_CFG_S *pstRecCfg) {
  bool bUseRga = false;
  RKADK_U32 u32SrcWidth = pstRecCfg->vi_attr[index].stChnAttr.u32Width;
  RKADK_U32 u32SrcHeight = pstRecCfg->vi_attr[index].stChnAttr.u32Height;
  RKADK_U32 u32DstWidth = pstRecCfg->attribute[index].width;
  RKADK_U32 u32DstHeight = pstRecCfg->attribute[index].height;

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
    // Froce enable the rga
    // if (pstSensorCfg->flip || pstSensorCfg->mirror)
    bUseRga = true;
  }

  return bUseRga;
}

static int RKADK_RECORD_CreateVideoChn(RKADK_S32 s32CamID) {
  int ret;
  bool bUseRga = false;
  VENC_CHN_ATTR_S stVencChnAttr;
  VENC_RC_PARAM_S stVencRcParam;
  RGA_ATTR_S stRgaAttr;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(s32CamID);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    ret = RKADK_RECORD_SetVideoAttr(i, s32CamID, pstRecCfg, &stVencChnAttr);
    if (ret) {
      RKADK_LOGE("RKADK_RECORD_SetVideoAttr(%d) failed", i);
      return ret;
    }

    // Create VI
    ret = RKADK_MPI_VI_Init(s32CamID, pstRecCfg->vi_attr[i].u32ViChn,
                            &(pstRecCfg->vi_attr[i].stChnAttr));
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VI_Init faile, ret = %d", ret);
      return ret;
    }

    // Create RGA
    bUseRga = RKADK_RECORD_IsUseRga(s32CamID, i, pstRecCfg);
    if (bUseRga) {
      memset(&stRgaAttr, 0, sizeof(stRgaAttr));

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
      stRgaAttr.stImgIn.imgType = pstRecCfg->vi_attr[i].stChnAttr.enPixFmt;
      stRgaAttr.stImgIn.u32Width = pstRecCfg->vi_attr[i].stChnAttr.u32Width;
      stRgaAttr.stImgIn.u32Height = pstRecCfg->vi_attr[i].stChnAttr.u32Height;
      stRgaAttr.stImgIn.u32HorStride = pstRecCfg->vi_attr[i].stChnAttr.u32Width;
      stRgaAttr.stImgIn.u32VirStride =
          pstRecCfg->vi_attr[i].stChnAttr.u32Height;
      stRgaAttr.stImgOut.imgType = stRgaAttr.stImgIn.imgType;
      stRgaAttr.stImgOut.u32Width = pstRecCfg->attribute[i].width;
      stRgaAttr.stImgOut.u32Height = pstRecCfg->attribute[i].height;
      stRgaAttr.stImgOut.u32HorStride = pstRecCfg->attribute[i].width;
      stRgaAttr.stImgOut.u32VirStride = pstRecCfg->attribute[i].height;
      ret = RKADK_MPI_RGA_Init(pstRecCfg->attribute[i].rga_chn, &stRgaAttr);
      if (ret) {
        RKADK_LOGE("Init Rga[%d] falied[%d]", pstRecCfg->attribute[i].rga_chn,
                   ret);
        RKADK_MPI_VI_DeInit(s32CamID, pstRecCfg->vi_attr[i].u32ViChn);
        return ret;
      }
    }

    // Create VENC
    ret = RKADK_MPI_VENC_Init(pstRecCfg->attribute[i].venc_chn, &stVencChnAttr);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VENC_Init failed(%d)", ret);

      if (bUseRga)
        RKADK_MPI_RGA_DeInit(pstRecCfg->attribute[i].rga_chn);

      RKADK_MPI_VI_DeInit(s32CamID, pstRecCfg->vi_attr[i].u32ViChn);
      return ret;
    }

    ret = RKADK_PARAM_GetRcParam(pstRecCfg->attribute[i], &stVencRcParam);
    if (!ret) {
      ret = RK_MPI_VENC_SetRcParam(pstRecCfg->attribute[i].venc_chn,
                                   &stVencRcParam);
      if (ret)
        RKADK_LOGW("RK_MPI_VENC_SetRcParam failed(%d), use default cfg", ret);
    }
  }

  return 0;
}

static int RKADK_RECORD_DestoryVideoChn(RKADK_S32 s32CamID) {
  int ret;
  bool bUseRga = false;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    // Destroy VENC
    ret = RKADK_MPI_VENC_DeInit(pstRecCfg->attribute[i].venc_chn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VENC_DeInit failed(%d)", ret);
      return ret;
    }

    bUseRga = RKADK_RECORD_IsUseRga(s32CamID, i, pstRecCfg);
    if (bUseRga) {
      ret = RKADK_MPI_RGA_DeInit(pstRecCfg->attribute[i].rga_chn);
      if (ret) {
        RKADK_LOGE("RKADK_MPI_RGA_DeInit failed[%d]", ret);
        return ret;
      }
    }

    // Destroy VI
    ret = RKADK_MPI_VI_DeInit(s32CamID, pstRecCfg->vi_attr[i].u32ViChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_VI_DeInit failed, ret = %d", ret);
      return ret;
    }
  }

  return 0;
}

static int RKADK_RECORD_CreateAudioChn() {
  int ret;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  // Create AI
  AI_CHN_ATTR_S stAiAttr;
  stAiAttr.pcAudioNode = pstAudioParam->audio_node;
  stAiAttr.enSampleFormat = pstAudioParam->sample_format;
  stAiAttr.u32NbSamples = pstAudioParam->samples_per_frame;
  stAiAttr.u32SampleRate = pstAudioParam->samplerate;
  stAiAttr.u32Channels = pstAudioParam->channels;
  stAiAttr.enAiLayout = pstAudioParam->ai_layout;
  ret = RKADK_MPI_AI_Init(RECORD_AI_CHN, &stAiAttr, pstAudioParam->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_Init faile(%d)", ret);
    return ret;
  }

  // Create AENC
  AENC_CHN_ATTR_S stAencAttr;
  stAencAttr.enCodecType =
      RKADK_MEDIA_GetRkCodecType(pstAudioParam->codec_type);
  stAencAttr.u32Bitrate = pstAudioParam->bitrate;
  stAencAttr.u32Quality = 1;

  switch (pstAudioParam->codec_type) {
  case RKADK_CODEC_TYPE_G711A:
    stAencAttr.stAencG711A.u32Channels = pstAudioParam->channels;
    stAencAttr.stAencG711A.u32NbSample = pstAudioParam->samples_per_frame;
    stAencAttr.stAencG711A.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_G711U:
    stAencAttr.stAencG711U.u32Channels = pstAudioParam->channels;
    stAencAttr.stAencG711U.u32NbSample = pstAudioParam->samples_per_frame;
    stAencAttr.stAencG711U.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_MP3:
    stAencAttr.stAencMP3.u32Channels = pstAudioParam->channels;
    stAencAttr.stAencMP3.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_G726:
    stAencAttr.stAencG726.u32Channels = pstAudioParam->channels;
    stAencAttr.stAencG726.u32SampleRate = pstAudioParam->samplerate;
    break;
  case RKADK_CODEC_TYPE_MP2:
    stAencAttr.stAencMP2.u32Channels = pstAudioParam->channels;
    stAencAttr.stAencMP2.u32SampleRate = pstAudioParam->samplerate;
    break;

  default:
    RKADK_LOGE("Nonsupport codec type(%d)", pstAudioParam->codec_type);
    return -1;
  }

  stAencAttr.stAencMP3.u32Channels = pstAudioParam->channels;
  stAencAttr.stAencMP3.u32SampleRate = pstAudioParam->samplerate;
  ret = RKADK_MPI_AENC_Init(RECORD_AENC_CHN, &stAencAttr);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AENC_Init failed(%d)", ret);
    RKADK_MPI_AI_DeInit(RECORD_AI_CHN, pstAudioParam->vqe_mode);
    return -1;
  }

  return 0;
}

static int RKADK_RECORD_DestoryAudioChn() {
  int ret;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  ret = RKADK_MPI_AENC_DeInit(RECORD_AENC_CHN);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AENC_DeInit failed(%d)", ret);
    return ret;
  }

  ret = RKADK_MPI_AI_DeInit(RECORD_AI_CHN, pstAudioParam->vqe_mode);
  if (ret) {
    RKADK_LOGE("RKADK_MPI_AI_DeInit failed(%d)", ret);
    return ret;
  }

  return 0;
}

static int RKADK_RECORD_BindChn(RKADK_S32 s32CamID,
                                RKADK_REC_TYPE_E enRecType) {
  int ret;
  bool bUseRga;
  MPP_CHN_S stSrcChn, stDestChn, stRgaChn;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  if (enRecType != RKADK_REC_TYPE_LAPSE && RKADK_REC_EnableAudio(s32CamID)) {
    // Bind AI to AENC
    RKADK_RECORD_SetAudioChn(&stSrcChn, &stDestChn);
    ret = RKADK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_Bind failed(%d)", ret);
      return ret;
    }
  }

  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    RKADK_RECORD_SetVideoChn(i, pstRecCfg, &stSrcChn, &stDestChn, &stRgaChn);

    bUseRga = RKADK_RECORD_IsUseRga(s32CamID, i, pstRecCfg);
    if (bUseRga) {
      // RGA Bind VENC
      ret = RKADK_MPI_SYS_Bind(&stRgaChn, &stDestChn);
      if (ret) {
        RKADK_LOGE("Bind RGA[%d] to VENC[%d] failed[%d]", stRgaChn.s32ChnId,
                   stDestChn.s32ChnId, ret);
        return ret;
      }

      // VI Bind RGA
      ret = RKADK_MPI_SYS_Bind(&stSrcChn, &stRgaChn);
      if (ret) {
        RKADK_LOGE("Bind VI[%d] to RGA[%d] failed[%d]", stSrcChn.s32ChnId,
                   stRgaChn.s32ChnId, ret);
        return ret;
      }
    } else {
      // Bind VI to VENC
      ret = RKADK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
      if (ret) {
        RKADK_LOGE("RKADK_MPI_SYS_Bind failed(%d)", ret);
        return ret;
      }
    }
  }

  return 0;
}

static int RKADK_RECORD_UnBindChn(RKADK_S32 s32CamID,
                                  RKADK_REC_TYPE_E enRecType) {
  int ret;
  bool bUseRga = false;
  MPP_CHN_S stSrcChn, stDestChn, stRgaChn;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  if (enRecType != RKADK_REC_TYPE_LAPSE && RKADK_REC_EnableAudio(s32CamID)) {
    // UnBind AI to AENC
    RKADK_RECORD_SetAudioChn(&stSrcChn, &stDestChn);
    ret = RKADK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if (ret) {
      RKADK_LOGE("RKADK_MPI_SYS_UnBind failed(%d)", ret);
      return ret;
    }
  }

  // UnBind VI to VENC
  for (int i = 0; i < (int)pstRecCfg->file_num; i++) {
    RKADK_RECORD_SetVideoChn(i, pstRecCfg, &stSrcChn, &stDestChn, &stRgaChn);

    bUseRga = RKADK_RECORD_IsUseRga(s32CamID, i, pstRecCfg);
    if (bUseRga) {
      // RGA UnBind VENC
      ret = RKADK_MPI_SYS_UnBind(&stRgaChn, &stDestChn);
      if (ret) {
        RKADK_LOGE("UnBind RGA[%d] to VENC[%d] failed[%d]", stRgaChn.s32ChnId,
                   stDestChn.s32ChnId, ret);
        return ret;
      }

      // VI UnBind RGA
      ret = RKADK_MPI_SYS_UnBind(&stSrcChn, &stRgaChn);
      if (ret) {
        RKADK_LOGE("UnBind VI[%d] to RGA[%d] failed[%d]", stSrcChn.s32ChnId,
                   stRgaChn.s32ChnId, ret);
        return ret;
      }
    } else {
      ret = RKADK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
      if (ret) {
        RKADK_LOGE("RKADK_MPI_SYS_UnBind failed(%d)", ret);
        return ret;
      }
    }
  }

  return 0;
}

static RKADK_S32 RKADK_RECORD_SetRecordAttr(RKADK_S32 s32CamID,
                                            RKADK_REC_TYPE_E enRecType,
                                            RKADK_REC_ATTR_S *pstRecAttr) {
  bool bEnableAudio = false;
  RKADK_U32 u32Integer = 0, u32Remainder = 0;
  RKADK_PARAM_AUDIO_CFG_S *pstAudioParam = NULL;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;
  RKADK_PARAM_SENSOR_CFG_S *pstSensorCfg = NULL;

  RKADK_CHECK_CAMERAID(s32CamID, RKADK_FAILURE);

  pstAudioParam = RKADK_PARAM_GetAudioCfg();
  if (!pstAudioParam) {
    RKADK_LOGE("RKADK_PARAM_GetAudioCfg failed");
    return -1;
  }

  pstRecCfg = RKADK_PARAM_GetRecCfg(s32CamID);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return -1;
  }

  bEnableAudio = RKADK_REC_EnableAudio(s32CamID);

  pstSensorCfg = RKADK_PARAM_GetSensorCfg(s32CamID);
  if (!pstSensorCfg) {
    RKADK_LOGE("RKADK_PARAM_GetSensorCfg failed");
    return -1;
  }

  memset(pstRecAttr, 0, sizeof(RKADK_REC_ATTR_S));

  if (pstRecCfg->record_type != enRecType)
    RKADK_LOGW("enRecType(%d) != param record_type(%d)", enRecType,
               pstRecCfg->record_type);

  u32Integer = pstRecCfg->attribute[0].gop / pstSensorCfg->framerate;
  u32Remainder = pstRecCfg->attribute[0].gop % pstSensorCfg->framerate;
  pstRecAttr->stPreRecordAttr.u32PreRecCacheTime =
      pstRecCfg->pre_record_time + u32Integer;
  if (u32Remainder)
    pstRecAttr->stPreRecordAttr.u32PreRecCacheTime += 1;

  pstRecAttr->enRecType = enRecType;
  pstRecAttr->stRecSplitAttr.enMode = MUXER_MODE_AUTOSPLIT;
  pstRecAttr->u32StreamCnt = pstRecCfg->file_num;
  pstRecAttr->stPreRecordAttr.u32PreRecTimeSec = pstRecCfg->pre_record_time;
  pstRecAttr->stPreRecordAttr.enPreRecordMode = pstRecCfg->pre_record_mode;

  MUXER_SPLIT_ATTR_S *stSplitAttr = &(pstRecAttr->stRecSplitAttr.stSplitAttr);
  stSplitAttr->enSplitType = MUXER_SPLIT_TYPE_TIME;
  stSplitAttr->enSplitNameType = MUXER_SPLIT_NAME_TYPE_CALLBACK;
  stSplitAttr->stNameCallBackAttr.pcbRequestFileNames = GetRecordFileName;

  for (int i = 0; i < (int)pstRecAttr->u32StreamCnt; i++) {
    pstRecAttr->astStreamAttr[i].enType = pstRecCfg->file_type;
    if (pstRecAttr->enRecType == RKADK_REC_TYPE_LAPSE) {
      pstRecAttr->astStreamAttr[i].u32TimeLenSec =
          pstRecCfg->record_time_cfg[i].lapse_interval;
      pstRecAttr->astStreamAttr[i].u32TrackCnt = 1; // only video track
    } else {
      pstRecAttr->astStreamAttr[i].u32TimeLenSec =
          pstRecCfg->record_time_cfg[i].record_time;
      pstRecAttr->astStreamAttr[i].u32TrackCnt = RKADK_REC_TRACK_MAX_CNT;
    }

    if (pstRecAttr->astStreamAttr[i].u32TrackCnt == RKADK_REC_TRACK_MAX_CNT)
      if (!bEnableAudio)
        pstRecAttr->astStreamAttr[i].u32TrackCnt = 1;

    // video track
    RKADK_TRACK_SOURCE_S *aHTrackSrcHandle =
        &(pstRecAttr->astStreamAttr[i].aHTrackSrcHandle[0]);
    aHTrackSrcHandle->enTrackType = RKADK_TRACK_SOURCE_TYPE_VIDEO;
    aHTrackSrcHandle->s32PrivateHandle = pstRecCfg->attribute[i].venc_chn;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.enCodecType =
        RKADK_MEDIA_GetRkCodecType(pstRecCfg->attribute[i].codec_type);
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.enImageType =
        pstRecCfg->vi_attr[i].stChnAttr.enPixFmt;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32FrameRate =
        pstSensorCfg->framerate;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u16Level = 41;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u16Profile =
        pstRecCfg->attribute[i].profile;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32BitRate =
        pstRecCfg->attribute[i].bitrate;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32Width =
        pstRecCfg->attribute[i].width;
    aHTrackSrcHandle->unTrackSourceAttr.stVideoInfo.u32Height =
        pstRecCfg->attribute[i].height;

    if (pstRecAttr->enRecType == RKADK_REC_TYPE_LAPSE || !bEnableAudio)
      continue;

    // audio track
    aHTrackSrcHandle = &(pstRecAttr->astStreamAttr[i].aHTrackSrcHandle[1]);
    aHTrackSrcHandle->enTrackType = RKADK_TRACK_SOURCE_TYPE_AUDIO;
    aHTrackSrcHandle->s32PrivateHandle = RECORD_AENC_CHN;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.enCodecType =
        RKADK_MEDIA_GetRkCodecType(pstAudioParam->codec_type);
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.enSampFmt =
        pstAudioParam->sample_format;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.u32ChnCnt =
        pstAudioParam->channels;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.u32SamplesPerFrame =
        pstAudioParam->samples_per_frame;
    aHTrackSrcHandle->unTrackSourceAttr.stAudioInfo.u32SampleRate =
        pstAudioParam->samplerate;
  }

  return 0;
}

RKADK_S32 RKADK_RECORD_Create(RKADK_RECORD_ATTR_S *pstRecAttr,
                              RKADK_MW_PTR *ppRecorder) {
  int ret;
  bool bEnableAudio = false;

  RKADK_CHECK_POINTER(pstRecAttr, RKADK_FAILURE);
  RKADK_CHECK_CAMERAID(pstRecAttr->s32CamID, RKADK_FAILURE);

  RKADK_LOGI("Create Record[%d, %d] Start...", pstRecAttr->s32CamID,
             pstRecAttr->enRecType);

  RK_MPI_SYS_Init();
  RKADK_PARAM_Init(NULL, NULL);

  if (RKADK_RECORD_CreateVideoChn(pstRecAttr->s32CamID))
    return -1;

  bEnableAudio = RKADK_REC_EnableAudio(pstRecAttr->s32CamID);
  if (pstRecAttr->enRecType != RKADK_REC_TYPE_LAPSE && bEnableAudio) {
    if (RKADK_RECORD_CreateAudioChn()) {
      RKADK_RECORD_DestoryVideoChn(pstRecAttr->s32CamID);
      return -1;
    }
  }

  g_pfnRequestFileNames[pstRecAttr->s32CamID] = pstRecAttr->pfnRequestFileNames;

  RKADK_REC_ATTR_S stRecAttr;
  ret = RKADK_RECORD_SetRecordAttr(pstRecAttr->s32CamID, pstRecAttr->enRecType,
                                   &stRecAttr);
  if (ret) {
    RKADK_LOGE("RKADK_RECORD_SetRecordAttr failed");
    goto failed;
  }

  if (RKADK_REC_Create(&stRecAttr, ppRecorder, pstRecAttr->s32CamID))
    goto failed;

  RKADK_LOGI("Create Record[%d, %d] End...", pstRecAttr->s32CamID,
             pstRecAttr->enRecType);
  return 0;

failed:
  RKADK_LOGE("Create Record[%d, %d] failed", pstRecAttr->s32CamID,
             pstRecAttr->enRecType);
  RKADK_RECORD_DestoryVideoChn(pstRecAttr->s32CamID);

  if (pstRecAttr->enRecType != RKADK_REC_TYPE_LAPSE && bEnableAudio)
    RKADK_RECORD_DestoryAudioChn();

  return -1;
}

RKADK_S32 RKADK_RECORD_Destroy(RKADK_MW_PTR pRecorder) {
  RKADK_S32 ret, index;
  RKADK_S32 s32CamId;
  RKADK_REC_TYPE_E enRecType;
  RKADK_RECORDER_HANDLE_S *stRecorder = NULL;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);
  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  if (!stRecorder) {
    RKADK_LOGE("stRecorder is null");
    return -1;
  }

  RKADK_LOGI("Destory Record[%d, %d] Start...", stRecorder->s32CamId,
             stRecorder->enRecType);

  s32CamId = stRecorder->s32CamId;
  enRecType = stRecorder->enRecType;
  RKADK_CHECK_CAMERAID(s32CamId, RKADK_FAILURE);

  for (int i = 0; i < (int)stRecorder->u32StreamCnt; i++) {
    index = i + (RKADK_REC_STREAM_MAX_CNT * s32CamId);
    while (!g_fileNameDeque[index].empty()) {
      RKADK_LOGI("clear file name deque[%d]", index);
      auto front = g_fileNameDeque[index].front();
      g_fileNameDeque[index].pop_front();
      free(front);
    }
    g_fileNameDeque[index].clear();
  }

  ret = RKADK_REC_Destroy(pRecorder);
  if (ret) {
    RKADK_LOGE("RK_REC_Destroy failed, ret = %d", ret);
  }

  ret = RKADK_RECORD_UnBindChn(s32CamId, enRecType);
  if (ret) {
    RKADK_LOGE("RKADK_RECORD_UnBindChn failed, ret = %d", ret);
  }

  ret = RKADK_RECORD_DestoryVideoChn(s32CamId);
  if (ret) {
    RKADK_LOGE("RKADK_RECORD_DestoryVideoChn failed, ret = %d", ret);
  }

  if (enRecType != RKADK_REC_TYPE_LAPSE && RKADK_REC_EnableAudio(s32CamId)) {
    ret = RKADK_RECORD_DestoryAudioChn();
    if (ret) {
      RKADK_LOGE("RKADK_RECORD_DestoryAudioChn failed, ret = %d", ret);
    }
  }

  g_pfnRequestFileNames[s32CamId] = NULL;
  RKADK_LOGI("Destory Record[%d, %d] End...", s32CamId, enRecType);

  // Put the pRecorder at the very end, because rkmedia muxer_flow uses
  // FlushThread(with cache) to write data. If free pRecorder before
  // Destory MuxerFlow, then GetRecordFileName() may encounter a use-after-free
  // crash
  if (pRecorder) {
    free(pRecorder);
    pRecorder = NULL;
  }

  return 0;
}

RKADK_S32 RKADK_RECORD_Start(RKADK_MW_PTR pRecorder) {
  RKADK_S32 ret;
  RKADK_RECORDER_HANDLE_S *stRecorder;

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;

  if ((ret = RKADK_REC_Start(pRecorder)) != 0)
    return ret;

  // Once channels are bound, VI streaming and encoder starting right away
  // To avoid missing the first I-frame, RKADK_REC_Start() muxer before binding.
  if ((ret = RKADK_RECORD_BindChn(stRecorder->s32CamId, stRecorder->enRecType)) != 0) {
    RKADK_REC_Stop(pRecorder);
    return ret;
  }

  return 0;
}

RKADK_S32 RKADK_RECORD_Stop(RKADK_MW_PTR pRecorder) {
  return RKADK_REC_Stop(pRecorder);
}

RKADK_S32 RKADK_RECORD_SetFrameRate(RKADK_MW_PTR pRecorder,
                                    RKADK_RECORD_FPS_ATTR_S stFpsAttr) {
  return RKADK_REC_SetFrameRate(pRecorder, stFpsAttr);
}

RKADK_S32
RKADK_RECORD_ManualSplit(RKADK_MW_PTR pRecorder,
                         RKADK_REC_MANUAL_SPLIT_ATTR_S *pstSplitAttr) {
  return RKADK_REC_ManualSplit(pRecorder, pstSplitAttr);
}

RKADK_S32 RKADK_RECORD_RegisterEventCallback(
    RKADK_MW_PTR pRecorder, RKADK_REC_EVENT_CALLBACK_FN pfnEventCallback) {
  return RKADK_REC_RegisterEventCallback(pRecorder, pfnEventCallback);
}

RKADK_S32 RKADK_RECORD_GetAencChn() { return RECORD_AENC_CHN; }

RKADK_S32 RKADK_RECORD_ToggleMirror(RKADK_MW_PTR pRecorder,
                                    RKADK_STREAM_TYPE_E enStrmType,
                                    int mirror) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;
  RGA_ATTR_S stRgaAttr;
  int s32RgaChn, ret, i;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  pstRecCfg = RKADK_PARAM_GetRecCfg(stRecorder->s32CamId);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return RKADK_FAILURE;
  }

  for (i = 0; i < (int)pstRecCfg->file_num; i++) {
    if (!RKADK_RECORD_IsUseRga(stRecorder->s32CamId, i, pstRecCfg)) {
      RKADK_LOGW("Rga is not enable\n");
      return RKADK_FAILURE;
    }

    s32RgaChn = RKADK_PARAM_GetRgaChnId(stRecorder->s32CamId, enStrmType);
    ret = RK_MPI_RGA_GetChnAttr(s32RgaChn, &stRgaAttr);
    if (ret) {
      RKADK_LOGE("mirror: get rga channel [%d] attr error, ret = %d\n",
                 s32RgaChn, ret);
      return RKADK_FAILURE;
    }

    if (mirror)
      stRgaAttr.enFlip = (RGA_FLIP_E)(stRgaAttr.enFlip | RGA_FLIP_H);
    else
      stRgaAttr.enFlip = (RGA_FLIP_E)(stRgaAttr.enFlip & (~RGA_FLIP_H));

    ret = RK_MPI_RGA_SetChnAttr(s32RgaChn, &stRgaAttr);
  }

  return ret;
}

RKADK_S32 RKADK_RECORD_ToggleFlip(RKADK_MW_PTR pRecorder,
                                  RKADK_STREAM_TYPE_E enStrmType,
                                  int flip) {
  RKADK_RECORDER_HANDLE_S *stRecorder;
  RKADK_PARAM_REC_CFG_S *pstRecCfg = NULL;
  RGA_ATTR_S stRgaAttr;
  int s32RgaChn, ret, i;

  RKADK_CHECK_POINTER(pRecorder, RKADK_FAILURE);

  stRecorder = (RKADK_RECORDER_HANDLE_S *)pRecorder;
  pstRecCfg = RKADK_PARAM_GetRecCfg(stRecorder->s32CamId);
  if (!pstRecCfg) {
    RKADK_LOGE("RKADK_PARAM_GetRecCfg failed");
    return RKADK_FAILURE;
  }

  for (i = 0; i < (int)pstRecCfg->file_num; i++) {
    if (!RKADK_RECORD_IsUseRga(stRecorder->s32CamId, i, pstRecCfg)) {
      RKADK_LOGW("Rga is not enable\n");
      continue;
    }

    s32RgaChn = RKADK_PARAM_GetRgaChnId(stRecorder->s32CamId, enStrmType);
    ret = RK_MPI_RGA_GetChnAttr(s32RgaChn, &stRgaAttr);
    if (ret) {
      RKADK_LOGE("flip: get rga channel [%d] attr error, ret = %d\n",
                 s32RgaChn, ret);
      return RKADK_FAILURE;
    }

    if (flip)
      stRgaAttr.enFlip = (RGA_FLIP_E)(stRgaAttr.enFlip | RGA_FLIP_V);
    else
      stRgaAttr.enFlip = (RGA_FLIP_E)(stRgaAttr.enFlip & (~RGA_FLIP_V));

    ret = RK_MPI_RGA_SetChnAttr(s32RgaChn, &stRgaAttr);
  }

  return ret;
}
