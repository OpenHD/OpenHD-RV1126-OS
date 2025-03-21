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

#ifndef CVR_RESOURCE_MANAGER_H_
#define CVR_RESOURCE_MANAGER_H_

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Load lvgl resource from specific file path.
// The lv_img resource file should be png.
lv_img_dsc_t* lv_res_create(char* res_path);

// Destroy lvgl resource.
int lv_res_destroy(lv_img_dsc_t** res);

#ifdef __cplusplus
}
#endif

#endif // CVR_RESOURCE_MANAGER_H_