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

#include "RTMediaBuffer.h"
#include "RTMetadataRetriever.h"
#include "rkadk_param.h"
#include "rkadk_media_comm.h"
#include "rkadk_record.h"
#include "rkadk_thumb.h"
#include "rkmedia_api.h"
#include <malloc.h>
#include <rga/RgaApi.h>
#include <rga/rga.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <utime.h>

// #define DUMP_RUN_TIME
// #define THUMB_SAVE_FILE
#define THM_BOX_HEADER_LEN 8 /* size: 4byte, type: 4byte */
#define RGA_ZOOM_MAX 16

typedef struct {
  RKADK_U32 width;
  RKADK_U32 height;
  RKADK_U32 strideWidth;
  RKADK_U32 strideHeight;
} FRAME_RECT_S;

typedef struct {
  int format;
  void *virAddr;
  FRAME_RECT_S stRect;
} THUMB_INFO_S;

static int GetRgaType(RKADK_THUMB_TYPE_E enType) {
  int format = RK_FORMAT_UNKNOWN;

  switch (enType) {
  case RKADK_THUMB_TYPE_NV12:
  case RKADK_THUMB_TYPE_JPEG:
    format = RK_FORMAT_YCbCr_420_SP;
    break;
  case RKADK_THUMB_TYPE_RGB565:
    format = RK_FORMAT_RGB_565;
    break;
  case RKADK_THUMB_TYPE_RGB888:
    format = RK_FORMAT_RGB_888;
    break;
  case RKADK_THUMB_TYPE_RGBA8888:
    format = RK_FORMAT_RGBA_8888;
    break;
  default:
    RKADK_LOGE("Invalid enType[%d]", enType);
    break;
  }

  return format;
}

static RKADK_U32 GetDataLen(RKADK_U32 width, RKADK_U32 height, int format) {
  RKADK_U32 u32DataLen;

  if (format == RK_FORMAT_YCbCr_420_SP)
    u32DataLen = width * height * 3 / 2;
  else if (format == RK_FORMAT_RGB_565)
    u32DataLen = width * height * 2;
  else if (format == RK_FORMAT_RGB_888)
    u32DataLen = width * height * 3;
  else if (format == RK_FORMAT_RGBA_8888)
    u32DataLen = width * height * 4;
  else {
    RKADK_LOGE("Invalid format[%d]", format);
    return -1;
  }

  return u32DataLen;
}

static int RgaProcess(THUMB_INFO_S *stSrcInfo, THUMB_INFO_S *stDstInfo) {
  int ret;
  rga_info_t srcInfo;
  rga_info_t dstInfo;

  memset(&srcInfo, 0, sizeof(rga_info_t));
  srcInfo.fd = -1;
  srcInfo.virAddr = stSrcInfo->virAddr;
  srcInfo.mmuFlag = 1;
  srcInfo.rotation = 0;
  rga_set_rect(&srcInfo.rect, 0, 0, stSrcInfo->stRect.width,
               stSrcInfo->stRect.height, stSrcInfo->stRect.strideWidth,
               stSrcInfo->stRect.strideHeight, stSrcInfo->format);

  memset(&dstInfo, 0, sizeof(rga_info_t));
  dstInfo.fd = -1;
  dstInfo.virAddr = stDstInfo->virAddr;
  dstInfo.mmuFlag = 1;
  rga_set_rect(&dstInfo.rect, 0, 0, stDstInfo->stRect.width,
               stDstInfo->stRect.height, stDstInfo->stRect.strideWidth,
               stDstInfo->stRect.strideHeight, stDstInfo->format);

  ret = c_RkRgaBlit(&srcInfo, &dstInfo, NULL);
  if (ret)
    RKADK_LOGE("c_RkRgaBlit scale failed(%d)", ret);

  return ret;
}

static RKADK_U32 getZoomCount(RKADK_U32 u32SrcLen, RKADK_U32 u32DstLen) {
  RKADK_U32 count = 0;
  float remainder;

  remainder = u32SrcLen / RGA_ZOOM_MAX;
  if (remainder <= u32DstLen)
    return 1;

  count++;
  while (remainder > u32DstLen) {
    remainder = remainder / RGA_ZOOM_MAX;
    count++;
  }

  return count;
}

static RKADK_U32 ZoomCount(FRAME_RECT_S stSrcRect, FRAME_RECT_S stDstRect) {
  RKADK_U32 widthZoomcnt = 0, heightZoomcnt = 0;

  widthZoomcnt = getZoomCount(stSrcRect.strideWidth, stDstRect.strideWidth);
  heightZoomcnt = getZoomCount(stSrcRect.strideHeight, stDstRect.strideHeight);

  return widthZoomcnt >= heightZoomcnt ? widthZoomcnt : heightZoomcnt;
}

