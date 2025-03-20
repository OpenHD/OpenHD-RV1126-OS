/*
 *  Copyright (c) 2021 Rockchip Corporation
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

#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include "Isp20PollThread.h"
#include "Isp20SpThread.h"
#include "motion_detect.h"
#include "rk_aiq_types_af_algo.h"

namespace RkCam {
#define DEBUG_TIMESTAMP                 1
#define RATIO_PP_FLG                    0
#define WRITE_FLG                       0
#define WRITE_FLG_OTHER                 1
int write_frame_num     = 2;
int frame_write_st      = -1;
char name_wr_flg[100] = "/tmp/motion_detection_wr_flg";
char name_wr_other_flg[100] = "/tmp/motion_detection_wr_other_flg";
char path_mt_test[100]      = "/tmp/mt_test/";
static int thread_bind_cpu(int target_cpu)
{
    cpu_set_t mask;
    int cpu_num = sysconf(_SC_NPROCESSORS_CONF);
    int i;

    if (target_cpu >= cpu_num)
        return -1;

    CPU_ZERO(&mask);
    CPU_SET(target_cpu, &mask);

    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "pthread_setaffinity_np");

    if (pthread_getaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "pthread_getaffinity_np");

    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "Thread bound to cpu:");
    for (i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &mask))
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, " %d", i);
    }

    return i >= cpu_num ? -1 : i;
}

XCamReturn
Isp20SpThread::select_motion_params(RKAnr_Mt_Params_Select_t *stmtParamsSelected, uint32_t frameid)
{
    SmartPtr<SensorHw> snsSubdev = _sensor_dev.dynamic_cast_ptr<SensorHw>();
    SmartPtr<RkAiqExpParamsProxy> expParams = nullptr;
    int iso = 50;

    snsSubdev->getEffectiveExpParams(expParams, frameid);
    if (expParams.ptr()) {
        RKAiqAecExpInfo_t* exp_tbl = &expParams->data()->aecExpInfo;

        if(_working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            iso = exp_tbl->LinearExp.exp_real_params.analog_gain * 50;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(_working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
            iso = exp_tbl->HdrExp[1].exp_real_params.analog_gain * 50;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(_working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
            iso = exp_tbl->HdrExp[2].exp_real_params.analog_gain * 50;
        }
    }

    float gain                              = iso / 50;
    float gain_f                            = log2(gain);
    uint8_t gain_l                          = ceil(gain_f);
    uint8_t gain_r                          = floor(gain_f);
    float ratio                             = (float)(gain_f - gain_r);

    float dynamicSw                         = 0.0;

    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "current gain is %f \n",gain);
    if (_motion_params.stMotion.dynamicEnable) {
        if (_motion_detect_control)
            dynamicSw = _motion_params.stMotion.dynamicSw[0];
        else
            dynamicSw = _motion_params.stMotion.dynamicSw[1];
        if (gain > dynamicSw){
            _motion_detect_control = true;
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "dynamicSwitch on\n");
        } else {
            _motion_detect_control = false;
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "dynamicSwitch off\n");
        }
    }
    else {
        _motion_detect_control = true;
    }

    SmartLock locker (_motion_param_mutex);
    stmtParamsSelected->enable              = _motion_params.stMotion.enable;
    stmtParamsSelected->gain_upt_flg        = _motion_params.stMotion.gainUpdFlag;

    if(stmtParamsSelected->enable  == 0)
        stmtParamsSelected->gain_upt_flg    = 0;

    stmtParamsSelected->sigmaHScale         = (_motion_params.stMotion.sigmaHScale       [gain_l] * ratio + _motion_params.stMotion.sigmaHScale       [gain_r] * (1 - ratio));
    stmtParamsSelected->sigmaLScale         = (_motion_params.stMotion.sigmaLScale       [gain_l] * ratio + _motion_params.stMotion.sigmaLScale       [gain_r] * (1 - ratio));
    stmtParamsSelected->uv_weight           = (_motion_params.stMotion.uvWeight          [gain_l] * ratio + _motion_params.stMotion.uvWeight          [gain_r] * (1 - ratio));
    stmtParamsSelected->mfnr_sigma_scale    = (_motion_params.stMotion.mfnrSigmaScale   [gain_l] * ratio + _motion_params.stMotion.mfnrSigmaScale   [gain_r] * (1 - ratio));
    if(stmtParamsSelected->gain_upt_flg == 0)
        stmtParamsSelected->mfnr_sigma_scale = 1.0;
    stmtParamsSelected->mfnr_gain_scale     = (_motion_params.stMotion.mfnrGainScale        [gain_l] * ratio + _motion_params.stMotion.mfnrGainScale        [gain_r] * (1 - ratio));
    stmtParamsSelected->motion_dn_str       = (_motion_params.stMotion.mfnrDnStr        [gain_l] * ratio + _motion_params.stMotion.mfnrDnStr        [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[0] = (_motion_params.stMotion.yuvnrGainScale0   [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale0   [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[1] = (_motion_params.stMotion.yuvnrGainScale1   [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale1   [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[2] = (_motion_params.stMotion.yuvnrGainScale2   [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale2   [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale[3] = (_motion_params.stMotion.yuvnrGainScale3        [gain_l] * ratio + _motion_params.stMotion.yuvnrGainScale3        [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_scale_glb = (_motion_params.stMotion.yuvnrGainGlb        [gain_l] * ratio + _motion_params.stMotion.yuvnrGainGlb        [gain_r] * (1 - ratio));
    stmtParamsSelected->yuvnr_gain_th       = (_motion_params.stMotion.yuvnrGainTH        [gain_l] * ratio + _motion_params.stMotion.yuvnrGainTH        [gain_r] * (1 - ratio));

    stmtParamsSelected->md_sigma_scale      = (_motion_params.stMotion.mdSigmaScale     [gain_l] * ratio + _motion_params.stMotion.mdSigmaScale     [gain_r] * (1 - ratio));
    stmtParamsSelected->frame_limit_y       = (_motion_params.stMotion.frame_limit_y     [gain_l] * ratio + _motion_params.stMotion.frame_limit_y     [gain_r] * (1 - ratio));
    stmtParamsSelected->frame_limit_uv      = (_motion_params.stMotion.frame_limit_uv    [gain_l] * ratio + _motion_params.stMotion.frame_limit_uv    [gain_r] * (1 - ratio));


    if(stmtParamsSelected->mfnr_sigma_scale > 0)
        static_ratio_r_bit = static_ratio_l_bit - (RATIO_BITS_NUM - RATIO_BITS_R_NUM) - ceil(log2(stmtParamsSelected->motion_dn_str));
    else
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "stmtParamsSelected->mfnr_sigma_scale %d is out of range\n", stmtParamsSelected->mfnr_sigma_scale);
    stmtParamsSelected->gain                = gain;
    stmtParamsSelected->gain_ratio          = _motion_params.gain_ratio;
    for(int i = 0; i < 6; i++)
        stmtParamsSelected->noise_curve[i]  = (_motion_params.noise_curve[gain_l][i] * ratio + _motion_params.noise_curve[gain_r][i] * (1 - ratio));


    stmtParamsSelected->threshold[0]        = (_motion_params.stMotion.thresh01[gain_l] * ratio + _motion_params.stMotion.thresh01    [gain_r] * (1 - ratio));
    stmtParamsSelected->threshold[1]        = (_motion_params.stMotion.thresh01[gain_l] * ratio + _motion_params.stMotion.thresh01    [gain_r] * (1 - ratio));
    stmtParamsSelected->threshold[2]        = (_motion_params.stMotion.thresh23[gain_l] * ratio + _motion_params.stMotion.thresh23    [gain_r] * (1 - ratio));
    stmtParamsSelected->threshold[3]        = (_motion_params.stMotion.thresh23[gain_l] * ratio + _motion_params.stMotion.thresh23    [gain_r] * (1 - ratio));

    stmtParamsSelected->med_en              = _motion_params.stMotion.medEn;

    stmtParamsSelected->imgHeight           = img_height_align;
    stmtParamsSelected->imgWidth            = img_width_align;
    stmtParamsSelected->proHeight           = img_height;
    stmtParamsSelected->proWidth            = img_width;
    stmtParamsSelected->imgStride           = img_blk_isp_stride;
    stmtParamsSelected->gainHeight          = gain_height;
    stmtParamsSelected->gainWidth           = gain_width;
    stmtParamsSelected->gainStride          = gain_blk_isp_stride;
    stmtParamsSelected->img_ds_size_y       = img_ds_size_y;
    stmtParamsSelected->img_ds_size_x       = img_ds_size_x;
    stmtParamsSelected->static_ratio_r_bit  = static_ratio_r_bit;


    static int select_num = 0;

    // if((select_num % 300) == 0)
    //     printf("selected enable %d upt_flg %d mfnr_sigma_scale %f\n",stmtParamsSelected->enable, stmtParamsSelected->gain_upt_flg, stmtParamsSelected->mfnr_sigma_scale);
    if((select_num % 600) == 0)
    {
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "selected:iso %d upt %d med_en %d HS %f LS %f uv %f mss %f ygt %f ygs %f, %f, %f, %f mgs %f fly %f flu %f mns %f g %f ygsg %f th %f %f\n", iso, stmtParamsSelected->gain_upt_flg, stmtParamsSelected->med_en,
                        stmtParamsSelected->sigmaHScale, stmtParamsSelected->sigmaLScale, stmtParamsSelected->uv_weight, stmtParamsSelected->mfnr_sigma_scale, stmtParamsSelected->yuvnr_gain_th,
                        stmtParamsSelected->yuvnr_gain_scale[0], stmtParamsSelected->yuvnr_gain_scale[1], stmtParamsSelected->yuvnr_gain_scale[2], stmtParamsSelected->yuvnr_gain_scale[3], stmtParamsSelected->mfnr_gain_scale,
                        mtParamsSelect.frame_limit_y, mtParamsSelect.frame_limit_uv,  stmtParamsSelected->motion_dn_str,  stmtParamsSelected->gain,  stmtParamsSelected->yuvnr_gain_scale_glb,
                        stmtParamsSelected->threshold[0], stmtParamsSelected->threshold[1]);
    }

    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "selected:gain_r %d gain_l:%d iso %d ratio %f, sigmascle:%f, %f, uv:%f, mfnr_sigma_scale:%f, yuvnr_gain_scale:%f, %f, %f, %f, frame limit:%f, %f, %f, noise_curve:%e, %e, %e, %e, %e, r_bit %d\n", gain_r, gain_l, iso,
                    ratio, stmtParamsSelected->sigmaHScale, stmtParamsSelected->sigmaLScale, stmtParamsSelected->uv_weight, stmtParamsSelected->mfnr_sigma_scale,
                    stmtParamsSelected->yuvnr_gain_scale[0], stmtParamsSelected->yuvnr_gain_scale[1], stmtParamsSelected->yuvnr_gain_scale[2], stmtParamsSelected->yuvnr_gain_scale[3],
                    mtParamsSelect.frame_limit_y, mtParamsSelect.frame_limit_uv,  stmtParamsSelected->motion_dn_str, stmtParamsSelected->noise_curve[0],
                    stmtParamsSelected->noise_curve[1], stmtParamsSelected->noise_curve[2], stmtParamsSelected->noise_curve[3], stmtParamsSelected->noise_curve[4], static_ratio_r_bit);
    // printf("select %d %d %d %d %f\n", _motion_params.stMotion.enable, stmtParamsSelected->enable, stmtParamsSelected->gain_upt_flg, static_ratio_r_bit, stmtParamsSelected->motion_dn_str);

    select_num++;
    return XCAM_RETURN_NO_ERROR;
}


int get_wr_flg_func(int framenum, int pp_flg)
{

    if(!pp_flg && framenum == 0)
    {
        if(WRITE_FLG)
        {
            write_frame_num         = 2;
            frame_write_st          = 300;
            printf("%s WRITE_FLG 1\n", __func__);
        }
        else
        {
            write_frame_num         = 0;
            frame_write_st          = -1;
        }
    }

    if(!pp_flg && frame_write_st == -1)
    {
        int fp = -1;
        int write_flg = 0;
        const char *delim   = " ";
        char buffer[16]     = {0};
        char *name          = name_wr_flg;
        if (access(name, F_OK) == 0) {
            printf("%s WRITE_FLG 21\n", __func__);
            fp = open(name, O_RDONLY | O_SYNC);
            printf("%s access ! framenum %d\n", __func__, framenum);
            if (read(fp, buffer, sizeof(buffer)) <= 0) {
                printf("%s read %s fail! empty\n", __func__, name);
                write_frame_num             = 0;
                write_flg                   = 0;
                remove(name);
            } else {
                char *p = NULL;
                p = strtok(buffer, delim);
                if (p != NULL) {
                    int value = atoi(p);
                    if(value < 0 || value > 100)
                    {
                        printf("%s read framenum %d failed! framenum %d \n", __func__, value, framenum);
                        write_flg           = 0;
                        write_frame_num     = 0;
                        remove(name);
                    }
                    else
                    {
                        printf("%s read  success value %d !framenum %d\n", __func__, value, framenum);
                        write_flg           = 1;
                        write_frame_num     = value;
                    }
                }
            }
            close(fp);
        }
        else
        {
            write_flg                       = 0;
            write_frame_num                 = 0;
        }
        if(write_flg)
            frame_write_st                  = framenum + 2;
    }
    int write_flg_cur;
    if((frame_write_st != -1) && ((framenum >= frame_write_st) && (framenum < frame_write_st + write_frame_num)))
        write_flg_cur                       = 1;
    else
        write_flg_cur                       = 0;

    if(access(path_mt_test, F_OK) == -1)
    {
        if (mkdir(path_mt_test, 0755) < 0)
        {
            write_flg_cur = 0;
        }
        printf("Isp20SpThread create dir %s\n", path_mt_test);
    }

    return write_flg_cur;
}
void set_wr_flg_func(int framenum)
{
    char *name = name_wr_flg;
    if((frame_write_st != -1) && (framenum > frame_write_st + write_frame_num))
    {
        if (access(name, F_OK) == 0)
        {
            printf("%s remove /tmp/motion_detection_wr_flg name %s frame_write_st %d write_frame_num %d framenum %d\n", __func__, name, frame_write_st, write_frame_num, framenum);
            remove(name);
            frame_write_st              = -1;
            write_frame_num             = 0;
        }

    }
}


int get_wr_other_flg_func()
{

    int write_other_flg = 0;
    char *name          = name_wr_other_flg;
    if (access(name, F_OK) == 0)
    {

        write_other_flg                 = 1;
    }
    else
    {
        write_other_flg                 = 0;
    }

    return write_other_flg;
}
Isp20SpThread::Isp20SpThread ()
    : Thread("motionDetectThread")
{
    mKgThread = new KgProcThread(this);
    mWrThread = new WrProcThread(this);
    mWrThread2 = new WrProcThread2(this);
    _working_mode = RK_AIQ_WORKING_MODE_NORMAL;
}

Isp20SpThread::~Isp20SpThread()
{
}

void
Isp20SpThread::set_calibDb(const CamCalibDbContext_t* calib) {
    _calibDb = calib;
}

void
Isp20SpThread::get_calibDb_thumbs_ds_size(unsigned int *ds_width, unsigned int *ds_height) {
    img_ds_size_x = 4;
    img_ds_size_y = 8;

    if(_calibDb != NULL) {
        img_ds_size_x = _calibDb->mfnr.motion_detect_ds_size[0];
        img_ds_size_y = _calibDb->mfnr.motion_detect_ds_size[1];
        printf("oyyf calib ds_size: %d %d\n",
               _calibDb->mfnr.motion_detect_ds_size[0],
               _calibDb->mfnr.motion_detect_ds_size[1]);
    }

    if(img_ds_size_x != 0 && img_ds_size_y != 0) {
        *ds_width = img_ds_size_x;
        *ds_height = img_ds_size_y;
    } else {
        *ds_width = 4;
        *ds_height = 8;
    }
}

void
Isp20SpThread::start()
{
    SmartPtr<LensHw> lensHw = _focus_dev.dynamic_cast_ptr<LensHw>();

    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s", RK_AIQ_MOTION_DETECTION_VERSION);
    init();
    if (create_stop_fds_ispsp()) {
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM,  "create ispsp stop fds failed !");
        return;
    }

    xcam_mem_clear(_lens_des);
    if (lensHw.ptr())
        lensHw->getLensModeData(_lens_des);

    pthread_attr_t &attr = get_pthread_attr();
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    struct sched_param  param;
    param.sched_priority = 99;
    pthread_attr_setschedparam(&attr, &param);

    mKgThread->start();
    mWrThread->start();
    mWrThread2->start();
    Thread::start();
    struct rkispp_trigger_mode tnr_trigger;
    tnr_trigger.module = ISPP_MODULE_TNR;
    tnr_trigger.on = 1;
    int ret = _ispp_dev->io_control(RKISPP_CMD_TRIGGER_MODE, &tnr_trigger);
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "start tnr process,ret=%d", ret);
}

void
Isp20SpThread::stop()
{
    struct rkispp_trigger_mode tnr_trigger;
    tnr_trigger.module = ISPP_MODULE_TNR;
    tnr_trigger.on = 0;
    int ret = _ispp_dev->io_control(RKISPP_CMD_TRIGGER_MODE, &tnr_trigger);
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "stop tnr process,ret=%d", ret);
    usleep(300000);
    notify_stop_fds_exit();
    Thread::stop();
    _kgWrList.push(SmartPtr<rk_aiq_tnr_buf_info_t>(NULL));
    mKgThread->stop();
    notify_wr_thread_exit();
    notify_wr2_thread_exit();
    mWrThread->stop();
    mWrThread2->stop();
    destroy_stop_fds_ispsp();
    deinit();
    for (int i = 0; i < _isp_buf_num.load(); i++)
        ::close(_isp_fd_array[i]);
    _isp_buf_num.store(0);
}

void
Isp20SpThread::pause()
{
}

void
Isp20SpThread::resume()
{
}

bool Isp20SpThread::init_fbcbuf_fd()
{
    struct isp2x_buf_idxfd ispbuf_fd;
    int res = -1;

    memset(&ispbuf_fd, 0, sizeof(ispbuf_fd));
    res = _isp_dev->io_control(RKISP_CMD_GET_FBCBUF_FD, &ispbuf_fd);
    if (res)
        return false;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "ispbuf_num=%d", ispbuf_fd.buf_num);
    for (uint32_t i = 0; i < ispbuf_fd.buf_num; i++) {
        if (ispbuf_fd.dmafd[i] < 0) {
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "fbcbuf_fd[%u]:%d is illegal!", ispbuf_fd.index[i], ispbuf_fd.dmafd[i]);
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
            assert(0);
        }
        _isp_fd_array[i] = ispbuf_fd.dmafd[i];
        _isp_idx_array[i] = ispbuf_fd.index[i];
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "fbcbuf_fd[%u]:%d", ispbuf_fd.index[i], ispbuf_fd.dmafd[i]);
    }
    _isp_buf_num.store(ispbuf_fd.buf_num);
    return true;
}

XCamReturn
Isp20SpThread::kg_proc_loop ()
{
    SmartPtr<rk_aiq_tnr_buf_info_t> tnr_inf = new rk_aiq_tnr_buf_info_t();
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);

    tnr_inf = _kgWrList.pop(-1);
    if (tnr_inf.ptr()) {
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s: id(%d), rect: %dx%d, kg fd: %d, size: %d, wr_idx: %d, wr fd: %d, size: %d\n",
               __func__,
               tnr_inf->frame_id,
               tnr_inf->width,
               tnr_inf->height,
               tnr_inf->kg_buf_fd,
               tnr_inf->kg_buf_size,
               tnr_inf->wr_buf_index,
               tnr_inf->wr_buf_fd,
               tnr_inf->wr_buf_size);
    } else {
        return XCAM_RETURN_ERROR_UNKNOWN;
    }

    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "kg_loop frame_num_pp %d flg %d\n", frame_num_pp, frame_detect_flg[static_ratio_idx_out]);

    if (frame_detect_flg[static_ratio_idx_out] && _calibDb->mfnr.enable && _calibDb->mfnr.motion_detect_en && _motion_detect_control)
    {
        _motion_detect_control_proc = true;
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "detect ON in kg_loop\n");
        SmartPtr<sp_msg_t> msg = new sp_msg_t();
        msg->cmd = MSG_CMD_WR_START;
        msg->sync = false;
        msg->arg1 = static_ratio_idx_out;
        msg->arg2 = tnr_inf;
        notify_yg_cmd(msg);
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "send MSG_CMD_WR_START,frameid=%u", tnr_inf->frame_id);
    } else {
        if (_nr_dev.ptr()) {
            rk_aiq_amd_result_t amdResult;
            amdResult.frame_id = tnr_inf->frame_id;
            amdResult.wr_buf_index = tnr_inf->wr_buf_index;
            amdResult.wr_buf_size = tnr_inf->wr_buf_size;
            _nr_dev->configAmdResult(amdResult);
        }
        _motion_detect_control_proc = false;
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "MOTION_detect OFF in kg_loop\n");
    }
    do {
        _buf_list_mutex.lock();
        if (_isp_buf_list.empty()) {
            _buf_list_mutex.unlock();
            usleep(1000);
            if (!is_running())
                break;
        } else {
            _buf_list_mutex.unlock();
            if (frame_num_isp <= frame_num_pp) {
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "frame_num_isp(%d) should be greater than frame_num_pp(%d)!", frame_num_isp, frame_num_pp);
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                assert(0);
            }

            if(frame_detect_flg[static_ratio_idx_out] && _motion_detect_control_proc)
            {
                uint8_t* ratio                  = static_ratio[static_ratio_idx_out];
                uint8_t* ratio_next                             = static_ratio[(static_ratio_idx_out + 1) % static_ratio_num];
                uint8_t *gain_isp_buf_cur                       = gain_isp_buf_bak[static_ratio_idx_out];

                RKAnr_Mt_Params_Select_t mtParamsSelect_cur     = *(mtParamsSelect_list[static_ratio_idx_out]);
                RKAnr_Mt_Params_Select_t mtParamsSelect_last    = *(mtParamsSelect_list[(static_ratio_idx_out + static_ratio_num - 1) % static_ratio_num]);
#if DEBUG_TIMESTAMP
                struct timeval tv0, tv1, tv2, tv3;
                gettimeofday(&tv0, NULL);
#endif
                void *gainkg_addr = mmap(NULL, tnr_inf->kg_buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, tnr_inf->kg_buf_fd, 0);
                if (MAP_FAILED == gainkg_addr) {
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "mmap gainkg_fd failed!!!(errno=%d),fd: %d, size: %d", errno, tnr_inf->kg_buf_fd, tnr_inf->kg_buf_size);
                    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                    assert(0);
                }

#if DEBUG_TIMESTAMP
                gettimeofday(&tv1, NULL);
#endif
                set_gainkg(gainkg_addr,     ratio, ratio_next, gain_isp_buf_cur, mtParamsSelect_cur, mtParamsSelect_last);
#if DEBUG_TIMESTAMP
                gettimeofday(&tv2, NULL);
#endif
                munmap(gainkg_addr, tnr_inf->kg_buf_size);
#if DEBUG_TIMESTAMP
                gettimeofday(&tv3, NULL);
#endif
#if DEBUG_TIMESTAMP
                static int num = 0;
                num++;
                if((num % 1) == 0)
                    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_kg idx %d fid %u %8ld %8ld %8ld %8ld  delta %8ld %8ld %8ld %8ld \n", static_ratio_idx_out, tnr_inf->frame_id,
                                    tv0.tv_usec, tv1.tv_usec, tv2.tv_usec, tv3.tv_usec,  tv1.tv_usec - tv0.tv_usec,
                                    tv2.tv_usec - tv1.tv_usec, tv3.tv_usec - tv2.tv_usec, tv3.tv_usec - tv0.tv_usec   );
#endif
            }
            set_wr_flg_func(frame_num_pp);
            static_ratio_idx_out++;
            static_ratio_idx_out    %= static_ratio_num;
            frame_id_pp_upt         = tnr_inf->frame_id;
            frame_num_pp++;

            _buf_list_mutex.lock();
            LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "v4l2buf index %d pop list\n", _isp_buf_list.front()->get_v4l2_buf_index());
            _isp_buf_list.pop_front();//feed new frame to tnr
            _buf_list_mutex.unlock();
            break;
        }
    } while (1);

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
    return XCAM_RETURN_NO_ERROR;
}

bool
Isp20SpThread::wr_proc_loop ()
{
    SmartPtr<sp_msg_t> msg;
    void* gainwr_addr = NULL;
    uint32_t ratio_idx;
    SmartPtr<rk_aiq_tnr_buf_info_t> tnr_inf;
    struct timeval tv0, tv1, tv2, tv3, tv4, tv5, tv6;
    bool loop_live = true;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    while (loop_live) {
        msg = _notifyYgCmdQ.pop(-1);
        if (!msg.ptr())
            continue;
        switch(msg->cmd)
        {
        case MSG_CMD_WR_START:
        {
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "MSG_CMD_WR_START received");
            ratio_idx = msg->arg1;
            tnr_inf = msg->arg2;
            LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s, id(%d), rect: %dx%d, kg fd: %d, size: %d, wr_idx: %d, wr fd: %d, size: %d\n",
                   __func__,
                   tnr_inf->frame_id,
                   tnr_inf->width,
                   tnr_inf->height,
                   tnr_inf->kg_buf_fd,
                   tnr_inf->kg_buf_size,
                   tnr_inf->wr_buf_index,
                   tnr_inf->wr_buf_fd,
                   tnr_inf->wr_buf_size);
            if (tnr_inf->frame_id == 0 || tnr_inf->wr_buf_fd < 0 || tnr_inf->kg_buf_fd < 0) {
                rk_aiq_amd_result_t amdResult;
                amdResult.frame_id = tnr_inf->frame_id;
                amdResult.wr_buf_index = tnr_inf->wr_buf_index;
                amdResult.wr_buf_size = tnr_inf->wr_buf_size;
                _nr_dev->configAmdResult(amdResult);
                break;
            }
#if DEBUG_TIMESTAMP
            gettimeofday(&tv0, NULL);
#endif
            gainwr_addr = mmap(NULL, tnr_inf->wr_buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, tnr_inf->wr_buf_fd, 0);
            if (MAP_FAILED == gainwr_addr) {
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "mmap gainwr_fd failed!!!(errno=%d),fd: %d, size: %d", errno, tnr_inf->wr_buf_fd, tnr_inf->wr_buf_size);
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                assert(0);
            }

#if DEBUG_TIMESTAMP
            gettimeofday(&tv1, NULL);
#endif
            if (static_ratio[ratio_idx] == NULL) {
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "ratio_idx=%d", ratio_idx);
                LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
                assert(0);
            }

            {
                //wake up wr thread2 to proc half gain buffer
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "send MSG_CMD_WR_START2");
                SmartPtr<sp_msg_t> msg = new sp_msg_t();
                msg->cmd = MSG_CMD_WR_START;
                msg->sync = false;
                msg->arg1 = ratio_idx;
                msg->arg3 = gainwr_addr;
                notify_yg2_cmd(msg);
            }

            set_gain_wr(gainwr_addr,    static_ratio[ratio_idx], gain_isp_buf_bak[ratio_idx], *(mtParamsSelect_list[ratio_idx]), 0, gain_blk_ispp_h / 2);
#if DEBUG_TIMESTAMP
            gettimeofday(&tv2, NULL);
#endif


            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr top done");
            char ch;
            read(sync_pipe_fd[0], &ch, 1);//blocked, wait for wr2 thread process compelete!


            munmap(gainwr_addr, tnr_inf->wr_buf_size);
#if DEBUG_TIMESTAMP
            gettimeofday(&tv3, NULL);
#endif
            //_ispp_dev->io_control(RKISPP_CMD_TRIGGER_YNRRUN, tnr_info);
            //_nr_dev->configAmdResult(amdResult);
            if (_nr_dev.ptr ()) {
                rk_aiq_amd_result_t amdResult;
                amdResult.frame_id = tnr_inf->frame_id;
                amdResult.wr_buf_index = tnr_inf->wr_buf_index;
                amdResult.wr_buf_size = tnr_inf->wr_buf_size;
                _nr_dev->configAmdResult (amdResult);
            }
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM," WR config Result back to AIQ!! tnr_inf->frame_id = %d\n", tnr_inf->frame_id);
#if DEBUG_TIMESTAMP
            gettimeofday(&tv4, NULL);
#endif


#if DEBUG_TIMESTAMP
            static int num = 0;
            num++;
            if((num %1) == 0)
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr %8ld %8ld %8ld %8ld %8ld delta %8ld %8ld %8ld %8ld %8ld\n",
                                tv0.tv_usec, tv1.tv_usec, tv2.tv_usec, tv3.tv_usec, tv4.tv_usec, tv1.tv_usec - tv0.tv_usec,
                                tv2.tv_usec - tv1.tv_usec, tv3.tv_usec - tv2.tv_usec, tv4.tv_usec - tv3.tv_usec, tv4.tv_usec - tv0.tv_usec  );
#endif
            break;
        }
        case MSG_CMD_WR_EXIT:
        {
            if (msg->sync) {
                msg->mutex->lock();
                msg->cond->broadcast ();
                msg->mutex->unlock();
            }
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s: wr_proc_loop exit", __FUNCTION__);
            loop_live = false;
            break;
        }
        }
    }
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
    return false;
}

int Isp20SpThread::get_lowpass_fv(uint32_t sequence, SmartPtr<V4l2BufferProxy> buf_proxy)
{
    SmartPtr<LensHw> lensHw = _focus_dev.dynamic_cast_ptr<LensHw>();
    uint8_t *image_buf = (uint8_t *)buf_proxy->get_v4l2_planar_userptr(0);
    rk_aiq_af_algo_meas_t meas_param;

    _afmeas_param_mutex.lock();
    meas_param = _af_meas_params;
    _afmeas_param_mutex.unlock();

    if (meas_param.sp_meas.enable) {
        meas_param.wina_h_offs /= img_ds_size_x;
        meas_param.wina_v_offs /= img_ds_size_y;
        meas_param.wina_h_size /= img_ds_size_x;
        meas_param.wina_v_size /= img_ds_size_y;
        get_lpfv(sequence, image_buf, af_img_width, af_img_height,
                 af_img_width_align, af_img_height_align, pAfTmp, sub_shp4_4,
                 sub_shp8_8, high_light, high_light2, &meas_param);

        lensHw->setLowPassFv(sub_shp4_4, sub_shp8_8, high_light, high_light2, sequence);
    }

    return 0;
}


bool
Isp20SpThread::wr_proc_loop2 ()
{
    SmartPtr<sp_msg_t> msg;
    void *gainwr_addr = NULL;
    uint32_t ratio_idx;
    struct timeval tv0, tv1;
    bool loop_live = true;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    while (loop_live) {
        msg = _notifyYgCmdQ2.pop(-1);
        if (!msg.ptr())
            continue;
        switch(msg->cmd)
        {
        case MSG_CMD_WR_START:
        {
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "MSG_CMD_WR_START2 received");
            ratio_idx = msg->arg1;
            gainwr_addr = msg->arg3;
            uint8_t* ratio                              = static_ratio[ratio_idx];
#if DEBUG_TIMESTAMP
            gettimeofday(&tv0, NULL);
#endif
            //set_gain_wr(gainwr_addr,    ratio, gain_isp_buf_cur, 0,                    gain_blk_ispp_h / 2);
            //set_gain_wr(gainwr_addr,    ratio, gain_isp_buf_cur, gain_blk_ispp_h / 2,  gain_blk_ispp_h);

            set_gain_wr(gainwr_addr,    static_ratio[ratio_idx], gain_isp_buf_bak[ratio_idx], *(mtParamsSelect_list[ratio_idx]), gain_blk_ispp_h / 2, gain_blk_ispp_h);
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr bottom done");
#if DEBUG_TIMESTAMP
            gettimeofday(&tv1, NULL);
#endif
#if DEBUG_TIMESTAMP
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr2 %8ld \n", tv1.tv_usec - tv0.tv_usec);
#endif
            char ch = 0x1;//whatever
            write(sync_pipe_fd[1], &ch, 1);//nonblock, inform wr thread that wr2 thread process done!
            break;
        }
        case MSG_CMD_WR_EXIT:
        {
            if (msg->sync) {
                msg->mutex->lock();
                msg->cond->broadcast ();
                msg->mutex->unlock();
            }
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s: wr_proc_loop2 exit", __FUNCTION__);
            loop_live = false;
            break;
        }
        }
    }
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
    return false;
}


bool
Isp20SpThread::loop () {
    SmartPtr<V4l2Buffer> buf;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    int poll_ret;
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    if (_fd_init_flag.load() && !_isp_buf_num.load()) {
        if (!init_fbcbuf_fd()) {
            usleep(1000);
            return true;
        }
        _fd_init_flag.store(false);
        thread_bind_cpu(3);
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "isp&ispp buf fd init success!");
    }

    poll_ret = _isp_sp_dev->poll_event(1000, ispsp_stop_fds[0]);
    if (poll_ret == POLL_STOP_RET) {
        LOGW_CAMHW_SUBM(MOTIONDETECT_SUBM, "poll isp sp stop sucess !");
        // stop success, return error to stop the poll thread
        return false;
    }

    if (poll_ret <= 0) {
        LOGW_CAMHW_SUBM(MOTIONDETECT_SUBM,  "isp sp poll buffer event got error(%d) but continue\n", poll_ret);
        return true;
    }

    ret = _isp_sp_dev->dequeue_buffer (buf);
    if (ret != XCAM_RETURN_NO_ERROR) {
        LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "dequeue isp_sp_dev buffer failed");
        return false;
    }

    SmartPtr<V4l2BufferProxy> buf_proxy         = new V4l2BufferProxy(buf, _isp_sp_dev);
    uint8_t *image_buf                          = (uint8_t *)buf_proxy->get_v4l2_planar_userptr(0);
    unsigned long long image_ts, ispgain_ts, mfbc_ts;
    image_ts                                    = *(unsigned long long*)(image_buf + buf_proxy->get_v4l2_buf_planar_length(0) - 8);
    struct isp2x_ispgain_buf *ispgain           = (struct isp2x_ispgain_buf *)buf_proxy->get_v4l2_planar_userptr(1);

    select_motion_params(&mtParamsSelect, ispgain->frame_id);

    int detect_flg                      = mtParamsSelect.enable;
    int detect_flg_last                 = frame_detect_flg[(static_ratio_idx_in + static_ratio_num - 1) % static_ratio_num];

    while(frame_id_isp_upt != -1 && frame_id_isp_upt != frame_id_pp_upt && static_ratio_idx_out == static_ratio_idx_in)
    {
        if (!is_running())
            break;
        usleep(1000);
    }

    if (_calibDb->af.ldg_param.enable)
        get_lowpass_fv(ispgain->frame_id, buf_proxy);

    uint8_t *static_ratio_cur                   = static_ratio[static_ratio_idx_in];

    LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "detect_flg=%d,_calibDb->mfnr.enable=%d,_calibDb->mfnr.motion_detect_en=%x \n",detect_flg,_calibDb->mfnr.enable,_calibDb->mfnr.motion_detect_en);
    if (detect_flg && _calibDb->mfnr.enable && _calibDb->mfnr.motion_detect_en && _motion_detect_control)
    {
        int wr_flg = get_wr_flg_func(frame_num_isp, 0);
        int wr_other_flg    = get_wr_other_flg_func();

        char name_tmp[200];
        char fop_mode[10];
        static int wr_flg_last                  = 0;
        if(wr_flg == 1 && wr_flg_last == 0)
            strcpy(fop_mode, "wb");
        else
            strcpy(fop_mode, "ab");

        struct timeval tv0, tv1, tv2, tv3, tv4, tv5, tv6, tva, tvb;
        gettimeofday(&tv0, NULL);
        int gain_fd = -1, mfbc_fd = -1;

        for (int i = 0; i < _isp_buf_num.load(); i++) {
            if (ispgain->gain_dmaidx == _isp_idx_array[i]) {
                gain_fd = _isp_fd_array[i];
            }
            if (ispgain->mfbc_dmaidx == _isp_idx_array[i]) {
                mfbc_fd = _isp_fd_array[i];
            }
        }
        LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "loop gain_dmaidx %u gain_fd %d mfbc_dmaidx %u mfbc_fd %d\n", ispgain->gain_dmaidx, gain_fd, ispgain->mfbc_dmaidx, mfbc_fd);
        void *gain_addr                             = mmap(NULL, ispgain->gain_size, PROT_READ | PROT_WRITE, MAP_SHARED, gain_fd, 0);
        if (MAP_FAILED == gain_addr) {
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "mmap gain_dmafd failed!!!(errno:%d),fd: %d, idx: %u, size: %d", errno, gain_fd, ispgain->gain_dmaidx, ispgain->gain_size);
            LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "\n*** ASSERT: In File %s,line %d ***\n", __FILE__, __LINE__);
            assert(0);
        }

        uint8_t *pCurIn                     = pImgbuf[static_ratio_idx_in];
        uint8_t *pPreIn                     = pImgbuf[(static_ratio_idx_in - 1 + static_ratio_num) % static_ratio_num];
#if DEBUG_TIMESTAMP
        gettimeofday(&tva, NULL);
#endif
        //memcpy(pCurIn, image_buf, img_buf_size + img_buf_size_uv);
        memcpy(pCurIn, image_buf, img_buf_size);
        memcpy(pCurIn + img_buf_size, image_buf + ALIGN_UP(img_buf_size, 64), img_buf_size_uv);
#if DEBUG_TIMESTAMP
        gettimeofday(&tvb, NULL);
#endif
        {
            if(wr_flg)
            {
                FILE *fd_ds_wr                                      = NULL;
                if(fd_ds_wr == NULL)
                {

                    strcpy(name_tmp, path_mt_test);
                    fd_ds_wr                                        = fopen(strcat(name_tmp, "ds_out.yuv"), fop_mode);

                    printf("path_mt_test %s name_tmp %s\n ", path_mt_test, name_tmp);

                    fwrite(pCurIn,              img_buf_size + img_buf_size_uv,             1,   fd_ds_wr);
                    fclose(fd_ds_wr);
                }
                FILE *fd_ratio_iir_out                              = NULL;
                if(fd_ratio_iir_out == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_ratio_iir_out                                = fopen(strcat(name_tmp, "ratio_iir_out.yuv"), fop_mode);
                    fwrite(pPreAlpha,           img_blk_isp_stride * gain_kg_tile_h_align,  1,   fd_ratio_iir_out);
                    fclose(fd_ratio_iir_out);
                }


                FILE *fd_ds_last_wr                                 = NULL;
                if(fd_ds_last_wr == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_ds_last_wr                                   = fopen(strcat(name_tmp, "ds_out_last.yuv"), fop_mode);
                    fwrite(pPreIn,              img_buf_size + img_buf_size_uv,             1,   fd_ds_last_wr);
                    fclose(fd_ds_last_wr);
                }


            }



            if(wr_flg && wr_other_flg)
            {

                FILE *fd_gain_isp_wr                                  = NULL;
                if(fd_gain_isp_wr == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_gain_isp_wr                                    = fopen(strcat(name_tmp, "gain_isp_out.yuv"), fop_mode);
                    fwrite(gain_addr, gain_blk_isp_stride * gain_blk_isp_h, 1,    fd_gain_isp_wr);
                    fclose(fd_gain_isp_wr);
                }
            }
        }

        if(detect_flg_last == 1)
        {
#if DEBUG_TIMESTAMP
            gettimeofday(&tv1, NULL);
#endif

            uint8_t *src = (uint8_t*)gain_addr;
            uint8_t *gain_isp_buf_cur           = gain_isp_buf_bak[static_ratio_idx_in];
            mtParamsSelect.wr_flg               = wr_flg & wr_other_flg;
            mtParamsSelect.wr_flg_last          = wr_flg_last;
            memcpy(gain_isp_buf_cur, src, gain_blk_isp_stride * gain_blk_isp_h);
#if DEBUG_TIMESTAMP
            gettimeofday(&tv2, NULL);
#endif
            motion_detect(pCurIn, pPreIn, pTmpBuf, static_ratio_cur, pPreAlpha, src, &mtParamsSelect);
            LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "motindetected!!! in loop \n");
            if(mtParamsSelect.static_flg)
                static_frame_num++;
            else
                static_frame_num = 0;
            if(static_frame_num < 30)
                mtParamsSelect.yuvnr_gain_scale_glb = 1.0;

#if DEBUG_TIMESTAMP
            gettimeofday(&tv3, NULL);
#endif

#if DEBUG_TIMESTAMP
            static int num = 0;
            num++;
            if((num % 1) == 0)
                LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM,  "set_gain_isp frame_write_st %d time %8ld %8ld %8ld %8ld delta %8ld %8ld %8ld %8ld ratio_cur:%d %x\n", frame_write_st,
                                tv0.tv_usec, tv1.tv_usec, tv2.tv_usec, tv3.tv_usec, tv1.tv_usec - tv0.tv_usec, tv2.tv_usec - tv1.tv_usec,
                                tv3.tv_usec - tv2.tv_usec, tv3.tv_usec - tv0.tv_usec, static_ratio_cur[0], gain_blk_isp_stride * gain_kg_tile_h_align);
#endif

        }
        else
            memset(static_ratio_cur, static_ratio_l, gain_kg_tile_h_align * gain_blk_isp_stride);
        if(wr_flg)
        {
            FILE *fd_param_out                                  = NULL;
            if(fd_param_out == NULL)
            {
                mtParamsSelect.gain_ratio_cur                   = mtParamsSelect.gain_ratio;
                mtParamsSelect.gain_ratio_last                  = (*(mtParamsSelect_list[(static_ratio_idx_in + static_ratio_num - 1) % static_ratio_num])).gain_ratio;
                char header[5]                                  = "HEAD";
                int len                                         = sizeof(mtParamsSelect) + sizeof(float) * 2;
                strcpy(name_tmp, path_mt_test);
                fd_param_out                                    = fopen(strcat(name_tmp, "param_out.yuv"),   fop_mode);
                fwrite(header,              4,                                          1,   fd_param_out);
                fwrite(&len,                4,                                          1,   fd_param_out);
                fwrite(&mtParamsSelect,     sizeof(mtParamsSelect),                     1,   fd_param_out);
                fclose(fd_param_out);
            }
        }
        wr_flg_last = wr_flg;
        if(wr_flg)
        {

            FILE *fd_ratio_wr                                  = NULL;
            if(fd_ratio_wr == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_ratio_wr                                    = fopen(strcat(name_tmp, "ratio_out.yuv"), fop_mode);
                fwrite(static_ratio_cur,        img_blk_isp_stride * gain_kg_tile_h_align,    1,  fd_ratio_wr);
                fclose(fd_ratio_wr);
            }
        }

        if(wr_flg && wr_other_flg)
        {

            FILE *fd_ds_wr                                      = NULL;
            if(fd_ds_wr == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_ds_wr                                        = fopen(strcat(name_tmp, "ds_out_iir.yuv"), fop_mode);
                fwrite(pCurIn,              img_buf_size + img_buf_size_uv,         1,  fd_ds_wr);
                fclose(fd_ds_wr);
            }




            FILE *fd_gain_isp_up_out                            = NULL;
            if(fd_gain_isp_up_out == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_gain_isp_up_out                              = fopen(strcat(name_tmp, "gain_isp_up_out.yuv"), fop_mode);
                fwrite(gain_addr,           gain_blk_isp_stride * gain_blk_isp_h,   1,  fd_gain_isp_up_out);
                fclose(fd_gain_isp_up_out);
            }

        }

        munmap(gain_addr, ispgain->gain_size);
    }
    else {
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "motindete not done in loop\n");
    }
    frame_detect_flg[static_ratio_idx_in]       = detect_flg;
    *(mtParamsSelect_list[static_ratio_idx_in]) = mtParamsSelect;
    frame_num_isp++;
    static_ratio_idx_in++;
    static_ratio_idx_in     %= static_ratio_num;
    frame_id_isp_upt        = ispgain->frame_id;
    LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "loop frame_num_isp %d fid %u \n", frame_num_isp, ispgain->frame_id);

    if(_first_frame) {
        _first_frame = false;
        LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "first frame\n");
    }
    else {
        _buf_list_mutex.lock();
        LOGD_CAMHW_SUBM(MOTIONDETECT_SUBM, "v4l2buf index %d push list\n", buf_proxy->get_v4l2_buf_index());
        _isp_buf_list.push_back(buf_proxy);
        _buf_list_mutex.unlock();
    }

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);

    return true;
}

void
Isp20SpThread::set_sp_dev(SmartPtr<V4l2SubDevice> ispdev, SmartPtr<V4l2Device> ispspdev,
                          SmartPtr<V4l2SubDevice> isppdev, SmartPtr<V4l2SubDevice> snsdev,
                          SmartPtr<V4l2SubDevice> lensdev, SmartPtr<NrProcUint> nrdev)
{
    _isp_dev = ispdev;
    _isp_sp_dev = ispspdev;
    _ispp_dev = isppdev;
    _sensor_dev = snsdev;
    _focus_dev = lensdev;
    _nr_dev = nrdev;
    /*
     * if (_nr_dev.ptr ()) {
     *     rk_aiq_amd_result_t amdResult;
     *     _nr_dev->configAmdResult (amdResult);
     * }
     */
}

