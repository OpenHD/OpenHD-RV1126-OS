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

#ifndef _CVR_PHOTO_H_
#define _CVR_PHOTO_H_

#include <rkadk/rkadk_photo.h>
#include <rkadk/rkadk_common.h>

#define MAX_PATH_LEN 128

int photo_init(RKADK_U32 u32CamID);
int photo_deinit();
int photo_take_photo();
int photo_display_start(char *photo_path, RKADK_PHOTO_DATA_ATTR_S *pDataAttr,
        RKADK_U32 photo_width, RKADK_U32 photo_height);
int photo_display_stop(RKADK_PHOTO_DATA_ATTR_S *pDataAttr);

#endif //_CVR_PHOTO_H_

