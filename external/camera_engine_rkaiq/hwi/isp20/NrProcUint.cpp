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
#include "NrProcUint.h"

#include <fcntl.h>

#include "rk_aiq_pool.h"
#include "rkisp1-config.h"

namespace RkCam {

NrProcUint::NrProcUint ()
    : _poll_thread (nullptr)
    , _poll_callback (nullptr)
    , _nr_params (nullptr)
    , _nr_stats (nullptr)

{
    _nr_stats = new NrStatsUint();
    if (!_nr_stats.ptr ())
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to new NR stats unit!\n");
    _nr_params = new NrParamsUint();
    if (!_nr_params.ptr ())
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to new NR params unit!\n");
}

NrProcUint::~NrProcUint ()
{
}

XCamReturn
NrProcUint::init (const rk_sensor_full_info_t *s_info)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== init =======================");
    if (_state != NR_STATE_INVALID)
        return ret;

    ret = _nr_stats->init(s_info);
    if (ret) {
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "failed to initialize NR stats unit!\n");
        return ret;
    }

    ret = _nr_params->init(s_info);
    if (ret) {
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "failed to initialize NR params unit!\n");
        return ret;
    }

    if (!_poll_thread.ptr ())
        _poll_thread = new PollThread ();
    if (_poll_thread.ptr ()) {
        _nr_params->set_poll_thread (_poll_thread);
        if (_enEis)
            _nr_stats->set_poll_thread (_poll_thread);
        _poll_thread->set_poll_callback (this);
    }

    /* TODO: is the motion detection open? */
    if (1)
        _amdResPool = new RkAiqAmdResPool("amdResultPool", 10);

    _state = NR_STATE_INITED;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== inited =======================");
    return ret;
}

XCamReturn
NrProcUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== de-init =======================");

    if (_nr_stats.ptr ())
        _nr_stats->deInit ();
    if (_nr_params.ptr ())
        _nr_params->deInit ();

    _state = NR_STATE_INVALID;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== de-inited =======================");
    return ret;
}

XCamReturn
NrProcUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    return ret;
}

XCamReturn
NrProcUint::start ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== start =======================");
    if (_state != NR_STATE_INITED && _state != NR_STATE_STOPPED)
        return ret;

    if (_nr_stats.ptr ()) {
        ret = _nr_stats->start ();
        if (ret != XCAM_RETURN_NO_ERROR)
            return ret;
    }
    if (_nr_params.ptr ()) {
        _nr_params->start ();
        if (ret != XCAM_RETURN_NO_ERROR)
            return ret;
    }

    if (_poll_thread.ptr () ) {
        ret = _poll_thread->start ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to start pollthread");
            return ret;
        }
    }
    _state = NR_STATE_STARTED;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== started =======================");
    return ret;
}

XCamReturn
NrProcUint::stop ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== stop =======================");

    if (_state != NR_STATE_STARTED)
        return ret;

    if (_poll_thread.ptr ())
        _poll_thread->stop ();
    if (_nr_stats.ptr ())
        _nr_stats->stop ();
    if (_nr_params.ptr ())
        _nr_params->stop ();

    _state = NR_STATE_STOPPED;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "=================== stoped =======================");
    return ret;
}

bool
NrProcUint::set_poll_thread (SmartPtr<PollThread> thread)
{
    XCAM_ASSERT (thread.ptr () && !_poll_thread.ptr () && _nr_stats.ptr ());
    _nr_stats->set_poll_thread(thread);
    _nr_params->set_poll_thread(thread);
    return true;
}

bool
NrProcUint::set_poll_callback (PollCallback *callback)
{
	XCAM_ASSERT (!_poll_callback);
	_poll_callback = callback;

    if (_poll_callback)
        _nr_stats->set_poll_callback (_poll_callback);

	return true;
}

bool
NrProcUint::set_params_translator (Isp20Params* translator)
{
	XCAM_ASSERT (translator);
    if (_nr_params.ptr ())
        _nr_params->set_params_translator (translator);

	return true;
}

bool
NrProcUint::set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev)
{
    XCAM_ASSERT (dev.ptr () && _nr_stats.ptr ());
    _nr_stats->set_ispp_subdevice(dev);
    return true;
}

