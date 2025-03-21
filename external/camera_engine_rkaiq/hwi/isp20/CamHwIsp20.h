
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

#ifndef _CAM_HW_ISP20_H_
#define _CAM_HW_ISP20_H_

#include "CamHwBase.h"
#include "Isp20Params.h"
#include "SensorHw.h"
#include "LensHw.h"
#ifndef DISABLE_ISP20SPTHREAD
#include "Isp20SpThread.h"
#endif
#include "TnrProcUint.h"
#include "NrProcUint.h"
#include "FecProcUint.h"
struct media_device;

#define ISP20HW_SUBM (1 << 0)

namespace RkCam {

class CamHwIsp20
    : public CamHwBase, public Isp20Params, public V4l2Device
    , public isp_drv_share_mem_ops_t {
public:
    explicit CamHwIsp20();
    virtual ~CamHwIsp20();

    virtual XCamReturn set_read_back_mode(bool on);

    // from CamHwBase
    virtual XCamReturn init(const char* sns_ent_name);
    virtual XCamReturn deInit();
    virtual XCamReturn prepare(uint32_t width, uint32_t height, int mode, int t_delay, int g_delay);
    virtual XCamReturn start();
    virtual XCamReturn stop();
    virtual XCamReturn pause();
    virtual XCamReturn resume();
    virtual XCamReturn poll_event_ready (uint32_t sequence, int type);
    virtual XCamReturn poll_event_failed (int64_t timestamp, const char *msg);
    virtual XCamReturn swWorkingModeDyn(int mode);
    virtual void getIspWorkMode(int& mode) {
        if (mNormalNoReadBack)
            mode = mWorkingMode | RK_AIQ_WORKING_MODE_ISP_ONLINE;
        else
            mode = mWorkingMode;
    };
    virtual XCamReturn getSensorModeData(const char* sns_ent_name,
                                         rk_aiq_exposure_sensor_descriptor& sns_des);
    XCamReturn setIspParamsSync(int frameId);
    XCamReturn setIsppParamsSync(int frameId);
    virtual XCamReturn setIspMeasParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispMeasParams);
    virtual XCamReturn setIspOtherParams(SmartPtr<RkAiqIspOtherParamsProxy>& ispOtherParams);
    virtual XCamReturn setExposureParams(SmartPtr<RkAiqExpParamsProxy>& expPar);
    virtual XCamReturn setIrisParams(SmartPtr<RkAiqIrisParamsProxy>& irisPar, CalibDb_IrisType_t irisType);
    virtual XCamReturn setHdrProcessCount(rk_aiq_luma_params_t luma_params);
    virtual XCamReturn setFocusParams(SmartPtr<RkAiqFocusParamsProxy>& focus_params);
    virtual XCamReturn setCpslParams(SmartPtr<RkAiqCpslParamsProxy>& cpsl_params);
    virtual XCamReturn setIsppMeasParams(SmartPtr<RkAiqIsppMeasParamsProxy>& isppParams);
    virtual XCamReturn setIsppOtherParams(SmartPtr<RkAiqIsppOtherParamsProxy>& isppOtherParams);
    virtual XCamReturn setFecParams(SmartPtr<RkAiqFecParamsProxy>& fecParams);
    virtual XCamReturn setTnrParams(SmartPtr<RkAiqTnrParamsProxy>& tnrParams);
    static rk_aiq_static_info_t* getStaticCamHwInfo(const char* sns_ent_name, uint16_t index = 0);
    static rk_aiq_static_info_t* getStaticCamHwInfoByPhyId(
        const char* sns_ent_name, uint16_t index = 0);
    static XCamReturn clearStaticCamHwInfo();
    static void findAttachedSubdevs(struct media_device *device, uint32_t count, rk_sensor_full_info_t *s_info);
    static XCamReturn initCamHwInfos();
    XCamReturn setupHdrLink(int mode, int isp_index, bool enable);
    static XCamReturn selectIqFile(const char* sns_ent_name, char* iqfile_name);
    static const char* getBindedSnsEntNmByVd(const char* vd);
    XCamReturn setExpDelayInfo(int mode);
    XCamReturn getEffectiveIspParams(SmartPtr<RkAiqIspMeasParamsProxy>& ispParams, int frame_id);
    XCamReturn setModuleCtl(rk_aiq_module_id_t moduleId, bool en);
    XCamReturn getModuleCtl(rk_aiq_module_id_t moduleId, bool& en);
    XCamReturn notify_capture_raw();
    XCamReturn capture_raw_ctl(capture_raw_t type, int count = 0, const char* capture_dir = nullptr, char* output_dir = nullptr);
    uint64_t getIspModuleEnState();
    void setHdrGlobalTmoMode(int frame_id, bool mode);
    XCamReturn setSharpFbcRotation(rk_aiq_rotation_t rot) {
        _sharp_fbc_rotation = rot;
        return XCAM_RETURN_NO_ERROR;
    }
    XCamReturn getSensorCrop(rk_aiq_rect_t& rect);
    XCamReturn setSensorCrop(rk_aiq_rect_t& rect);
    XCamReturn setSensorFlip(bool mirror, bool flip, int skip_frm_cnt);
    XCamReturn getSensorFlip(bool& mirror, bool& flip);
    void setMulCamConc(bool cc);
    XCamReturn getZoomPosition(int& position);
    XCamReturn getLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg);
    XCamReturn setLensVcmCfg(rk_aiq_lens_vcmcfg& lens_cfg);
    XCamReturn setLensVcmCfg();
    XCamReturn FocusCorrection();
    XCamReturn ZoomCorrection();
    virtual void getShareMemOps(isp_drv_share_mem_ops_t** mem_ops);
    XCamReturn getEffectiveExpParams(uint32_t id, SmartPtr<RkAiqExpParamsProxy>& ExpParams);
    XCamReturn getEffectiveExpParams(uint32_t id, SmartPtr<RkAiqExpParamsProxy>& expParams, uint64_t &timestamp);