void
Isp20SpThread::set_sp_img_size(int w, int h, int w_align, int h_align, int ds_w, int ds_h)
{
    _gain_width         = w;
    _gain_height        = h;
    _img_width_align    = w_align;
    _img_height_align   = h_align;
    img_ds_size_x = ds_w;
    img_ds_size_y = ds_h;
    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "_gain_height %d %d _gain_width %d %d ds_size: %d %d \n",
                    h, h_align, w, w_align, ds_w, ds_h);
    //assert(((w == (w_align - 1)) || (w == w_align)) && ((h == (h_align - 1)) || (h == h_align )));
}

void
Isp20SpThread::set_af_img_size(int w, int h, int w_align, int h_align)
{
    af_img_width         = w;
    af_img_height        = h;
    af_img_width_align   = w_align;
    af_img_height_align  = h_align;
    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "af_img_width %d af_img_height %d af_img_width_align: %d af_img_height_align: %d\n",
                    w, h, w_align, h_align);
}

void
Isp20SpThread::set_gain_isp(void *buf, uint8_t* ratio)
{}


void
Isp20SpThread::set_gain_wr(void *buf, uint8_t* ratio, uint8_t *gain_isp_buf_cur, RKAnr_Mt_Params_Select_t mtParamsSelect_cur, uint16_t h_st, uint16_t h_end)
{


    uint8_t *src                                    = (uint8_t*)buf;
    uint16_t gain_upt_flg;
    float yuvnr_gain_th;
    float yuvnr_gain_scale[4];
    float yuvnr_gain_scale_glb;
    gain_upt_flg                                    = mtParamsSelect_cur.gain_upt_flg;
    yuvnr_gain_th                                   = mtParamsSelect_cur.yuvnr_gain_th;
    for(int i = 0; i < 4; i++)
        yuvnr_gain_scale[i]                         = mtParamsSelect_cur.yuvnr_gain_scale[i];
    yuvnr_gain_scale_glb                            = mtParamsSelect_cur.yuvnr_gain_scale_glb;
    uint16_t yuvnr_gain_th_fix;
    uint16_t yuvnr_gain_scale_fix[4];
    uint16_t yuvnr_gain_scale_glb_fix;
    yuvnr_gain_th_fix                               = ROUND_F(yuvnr_gain_th         * (1 << YUV_SCALE_FIX_BITS));
    yuvnr_gain_scale_fix[0]                         = ROUND_F(yuvnr_gain_scale[0]   * (1 << YUV_SCALE_FIX_BITS));
    yuvnr_gain_scale_fix[1]                         = ROUND_F(yuvnr_gain_scale[1]   * (1 << YUV_SCALE_FIX_BITS));
    yuvnr_gain_scale_fix[2]                         = ROUND_F(yuvnr_gain_scale[2]   * (1 << YUV_SCALE_FIX_BITS));
    yuvnr_gain_scale_fix[3]                         = ROUND_F(yuvnr_gain_scale[3]   * (1 << YUV_SCALE_FIX_BITS));
    yuvnr_gain_scale_glb_fix                        = ROUND_F(yuvnr_gain_scale_glb  * (1 << YUV_SCALE_FIX_BITS));

    uint16_t ratio_static                           = (1 << static_ratio_l_bit) - 8;
    static int num  = 0;
    if((num % 600) == 0)
        LOGI_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr yuvnr_gain_scale_fix %d %d %d %d yuvnr_gain_th_fix %d\n ", yuvnr_gain_scale_fix[0], yuvnr_gain_scale_fix[1], yuvnr_gain_scale_fix[2], yuvnr_gain_scale_fix[3], yuvnr_gain_th_fix);
    num++;

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "set_gain_wr frame_num_pp %d frame_write_st %d  write_frame_num %d\n ", frame_num_pp, frame_write_st, write_frame_num);
    int wr_flg                                      = get_wr_flg_func(frame_num_pp, 1);
    int wr_other_flg                                = get_wr_other_flg_func();
    static int wr_flg_last                          = 0;
    char name_tmp[200];
    char fop_mode[10];

    if(wr_flg == 1 && wr_flg_last == 0)
        strcpy(fop_mode, "wb");
    else
        strcpy(fop_mode, "ab");

    wr_flg_last                                     = wr_flg;
    wr_flg                                          &= wr_other_flg;
    uint8_t *test_buff[2];
    uint8_t *test_buff_ori[2];
    uint8_t *gain_isp_buf_ds                        = NULL;

    //  memset(src, 0x2b, gain_blk_ispp_stride * gain_blk_ispp_h);
    //  memset(gain_isp_buf_cur, 0x4c, gain_blk_ispp_stride * gain_blk_ispp_h);
    //  memset(ratio, 0x68, gain_blk_ispp_stride * gain_blk_ispp_h);
    if(wr_flg)
    {
        for(int i = 0; i < 2; i++)
        {
            test_buff[i]                            = (uint8_t*)malloc(gain_blk_ispp_stride * gain_blk_ispp_h * 2 * sizeof(test_buff[0][0]) );
            test_buff_ori[i]                        = (uint8_t*)malloc(gain_blk_ispp_stride * gain_blk_ispp_h * 2 * sizeof(test_buff[0][0]) );
        }
        gain_isp_buf_ds                             = (uint8_t*)malloc(gain_blk_ispp_stride * gain_blk_ispp_h * sizeof(gain_isp_buf_ds[0]));
        FILE *fd_gain_yuvnr_wr                      = NULL;
        if(fd_gain_yuvnr_wr == NULL)
        {
            strcpy(name_tmp, path_mt_test);
            fd_gain_yuvnr_wr                        = fopen(strcat(name_tmp, "gain_pp_out.yuv"),  fop_mode);
            fwrite(src, gain_blk_ispp_stride * gain_blk_ispp_h * 2, 1, fd_gain_yuvnr_wr);
            fclose(fd_gain_yuvnr_wr);
        }

    }

