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
#include "TnrProcUint.h"

#include <fcntl.h>

#include "rk_aiq_pool.h"

namespace RkCam {

TnrProcUint::TnrProcUint ()
    : _poll_callback (nullptr)
    , _poll_thread (nullptr)
    , _tnr_stats (nullptr)
    , _tnr_params (nullptr)
{
    _tnr_params = new TnrParamsUint();
    if (!_tnr_params.ptr ())
        LOGE_CAMHW_SUBM(TNRPROC_SUBM, "Failed to new TNR params unit!\n");
}

TnrProcUint::~TnrProcUint ()
{
}

XCamReturn
TnrProcUint::init (const rk_sensor_full_info_t *s_info, bool enStats)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== init =======================");

    if (_state != TNR_STATE_INVALID)
        return ret;

    if (!_tnr_params.ptr ())
        return XCAM_RETURN_BYPASS;


    isStatsEn = enStats;
    if (isStatsEn) {
        if (!_tnr_stats.ptr ())
            _tnr_stats = new TnrStatsUint();
        if (!_tnr_stats.ptr ()) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "Failed to new TNR stats unit!\n");
            return XCAM_RETURN_ERROR_MEM;
        }
        ret = _tnr_stats->init (s_info);
        if (ret) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "failed to initialize TNR stats unit!\n");
            return XCAM_RETURN_ERROR_FAILED;
        }
    }

    ret = _tnr_params->init (s_info);
    if (ret) {
        LOGE_CAMHW_SUBM(TNRPROC_SUBM, "failed to initialize TNR params unit!\n");
        return  XCAM_RETURN_ERROR_FAILED;
    }

    if (!_poll_thread.ptr ())
        _poll_thread = new PollThread ();
    if (_poll_thread.ptr ()) {
        _tnr_params->set_poll_thread (_poll_thread);
        if (isStatsEn)
            _tnr_stats->set_poll_thread (_poll_thread);
        _poll_thread->set_poll_callback (this);
    }

    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== inited =======================");
    _state = TNR_STATE_INITED;
    return ret;
}

XCamReturn
TnrProcUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== de-init =======================");

    if (_tnr_stats.ptr ()) {
        _tnr_stats->deInit ();
        _tnr_stats.release();
        _tnr_stats = nullptr;
    }
    if (_tnr_params.ptr ())
        _tnr_params->deInit ();

    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== de-inited =======================");
    _state = TNR_STATE_INVALID;
    return ret;
}

XCamReturn
TnrProcUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
TnrProcUint::start ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== start =======================");
    if (_state != TNR_STATE_INITED && _state != TNR_STATE_STOPPED)
        return ret;

    if (_tnr_stats.ptr ())
        _tnr_stats->start ();
    if (_tnr_params.ptr ())
        _tnr_params->start ();

    if (_poll_thread.ptr () ) {
        ret = _poll_thread->start ();
        if (ret != XCAM_RETURN_NO_ERROR)
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "Failed to start pollthread");
    }

    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== started =======================");
    _state = TNR_STATE_STARTED;
    return ret;
}

XCamReturn
TnrProcUint::stop (bool is_pause)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== stop =======================");
    if (_state != TNR_STATE_STARTED)
        return ret;

    if (_poll_thread.ptr ())
        _poll_thread->stop ();
    if (_tnr_stats.ptr ())
        _tnr_stats->stop (is_pause);
    if (_tnr_params.ptr ())
        _tnr_params->stop ();

    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "=================== stopped =======================");
    _state = TNR_STATE_STOPPED;
    return ret;
}

bool
TnrProcUint::set_poll_callback (PollCallback *callback)
{
	XCAM_ASSERT (!_poll_callback);
	_poll_callback = callback;
	return true;
}

bool
TnrProcUint::set_params_translator (Isp20Params* translator)
{
	XCAM_ASSERT (translator);
    if (_tnr_params.ptr ())
        _tnr_params->set_params_translator (translator);

	return true;
}

XCamReturn
TnrProcUint::configParams(SmartPtr<RkAiqTnrParamsProxy>& tnrParams)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    XCAM_ASSERT (_tnr_params.ptr ());
    ret = _tnr_params->configParams(tnrParams);
    if (ret != XCAM_RETURN_NO_ERROR)
        LOGW_CAMHW_SUBM(TNRPROC_SUBM, "Failed to set TNR params: %d\n");

    return ret;
}