XCamReturn
NrProcUint::set_amd_status (bool en)
{
    if (_nr_params.ptr ()) {
        _nr_params->set_amd_status (en);
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
NrProcUint::set_eis_status (bool en)
{

    _enEis = en;

    LOGD_CAMHW_SUBM(NRPROC_SUBM, "eis %s\n", _enEis ? "enable" : "disable");

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn NrProcUint::poll_event_ready(uint32_t sequence, int type) {
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    XCAM_ASSERT(_poll_callback);
    ret = _poll_callback->poll_event_ready(sequence, type);
    return ret;
}

XCamReturn
NrProcUint::poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type)
{
#if 0
    if (_camHw->mHwResLintener) {
        _camHw->mHwResLintener->hwResCb(buf);
        //NR image
        SmartPtr<V4l2BufferProxy> nrstats = buf.dynamic_cast_ptr<V4l2BufferProxy>();
        struct rkispp_stats_nrbuf *stats;
        stats = (struct rkispp_stats_nrbuf *)(nrstats->get_v4l2_userptr());
        struct VideoBufferInfo vbufInfo;
        vbufInfo.init(V4L2_PIX_FMT_NV12, _ispp_fmt.format.width, _ispp_fmt.format.height,
                     _ispp_fmt.format.width, _ispp_fmt.format.height, stats->image.size);
        SmartPtr<VideoBuffer> nrImage = new SubVideoBuffer(
            _buf_num, stats->image.index, get_NRimg_fd_by_index(stats->image.index), vbufInfo);
        nrImage->set_sequence(stats->frame_id);
        nrImage->_buf_type = ISP_NR_IMG;
        _camHw->mHwResLintener->hwResCb(nrImage);
    }
#else
    //NR image
    SmartPtr<V4l2BufferProxy> nrstats = buf.dynamic_cast_ptr<V4l2BufferProxy>();
    /*
     * struct rkispp_stats_nrbuf *stats;
     * stats = (struct rkispp_stats_nrbuf *)(nrstats->get_v4l2_userptr());
     */

    if (nrstats.ptr () && !_nr_stats->fill_buffer_info (nrstats))
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to fill nr stats info\n");

    XCAM_ASSERT(_poll_callback);
    _poll_callback->poll_buffer_ready(buf, type);
#endif

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
NrProcUint::poll_buffer_failed (int64_t timestamp, const char *msg)
{
    // TODO
    return XCAM_RETURN_BYPASS;
}

XCamReturn
NrProcUint::configParams(SmartPtr<RkAiqIsppOtherParamsProxy>& nrParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    XCAM_ASSERT (_nr_params.ptr ());

    LOGV_CAMHW_SUBM(NRPROC_SUBM, "%s:%d NR Param %d\n", __func__, __LINE__, nrParams->getId());
    ret = _nr_params->configParams(nrParams);
    if (ret < 0)
        LOGW_CAMHW_SUBM(NRPROC_SUBM, "Failed to set NR params: %d\n", nrParams->data()->frame_id);

    return ret;
}

XCamReturn
NrProcUint::configAmdResult (rk_aiq_amd_result_t &amdResult)
{
    SmartPtr<RkAiqAmdResProxy> amdResProxy = nullptr;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (_amdResPool->has_free_items()) {
       amdResProxy = _amdResPool->get_item();
       amdResProxy->setId (amdResult.frame_id);
       amdResProxy->setType (RESULT_TYPE_AMD);
       amdResProxy->setProcessType (RKAIQ_ITEM_PROCESS_MANDATORY);
       amdResProxy->data()->frame_id        = amdResult.frame_id;
       amdResProxy->data()->wr_buf_index    = amdResult.wr_buf_index;
       amdResProxy->data()->wr_buf_size     = amdResult.wr_buf_size;
    } else {
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "no free buffer for amd result in pool!\n");
        return XCAM_RETURN_ERROR_MEM;
    }

    LOGV_CAMHW_SUBM(NRPROC_SUBM, "%s:%d AMD Param %d wr %d\n", __func__,
                    __LINE__, amdResult.frame_id, amdResult.wr_buf_index);
    XCAM_ASSERT(amdResProxy.ptr());
    ret = _nr_params->configParams (amdResProxy);
    if (ret != XCAM_RETURN_NO_ERROR)
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to set amd result\n", amdResProxy->data()->frame_id);
    return ret;
}


/*
 * The following is responsible for NR parameters configuration
 */
NrParamsUint::NrParamsUint()
    : _nr_params_dev (nullptr)
    , _translator (nullptr)
    , _poll_thread (nullptr)
{
}

NrParamsUint::~NrParamsUint()
{
}

XCamReturn
NrParamsUint::init (const rk_sensor_full_info_t *s_info)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    xcam_mem_clear (_last_nr_params);

	if (_nr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

	_nr_params_dev = new V4l2Device (s_info->ispp_info->pp_nr_params_path);
    if (!_nr_params_dev->is_opened()) {
        ret = _nr_params_dev->open ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "failed to open NR param device: %s!\n",
                    s_info->ispp_info->pp_nr_params_path);
            return ret;
        }
    }

    mParamsAssembler = new IspParamsAssembler<cam3aResult>("NR_PARAMS_ASSEMBLER");
    mParamsAssembler->addReadyCondition(RESULT_TYPE_ANR);

    // set inital params
    if (mParamsAssembler.ptr()) {
        ret = mParamsAssembler->start();
        if (ret < 0) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "params assembler start err: %d\n", ret);
            return XCAM_RETURN_ERROR_FAILED;
        }
    }

	return ret;
}

