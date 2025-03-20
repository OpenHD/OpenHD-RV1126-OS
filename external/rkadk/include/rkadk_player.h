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

#ifndef __RKADK_PLAYER_H__
#define __RKADK_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rkadk_common.h"

/** Error information of the player*/
typedef enum {
  RKADK_PLAYER_ERROR_VID_PLAY_FAIL = 0x0, /**< The video fails to be played. */
  RKADK_PLAYER_ERROR_AUD_PLAY_FAIL,       /**< The audio fails to be played. */
  RKADK_PLAYER_ERROR_DEMUX_FAIL,          /**< The file fails to be played. */
  RKADK_PLAYER_ERROR_TIMEOUT, /**< Operation timeout. For example, reading data
                                 timeout. */
  RKADK_PLAYER_ERROR_NOT_SUPPORT, /**< The file format is not supportted. */
  RKADK_PLAYER_ERROR_UNKNOW,      /**< Unknown error. */
  RKADK_PLAYER_ERROR_ILLEGAL_STATEACTION, /**< illegal action at cur state. */
  RKADK_PLAYER_ERROR_BUTT,
} RKADK_PLAYER_ERROR_E;

/** Player status*/
typedef enum {
  RKADK_PLAYER_STATE_IDLE = 0, /**< The player state before init . */
  RKADK_PLAYER_STATE_INIT, /**< The player is in the initial state. It changes
                              to the initial state after being SetDataSource. */
  RKADK_PLAYER_STATE_PREPARED, /**< The player is in the prepared state. */
  RKADK_PLAYER_STATE_PLAY,     /**< The player is in the playing state. */
  RKADK_PLAYER_STATE_TPLAY,    /**< The player is in the trick playing state*/
  RKADK_PLAYER_STATE_PAUSE,    /**< The player is in the pause state. */
  RKADK_PLAYER_STATE_ERR,      /**< The player is in the err state. */
  RKADK_PLAYER_STATE_BUTT
} RKADK_PLAYER_STATE_E;

typedef enum {
  RKADK_PLAYER_EVENT_STATE_CHANGED = 0x0, /**< the player status changed */
  RKADK_PLAYER_EVENT_PREPARED,
  RKADK_PLAYER_EVENT_STARTED,
  RKADK_PLAYER_EVENT_PAUSED,
  RKADK_PLAYER_EVENT_STOPPED,
  RKADK_PLAYER_EVENT_EOF, /**< the player is playing the end */
  RKADK_PLAYER_EVENT_SOF, /**< the player backward tplay to the start of file*/
  RKADK_PLAYER_EVENT_PROGRESS, /**< current playing progress. it will be called
                                  every one second. the additional value that in
                                  the unit of ms is current playing time */
  RKADK_PLAYER_EVENT_SEEK_END, /**< seek time jump, the additional value is the
                                  seek value */
  RKADK_PLAYER_EVENT_ERROR,    /**< play error */
  RKADK_PLAYER_EVENT_BUTT
} RKADK_PLAYER_EVENT_E;

typedef enum {
  VO_FORMAT_ARGB8888 = 0,
  VO_FORMAT_ABGR8888,
  VO_FORMAT_RGB888,
  VO_FORMAT_BGR888,
  VO_FORMAT_ARGB1555,
  VO_FORMAT_ABGR1555,
  VO_FORMAT_NV12,
  VO_FORMAT_NV21
} RKADK_PLAYER_VO_FORMAT_E;

typedef enum { VO_DEV_HD0 = 0, VO_DEV_HD1 } RKADK_PLAYER_VO_DEV_E;

typedef enum {
  DISPLAY_TYPE_HDMI = 0,
  DISPLAY_TYPE_EDP,
  DISPLAY_TYPE_VGA,
  DISPLAY_TYPE_MIPI,
} RKADK_PLAYER_VO_INTF_TYPE_E;

