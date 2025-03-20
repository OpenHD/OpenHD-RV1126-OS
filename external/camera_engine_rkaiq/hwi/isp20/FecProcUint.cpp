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

#include <fcntl.h>
#include "rk_aiq_pool.h"
#include "FecProcUint.h"

namespace RkCam {

FecProcUint::FecProcUint ()
    : _poll_thread (nullptr)
    , _fec_params (nullptr)
{
}

FecProcUint::~FecProcUint ()
{
}

XCamReturn
FecProcUint::init (const rk_sensor_full_info_t *s_info)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    _fec_params = new FecParamsUint();
    ret = _fec_params->init(s_info);
    if (ret) {
        LOGE_CAMHW_SUBM(FECPROC_SUBM, "failed to initialize fec params unit!\n");
        return ret;
    }

    if (!_poll_thread.ptr ())
        _poll_thread = new PollThread ();
    if (_poll_thread.ptr ()) {
        _fec_params->set_poll_thread (_poll_thread);
        _poll_thread->set_poll_callback (this);
    }

    return ret;
}

XCamReturn
FecProcUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (_fec_params.ptr ())
        _fec_params->deInit ();

    return ret;
}

XCamReturn
FecProcUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
FecProcUint::start ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (_fec_params.ptr ())
        _fec_params->start ();
    if (_poll_thread.ptr () ) {
        ret = _poll_thread->start ();
        if (ret != XCAM_RETURN_NO_ERROR)
            LOGE_CAMHW_SUBM(FECPROC_SUBM, "Failed to start pollthread");
    }

    return ret;
}

XCamReturn
FecProcUint::stop ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (_poll_thread.ptr ())
        _poll_thread->stop ();
    if (_fec_params.ptr ())
        _fec_params->stop ();

    return ret;
}

XCamReturn
FecProcUint::configParams(SmartPtr<RkAiqFecParamsProxy>& fecParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (_fec_params.ptr ()) {
        ret = _fec_params->configParams (fecParams);
        if (ret < 0)
            LOGE_CAMHW_SUBM(FECPROC_SUBM, "failed to config fec params!\n");
    }

    return ret;
}

bool
FecProcUint::set_params_translator (Isp20Params* translator)
{
	XCAM_ASSERT (translator);
    if (_fec_params.ptr ())
        _fec_params->set_params_translator (translator);

	return true;
}


/*
 * The following is responsible for fec parameters configuration
 */

FecParamsUint::FecParamsUint()
    : _fec_params_dev (nullptr)
    , _poll_thread (nullptr)
    , _translator (nullptr)
{
}

FecParamsUint::~FecParamsUint()
{
}

XCamReturn
FecParamsUint::init (const rk_sensor_full_info_t *s_info)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (_fec_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

	_fec_params_dev = new V4l2Device (s_info->ispp_info->pp_fec_params_path);
    if (!_fec_params_dev->is_opened()) {
        ret = _fec_params_dev->open ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(FECPROC_SUBM, "failed to open fec param device: %s!\n",
                    s_info->ispp_info->pp_fec_params_path);
            return ret;
        }
    }

	return ret;
}

XCamReturn
FecParamsUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_fec_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_fec_params_dev->is_opened()) {
        ret = _fec_params_dev->close ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(FECPROC_SUBM, "failed to close fec param device!\n");
            return ret;
        }
    }

    return ret;
}

XCamReturn
FecParamsUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
FecParamsUint::start ()
{
	if (!_fec_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (!_fec_params_dev->is_activated ())
        ret = _fec_params_dev->start ();
        if (ret < 0)
            LOGE_CAMHW_SUBM(FECPROC_SUBM, "Failed to start fec params dev\n");

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
FecParamsUint::stop ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_fec_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_fec_params_dev->is_activated ())
        _fec_params_dev->stop ();

    return ret;
}

bool
FecParamsUint::set_params_translator (Isp20Params* translator)
{
    XCAM_ASSERT (!_translator);
    _translator = translator;
    return true;
}

bool
FecParamsUint::set_poll_thread (SmartPtr<PollThread> thread)
{
    XCAM_ASSERT (thread.ptr () && !_poll_thread.ptr ());
    _poll_thread = thread;

    if (_poll_thread.ptr ())
        _poll_thread->set_fec_params_device (_fec_params_dev);
    return true;
}

XCamReturn
FecParamsUint::configParams (SmartPtr<RkAiqFecParamsProxy>& aiqFecParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (!_fec_params_dev->is_activated ()) {
        ret = _fec_params_dev->start ();
        if (ret < 0)
            LOGE_CAMHW_SUBM(FECPROC_SUBM, "Failed to start fec params dev\n");

        LOGW_CAMHW_SUBM(FECPROC_SUBM, "fec params dev isn't started yet\n");
    }

    SmartLock locker (_mutex);
    if (configToDrv (aiqFecParams) < 0)
        LOGW_CAMHW_SUBM(FECPROC_SUBM, "Failed to config fec params to driver!\n");

    return ret;
}

XCamReturn
FecParamsUint::configToDrv (SmartPtr<RkAiqFecParamsProxy>& aiqFecParams)
{
    if (!_fec_params_dev.ptr ())
        return  XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;


    SmartPtr<V4l2Buffer> v4l2buf                 = nullptr;
    struct rkispp_params_feccfg *fecParams       = nullptr;

    ret = _fec_params_dev->get_buffer(v4l2buf);
    if (ret) {
        LOGW_CAMHW_SUBM(FECPROC_SUBM, "Can not get fec params buffer\n");
        return XCAM_RETURN_ERROR_PARAM;
    }

    fecParams = (struct rkispp_params_feccfg*)v4l2buf->get_buf().m.userptr;
    if (aiqFecParams->data()->update_mask & RKAIQ_ISPP_FEC_ID)
        _translator->convertAiqFecToIsp20Params(*fecParams, aiqFecParams);

    if (memcmp(&_last_fec_params, fecParams, sizeof(_last_fec_params)) == 0) {
        LOGD_CAMHW_SUBM(FECPROC_SUBM, "fec: no need update !");
        ret = XCAM_RETURN_NO_ERROR;
        goto ret_fec_buf;
    }

    fecParams->head.frame_id = aiqFecParams->getId();

    fecParams->head.module_en_update = fecParams->head.module_ens ^ _last_fec_params.head.module_ens;

    if (1/* (tnrParams->head.module_cfg_update) || (tnrParams->head.module_en_update) */) {
        if (_fec_params_dev->queue_buffer (v4l2buf) != 0) {
            LOGD_CAMHW_SUBM(FECPROC_SUBM, "RKISP1: fec: failed to ioctl VIDIOC_QBUF for index %d, %d %s.\n",
                    v4l2buf->get_buf().index, errno, strerror(errno));
            goto ret_fec_buf;
        }

        LOGD_CAMHW_SUBM(FECPROC_SUBM, "device(%s) id %d queue buffer index %d, queue cnt %d",
                XCAM_STR (_fec_params_dev->get_device_name()),
                fecParams->head.frame_id,
                v4l2buf->get_buf().index,
                _fec_params_dev->get_queued_bufcnt());

        _last_fec_params = *fecParams;
    } else {
        goto ret_fec_buf;
    }

    return XCAM_RETURN_NO_ERROR;

ret_fec_buf:
    if (v4l2buf.ptr())
        _fec_params_dev->return_buffer_to_pool (v4l2buf);
    return ret;
}

};
