/*
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "rk_aiq_utils.h"
#include "xcam_log.h"

RKAIQ_BEGIN_DECLARE

XCamReturn rk_aiq_AeReg2RealValue
(
    const CalibDb_Sensor_Para_t* stSensorInfo,
    float   PixelPeriodsPerLine,
    float   LinePeriodsPerField,
    float   PixelClockFreqMHZ,
    uint32_t sensorRegTime,
    uint32_t sensorRegGain,
    float* realTime,
    float* realGain
) {

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    //gain
    if(stSensorInfo->GainRange.IsLinear) {
        float gainRange[] = {1, 2, 128, 0, 1, 128, 255,
                             2, 4, 64, -248, 1, 376, 504,
                             4, 8, 32, -756, 1, 884, 1012,
                             8, 16, 16, -1784, 1, 1912, 2040
                            };

        const float* pgainrange = NULL;
        uint32_t size = 0;

        if (stSensorInfo->GainRange.array_size <= 0) {
            pgainrange = gainRange;
            size = sizeof(gainRange) / sizeof(float);
        } else {
            pgainrange = stSensorInfo->GainRange.pGainRange;
            size = stSensorInfo->GainRange.array_size;
        }

        int *revert_gain_array = (int *)malloc((size / 7 * 2) * sizeof(int));
        if(revert_gain_array == NULL) {
            LOGE("%s: malloc fail", __func__);
            return  XCAM_RETURN_ERROR_MEM;
        }

        for(uint32_t i = 0; i < (size / 7); i++) {
            revert_gain_array[i * 2 + 0] = (int)(pgainrange[i * 7 + 2] * pow(pgainrange[i * 7 + 0], pgainrange[i * 7 + 4]) - pgainrange[i * 7 + 3] + 0.5f);
            revert_gain_array[i * 2 + 1] = (int)(pgainrange[i * 7 + 2] * pow(pgainrange[i * 7 + 1], pgainrange[i * 7 + 4]) - pgainrange[i * 7 + 3] + 0.5f);
        }

        // AG = (C1 * (analog gain^M0) - C0) + 0.5f
        float C1 = 0.0f, C0 = 0.0f, M0 = 0.0f, minReg = 0.0f, maxReg = 0.0f, minrealGain = 0.0f, maxrealGain = 0.0f;
        float ag = sensorRegGain;
        uint32_t i = 0;
        for(i = 0; i < (size / 7); i++) {
            if (ag >= revert_gain_array[i * 2 + 0] && ag <= revert_gain_array[i * 2 + 1]) {
                C1 = pgainrange[i * 7 + 2];
                C0 = pgainrange[i * 7 + 3];
                M0 = pgainrange[i * 7 + 4];
                minReg = pgainrange[i * 7 + 5];
                maxReg = pgainrange[i * 7 + 6];
                //LOGE("gain(%f) c1:%f c0:%f m0:%f min:%f max:%f", ag, C1, C0, M0, minReg, maxReg);
                break;
            }
        }

        if(i > (size / 7)) {
            LOGE("GAIN OUT OF RANGE: lasttime-gain: %d-%d", sensorRegTime, sensorRegGain);
            C1 = 16;
            C0 = 0;
            M0 = 1;
            minReg = 16;
            maxReg = 255;
        }

        *realGain = pow(10, log10(((float)sensorRegGain + C0) / C1) / M0);
        minrealGain = pow(10, log10(((float)minReg + C0) / C1) / M0);
        maxrealGain = pow(10, log10(((float)maxReg + C0) / C1) / M0);

        if (*realGain < minrealGain)
            *realGain = minrealGain;
        if (*realGain > maxrealGain)
            *realGain = maxrealGain;

        if(revert_gain_array != NULL) {
            free(revert_gain_array);
            revert_gain_array = NULL;
        }
    } else {
        *realGain = pow(10, (float)sensorRegGain * 3 / (10.0f * 20.0f));
    }

    //float dcg_ratio = (sensorDcgmode >= 1 ? pConfig->stDcgInfo.Normal.dcg_ratio : 1.0f);
    //realGain *= dcg_ratio;

    //time
    float timeC0 = stSensorInfo->TimeFactor[0];
    float timeC1 = stSensorInfo->TimeFactor[1];
    float timeC2 = stSensorInfo->TimeFactor[2];
    float timeC3 = stSensorInfo->TimeFactor[3];

    *realTime = ((sensorRegTime - timeC0 * LinePeriodsPerField - timeC1) / timeC2 /*- timeC3*/) *
                  PixelPeriodsPerLine / (PixelClockFreqMHZ * 1000000);

    return (ret);
}

RKAIQ_END_DECLARE