typedef enum {
  CHNN_ASPECT_RATIO_AUTO = 1,
  CHNN_ASPECT_RATIO_MANUAL,
} RKADK_PLAYER_VO_CHNN_MODE_E;
typedef RKADK_VOID (*RKADK_PLAYER_EVENT_FN)(RKADK_MW_PTR pPlayer,
                                            RKADK_PLAYER_EVENT_E enEvent,
                                            RKADK_VOID *pData);

/** player configuration */
typedef struct {
  RKADK_BOOL bEnableVideo;
  RKADK_BOOL bEnableAudio;
  RKADK_PLAYER_EVENT_FN pfnPlayerCallback;
} RKADK_PLAYER_CFG_S;

/* Must be consistent with rockit/rk_comm_vo.h -> VO_INTF_SYNC_E */
typedef enum {
  RKADK_VO_OUTPUT_PAL = 0, /* PAL standard */
  RKADK_VO_OUTPUT_NTSC,    /* NTSC standard */

  RKADK_VO_OUTPUT_1080P24, /* 1920 x 1080 at 24 Hz. */
  RKADK_VO_OUTPUT_1080P25, /* 1920 x 1080 at 25 Hz. */
  RKADK_VO_OUTPUT_1080P30, /* 1920 x 1080 at 30 Hz. */

  RKADK_VO_OUTPUT_720P50, /* 1280 x  720 at 50 Hz. */
  RKADK_VO_OUTPUT_720P60, /* 1280 x  720 at 60 Hz. */

  RKADK_VO_OUTPUT_1080I50, /* 1920 x 1080 at 50 Hz, interlace. */
  RKADK_VO_OUTPUT_1080I60, /* 1920 x 1080 at 60 Hz, interlace. */
  RKADK_VO_OUTPUT_1080P50, /* 1920 x 1080 at 50 Hz. */
  RKADK_VO_OUTPUT_1080P60, /* 1920 x 1080 at 60 Hz. */

  RKADK_VO_OUTPUT_576P50, /* 720  x  576 at 50 Hz. */
  RKADK_VO_OUTPUT_480P60, /* 720  x  480 at 60 Hz. */

  RKADK_VO_OUTPUT_800x600_60,   /* VESA 800 x 600 at 60 Hz (non-interlaced) */
  RKADK_VO_OUTPUT_1024x768_60,  /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
  RKADK_VO_OUTPUT_1280x1024_60, /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
  RKADK_VO_OUTPUT_1366x768_60,  /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
  RKADK_VO_OUTPUT_1440x900_60,  /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT
                                   Compliant */
  RKADK_VO_OUTPUT_1280x800_60,  /* 1280*800@60Hz VGA@60Hz */
  RKADK_VO_OUTPUT_1600x1200_60, /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
  RKADK_VO_OUTPUT_1680x1050_60, /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
  RKADK_VO_OUTPUT_1920x1200_60, /* VESA 1920 x 1600 at 60 Hz (non-interlaced)
                                   CVT (Reduced Blanking) */
  RKADK_VO_OUTPUT_640x480_60, /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
  RKADK_VO_OUTPUT_960H_PAL,  /* ITU-R BT.1302 960 x 576 at 50 Hz (interlaced) */
  RKADK_VO_OUTPUT_960H_NTSC, /* ITU-R BT.1302 960 x 480 at 60 Hz (interlaced) */
  RKADK_VO_OUTPUT_1920x2160_30, /* 1920x2160_30 */
  RKADK_VO_OUTPUT_2560x1440_30, /* 2560x1440_30 */
  RKADK_VO_OUTPUT_2560x1440_60, /* 2560x1440_60 */
  RKADK_VO_OUTPUT_2560x1600_60, /* 2560x1600_60 */
  RKADK_VO_OUTPUT_3840x2160_24, /* 3840x2160_24 */
  RKADK_VO_OUTPUT_3840x2160_25, /* 3840x2160_25 */
  RKADK_VO_OUTPUT_3840x2160_30, /* 3840x2160_30 */
  RKADK_VO_OUTPUT_3840x2160_50, /* 3840x2160_50 */
  RKADK_VO_OUTPUT_3840x2160_60, /* 3840x2160_60 */
  RKADK_VO_OUTPUT_4096x2160_24, /* 4096x2160_24 */
  RKADK_VO_OUTPUT_4096x2160_25, /* 4096x2160_25 */
  RKADK_VO_OUTPUT_4096x2160_30, /* 4096x2160_30 */
  RKADK_VO_OUTPUT_4096x2160_50, /* 4096x2160_50 */
  RKADK_VO_OUTPUT_4096x2160_60, /* 4096x2160_60 */
  RKADK_VO_OUTPUT_320x240_60,   /* For ota5182 at 60 Hz (8bit)  */
  RKADK_VO_OUTPUT_320x240_50,   /* For ili9342 at 50 Hz (6bit)  */
  RKADK_VO_OUTPUT_240x320_50,   /* For ili9341 at 50 Hz (6bit) */
                                /* For st7789 at 50Hz(6bit) */
  RKADK_VO_OUTPUT_240x320_60,   /* For ili9341 at 60 Hz (16bit) */
  RKADK_VO_OUTPUT_800x600_50,   /* For LCD     at 50 Hz (24bit) */
  RKADK_VO_OUTPUT_720x1280_60,  /* For MIPI DSI Tx 720 x1280 at 60 Hz */
  RKADK_VO_OUTPUT_1080x1920_60, /* For MIPI DSI Tx 1080x1920 at 60 Hz */
  RKADK_VO_OUTPUT_7680x4320_30, /* For HDMI2.1 at 30 Hz         */
  RKADK_VO_OUTPUT_USER,         /* User timing. */
  RKADK_VO_OUTPUT_DEFAULT,

  RKADK_VO_OUTPUT_BUTT
} RKADK_VO_INTF_SYNC_E;

