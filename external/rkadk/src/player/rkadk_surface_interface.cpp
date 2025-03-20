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

#ifdef ROCKIT

#include "rkadk_surface_interface.h"
#include "RTMediaBuffer.h"
#include "RTMediaData.h"
#include "rk_common.h"
#include "rk_mpi_vo.h"
#include "rkadk_common.h"
#include "rt_mem.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <unistd.h>

#define VOP_LAYER_CLUSTER_0 0
#define VOP_LAYER_CLUSTER_1 1

static RKADK_S32 RKADK_VO_GetPictureData(VO_FRAME_INFO_S *pstFrameInfo,
                                         RTMediaBuffer *buffer) {
  RKADK_CHECK_POINTER(pstFrameInfo, RKADK_FAILURE);
  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);

  memcpy(pstFrameInfo->pData, buffer->getData(), buffer->getLength());
  return RKADK_SUCCESS;
}

static RKADK_S32 RKADK_VO_CreateGFXData(RKADK_U32 u32Width, RKADK_U32 u32Height,
                                        RKADK_U32 foramt, RKADK_U32 value,
                                        RKADK_VOID **pMblk,
                                        RTMediaBuffer *buffer) {
  VO_FRAME_INFO_S stFrameInfo;
  RKADK_U32 u32BuffSize;

  RKADK_CHECK_POINTER(pMblk, RKADK_FAILURE);
  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);

  rt_memset(&stFrameInfo, 0, sizeof(VO_FRAME_INFO_S));

  u32BuffSize =
      RK_MPI_VO_CreateGraphicsFrameBuffer(u32Width, u32Height, foramt, pMblk);
  if (u32BuffSize == 0) {
    RKADK_LOGD("can not create gfx buffer");
    return RKADK_FAILURE;
  }

  RK_MPI_VO_GetFrameInfo(*pMblk, &stFrameInfo);
  RKADK_VO_GetPictureData(&stFrameInfo, buffer);

  return RKADK_SUCCESS;
}

static RKADK_S32
RKADK_VO_EnableLayer(VO_LAYER voLayer,
                     const VO_VIDEO_LAYER_ATTR_S *pstLayerAttr) {
  RKADK_S32 s32Ret = RKADK_SUCCESS;

  RKADK_CHECK_POINTER(pstLayerAttr, RKADK_FAILURE);

  s32Ret = RK_MPI_VO_SetLayerSpliceMode(voLayer, VO_SPLICE_MODE_RGA);
  if (s32Ret) {
    RKADK_LOGD("RK_MPI_VO_SetLayerSpliceMode failed[%d]", s32Ret);
    return s32Ret;
  }

  s32Ret = RK_MPI_VO_SetLayerAttr(voLayer, pstLayerAttr);
  if (s32Ret) {
    RKADK_LOGD("RK_MPI_VO_SetLayerAttr failed[%d]", s32Ret);
    return s32Ret;
  }

  s32Ret = RK_MPI_VO_EnableLayer(voLayer);
  if (s32Ret) {
    RKADK_LOGD("RK_MPI_VO_EnableLayer failed[%d]", s32Ret);
    return s32Ret;
  }

  return s32Ret;
}

static RKADK_S32 RKADK_VO_Enable(VO_DEV voDev, VO_PUB_ATTR_S *pstPubAttr) {
  RKADK_S32 s32Ret = RKADK_SUCCESS;

  RKADK_CHECK_POINTER(pstPubAttr, RKADK_FAILURE);

  s32Ret = RK_MPI_VO_SetPubAttr(voDev, pstPubAttr);
  if (s32Ret) {
    RKADK_LOGD("Set public attribute failed[%d]", s32Ret);
    return s32Ret;
  }

  s32Ret = RK_MPI_VO_Enable(voDev);
  if (s32Ret) {
    RKADK_LOGD("VO enable failed[%d]", s32Ret);
    return s32Ret;
  }

  return s32Ret;
}