XCamReturn
NrParamsUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (mParamsAssembler.ptr())
        mParamsAssembler->stop();

    xcam_mem_clear (_last_nr_params);

	if (!_nr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_nr_params_dev->is_opened()) {
        ret = _nr_params_dev->close ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "failed to close NR param device: %s!\n",
                    _s_info->ispp_info->pp_nr_params_path);
            return ret;
        }
    }

	return ret;
}

XCamReturn
NrParamsUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
NrParamsUint::start ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_nr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (!mParamsAssembler->is_started()) {
        mParamsAssembler->start();
    }

    _nr_params_dev->subscribe_event(CIFISP_V4L2_EVENT_STREAM_START);
    _nr_params_dev->subscribe_event(CIFISP_V4L2_EVENT_STREAM_STOP);

    if (!_nr_params_dev->is_activated ()) {
        _nr_params_dev->start ();
    }

    if (mParamsAssembler->ready()) {
        IspParamsAssembler<cam3aResult>::tList ready_results;
        ret = mParamsAssembler->deQueOne(ready_results, 0);
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGW_CAMHW_SUBM(NRPROC_SUBM, "Can not dq aiq nr params buffer");
            return XCAM_RETURN_ERROR_PARAM;
        }

        {
            SmartLock locker (_mutex);
            ret = configToDrv (ready_results);
        }

        if (ret < 0) {
            mParamsAssembler->queue(ready_results);
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to config nr params to driver!\n");
            return XCAM_RETURN_ERROR_UNKNOWN;
        }
        ready_results.clear();
        LOGD_CAMHW_SUBM(NRPROC_SUBM, "config inital nr params");
    } else {
        LOGW_CAMHW_SUBM(NRPROC_SUBM, "no inital nr params ready");
    }

    if (_enAmd) {
        mParamsAssembler->addReadyCondition(RESULT_TYPE_AMD);
        mParamsAssembler->setCriticalConditions(1 << RESULT_TYPE_AMD);
    }

    return ret;
}

XCamReturn
NrParamsUint::stop ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    xcam_mem_clear (_last_nr_params);

	if (!_nr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_nr_params_dev->is_activated ())
        _nr_params_dev->stop ();

    _nr_params_dev->unsubscribe_event(CIFISP_V4L2_EVENT_STREAM_START);
    _nr_params_dev->unsubscribe_event(CIFISP_V4L2_EVENT_STREAM_STOP);

	_enAmd = false;

    if (mParamsAssembler.ptr()) {
        mParamsAssembler->reset();
        mParamsAssembler->addReadyCondition(RESULT_TYPE_ANR);
    }
    return ret;
}

XCamReturn
NrParamsUint::configParams(SmartPtr<cam3aResult> result)
{

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOGD_CAMHW_SUBM(NRPROC_SUBM, "config nr params, result id %d, type %s\n",
            result->getId (), Cam3aResultType2Str[result->getType()]);

    if (!_nr_params_dev->is_activated ()) {
        ret = mParamsAssembler->queue(result);
        if (ret < 0) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to queue nr params\n");
            return XCAM_RETURN_ERROR_IOCTL;
        }

        return XCAM_RETURN_BYPASS;
    }

    ret = mParamsAssembler->queue (result);
    if (ret < 0)
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to queue nr params\n");

    while (mParamsAssembler->ready()) {
        if (!_nr_params_dev->is_activated ()) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "nr params dev stopped while configuring\n");
            return XCAM_RETURN_ERROR_PARAM;
        }
        IspParamsAssembler<cam3aResult>::tList ready_results;
        ready_results.clear();
        ret = mParamsAssembler->deQueOne(ready_results, result->getId ());
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGW_CAMHW_SUBM(NRPROC_SUBM, "Can not dq aiq nr params buffer");
            return XCAM_RETURN_ERROR_PARAM;
        }

        {
            SmartLock locker (_mutex);
            ret =  configToDrv (ready_results);
        }

        if (ret < 0) {
            mParamsAssembler->queue(ready_results);
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to config nr params to driver!\n");
            ret = XCAM_RETURN_NO_ERROR;
            break;
        }
        ready_results.clear();
    }

    return ret;
}

