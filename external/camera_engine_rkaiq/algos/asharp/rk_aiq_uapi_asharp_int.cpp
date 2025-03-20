#include "rk_aiq_uapi_asharp_int.h"
#include "asharp/rk_aiq_types_asharp_algo_prvt.h"
#include "rk_aiq_asharp_algo_sharp.h"
#include "rk_aiq_asharp_algo_edgefilter.h"



XCamReturn
rk_aiq_uapi_asharp_SetAttrib(RkAiqAlgoContext *ctx,
                             rk_aiq_sharp_attrib_t *attr,
                             bool need_sync)
{

    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;

    pAsharpCtx->eMode = attr->eMode;
    pAsharpCtx->stAuto = attr->stAuto;
    pAsharpCtx->stManual = attr->stManual;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_asharp_GetAttrib(const RkAiqAlgoContext *ctx,
                             rk_aiq_sharp_attrib_t *attr)
{

    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;

    attr->eMode = pAsharpCtx->eMode;
    memcpy(&attr->stAuto, &pAsharpCtx->stAuto, sizeof(Asharp_Auto_Attr_t));
    memcpy(&attr->stManual, &pAsharpCtx->stManual, sizeof(Asharp_Manual_Attr_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_sharp_OverWriteCalibByCurAttr(const RkAiqAlgoContext *ctx,
                                     CalibDb_Sharp_2_t *pCalibdb)
{
    if(ctx == NULL || pCalibdb == NULL) {
        LOGE_ASHARP("%s(%d): error!!!  \n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    int i = 0;
    int j = 0;
    int mode_idx = 0;
    int setting_idx = 0;
    int max_iso_step = MAX_ISO_STEP;
    AsharpResult_t res = ASHARP_RET_SUCCESS;
    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;
    RKAsharp_Sharp_HW_Params_t *pParams = &pAsharpCtx->stAuto.stSharpParam.rk_sharpen_params_V1;


    res = sharp_get_mode_cell_idx_by_name_v1(pCalibdb, pAsharpCtx->mode_name, &mode_idx);
    if(res != ASHARP_RET_SUCCESS) {
        LOGE_ASHARP("%s(%d): error!!!  can't find mode name in iq files, return\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    res = sharp_get_setting_idx_by_name_v1(pCalibdb, pAsharpCtx->snr_name, mode_idx, &setting_idx);
    if(res != ASHARP_RET_SUCCESS) {
        LOGE_ASHARP("%s(%d): error!!!  can't find setting in iq files, return\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    pCalibdb->enable = pAsharpCtx->stAuto.sharpEn;

    CalibDb_Sharp_Setting_t *pSetting = &pCalibdb->mode_cell[mode_idx].setting[setting_idx];
    for (i = 0; i < max_iso_step; i++) {
#ifndef RK_SIMULATOR_HW
        pSetting->sharp_iso[i].iso = pParams->iso[i];
#endif
        pSetting->sharp_iso[i].lratio = pParams->lratio[i];
        pSetting->sharp_iso[i].hratio = pParams->hratio[i];
        pSetting->sharp_iso[i].mf_sharp_ratio = pParams->M_ratio[i];
        pSetting->sharp_iso[i].hf_sharp_ratio = pParams->H_ratio[i];
    }

    for(int j = 0; j < RK_EDGEFILTER_LUMA_POINT_NUM; j++) {
        pCalibdb->luma_point[j] = pParams->luma_point[j];
    }

    for (i = 0; i < max_iso_step; i++) {
        for(int j = 0; j < RK_EDGEFILTER_LUMA_POINT_NUM; j++) {
            pSetting->sharp_iso[i].luma_sigma[j] = pParams->luma_sigma[i][j];
        }
        pSetting->sharp_iso[i].pbf_gain = pParams->pbf_gain[i];
        pSetting->sharp_iso[i].pbf_ratio = pParams->pbf_ratio[i];
        pSetting->sharp_iso[i].pbf_add = pParams->pbf_add[i];

        for(int j = 0; j < RK_EDGEFILTER_LUMA_POINT_NUM; j++) {
            pSetting->sharp_iso[i].mf_clip_pos[j] = pParams->lum_clp_m[i][j];
        }

        for(int j = 0; j < RK_EDGEFILTER_LUMA_POINT_NUM; j++) {
            pSetting->sharp_iso[i].mf_clip_neg[j] = pParams->lum_min_m[i][j];
        }

        for(int j = 0; j < RK_SHARPFILTER_LUMA_POINT_NUM; j++) {
            pSetting->sharp_iso[i].hf_clip[j] = pParams->lum_clp_h[i][j];
        }

        pSetting->sharp_iso[i].mbf_gain = pParams->mbf_gain[i];
        pSetting->sharp_iso[i].hbf_gain = pParams->hbf_gain[i];
        pSetting->sharp_iso[i].hbf_ratio = pParams->hbf_ratio[i];
        pSetting->sharp_iso[i].mbf_add = pParams->mbf_add[i];
        pSetting->sharp_iso[i].hbf_add = pParams->hbf_add[i];
        pSetting->sharp_iso[i].local_sharp_strength = pParams->ehf_th[i];
        pSetting->sharp_iso[i].pbf_coeff_percent = pParams->pbf_coeff_percent[i];
        pSetting->sharp_iso[i].rf_m_coeff_percent = pParams->rf_m_coeff_percent[i];
        pSetting->sharp_iso[i].rf_h_coeff_percent = pParams->rf_h_coeff_percent[i];
        pSetting->sharp_iso[i].hbf_coeff_percent = pParams->hbf_coeff_percent[i];
    }


    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_GAU_DIAM;
        int w = RKSHAPRENHW_GAU_DIAM;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].gauss_luma_coeff[m * w + n] = pParams->gaus_luma_kernel[i][m * w + n];
            }
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_PBF_DIAM;
        int w = RKSHAPRENHW_PBF_DIAM;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].pbf_coeff_l[m * w + n]  = pParams->kernel_pbf_l[i][m * w + n];
                pCalibdb->mode_cell[mode_idx].pbf_coeff_h[m * w + n] = pParams->kernel_pbf_h[i][m * w + n];
            }
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_MRF_DIAM;
        int w = RKSHAPRENHW_MRF_DIAM;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].rf_m_coeff_l[m * w + n]  = pParams->h_rf_m_l[i][m * w + n];
                pCalibdb->mode_cell[mode_idx].rf_m_coeff_h[m * w + n]  = pParams->h_rf_m_h[i][m * w + n];
            }
        }
    }


    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_MBF_DIAM_Y;
        int w = RKSHAPRENHW_MBF_DIAM_X;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].mbf_coeff[m * w + n] = pParams->kernel_mbf[i][m * w + n];
            }
        }
    }


    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_HRF_DIAM;
        int w = RKSHAPRENHW_HRF_DIAM;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].rf_h_coeff_l[m * w + n] = pParams->h_rf_h_l[i][m * w + n];
                pCalibdb->mode_cell[mode_idx].rf_h_coeff_l[m * w + n] = pParams->h_rf_h_h[i][m * w + n];
            }
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_HRF_DIAM;
        int w = RKSHAPRENHW_HRF_DIAM;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].rf_h_coeff_l[m * w + n] = pParams->h_rf_h_l[i][m * w + n];
                pCalibdb->mode_cell[mode_idx].rf_h_coeff_l[m * w + n] = pParams->h_rf_h_h[i][m * w + n];
            }
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        int h = RKSHAPRENHW_HBF_DIAM;
        int w = RKSHAPRENHW_HBF_DIAM;
        for(int m = 0; m < h; m++) {
            for(int n = 0; n < w; n++) {
                pCalibdb->mode_cell[mode_idx].hbf_coeff_l[m * w + n] = pParams->kernel_hbf_l[i][m * w + n];
                pCalibdb->mode_cell[mode_idx].hbf_coeff_l[m * w + n] = pParams->kernel_hbf_h[i][m * w + n];
            }
        }
    }


    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_edgefilter_OverWriteCalibByCurAttr(const RkAiqAlgoContext *ctx, CalibDb_EdgeFilter_2_t *pCalibdb )
{

    if(ctx == NULL || pCalibdb == NULL) {
        LOGE_ASHARP("%s(%d): error!!!	\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    int i = 0;
    int j = 0;
    int mode_idx = 0;
    int setting_idx = 0;
    int max_iso_step = MAX_ISO_STEP;
    AsharpResult_t res = ASHARP_RET_SUCCESS;
    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;
    RKAsharp_EdgeFilter_Params_t *pParams = &pAsharpCtx->stAuto.stEdgefilterParams;

    res = edgefilter_get_mode_cell_idx_by_name(pCalibdb, pAsharpCtx->mode_name, &mode_idx);
    if(res != ASHARP_RET_SUCCESS) {
        LOGW_ASHARP("%s(%d): error!!!  can't find mode name in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    res = edgefilter_get_setting_idx_by_name(pCalibdb, pAsharpCtx->snr_name, mode_idx,  &setting_idx);
    if(res != ASHARP_RET_SUCCESS) {
        LOGW_ASHARP("%s(%d): error!!!  can't find setting in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    pCalibdb->enable = pAsharpCtx->stAuto.edgeFltEn;

    CalibDb_EdgeFilter_Setting_t *pSetting = &pCalibdb->mode_cell[mode_idx].setting[setting_idx];
    for(i = 0; i < max_iso_step; i++) {
#ifndef RK_SIMULATOR_HW
        pSetting->edgeFilter_iso[i].iso = pParams->iso[i];
#endif
        pSetting->edgeFilter_iso[i].edge_thed = pParams->edge_thed[i];
        pSetting->edgeFilter_iso[i].src_wgt = pParams->smoth4[i];
        pSetting->edgeFilter_iso[i].alpha_adp_en = pParams->alpha_adp_en[i];
        pSetting->edgeFilter_iso[i].local_alpha = pParams->l_alpha[i];
        pSetting->edgeFilter_iso[i].global_alpha = pParams->g_alpha[i];
    }

    for(j = 0; j < RK_EDGEFILTER_LUMA_POINT_NUM; j++) {
        pCalibdb->luma_point[j] = pParams->enhance_luma_point[j];
    }

    for (i = 0; i < max_iso_step; i++) {
        for(j = 0; j < RK_EDGEFILTER_LUMA_POINT_NUM; j++) {
            pSetting->edgeFilter_iso[i].noise_clip[j] = pParams->edge_thed_1[i][j];
            pSetting->edgeFilter_iso[i].dog_clip_pos[j] = pParams->clamp_pos_dog[i][j];
            pSetting->edgeFilter_iso[i].dog_clip_neg[j] = pParams->clamp_neg_dog[i][j];
            pSetting->edgeFilter_iso[i].dog_alpha[j] = pParams->detail_alpha_dog[i][j];
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        int h = RKEDGEFILTER_DIR_SMTH_DIAM;
        int w = RKEDGEFILTER_DIR_SMTH_DIAM;
        for(int n = 0; n < w; n++)
            pSetting->edgeFilter_iso[i].direct_filter_coeff[n] =  pParams->h0_h_coef_5x5[i][2 * w + n];
    }

    for (i = 0; i < max_iso_step; i++) {
        int h = RKEDGEFILTER_DOG_DIAM;
        int w = RKEDGEFILTER_DOG_DIAM;
        int iso         = (1 << i) * 50;
        int gain        = (1 << i);

        for(int n = 0; n < w; n++) {
            pSetting->edgeFilter_iso[i].dog_kernel_row0[n] = pParams->dog_kernel_l[i][0 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row1[n] = pParams->dog_kernel_l[i][1 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row2[n] = pParams->dog_kernel_l[i][2 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row3[n] = pParams->dog_kernel_l[i][3 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row4[n] = pParams->dog_kernel_l[i][4 * w + n];

            pSetting->edgeFilter_iso[i].dog_kernel_row0[n] = pParams->dog_kernel_h[i][0 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row1[n] = pParams->dog_kernel_h[i][1 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row2[n] = pParams->dog_kernel_h[i][2 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row3[n] = pParams->dog_kernel_h[i][3 * w + n];
            pSetting->edgeFilter_iso[i].dog_kernel_row4[n] = pParams->dog_kernel_h[i][4 * w + n];
        }
        pSetting->edgeFilter_iso[i].dog_kernel_percent = pParams->dog_kernel_percent[i];
    }


    for (i = 0; i < max_iso_step; i++) {
        for(int j = 0; j < RKEDGEFILTER_DOG_DIAM * RKEDGEFILTER_DOG_DIAM; j++) {
            pCalibdb->mode_cell[mode_idx].dog_kernel_l[j] = pParams->dog_kernel_l[i][j];
            pCalibdb->mode_cell[mode_idx].dog_kernel_h[j] = pParams->dog_kernel_h[i][j];
        }
        pSetting->edgeFilter_iso[i].dog_kernel_percent = pParams->dog_kernel_percent[i];

    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_asharp_SetIQpara(RkAiqAlgoContext *ctx,
                             rk_aiq_sharp_IQpara_t *para,
                             bool need_sync)
{

    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;

    if(para->module_bits & (1 << ASHARP_MODULE_SHARP)) {
        //pAsharpCtx->stSharpCalib = para->stSharpPara;
        pAsharpCtx->stSharpCalib.enable = para->stSharpPara.enable;
        memcpy(pAsharpCtx->stSharpCalib.version, para->stSharpPara.version, sizeof(para->stSharpPara.version));
        for(int i = 0; i < 8; i++) {
            pAsharpCtx->stSharpCalib.luma_point[i] = para->stSharpPara.luma_point[i];
        }
        for(int i = 0; i < pAsharpCtx->stSharpCalib.mode_num; i++) {
            pAsharpCtx->stSharpCalib.mode_cell[i] = para->stSharpPara.mode_cell[i];
        }
        pAsharpCtx->isIQParaUpdate = true;
    }

    if(para->module_bits & (1 << ASHARP_MODULE_EDGEFILTER)) {
        //pAsharpCtx->stEdgeFltCalib = para->stEdgeFltPara;
        pAsharpCtx->stEdgeFltCalib.enable = para->stEdgeFltPara.enable;
        memcpy(pAsharpCtx->stEdgeFltCalib.version, para->stEdgeFltPara.version, sizeof(para->stEdgeFltPara.version));
        for(int i = 0; i < 8; i++) {
            pAsharpCtx->stEdgeFltCalib.luma_point[i] = para->stEdgeFltPara.luma_point[i];
        }
        for(int i = 0; i < pAsharpCtx->stEdgeFltCalib.mode_num; i++) {
            pAsharpCtx->stEdgeFltCalib.mode_cell[i] = para->stEdgeFltPara.mode_cell[i];
        }
        pAsharpCtx->isIQParaUpdate = true;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_asharp_GetIQpara(const RkAiqAlgoContext *ctx,
                             rk_aiq_sharp_IQpara_t *para)
{

    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;

    rk_aiq_sharp_OverWriteCalibByCurAttr(ctx, &pAsharpCtx->stSharpCalib);
    rk_aiq_edgefilter_OverWriteCalibByCurAttr(ctx, &pAsharpCtx->stEdgeFltCalib);

    //para->stSharpPara = pAsharpCtx->stSharpCalib;
    //para->stEdgeFltPara = pAsharpCtx->stEdgeFltCalib;
    memset(&para->stSharpPara, 0x00, sizeof(CalibDb_Sharp_t));
    para->stSharpPara.enable = pAsharpCtx->stSharpCalib.enable;
    memcpy(para->stSharpPara.version, pAsharpCtx->stSharpCalib.version, sizeof(para->stSharpPara.version));
    for(int i = 0; i < 8; i++) {
        para->stSharpPara.luma_point[i] = pAsharpCtx->stSharpCalib.luma_point[i];
    }
    for(int i = 0; i < pAsharpCtx->stSharpCalib.mode_num; i++) {
        para->stSharpPara.mode_cell[i] = pAsharpCtx->stSharpCalib.mode_cell[i];
    }

    memset(&para->stEdgeFltPara, 0x00, sizeof(CalibDb_EdgeFilter_t));
    para->stEdgeFltPara.enable = pAsharpCtx->stEdgeFltCalib.enable;
    memcpy(para->stEdgeFltPara.version, pAsharpCtx->stEdgeFltCalib.version, sizeof(para->stEdgeFltPara.version));
    for(int i = 0; i < 8; i++) {
        para->stEdgeFltPara.luma_point[i] = pAsharpCtx->stEdgeFltCalib.luma_point[i];
    }
    for(int i = 0; i < pAsharpCtx->stEdgeFltCalib.mode_num; i++) {
        para->stEdgeFltPara.mode_cell[i] = pAsharpCtx->stEdgeFltCalib.mode_cell[i];
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_asharp_SetStrength(const RkAiqAlgoContext *ctx,
                               float fPercent)
{

    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;
    float fMax = SHARP_MAX_STRENGTH_PERCENT;
    float fStrength = 1.0;


    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        fStrength = (fPercent - 0.5) * (fMax - 1) * 2 + 1;
    }

    pAsharpCtx->fStrength = fStrength;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_asharp_GetStrength(const RkAiqAlgoContext *ctx,
                               float *pPercent)
{

    AsharpContext_t* pAsharpCtx = (AsharpContext_t*)ctx;
    float fMax = SHARP_MAX_STRENGTH_PERCENT;
    float fStrength = 1.0;

    fStrength = pAsharpCtx->fStrength;

    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        *pPercent =  (fStrength - 1) / ((fMax - 1) * 2 ) + 0.5;
    }

    return XCAM_RETURN_NO_ERROR;
}