#ifndef ENABLE_NEON
    for(int i = h_st; i < h_end; i++)
        for(int j = 0; j < gain_blk_ispp_stride; j++)
        {
            int idx_isp                     = i * gain_blk_isp_stride + j * 2;
            int idx_gain                    = (i * gain_blk_ispp_stride + j) * 2;
            int idx_ispp                    = (i * gain_blk_ispp_stride + j);
            int idx_ratio;
            int ratio_cur;
            if(img_ds_size_x == 8)
            {
                idx_ratio                   = i * img_blk_isp_stride + j * 1;
                ratio_cur                   = ratio[idx_ratio];
            }
            else
            {
                idx_ratio                   = i * img_blk_isp_stride + j * 2;
                ratio_cur                   = MAX(ratio[idx_ratio], ratio[idx_ratio + 1]);
            }
            int gain_isp_cur                = MAX(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);

            if(wr_flg)
            {
                test_buff_ori[0][idx_ispp]  = src[idx_gain + 0];
                test_buff_ori[1][idx_ispp]  = src[idx_gain + 1];
                gain_isp_buf_ds[idx_ispp]   = gain_isp_cur;
            }

            uint16_t tmp0;
            uint16_t tmp1;
            float rr[2];
            if(ratio_cur > ratio_static)
            {
                float gain_scale_cur            = 1.0f * src[idx_gain + 1] / gain_isp_cur;
                gain_scale_cur                  = MIN(gain_scale_cur, 1);
                gain_scale_cur                  = MAX(gain_scale_cur - yuvnr_gain_th, 0);
                gain_scale_cur                  = gain_scale_cur / (1 - yuvnr_gain_th);
                gain_scale_cur                  = gain_scale_cur != 0;
                rr[0]                           = yuvnr_gain_scale_glb * (1 - gain_scale_cur) + yuvnr_gain_scale[2] * gain_scale_cur ;
                rr[1]                           = yuvnr_gain_scale_glb * (1 - gain_scale_cur) + yuvnr_gain_scale[3] * gain_scale_cur ;

            }
            else
            {

                rr[0]                           = yuvnr_gain_scale[0] ;
                rr[1]                           = yuvnr_gain_scale[1];
            }
            float tmp0_f, tmp1_f;
            tmp0_f                              = src[idx_gain]     * rr[0]; // low bit 8 * bit 5+2
            tmp1_f                              = src[idx_gain + 1]   * rr[1]; // high

            if(gain_upt_flg)
            {
                tmp0_f                          = (tmp0_f * (1 << static_ratio_l_bit)) / ratio_cur;
                tmp1_f                          = (tmp1_f * (1 << static_ratio_l_bit)) / ratio_cur;
            }
            tmp0                                = ROUND_F(tmp0_f);
            tmp1                                = ROUND_F(tmp1_f);
            src[idx_gain]                       = MIN(255, tmp0);
            src[idx_gain + 1]                   = MIN(255, tmp1);
            if(wr_flg)
            {
                test_buff[0][idx_ispp]          = src[idx_gain + 0];
                test_buff[1][idx_ispp]          = src[idx_gain + 1];
            }
        }