static int SetThumbInfo(THUMB_INFO_S stSrcInfo, THUMB_INFO_S *pstDstInfo,
                        RKADK_THUMB_TYPE_E enType) {
  RKADK_U32 u32DataLen;

  pstDstInfo->format = stSrcInfo.format;
  pstDstInfo->stRect.width = stSrcInfo.stRect.width / RGA_ZOOM_MAX;
  if (stSrcInfo.stRect.width % RGA_ZOOM_MAX)
    pstDstInfo->stRect.width += 1;
  pstDstInfo->stRect.width = UPALIGNTO(pstDstInfo->stRect.width, 2);
  pstDstInfo->stRect.strideWidth = UPALIGNTO(pstDstInfo->stRect.width, 4);

  pstDstInfo->stRect.height = stSrcInfo.stRect.height / RGA_ZOOM_MAX;
  if (stSrcInfo.stRect.height % RGA_ZOOM_MAX)
    pstDstInfo->stRect.height += 1;
  pstDstInfo->stRect.height = UPALIGNTO(pstDstInfo->stRect.height, 2);
  pstDstInfo->stRect.strideHeight = pstDstInfo->stRect.height;

  u32DataLen = GetDataLen(pstDstInfo->stRect.strideWidth,
                          pstDstInfo->stRect.strideHeight, GetRgaType(enType));
  if (u32DataLen <= 0) {
    RKADK_LOGE("GetDataLen failed");
    return -1;
  }

  pstDstInfo->virAddr = malloc(u32DataLen);
  if (!pstDstInfo->virAddr) {
    RKADK_LOGE("malloc pstDstInfo.virAddr failed");
    return -1;
  }

  memset(pstDstInfo->virAddr, 0, u32DataLen);
  RKADK_LOGD("malloc pstDstInfo->virAddr[%p]", pstDstInfo->virAddr);
  return 0;
}

static int GetFrameRect(RtMetaData *meta, FRAME_RECT_S *pstRect) {
  RKADK_CHECK_POINTER(meta, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstRect, RKADK_FAILURE);

  // valid width and height of datas
  if (!meta->findInt32(kKeyVCodecWidth, (int *)&pstRect->width)) {
    RKADK_LOGW("not find width in meta");
    return -1;
  }

  if (!meta->findInt32(kKeyVCodecHeight, (int *)&pstRect->height)) {
    RKADK_LOGW("not find height in meta");
    return -1;
  }

  // virtual of width and height
  if (!meta->findInt32(kKeyFrameW, (int *)&pstRect->strideWidth)) {
    RKADK_LOGW("not find width virtual in meta");
    return -1;
  }

  if (!meta->findInt32(kKeyFrameH, (int *)&pstRect->strideHeight)) {
    RKADK_LOGW("not find height virtual in meta");
    return -1;
  }

  return 0;
}

static int YuvScale(RTMediaBuffer *buffer, char *scaleBuffer,
                    RKADK_U32 scaleBufLen, FRAME_RECT_S stScaleRect,
                    RKADK_THUMB_TYPE_E enType) {
  int ret = 0, error;
  int zoomCnt;
  THUMB_INFO_S stSrcInfo, stDstInfo;
  THUMB_INFO_S stSrcTmpInfo, stDstTmpInfo;
  THUMB_INFO_S *pstSrcInfo = NULL, *pstDstInfo = NULL;

#ifdef DUMP_RUN_TIME
  struct timeval tv_start, tv_end;
  long start, end;

  gettimeofday(&tv_start, NULL);
  start = ((long)tv_start.tv_sec) * 1000 + (long)tv_start.tv_usec / 1000;
#endif

  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(scaleBuffer, RKADK_FAILURE);

  memset(&stSrcInfo, 0, sizeof(THUMB_INFO_S));
  memset(&stDstInfo, 0, sizeof(THUMB_INFO_S));
  memset(&stSrcTmpInfo, 0, sizeof(THUMB_INFO_S));
  memset(&stDstTmpInfo, 0, sizeof(THUMB_INFO_S));

  RtMetaData *meta = buffer->getMetaData();
  meta->findInt32(kKeyFrameError, &error);
  if (error) {
    RKADK_LOGE("frame error");
    return -1;
  }

  stSrcInfo.format = RK_FORMAT_YCbCr_420_SP;
  stSrcInfo.virAddr = buffer->getData();

  if (GetFrameRect(meta, &stSrcInfo.stRect))
    return -1;

  RKADK_LOGD("Src [%d, %d, %d, %d]", stSrcInfo.stRect.width,
             stSrcInfo.stRect.height, stSrcInfo.stRect.strideWidth,
             stSrcInfo.stRect.strideHeight);
  RKADK_LOGD("Src buffer size = %d", buffer->getSize());

  if (enType == RKADK_THUMB_TYPE_RGB565)
    stDstInfo.format = RK_FORMAT_RGB_565;
  else if (enType == RKADK_THUMB_TYPE_RGB888)
    stDstInfo.format = RK_FORMAT_RGB_888;
  else if (enType == RKADK_THUMB_TYPE_RGBA8888)
    stDstInfo.format = RK_FORMAT_RGBA_8888;
  else
    stDstInfo.format = RK_FORMAT_YCbCr_420_SP;
  stDstInfo.virAddr = scaleBuffer;
  memcpy(&stDstInfo.stRect, &stScaleRect, sizeof(FRAME_RECT_S));

  RKADK_LOGD("Dts [%d, %d, %d, %d]", stScaleRect.width, stScaleRect.height,
             stScaleRect.strideWidth, stScaleRect.strideHeight);

  if ((stDstInfo.format == stSrcInfo.format) &&
      (stDstInfo.stRect.strideWidth == stSrcInfo.stRect.strideWidth) &&
      (stDstInfo.stRect.strideHeight && stSrcInfo.stRect.strideHeight)) {
    RKADK_U32 u32CopySize =
        scaleBufLen > buffer->getSize() ? buffer->getSize() : scaleBufLen;
    memcpy(scaleBuffer, buffer->getData(), u32CopySize);
    return 0;
  }

  ret = c_RkRgaInit();
  if (ret < 0) {
    RKADK_LOGE("c_RkRgaInit failed(%d)", ret);
    return -1;
  }

  zoomCnt = ZoomCount(stSrcInfo.stRect, stDstInfo.stRect);
  RKADK_LOGD("zoomCnt = %d", zoomCnt);
  if (zoomCnt == 1) {
    ret = RgaProcess(&stSrcInfo, &stDstInfo);
  } else {
    for (int i = 0; i < zoomCnt; i++) {
      if (i == 0) {
        pstSrcInfo = &stSrcInfo;

        ret = SetThumbInfo(stSrcInfo, &stDstTmpInfo, enType);
        if (ret) {
          RKADK_LOGE("SetThumbInfo failed");
          goto End;
        }

        pstDstInfo = &stDstTmpInfo;
      } else if (i == (zoomCnt - 1)) {
        pstSrcInfo = &stDstTmpInfo;
        pstDstInfo = &stDstInfo;
      } else {
        if (stSrcTmpInfo.virAddr) {
          RKADK_LOGD("free stSrcTmpInfo.virAddr[%p]", stSrcTmpInfo.virAddr);
          free(stSrcTmpInfo.virAddr);
        }

        memcpy(&stSrcTmpInfo, &stDstTmpInfo, sizeof(THUMB_INFO_S));

        ret = SetThumbInfo(stSrcTmpInfo, &stDstTmpInfo, enType);
        if (ret) {
          RKADK_LOGE("SetThumbInfo failed");
          goto End;
        }

        pstSrcInfo = &stSrcTmpInfo;
        pstDstInfo = &stDstTmpInfo;
      }

      RKADK_LOGD("Zoom %d", i);
      RKADK_LOGD("Src[%d, %d, %d, %d]", pstSrcInfo->stRect.width,
                 pstSrcInfo->stRect.height, pstSrcInfo->stRect.strideWidth,
                 pstSrcInfo->stRect.strideHeight);
      RKADK_LOGD("Dst[%d, %d, %d, %d]", pstDstInfo->stRect.width,
                 pstDstInfo->stRect.height, pstDstInfo->stRect.strideWidth,
                 pstDstInfo->stRect.strideHeight);

      ret = RgaProcess(pstSrcInfo, pstDstInfo);
      if (ret) {
        RKADK_LOGE("RgaProcess failed(%d)", ret);
        goto End;
      }
    }
  }

End:
  if (stSrcTmpInfo.virAddr) {
    RKADK_LOGD("free stSrcTmpInfo.virAddr[%p]", stSrcTmpInfo.virAddr);
    free(stSrcTmpInfo.virAddr);
  }

  if (stDstTmpInfo.virAddr) {
    RKADK_LOGD("free stDstTmpInfo.virAddr[%p]", stDstTmpInfo.virAddr);
    free(stDstTmpInfo.virAddr);
  }

  c_RkRgaDeInit();

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_end, NULL);
  end = ((long)tv_end.tv_sec) * 1000 + (long)tv_end.tv_usec / 1000;
  RKADK_LOGD("rga scale run time: %ld\n", end - start);
