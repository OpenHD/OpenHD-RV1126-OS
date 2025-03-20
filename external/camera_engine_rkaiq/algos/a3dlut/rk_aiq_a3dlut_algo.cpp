/*
* alut3d.cpp

* for rockchip v2.0.0
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
/* for rockchip v2.0.0*/

#include "a3dlut/rk_aiq_a3dlut_algo.h"
#include "xcam_log.h"


RKAIQ_BEGIN_DECLARE

XCamReturn  A3dlut_get_mode_cell_idx_by_name(const CalibDb_Lut3d_t *pCalibdb, char *name, int *mode_idx)
{
    int i = 0;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if(pCalibdb == NULL){
        LOGE_A3DLUT("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    if(name == NULL){
        LOGE_A3DLUT("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    if(mode_idx == NULL){
        LOGE_A3DLUT("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    for(i=0; i<ALut3d_PARAM_MODE_MAX; i++){
        if(strcmp(name, pCalibdb->mode_cell[i].name) == 0){
            break;
        }
    }

    if(i<ALut3d_PARAM_MODE_MAX){
        *mode_idx = i;
    }else{
        *mode_idx = 0;
        LOGW_A3DLUT("%s: error!!!  can't find mode name %s in iq files, use %s instead\n", __FUNCTION__, name,
            pCalibdb->mode_cell[*mode_idx].name);
    }

    LOGD_A3DLUT("%s:%d mode_name:%s  mode_idx:%d i:%d \n", __FUNCTION__, __LINE__,name, *mode_idx, i);
    return ret;

}


XCamReturn Alut3dAutoConfig
(
    alut3d_handle_t hAlut3d
) {

    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    bool ret1 = true;

    if (hAlut3d->updateAtt) {
        hAlut3d->lastAlp = hAlut3d->mCurAtt.stAuto.alpha;
        hAlut3d->updateAtt = false;
    }

    const float curAlp = hAlut3d->_a3dlut_adjust->get_output_alp();

    if (fabs(hAlut3d->lastAlp - curAlp) > DIVMIN || hAlut3d->updateCalib) {
        ret1 = hAlut3d->_a3dlut_adjust->store_new(hAlut3d->calib_lut3d->mode_cell[hAlut3d->mode_idx].look_up_table_r,
            hAlut3d->calib_lut3d->mode_cell[hAlut3d->mode_idx].look_up_table_g, hAlut3d->calib_lut3d->mode_cell[hAlut3d->mode_idx].look_up_table_b, hAlut3d->lastAlp, hAlut3d->updateCalib);
        if (!ret1)
            LOGI_A3DLUT( " receive Alpha:%f failed \n", hAlut3d->lastAlp);
        else {
            hAlut3d->updateCalib = false;
            LOGI_A3DLUT( " receive Alpha:%f success \n", hAlut3d->lastAlp);
        }
    }
    if (hAlut3d->_a3dlut_adjust->try_lock_output()) {
        if (hAlut3d->_a3dlut_adjust->is_output_new()) {
            const uint16_t* pTableR = hAlut3d->_a3dlut_adjust->get_output_table_r();
            const uint16_t* pTableG = hAlut3d->_a3dlut_adjust->get_output_table_g();
            const uint16_t* pTableB = hAlut3d->_a3dlut_adjust->get_output_table_b();
            memcpy(hAlut3d->lut3d_hw_conf.look_up_table_r, pTableR, sizeof(hAlut3d->lut3d_hw_conf.look_up_table_r));
            memcpy(hAlut3d->lut3d_hw_conf.look_up_table_g, pTableG, sizeof(hAlut3d->lut3d_hw_conf.look_up_table_g));
            memcpy(hAlut3d->lut3d_hw_conf.look_up_table_b, pTableB, sizeof(hAlut3d->lut3d_hw_conf.look_up_table_b));
            hAlut3d->_a3dlut_adjust->mark_output_used();
        }
        hAlut3d->_a3dlut_adjust->unlock_output();
    }
    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);

    return (ret);
}

XCamReturn Alut3dManualConfig
(
    alut3d_handle_t hAlut3d
) {

    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;


    memcpy(hAlut3d->lut3d_hw_conf.look_up_table_r, hAlut3d->mCurAtt.stManual.look_up_table_r,
           sizeof(hAlut3d->mCurAtt.stManual.look_up_table_r));
    memcpy(hAlut3d->lut3d_hw_conf.look_up_table_g, hAlut3d->mCurAtt.stManual.look_up_table_g,
           sizeof(hAlut3d->mCurAtt.stManual.look_up_table_g));
    memcpy(hAlut3d->lut3d_hw_conf.look_up_table_b, hAlut3d->mCurAtt.stManual.look_up_table_b,
           sizeof(hAlut3d->mCurAtt.stManual.look_up_table_b));

    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);

    return (ret);
}

XCamReturn Alut3dConfig
(
    alut3d_handle_t hAlut3d
) {

    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    if (hAlut3d == NULL) {
        return XCAM_RETURN_ERROR_PARAM;
    }

    LOGD_A3DLUT("%s: updateAtt: %d\n", __FUNCTION__, hAlut3d->updateAtt);
    if(hAlut3d->updateAtt) {
        hAlut3d->mCurAtt = hAlut3d->mNewAtt;
    }
    LOGD_A3DLUT("%s: byPass: %d  mode:%d \n", __FUNCTION__, hAlut3d->mCurAtt.byPass, hAlut3d->mCurAtt.mode);
    if(hAlut3d->mCurAtt.byPass != true) {
        hAlut3d->lut3d_hw_conf.enable = true;
        hAlut3d->lut3d_hw_conf.bypass_en = false;
        if (hAlut3d->mCurAtt.stAuto.alpha < 0.0) {
            hAlut3d->mCurAtt.stAuto.alpha = 0.0;
            LOGE_A3DLUT("alpha is out of range, set default to 0.0 \n ");
        }
        else if (hAlut3d->mCurAtt.stAuto.alpha > 1.0){
            hAlut3d->mCurAtt.stAuto.alpha = 1.0;
            LOGE_A3DLUT("alpha is out of range, set default to 1.0 \n ");
        }

        if(hAlut3d->mCurAtt.mode == RK_AIQ_LUT3D_MODE_AUTO) {
            Alut3dAutoConfig(hAlut3d);
        } else if(hAlut3d->mCurAtt.mode == RK_AIQ_LUT3D_MODE_MANUAL) {
            Alut3dManualConfig(hAlut3d);
        } else {
            LOGE_A3DLUT("%s: hAlut3d->mCurAtt.mode(%d) is invalid \n", __FUNCTION__, hAlut3d->mCurAtt.mode);
        }
        memcpy(hAlut3d->mCurAtt.stManual.look_up_table_r, hAlut3d->lut3d_hw_conf.look_up_table_r,
               sizeof(hAlut3d->mCurAtt.stManual.look_up_table_r));
        memcpy( hAlut3d->mCurAtt.stManual.look_up_table_g, hAlut3d->lut3d_hw_conf.look_up_table_g,
                sizeof(hAlut3d->mCurAtt.stManual.look_up_table_g));
        memcpy(hAlut3d->mCurAtt.stManual.look_up_table_b, hAlut3d->lut3d_hw_conf.look_up_table_b,
               sizeof(hAlut3d->mCurAtt.stManual.look_up_table_b));
    } else {
        hAlut3d->lut3d_hw_conf.enable = false;
        hAlut3d->lut3d_hw_conf.bypass_en = true;
    }

    LOGD_A3DLUT("%s: enable:(%d),bypass_en(%d) \n", __FUNCTION__,
                hAlut3d->lut3d_hw_conf.enable,
                hAlut3d->lut3d_hw_conf.bypass_en);

    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);

    return (ret);
}