#else

    int     lastProNum = 0;
    if(gain_blk_ispp_stride & 7)
    {
        lastProNum = gain_blk_ispp_stride & 7;
    }
    uint8_t gain_upt_flg_bits                   = gain_upt_flg * 0xffff;
    for(int i = h_st; i < h_end; i++)
    {
        int idx_isp             = 0;
        int idx_gain            = 0;
        int idx_ratio           = 0;
        uint8_t *pGainIsp00     = gain_isp_buf_cur  + i * gain_blk_isp_stride;
        uint8_t *pSrc00         = src               + i * gain_blk_ispp_stride * 2;
        uint8_t *pRatio00       = ratio             + i * img_blk_isp_stride;
        uint8x8x2_t             vSrc00;
        uint8x8x2_t             vGainIsp00;
        uint8x8x2_t             vRatio_u8;




        for(int j = 0; j < gain_blk_ispp_stride ; j += 8)
        {
            uint16x8x2_t            vSrc00_u16;
            uint16x8_t              vsrc_cmp_l;
            uint16x8_t              vsrc_cmp_r;
            uint16x8_t              vFlag00_u16, vFlag01_u16;
            uint16x8x2_t            vRR00;
            uint16x4x2_t            vRR00_0;
            uint16x4x2_t            vRR00_1;


            vSrc00                  = vld2_u8(pSrc00        + idx_isp);
            vGainIsp00              = vld2_u8(pGainIsp00    + idx_gain);
            vRatio_u8               = vld2_u8(pRatio00      + idx_ratio);

            vSrc00_u16.val[0]       = vmovl_u8(vSrc00.val[0]);
            vSrc00_u16.val[1]       = vmovl_u8(vSrc00.val[1]);

            vGainIsp00.val[0]       = vmax_u8(vGainIsp00.val[0],            vGainIsp00.val[1]);
            if(img_ds_size_x == 8)
            {
                vRatio_u8           = vzip_u8(vRatio_u8.val[0],             vRatio_u8.val[1]);
                idx_ratio           += 8;
            }
            else
            {
                vRatio_u8.val[0]    = vmax_u8(vRatio_u8.val[0],             vRatio_u8.val[1]);
                idx_ratio           += 16;
            }


            vsrc_cmp_l              = vmulq_n_u16(vmovl_u8(vSrc00.val[1]),        (1 << YUV_SCALE_FIX_BITS));
            vsrc_cmp_r              = vmulq_n_u16(vmovl_u8(vGainIsp00.val[0]),    yuvnr_gain_th_fix);
            // (1.0f*src[idx_gain+1])/gain_isp_cur  > 13.0/40
            vFlag00_u16             = vcgtq_u16(vsrc_cmp_r,                 vsrc_cmp_l);
            // ratio_cur >  (1 << static_ratio_l_bit) - 20
            vFlag01_u16             = vcgtq_u16(vmovl_u8(vRatio_u8.val[0]), vdupq_n_u16(ratio_static));
            vRR00.val[0]            = vbslq_u16(vFlag00_u16,                vdupq_n_u16(yuvnr_gain_scale_glb_fix),  vdupq_n_u16(yuvnr_gain_scale_fix[2]));
            vRR00.val[1]            = vbslq_u16(vFlag00_u16,                vdupq_n_u16(yuvnr_gain_scale_glb_fix),  vdupq_n_u16(yuvnr_gain_scale_fix[3]));

            vRR00.val[0]            = vbslq_u16(vFlag01_u16,                vRR00.val[0],                           vdupq_n_u16(yuvnr_gain_scale_fix[0]));
            vRR00.val[1]            = vbslq_u16(vFlag01_u16,                vRR00.val[1],                           vdupq_n_u16(yuvnr_gain_scale_fix[1]));


            // tmp0                 = (src[idx_gain]     * rr) << static_ratio_l_bit;
            vSrc00_u16.val[0]       = vmulq_u16(vSrc00_u16.val[0],          vRR00.val[0]);
            vSrc00_u16.val[1]       = vmulq_u16(vSrc00_u16.val[1],          vRR00.val[1]);


            float32x4x2_t           vSrc_l_f32, vSrc_h_f32;
            float32x4x2_t           vRatio_f32_4;

            float32x4x2_t           reciprocal_vRatio;

            // multiply ,1 for rounding
            uint32x4_t              vOut00_lo, vOut00_hi, vOut01_lo, vOut01_hi;

            //0 1 low 2 3 high
            vSrc_l_f32.val[0]       = vcvtq_f32_u32(vmovl_u16(vget_low_u16   (vSrc00_u16.val[0])));
            vSrc_l_f32.val[1]       = vcvtq_f32_u32(vmovl_u16(vget_high_u16  (vSrc00_u16.val[0])));
            vSrc_h_f32.val[0]       = vcvtq_f32_u32(vmovl_u16(vget_low_u16   (vSrc00_u16.val[1])));
            vSrc_h_f32.val[1]       = vcvtq_f32_u32(vmovl_u16(vget_high_u16  (vSrc00_u16.val[1])));
            ///////////////////////////////////////////////////////////////////////////////////////////////
            // tmp0                 = (tmp0 << static_ratio_l_bit)/ratio_cur;
            // reciprocal
            vRatio_f32_4.val[0]     = vcvtq_f32_u32(vmovl_u16(vget_low_u16   (vmovl_u8(vRatio_u8.val[0]))));
            vRatio_f32_4.val[1]     = vcvtq_f32_u32(vmovl_u16(vget_high_u16  (vmovl_u8(vRatio_u8.val[0]))));


            reciprocal_vRatio.val[0] = vrecpeq_f32(vRatio_f32_4.val[0]);
            reciprocal_vRatio.val[1] = vrecpeq_f32(vRatio_f32_4.val[1]);

            reciprocal_vRatio.val[0] = vmulq_f32(vrecpsq_f32(vRatio_f32_4.val[0], reciprocal_vRatio.val[0]), reciprocal_vRatio.val[0]);
            reciprocal_vRatio.val[1] = vmulq_f32(vrecpsq_f32(vRatio_f32_4.val[1], reciprocal_vRatio.val[1]), reciprocal_vRatio.val[1]);

            vSrc_l_f32.val[0]       = vmulq_f32(vSrc_l_f32.val[0], reciprocal_vRatio.val[0]);
            vSrc_h_f32.val[0]       = vmulq_f32(vSrc_h_f32.val[0], reciprocal_vRatio.val[0]);
            vSrc_l_f32.val[1]       = vmulq_f32(vSrc_l_f32.val[1], reciprocal_vRatio.val[1]);
            vSrc_h_f32.val[1]       = vmulq_f32(vSrc_h_f32.val[1], reciprocal_vRatio.val[1]);

            vOut00_lo               = vcvtq_n_u32_f32(vSrc_l_f32.val[0], 1);
            vOut00_hi               = vcvtq_n_u32_f32(vSrc_h_f32.val[0], 1);

            vOut01_lo               = vcvtq_n_u32_f32(vSrc_l_f32.val[1], 1);
            vOut01_hi               = vcvtq_n_u32_f32(vSrc_h_f32.val[1], 1);

            uint16x8_t              vOut_lo, vOut_hi;
            uint8x8x2_t             vOut00;
            vOut_lo                 = vcombine_u16(vmovn_u32(vOut00_lo), vmovn_u32(vOut01_lo));
            vOut_hi                 = vcombine_u16(vmovn_u32(vOut00_hi), vmovn_u32(vOut01_hi));

            //  vOut00.val[0]           = vqrshrn_n_u16(vOut_lo, RATIO_BITS_NUM+ 1 - YUV_SCALE_FIX_BITS);
            //  vOut00.val[1]           = vqrshrn_n_u16(vOut_hi, RATIO_BITS_NUM+ 1 - YUV_SCALE_FIX_BITS);
#if YUV_SCALE_FIX_BITS == 8
            vOut00.val[0]           = vqrshrn_n_u16(vOut_lo, YUV_SCALE_FIX_BITS + 1 - RATIO_BITS_NUM);
            vOut00.val[1]           = vqrshrn_n_u16(vOut_hi, YUV_SCALE_FIX_BITS + 1 - RATIO_BITS_NUM);
#else
            vOut00.val[0]           = vmovn_u16(vshlq_n_u16(vOut_lo, RATIO_BITS_NUM - YUV_SCALE_FIX_BITS - 1));
            vOut00.val[1]           = vmovn_u16(vshlq_n_u16(vOut_hi, RATIO_BITS_NUM - YUV_SCALE_FIX_BITS - 1));
#endif
            // for gain_upt_flg == 0
            vOut00.val[0]           = vbsl_u8(vdup_n_u8(gain_upt_flg_bits), vOut00.val[0],   vqrshrn_n_u16(vSrc00_u16.val[0],       YUV_SCALE_FIX_BITS));
            vOut00.val[1]           = vbsl_u8(vdup_n_u8(gain_upt_flg_bits), vOut00.val[1],   vqrshrn_n_u16(vSrc00_u16.val[1],       YUV_SCALE_FIX_BITS));
            vst2_u8(pSrc00 + idx_isp, vOut00);
            idx_isp                 += 16;
            idx_gain                += 16;


        }
    }