#endif

  return ret;
}

static int YuvToJpg(RTMediaBuffer *buffer, RKADK_FRAME_ATTR_S *pstFrameAttr,
                    RKADK_U32 u32VeChn) {
  int ret = 0;
  char *scaleBuffer;
  RKADK_U32 scaleBufferLen = 0;
  RKADK_U32 jpgSize = 0;
  FRAME_RECT_S stScaleRect;
  VENC_CHN_ATTR_S vencChnAttr;
  MEDIA_BUFFER mb = NULL;
  MEDIA_BUFFER jpg_mb = NULL;

#ifdef THUMB_SAVE_FILE
  FILE *file = NULL;
#endif

  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstFrameAttr, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstFrameAttr->pu8Buf, RKADK_FAILURE);

  memset(pstFrameAttr->pu8Buf, 0, pstFrameAttr->u32BufSize);

  stScaleRect.width = pstFrameAttr->u32Width;
  stScaleRect.height = pstFrameAttr->u32Height;
  stScaleRect.strideWidth = pstFrameAttr->u32VirWidth;
  stScaleRect.strideHeight = pstFrameAttr->u32VirHeight;
  scaleBufferLen =
      stScaleRect.strideWidth * stScaleRect.strideHeight * 3 / 2; // NV12

  MB_IMAGE_INFO_S stImageInfo = {stScaleRect.width, stScaleRect.height,
                                 stScaleRect.strideWidth,
                                 stScaleRect.strideHeight, IMAGE_TYPE_NV12};

  mb = RK_MPI_MB_CreateImageBuffer(&stImageInfo, RK_TRUE, MB_FLAG_NOCACHED);
  if (!mb) {
    RKADK_LOGE("no space left");
    return -1;
  }

  scaleBuffer = (char *)RK_MPI_MB_GetPtr(mb);
  ret = YuvScale(buffer, scaleBuffer, RK_MPI_MB_GetSize(mb), stScaleRect,
                 RKADK_THUMB_TYPE_JPEG);
  if (ret) {
    RKADK_LOGE("YuvScale failed(%d)", ret);
    goto free_mb;
  }

#ifdef THUMB_SAVE_FILE
  file = fopen("/userdata/scale.yuv", "w");
  if (!file) {
    RKADK_LOGE("Create /userdata/scale.yuv failed");
  } else {
    fwrite(scaleBuffer, 1, scaleBufferLen, file);
    fclose(file);
    RKADK_LOGD("fwrite scale.yuv done");
  }