static PIXEL_FORMAT_E RKADK_FmtToRtfmt(RKADK_PLAYER_VO_FORMAT_E format) {
  PIXEL_FORMAT_E rtfmt;

  switch (format) {
  case VO_FORMAT_ARGB8888:
    rtfmt = RK_FMT_BGRA8888;
    break;
  case VO_FORMAT_ABGR8888:
    rtfmt = RK_FMT_RGBA8888;
    break;
  case VO_FORMAT_RGB888:
    rtfmt = RK_FMT_BGR888;
    break;
  case VO_FORMAT_BGR888:
    rtfmt = RK_FMT_RGB888;
    break;
  case VO_FORMAT_ARGB1555:
    rtfmt = RK_FMT_BGRA5551;
    break;
  case VO_FORMAT_ABGR1555:
    rtfmt = RK_FMT_RGBA5551;
    break;
  case VO_FORMAT_NV12:
    rtfmt = RK_FMT_YUV420SP;
    break;
  case VO_FORMAT_NV21:
    rtfmt = RK_FMT_YUV420SP_VU;
    break;
  default:
    RKADK_LOGW("invalid format: %d", format);
    rtfmt = RK_FMT_BUTT;
  }

  return rtfmt;
}

static RKADK_S32 RKADK_VO_SetRtSyncInfo(VO_SYNC_INFO_S *pstSyncInfo,
                                        VIDEO_FRAMEINFO_S stFrmInfo) {
  RKADK_CHECK_POINTER(pstSyncInfo, RKADK_FAILURE);

  pstSyncInfo->bIdv = (RK_BOOL)stFrmInfo.stSyncInfo.bIdv;
  pstSyncInfo->bIhs = (RK_BOOL)stFrmInfo.stSyncInfo.bIhs;
  pstSyncInfo->bIvs = (RK_BOOL)stFrmInfo.stSyncInfo.bIvs;
  pstSyncInfo->bSynm = (RK_BOOL)stFrmInfo.stSyncInfo.bSynm;
  pstSyncInfo->bIop = (RK_BOOL)stFrmInfo.stSyncInfo.bIop;
  pstSyncInfo->u16FrameRate = (stFrmInfo.stSyncInfo.u16FrameRate > 0)
                                  ? stFrmInfo.stSyncInfo.u16FrameRate
                                  : 60;
  pstSyncInfo->u16PixClock = (stFrmInfo.stSyncInfo.u16PixClock > 0)
                                 ? stFrmInfo.stSyncInfo.u16PixClock
                                 : 65000;
  pstSyncInfo->u16Hact =
      (stFrmInfo.stSyncInfo.u16Hact > 0) ? stFrmInfo.stSyncInfo.u16Hact : 1200;
  pstSyncInfo->u16Hbb =
      (stFrmInfo.stSyncInfo.u16Hbb > 0) ? stFrmInfo.stSyncInfo.u16Hbb : 24;
  pstSyncInfo->u16Hfb =
      (stFrmInfo.stSyncInfo.u16Hfb > 0) ? stFrmInfo.stSyncInfo.u16Hfb : 240;
  pstSyncInfo->u16Hpw =
      (stFrmInfo.stSyncInfo.u16Hpw > 0) ? stFrmInfo.stSyncInfo.u16Hpw : 136;
  pstSyncInfo->u16Hmid =
      (stFrmInfo.stSyncInfo.u16Hmid > 0) ? stFrmInfo.stSyncInfo.u16Hmid : 0;
  pstSyncInfo->u16Vact =
      (stFrmInfo.stSyncInfo.u16Vact > 0) ? stFrmInfo.stSyncInfo.u16Vact : 1200;
  pstSyncInfo->u16Vbb =
      (stFrmInfo.stSyncInfo.u16Vbb > 0) ? stFrmInfo.stSyncInfo.u16Vbb : 200;
  pstSyncInfo->u16Vfb =
      (stFrmInfo.stSyncInfo.u16Vfb > 0) ? stFrmInfo.stSyncInfo.u16Vfb : 194;
  pstSyncInfo->u16Vpw =
      (stFrmInfo.stSyncInfo.u16Vpw > 0) ? stFrmInfo.stSyncInfo.u16Vpw : 6;

  return 0;
}

