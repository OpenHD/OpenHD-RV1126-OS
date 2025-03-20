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

#include "shared_item_pool.h"
#include "rk_aiq_types_priv.h"

#ifndef RK_AIQ_POOL_H
#define RK_AIQ_POOL_H

namespace RkCam {

#if 0
class RkAiqPartResults {
public:
    explicit RkAiqPartResults() {
        mIspMeasMeasParams = NULL;
        mExposureParams = NULL;
        mFocusParams = NULL;
    };
    ~RkAiqPartResults() {};

    rk_aiq_isp_meas_params_t* mIspMeasParams;
    rk_aiq_exposure_params_comb_t* mExposureParams;
    rk_aiq_focus_params_t* mFocusParams;

private:
    XCAM_DEAD_COPY (RkAiqPartResults);
};
#endif
typedef struct RKAiqAecExpInfoWrapper_s {
    RKAiqAecExpInfo_t aecExpInfo;
    RKAiqAecExpInfo_t exp_tbl[MAX_AEC_EFFECT_FNUM + 1];
    Sensor_dpcc_res_t SensorDpccInfo;
    int exp_tbl_size;
    int algo_id;
} RKAiqAecExpInfoWrapper_t;

typedef struct RKAiqAfInfoWrapper_s {
    struct timeval focusStartTim;
    struct timeval focusEndTim;
    struct timeval zoomStartTim;
    struct timeval zoomEndTim;
    int64_t sofTime;
    int32_t focusCode;
    int32_t zoomCode;
    uint32_t lowPassId;
    int32_t lowPassFv4_4[RKAIQ_RAWAF_SUMDATA_NUM];
    int32_t lowPassFv8_8[RKAIQ_RAWAF_SUMDATA_NUM];
    int32_t lowPassHighLht[RKAIQ_RAWAF_SUMDATA_NUM];
    int32_t lowPassHighLht2[RKAIQ_RAWAF_SUMDATA_NUM];
    bool zoomCorrection;
    bool focusCorrection;
} RKAiqAfInfoWrapper_t;

typedef struct RkAiqPirisInfoWrapper_s {
    int             step;
    int             laststep;
    bool            update;
    struct timeval  StartTim;
    struct timeval  EndTim;
} RkAiqPirisInfoWrapper_t;

typedef struct RkAiqIrisInfoWrapper_s {
    RkAiqPirisInfoWrapper_t   PIris;
    RkAiqDCIrisParam_t        DCIris;
    uint64_t                  sofTime;
} RkAiqIrisInfoWrapper_t;

typedef struct RKAiqCpslInfoWrapper_s {
    rk_aiq_flash_setting_t fl;
    bool update_fl;
    rk_aiq_ir_setting_t ir;
    rk_aiq_flash_setting_t fl_ir;
    bool update_ir;
} RKAiqCpslInfoWrapper_t;

typedef enum RkAiqParamsType_e {
    RK_AIQ_PARAMS_ALL,
    RK_AIQ_PARAMS_MEAS,
    RK_AIQ_PARAMS_OTHER,
} RkAiqParamsType_t;

typedef enum _cam3aResultType {
	RESULT_TYPE_INVALID = -1,
	RESULT_TYPE_ANR,
	RESULT_TYPE_AMD,
	RESULT_TYPE_ATNR,
	RESULT_TYPE_AFEC,
	RESULT_TYPE_AORB,
	RESULT_TYPE_MAX_PARAM,
} cam3aResultType;

static const char* Cam3aResultType2Str[RESULT_TYPE_MAX_PARAM] = {
    [RESULT_TYPE_ANR]       = "ANR",
    [RESULT_TYPE_AMD]       = "MOTION_DETECTION",
    [RESULT_TYPE_ATNR]      = "ATNR",
    [RESULT_TYPE_AFEC]      = "AFEC",
    [RESULT_TYPE_AORB]      = "AORB",
};


// typedef rk_aiq_isp_params_base_t ispParamsBase;
typedef SharedItemBase cam3aResult;

typedef RKAiqAecExpInfoWrapper_t rk_aiq_exposure_params_wrapper_t;
typedef RKAiqAfInfoWrapper_t rk_aiq_af_info_wrapper_t;
typedef RkAiqIrisInfoWrapper_t rk_aiq_iris_params_wrapper_t;

typedef SharedItemPool<rk_aiq_exposure_params_wrapper_t> RkAiqExpParamsPool;
typedef SharedItemProxy<rk_aiq_exposure_params_wrapper_t> RkAiqExpParamsProxy;
typedef SharedItemPool<rk_aiq_iris_params_wrapper_t> RkAiqIrisParamsPool;
typedef SharedItemProxy<rk_aiq_iris_params_wrapper_t> RkAiqIrisParamsProxy;
typedef SharedItemPool<rk_aiq_af_info_wrapper_t> RkAiqAfInfoPool;
typedef SharedItemProxy<rk_aiq_af_info_wrapper_t> RkAiqAfInfoProxy;
typedef SharedItemPool<rk_aiq_isp_meas_params_t> RkAiqIspMeasParamsPool;
typedef SharedItemProxy<rk_aiq_isp_meas_params_t> RkAiqIspMeasParamsProxy;
typedef SharedItemPool<rk_aiq_isp_other_params_t> RkAiqIspOtherParamsPool;
typedef SharedItemProxy<rk_aiq_isp_other_params_t> RkAiqIspOtherParamsProxy;
typedef SharedItemPool<rk_aiq_focus_params_t> RkAiqFocusParamsPool;
typedef SharedItemProxy<rk_aiq_focus_params_t> RkAiqFocusParamsProxy;
typedef SharedItemPool<rk_aiq_ispp_meas_params_t> RkAiqIsppMeasParamsPool;
typedef SharedItemProxy<rk_aiq_ispp_meas_params_t> RkAiqIsppMeasParamsProxy;
typedef SharedItemPool<rk_aiq_ispp_other_params_t> RkAiqIsppOtherParamsPool;
typedef SharedItemProxy<rk_aiq_ispp_other_params_t> RkAiqIsppOtherParamsProxy;
/* TODO: */
typedef RkAiqIsppOtherParamsProxy RkAiqNrParamsProxy;
typedef SharedItemPool<RKAiqCpslInfoWrapper_t> RkAiqCpslParamsPool;
typedef SharedItemProxy<RKAiqCpslInfoWrapper_t> RkAiqCpslParamsProxy;
typedef SharedItemPool<rk_aiq_tnr_params_t>     RkAiqTnrParamsPool;
typedef SharedItemProxy<rk_aiq_tnr_params_t>    RkAiqTnrParamsProxy;
typedef SharedItemPool<rk_aiq_fec_params_t>     RkAiqFecParamsPool;
typedef SharedItemProxy<rk_aiq_fec_params_t>    RkAiqFecParamsProxy;
typedef SharedItemPool<rk_aiq_amd_result_t>     RkAiqAmdResPool;
typedef SharedItemProxy<rk_aiq_amd_result_t>    RkAiqAmdResProxy;

class RkAiqFullParams {
public:
    explicit RkAiqFullParams()
        : mExposureParams(NULL)
        , mIspMeasParams(NULL)
        , mIspOtherParams(NULL)
        , mIsppMeasParams(NULL)
        , mIsppOtherParams(NULL)
        , mFocusParams(NULL)
        , mIrisParams(NULL)
        , mCpslParams(NULL)
        , mTnrParams(NULL)
        , mFecParams(NULL) {
    };
    ~RkAiqFullParams() {};

