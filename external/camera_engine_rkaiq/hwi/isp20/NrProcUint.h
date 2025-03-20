
/*
 * NrProcUint.h
 *
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
#ifndef _NR_PROC_H_
#define _NR_PROC_H_

#include "poll_thread.h"
#include "smartptr.h"
#include "v4l2_device.h"
#include "IspParamsAssembler.h"
#include "Isp20Params.h"

#define NRPROC_SUBM (1 << 7)

using namespace XCam;

namespace RkCam {

class NrParamsUint;
class NrStatsUint;

class NrProcUint
    : public PollCallback
{
public:
    NrProcUint ();
    virtual ~NrProcUint ();
    virtual XCamReturn init (const rk_sensor_full_info_t *s_info);
    virtual XCamReturn deInit ();
    virtual XCamReturn prepare ();
	virtual XCamReturn start ();
	virtual XCamReturn stop ();
    XCamReturn pause () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn resume () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn configParams(SmartPtr<RkAiqIsppOtherParamsProxy>& nrParams);
    XCamReturn configAmdResult (rk_aiq_amd_result_t &amdResult);
    // XCamReturn mergeResults (SmartPtr<cam3aResult> result);
    bool set_poll_thread (SmartPtr<PollThread> thread);
	bool set_poll_callback (PollCallback *callback);
	bool set_params_translator (Isp20Params* translator);
    bool set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev);
    XCamReturn set_amd_status (bool en);
    XCamReturn set_eis_status (bool en);

    // from PollCallback
    virtual XCamReturn poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type);
    virtual XCamReturn poll_buffer_failed (int64_t timestamp, const char *msg);
    virtual XCamReturn poll_event_ready (uint32_t sequence, int type);
    virtual XCamReturn poll_event_failed (int64_t timestamp, const char *msg) {
        return  XCAM_RETURN_ERROR_FAILED;
    };

protected:

private:
    enum nr_state_e {
        NR_STATE_INVALID,
        NR_STATE_INITED,
        NR_STATE_PREPARED,
        NR_STATE_STARTED,
        NR_STATE_PAUSED,
        NR_STATE_STOPPED,
    };

    // SmartPtr<CamHwBase>                 _cam_hw;
    SmartPtr<PollThread>                _poll_thread;
	PollCallback                        *_poll_callback;
	SmartPtr<NrStatsUint>               _nr_stats;
	SmartPtr<NrParamsUint>              _nr_params;
    SmartPtr<RkAiqAmdResPool>           _amdResPool;
    bool                                _enEis {false};
    int                                 _state{NR_STATE_INVALID};
};

class NrParamsUint {
public:
	NrParamsUint ();
    virtual ~NrParamsUint ();
    virtual XCamReturn init (const rk_sensor_full_info_t *s_info);
    virtual XCamReturn deInit ();
    virtual XCamReturn prepare ();
    XCamReturn start ();
    XCamReturn stop ();
    XCamReturn pause () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn resume () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
	bool set_params_translator (Isp20Params* translator);
    bool set_poll_thread (SmartPtr<PollThread> thread);
    XCamReturn set_amd_status (bool en);

    XCamReturn configParams(SmartPtr<cam3aResult> result);
    XCamReturn configToDrv(IspParamsAssembler<cam3aResult>::tList& aiqNrParams);
protected:
    SmartPtr<IspParamsAssembler<cam3aResult>> mParamsAssembler;

private:
    XCAM_DEAD_COPY (NrParamsUint);
    Mutex                                       _mutex;
    SmartPtr<V4l2Device> 				        _nr_params_dev;
    Isp20Params                                 *_translator;
    SmartPtr<PollThread>                        _poll_thread;
    struct rkispp_params_nrcfg 				    _last_nr_params;
    bool                                        _enAmd {false};

    SmartPtr<V4l2SubDevice> 			mIsppSubDev;
    struct rkispp_params_tnrcfg 		last_ispp_tnr_params;
    rk_sensor_full_info_t               *_s_info;
};

/*
 * typedef struct nr_buf_info_s {
 *     uint32_t    width;
 *     uint32_t    height;
 *
 *     int32_t     fd_array[16];
 *     int32_t     idx_array[16];
 *     int32_t     buf_cnt;
 * } nr_buf_info_t;
 */

class NrStatsUint
{
public:
    NrStatsUint ();
    virtual ~NrStatsUint ();

    virtual XCamReturn init (const rk_sensor_full_info_t *s_info);
    virtual XCamReturn deInit ();
    virtual XCamReturn prepare ();
    XCamReturn start ();
    XCamReturn stop ();
    XCamReturn pause () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn resume () {
        return  XCAM_RETURN_ERROR_FAILED;
    };

    bool is_buf_init () {
       return _init_fd.load();
    };
    bool set_poll_thread (SmartPtr<PollThread> thread);
    bool set_poll_callback (PollCallback *callback);
    bool set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev);
    int queue_NRImg_fd(int fd, uint32_t frameid);
    int get_NRImg_fd(uint32_t frameid);
    bool fill_buffer_info (SmartPtr<V4l2BufferProxy> &buf);

protected:
    bool init_nrbuf_fd          ();
    bool deinit_nrbuf_fd        ();
    int get_NRimg_fd_by_index(int index);
    int32_t get_idx_by_fd(int32_t fd);
    XCamReturn get_buf_format (rk_aiq_nr_buf_info_t& buf_info);

private:
    SmartPtr<V4l2Device>                _nr_stats_dev;
    SmartPtr<V4l2SubDevice>             _ispp_subdev;
    SmartPtr<PollThread>                _poll_thread;
    rk_sensor_full_info_t               *_s_info;
	PollCallback                    	*_poll_callback;

    struct v4l2_subdev_format _ispp_fmt;
    rk_aiq_nr_buf_info_t _nr_buf_info;
    /*
     * int _fd_array[16];
     * int _idx_array[16];
     * int _buf_num;
     */
    std::atomic<bool> _init_fd{false};
    //SmartPtr<NrParamProcThread> _proc_thread;
    Mutex _list_mutex;
    // CamHwIsp20* _camHw;
    SmartPtr<VideoBuffer> _NrImage;
    std::map<uint32_t, int> _NrImg_ready_map;
};

};
#endif
