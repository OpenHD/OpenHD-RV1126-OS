
/*
 * FecProcUint.h
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
#ifndef _FEC_PROC_H_
#define _FEC_PROC_H_

#include <map>
#include "poll_thread.h"
#include "smartptr.h"
#include "v4l2_device.h"
#include "Isp20Params.h"

#define FECPROC_SUBM (1 << 7)

using namespace XCam;

namespace RkCam {

class FecParamsUint;

class FecProcUint
    : public PollCallback
{
public:
    FecProcUint ();
    virtual ~FecProcUint ();
    virtual XCamReturn init (const rk_sensor_full_info_t *s_info);
    virtual XCamReturn deInit ();
    virtual XCamReturn prepare ();
	virtual XCamReturn start ();
	virtual XCamReturn stop ();
    virtual XCamReturn pause () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    virtual XCamReturn resume () {
        return  XCAM_RETURN_ERROR_FAILED;
    };
	bool set_poll_callback (PollCallback *callback);
	bool set_params_translator (Isp20Params* translator);
    XCamReturn configParams(SmartPtr<RkAiqFecParamsProxy>& fecParams);

    // from PollCallback
    virtual XCamReturn poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type) {
        return  XCAM_RETURN_ERROR_FAILED;
    };

    virtual XCamReturn poll_buffer_failed (int64_t timestamp, const char *msg) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    virtual XCamReturn poll_event_ready (uint32_t sequence, int type) {
        return  XCAM_RETURN_ERROR_FAILED;
    };
    virtual XCamReturn poll_event_failed (int64_t timestamp, const char *msg) {
        return  XCAM_RETURN_ERROR_FAILED;
    };

protected:

private:
    SmartPtr<PollThread>                _poll_thread;
	SmartPtr<FecParamsUint>             _fec_params;
};

class FecParamsUint {
public:
	FecParamsUint();
    virtual ~FecParamsUint();
    virtual XCamReturn init (const rk_sensor_full_info_t *s_info);
    virtual XCamReturn deInit ();
    virtual XCamReturn prepare ();
    XCamReturn start ();
    XCamReturn stop ();
    XCamReturn pause ();
    XCamReturn resume ();
    bool set_params_translator (Isp20Params* translator);
    bool set_poll_thread (SmartPtr<PollThread> thread);
    XCamReturn configParams (SmartPtr<RkAiqFecParamsProxy>& fecParams);
    XCamReturn configToDrv (SmartPtr<RkAiqFecParamsProxy>& aiqFecParams);

protected:

private:
    XCAM_DEAD_COPY (FecParamsUint);
    Mutex                                       _mutex;
    SmartPtr<V4l2Device> 	                    _fec_params_dev;
    SmartPtr<PollThread>                        _poll_thread;
    Isp20Params                                 *_translator;
    struct rkispp_params_feccfg 				_last_fec_params;
};

};
#endif