#endif

  RK_MPI_SYS_Init();

  memset(&vencChnAttr, 0, sizeof(vencChnAttr));
  vencChnAttr.stVencAttr.enType = RK_CODEC_TYPE_JPEG;
  vencChnAttr.stVencAttr.imageType = IMAGE_TYPE_NV12;
  vencChnAttr.stVencAttr.u32PicWidth = stScaleRect.width;
  vencChnAttr.stVencAttr.u32PicHeight = stScaleRect.height;
  vencChnAttr.stVencAttr.u32VirWidth = stScaleRect.strideWidth;
  vencChnAttr.stVencAttr.u32VirHeight = stScaleRect.strideHeight;
  ret = RK_MPI_VENC_CreateChn(u32VeChn, &vencChnAttr);
  if (ret) {
    printf("Create Thumb Venc failed! ret=%d\n", ret);
    goto free_mb;
  }

  RK_MPI_MB_SetSize(mb, scaleBufferLen);
  RK_MPI_SYS_SendMediaBuffer(RK_ID_VENC, u32VeChn, mb);

  jpg_mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_VENC, u32VeChn, -1);
  if (!jpg_mb) {
    RKADK_LOGE("get null jpg buffer");
    ret = -1;
    goto exit;
  }

  jpgSize = RK_MPI_MB_GetSize(jpg_mb);
  if (pstFrameAttr->u32BufSize < jpgSize)
    RKADK_LOGW("buffer size[%d] < jpg size[%d]", pstFrameAttr->u32BufSize,
               jpgSize);
  else
    pstFrameAttr->u32BufSize = jpgSize;
  memcpy(pstFrameAttr->pu8Buf, RK_MPI_MB_GetPtr(jpg_mb),
         pstFrameAttr->u32BufSize);

#ifdef THUMB_SAVE_FILE
  file = fopen("/userdata/thumb.jpg", "w");
  if (!file) {
    RKADK_LOGE("Create /userdata/thumb.jpg failed");
  } else {
    fwrite(RK_MPI_MB_GetPtr(jpg_mb), 1, jpgSize, file);
    fclose(file);
    RKADK_LOGD("fwrite thumb.jpg done");
  }
#endif

  if (RK_MPI_SYS_StopGetMediaBuffer(RK_ID_VENC, u32VeChn))
    RKADK_LOGW("RK_MPI_SYS_StopGetMediaBuffer faield");

exit:
  RK_MPI_VENC_DestroyChn(u32VeChn);

free_mb:
  if (jpg_mb)
    RK_MPI_MB_ReleaseBuffer(jpg_mb);

  if (mb)
    RK_MPI_MB_ReleaseBuffer(mb);

  return ret;
}

static int YuvConvert(RTMediaBuffer *buffer, RKADK_FRAME_ATTR_S *pstFrameAttr) {
  FRAME_RECT_S stRect;
  RKADK_U32 u32DataLen;

  RKADK_CHECK_POINTER(buffer, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstFrameAttr, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstFrameAttr->pu8Buf, RKADK_FAILURE);

  memset(pstFrameAttr->pu8Buf, 0, pstFrameAttr->u32BufSize);

  stRect.width = pstFrameAttr->u32Width;
  stRect.height = pstFrameAttr->u32Height;
  stRect.strideWidth = pstFrameAttr->u32VirWidth;
  stRect.strideHeight = pstFrameAttr->u32VirHeight;
  u32DataLen = GetDataLen(stRect.strideWidth, stRect.strideHeight,
                          GetRgaType(pstFrameAttr->enType));
  if (u32DataLen > pstFrameAttr->u32BufSize) {
    RKADK_LOGE("thm data size[%d] > buffer size[%d]", u32DataLen,
               pstFrameAttr->u32BufSize);
    return -1;
  } else if (u32DataLen <= 0) {
    RKADK_LOGE("GetDataLen failed");
    return -1;
  }

  pstFrameAttr->u32BufSize = u32DataLen;
  return YuvScale(buffer, (char *)pstFrameAttr->pu8Buf,
                  pstFrameAttr->u32BufSize, stRect, pstFrameAttr->enType);
}