    void reset() {
        mExposureParams.release();
        mIspMeasParams.release();
        mIspOtherParams.release();
        mIsppMeasParams.release();
        mIsppOtherParams.release();
        mFocusParams.release();
        mIrisParams.release();
        mCpslParams.release();
        mTnrParams.release();
        mFecParams.release();
    };
    SmartPtr<RkAiqExpParamsProxy> mExposureParams;
    SmartPtr<RkAiqIspMeasParamsProxy> mIspMeasParams;
    SmartPtr<RkAiqIspOtherParamsProxy> mIspOtherParams;
    SmartPtr<RkAiqIsppMeasParamsProxy> mIsppMeasParams;
    SmartPtr<RkAiqIsppOtherParamsProxy> mIsppOtherParams;
    SmartPtr<RkAiqFocusParamsProxy> mFocusParams;
    SmartPtr<RkAiqIrisParamsProxy> mIrisParams;
    SmartPtr<RkAiqCpslParamsProxy> mCpslParams;
    SmartPtr<RkAiqTnrParamsProxy> mTnrParams;
    SmartPtr<RkAiqFecParamsProxy> mFecParams;

private:
    XCAM_DEAD_COPY (RkAiqFullParams);
};

typedef SharedItemPool<RkAiqFullParams> RkAiqFullParamsPool;

/*
 * specific template instance for RkAiqFullParams proxy.
 * When the proxy returns the _data to its owner pool, the
 * _data is not been cleared(reset). It will cause the resources
 * in _data are not been recycled.
 * RkAiqFullParams contains other proxy's SmartPtr, so we will
 * clear it explicitly befor it is returned to pool so that the
 * containd resouces can be recycled correctly.
 */
template<> class SharedItemProxy<RkAiqFullParams>
{
public:
    explicit SharedItemProxy<RkAiqFullParams>(const SmartPtr<RkAiqFullParams> &data) : _data(data) {};
    ~SharedItemProxy<RkAiqFullParams>() {
        _data->reset();
        if (_pool.ptr())
            _pool->release(_data);
        _data.release();
    };

    void set_buf_pool (SmartPtr<SharedItemPool<RkAiqFullParams>> pool) {
        _pool = pool;
    }

    SmartPtr<RkAiqFullParams> &data() {
        return _data;
    }

private:
    XCAM_DEAD_COPY (SharedItemProxy);

private:
    SmartPtr<RkAiqFullParams>                       _data;
    SmartPtr<SharedItemPool<RkAiqFullParams>>       _pool;
};

typedef SharedItemProxy<RkAiqFullParams> RkAiqFullParamsProxy;

};

#endif //RK_AIQ_POOL_H