/* Must be consistent with rockit/rk_comm_vo.h -> VO_SYNC_INFO_S */
typedef struct {
  RKADK_BOOL bSynm; /* RW; sync mode(0:timing,as BT.656; 1:signal,as LCD) */
  RKADK_BOOL bIop;  /* RW; interlaced or progressive display(0:i; 1:p) */

  RKADK_U16 u16Vact; /* RW; vertical active area */
  RKADK_U16 u16Vbb;  /* RW; vertical back blank porch */
  RKADK_U16 u16Vfb;  /* RW; vertical front blank porch */

  RKADK_U16 u16Hact; /* RW; horizontal active area */
  RKADK_U16 u16Hbb;  /* RW; horizontal back blank porch */
  RKADK_U16 u16Hfb;  /* RW; horizontal front blank porch */
  RKADK_U16 u16Hmid; /* RW; bottom horizontal active area */

  RKADK_U16 u16Bvact; /* RW; bottom vertical active area */
  RKADK_U16 u16Bvbb;  /* RW; bottom vertical back blank porch */
  RKADK_U16 u16Bvfb;  /* RW; bottom vertical front blank porch */

  RKADK_U16 u16Hpw; /* RW; horizontal pulse width */
  RKADK_U16 u16Vpw; /* RW; vertical pulse width */

  RKADK_BOOL bIdv; /* RW; inverse data valid of output */
  RKADK_BOOL bIhs; /* RW; polarity of horizontal synch signal, 0: negative, 1:
                      positive */
  RKADK_BOOL bIvs; /* RW; polarity of vertical synch signal, 0: negative, 1:
                      positive */

  RKADK_U16 u16FrameRate; /* RW; frame rate of output */
  RKADK_U16 u16PixClock;  /* RW; pixel clock, the unit is KHZ */
} RKADK_VO_SYNC_INFO_S;