static RKADK_S32 BuildInThm(RKADK_CHAR *pszFileName,
                            RKADK_THUMB_ATTR_S *pstThumbAttr) {
  FILE *fd = NULL;
  int ret = -1;
  long file_size, cur_pos;
  bool bBuildInThm = false;
  RKADK_U32 u32BoxSize;
  char boxHeader[THM_BOX_HEADER_LEN];
  char largeSize[THM_BOX_HEADER_LEN];

  fd = fopen(pszFileName, "r+");
  if (!fd) {
    RKADK_LOGE("open %s failed", pszFileName);
    return -1;
  }

  if (fseek(fd, 0, SEEK_END) != 0 ||
      (file_size = ftell(fd)) < 0 ||
      fseek(fd, 0, SEEK_SET) != 0) {
    fclose(fd);
    RKADK_LOGE("get file %s size failed, errno = %d\n", pszFileName, errno);
    return -1;
  }

  while (!feof(fd)) {
    if (fread(boxHeader, THM_BOX_HEADER_LEN, 1, fd) != 1) {
      if (feof(fd)) {
        RKADK_LOGD("EOF");
        bBuildInThm = true;
      } else {
        RKADK_LOGE("Can't read box header");
      }
      break;
    }

    u32BoxSize = boxHeader[0] << 24 | boxHeader[1] << 16 | boxHeader[2] << 8 |
                 boxHeader[3];

    if (!u32BoxSize) {
      if (!boxHeader[4] && !boxHeader[5] && !boxHeader[6] && !boxHeader[7]) {
        RKADK_LOGE("invalid data, cover!!!");

        if (fseek(fd, -THM_BOX_HEADER_LEN, SEEK_CUR))
          RKADK_LOGE("seek failed");
        else
          bBuildInThm = true;
      } else {
        RKADK_LOGE("u32BoxSize = %d, invalid data", u32BoxSize);
      }

      break;
    } else if (u32BoxSize == 1) {
      if (fread(largeSize, THM_BOX_HEADER_LEN, 1, fd) != 1) {
        RKADK_LOGE("read largeSize failed");
        break;
      }

      u32BoxSize = (RKADK_U64)largeSize[0] << 56 |
                   (RKADK_U64)largeSize[1] << 48 |
                   (RKADK_U64)largeSize[2] << 40 |
                   (RKADK_U64)largeSize[3] << 32 | largeSize[4] << 24 |
                   largeSize[5] << 16 | largeSize[6] << 8 | largeSize[7];

      cur_pos = ftell(fd);

      if (u32BoxSize - THM_BOX_HEADER_LEN * 2 + cur_pos < 0 ||
          u32BoxSize - THM_BOX_HEADER_LEN * 2 + cur_pos > file_size) {
        RKADK_LOGE("bad largeSize value, this box is invalid. Insert thumb here!");
        fseek(fd, 0 - THM_BOX_HEADER_LEN * 2, SEEK_CUR);
        bBuildInThm = true;
        break;
      } else {
        fseek(fd, u32BoxSize - (THM_BOX_HEADER_LEN * 2), SEEK_CUR);
      }
    } else {
      cur_pos = ftell(fd);

      if (u32BoxSize - THM_BOX_HEADER_LEN + cur_pos < 0 ||
          u32BoxSize - THM_BOX_HEADER_LEN + cur_pos > file_size) {
        RKADK_LOGE("bad BoxSize value, this box is invalid. Insert thumb here!");
        fseek(fd, 0 - THM_BOX_HEADER_LEN, SEEK_CUR);
        bBuildInThm = true;
        break;
      } else {
          fseek(fd, u32BoxSize - THM_BOX_HEADER_LEN, SEEK_CUR);
      }
    }
  }

  if (!bBuildInThm)
    goto exit;

  // 16: 4bytes width + 4bytes height + 4bytes VirWidth + 4bytes VirHeight
  u32BoxSize = pstThumbAttr->u32BufSize + THM_BOX_HEADER_LEN + 16;
  boxHeader[0] = u32BoxSize >> 24;
  boxHeader[1] = (u32BoxSize & 0x00FF0000) >> 16;
  boxHeader[2] = (u32BoxSize & 0x0000FF00) >> 8;
  boxHeader[3] = u32BoxSize & 0x000000FF;
  boxHeader[4] = 't';
  boxHeader[5] = 'h';
  boxHeader[6] = 'm';
  boxHeader[7] = pstThumbAttr->enType;

  if (fwrite(boxHeader, THM_BOX_HEADER_LEN, 1, fd) != 1) {
    RKADK_LOGE("write thm box header failed");
    goto exit;
  }

  if (fwrite(&(pstThumbAttr->u32Width), 4, 1, fd) != 1) {
    RKADK_LOGE("write thm width failed");
    goto exit;
  }

  if (fwrite(&(pstThumbAttr->u32Height), 4, 1, fd) != 1) {
    RKADK_LOGE("write thm height failed");
    goto exit;
  }

  if (fwrite(&(pstThumbAttr->u32VirWidth), 4, 1, fd) != 1) {
    RKADK_LOGE("write thm virtual width failed");
    goto exit;
  }

  if (fwrite(&(pstThumbAttr->u32VirHeight), 4, 1, fd) != 1) {
    RKADK_LOGE("write thm virtual height failed");
    goto exit;
  }

  if (fwrite(pstThumbAttr->pu8Buf, pstThumbAttr->u32BufSize, 1, fd) != 1) {
    RKADK_LOGE("write thm box body failed");
    goto exit;
  }

  ret = 0;
  RKADK_LOGD("done!");

exit:
  if (fd)
    fclose(fd);

  return ret;
}