XCamReturn
TnrProcUint::poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type)
{
    SmartPtr<V4l2BufferProxy> tnrStats = buf.dynamic_cast_ptr<V4l2BufferProxy>();
    if (tnrStats.ptr () && !_tnr_stats->fill_buffer_info (tnrStats))
        LOGE_CAMHW_SUBM(TNRPROC_SUBM, "Failed to fill tnr stats info\n");

    XCAM_ASSERT(_poll_callback);
    _poll_callback->poll_buffer_ready(buf, type);
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
TnrProcUint::poll_buffer_failed (int64_t timestamp, const char *msg)
{
    // TODO
    return XCAM_RETURN_BYPASS;
}

bool
TnrProcUint::set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev)
{
    XCAM_ASSERT (dev.ptr () && _tnr_stats.ptr ());
    _tnr_stats->set_ispp_subdevice(dev);
    return true;
}


/*
 * The following is responsible for TNR parameters configuration
 */
TnrParamsUint::TnrParamsUint()
    : _tnr_params_dev (nullptr)
    , _poll_thread (nullptr)
    , _translator (nullptr)
{
}

TnrParamsUint::~TnrParamsUint()
{
}

XCamReturn
TnrParamsUint::init (const rk_sensor_full_info_t *s_info)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	_tnr_params_dev = new V4l2Device (s_info->ispp_info->pp_tnr_params_path);
    if (!_tnr_params_dev->is_opened()) {
        ret = _tnr_params_dev->open ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "failed to open TNR param device: %s!\n",
                    s_info->ispp_info->pp_tnr_params_path);
            return ret;
        }
    }

	return ret;
}

XCamReturn
TnrParamsUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_tnr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_tnr_params_dev->is_opened()) {
        ret = _tnr_params_dev->close ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "failed to close TNR param device: %s!\n",
                    _s_info->ispp_info->pp_tnr_params_path);
            return ret;
        }
    }

    return ret;
}

XCamReturn
TnrParamsUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
TnrParamsUint::start ()
{
    SmartLock locker (_mutex);
	if (!_tnr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (!_tnr_params_dev->is_activated ())
        _tnr_params_dev->start ();

    return ret;
}

bool
TnrParamsUint::set_params_translator (Isp20Params* translator)
{
    XCAM_ASSERT (!_translator);
    _translator = translator;
    return true;
}

bool
TnrParamsUint::set_poll_thread (SmartPtr<PollThread> thread)
{
    XCAM_ASSERT (thread.ptr () && !_poll_thread.ptr ());
    _poll_thread = thread;

    if (_poll_thread.ptr ())
        _poll_thread->set_tnr_params_device (_tnr_params_dev);
    return true;
}

XCamReturn
TnrParamsUint::stop ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    SmartLock locker (_mutex);

	if (!_tnr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_tnr_params_dev->is_activated ())
        _tnr_params_dev->stop ();

    xcam_mem_clear(_last_tnr_params);

    return ret;
}

XCamReturn
TnrParamsUint::configParams(SmartPtr<RkAiqTnrParamsProxy>& tnrParams)
{
    SmartLock locker (_mutex);
	if (!_tnr_params_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (!_tnr_params_dev->is_activated ()) {
        ret = _tnr_params_dev->start ();
        if (ret < 0)
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "Failed to start tnr params dev\n");

        LOGW_CAMHW_SUBM(TNRPROC_SUBM, "tnr params dev isn't started yet\n");
    }

    if (configToDrv (tnrParams) < 0)
        LOGW_CAMHW_SUBM(TNRPROC_SUBM, "Failed to config tnr params to driver!\n");

    return ret;
}

XCamReturn
TnrParamsUint::configToDrv(SmartPtr<RkAiqTnrParamsProxy>& aiqTnrParams)
{
	if (!_tnr_params_dev.ptr ())
		return  XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

   SmartPtr<V4l2Buffer> v4l2buf                 = nullptr;
   struct rkispp_params_tnrcfg *tnrParams       = nullptr;

   ret = _tnr_params_dev->get_buffer(v4l2buf);
   if (ret) {
       LOGW_CAMHW_SUBM(TNRPROC_SUBM, "Can not get tnr params buffer\n");
       return XCAM_RETURN_ERROR_PARAM;
   }

   tnrParams = (struct rkispp_params_tnrcfg*)v4l2buf->get_buf().m.userptr;
   if (aiqTnrParams->data()->update_mask & RKAIQ_ISPP_TNR_ID)
       _translator->convertAiqTnrResultsToTnrParams(*tnrParams, aiqTnrParams);

   if (memcmp(&_last_tnr_params, tnrParams, sizeof(_last_tnr_params)) == 0) {
	   LOGD_CAMHW_SUBM(TNRPROC_SUBM, "tnr: no need update !");
	   ret = XCAM_RETURN_NO_ERROR;
	   goto ret_tnr_buf;
   }

   tnrParams->head.frame_id = aiqTnrParams->getId();

   tnrParams->head.module_en_update = tnrParams->head.module_ens ^ _last_tnr_params.head.module_ens;

   if (1/* (tnrParams->head.module_cfg_update) || (tnrParams->head.module_en_update) */) {
	   if (_tnr_params_dev->queue_buffer (v4l2buf) != 0) {
		   LOGD_CAMHW_SUBM(TNRPROC_SUBM, "RKISP1: tnr: failed to ioctl VIDIOC_QBUF for index %d, %d %s.\n",
				   v4l2buf->get_buf().index, errno, strerror(errno));
		   goto ret_tnr_buf;
	   }
       _last_tnr_params = *tnrParams;

       LOGD_CAMHW_SUBM(TNRPROC_SUBM, "device(%s) id %d queue buffer index %d, queue cnt %d",
               XCAM_STR (_tnr_params_dev->get_device_name()),
               tnrParams->head.frame_id,
               v4l2buf->get_buf().index,
               _tnr_params_dev->get_queued_bufcnt());
   } else {
	   goto ret_tnr_buf;
   }

   return XCAM_RETURN_NO_ERROR;

ret_tnr_buf:
   if (v4l2buf.ptr())
	   _tnr_params_dev->return_buffer_to_pool (v4l2buf);
   return ret;
}