static RKADK_S32 RKADK_VO_SetLayerRect(VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
                                       VO_DEV voDev,
                                       VIDEO_FRAMEINFO_S stFrmInfo) {
  int ret;
  VO_PUB_ATTR_S pstAttr;

  RKADK_CHECK_POINTER(pstLayerAttr, RKADK_FAILURE);

  rt_memset(&pstAttr, 0, sizeof(VO_PUB_ATTR_S));
  ret = RK_MPI_VO_GetPubAttr(voDev, &pstAttr);
  if (ret) {
    RKADK_LOGD("RK_MPI_VO_GetPubAttr failed[%d]", ret);
    return ret;
  }

  if (0 < stFrmInfo.u32FrmInfoS32x) {
    pstLayerAttr->stDispRect.s32X = stFrmInfo.u32FrmInfoS32x;
  } else {
    RKADK_LOGD("Layer stDispRect x uses default value[0]");
    pstLayerAttr->stDispRect.s32X = 0;
  }

  if (0 < stFrmInfo.u32FrmInfoS32y) {
    pstLayerAttr->stDispRect.s32Y = stFrmInfo.u32FrmInfoS32y;
  } else {
    RKADK_LOGD("Layer stDispRect y uses default value[0]");
    pstLayerAttr->stDispRect.s32Y = 0;
  }

  if (0 < stFrmInfo.u32DispWidth) {
    pstLayerAttr->stDispRect.u32Width = stFrmInfo.u32DispWidth;
  } else {
    pstLayerAttr->stDispRect.u32Width = pstAttr.stSyncInfo.u16Hact;
    RKADK_LOGD("Layer stDispRect w uses default value[%d]",
               pstLayerAttr->stDispRect.u32Width);
  }

  if (0 < stFrmInfo.u32DispHeight) {
    pstLayerAttr->stDispRect.u32Height = stFrmInfo.u32DispHeight;
  } else {
    pstLayerAttr->stDispRect.u32Height = pstAttr.stSyncInfo.u16Vact;
    RKADK_LOGD("Layer stDispRect h uses default value[%d]",
               pstLayerAttr->stDispRect.u32Height);
  }

  if (0 < stFrmInfo.u32ImgWidth) {
    pstLayerAttr->stImageSize.u32Width = stFrmInfo.u32ImgWidth;
  } else {
    pstLayerAttr->stImageSize.u32Width = pstAttr.stSyncInfo.u16Hact;
    RKADK_LOGD("Layer stImageSize w uses default value[%d]",
               pstLayerAttr->stImageSize.u32Width);
  }

  if (0 < stFrmInfo.u32ImgHeight) {
    pstLayerAttr->stImageSize.u32Height = stFrmInfo.u32ImgHeight;
  } else {
    pstLayerAttr->stImageSize.u32Height = pstAttr.stSyncInfo.u16Vact;
    RKADK_LOGD("Layer stImageSize h uses default value[%d]",
               pstLayerAttr->stImageSize.u32Height);
  }

  return 0;
}

