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
#include "vi_isp.h"
#include <rkadk/rkadk_param.h>
#include <rkadk/rkadk_vi_isp.h>

static char *iq_file_path = "/etc/iqfiles";

void cvr_wait_vi_converge(void) {
    int num = 0, ret;
    Uapi_ExpQueryInfo_t *expinfo = NULL;

    expinfo = (Uapi_ExpQueryInfo_t *)malloc(sizeof(Uapi_ExpQueryInfo_t));
    if (!expinfo) {
        printf("malloc Uapi_ExpQueryInfo_t fail!\n");
    } else {
        while (1) {
            usleep(200 * 1000);
            ret = RKADK_VI_ISP_GET_AeExpResInfo(0, expinfo);
            if (!ret) {
                if (expinfo->IsConverged) {
                    break;
                }
            }
            num++;
            if (num >= 10) {
                break;
            }
        }
        free(expinfo);
        expinfo = NULL;
    }
}

int cvr_sdk_param_init() {
    char* global_setting = "/oem/etc/rkadk/rkadk_setting.ini";
    char* sensor_setting_array[] = { "/oem/etc/rkadk/rkadk_setting_sensor_0.ini", NULL };
    RKADK_PARAM_Init(global_setting, sensor_setting_array);
}

int cvr_vi_init(void) {
    int fps;

    if (RKADK_PARAM_GetCamParam(0, RKADK_PARAM_TYPE_FPS, &fps)) {
        printf("RKADK_PARAM_GetCamParam fps failed");
        return -1;
    }

    rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
    RKADK_BOOL fec_enable = RKADK_FALSE;
    RKADK_VI_ISP_Start(0, hdr_mode, fec_enable, iq_file_path, fps);

    return 0;
}

int cvr_vi_deinit(void)
{
    RKADK_VI_ISP_Stop(0);
    return 0;
}