static RKADK_S32 GetThmInBox(RKADK_CHAR *pszFileName,
                             RKADK_THUMB_ATTR_S *pstThumbAttr) {
  FILE *fd = NULL;
  int ret = -1;
  int dstFormat;
  bool bMallocBuf = false;
  RKADK_U64 u32BoxSize;
  char boxHeader[THM_BOX_HEADER_LEN];
  char largeSize[THM_BOX_HEADER_LEN];
  THUMB_INFO_S stSrcInfo, stDstInfo;
  RKADK_U32 u32DstDataLen;

  fd = fopen(pszFileName, "r");
  if (!fd) {
    RKADK_LOGE("open %s failed", pszFileName);
    return -1;
  }

  if (!pstThumbAttr->pu8Buf)
    bMallocBuf = true;

  while (!feof(fd)) {
    if (fread(boxHeader, THM_BOX_HEADER_LEN, 1, fd) != 1) {
      if (feof(fd)) {
        RKADK_LOGD("EOF");
      } else {
        RKADK_LOGE("Can't read box header");
      }
      break;
    }

    u32BoxSize = boxHeader[0] << 24 | boxHeader[1] << 16 | boxHeader[2] << 8 |
                 boxHeader[3];
    if (u32BoxSize <= 0) {
      RKADK_LOGI("last one box, not find thm box");
      break;
    }

    if (u32BoxSize == 1) {
      if (fread(largeSize, THM_BOX_HEADER_LEN, 1, fd) != 1) {
        RKADK_LOGE("read largeSize failed");
        break;
      }

      u32BoxSize = (RKADK_U64)largeSize[0] << 56 |
                   (RKADK_U64)largeSize[1] << 48 |
                   (RKADK_U64)largeSize[2] << 40 |
                   (RKADK_U64)largeSize[3] << 32 | largeSize[4] << 24 |
                   largeSize[5] << 16 | largeSize[6] << 8 | largeSize[7];

      if (fseek(fd, u32BoxSize - (THM_BOX_HEADER_LEN * 2), SEEK_CUR)) {
        RKADK_LOGE("largeSize seek failed");
        break;
      }

      continue;
    }

    if (boxHeader[4] == 't' && boxHeader[5] == 'h' && boxHeader[6] == 'm' &&
        boxHeader[7] == pstThumbAttr->enType) {

      memset(&stSrcInfo, 0, sizeof(THUMB_INFO_S));
      memset(&stDstInfo, 0, sizeof(THUMB_INFO_S));
      if (fread(&(stSrcInfo.stRect.width), 4, 1, fd) != 1) {
        RKADK_LOGE("read thm width failed");
        break;
      }

      if (fread(&(stSrcInfo.stRect.height), 4, 1, fd) != 1) {
        RKADK_LOGE("read thm height failed");
        break;
      }

      if (fread(&(stSrcInfo.stRect.strideWidth), 4, 1, fd) != 1) {
        RKADK_LOGE("read thm virtual width failed");
        break;
      }

      if (fread(&(stSrcInfo.stRect.strideHeight), 4, 1, fd) != 1) {
        RKADK_LOGE("read thm virtual height failed");
        break;
      }

      // 16: 4bytes width + 4bytes height + 4bytes VirWidth + 4bytes VirHeight
      RKADK_U32 u32DataSize = u32BoxSize - THM_BOX_HEADER_LEN - 16;

      if (stSrcInfo.stRect.width != pstThumbAttr->u32Width ||
        stSrcInfo.stRect.height != pstThumbAttr->u32Height) {

          if (pstThumbAttr->enType == RKADK_THUMB_TYPE_RGB565)
            dstFormat = RK_FORMAT_RGB_565;
          else if (pstThumbAttr->enType == RKADK_THUMB_TYPE_RGB888)
            dstFormat = RK_FORMAT_RGB_888;
          else if (pstThumbAttr->enType == RKADK_THUMB_TYPE_RGBA8888)
            dstFormat = RK_FORMAT_RGBA_8888;
          else
            break;
          stSrcInfo.format = dstFormat;
          stDstInfo.format = dstFormat;

          if (bMallocBuf) {
            ret = RKADK_MEDIA_FrameBufMalloc(pstThumbAttr);
            if (ret) {
              RKADK_LOGE("malloc thumbnail buffer[%d] failed", pstThumbAttr->u32BufSize);
              break;
            }
          } else {
            if (dstFormat == RK_FORMAT_RGB_565)
              u32DstDataLen = pstThumbAttr->u32VirWidth * pstThumbAttr->u32VirHeight * 2;
            else if (dstFormat == RK_FORMAT_RGB_888)
              u32DstDataLen = pstThumbAttr->u32VirWidth * pstThumbAttr->u32VirHeight * 3;
            else if (dstFormat == RK_FORMAT_RGBA_8888)
              u32DstDataLen = pstThumbAttr->u32VirWidth * pstThumbAttr->u32VirHeight * 4;
            if (pstThumbAttr->u32BufSize < u32DstDataLen) {
              RKADK_LOGW("buffer size[%d] < thm data size[%d]",
                         pstThumbAttr->u32BufSize, u32DstDataLen);
              break;
            }
          }
          stDstInfo.virAddr = pstThumbAttr->pu8Buf;
          stDstInfo.stRect.height = pstThumbAttr->u32Height;
          stDstInfo.stRect.width = pstThumbAttr->u32Width;
          stDstInfo.stRect.strideHeight = pstThumbAttr->u32VirHeight;
          stDstInfo.stRect.strideWidth = pstThumbAttr->u32VirWidth;

          stSrcInfo.virAddr = (RKADK_U8 *) malloc(u32DataSize);
          if (!stSrcInfo.virAddr) {
            RKADK_LOGE("malloc stSrcInfo.virAddr[%d] failed", u32DataSize);
            break;
          }
          if (fread(stSrcInfo.virAddr, u32DataSize, 1, fd) != 1){
            RKADK_LOGE("stSrcInfo.virAddr read thm box body failed");
            if (stSrcInfo.virAddr)
              free(stSrcInfo.virAddr);
            break;
          }

          if (ret = c_RkRgaInit() < 0 ||
              (ret = RgaProcess(&stSrcInfo, &stDstInfo))) {
            RKADK_LOGE("RgaProcess failed(%d)", ret);
            if (stSrcInfo.virAddr)
              free(stSrcInfo.virAddr);
            break;
          }
          c_RkRgaDeInit();
          if (stSrcInfo.virAddr) {
            free(stSrcInfo.virAddr);
          }
          break;
      } else {
        if (bMallocBuf) {
          pstThumbAttr->pu8Buf = (RKADK_U8 *)malloc(u32DataSize);
          if (!pstThumbAttr->pu8Buf) {
            RKADK_LOGE("malloc thumbnail buffer[%d] failed", u32DataSize);
            break;
          }
          pstThumbAttr->u32BufSize = u32DataSize;
        } else {
          if (pstThumbAttr->u32BufSize < u32DataSize)
            RKADK_LOGW("buffer size[%d] < thm data size[%d]",
                       pstThumbAttr->u32BufSize, u32DataSize);
          else
            pstThumbAttr->u32BufSize = u32DataSize;
        }

        if (fread(pstThumbAttr->pu8Buf, pstThumbAttr->u32BufSize, 1, fd) != 1)
          RKADK_LOGE("read thm box body failed");
        else {
          pstThumbAttr->u32Width = stSrcInfo.stRect.width;
          pstThumbAttr->u32Height = stSrcInfo.stRect.height;
          pstThumbAttr->u32VirWidth = stSrcInfo.stRect.strideWidth;
          pstThumbAttr->u32VirHeight = stSrcInfo.stRect.strideHeight;
          ret = 0;
        }
        break;
      }
    } else {
      if (fseek(fd, u32BoxSize - THM_BOX_HEADER_LEN, SEEK_CUR)) {
        RKADK_LOGE("seek failed");
        break;
      }
    }
  }

  if (fd)
    fclose(fd);

  if (ret && bMallocBuf)
    RKADK_ThmBufFree(pstThumbAttr);

  return ret;
}