static RKADK_S32 RKADK_VO_EnableChnn(VO_LAYER voLayer,
                                     VO_VIDEO_LAYER_ATTR_S *pstLayerAttr,
                                     VIDEO_FRAMEINFO_S stFrmInfo) {
  int ret;
  VO_CHN_ATTR_S stChnAttr;
  VO_CHN_PARAM_S stChnParam;
  VO_BORDER_S stBorder;

  rt_memset(&stChnAttr, 0, sizeof(VO_CHN_ATTR_S));
  rt_memset(&stChnParam, 0, sizeof(VO_CHN_PARAM_S));
  rt_memset(&stBorder, 0, sizeof(VO_BORDER_S));

  if (!stFrmInfo.stVoAttr.stChnRect.u32Width ||
      !stFrmInfo.stVoAttr.stChnRect.u32Height) {
    RKADK_LOGD("VO channel rectangle uses layer stDispRect");
    stChnAttr.stRect.s32X = pstLayerAttr->stDispRect.s32X;
    stChnAttr.stRect.s32Y = pstLayerAttr->stDispRect.s32Y;
    stChnAttr.stRect.u32Width = pstLayerAttr->stDispRect.u32Width;
    stChnAttr.stRect.u32Height = pstLayerAttr->stDispRect.u32Height;
  } else {
    stChnAttr.stRect.s32X = stFrmInfo.stVoAttr.stChnRect.u32X;
    stChnAttr.stRect.s32Y = stFrmInfo.stVoAttr.stChnRect.u32Y;
    stChnAttr.stRect.u32Width = stFrmInfo.stVoAttr.stChnRect.u32Width;
    stChnAttr.stRect.u32Height = stFrmInfo.stVoAttr.stChnRect.u32Height;
  }

  stChnAttr.bMirror = (RK_BOOL)stFrmInfo.stVoAttr.bMirror;
  stChnAttr.bFlip = (RK_BOOL)stFrmInfo.stVoAttr.bFlip;
  switch (stFrmInfo.stVoAttr.u32Rotation) {
  case 90:
    stChnAttr.enRotation = ROTATION_90;
    break;
  case 180:
    stChnAttr.enRotation = ROTATION_180;
    break;
  case 270:
    stChnAttr.enRotation = ROTATION_270;
    break;
  default:
    stChnAttr.enRotation = ROTATION_0;
    break;
  }

  // set priority
  stChnAttr.u32Priority = 1;
  ret = RT_MPI_VO_SetChnAttr(voLayer, stFrmInfo.u32ChnnNum, &stChnAttr);
  if (ret) {
    RKADK_LOGD("RT_MPI_VO_SetChnAttr failed(%d)", ret);
    return RK_FAILURE;
  }

  // set video aspect ratio
  if (stFrmInfo.u32EnMode == CHNN_ASPECT_RATIO_MANUAL) {
    stChnParam.stAspectRatio.enMode = ASPECT_RATIO_MANUAL;
    stChnParam.stAspectRatio.stVideoRect.s32X = pstLayerAttr->stDispRect.s32X;
    stChnParam.stAspectRatio.stVideoRect.s32Y = pstLayerAttr->stDispRect.s32Y;
    stChnParam.stAspectRatio.stVideoRect.u32Width =
        pstLayerAttr->stDispRect.u32Width / 2;
    stChnParam.stAspectRatio.stVideoRect.u32Height =
        pstLayerAttr->stDispRect.u32Height / 2;
    RK_MPI_VO_SetChnParam(voLayer, stFrmInfo.u32ChnnNum, &stChnParam);
  } else if (stFrmInfo.u32EnMode == CHNN_ASPECT_RATIO_AUTO) {
    stChnParam.stAspectRatio.enMode = ASPECT_RATIO_AUTO;
    stChnParam.stAspectRatio.stVideoRect.s32X = pstLayerAttr->stDispRect.s32X;
    stChnParam.stAspectRatio.stVideoRect.s32Y = pstLayerAttr->stDispRect.s32Y;
    stChnParam.stAspectRatio.stVideoRect.u32Width =
        pstLayerAttr->stDispRect.u32Width;
    stChnParam.stAspectRatio.stVideoRect.u32Height =
        pstLayerAttr->stDispRect.u32Height;
    RK_MPI_VO_SetChnParam(voLayer, stFrmInfo.u32ChnnNum, &stChnParam);
  }

  stBorder.stBorder.u32Color = stFrmInfo.u32BorderColor;
  stBorder.stBorder.u32TopWidth = stFrmInfo.u32BorderTopWidth;
  stBorder.stBorder.u32BottomWidth = stFrmInfo.u32BorderTopWidth;
  stBorder.stBorder.u32LeftWidth = stFrmInfo.u32BorderLeftWidth;
  stBorder.stBorder.u32RightWidth = stFrmInfo.u32BorderRightWidth;
  stBorder.bBorderEn = RK_TRUE;
  ret = RK_MPI_VO_SetChnBorder(voLayer, stFrmInfo.u32ChnnNum, &stBorder);
  if (ret) {
    RKADK_LOGD("RK_MPI_VO_SetChnBorder failed(%d)", ret);
    return RK_FAILURE;
  }

  ret = RK_MPI_VO_EnableChn(voLayer, stFrmInfo.u32ChnnNum);
  if (ret) {
    RKADK_LOGD("RK_MPI_VO_EnableChn failed(%d)", ret);
    return RK_FAILURE;
  }

  return ret;
}