#endif

    if(wr_flg)
    {
        FILE *fdtmp = NULL;
        if(fdtmp == NULL)
        {
            strcpy(name_tmp, path_mt_test);
            fdtmp                       = fopen(strcat(name_tmp, "gain_isp_ds_out.yuv"),    fop_mode);
            fwrite(gain_isp_buf_ds, gain_blk_ispp_stride * gain_blk_ispp_h, 1, fdtmp);
            fclose(fdtmp);
        }
        if(gain_isp_buf_ds)
            free(gain_isp_buf_ds);
    }
    if(wr_flg)
    {
        FILE *fd_gain_yuvnr_up_wr = NULL;
        if(fd_gain_yuvnr_up_wr == NULL)
        {
            strcpy(name_tmp, path_mt_test);
            fd_gain_yuvnr_up_wr         = fopen(strcat(name_tmp, "gain_pp_up_out.yuv"),     fop_mode);
            fwrite(src, gain_blk_ispp_stride * gain_blk_ispp_h * 2, 1, fd_gain_yuvnr_up_wr);
            fclose(fd_gain_yuvnr_up_wr);
        }

        FILE *fd_gain_yuvnr_sp_out = NULL;
        if(fd_gain_yuvnr_sp_out == NULL)
        {
            strcpy(name_tmp, path_mt_test);
            fd_gain_yuvnr_sp_out        = fopen(strcat(name_tmp, "gain_pp_sp_out.yuv"),     fop_mode);
            fwrite(test_buff_ori[0], gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gain_yuvnr_sp_out);
            fwrite(test_buff_ori[1], gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gain_yuvnr_sp_out);
            fclose(fd_gain_yuvnr_sp_out);
        }

        FILE *fd_gain_yuvnr_up_sp_out = NULL;
        if(fd_gain_yuvnr_up_sp_out == NULL)
        {
            strcpy(name_tmp, path_mt_test);
            fd_gain_yuvnr_up_sp_out     = fopen(strcat(name_tmp, "gain_pp_up_sp_out.yuv"),  fop_mode);
            fwrite(test_buff[0], gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gain_yuvnr_up_sp_out);
            fwrite(test_buff[1], gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gain_yuvnr_up_sp_out);
            fclose(fd_gain_yuvnr_up_sp_out);
        }
    }
    if(wr_flg)
    {
        for(int i = 0; i < 2; i++)
        {
            free(test_buff[i]);
            free(test_buff_ori[i]);
        }
    }

}






