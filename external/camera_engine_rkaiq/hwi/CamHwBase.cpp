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

#include "CamHwBase.h"
#include "SensorHw.h"
#include "LensHw.h"

namespace RkCam {

CamHwBase::CamHwBase()
    : mKpHwSt (false)
{}

CamHwBase::~CamHwBase()
{}

XCamReturn
CamHwBase::init(const char* sns_ent_name)
{
    // TODO: new all subdevices and open
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::deInit()
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::prepare(uint32_t width, uint32_t height, int mode, int t_delay, int g_delay)
{
    // TODO
    // check sensor's output width,height
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::start()
{
    // TODO
    // subscribe events
    // start devices
    // start pollthread
    mPollthread->start();
    mPollLumathread->start();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::stop()
{
    // TODO
    // unsubscribe events
    // stop pollthread
    // stop devices
    mPollthread->stop();
    mPollLumathread->stop();

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn CamHwBase::pause()
{
    return XCAM_RETURN_NO_ERROR;
}
XCamReturn CamHwBase::resume()
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::getSensorModeData(const char* sns_ent_name,
                             rk_aiq_exposure_sensor_descriptor& sns_des)
{
    // TODO
    // get from SensorHw

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIspMeasParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispMeasParams)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIspOtherParams(SmartPtr<RkAiqIspOtherParamsProxy>& ispOtherParams)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setExposureParams(SmartPtr<RkAiqExpParamsProxy>& expPar)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIrisParams(SmartPtr<RkAiqIrisParamsProxy>& irisPar, CalibDb_IrisType_t irisType)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setFocusParams(SmartPtr<RkAiqFocusParamsProxy>& focus_params)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIsppMeasParams(SmartPtr<RkAiqIsppMeasParamsProxy>& isppParams)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIsppOtherParams(SmartPtr<RkAiqIsppOtherParamsProxy>& isppOtherParams)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIspLumaListener(IspLumaListener* lumaListener)
{
    mIspLumaListener = lumaListener;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIsppStatsListener(IsppStatsListener* isppStatsListener)
{
    mIsppStatsListener = isppStatsListener;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIspStatsListener(IspStatsListener* statsListener)
{
    mIspStatsLintener = statsListener;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setEvtsListener(IspEvtsListener* evtListener)
{
    mIspEvtsListener = evtListener;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setIspTxBufListener(IspTxBufListener* txBufListener)
{
    mIspTxBufLintener = txBufListener;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::setNrStatsListener(nrStatsListener* nrStatsListener)
{
    mNrStatsLintener = nrStatsListener;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::poll_buffer_ready (SmartPtr<VideoBuffer> &buf, int type)
{
    if ((type == ISP_POLL_3A_STATS || type == ISPP_POLL_STATS) && mIspStatsLintener) {
        return mIspStatsLintener->ispStatsCb(buf);
    }

    if (type == ISP_POLL_LUMA && mIspLumaListener) {
        return mIspLumaListener->ispLumaCb(buf);
    }

    if (type == ISP_POLL_TX_BUF && mIspTxBufLintener) {
        return mIspTxBufLintener->ispTxBufCb(buf);
    }

    if (type == ISPP_POLL_NR_STATS && mNrStatsLintener) {
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s, id(%d), nr stats",
                        __func__,
                        buf->get_sequence());
        return mNrStatsLintener->nrStatsCb(buf);
    }

    if (type == ISPP_POLL_TNR_STATS) {
        const SmartPtr<V4l2BufferProxy> tnrStatsBufProxy = buf.dynamic_cast_ptr<V4l2BufferProxy>();
        rk_aiq_tnr_buf_info_t *tnr_buf_info = (rk_aiq_tnr_buf_info_t *)tnrStatsBufProxy->get_private_info ();
        if (tnr_buf_info) {
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s, id(%d), rect: %dx%d, kg fd: %d, index: %d, size: %d, wr fd: %d, size: %d\n",
                    __func__,
                    tnr_buf_info->frame_id,
                    tnr_buf_info->width,
                    tnr_buf_info->height,
                    tnr_buf_info->kg_buf_fd,
                    tnr_buf_info->wr_buf_index,
                    tnr_buf_info->kg_buf_size,
                    tnr_buf_info->wr_buf_fd,
                    tnr_buf_info->wr_buf_size);
            /* TODO: Transmit to motion dectecion */
            SmartPtr<rk_aiq_tnr_buf_info_t> tnr     = new rk_aiq_tnr_buf_info_t();
            tnr->frame_id                           = tnr_buf_info->frame_id;
            tnr->width                              = tnr_buf_info->width;
            tnr->height                             = tnr_buf_info->height;
            tnr->kg_buf_fd                          = tnr_buf_info->kg_buf_fd;
            tnr->kg_buf_size                        = tnr_buf_info->kg_buf_size;
            tnr->wr_buf_index                       = tnr_buf_info->wr_buf_index;
            tnr->wr_buf_fd                          = tnr_buf_info->wr_buf_fd;
            tnr->wr_buf_size                        = tnr_buf_info->wr_buf_size;
            mIspSpThread->set_tnr_info(tnr);
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM,"tnr info set\n");
            return XCAM_RETURN_NO_ERROR;
        }
        return XCAM_RETURN_ERROR_PARAM;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::poll_buffer_failed (int64_t timestamp, const char *msg)
{
    // TODO
    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn
CamHwBase::poll_event_ready (uint32_t sequence, int type)
{
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
CamHwBase::poll_event_failed (int64_t timestamp, const char *msg)
{
    return XCAM_RETURN_ERROR_FAILED;
}

XCamReturn CamHwBase::setCpslParams(SmartPtr<RkAiqCpslParamsProxy>& cpsl_params)
{
    return XCAM_RETURN_NO_ERROR;
}

}; //namspace RkCam