static VO_INTF_SYNC_E RKADK_VO_GetIntfSync(RKADK_VO_INTF_SYNC_E enIntfSync) {
  VO_INTF_SYNC_E enVoIntfSync = VO_OUTPUT_BUTT;

  switch (enIntfSync) {
  case RKADK_VO_OUTPUT_PAL:
    enVoIntfSync = VO_OUTPUT_PAL;
    break;
  case RKADK_VO_OUTPUT_NTSC:
    enVoIntfSync = VO_OUTPUT_NTSC;
    break;
  case RKADK_VO_OUTPUT_1080P24:
    enVoIntfSync = VO_OUTPUT_1080P24;
    break;
  case RKADK_VO_OUTPUT_1080P25:
    enVoIntfSync = VO_OUTPUT_1080P25;
    break;
  case RKADK_VO_OUTPUT_1080P30:
    enVoIntfSync = VO_OUTPUT_1080P30;
    break;
  case RKADK_VO_OUTPUT_720P50:
    enVoIntfSync = VO_OUTPUT_720P50;
    break;
  case RKADK_VO_OUTPUT_720P60:
    enVoIntfSync = VO_OUTPUT_720P60;
    break;
  case RKADK_VO_OUTPUT_1080I50:
    enVoIntfSync = VO_OUTPUT_1080I50;
    break;
  case RKADK_VO_OUTPUT_1080I60:
    enVoIntfSync = VO_OUTPUT_1080I60;
    break;
  case RKADK_VO_OUTPUT_1080P50:
    enVoIntfSync = VO_OUTPUT_1080P50;
    break;
  case RKADK_VO_OUTPUT_1080P60:
    enVoIntfSync = VO_OUTPUT_1080P60;
    break;
  case RKADK_VO_OUTPUT_576P50:
    enVoIntfSync = VO_OUTPUT_576P50;
    break;
  case RKADK_VO_OUTPUT_480P60:
    enVoIntfSync = VO_OUTPUT_480P60;
    break;
  case RKADK_VO_OUTPUT_800x600_60:
    enVoIntfSync = VO_OUTPUT_800x600_60;
    break;
  case RKADK_VO_OUTPUT_1024x768_60:
    enVoIntfSync = VO_OUTPUT_1024x768_60;
    break;
  case RKADK_VO_OUTPUT_1280x1024_60:
    enVoIntfSync = VO_OUTPUT_1280x1024_60;
    break;
  case RKADK_VO_OUTPUT_1366x768_60:
    enVoIntfSync = VO_OUTPUT_1366x768_60;
    break;
  case RKADK_VO_OUTPUT_1440x900_60:
    enVoIntfSync = VO_OUTPUT_1440x900_60;
    break;
  case RKADK_VO_OUTPUT_1280x800_60:
    enVoIntfSync = VO_OUTPUT_1280x800_60;
    break;
  case RKADK_VO_OUTPUT_1600x1200_60:
    enVoIntfSync = VO_OUTPUT_1600x1200_60;
    break;
  case RKADK_VO_OUTPUT_1680x1050_60:
    enVoIntfSync = VO_OUTPUT_1680x1050_60;
    break;
  case RKADK_VO_OUTPUT_1920x1200_60:
    enVoIntfSync = VO_OUTPUT_1920x1200_60;
    break;
  case RKADK_VO_OUTPUT_640x480_60:
    enVoIntfSync = VO_OUTPUT_640x480_60;
    break;
  case RKADK_VO_OUTPUT_960H_PAL:
    enVoIntfSync = VO_OUTPUT_960H_PAL;
    break;
  case RKADK_VO_OUTPUT_960H_NTSC:
    enVoIntfSync = VO_OUTPUT_960H_NTSC;
    break;
  case RKADK_VO_OUTPUT_1920x2160_30:
    enVoIntfSync = VO_OUTPUT_1920x2160_30;
    break;
  case RKADK_VO_OUTPUT_2560x1440_30:
    enVoIntfSync = VO_OUTPUT_2560x1440_30;
    break;
  case RKADK_VO_OUTPUT_2560x1440_60:
    enVoIntfSync = VO_OUTPUT_2560x1440_60;
    break;
  case RKADK_VO_OUTPUT_2560x1600_60:
    enVoIntfSync = VO_OUTPUT_2560x1600_60;
    break;
  case RKADK_VO_OUTPUT_3840x2160_24:
    enVoIntfSync = VO_OUTPUT_3840x2160_24;
    break;
  case RKADK_VO_OUTPUT_3840x2160_25:
    enVoIntfSync = VO_OUTPUT_3840x2160_25;
    break;
  case RKADK_VO_OUTPUT_3840x2160_30:
    enVoIntfSync = VO_OUTPUT_3840x2160_30;
    break;
  case RKADK_VO_OUTPUT_3840x2160_50:
    enVoIntfSync = VO_OUTPUT_3840x2160_50;
    break;
  case RKADK_VO_OUTPUT_3840x2160_60:
    enVoIntfSync = VO_OUTPUT_3840x2160_60;
    break;
  case RKADK_VO_OUTPUT_4096x2160_24:
    enVoIntfSync = VO_OUTPUT_4096x2160_24;
    break;
  case RKADK_VO_OUTPUT_4096x2160_25:
    enVoIntfSync = VO_OUTPUT_4096x2160_25;
    break;
  case RKADK_VO_OUTPUT_4096x2160_30:
    enVoIntfSync = VO_OUTPUT_4096x2160_30;
    break;
  case RKADK_VO_OUTPUT_4096x2160_50:
    enVoIntfSync = VO_OUTPUT_4096x2160_50;
    break;
  case RKADK_VO_OUTPUT_4096x2160_60:
    enVoIntfSync = VO_OUTPUT_4096x2160_60;
    break;
  case RKADK_VO_OUTPUT_320x240_60:
    enVoIntfSync = VO_OUTPUT_320x240_60;
    break;
  case RKADK_VO_OUTPUT_320x240_50:
    enVoIntfSync = VO_OUTPUT_320x240_50;
    break;
  case RKADK_VO_OUTPUT_240x320_50:
    enVoIntfSync = VO_OUTPUT_240x320_50;
    break;
  case RKADK_VO_OUTPUT_240x320_60:
    enVoIntfSync = VO_OUTPUT_240x320_60;
    break;
  case RKADK_VO_OUTPUT_800x600_50:
    enVoIntfSync = VO_OUTPUT_800x600_50;
    break;
  case RKADK_VO_OUTPUT_720x1280_60:
    enVoIntfSync = VO_OUTPUT_720x1280_60;
    break;
  case RKADK_VO_OUTPUT_1080x1920_60:
    enVoIntfSync = VO_OUTPUT_1080x1920_60;
    break;
  case RKADK_VO_OUTPUT_7680x4320_30:
    enVoIntfSync = VO_OUTPUT_7680x4320_30;
    break;
  case RKADK_VO_OUTPUT_USER:
    enVoIntfSync = VO_OUTPUT_USER;
    break;
  case RKADK_VO_OUTPUT_DEFAULT:
    enVoIntfSync = VO_OUTPUT_DEFAULT;
    break;
  default:
    RKADK_LOGW("Invalid enIntfSync[%d], use VO_OUTPUT_DEFAULT", enIntfSync);
    enVoIntfSync = VO_OUTPUT_DEFAULT;
    break;
  }

  return enVoIntfSync;
}