void
Isp20SpThread::set_gainkg(void *buf, uint8_t* ratio, uint8_t* ratio_next, uint8_t *gain_isp_buf_cur, RKAnr_Mt_Params_Select_t mtParamsSelect_cur, RKAnr_Mt_Params_Select_t mtParamsSelect_last)
{
    uint8_t *src                                        = (uint8_t *)buf;
    if(mtParamsSelect_cur.gain_upt_flg == 0) {
        return;
    }

    uint16_t frame_limit_div_y                          = 256 / sqrt(mtParamsSelect_cur.frame_limit_y);
    uint16_t frame_limit_div_uv                         = 256 / sqrt(mtParamsSelect_cur.frame_limit_uv);
    uint16_t gain_upt_flg                               = mtParamsSelect_cur.gain_upt_flg;
    float gain_ratio_cur                                = mtParamsSelect_cur.gain_ratio;
    float gain_ratio_last                               = mtParamsSelect_last.gain_ratio;
    uint16_t mfnr_gain_scale_fix                        = ROUND_F(mtParamsSelect_cur.mfnr_gain_scale * (1 << GAIN_SCALE_MOTION_BITS));


    int src_stride                                      = gain_tile_gainkg_size;
    int dst_stride                                      = 2;
    int gain_min_val                                    = 1;
    uint8_t ratio_shf_bit                               = static_ratio_l_bit;
    uint16_t ratio_nxt_th                               = ALPHA_MAX_VAL - 8;

    int8_t gain_ratio_shf_bits                          = (int8_t)((log(gain_ratio_cur / gain_ratio_last) / log(2.0)) / 2);
    uint8_t gain_ratio_shf_bits_abs                     = abs(gain_ratio_shf_bits);


    if(gain_ratio_last == -1)
    {
        gain_ratio_shf_bits                         = 0;
        gain_ratio_shf_bits_abs                     = 0;
    }
    else
    {
        gain_ratio_shf_bits                             = (int8_t)((log(gain_ratio_cur / gain_ratio_last) / log(2.0)) / 2);
        gain_ratio_shf_bits_abs                     = abs(gain_ratio_shf_bits);
    }

    uint8_t *src_ori = NULL;
    uint8_t *src_mid = NULL;
    uint8_t *test_buff[4];
    uint8_t *test_buff_mid[4];
    uint8_t *test_buff_mid1[4];
    uint8_t *test_buff_ori[4];
    int wr_flg                                      = get_wr_flg_func(frame_num_pp, 1);
    int wr_other_flg                                = get_wr_other_flg_func();
    static int wr_flg_last                          = 0;

    char name_tmp[200];
    char fop_mode[10];
    if(wr_flg == 1 && wr_flg_last == 0)
        strcpy(fop_mode, "wb");
    else
        strcpy(fop_mode, "ab");

    wr_flg_last                                     = wr_flg;
    wr_flg                                          &= wr_other_flg;

    if(wr_flg)
    {

        for(int i = 0; i < 4; i++)
        {
            test_buff[i]        = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff[0][0]) );
            test_buff_mid[i]    = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff_mid[0][0]) );
            test_buff_mid1[i]   = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff_mid1[0][0]) );
            test_buff_ori[i]    = (uint8_t*)malloc((gain_tile_ispp_y * gain_tile_ispp_x * 1) * gain_tile_gainkg_h * gain_tile_gainkg_w * sizeof(test_buff_ori[0][0]) );
        }
        src_mid                 = (uint8_t*)malloc(gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]) );
        memset(src_mid, 0, gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]));
        src_ori                 = (uint8_t*)malloc(gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]) );
        memcpy(src_ori, src, gain_tile_gainkg_stride * gain_tile_gainkg_h * sizeof(test_buff[0][0]));
    }

    for(int i = 0; i < gain_tile_gainkg_h; i++)
    {
        uint8_t block_h_cur = MIN(gain_blk_ispp_h - i * gain_tile_ispp_y, gain_tile_ispp_y);
        for(int j = 0; j < gain_tile_gainkg_w; j += gainkg_tile_num)
        {
            //   for(int tile_idx = 0; tile_idx < gainkg_tile_num; tile_idx++)
            int tile_off        = i * gain_tile_gainkg_stride + j * gain_tile_gainkg_size;
            int tile_i_ispp     = i * gain_tile_ispp_y;
            int tile_j_ispp     = j * gain_tile_ispp_x;
#ifndef ENABLE_NEON
            for(int y = 0; y < gain_tile_ispp_y; y++)
            {
                for(int x = 0; x < gain_tile_ispp_x; x++)
                {

                    int idx_isp;
                    int idx_gain;
                    int idx_ratio;
                    int idx_ispp;
                    uint8_t ratio_cur;
                    uint16_t ratio_nxt;
                    uint16_t ratio_nxt_scale[4];
                    int gain_isp_cur;
                    int gain_isp_cur_y;
                    int gain_isp_cur_uv;
                    int i_act                           = tile_i_ispp + y;
                    int j_act                           = tile_j_ispp + x;
                    idx_isp                     = i_act * gain_blk_isp_stride   + j_act * 2;
                    idx_gain                    = tile_off + y * gain_tile_ispp_x * gainkg_unit + x;
                    idx_ispp                    = i_act * gain_blk_ispp_stride + j_act;

                    if(idx_gain >= 0x54c0) //0x8fd)
                        idx_ispp = idx_ispp;
                    gain_isp_cur                = MIN(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);
                    gain_isp_cur_y              = MAX((gain_isp_cur * frame_limit_div_y + 128) >> 8, 1);
                    gain_isp_cur_uv             = MAX((gain_isp_cur * frame_limit_div_uv + 128) >> 8, 1);


                    if(img_ds_size_x == 8)
                    {
                        idx_ratio                       = i_act * img_blk_isp_stride    + j_act;
                        ratio_cur                       = ratio[idx_ratio];

                    }
                    else
                    {
                        idx_ratio                       = i_act * img_blk_isp_stride    + j_act * 2;
                        ratio_cur                       = MAX(ratio[idx_ratio], ratio[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                    }


                    if(i_act > gain_blk_ispp_h - 1 || j_act > gain_blk_ispp_w - 1)
                        continue;
                    if(wr_flg)
                    {
                        test_buff_ori[0][idx_ispp]  = src[idx_gain + 0];
                        test_buff_ori[1][idx_ispp]  = src[idx_gain + 2];
                        test_buff_ori[2][idx_ispp]  = src[idx_gain + 4];
                        test_buff_ori[3][idx_ispp]  = src[idx_gain + 6];
                    }
                    src[idx_gain + 0]               = MIN(255, ROUND_F((src[idx_gain + 0]    << ratio_shf_bit) / ratio_cur));
                    src[idx_gain + 2]               = MIN(255, ROUND_F((src[idx_gain + 2]    << ratio_shf_bit) / ratio_cur));
                    src[idx_gain + 4]               = MIN(255, ROUND_F((src[idx_gain + 4]    << ratio_shf_bit) / ratio_cur));
                    src[idx_gain + 6]               = MIN(255, ROUND_F((src[idx_gain + 6]    << ratio_shf_bit) / ratio_cur));
#if 1

                    src[idx_gain + 0]               = MAX(gain_isp_cur_y,   src[idx_gain + 0]);
                    src[idx_gain + 2]               = MAX(gain_isp_cur_uv,  src[idx_gain + 2]);
                    src[idx_gain + 4]               = MAX(gain_isp_cur_y,   src[idx_gain + 4]);
                    src[idx_gain + 6]               = MAX(gain_isp_cur_uv,  src[idx_gain + 6]);
#endif
                    if(img_ds_size_x == 8)
                        ratio_nxt                       = ratio_next[idx_ratio];//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                    else
                        ratio_nxt                       = MAX(ratio_next[idx_ratio], ratio_next[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                    if(wr_flg)
                    {
                        test_buff_mid[0][idx_ispp]  = src[idx_gain + 0];
                        test_buff_mid[1][idx_ispp]  = src[idx_gain + 2];
                        test_buff_mid[2][idx_ispp]  = src[idx_gain + 4];
                        test_buff_mid[3][idx_ispp]  = src[idx_gain + 6];
                    }

                    if(gain_ratio_shf_bits > 0)
                    {

                        src[idx_gain + 0]               = MIN(255, (src[idx_gain + 0]    << gain_ratio_shf_bits_abs) );
                        src[idx_gain + 2]               = MIN(255, (src[idx_gain + 2]    << gain_ratio_shf_bits_abs) );
                        src[idx_gain + 4]               = MIN(255, (src[idx_gain + 4]    << gain_ratio_shf_bits_abs) );
                        src[idx_gain + 6]               = MIN(255, (src[idx_gain + 6]    << gain_ratio_shf_bits_abs) );
                    }
                    else
                    {

                        src[idx_gain + 0]               = MAX(1, ROUND(src[idx_gain + 0]    >> gain_ratio_shf_bits_abs) );
                        src[idx_gain + 2]               = MAX(1, ROUND(src[idx_gain + 2]    >> gain_ratio_shf_bits_abs) );
                        src[idx_gain + 4]               = MAX(1, ROUND(src[idx_gain + 4]    >> gain_ratio_shf_bits_abs) );
                        src[idx_gain + 6]               = MAX(1, ROUND(src[idx_gain + 6]    >> gain_ratio_shf_bits_abs) );

                    }
                    if(wr_flg)
                    {
                        test_buff_mid1[0][idx_ispp]  = src[idx_gain + 0];
                        test_buff_mid1[1][idx_ispp]  = src[idx_gain + 2];
                        test_buff_mid1[2][idx_ispp]  = src[idx_gain + 4];
                        test_buff_mid1[3][idx_ispp]  = src[idx_gain + 6];
                    }


                    if(ratio_nxt > ratio_nxt_th)
                        ratio_nxt                       = ratio_nxt;
                    else
                        ratio_nxt                       = ((uint32_t)ratio_nxt * mfnr_gain_scale_fix + (1 << (GAIN_SCALE_MOTION_BITS - 1))) >> GAIN_SCALE_MOTION_BITS;

                    src[idx_gain + 0]                   = ROUND_INT(src[idx_gain + 0] * ratio_nxt, ratio_shf_bit);// + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                    src[idx_gain + 2]                   = ROUND_INT(src[idx_gain + 2] * ratio_nxt, ratio_shf_bit);//(src[idx_gain + 2] * ratio_nxt + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                    src[idx_gain + 4]                   = ROUND_INT(src[idx_gain + 4] * ratio_nxt, ratio_shf_bit);//(src[idx_gain + 4] * ratio_nxt + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                    src[idx_gain + 6]                   = ROUND_INT(src[idx_gain + 6] * ratio_nxt, ratio_shf_bit);//(src[idx_gain + 6] * ratio_nxt + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;

                    src[idx_gain + 0]               = MAX(gain_min_val,    src[idx_gain + 0]);
                    src[idx_gain + 2]               = MAX(gain_min_val,    src[idx_gain + 2]);
                    src[idx_gain + 4]               = MAX(gain_min_val,    src[idx_gain + 4]);
                    src[idx_gain + 6]               = MAX(gain_min_val,    src[idx_gain + 6]);

                    if(wr_flg)
                    {
                        src_mid[idx_gain + 0]       = src[idx_gain + 0];
                        src_mid[idx_gain + 2]       = src[idx_gain + 2];
                        src_mid[idx_gain + 4]       = src[idx_gain + 4];
                        src_mid[idx_gain + 6]       = src[idx_gain + 6];

                        test_buff[0][idx_ispp]      = src[idx_gain + 0];
                        test_buff[1][idx_ispp]      = src[idx_gain + 2];
                        test_buff[2][idx_ispp]      = src[idx_gain + 4];
                        test_buff[3][idx_ispp]      = src[idx_gain + 6];
                    }

                    if(i_act > gain_blk_ispp_h - 1 || j_act + 1 > gain_blk_ispp_w - 1)
                        continue;
                    idx_gain                        = tile_off                      +  y * gain_tile_ispp_x * gainkg_unit  + gainkg_unit + x;
                    idx_ispp                        = i_act * gain_blk_ispp_stride  + j_act + dst_stride;
                    idx_isp                         = i_act * gain_blk_isp_stride   + (j_act + dst_stride) * 2;


                    gain_isp_cur                    = MIN(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);
                    gain_isp_cur_y                  = MAX((gain_isp_cur * frame_limit_div_y + 128) >> 8, 1);
                    gain_isp_cur_uv                 = MAX((gain_isp_cur * frame_limit_div_uv + 128) >> 8, 1);

                    if(img_ds_size_x == 8)
                    {
                        idx_ratio                       = i_act * img_blk_isp_stride                + (j_act + dst_stride) * 1;
                        ratio_cur                       = ratio[idx_ratio];

                    }
                    else
                    {
                        idx_ratio                       = i_act * img_blk_isp_stride                + (j_act + dst_stride) * 2;
                        ratio_cur                       = MAX(ratio[idx_ratio], ratio[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                    }


                    if(wr_flg)
                    {
                        test_buff_ori[0][idx_ispp]  = src[idx_gain + 0];
                        test_buff_ori[1][idx_ispp]  = src[idx_gain + 2];
                        test_buff_ori[2][idx_ispp]  = src[idx_gain + 4];
                        test_buff_ori[3][idx_ispp]  = src[idx_gain + 6];
                    }

                    src[idx_gain + 0]               = MIN(255, ROUND_F((src[idx_gain + 0]    << ratio_shf_bit) / ratio_cur));
                    src[idx_gain + 2]               = MIN(255, ROUND_F((src[idx_gain + 2]    << ratio_shf_bit) / ratio_cur));
                    src[idx_gain + 4]               = MIN(255, ROUND_F((src[idx_gain + 4]    << ratio_shf_bit) / ratio_cur));
                    src[idx_gain + 6]               = MIN(255, ROUND_F((src[idx_gain + 6]    << ratio_shf_bit) / ratio_cur));


#if 1
                    src[idx_gain + 0]               = MAX(gain_isp_cur_y,   src[idx_gain + 0]);
                    src[idx_gain + 2]               = MAX(gain_isp_cur_uv,  src[idx_gain + 2]);
                    src[idx_gain + 4]               = MAX(gain_isp_cur_y,   src[idx_gain + 4]);
                    src[idx_gain + 6]                   = MAX(gain_isp_cur_uv,  src[idx_gain + 6]);
#endif
                    if(img_ds_size_x == 8)
                        ratio_nxt                       = ratio_next[idx_ratio];//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                    else
                        ratio_nxt                       = MAX(ratio_next[idx_ratio], ratio_next[idx_ratio + 1]);//ROUND_INT(ratio[idx_ratio] + ratio[idx_ratio + 1], 1);
                    if(wr_flg)
                    {
                        src_mid[idx_gain + 0]       = src[idx_gain + 0];
                        src_mid[idx_gain + 2]       = src[idx_gain + 2];
                        src_mid[idx_gain + 4]       = src[idx_gain + 4];
                        src_mid[idx_gain + 6]       = src[idx_gain + 6];

                        test_buff_mid[0][idx_ispp]  = src[idx_gain + 0];
                        test_buff_mid[1][idx_ispp]  = src[idx_gain + 2];
                        test_buff_mid[2][idx_ispp]  = src[idx_gain + 4];
                        test_buff_mid[3][idx_ispp]  = src[idx_gain + 6];
                    }

                    if(gain_ratio_shf_bits > 0)
                    {

                        src[idx_gain + 0]               = MIN(255, (src[idx_gain + 0]    << gain_ratio_shf_bits_abs) );
                        src[idx_gain + 2]               = MIN(255, (src[idx_gain + 2]    << gain_ratio_shf_bits_abs) );
                        src[idx_gain + 4]               = MIN(255, (src[idx_gain + 4]    << gain_ratio_shf_bits_abs) );
                        src[idx_gain + 6]               = MIN(255, (src[idx_gain + 6]    << gain_ratio_shf_bits_abs) );
                    }
                    else
                    {

                        src[idx_gain + 0]               = MAX(1, ROUND(src[idx_gain + 0]    >> gain_ratio_shf_bits_abs) );
                        src[idx_gain + 2]               = MAX(1, ROUND(src[idx_gain + 2]    >> gain_ratio_shf_bits_abs) );
                        src[idx_gain + 4]               = MAX(1, ROUND(src[idx_gain + 4]    >> gain_ratio_shf_bits_abs) );
                        src[idx_gain + 6]               = MAX(1, ROUND(src[idx_gain + 6]    >> gain_ratio_shf_bits_abs) );

                    }

                    if(wr_flg)
                    {
                        test_buff_mid1[0][idx_ispp]  = src[idx_gain + 0];
                        test_buff_mid1[1][idx_ispp]  = src[idx_gain + 2];
                        test_buff_mid1[2][idx_ispp]  = src[idx_gain + 4];
                        test_buff_mid1[3][idx_ispp]  = src[idx_gain + 6];
                    }

                    if(ratio_nxt > ratio_nxt_th)
                        ratio_nxt                       = ratio_nxt;
                    else
                        ratio_nxt                       = ((uint32_t)ratio_nxt * mfnr_gain_scale_fix + (1 << (GAIN_SCALE_MOTION_BITS - 1))) >> GAIN_SCALE_MOTION_BITS;


                    src[idx_gain + 0]                   = ROUND_INT(src[idx_gain + 0] * ratio_nxt, ratio_shf_bit);// + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                    src[idx_gain + 2]                   = ROUND_INT(src[idx_gain + 2] * ratio_nxt, ratio_shf_bit);//(src[idx_gain + 2] * ratio_nxt + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                    src[idx_gain + 4]                   = ROUND_INT(src[idx_gain + 4] * ratio_nxt, ratio_shf_bit);//(src[idx_gain + 4] * ratio_nxt + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;
                    src[idx_gain + 6]                   = ROUND_INT(src[idx_gain + 6] * ratio_nxt, ratio_shf_bit);//(src[idx_gain + 6] * ratio_nxt + (1 << (ratio_shf_bit - 1))) >> ratio_shf_bit;



                    src[idx_gain + 0]               = MAX(gain_min_val,    src[idx_gain + 0]);
                    src[idx_gain + 2]               = MAX(gain_min_val,    src[idx_gain + 2]);
                    src[idx_gain + 4]               = MAX(gain_min_val,    src[idx_gain + 4]);
                    src[idx_gain + 6]               = MAX(gain_min_val,    src[idx_gain + 6]);
                    if(wr_flg)
                    {

                        test_buff[0][idx_ispp]      = src[idx_gain + 0];
                        test_buff[1][idx_ispp]      = src[idx_gain + 2];
                        test_buff[2][idx_ispp]      = src[idx_gain + 4];
                        test_buff[3][idx_ispp]      = src[idx_gain + 6];
                    }


                }
            }
#else
            uint8_t gain_ratio_shf_bits_bsl = (gain_ratio_shf_bits > 0) * 0xff;
            int idx_ratio0;
            if(img_ds_size_x == 8)
                idx_ratio0              = tile_i_ispp * img_blk_isp_stride          + tile_j_ispp * 1;

            else
                idx_ratio0              = tile_i_ispp * img_blk_isp_stride          + tile_j_ispp * 2;

            uint8_t *pSrc00             = src               + tile_off;
            uint8_t *pSrc00_st          = src               + tile_off;
            uint8_t *ratio_addr         = ratio             + idx_ratio0;
            uint8_t *ratio_next_addr    = ratio_next        + idx_ratio0;
            uint8_t *pGainIsp00         = gain_isp_buf_cur  + tile_i_ispp * gain_blk_isp_stride   + tile_j_ispp * 2;



            uint16_t                    ratio0, ratio1, ratio2, ratio3;
            uint16_t                    ratio_nxt0, ratio_nxt1, ratio_nxt2, ratio_nxt3;

            uint8x8x2_t                 ratio_u8, ratio_nxt_u8;
            uint8x8_t                   ratio_nxt_u8_flg;
            uint16x8_t                  ratio_nxt_u16;
            uint16x4_t                  ratio_u16;
            uint8x8x2_t                 vSrc0, vSrc;
            uint8x8x2_t                 vGainIsp00;
            uint16x8_t                  frame_limit_reg = vcombine_u16(vdup_n_u16(frame_limit_div_y), vdup_n_u16(frame_limit_div_uv));
            //  int16x8_t frame_limit_reg1=frame_limit_reg;

            for(uint16_t y = 0; y < block_h_cur; y++)
            {

                if(j == 0x72 && y == 10)
                    j = j;
                //??????,??ratio?gain??32???????,,??gain??16??,???????16???,ratio,gain?vld2???16
                ratio_u8                    = vld2_u8(ratio_addr);
                ratio_nxt_u8                = vld2_u8(ratio_next_addr);
                vGainIsp00                  = vld2_u8(pGainIsp00);
                vSrc0                       = vld2_u8(pSrc00);

                if(img_ds_size_x == 8)
                {

                    ratio_u8                = vzip_u8(ratio_u8.val[0],      ratio_u8.val[1]);
                    ratio_nxt_u8            = vzip_u8(ratio_nxt_u8.val[0],  ratio_nxt_u8.val[1]);
                }
                else
                {
                    ratio_u8.val[0]         = vmax_u8(ratio_u8.val[0],      ratio_u8.val[1]);
                    ratio_nxt_u8.val[0]     = vmax_u8(ratio_nxt_u8.val[0],  ratio_nxt_u8.val[1]);
                }
                ratio_u16                   = vget_low_u16(vmovl_u8(ratio_u8.val[0]));

                ratio_nxt_u8_flg            = vcgt_u8(ratio_nxt_u8.val[0], vdup_n_u8(ratio_nxt_th));

                ratio_nxt_u16               = vmull_u8(ratio_nxt_u8.val[0], vbsl_u8(ratio_nxt_u8_flg,     vdup_n_u8((1 << GAIN_SCALE_MOTION_BITS)), vdup_n_u8(mfnr_gain_scale_fix)));
                ratio_nxt_u16               = vrshrq_n_u16(ratio_nxt_u16,    GAIN_SCALE_MOTION_BITS);
                float32x4_t                 ratio_f32;
                float32x4_t                 reciprocal_ratio;

                ratio_f32                   = vcvtq_f32_u32(vmovl_u16(ratio_u16));
                reciprocal_ratio            = vrecpeq_f32(ratio_f32);
                reciprocal_ratio            = vmulq_f32(vrecpsq_f32(ratio_f32, reciprocal_ratio), reciprocal_ratio);


                uint16x8x2_t                vSrc_u16;
                float32x4x4_t               vSrc_f32;
                uint32x4x4_t                vSrc_u32;


                vSrc_u16.val[0]             = vmovl_u8(vSrc0.val[0]);
                vSrc_u16.val[1]             = vmovl_u8(vSrc0.val[1]);

                //+1 is for float rounding
                // 2 bblock is a tile, vSrc_u16 val 0 is block 0 of each tile, 1 is block of each tile
                vSrc_f32.val[0]             = vcvtq_f32_u32(vshll_n_u16(vget_low_u16   (vSrc_u16.val[0]), RATIO_BITS_NUM + 1));
                vSrc_f32.val[1]             = vcvtq_f32_u32(vshll_n_u16(vget_high_u16  (vSrc_u16.val[0]), RATIO_BITS_NUM + 1));
                vSrc_f32.val[2]             = vcvtq_f32_u32(vshll_n_u16(vget_low_u16   (vSrc_u16.val[1]), RATIO_BITS_NUM + 1));
                vSrc_f32.val[3]             = vcvtq_f32_u32(vshll_n_u16(vget_high_u16  (vSrc_u16.val[1]), RATIO_BITS_NUM + 1));

                // one ratio for two low & high data
                vSrc_u32.val[0]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[0], vgetq_lane_f32(reciprocal_ratio, 0)));
                vSrc_u32.val[1]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[1], vgetq_lane_f32(reciprocal_ratio, 2)));
                vSrc_u32.val[2]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[2], vgetq_lane_f32(reciprocal_ratio, 1)));
                vSrc_u32.val[3]             = vcvtq_u32_f32(vmulq_n_f32(vSrc_f32.val[3], vgetq_lane_f32(reciprocal_ratio, 3)));

                //+1 is for float rounding
                vSrc_u16.val[0]             = vcombine_u16(vmovn_u32(vSrc_u32.val[0]), vmovn_u32(vSrc_u32.val[1]));
                vSrc_u16.val[1]             = vcombine_u16(vmovn_u32(vSrc_u32.val[2]), vmovn_u32(vSrc_u32.val[3]));
                vSrc.val[0]                 = vqrshrn_n_u16(vSrc_u16.val[0],  1);
                vSrc.val[1]                 = vqrshrn_n_u16(vSrc_u16.val[1],  1);


                //
                uint32x4x2_t                tmpVacc00, tmpVacc01;
                uint16x8_t                  tmpVacc00_u16, tmpVacc01_u16;
                uint8x8x2_t                 tmpVacc00_u8;
                uint16x8_t                  vMaxGainIsp00;


                // gain_isp_cur                = MIN(gain_isp_buf_cur[idx_isp], gain_isp_buf_cur[idx_isp + 1]);
                vGainIsp00.val[0]           = vmin_u8(vGainIsp00.val[0], vGainIsp00.val[1]);
                uint8x8_t flag_h;
                flag_h                      = vcgt_u8(vdup_n_u8(block_h_cur), vdup_n_u8(y));
                vGainIsp00.val[0]           = vbsl_u8(flag_h,                   vGainIsp00.val[0],        vdup_n_u8(GAIN_MIN_VAL));


                vMaxGainIsp00               = vmovl_u8(vGainIsp00.val[0]);
                uint16x4_t                  tmpGain, tmpGain1;
                // gain 0 1 2 3 to 0 2 1 3 ,y and uv is y 0 2 1 3 uv 4 6 5 7
                tmpGain                     = vget_low_u16(vMaxGainIsp00);
                tmpGain                     = vset_lane_u16(vgetq_lane_u16(vMaxGainIsp00, 2), tmpGain, 1);
                tmpGain                     = vset_lane_u16(vgetq_lane_u16(vMaxGainIsp00, 1), tmpGain, 2);

                vMaxGainIsp00               = vcombine_u16(tmpGain, tmpGain);


                // y0, y1, y2, y3 uv0, uv1, uv2, uv3
                tmpVacc00_u16               = vmulq_u16(vMaxGainIsp00,      frame_limit_reg);
                tmpVacc00_u8.val[0]         = vqrshrn_n_u16(tmpVacc00_u16,  8);
                uint16x4_t                  vGain_isp_cur_y, vGain_isp_cur_uv;
                tmpVacc00_u8.val[0]         = vmax_u8(tmpVacc00_u8.val[0], vdup_n_u8(GAIN_MIN_VAL));
                //  0 2 1 3 uv 4 6 5 7 to  00 44 22 66 11 55 33 77

                tmpVacc00_u8                = vzip_u8(tmpVacc00_u8.val[0], tmpVacc00_u8.val[0]);
                //  0 4 0 4 2 6 2 6 1 5 1 5 3 7 3 7
                tmpVacc00_u8                = vzip_u8(tmpVacc00_u8.val[0], tmpVacc00_u8.val[1]);
                // vSrc 0 y uv y uv y uv y uv equal to 0 4 0 4 2 6 2 6 1 5 1 5 3 7 3 7

                vSrc.val[0]                 = vmax_u8(vSrc.val[0],                          tmpVacc00_u8.val[0]);
                vSrc.val[1]                 = vmax_u8(vSrc.val[1],                          tmpVacc00_u8.val[1]);


                vSrc_u16.val[0]             = vrshlq_u16(vmovl_u8(vSrc.val[0]),             vdupq_n_s16(gain_ratio_shf_bits));
                vSrc_u16.val[0]             = vminq_u16(vSrc_u16.val[0],                    vdupq_n_u16(255));
                vSrc_u16.val[0]             = vmaxq_u16(vSrc_u16.val[0],                    vdupq_n_u16(1));
                vSrc_u16.val[1]             = vrshlq_u16(vmovl_u8(vSrc.val[1]),             vdupq_n_s16(gain_ratio_shf_bits));
                vSrc_u16.val[1]             = vminq_u16(vSrc_u16.val[1],                    vdupq_n_u16(255));
                vSrc_u16.val[1]             = vmaxq_u16(vSrc_u16.val[1],                    vdupq_n_u16(1));


                tmpVacc00_u16               = vmulq_u16(vSrc_u16.val[0], vcombine_u16(vdup_n_u16(vgetq_lane_u16(ratio_nxt_u16, 0)), vdup_n_u16(vgetq_lane_u16(ratio_nxt_u16, 2))));
                tmpVacc01_u16               = vmulq_u16(vSrc_u16.val[1], vcombine_u16(vdup_n_u16(vgetq_lane_u16(ratio_nxt_u16, 1)), vdup_n_u16(vgetq_lane_u16(ratio_nxt_u16, 3))));


                ratio_addr                  += img_blk_isp_stride;
                ratio_next_addr             += img_blk_isp_stride;
                pSrc00                      += gain_tile_ispp_x * gainkg_unit;
                pGainIsp00                  += gain_blk_isp_stride;

                vSrc.val[0]                 = vrshrn_n_u16(tmpVacc00_u16, RATIO_BITS_NUM);
                vSrc.val[1]                 = vrshrn_n_u16(tmpVacc01_u16, RATIO_BITS_NUM);
                vSrc.val[0]                 = vmax_u8(vSrc.val[0], vdup_n_u8(GAIN_MIN_VAL));
                vSrc.val[1]                 = vmax_u8(vSrc.val[1], vdup_n_u8(GAIN_MIN_VAL));

                vst2_u8(pSrc00_st, vSrc);
                pSrc00_st                   += gain_tile_ispp_x * gainkg_unit;
            }


#endif
        }
    }



    {
        FILE *fd_gainkg_out              = NULL;
        FILE *fd_ratio_nxt_wr            = NULL;
        FILE *fd_gainkg_mid_out          = NULL;
        FILE *fd_gainkg_up_out           = NULL;
        FILE *fd_gainkg_up_sp_out        = NULL;
        FILE *fd_gainkg_mid_sp_out       = NULL;
        FILE *fd_gainkg_mid1_sp_out      = NULL;
        FILE *fd_gainkg_sp_out           = NULL;
        if(wr_flg)
        {
            if(fd_ratio_nxt_wr == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_ratio_nxt_wr             = fopen(strcat(name_tmp, "ratio_nxt_out.yuv"), fop_mode);
                fwrite(ratio_next, img_blk_isp_stride * gain_kg_tile_h_align, 1,    fd_ratio_nxt_wr);
                fclose(fd_ratio_nxt_wr);
            }
            if(fd_gainkg_out == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_gainkg_out               = fopen(strcat(name_tmp, "gainkg_out.yuv"), fop_mode);
                fwrite(src_ori, gain_tile_gainkg_stride * gain_tile_gainkg_h,  1, fd_gainkg_out);
                fclose(fd_gainkg_out);
            }
            if(fd_gainkg_mid_out == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_gainkg_mid_out           = fopen(strcat(name_tmp, "gainkg_mid_out.yuv"), fop_mode);
                fwrite(src_mid, gain_tile_gainkg_stride * gain_tile_gainkg_h, 1, fd_gainkg_mid_out);
                fclose(fd_gainkg_mid_out);
            }

            if(fd_gainkg_up_out == NULL)
            {
                strcpy(name_tmp, path_mt_test);
                fd_gainkg_up_out            = fopen(strcat(name_tmp, "gainkg_up_out.yuv"), fop_mode);
                fwrite(src,     gain_tile_gainkg_stride * gain_tile_gainkg_h, 1, fd_gainkg_up_out);
                fclose(fd_gainkg_up_out);
            }

            if (1)
            {
                if(fd_gainkg_up_sp_out == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_gainkg_up_sp_out     = fopen(strcat(name_tmp, "gainkg_up_sp_out.yuv"),   fop_mode);
                }

                if(fd_gainkg_mid_sp_out == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_gainkg_mid_sp_out    = fopen(strcat(name_tmp, "gainkg_mid_sp_out.yuv"),  fop_mode);
                }
                if(fd_gainkg_mid1_sp_out == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_gainkg_mid1_sp_out   = fopen(strcat(name_tmp, "gainkg_mid1_sp_out.yuv"), fop_mode);
                }
                if(fd_gainkg_sp_out == NULL)
                {
                    strcpy(name_tmp, path_mt_test);
                    fd_gainkg_sp_out        = fopen(strcat(name_tmp, "gainkg_sp_out.yuv"),      fop_mode);
                }
                for(int i = 0; i < 4; i++)
                {
                    if(fd_gainkg_up_sp_out)
                    {
                        fwrite(test_buff[i],       gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_up_sp_out);
                        fflush(fd_gainkg_up_sp_out);
                    }
                    if(fd_gainkg_mid_sp_out)
                    {
                        fwrite(test_buff_mid[i],   gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_mid_sp_out);
                        fflush(fd_gainkg_mid_sp_out);
                    }
                    if(fd_gainkg_mid1_sp_out)
                    {
                        fwrite(test_buff_mid1[i],   gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_mid1_sp_out);
                        fflush(fd_gainkg_mid1_sp_out);
                    }
                    if(fd_gainkg_sp_out)
                    {
                        fwrite(test_buff_ori[i],   gain_blk_ispp_stride * gain_blk_ispp_h, 1, fd_gainkg_sp_out);
                        fflush(fd_gainkg_sp_out);
                    }
                }
                fclose(fd_gainkg_up_sp_out);
                fclose(fd_gainkg_mid_sp_out);
                fclose(fd_gainkg_mid1_sp_out);
                fclose(fd_gainkg_sp_out);
            }

            for(int i = 0; i < 4; i++)
            {
                if(test_buff[i])
                    free(test_buff[i]);
                if(test_buff_mid[i])
                    free(test_buff_mid[i]);
                if(test_buff_mid1[i])
                    free(test_buff_mid1[i]);
                if(test_buff_ori[i])
                    free(test_buff_ori[i]);
            }
            if(src_mid)
                free(src_mid);
            if(src_ori)
                free(src_ori);
        }
        else
        {
            fd_gainkg_out          = NULL;
            fd_gainkg_mid_out      = NULL;
            fd_gainkg_up_out       = NULL;
            fd_gainkg_up_sp_out    = NULL;
            fd_gainkg_mid_sp_out   = NULL;
            fd_gainkg_mid1_sp_out  = NULL;
            fd_gainkg_sp_out       = NULL;
            fd_ratio_nxt_wr        = NULL;
        }
    }
}