protected:
    XCAM_DEAD_COPY(CamHwIsp20);
    enum cam_hw_state_e {
        CAM_HW_STATE_INVALID,
        CAM_HW_STATE_INITED,
        CAM_HW_STATE_PREPARED,
        CAM_HW_STATE_STARTED,
        CAM_HW_STATE_PAUSED,
        CAM_HW_STATE_STOPPED,
    };
    enum ircut_state_e {
        IRCUT_STATE_CLOSED, /* close ir-cut,meaning that infrared ray can be received */
        IRCUT_STATE_CLOSING,
        IRCUT_STATE_OPENING,
        IRCUT_STATE_OPENED, /* open ir-cut,meaning that only visible light can be received */
    };

    //for api to set
    uint16_t _isp_tx_buf_num;
    //for api to set
    uint16_t _vicap_tx_buf_num;
    XCamReturn set_tx_buf_num(uint16_t buf_num);

    int _hdr_mode;
    Mutex _mutex;
    int _state;
    volatile bool _is_exit;
    bool _linked_to_isp;
    bool _dvp_itf;
    SmartPtr<RkAiqIspMeasParamsProxy> _lastAiqIspMeasResult;
    SmartPtr<RkAiqIspOtherParamsProxy> _lastAiqIspOtherResult;
    SmartPtr<RkAiqIsppMeasParamsProxy> _lastAiqIsppMeasResult;
    SmartPtr<RkAiqIsppOtherParamsProxy> _lastAiqIsppOtherResult;
    struct isp2x_isp_params_cfg _full_active_isp_params;
    struct rkispp_params_cfg _full_active_ispp_params;
    uint32_t _ispp_module_init_ens;
    SmartPtr<V4l2Device> _mipi_tx_devs[3];
    SmartPtr<V4l2Device> _mipi_rx_devs[3];
    SmartPtr<V4l2SubDevice> _ispp_sd;
    SmartPtr<V4l2SubDevice> _cif_csi2_sd;
    char sns_name[32];
    std::list<SmartPtr<RkAiqIspMeasParamsProxy>> _pending_isp_meas_params_queue;
    std::list<SmartPtr<RkAiqIspOtherParamsProxy>> _pending_isp_other_params_queue;
    std::list<SmartPtr<RkAiqIsppMeasParamsProxy>> _pending_ispp_meas_params_queue;
    std::list<SmartPtr<RkAiqIsppOtherParamsProxy>> _pending_ispp_other_params_queue;
    std::map<int, SmartPtr<RkAiqIspMeasParamsProxy>> _effectingIspMeasParmMap;
    static std::map<std::string, SmartPtr<rk_aiq_static_info_t>> mCamHwInfos;
    static rk_aiq_isp_hw_info_t mIspHwInfos;
    static rk_aiq_cif_hw_info_t mCifHwInfos;
    static std::map<std::string, SmartPtr<rk_sensor_full_info_t>> mSensorHwInfos;
    void gen_full_isp_params(const struct isp2x_isp_params_cfg* update_params,
                             struct isp2x_isp_params_cfg* full_params);
    void gen_full_ispp_params(const struct rkispp_params_cfg* update_params,
                              struct rkispp_params_cfg* full_params);
    XCamReturn overrideExpRatioToAiqResults(const sint32_t frameId,
                                            int module_id,
                                            SmartPtr<RkAiqIspMeasParamsProxy>& aiq_results,
                                            SmartPtr<RkAiqIspOtherParamsProxy>& aiq_other_results);
    void dump_isp_config(struct isp2x_isp_params_cfg* isp_params,
                         SmartPtr<RkAiqIspMeasParamsProxy> aiq_results,
                         SmartPtr<RkAiqIspOtherParamsProxy> aiq_other_results);
    void dumpRawnrFixValue(struct isp2x_rawnr_cfg * pRawnrCfg );
    void dumpTnrFixValue(struct rkispp_tnr_config  * pTnrCfg);
    void dumpUvnrFixValue(struct rkispp_nr_config  * pNrCfg);
    void dumpYnrFixValue(struct rkispp_nr_config  * pNrCfg);
    void dumpSharpFixValue(struct rkispp_sharp_config  * pSharpCfg);
    XCamReturn setIrcutParams(bool on);
    XCamReturn setIsppSharpFbcRot(struct rkispp_sharp_config* shp_cfg);
    XCamReturn setupPipelineFmt();
    XCamReturn setupPipelineFmtIsp(struct v4l2_subdev_selection& sns_sd_sel,
                                   struct v4l2_subdev_format& sns_sd_fmt,
                                   __u32 sns_v4l_pix_fmt);
    XCamReturn setupPipelineFmtCif(struct v4l2_subdev_selection& sns_sd_sel,
                                   struct v4l2_subdev_format& sns_sd_fmt,
                                   __u32 sns_v4l_pix_fmt);
    XCamReturn setupHdrLink_vidcap(int hdr_mode, int cif_index, bool enable);
    enum mipi_stream_idx {
        MIPI_STREAM_IDX_0   = 1,
        MIPI_STREAM_IDX_1   = 2,
        MIPI_STREAM_IDX_2   = 4,
        MIPI_STREAM_IDX_ALL = 7,
    };
    XCamReturn hdr_mipi_start(int idx = MIPI_STREAM_IDX_ALL);
    XCamReturn hdr_mipi_start_mode(int mode);
    XCamReturn hdr_mipi_stop(int idx = MIPI_STREAM_IDX_ALL);
    XCamReturn hdr_mipi_prepare_mode(int mode);
    XCamReturn hdr_mipi_prepare(int idx);
    void prepare_cif_mipi();
    static void allocMemResource(void *ops_ctx, void *config, void **mem_ctx);
    static void releaseMemResource(void *mem_ctx);
    static void* getFreeItem(void *mem_ctx);
    void prepare_motion_detection(int mode);
    uint32_t _isp_module_ens;
    bool mNormalNoReadBack;
    uint64_t ispModuleEns;
    rk_aiq_rotation_t _sharp_fbc_rotation;

    rk_aiq_ldch_share_mem_info_t ldch_mem_info_array[ISP2X_LDCH_BUF_NUM];
    rk_aiq_fec_share_mem_info_t fec_mem_info_array[FEC_MESH_BUF_NUM];
    typedef struct drv_share_mem_ctx_s {
        void* ops_ctx;
        void* mem_info;
        rk_aiq_drv_share_mem_type_t type;
    } drv_share_mem_ctx_t;
    drv_share_mem_ctx_t _ldch_drv_mem_ctx;
    drv_share_mem_ctx_t _fec_drv_mem_ctx;
    Mutex _mem_mutex;
    rk_aiq_rect_t _crop_rect;



    uint32_t _gain_width;
    uint32_t _gain_height;
    uint32_t _ds_width;
    uint32_t _ds_heigth;
    uint32_t _ds_width_align;
    uint32_t _ds_heigth_align;
    uint32_t _exp_delay;
    rk_aiq_lens_descriptor _lens_des;

    SmartPtr<TnrProcUint>       mTnrProcUint;
    SmartPtr<NrProcUint>        mNrProcUint;
    SmartPtr<FecProcUint>       mFecProcUint;
    uint32_t mPpModuleInitEns = 0;
    void analyzePpInitEns(SmartPtr<cam3aResult> result);
};

};
#endif