/*
 * The following section is responsible for the processing of TNR statistics
 */
TnrStatsUint::TnrStatsUint ()
    : _tnr_stats_dev (nullptr)
    , _ispp_subdev (nullptr)
    , _poll_thread (nullptr)
    , _s_info (nullptr)
    , _poll_callback (nullptr)
    , _tnr_buf_info{0}
{
}

TnrStatsUint::~TnrStatsUint ()
{
}

XCamReturn
TnrStatsUint::init (const rk_sensor_full_info_t *s_info)
{
	if (_tnr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	_tnr_stats_dev = new V4l2Device (s_info->ispp_info->pp_tnr_stats_path);
    if (!_tnr_stats_dev->is_opened()) {
        ret = _tnr_stats_dev->open ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "failed to open TNR stats device: %s!\n",
                    _s_info->ispp_info->pp_tnr_stats_path);
            return ret;
        }
    }

    // _poll_thread = new PollThread ();
    // _poll_thread->set_capture_device (_tnr_stats_dev);
    // _poll_thread->set_poll_callback (this);
    // _poll_thread->set_poll_stop_fd (_tnr_stats_poll_stop_fd);

	return ret;
}

XCamReturn
TnrStatsUint::deInit ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    {
        std::lock_guard<std::mutex> lg(_tnr_buf_mtx);
        if (_tnr_init_buf_info._fd_init_flag.load()) deinit_tnrbuf();
    }

        if (!_tnr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (_tnr_stats_dev->is_opened()) {
        ret = _tnr_stats_dev->close ();
        if (ret != XCAM_RETURN_NO_ERROR) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "failed to open TNR stats device: %s!\n",
                    _s_info->ispp_info->pp_tnr_stats_path);
        }
    }

    return ret;
}

XCamReturn
TnrStatsUint::prepare ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    return ret;
}

XCamReturn
TnrStatsUint::start ()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_tnr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    if (!_tnr_stats_dev->is_activated ())
        _tnr_stats_dev->start ();

    return ret;
}

XCamReturn
TnrStatsUint::stop (bool is_pause)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

	if (!_tnr_stats_dev.ptr ())
		return XCAM_RETURN_BYPASS;

    _tnr_stats_dev->stop ();

    std::lock_guard<std::mutex> lg(_tnr_buf_mtx);
    if (!is_pause && _tnr_init_buf_info._fd_init_flag.load())
        deinit_tnrbuf ();

    return ret;
}

bool
TnrStatsUint::set_poll_thread (SmartPtr<PollThread> thread)
{
    XCAM_ASSERT (thread.ptr () && !_poll_thread.ptr ());
    _poll_thread = thread;

    if (_poll_thread.ptr ())
        _poll_thread->set_tnr_stats_device (_tnr_stats_dev);
    return true;
}

bool
TnrStatsUint::set_ispp_subdevice (SmartPtr<V4l2SubDevice> &dev)
{
    XCAM_ASSERT (dev.ptr ());
    _ispp_subdev = dev;
    return true;
}

