
/*
 * TnrProcUint.h
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
#ifndef _TNR_PROC_H_
#define _TNR_PROC_H_

#include <map>
#include <mutex>

#include "poll_thread.h"
#include "smartptr.h"
#include "v4l2_device.h"
#include "Isp20Params.h"

#define TNRPROC_SUBM (1 << 7)

using namespace XCam;

namespace RkCam {

class TnrStatsUint;
class TnrParamsUint;

class TnrProcUint
    : public PollCallback
{
public:
    TnrProcUint ();
    virtual ~TnrProcUint ();
    XCamReturn init (const rk_sensor_full_info_t *s_info, bool enStats);
    XCamReturn deInit ();
    XCamReturn prepare ();
	XCamReturn start ();
	XCamReturn stop (bool is_pause = false);
	bool set_poll_callback (PollCallback *callback);
	bool set_params_translator (Isp20Params* translator);
    bool set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev);
    XCamReturn configParams(SmartPtr<RkAiqTnrParamsProxy>& tnrParams);

    // from PollCallback
    virtual XCamReturn poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type);
    virtual XCamReturn poll_buffer_failed (int64_t timestamp, const char *msg);
    virtual XCamReturn poll_event_ready (uint32_t sequence, int type) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    virtual XCamReturn poll_event_failed (int64_t timestamp, const char *msg) {
        return  XCAM_RETURN_ERROR_FAILED;
    };

protected:

private:
    enum tnr_state_e {
        TNR_STATE_INVALID,
        TNR_STATE_INITED,
        TNR_STATE_PREPARED,
        TNR_STATE_STARTED,
        TNR_STATE_PAUSED,
        TNR_STATE_STOPPED,
    };
    // SmartPtr<CamHwBase>                 _cam_hw;
	PollCallback                    	*_poll_callback;
    SmartPtr<PollThread>                _poll_thread;
    SmartPtr<TnrStatsUint>              _tnr_stats;
    SmartPtr<TnrParamsUint>             _tnr_params;
    /* is motion dection on? */
    bool isStatsEn;
    int _state{TNR_STATE_INVALID};
};

class TnrParamsUint {
public:
	TnrParamsUint();
    virtual ~TnrParamsUint();
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
    XCamReturn configParams(SmartPtr<RkAiqTnrParamsProxy>& tnrParams);
    XCamReturn configToDrv(SmartPtr<RkAiqTnrParamsProxy>& aiqTnrParams);

protected:

private:
    XCAM_DEAD_COPY (TnrParamsUint);
    Mutex                                       _mutex;
    SmartPtr<V4l2Device> 				        _tnr_params_dev;
    SmartPtr<PollThread>                        _poll_thread;
    Isp20Params                                 *_translator;
    struct rkispp_params_tnrcfg 				_last_tnr_params;

    SmartPtr<V4l2SubDevice> 			mIsppSubDev;
    rk_sensor_full_info_t               *_s_info;
};

class TnrStatsUint
{
public:
    TnrStatsUint ();
    virtual ~TnrStatsUint ();

    virtual XCamReturn init (const rk_sensor_full_info_t *s_info);
    virtual XCamReturn deInit ();
    virtual XCamReturn prepare ();
    XCamReturn start ();
    XCamReturn stop (bool is_pause);
    XCamReturn pause () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    XCamReturn resume () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    bool set_poll_thread (SmartPtr<PollThread> thread);
    bool set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev);
    bool fill_buffer_info (SmartPtr<V4l2BufferProxy> &buf);
    XCamReturn get_buf_format (rk_aiq_tnr_buf_info_t& buf_info);

protected:
    bool init_tnrbuf            ();
    void deinit_tnrbuf          ();
    int get_fd_by_index         (uint32_t index);

private:
    SmartPtr<V4l2Device>                _tnr_stats_dev;
    SmartPtr<V4l2SubDevice>             _ispp_subdev;
    SmartPtr<PollThread>                _poll_thread;
    rk_sensor_full_info_t               *_s_info;
	PollCallback                    	*_poll_callback;
    int32_t                             _tnr_stats_poll_stop_fd[2];

    typedef struct TnrInitBufInfo_s {
        uint32_t            _idx_array[64];
        int32_t             _fd_array[64];
        int32_t             _buf_num;
        std::atomic<bool>   _fd_init_flag {false};
    } TnrInitBufInfo_t;

    struct v4l2_subdev_format   _ispp_fmt;
    rk_aiq_tnr_buf_info_t       _tnr_buf_info;
    TnrInitBufInfo_t            _tnr_init_buf_info;
    std::map<uint32_t, int>     _idx_fd_map;
    Mutex _list_mutex;
    std::mutex _tnr_buf_mtx;
};

};
#endif