XCamReturn
NrParamsUint::configToDrv(IspParamsAssembler<cam3aResult>::tList& ready_results)
{
	if (!_nr_params_dev.ptr () || !_nr_params_dev->is_activated ())
		return  XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

   SmartPtr<V4l2Buffer> v4l2buf               = nullptr;
   struct rkispp_params_nrcfg *nrParams       = nullptr;
   SmartPtr<RkAiqIsppOtherParamsProxy> aiqNrParams      = nullptr;
   SmartPtr<RkAiqAmdResProxy> amdResProxy               = nullptr;

   ret = _nr_params_dev->get_buffer(v4l2buf);
   if (ret) {
       LOGE_CAMHW_SUBM(NRPROC_SUBM, "Can not get nr params buffer\n");
       return XCAM_RETURN_ERROR_PARAM;
   }

   LOGD_CAMHW_SUBM(NRPROC_SUBM, "device(%s) get buffer index %d",
                   XCAM_STR(_nr_params_dev->get_device_name()),
                   v4l2buf->get_buf().index);

   nrParams = (struct rkispp_params_nrcfg*)v4l2buf->get_buf().m.userptr;

   for (IspParamsAssembler<cam3aResult>::tList::iterator iter = ready_results.begin ();
           iter != ready_results.end (); iter++)
   {
       SmartPtr<cam3aResult> &cam3a_result = *iter;

       if (cam3a_result->getType() == RESULT_TYPE_ANR) {
           aiqNrParams = cam3a_result.dynamic_cast_ptr<RkAiqIsppOtherParamsProxy>();
           continue;
       }

       if (cam3a_result->getType() == RESULT_TYPE_AMD) {
           amdResProxy = cam3a_result.dynamic_cast_ptr<RkAiqAmdResProxy>();
           continue;
       }
   }

   if (aiqNrParams.ptr () && aiqNrParams->data()->update_mask & RKAIQ_ISPP_NR_ID) {
       _translator->convertAiqNrResultsToNrParams(*nrParams, aiqNrParams);
   } else if (amdResProxy.ptr ()) {
       LOGW_CAMHW_SUBM(NRPROC_SUBM,
                       "Haven't new nr params %d, use last params %d\n",
                       amdResProxy->getId(), _last_nr_params.head.frame_id);
       memcpy(nrParams, &_last_nr_params, sizeof(_last_nr_params));
       nrParams->head.frame_id = amdResProxy->getId();
   } else {
       LOGW_CAMHW_SUBM(NRPROC_SUBM, "Can not get nr params buffer\n");
       goto ret_nr_buf;
   }

   if (amdResProxy.ptr ()) {
       nrParams->gain.index     = amdResProxy->data()->wr_buf_index;
       nrParams->gain.size      = amdResProxy->data()->wr_buf_size;
       nrParams->head.frame_id  = amdResProxy->getId();
   } else {
       nrParams->gain.index     = -1;
       nrParams->gain.size      = -1;
   }

#if 0
   if (memcmp(&_last_nr_params, nrParams, sizeof(_last_nr_params)) == 0) {
	   LOGD_CAMHW_SUBM(NRPROC_SUBM, "nr: no need update !");
	   ret = XCAM_RETURN_NO_ERROR;
	   goto ret_nr_buf;
   }
#endif
   if (!_enAmd && aiqNrParams.ptr()) {
       nrParams->head.frame_id = aiqNrParams->getId();
   } else if (aiqNrParams.ptr() && 0 == aiqNrParams->getId()) {
       nrParams->head.frame_id = aiqNrParams->getId();
   }

   if (nrParams->head.frame_id < _last_nr_params.head.frame_id) {
       LOGV_CAMHW_SUBM(NRPROC_SUBM, "%s:%d config NR %d idx %d en update %x id wrong, DROP IT!\n",
                       __func__, __LINE__, nrParams->head.frame_id, v4l2buf->get_buf().index,
                       nrParams->head.module_en_update);
       goto ret_nr_buf;
   }

   nrParams->head.module_en_update = nrParams->head.module_ens ^ _last_nr_params.head.module_ens;

   LOGV_CAMHW_SUBM(NRPROC_SUBM, "%s:%d config NR %d idx %d en update %x\n", __func__,
                   __LINE__, nrParams->head.frame_id, v4l2buf->get_buf().index, nrParams->head.module_en_update);
   if (1/* (nrParams->head.module_cfg_update) || (nrParams->head.module_en_update) */) {
	   if (_nr_params_dev->queue_buffer (v4l2buf) != 0) {
		   LOGD_CAMHW_SUBM(NRPROC_SUBM, "RKISP1: nr: failed to ioctl VIDIOC_QBUF for index %d, %d %s.\n",
				   v4l2buf->get_buf().index, errno, strerror(errno));
		   goto ret_nr_buf;
	   }
       _last_nr_params = *nrParams;

       LOGD_CAMHW_SUBM(NRPROC_SUBM, "device(%s) id %d queue buffer index %d, queue cnt %d, gain idx %d",
               XCAM_STR (_nr_params_dev->get_device_name()),
               nrParams->head.frame_id,
               v4l2buf->get_buf().index,
               _nr_params_dev->get_queued_bufcnt(),
               nrParams->gain.index);
   } else {
	   goto ret_nr_buf;
   }

   return XCAM_RETURN_NO_ERROR;

ret_nr_buf:
   if (v4l2buf.ptr())
	   _nr_params_dev->return_buffer_to_pool (v4l2buf);
   return ret;
}