bool
TnrStatsUint::init_tnrbuf()
{
    struct rkispp_buf_idxfd isppbuf_fd;
    int res = -1;

    memset(&isppbuf_fd, 0, sizeof(isppbuf_fd));
    memset(isppbuf_fd.dmafd, -1, sizeof(isppbuf_fd.dmafd));
    res = _ispp_subdev->io_control(RKISPP_CMD_GET_TNRBUF_FD , &isppbuf_fd);
    if (res)
        return false;
    LOGE_CAMHW_SUBM(TNRPROC_SUBM, "tnr buf_num=%d",isppbuf_fd.buf_num);
    for (uint32_t i=0; i<isppbuf_fd.buf_num; i++) {
        if (isppbuf_fd.dmafd[i] < 0) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "tnrbuf_fd[%u]:%d is illegal!",isppbuf_fd.index[i],isppbuf_fd.dmafd[i]);
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
            assert(0);
        }
        _tnr_init_buf_info._fd_array[i] = isppbuf_fd.dmafd[i];
        _tnr_init_buf_info._idx_array[i] = isppbuf_fd.index[i];
        _idx_fd_map[isppbuf_fd.index[i]] = isppbuf_fd.dmafd[i];
        LOGE_CAMHW_SUBM(TNRPROC_SUBM, "tnrbuf_fd[%u]:%d",isppbuf_fd.index[i],isppbuf_fd.dmafd[i]);
    }
    _tnr_init_buf_info._buf_num = isppbuf_fd.buf_num;
    _tnr_init_buf_info._fd_init_flag.store(true);

    return true;
}

void
TnrStatsUint::deinit_tnrbuf()
{
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "%s enter", __FUNCTION__);
    std::map<uint32_t,int>::iterator it;
     for (it=_idx_fd_map.begin(); it!=_idx_fd_map.end(); ++it)
        ::close(it->second);

    _idx_fd_map.clear();
    _tnr_init_buf_info._fd_init_flag.store(false);
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "%s exit", __FUNCTION__);
}

bool
TnrStatsUint::fill_buffer_info (SmartPtr<V4l2BufferProxy> &buf)
{
    {
    std::lock_guard<std::mutex> lg(_tnr_buf_mtx);
    if (!_tnr_init_buf_info._fd_init_flag.load()) {
        if (!init_tnrbuf ()) {
            LOGE_CAMHW_SUBM(TNRPROC_SUBM, "Failed to init tnr buf\n");
        }
        get_buf_format(_tnr_buf_info);
    }
    }

    struct rkispp_stats_tnrbuf *stats = nullptr;
    stats = (struct rkispp_stats_tnrbuf *)(buf->get_v4l2_userptr());
    if (stats) {
        _tnr_buf_info.frame_id = stats->frame_id;
        _tnr_buf_info.kg_buf_fd         = get_fd_by_index(stats->gainkg.index);
        _tnr_buf_info.kg_buf_size       = stats->gainkg.size;
        _tnr_buf_info.wr_buf_index      = stats->gain.index;
        _tnr_buf_info.wr_buf_fd         = get_fd_by_index(stats->gain.index);
        _tnr_buf_info.wr_buf_size       = stats->gain.size;
        LOGD_CAMHW_SUBM(TNRPROC_SUBM, "%s, id(%d), rect: %dx%d, kg idx/fd: %d/%d, size: %d, wr idx/fd: %d/%d, size: %d",
                      __func__,
                      _tnr_buf_info.frame_id,
                      _tnr_buf_info.width,
                      _tnr_buf_info.height,
                      stats->gainkg.index,
                      _tnr_buf_info.kg_buf_fd,
                      _tnr_buf_info.kg_buf_size,
                      stats->gain.index,
                      _tnr_buf_info.wr_buf_fd,
                      _tnr_buf_info.wr_buf_size);
    }

    buf->set_private_info ((uintptr_t)&_tnr_buf_info);
    return true;
}

int
TnrStatsUint::get_fd_by_index(uint32_t index)
{
#if 0
    if (index < 0)
        return -1;
    for (int i=0; i<_buf_num; i++) {
        if (index == (int)_idx_array[i]) {
            return _fd_array[i];
        }
    }
    return -1;
#else
    int fd = -1;
    std::map<uint32_t, int>::iterator it;
    it = _idx_fd_map.find(index);
    if (it == _idx_fd_map.end())
        fd = -1;
    else
        fd = it->second;
    return fd;
#endif
}

XCamReturn
TnrStatsUint::get_buf_format (rk_aiq_tnr_buf_info_t& buf_info)
{
    if (!_ispp_subdev.ptr()) {
        LOGE_CAMHW_SUBM(TNRPROC_SUBM, "_ispp_subdev is null!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    struct v4l2_subdev_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.pad = 0;
    fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    ret = _ispp_subdev->getFormat(fmt);
    if (ret) {
        LOGE_CAMHW_SUBM(TNRPROC_SUBM, "get ispp_dev fmt failed !\n");
        return XCAM_RETURN_ERROR_IOCTL;
    }
    _ispp_fmt = fmt;
    LOGD_CAMHW_SUBM(TNRPROC_SUBM, "ispp fmt info: fmt 0x%x, %dx%d !",
            fmt.format.code, fmt.format.width, fmt.format.height);
    buf_info.width = fmt.format.width;
    buf_info.height = fmt.format.height;

    return ret;
}

};