RKADKSurfaceInterface::RKADKSurfaceInterface(VIDEO_FRAMEINFO_S *pstFrmInfo)
    : pCbMblk(nullptr), s32Flag(0) {
  memset(&stFrmInfo, 0, sizeof(VIDEO_FRAMEINFO_S));
  if (pstFrmInfo) {
    memcpy(&stFrmInfo, pstFrmInfo, sizeof(VIDEO_FRAMEINFO_S));
  } else {
    RKADK_LOGW("don't set video frame info");
    stFrmInfo.enIntfSync = RKADK_VO_OUTPUT_DEFAULT;
  }
}

RKADKSurfaceInterface::~RKADKSurfaceInterface() {
  RKADK_S32 s32Ret = 0;
  VO_LAYER voLayer;

  switch (stFrmInfo.u32VoDev) {
  case VO_DEV_HD0:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  case VO_DEV_HD1:
    voLayer = VOP_LAYER_CLUSTER_1;
    break;
  default:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  }

  if (s32Flag) {
      s32Ret = RK_MPI_VO_DisableChn(voLayer, stFrmInfo.u32ChnnNum);
    if (s32Ret)
      RKADK_LOGE("RK_MPI_VO_DisableChn failed[%d]", s32Ret);

    s32Ret = RK_MPI_VO_DisableLayer(voLayer);
    if (s32Ret)
      RKADK_LOGE("RK_MPI_VO_DisableLayer failed[%d]", s32Ret);

    s32Ret = RK_MPI_VO_Disable(stFrmInfo.u32VoDev);
    if (s32Ret)
      RKADK_LOGE("RK_MPI_VO_Disable failed[%d]", s32Ret);

    s32Ret = RK_MPI_VO_UnBindLayer(voLayer, stFrmInfo.u32VoDev);
    if (s32Ret)
      RKADK_LOGE("RK_MPI_VO_UnBindLayer failed[%d]", s32Ret);

    s32Flag = 0;
    RKADK_LOGD("done!");
  }
}