typedef struct {
  RKADK_BOOL bMirror;     /* RW; Mirror enable. */
  RKADK_BOOL bFlip;       /* RW; Flip enable. */
  RKADK_U32 u32Rotation;  /* RW; rotation: 0, 90, 180, 270 */
  RKADK_RECT_S stChnRect; /* RW; Rectangle of video output channel */
} RKADK_PLAYER_VO_ATTR_S;

/** video output frameinfo */
typedef struct {
  RKADK_U32 u32FrmInfoS32x;
  RKADK_U32 u32FrmInfoS32y;
  RKADK_U32 u32DispWidth;
  RKADK_U32 u32DispHeight;
  RKADK_U32 u32ImgWidth;
  RKADK_U32 u32ImgHeight;
  RKADK_U32 u32VoLayerMode;
  RKADK_U32 u32ChnnNum;
  RKADK_U32 u32BorderColor;
  RKADK_U32 u32BorderTopWidth;
  RKADK_U32 u32BorderBottomWidth;
  RKADK_U32 u32BorderLeftWidth;
  RKADK_U32 u32BorderRightWidth;
  RKADK_PLAYER_VO_CHNN_MODE_E u32EnMode;
  RKADK_PLAYER_VO_FORMAT_E u32VoFormat;
  RKADK_PLAYER_VO_DEV_E u32VoDev;
  RKADK_PLAYER_VO_INTF_TYPE_E u32EnIntfType;
  RKADK_U32 u32DispFrmRt;
  RKADK_VO_INTF_SYNC_E enIntfSync;
  RKADK_VO_SYNC_INFO_S stSyncInfo;
  RKADK_PLAYER_VO_ATTR_S stVoAttr;
} RKADK_PLAYER_FRAMEINFO_S;

/**
 * @brief create the player
 * @param[in] pstPlayCfg : player config
 * @param[out] ppPlayer : RKADK_MW_PTR*: handle of the player
 * @retval 0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Create(RKADK_MW_PTR *ppPlayer,
                              RKADK_PLAYER_CFG_S *pstPlayCfg);

/**
 * @brief  destroy the player
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 *  @retval 0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Destroy(RKADK_MW_PTR pPlayer);

/**
 * @brief    set the file for playing
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[in] filePath : RKADK_CHAR: media file path
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_SetDataSource(RKADK_MW_PTR pPlayer,
                                     const RKADK_CHAR *pszfilePath);

/**
 * @brief prepare for the playing
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Prepare(RKADK_MW_PTR pPlayer);

/**
 * @brief setcallback for the playing
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[in] frameinfo : RKADK_FRAMEINFO_S: record displayer info
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_SetVideoSink(RKADK_MW_PTR pPlayer,
                                    RKADK_PLAYER_FRAMEINFO_S *pstFrameInfo);

/**
 * @brief  do play of the stream
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Play(RKADK_MW_PTR pPlayer);

/**
 * @brief stop the stream playing, and release the resource
 * @param[in] pPlayer : void *: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Stop(RKADK_MW_PTR pPlayer);

/**
 * @brief pause the stream playing
 * @param[in] hPlayer : RKADK_MW_PTR: handle of the player
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Pause(RKADK_MW_PTR pPlayer);

/**
 * @brief seek by the time
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[in] s64TimeInMs : RKADK_S64: seek time
 * @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_Seek(RKADK_MW_PTR pPlayer, RKADK_S64 s64TimeInMs);

/**
 * @brief get the current play status
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[out] penState : RKADK_LITEPLAYER_STATE_E*: play state
 *  @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_GetPlayStatus(RKADK_MW_PTR pPlayer,
                                     RKADK_PLAYER_STATE_E *penState);
/**
 * @brief get the current play file duration(ms)
 * @param[in] pPlayer : RKADK_MW_PTR: handle of the player
 * @param[out] pDuration : play duration(ms)
 *  @retval  0 success, others failed
 */
RKADK_S32 RKADK_PLAYER_GetDuration(RKADK_MW_PTR pPlayer, RKADK_U32 *pDuration);

#ifdef __cplusplus
}
#endif
#endif