RKADK_S32 RKADK_GetSingleFrameAtTime(RKADK_CHAR *pszFileName,
                                     RKADK_FRAME_ATTR_S *pstFrameAttr,
                                     RKADK_U32 u32VeChn, RKADK_U64 u64TimeUs) {
  int ret = 0;
  bool bMallocBuf = false;
  RTMediaBuffer *buffer = NULL;
  RtMetaData metaData;

#ifdef THUMB_SAVE_FILE
  FILE *file = NULL;
#endif

#ifdef DUMP_RUN_TIME
  struct timeval tv_begin, tv_end, tv_frame_end;
  long start, end, frame_end;

  gettimeofday(&tv_begin, NULL);
  start = ((long)tv_begin.tv_sec) * 1000 + (long)tv_begin.tv_usec / 1000;
#endif

  RKADK_CHECK_POINTER(pszFileName, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstFrameAttr, RKADK_FAILURE);

  RTMetadataRetriever *retriever = new RTMetadataRetriever();
  if (!retriever) {
    RKADK_LOGE("new RTMetadataRetriever failed");
    return -1;
  }

  ret = retriever->setDataSource(pszFileName, NULL);
  if (ret) {
    RKADK_LOGE("setDataSource failed(%d)", ret);
    goto exit;
  }

  metaData.setInt64(kRetrieverFrameAtTime, u64TimeUs);
  metaData.setInt32(kKeyVMaxInputBufferCnt, 1);
  metaData.setInt32(kKeyVMinOutputBufferCnt, 1);
  metaData.setInt32(kKeyVMaxOutputBufferCnt, 1);
  metaData.setInt32(kKeyUserVideoRefFrameNum, 1);

  buffer = retriever->getSingleFrameAtTime(&metaData);
  if (buffer == NULL || buffer->getMetaData() == NULL) {
    RKADK_LOGE("getSingleFrameAtTime failed");
    ret = -1;
    goto exit;
  }

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_frame_end, NULL);
  frame_end =
      ((long)tv_frame_end.tv_sec) * 1000 + (long)tv_frame_end.tv_usec / 1000;
  RKADK_LOGD("get yuv frame run time: %ld\n", frame_end - start);
#endif

#ifdef THUMB_SAVE_FILE
  file = fopen("/userdata/frame.yuv", "w");
  if (!file) {
    RKADK_LOGE("Create /userdata/frame.yuv failed");
  } else {
    fwrite(buffer->getData(), 1, buffer->getSize(), file);
    fclose(file);
    RKADK_LOGD("fwrite frame.yuv done");
  }
#endif

  if (!pstFrameAttr->pu8Buf)
    bMallocBuf = true;

  if (!pstFrameAttr->u32Width || !pstFrameAttr->u32Height ||
      !pstFrameAttr->u32VirWidth || !pstFrameAttr->u32VirHeight) {
    FRAME_RECT_S stFrameRect;
    if (GetFrameRect(buffer->getMetaData(), &stFrameRect)) {
      ret = -1;
      goto exit;
    }

    pstFrameAttr->u32Width = stFrameRect.width;
    pstFrameAttr->u32Height = stFrameRect.height;
    pstFrameAttr->u32VirWidth = stFrameRect.strideWidth;
    pstFrameAttr->u32VirHeight = stFrameRect.strideHeight;
  }

  if (!RKADK_MEDIA_CheckFrameAttr((RKADK_FRAME_ATTR_S *)pstFrameAttr)) {
    ret = -1;
    goto exit;
  }

  ret = RKADK_MEDIA_FrameBufMalloc(pstFrameAttr);
  if (ret)
    goto exit;

  if (pstFrameAttr->enType == RKADK_THUMB_TYPE_JPEG) {
    ret = YuvToJpg(buffer, pstFrameAttr, u32VeChn);
    if (ret)
      RKADK_LOGE("YuvToJpg failed");
  } else {
    ret = YuvConvert(buffer, pstFrameAttr);
    if (ret)
      RKADK_LOGE("YuvConvert failed");
  }

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_end, NULL);
  end = ((long)tv_end.tv_sec) * 1000 + (long)tv_end.tv_usec / 1000;
  RKADK_LOGD("yuv convert run time: %ld\n", end - frame_end);