bool
Isp20SpThread::set_tnr_info(SmartPtr<rk_aiq_tnr_buf_info_s> tnr_inf){
    bool ret = true;

    // _sp_tnr_inf_t->frame_id = aiq_tnr_inf.frame_id;
    // _sp_tnr_inf_t->gainkg_size = aiq_tnr_inf.gainkg_size;
    // _sp_tnr_inf_t->gainwr_size = aiq_tnr_inf.gainwr_size;
    // _sp_tnr_inf_t->kg_fd = aiq_tnr_inf.kg_fd;
    // _sp_tnr_inf_t->wr_fd = aiq_tnr_inf.wr_fd;

    ret = _kgWrList.push(tnr_inf);

    return ret;
}

void
Isp20SpThread::init()
{
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    ispsp_stop_fds[0]                       = -1;
    ispsp_stop_fds[1]                       = -1;
    ispp_stop_fds[0]                        = -1;
    ispp_stop_fds[1]                        = -1;
    sync_pipe_fd[0]                         = -1;
    sync_pipe_fd[1]                         = -1;
    _isp_buf_num.store(0);
    _fd_init_flag.store(true);

    _motion_detect_control = false;
    _motion_detect_control_proc = false;

    _first_frame = true;
    _motion_params.stMotion = _calibDb->mfnr.mode_cell[0].motion;
    _motion_params.gain_ratio = 16.0f;
    /*----------------- motion detect algorithm parameters init---------------------*/
    max_list_num                        = 4;
    static_ratio_num                    = max_list_num + 0;

    frame_id_pp_upt                     = -1;
    frame_id_isp_upt                    = -1;
    frame_num_pp                        = 0;
    frame_num_isp                       = 0;

    gain_width                          = _gain_width;
    gain_height                         = _gain_height;
    img_width_align                     = _img_width_align;
    img_height_align                    = _img_height_align;
    if(img_ds_size_x == 8)
        img_width                       = (gain_width + 1 ) / 2;
    else
        img_width                       = gain_width;
    img_height                          = gain_height;
    img_blk_isp_stride                  = ((img_width + MT_ALIGN_STEP - 1) / MT_ALIGN_STEP) * MT_ALIGN_STEP;


    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "img size:%d %d gain size: %d %d, ds size:%d %d\n",
                    img_width, img_height, img_width_align, img_height_align,
                    img_ds_size_x, img_ds_size_y);


    img_buf_size                        = img_height_align * img_blk_isp_stride;
    img_buf_size_uv                     = img_buf_size / 2;

    gain_tile_ispp_w                    = (gain_width  + 3)        / 4;
    gain_tile_ispp_h                    = (gain_height + 15)       / 16;
    gain_tile_ispp_x                    = 2;
    gain_tile_ispp_y                    = 16;

    gain_tile_gainkg_w                  = (gain_tile_ispp_w + 1) & 0xfffe;
    gain_tile_gainkg_h                  = gain_tile_ispp_h;
    gainkg_unit                         = 8;
    gain_tile_gainkg_size               = (gain_tile_ispp_x * gain_tile_ispp_y * gainkg_unit);
    gain_tile_gainkg_stride             = gain_tile_gainkg_w * gain_tile_gainkg_size + gain_tile_gainkg_w * gain_tile_gainkg_size / 2;
    gainkg_tile_num                     = 2;
    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "gain_tile_gainkg_stride %d %d\n", gain_tile_gainkg_stride, gain_tile_gainkg_h);

    gain_blk_isp_w                      = gain_width;
    gain_blk_isp_stride                 = ((gain_blk_isp_w + 15) / 16) * 16;
    gain_blk_isp_h                      = gain_height;


    gain_blk_ispp_w                     = (gain_blk_isp_w + 1 ) / 2;
    gain_blk_ispp_stride                = ((gain_blk_ispp_w + 7) / 8) * 8;
    gain_blk_ispp_h                     = gain_height;
    gain_blk_ispp_mem_size              = gain_blk_ispp_stride * gain_blk_ispp_h;


    LOGE_CAMHW_SUBM(MOTIONDETECT_SUBM, "gain_blk_ispp_stride %d gain_blk_ispp_h %d\n", gain_blk_ispp_stride, gain_blk_ispp_h);
    gain_kg_tile_h_align                = (gain_blk_isp_h + 15) & 0xfff0;

    static_ratio_l_bit                  = RATIO_BITS_NUM;
    static_ratio_r_bit                  = RATIO_BITS_R_NUM;
    static_ratio_l                      = 1 << static_ratio_l_bit;

    static_frame_num                    = 0;

    static_ratio_idx_in                 = 0;
    static_ratio_idx_out                = 0;
    static_ratio                        = (uint8_t**)                   malloc(static_ratio_num * sizeof(uint8_t*));
    for(int i = 0; i < static_ratio_num; i++)
    {
        static_ratio[i]                 = (uint8_t*)malloc(gain_blk_isp_stride * (gain_kg_tile_h_align + 16)      * sizeof(static_ratio[i][0]));
        memset(static_ratio[i], static_ratio_l, gain_blk_isp_stride     * gain_kg_tile_h_align);
    }

    gain_isp_buf_bak                    = (uint8_t**)                   malloc(static_ratio_num * sizeof(uint8_t*));
    for(int i = 0; i < static_ratio_num; i++)
        gain_isp_buf_bak[i]             = (uint8_t*)malloc(gain_blk_isp_stride  * (gain_kg_tile_h_align + 16)     * sizeof(gain_isp_buf_bak[i][0]));

    pPreAlpha                           = (uint8_t*)malloc(gain_blk_isp_stride  * (gain_kg_tile_h_align + 16)     * sizeof(pPreAlpha[0]));


    pImgbuf                             = (uint8_t**)                   malloc(static_ratio_num * sizeof(uint8_t*));
    for(int i = 0; i < static_ratio_num; i++)
        pImgbuf[i]                      = (uint8_t*)malloc((img_buf_size  + img_buf_size_uv + 16)    *   sizeof(pImgbuf[i][0]));

    frame_detect_flg                    = (uint8_t*)malloc(static_ratio_num * sizeof(frame_detect_flg[0]));
    for(int i = 0; i < static_ratio_num; i++)
    {
        frame_detect_flg[i]             = -1;
    }

    mtParamsSelect_list                 = (RKAnr_Mt_Params_Select_t**)malloc(static_ratio_num               * sizeof(RKAnr_Mt_Params_Select_t*));
    for(int i = 0; i < static_ratio_num; i++)
    {
        mtParamsSelect_list[i]          = (RKAnr_Mt_Params_Select_t *)malloc(static_ratio_num   *   sizeof(mtParamsSelect_list[0][0]));
        (*(mtParamsSelect_list[i])).gain_ratio = -1;
    }
    pTmpBuf                             = (int16_t*)malloc(gain_blk_isp_w       * gain_blk_isp_h * 6        * sizeof(pTmpBuf[0]));

    memset(pPreAlpha, 0, gain_blk_isp_stride         * gain_kg_tile_h_align      * sizeof(pPreAlpha[0]));

    pAfTmp                              = (uint8_t*)malloc(img_buf_size * sizeof(pAfTmp[0]) * 3 / 2);
    _af_meas_params.sp_meas.ldg_xl      = _calibDb->af.ldg_param.ldg_xl;
    _af_meas_params.sp_meas.ldg_yl      = _calibDb->af.ldg_param.ldg_yl;
    _af_meas_params.sp_meas.ldg_kl      = _calibDb->af.ldg_param.ldg_kl;
    _af_meas_params.sp_meas.ldg_xh      = _calibDb->af.ldg_param.ldg_xh;
    _af_meas_params.sp_meas.ldg_yh      = _calibDb->af.ldg_param.ldg_yh;
    _af_meas_params.sp_meas.ldg_kh      = _calibDb->af.ldg_param.ldg_kh;
    _af_meas_params.sp_meas.highlight_th  = _calibDb->af.highlight.ther0;
    _af_meas_params.sp_meas.highlight2_th = _calibDb->af.highlight.ther1;

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "Isp20SpThread::%s exit w %d h %d\n", __FUNCTION__, gain_blk_isp_w, gain_blk_isp_h);
}