static XCamReturn UpdateLut3dCalibPara(alut3d_handle_t  hAlut3d)
{
    LOGI_A3DLUT("%s: (enter)  \n", __FUNCTION__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    bool config_calib = !!(hAlut3d->prepare_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB);
    LOGD_A3DLUT("config_calib %d",config_calib);
    if(!config_calib  )
    {
        if ((hAlut3d->prepare_type == RK_AIQ_ALGO_CONFTYPE_INIT) || !!(hAlut3d->prepare_type & 0x07))
            hAlut3d->updateCalib = true;
        else
            hAlut3d->updateCalib = false;
        return(ret);
    }
    hAlut3d->updateCalib = true;
    int mode_idx = 0;
    char mode_name[CALIBDB_MAX_MODE_NAME_LENGTH];
    memset(mode_name, 0x00, sizeof(mode_name));

    if(hAlut3d->calib_lut3d == NULL){
        LOGE_A3DLUT("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    if(hAlut3d->eParamMode == ALut3d_PARAM_MODE_NORMAL){
        sprintf(mode_name, "%s", "normal");
    }else if(hAlut3d->eParamMode == ALut3d_PARAM_MODE_HDR){
        sprintf(mode_name, "%s", "hdr");
    }else{
        LOGE_A3DLUT("%s(%d): not support param mode!\n", __FUNCTION__, __LINE__);
        sprintf(mode_name, "%s", "normal");
    }

    ret = A3dlut_get_mode_cell_idx_by_name(hAlut3d->calib_lut3d, mode_name, &mode_idx);
    RETURN_RESULT_IF_DIFFERENT(ret,XCAM_RETURN_NO_ERROR)
    hAlut3d->mode_idx = mode_idx;

    hAlut3d->lut3d_hw_conf.lut3d_lut_wsize = 0x2d9;
    memcpy(hAlut3d->lut3d_hw_conf.look_up_table_r, hAlut3d->calib_lut3d->mode_cell[mode_idx].look_up_table_r,
           sizeof(hAlut3d->calib_lut3d->mode_cell[mode_idx].look_up_table_r));
    memcpy(hAlut3d->lut3d_hw_conf.look_up_table_g, hAlut3d->calib_lut3d->mode_cell[mode_idx].look_up_table_g,
           sizeof(hAlut3d->calib_lut3d->mode_cell[mode_idx].look_up_table_g));
    memcpy(hAlut3d->lut3d_hw_conf.look_up_table_b, hAlut3d->calib_lut3d->mode_cell[mode_idx].look_up_table_b,
           sizeof(hAlut3d->calib_lut3d->mode_cell[mode_idx].look_up_table_b));
    LOGD_A3DLUT("lut3d r[0],r[4],r[end],g[0],b[0] = %d,%d,%d,%d,%d",hAlut3d->lut3d_hw_conf.look_up_table_r[0],
        hAlut3d->lut3d_hw_conf.look_up_table_r[4],
        hAlut3d->lut3d_hw_conf.look_up_table_r[728],
        hAlut3d->lut3d_hw_conf.look_up_table_g[0],
        hAlut3d->lut3d_hw_conf.look_up_table_b[0]);
    hAlut3d->mCurAtt.byPass = !(hAlut3d->calib_lut3d->enable);
    LOGI_A3DLUT("%s: (exit)  \n", __FUNCTION__);
    return(ret);
}

XCamReturn Alut3dInit(alut3d_handle_t *hAlut3d, const CamCalibDbContext_t* calib)
{
    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    *hAlut3d = (alut3d_context_t*)malloc(sizeof(alut3d_context_t));
    alut3d_context_t* alut3d_contex = *hAlut3d;
    alut3d_contex->eParamMode = ALut3d_PARAM_MODE_NORMAL;
    memset(alut3d_contex, 0, sizeof(alut3d_context_t));

    if(calib == NULL) {
        return XCAM_RETURN_ERROR_FAILED;
    }
    const CalibDb_Lut3d_t *calib_lut3d = &calib->lut3d;
    alut3d_contex->_a3dlut_adjust = new a3dlut_adjust;
    alut3d_contex->_a3dlut_adjust->init();
    alut3d_contex->calib_lut3d = calib_lut3d;
    alut3d_contex->mode_idx = 0;
    alut3d_contex->mCurAtt.mode = RK_AIQ_LUT3D_MODE_AUTO;
    alut3d_contex->mCurAtt.stAuto.alpha = 1.0;
    alut3d_contex->lastAlp = 1.0;
    alut3d_contex->prepare_type = RK_AIQ_ALGO_CONFTYPE_UPDATECALIB | RK_AIQ_ALGO_CONFTYPE_NEEDRESET;
    alut3d_contex->updateCalib = true;
    ret = UpdateLut3dCalibPara(alut3d_contex);
    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);
    return(ret);


}

XCamReturn Alut3dRelease(alut3d_handle_t hAlut3d)
{
    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    hAlut3d->_a3dlut_adjust->deinit();
    delete hAlut3d->_a3dlut_adjust;
    free(hAlut3d);

    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);
    return(ret);

}

XCamReturn Alut3dPrepare(alut3d_handle_t hAlut3d)
{
    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = UpdateLut3dCalibPara(hAlut3d);
    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);
    return ret;
}
XCamReturn Alut3dPreProc(alut3d_handle_t hAlut3d)
{

    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);
    return(ret);

}
XCamReturn Alut3dProcessing(alut3d_handle_t hAlut3d)
{
    LOGI_A3DLUT("%s: (enter)\n", __FUNCTION__);

    XCamReturn ret = XCAM_RETURN_NO_ERROR;


    LOGI_A3DLUT("%s: (exit)\n", __FUNCTION__);
    return(ret);
}




RKAIQ_END_DECLARE