bool
NrParamsUint::set_params_translator (Isp20Params* translator)
{
    XCAM_ASSERT (!_translator);
    _translator = translator;
    return true;
}

bool
NrParamsUint::set_poll_thread (SmartPtr<PollThread> thread)
{
    XCAM_ASSERT (thread.ptr () && !_poll_thread.ptr ());
    _poll_thread = thread;

    if (_poll_thread.ptr ()) {
        _poll_thread->set_nr_params_device (_nr_params_dev);
        _poll_thread->set_event_device(_nr_params_dev);
    }
    return true;
}

XCamReturn
NrParamsUint::set_amd_status (bool en)
{
    _enAmd = en;

    if (en) {
        mParamsAssembler->setCriticalConditions(1 << RESULT_TYPE_AMD);
    } else {
        mParamsAssembler->setCriticalConditions(0);
    }

    LOGD_CAMHW_SUBM(NRPROC_SUBM, "amd %s\n", _enAmd ? "enable" : "disable");

    return XCAM_RETURN_NO_ERROR;
}

/*
 * The following section is responsible for the processing of NR statistics
 */
NrStatsUint::NrStatsUint ()
    : _poll_callback (nullptr)
{
}

NrStatsUint::~NrStatsUint ()
{
}

XCamReturn
NrStatsUint::init (const rk_sensor_full_info_t *s_info)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (_nr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

	_nr_stats_dev = new V4l2Device (s_info->ispp_info->pp_nr_stats_path);
    if (!_nr_stats_dev->is_opened()) {
        ret = _nr_stats_dev->open ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "failed to open NR stats device: %s!\n",
                    _s_info->ispp_info->pp_nr_stats_path);
            return ret;
        }
    }

    return ret;
}

XCamReturn
NrStatsUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_nr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_nr_stats_dev->is_opened()) {
        ret = _nr_stats_dev->close ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "failed to open NR stats device: %s!\n",
                    _s_info->ispp_info->pp_nr_stats_path);
        }
    }

    _init_fd.store(false);

    return ret;
}

XCamReturn
NrStatsUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
NrStatsUint::start ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_nr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (!_nr_stats_dev->is_activated ())
        _nr_stats_dev->start ();

    return ret;
}

XCamReturn
NrStatsUint::stop ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_nr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_nr_stats_dev->is_activated ())
        _nr_stats_dev->stop ();

    if (_init_fd.load())
        deinit_nrbuf_fd ();

    return ret;
}