void
Isp20SpThread::deinit()
{
    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s enter", __FUNCTION__);
    SmartLock locker (_buf_list_mutex);
    _isp_buf_list.clear();
    _notifyYgCmdQ.clear();
    _kgWrList.clear();
    /*------- motion detect algorithm parameters deinit--------*/
    if(frame_detect_flg) {
        free(frame_detect_flg);
        frame_detect_flg = nullptr;
    }
    for(int i = 0; i < static_ratio_num; i++)
    {
        if(static_ratio && static_ratio[i]) {
            free(static_ratio[i]);
            static_ratio[i] = nullptr;
        }
        if(pImgbuf && pImgbuf[i]) {
            free(pImgbuf[i]);
            //pImgbuf = nullptr;
        }
        if(gain_isp_buf_bak && gain_isp_buf_bak[i]) {
            free(gain_isp_buf_bak[i]);
            gain_isp_buf_bak[i] = nullptr;
        }
        if(mtParamsSelect_list && mtParamsSelect_list[i]) {
            free(mtParamsSelect_list[i]);
            mtParamsSelect_list[i] = nullptr;
        }
    }
    if(static_ratio) {
        free(static_ratio);
        static_ratio = nullptr;
    }
    if(pImgbuf) {
        free(pImgbuf);
        pImgbuf = nullptr;
    }
    if(gain_isp_buf_bak) {
        free(gain_isp_buf_bak);
        gain_isp_buf_bak = nullptr;
    }
    if(mtParamsSelect_list) {
        free(mtParamsSelect_list);
        mtParamsSelect_list = nullptr;
    }
    if(pTmpBuf) {
        free(pTmpBuf);
        pTmpBuf = nullptr;
    }
    if(pPreAlpha) {
        free(pPreAlpha);
        pPreAlpha = nullptr;
    }

    if(pAfTmp) {
        free(pAfTmp);
        pAfTmp = nullptr;
    }

    LOG1_CAMHW_SUBM(MOTIONDETECT_SUBM, "%s exit", __FUNCTION__);
}

void
Isp20SpThread::set_working_mode(int mode)
{
    _working_mode = mode;
}

bool Isp20SpThread::notify_yg_cmd(SmartPtr<sp_msg_t> msg)
{
    bool ret = true;
    if (msg->sync) {
        msg->mutex = new Mutex();
        msg->cond = new XCam::Cond();
        SmartLock lock (*msg->mutex.ptr());
        ret = _notifyYgCmdQ.push(msg);
        msg->cond->wait(*msg->mutex.ptr());
    } else {
        ret = _notifyYgCmdQ.push(msg);
    }

    return ret;
}

bool Isp20SpThread::notify_yg2_cmd(SmartPtr<sp_msg_t> msg)
{
    bool ret = true;
    if (msg->sync) {
        msg->mutex = new Mutex();
        msg->cond = new XCam::Cond();
        SmartLock lock (*msg->mutex.ptr());
        ret = _notifyYgCmdQ2.push(msg);
        msg->cond->wait(*msg->mutex.ptr());
    } else {
        ret = _notifyYgCmdQ2.push(msg);
    }

    return ret;
}

void Isp20SpThread::notify_stop_fds_exit()
{
    if (ispp_stop_fds[1] != -1) {
        char buf = 0xf;  // random value to write to flush fd.
        unsigned int size = write(ispp_stop_fds[1], &buf, sizeof(char));
        if (size != sizeof(char))
            LOGW_CAMHW_SUBM(ISP20POLL_SUBM, "Flush write not completed");
    }

    if (ispsp_stop_fds[1] != -1) {
        char buf = 0xf;  // random value to write to flush fd.
        unsigned int size = write(ispsp_stop_fds[1], &buf, sizeof(char));
        if (size != sizeof(char))
            LOGW_CAMHW_SUBM(ISP20POLL_SUBM, "Flush write not completed");
    }
}

void Isp20SpThread::notify_wr_thread_exit()
{
    _notifyYgCmdQ.clear();
    SmartPtr<sp_msg_t> msg = new sp_msg_t();
    msg->cmd = MSG_CMD_WR_EXIT;
    msg->sync = false;
    notify_yg_cmd(msg);
}

void Isp20SpThread::notify_wr2_thread_exit()
{
    _notifyYgCmdQ2.clear();
    SmartPtr<sp_msg_t> msg = new sp_msg_t();
    msg->cmd = MSG_CMD_WR_EXIT;
    msg->sync = false;
    notify_yg2_cmd(msg);
}

void Isp20SpThread::destroy_stop_fds_ispsp () {

    if (ispsp_stop_fds[0] != -1 || ispsp_stop_fds[1] != -1) {
        close(ispsp_stop_fds[0]);
        close(ispsp_stop_fds[1]);
        ispsp_stop_fds[0] = -1;
        ispsp_stop_fds[1] = -1;
    }

    if (ispp_stop_fds[0] != -1 || ispp_stop_fds[1] != -1) {
        close(ispp_stop_fds[0]);
        close(ispp_stop_fds[1]);
        ispp_stop_fds[0] = -1;
        ispp_stop_fds[1] = -1;
    }

    if (sync_pipe_fd[0] != -1 || sync_pipe_fd[1] != -1) {
        close(sync_pipe_fd[0]);
        close(sync_pipe_fd[1]);
        sync_pipe_fd[0] = -1;
        sync_pipe_fd[1] = -1;
    }
}

XCamReturn
Isp20SpThread::create_stop_fds_ispsp () {
    int i, status = 0;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    destroy_stop_fds_ispsp();

    status = pipe(ispsp_stop_fds);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Failed to create ispsp poll stop pipe: %s", strerror(errno));
        ret = XCAM_RETURN_ERROR_UNKNOWN;
        goto exit_error;
    }
    status = fcntl(ispsp_stop_fds[0], F_SETFL, O_NONBLOCK);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Fail to set event ispsp stop pipe flag: %s", strerror(errno));
        goto exit_error;
    }

    status = pipe(ispp_stop_fds);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Failed to create ispp poll stop pipe: %s", strerror(errno));
        ret = XCAM_RETURN_ERROR_UNKNOWN;
        goto exit_error;
    }
    status = fcntl(ispp_stop_fds[0], F_SETFL, O_NONBLOCK);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Fail to set event ispp stop pipe flag: %s", strerror(errno));
        goto exit_error;
    }

    status = pipe(sync_pipe_fd);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Failed to create ispp poll sync pipe: %s", strerror(errno));
        ret = XCAM_RETURN_ERROR_UNKNOWN;
        goto exit_error;
    }
    status = fcntl(sync_pipe_fd[1], F_SETFL, O_NONBLOCK);
    if (status < 0) {
        LOGE_CAMHW_SUBM(ISP20POLL_SUBM, "Fail to set event ispp sync pipe flag: %s", strerror(errno));
        goto exit_error;
    }

    return XCAM_RETURN_NO_ERROR;
exit_error:
    destroy_stop_fds_ispsp();
    return ret;
}

void Isp20SpThread::update_motion_detection_params(ANRMotionParam_t *motion)
{
    SmartLock locker (_motion_param_mutex);
    if (motion && (0 != memcmp(motion, &_motion_params, sizeof(ANRMotionParam_t)))) {
        _motion_params = *motion;
    }
}

void Isp20SpThread::update_af_meas_params(rk_aiq_af_algo_meas_t *af_meas)
{
    SmartLock locker (_afmeas_param_mutex);
    if (af_meas && (0 != memcmp(af_meas, &_af_meas_params, sizeof(rk_aiq_af_algo_meas_t)))) {
        _af_meas_params = *af_meas;
    }
}

}; //namspace RkCam