INT32 RKADKSurfaceInterface::queueBuffer(void *buf, INT32 fence) {
  int ret = 0, error;
  VIDEO_FRAME_INFO_S stVFrameInfo;
  VO_LAYER voLayer;
  RTMediaBuffer *pstMediaBuffer;
  RtMetaData *pstMetaData;

  RKADK_CHECK_POINTER(buf, RKADK_FAILURE);

  pCbMblk = buf;
  rt_memset(&stVFrameInfo, 0, sizeof(VIDEO_FRAME_INFO_S));
  stVFrameInfo.stVFrame.pMbBlk = buf;

  pstMediaBuffer = reinterpret_cast<RTMediaBuffer *>(buf);
  pstMetaData = pstMediaBuffer->getMetaData();
  pstMetaData->findInt32(kKeyFrameError, &error);
  if (error) {
    RKADK_LOGE("find frame error");
    return -1;
  }

  if (!pstMetaData->findInt32(kKeyFrameW,
                              (int *)&stVFrameInfo.stVFrame.u32VirWidth)) {
    RKADK_LOGE("not find virtual width in meta");
    return -1;
  }
  if (!pstMetaData->findInt32(kKeyFrameH,
                              (int *)&stVFrameInfo.stVFrame.u32VirHeight)) {
    RKADK_LOGE("not find virtual height in meta");
    return -1;
  }

  if (!pstMetaData->findInt32(kKeyVCodecWidth,
                              (int *)&stVFrameInfo.stVFrame.u32Width)) {
    RKADK_LOGE("not find width in meta");
    return -1;
  }
  if (!pstMetaData->findInt32(kKeyVCodecHeight,
                              (int *)&stVFrameInfo.stVFrame.u32Height)) {
    RKADK_LOGE("not find height in meta");
    return -1;
  }

  stVFrameInfo.stVFrame.enPixelFormat = RK_FMT_YUV420SP;
  stVFrameInfo.stVFrame.enCompressMode = COMPRESS_MODE_NONE;

  switch (stFrmInfo.u32VoDev) {
  case VO_DEV_HD0:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  case VO_DEV_HD1:
    voLayer = VOP_LAYER_CLUSTER_1;
    break;
  default:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  }

  if (!s32Flag) {
    VO_LAYER_MODE_E mode;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;

    RKADK_LOGD("stVFrame[%d, %d, %d, %d]", stVFrameInfo.stVFrame.u32Width,
               stVFrameInfo.stVFrame.u32Height,
               stVFrameInfo.stVFrame.u32VirWidth,
               stVFrameInfo.stVFrame.u32VirHeight);

    rt_memset(&stVoPubAttr, 0, sizeof(VO_PUB_ATTR_S));
    rt_memset(&stLayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

    /* Bind Layer */
    switch (stFrmInfo.u32VoLayerMode) {
    case 0:
      mode = VO_LAYER_MODE_CURSOR;
      break;
    case 1:
      mode = VO_LAYER_MODE_GRAPHIC;
      break;
    case 2:
      mode = VO_LAYER_MODE_VIDEO;
      break;
    default:
      mode = VO_LAYER_MODE_VIDEO;
    }

    ret = RK_MPI_VO_BindLayer(voLayer, stFrmInfo.u32VoDev, mode);
    if (ret) {
      RKADK_LOGD("RK_MPI_VO_BindLayer failed(%d)", ret);
      goto failed;
    }

    /* Enable VO Device */
    switch (stFrmInfo.u32EnIntfType) {
    case DISPLAY_TYPE_HDMI:
      stVoPubAttr.enIntfType = VO_INTF_HDMI;
      break;
    case DISPLAY_TYPE_EDP:
      stVoPubAttr.enIntfType = VO_INTF_EDP;
      break;
    case DISPLAY_TYPE_VGA:
      stVoPubAttr.enIntfType = VO_INTF_VGA;
      break;
    case DISPLAY_TYPE_MIPI:
      stVoPubAttr.enIntfType = VO_INTF_MIPI;
      break;
    default:
      stVoPubAttr.enIntfType = VO_INTF_HDMI;
      RKADK_LOGD("option not set ,use HDMI default");
    }

    stVoPubAttr.enIntfSync = RKADK_VO_GetIntfSync(stFrmInfo.enIntfSync);
    if (VO_OUTPUT_USER == stVoPubAttr.enIntfSync)
      RKADK_VO_SetRtSyncInfo(&stVoPubAttr.stSyncInfo, stFrmInfo);

    ret = RKADK_VO_Enable(stFrmInfo.u32VoDev, &stVoPubAttr);
    if (ret) {
      RKADK_LOGD("RKADK_VO_Enable failed(%d)", ret);
      return ret;
    }

    /* Enable Layer */
    stLayerAttr.enPixFormat = RKADK_FmtToRtfmt(stFrmInfo.u32VoFormat);

    ret = RKADK_VO_SetLayerRect(&stLayerAttr, stFrmInfo.u32VoDev, stFrmInfo);
    if (ret) {
      RKADK_LOGD("RKADK_VO_SetLayerRect failed(%d)", ret);
      goto failed;
    }

    stLayerAttr.u32DispFrmRt = stFrmInfo.u32DispFrmRt;
    ret = RKADK_VO_EnableLayer(voLayer, &stLayerAttr);
    if (ret) {
      RKADK_LOGD("RKADK_VO_EnableLayer failed(%d)", ret);
      goto failed;
    }

    /* Enable channel */
    ret = RKADK_VO_EnableChnn(voLayer, &stLayerAttr, stFrmInfo);
    if (ret) {
      RKADK_LOGD("RKADK_VO_EnableChnn failed(%d)", ret);
      goto failed;
    }
    RK_MPI_VO_ClearChnBuffer(voLayer, stFrmInfo.u32ChnnNum, RK_TRUE);
    s32Flag = 1;
  }

  ret = RK_MPI_VO_SendFrame(voLayer, stFrmInfo.u32ChnnNum, &stVFrameInfo, 0);
  if (ret) {
    RKADK_LOGW("RK_MPI_VO_SendFrame failed(%d)", ret);
    return ret;
  }

  return RKADK_SUCCESS;

failed:
  RK_MPI_VO_DisableLayer(voLayer);
  RK_MPI_VO_Disable(stFrmInfo.u32VoDev);
  RK_MPI_VO_UnBindLayer(voLayer, stFrmInfo.u32VoDev);
  return -1;
}

void RKADKSurfaceInterface::resume() {
  VO_LAYER voLayer;

  switch (stFrmInfo.u32VoDev) {
  case VO_DEV_HD0:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  case VO_DEV_HD1:
    voLayer = VOP_LAYER_CLUSTER_1;
    break;
  default:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  }

  RK_MPI_VO_ResumeChn(voLayer, stFrmInfo.u32ChnnNum);
}

void RKADKSurfaceInterface::replay() {
  VO_LAYER voLayer;
  RKADK_S32 s32Ret = 0;

  switch (stFrmInfo.u32VoDev) {
  case VO_DEV_HD0:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  case VO_DEV_HD1:
    voLayer = VOP_LAYER_CLUSTER_1;
    break;
  default:
    voLayer = VOP_LAYER_CLUSTER_0;
    break;
  }

  /* pause channel to stop recevice any new buffers */
  RK_MPI_VO_PauseChn(voLayer, stFrmInfo.u32ChnnNum);
  if (s32Flag)
    RK_MPI_VO_ClearChnBuffer(voLayer, stFrmInfo.u32ChnnNum, RK_TRUE);
}

#endif // ROCKIT