bool
NrStatsUint::init_nrbuf_fd()
{
    struct rkispp_buf_idxfd isppbuf_fd;
    int res = -1;

    memset(&isppbuf_fd, 0, sizeof(isppbuf_fd));
    res = _ispp_subdev->io_control(RKISPP_CMD_GET_NRBUF_FD , &isppbuf_fd);
    if (res) {
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to get nr buf fd, error: %d (%s)\n",
                        strerror(errno));
        return false;
    }
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "%s: buf_num=%d",__FUNCTION__, isppbuf_fd.buf_num);
    _nr_buf_info.buf_cnt = isppbuf_fd.buf_num;
    for (uint32_t i=0; i<isppbuf_fd.buf_num; i++) {
        if (isppbuf_fd.dmafd[i] < 0) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "nrbuf_fd[%u]:%d is illegal!",isppbuf_fd.index[i],isppbuf_fd.dmafd[i]);
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
            assert(0);
        }
        _nr_buf_info.idx_array[i] = isppbuf_fd.index[i];
        _nr_buf_info.fd_array[i] = isppbuf_fd.dmafd[i];
        LOGD_CAMHW_SUBM(NRPROC_SUBM, "nrbuf_fd[%u]:%d",isppbuf_fd.index[i],isppbuf_fd.dmafd[i]);
    }

    _init_fd.store(true);

    return true;
}

bool
NrStatsUint::deinit_nrbuf_fd()
{
    for (int i=0; i<_nr_buf_info.buf_cnt; i++) {
        ::close(_nr_buf_info.fd_array[i]);
    }
    _nr_buf_info.buf_cnt = 0;

    _init_fd.store(false);

    return true;
}

int32_t
NrStatsUint::get_NRimg_fd_by_index(int index)
{
    int ret = -1;

    if (index < 0)
        return ret;
    for (int i=0; i<_nr_buf_info.buf_cnt; i++) {
        if (_nr_buf_info.idx_array[i] == index)
            return _nr_buf_info.fd_array[i];
    }

    return ret;
}


int
NrStatsUint::queue_NRImg_fd(int fd, uint32_t frameid)
{
    if (fd < 0)
        return -1;
    _list_mutex.lock();
    _NrImg_ready_map[frameid] = fd;
    _list_mutex.unlock();

    return 0;
}


int
NrStatsUint::get_NRImg_fd(uint32_t frameid)
{
    int fd = -1;
    std::map<uint32_t, int>::iterator it;
    _list_mutex.lock();
    it = _NrImg_ready_map.find(frameid);
    if (it == _NrImg_ready_map.end())
        fd = -1;
    else
        fd = it->second;
    _list_mutex.unlock();

    return fd;
}

int32_t
NrStatsUint::get_idx_by_fd(int fd)
{
    int ret = -1;

    if (fd < 0)
        return ret;

    for (int i=0; i<_nr_buf_info.buf_cnt; i++) {
        if (_nr_buf_info.fd_array[i] == fd)
            return _nr_buf_info.idx_array[i];
    }

    return ret;
}

bool
NrStatsUint::set_poll_callback (PollCallback *callback)
{
	XCAM_ASSERT (!_poll_callback);
	_poll_callback = callback;
	return true;
}

bool
NrStatsUint::set_poll_thread (SmartPtr<PollThread> thread)
{
    XCAM_ASSERT (thread.ptr () && !_poll_thread.ptr ());
    _poll_thread = thread;

    if (_poll_thread.ptr ())
        _poll_thread->set_nr_stats_device (_nr_stats_dev);
    return true;
}

bool
NrStatsUint::set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev)
{
    XCAM_ASSERT (!_ispp_subdev.ptr ());
    _ispp_subdev = dev;
    return true;
}

bool NrStatsUint::fill_buffer_info (SmartPtr<V4l2BufferProxy> &buf)
{
    if (!_init_fd.load()) {
        if (!init_nrbuf_fd ()) {
            LOGE_CAMHW_SUBM(NRPROC_SUBM, "Failed to init nr buf\n");
        }
        get_buf_format(_nr_buf_info);
    }
    buf->set_private_info ((uintptr_t)&_nr_buf_info);
    return true;
}

XCamReturn
NrStatsUint::get_buf_format (rk_aiq_nr_buf_info_t& buf_info)
{
    if (!_ispp_subdev.ptr()) {
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "_ispp_subdev is null!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    struct v4l2_subdev_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.pad = 0;
    fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = _ispp_subdev->getFormat(fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(NRPROC_SUBM, "get ispp_dev fmt failed !\n");
        return XCAM_RETURN_ERROR_IOCTL;
    }
    _ispp_fmt = fmt;
    LOGD_CAMHW_SUBM(NRPROC_SUBM, "ispp fmt info: fmt 0x%x, %dx%d !",
            fmt.format.code, fmt.format.width, fmt.format.height);
    buf_info.width = fmt.format.width;
    buf_info.height = fmt.format.height;

    return ret;
}

};
