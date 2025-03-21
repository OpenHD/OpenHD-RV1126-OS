
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

#ifndef _FAKE_CAM_HW_ISP20_H_
#define _FAKE_CAM_HW_ISP20_H_
#include "rk_aiq_offline_raw.h"
#include "CamHwIsp20.h"

namespace RkCam {

class FakeCamHwIsp20 : public CamHwIsp20 {
public:
    explicit FakeCamHwIsp20();
    virtual ~FakeCamHwIsp20();
    virtual XCamReturn init(const char* sns_ent_name);
    virtual XCamReturn prepare(uint32_t width, uint32_t height, int mode, int t_delay, int g_delay);
    virtual XCamReturn enqueueRawBuffer(void *rawdata, bool sync);
    virtual XCamReturn enqueueRawFile(const char *path);
    virtual XCamReturn registRawdataCb(void (*callback)(void *));
    virtual XCamReturn rawdataPrepare(rk_aiq_raw_prop_t prop);
    virtual XCamReturn poll_event_ready (uint32_t sequence, int type);
    XCamReturn get_rkraw_frame_info(rkraw_frame_info_t *frame_info);
private:
    XCamReturn init_mipi_devices(rk_sensor_full_info_t *s_info);
    void parse_rk_rawdata(void *rawdata, struct rk_aiq_vbuf *vbuf);
    void parse_rk_rawfile(FILE *fp, struct rk_aiq_vbuf *vbuf);
    struct _st_addrinfo _st_addr[3];
    struct _raw_format _rawfmt;
    struct _frame_info _finfo;
    enum v4l2_memory    _rx_memory_type;
    enum v4l2_memory    _tx_memory_type;
    rk_aiq_rawbuf_type_t _rawbuf_type;
};

};
#endif