#endif

exit:
  if (buffer)
    buffer->release();

  if (retriever)
    delete retriever;

  if (ret && bMallocBuf)
    RKADK_MEDIA_FrameFree(pstFrameAttr);

  malloc_trim(0);
  return ret;
}

static RKADK_S32 GetThmInMp4(RKADK_CHAR *pszFileName,
                             RKADK_THUMB_ATTR_S *pstThumbAttr) {
  int ret = 0, result;
  struct stat stStatBuf;
  struct utimbuf stTimebuf;

#ifdef DUMP_RUN_TIME
  struct timeval tv_begin, tv_end;
  long start, end;

  gettimeofday(&tv_begin, NULL);
  start = ((long)tv_begin.tv_sec) * 1000 + (long)tv_begin.tv_usec / 1000;
#endif

  RKADK_PARAM_Init(NULL, NULL);
  RKADK_PARAM_THUMB_CFG_S *ptsThumbCfg = RKADK_PARAM_GetThumbCfg();
  if (!ptsThumbCfg) {
    RKADK_LOGE("RKADK_PARAM_GetThumbCfg failed");
    return -1;
  }

  if (!pstThumbAttr->u32Width || !pstThumbAttr->u32Height) {
    pstThumbAttr->u32Width = UPALIGNTO(ptsThumbCfg->thumb_width, 4);
    pstThumbAttr->u32Height = UPALIGNTO(ptsThumbCfg->thumb_height, 2);
  }

  if (!pstThumbAttr->u32VirWidth || !pstThumbAttr->u32VirHeight) {
    pstThumbAttr->u32VirWidth = pstThumbAttr->u32Width;
    pstThumbAttr->u32VirHeight = pstThumbAttr->u32Height;
  }

  if (!RKADK_MEDIA_CheckFrameAttr((RKADK_FRAME_ATTR_S *)pstThumbAttr))
    return -1;

  memset(&stTimebuf, 0, sizeof(struct utimbuf));
  result = stat(pszFileName, &stStatBuf);
  if (result) {
    RKADK_LOGW("stat[%s] failed[%d]", pszFileName, result);
  } else {
    stTimebuf.actime = stStatBuf.st_atime;
    stTimebuf.modtime = stStatBuf.st_mtime;
  }

  ret = GetThmInBox(pszFileName, pstThumbAttr);
  if (!ret)
    goto exit;

  ret = RKADK_GetSingleFrameAtTime(pszFileName,
                                   (RKADK_FRAME_ATTR_S *)pstThumbAttr,
                                   ptsThumbCfg->venc_chn, 0);
  if (ret) {
    RKADK_LOGE("get single frame failed[%d]", ret);
    goto exit;
  }

  if (BuildInThm(pszFileName, pstThumbAttr))
    RKADK_LOGE("BuildInThm failed");

exit:
  if (stTimebuf.actime != 0 && stTimebuf.modtime != 0) {
    result = utime(pszFileName, &stTimebuf);
    if (result)
      RKADK_LOGW("utime[%s] failed[%d]", pszFileName, result);
  }

#ifdef DUMP_RUN_TIME
  gettimeofday(&tv_end, NULL);
  end = ((long)tv_end.tv_sec) * 1000 + (long)tv_end.tv_usec / 1000;
  RKADK_LOGD("get thumb run time: %ld\n", end - start);
#endif

  return ret;
}

RKADK_S32 RKADK_GetThmInMp4(RKADK_CHAR *pszFileName, RKADK_U8 *pu8Buf,
                            RKADK_U32 *pu32Size) {
  int ret;
  RKADK_THUMB_ATTR_S stThumbAttr;

  RKADK_CHECK_POINTER(pszFileName, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pu8Buf, RKADK_FAILURE);

  stThumbAttr.u32Width = 0;
  stThumbAttr.u32Height = 0;
  stThumbAttr.u32VirWidth = 0;
  stThumbAttr.u32VirHeight = 0;
  stThumbAttr.enType = RKADK_THUMB_TYPE_JPEG;
  stThumbAttr.pu8Buf = pu8Buf;
  stThumbAttr.u32BufSize = *pu32Size;

  ret = GetThmInMp4(pszFileName, &stThumbAttr);
  *pu32Size = stThumbAttr.u32BufSize;

  return ret;
}

RKADK_S32 RKADK_GetThmInMp4Ex(RKADK_CHAR *pszFileName,
                              RKADK_THUMB_ATTR_S *pstThumbAttr) {
  int ret;

  RKADK_CHECK_POINTER(pszFileName, RKADK_FAILURE);
  RKADK_CHECK_POINTER(pstThumbAttr, RKADK_FAILURE);

  ret = GetThmInMp4(pszFileName, pstThumbAttr);
  if (ret)
    RKADK_ThmBufFree(pstThumbAttr);

  return ret;
}

RKADK_S32 RKADK_ThmBufFree(RKADK_THUMB_ATTR_S *pstThumbAttr) {
  return RKADK_MEDIA_FrameFree((RKADK_FRAME_ATTR_S *)pstThumbAttr);
}

#endif // ROCKIT
