#include "rk_aiq_uapi_anr_int.h"
#include "anr/rk_aiq_types_anr_algo_prvt.h"
#include "rk_aiq_anr_algo_uvnr.h"
#include "rk_aiq_anr_algo_ynr.h"
#include "rk_aiq_anr_algo_mfnr.h"
#include "rk_aiq_anr_algo_bayernr.h"

#define NR_STRENGTH_MAX_PERCENT (50.0)
#define NR_LUMA_TF_STRENGTH_MAX_PERCENT NR_STRENGTH_MAX_PERCENT
#define NR_LUMA_SF_STRENGTH_MAX_PERCENT (100.0)
#define NR_CHROMA_TF_STRENGTH_MAX_PERCENT NR_STRENGTH_MAX_PERCENT
#define NR_CHROMA_SF_STRENGTH_MAX_PERCENT NR_STRENGTH_MAX_PERCENT
#define NR_RAWNR_SF_STRENGTH_MAX_PERCENT (80.0)

XCamReturn
rk_aiq_uapi_anr_SetAttrib(RkAiqAlgoContext *ctx,
                          rk_aiq_nr_attrib_t *attr,
                          bool need_sync)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    pAnrCtx->eMode = attr->eMode;
    pAnrCtx->stAuto = attr->stAuto;
    pAnrCtx->stManual = attr->stManual;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_anr_GetAttrib(const RkAiqAlgoContext *ctx,
                          rk_aiq_nr_attrib_t *attr)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    attr->eMode = pAnrCtx->eMode;
    memcpy(&attr->stAuto, &pAnrCtx->stAuto, sizeof(ANR_Auto_Attr_t));
    memcpy(&attr->stManual, &pAnrCtx->stManual, sizeof(ANR_Manual_Attr_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_bayernr_OverWriteCalibByCurAttr(RkAiqAlgoContext *ctx, CalibDb_BayerNr_2_t *pCalibdb)
{
    if(ctx == NULL || pCalibdb == NULL) {
        LOGE_ANR("%s(%d): error!!!  \n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
    RKAnr_Bayernr_Params_t *pParams =  &pAnrCtx->stAuto.stBayernrParams;
    //CalibDb_BayerNr_2_t *pCalibdb = &pAnrCtx->stBayernrCalib;
    int mode_idx = 0;
    int setting_idx = 0;
    ANRresult_t res = ANR_RET_SUCCESS;
    int i = 0;
    int j = 0;

    res = bayernr_get_mode_cell_idx_by_name(pCalibdb, pAnrCtx->param_mode_name, &mode_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!	can't find mode name in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    res = bayernr_get_setting_idx_by_name(pCalibdb, pAnrCtx->snr_name, mode_idx, &setting_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!	can't find setting in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    pCalibdb->enable = pAnrCtx->stAuto.bayernrEn;

    CalibDb_BayerNR_Params_t *pSetting = &pCalibdb->mode_cell[mode_idx].setting[setting_idx];

    //bayernr
    pCalibdb->enable = pAnrCtx->stAuto.bayernrEn;

    for(i = 0; i < MAX_ISO_STEP; i++) {
#ifndef RK_SIMULATOR_HW
        pSetting->iso[i] = pParams->iso[i];
#endif
        pSetting->iso[i] = pParams->a[i];
        pSetting->iso[i] = pParams->b[i];
        pSetting->filtPara[i] = pParams->filtpar[i];
    }

    for(i = 0; i < 8; i++) {
        pSetting->luLevelVal[i] = pParams->luLevel[i];
    }

    for(i = 0; i < MAX_ISO_STEP; i++) {
        for(j = 0; j < 8; j++) {
            pSetting->luRatio[j][i] = pParams->luRatio[i][j];
        }
    }

    for(i = 0; i < MAX_ISO_STEP; i++) {
        for(j = 0; j < 4; j++) {
            pSetting->fixW[j][i] = pParams->w[i][j];
        }
    }

    pSetting->lamda = pParams->peaknoisesigma;
    pSetting->gauss_en = pParams->sw_rawnr_gauss_en;
    pSetting->RGainOff = pParams->rgain_offs;
    pSetting->RGainFilp = pParams->rgain_filp;
    pSetting->BGainOff = pParams->bgain_offs;
    pSetting->BGainFilp = pParams->bgain_filp;
    pSetting->edgeSoftness = pParams->bayernr_edgesoftness;

    memcpy( pCalibdb->version, pParams->bayernr_ver_char, sizeof(pParams->bayernr_ver_char));

    return XCAM_RETURN_NO_ERROR;

}


XCamReturn
rk_aiq_ynr_OverWriteCalibByCurAttr(RkAiqAlgoContext *ctx, CalibDb_YNR_2_t* pYnrCalib)
{

    if(ctx == NULL || pYnrCalib == NULL) {
        LOGE_ANR("%s(%d): error!!!  \n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    int i = 0;
    int j = 0;
    int mode_idx = 0;
    int setting_idx = 0;
    ANRresult_t res = ANR_RET_SUCCESS;
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
    RKAnr_Ynr_Params_s *pYnrParams = &pAnrCtx->stAuto.stYnrParams;
    //CalibDb_YNR_2_t* pYnrCalib = &pAnrCtx->stYnrCalib;


    res = ynr_get_mode_cell_idx_by_name(pYnrCalib, pAnrCtx->param_mode_name, &mode_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!  can't find mode name in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
    }

    res = ynr_get_setting_idx_by_name(pYnrCalib, pAnrCtx->snr_name, mode_idx, &setting_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!  can't find setting in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
    }

    pYnrCalib->enable = pAnrCtx->stAuto.ynrEn;

    RKAnr_Ynr_Params_Select_t *pParams = pYnrParams->aYnrParamsISO;
    CalibDb_YNR_ISO_t *pCalibdb = pYnrCalib->mode_cell[mode_idx].setting[setting_idx].ynr_iso;

    short isoCurveSectValue;
    short isoCurveSectValue1;
    int bit_shift;
    int bit_proc;
    int bit_calib;

    bit_calib = 12;
    bit_proc = YNR_SIGMA_BITS;
    bit_shift = bit_calib - bit_proc;
    isoCurveSectValue =  (1 << (bit_calib - ISO_CURVE_POINT_BIT));//rawBit必须大于ISO_CURVE_POINT_BIT
    isoCurveSectValue1 =  (1 << bit_calib);// - 1;//rawBit必须大于ISO_CURVE_POINT_BIT, max use (1 << bit_calib);

#ifndef RK_SIMULATOR_HW
    for(j = 0; j < MAX_ISO_STEP; j++) {
        pCalibdb[j].iso =  pParams[j].iso;
    }
#endif

    for(j = 0; j < MAX_ISO_STEP; j++) {
        for(i = 0; i < WAVELET_LEVEL_NUM; i++) {
            pCalibdb[j].ynr_lci[i] = pParams[j].loFreqNoiseCi[i];
            pCalibdb[j].ynr_lhci[i] = pParams[j].ciISO[i * 3 + 0];
            pCalibdb[j].ynr_hlci[i] = pParams[j].ciISO[i * 3 + 1];
            pCalibdb[j].ynr_hhci[i] = pParams[j].ciISO[i * 3 + 2];
        }

        for(i = 0; i < WAVELET_LEVEL_NUM; i++) {
            pCalibdb[j].denoise_weight[i] = pParams[j].loFreqDenoiseWeight[i];
            pCalibdb[j].lo_bfScale[i] = pParams[j].loFreqBfScale[i];
        }

        for(i = 0; i < 6; i++) {
            pCalibdb[j].lo_lumaPoint[i] = pParams[j].loFreqLumaNrCurvePoint[i];
            pCalibdb[j].lo_lumaRatio[i] = pParams[j].loFreqLumaNrCurveRatio[i];
        }

        pCalibdb[j].imerge_ratio = pParams[j].loFreqDenoiseStrength[0];
        pCalibdb[j].imerge_bound = pParams[j].loFreqDenoiseStrength[1];
        pCalibdb[j].lo_directionStrength = pParams[j].loFreqDirectionStrength;

        for(i = 0; i < WAVELET_LEVEL_NUM; i++) {
            pCalibdb[j].hi_denoiseWeight[i] = pParams[j].hiFreqDenoiseWeight[i];
            pCalibdb[j].hi_bfScale[i] = pParams[j].hiFreqBfScale[i];
            pCalibdb[j].hwith_d[i] = pParams[j].hiFreqEdgeSoftness[i];
            pCalibdb[j].hi_soft_thresh_scale[i] = pParams[j].hiFreqSoftThresholdScale[i];
        }

        for(i = 0; i < 6; i++) {
            pCalibdb[j].hi_lumaPoint[i] = pParams[j].hiFreqLumaNrCurvePoint[i];
            pCalibdb[j].hi_lumaRatio[i] = pParams[j].hiFreqLumaNrCurveRatio[i];
        }
        pCalibdb[j].hi_denoiseStrength = pParams[j].hiFreqDenoiseStrength;

        for(i = 0; i < 6; i++) {
            pCalibdb[j].hgrad_y_level1[i] = pParams[j].detailThreRatioLevel[0][i];
            pCalibdb[j].hgrad_y_level2[i] = pParams[j].detailThreRatioLevel[1][i];
            pCalibdb[j].hgrad_y_level3[i] = pParams[j].detailThreRatioLevel[2][i];
            pCalibdb[j].hgrad_y_level4[i] = pParams[j].detailThreRatioLevel4[i];
        }

        pCalibdb[j].hi_detailMinAdjDnW = pParams[j].detailMinAdjDnW;
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uvnr_OverWriteCalibByCurAttr(RkAiqAlgoContext *ctx,  CalibDb_UVNR_2_t *pCalibdb)
{
    if(ctx == NULL || pCalibdb == NULL) {
        LOGE_ANR("%s(%d): error!!!	\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }


    int i = 0;
    int j = 0;
    int mode_idx = 0;
    int setting_idx = 0;
    ANRresult_t res = ANR_RET_SUCCESS;
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
    RKAnr_Uvnr_Params_t *pParams = &pAnrCtx->stAuto.stUvnrParams;
    // CalibDb_UVNR_2_t *pCalibdb = &pAnrCtx->stUvnrCalib;

    res = uvnr_get_mode_cell_idx_by_name(pCalibdb, pAnrCtx->param_mode_name, &mode_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!  can't find mode cell in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    res = uvnr_get_setting_idx_by_name(pCalibdb, pAnrCtx->snr_name, mode_idx, &setting_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!  can't find setting in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    pCalibdb->enable = pAnrCtx->stAuto.uvnrEn;

    CalibDb_UVNR_Params_t *pSetting = &pCalibdb->mode_cell[mode_idx].setting[setting_idx];
    for(i = 0; i < MAX_ISO_STEP; i++) {
#ifndef RK_SIMULATOR_HW
        pSetting->ISO[i] = pParams->iso[i];
#endif
        pSetting->step0_uvgrad_ratio[i] = pParams->ratio[i];
        pSetting->step0_uvgrad_offset[i] = pParams->offset[i];

        pSetting->step1_downSample_w[i] = pParams->wStep1[i];
        pSetting->step1_downSample_h[i] = pParams->hStep1[i];
        pSetting->step1_downSample_meansize[i] = pParams->meanSize1[i];

        pSetting->step1_median_size[i] = pParams->medSize1[i];
        pSetting->step1_median_ratio[i] = pParams->medRatio1[i];
        pSetting->step1_median_IIR[i] = pParams->isMedIIR1[i];

        pSetting->step1_bf_size[i] = pParams->bfSize1[i];
        pSetting->step1_bf_sigmaR[i] = pParams->sigmaR1[i];
        pSetting->step1_bf_sigmaD[i] = pParams->sigmaD1[i];
        pSetting->step1_bf_uvgain[i] = pParams->uvgain1[i];
        pSetting->step1_bf_ratio[i] = pParams->bfRatio1[i];
        pSetting->step1_bf_isRowIIR[i] = pParams->isRowIIR1[i];
        pSetting->step1_bf_isYcopy[i] = pParams->isYcopy1[i];

        pSetting->step2_downSample_w[i] = pParams->wStep2[i];
        pSetting->step2_downSample_h[i] = pParams->hStep2[i];
        pSetting->step2_downSample_meansize[i] = pParams->meanSize2[i];

        pSetting->step2_median_size[i] = pParams->medSize2[i];
        pSetting->step2_median_ratio[i] = pParams->medRatio2[i];
        pSetting->step2_median_IIR[i] = pParams->isMedIIR2[i];

        pSetting->step2_bf_size[i] = pParams->bfSize3[i];
        pSetting->step2_bf_sigmaR[i] = pParams->sigmaR2[i];
        pSetting->step2_bf_sigmaD[i] = pParams->sigmaD2[i];
        pSetting->step2_bf_uvgain[i] = pParams->uvgain2[i];
        pSetting->step2_bf_ratio[i] = pParams->bfRatio2[i];
        pSetting->step2_bf_isRowIIR[i] = pParams->isRowIIR2[i];
        pSetting->step2_bf_isYcopy[i] = pParams->isYcopy2[i];

        pSetting->step3_bf_size[i] = pParams->bfSize3[i];
        pSetting->step3_bf_sigmaR[i] = pParams->sigmaR3[i];
        pSetting->step3_bf_sigmaD[i] = pParams->sigmaD3[i];
        pSetting->step3_bf_uvgain[i] = pParams->uvgain3[i];
        pSetting->step3_bf_ratio[i] = pParams->bfRatio3[i];
        pSetting->step3_bf_isRowIIR[i] = pParams->isRowIIR3[i];
        pSetting->step3_bf_isYcopy[i] = pParams->isYcopy3[i];

    }

    for(i = 0; i < 4; i++) {
        pSetting->step1_nonMed1[i] = pParams->nonMed1[i];
        pSetting->step1_nonBf1[i] = pParams->nonBf1[i];
        pSetting->step2_nonExt_block[i] = pParams->block2_ext[i];
        pSetting->step2_nonMed[i] = pParams->nonMed2[i];
        pSetting->step2_nonBf[i] = pParams->nonBf2[i];
        pSetting->step3_nonBf3[i] = pParams->nonBf3[i];
    }

    for(i = 0; i < 3; i++) {
        pSetting->kernel_3x3[i] = pParams->kernel_3x3_table[i];
    }

    for(i = 0; i < 5; i++) {
        pSetting->kernel_5x5[i] = pParams->kernel_5x5_talbe[i];
    }

    for(i = 0; i < 8; i++) {
        pSetting->kernel_9x9[i] = pParams->kernel_9x9_table[i];
    }

    pSetting->kernel_9x9_num = pParams->kernel_9x9_num;

    for(i = 0; i < 9; i++) {
        pSetting->sigma_adj_luma[i] = pParams->sigmaAdj_x[i];
        pSetting->sigma_adj_ratio[i] = pParams->sigamAdj_y[i];

        pSetting->threshold_adj_luma[i] = pParams->threAdj_x[i];
        pSetting->threshold_adj_thre[i] = pParams->threAjd_y[i];
    }
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_mfnr_OverWriteCalibByCurAttr(RkAiqAlgoContext *ctx, CalibDb_MFNR_2_t *pCalibdb)
{
    if(ctx == NULL || pCalibdb == NULL) {
        LOGE_ANR("%s(%d): error!!!	\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    int i = 0;
    int j = 0;
    int mode_idx = 0;
    int setting_idx = 0;
    ANRresult_t res = ANR_RET_SUCCESS;
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;
    RKAnr_Mfnr_Params_t *pParams = &pAnrCtx->stAuto.stMfnrParams;
    //CalibDb_MFNR_2_t *pCalibdb = &pAnrCtx->stMfnrCalib;

    res = mfnr_get_mode_cell_idx_by_name(pCalibdb, pAnrCtx->param_mode_name, &mode_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!	can't find mode name in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    res = mfnr_get_setting_idx_by_name(pCalibdb, pAnrCtx->snr_name, mode_idx, &setting_idx);
    if(res != ANR_RET_SUCCESS) {
        LOGW_ANR("%s(%d): error!!!  can't find setting in iq files, use 0 instead\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    int max_iso_step        = MAX_ISO_STEP;
    int dir_num             = MFNR_DIR_NUM;
    int polyorder           = MFNR_POLYORDER;
    int max_lvl             = MFNR_MAX_LVL;
    int max_lvl_uv          = MFNR_MAX_LVL_UV;
    int lumancurve_step     = LUMANRCURVE_STEP;
    int range               = 1 << Y_CALIBRATION_BITS;
    int dir_lo              = DIR_LO;
    int dir_hi              = DIR_HI;

    pCalibdb->enable = pAnrCtx->stAuto.mfnrEn;

    CalibDb_MFNR_Setting_t *pSetting = &pCalibdb->mode_cell[mode_idx].setting[setting_idx];
    for(int lvl = 0; lvl < max_lvl; lvl++) {
        for (i = 0; i < max_iso_step; i++)
            pSetting->mfnr_iso[i].weight_limit_y[lvl]  = pParams->weight_limit_y[i][lvl];
    }

    for(int lvl = 0; lvl < max_lvl_uv; lvl++) {
        for (i = 0; i < max_iso_step; i++)
            pSetting->mfnr_iso[i].weight_limit_uv[lvl] = pParams->weight_limit_uv[i][lvl];
    }

    for(int j = 0; j < 4; j++) {
        for (i = 0; i < max_iso_step; i++)
            pSetting->mfnr_iso[i].ratio_frq[j] = pParams->ratio_frq[i][j];
    }

    for(int lvl = 0; lvl < max_lvl_uv; lvl++) {
        for (i = 0; i < max_iso_step; i++)
            pSetting->mfnr_iso[i].luma_w_in_chroma[lvl] = pParams->luma_w_in_chroma[i][lvl];
    }

    for(j = 0; j < 4; j++) {
        for(i = 0; i < 2; i++)
            pCalibdb->uv_ratio[j].ratio[i] = pParams->awb_uv_ratio[j][i];
    }

    for(int j = 0; j < polyorder + 1; j++) {
        for (i = 0; i < max_iso_step; i++)
            pSetting->mfnr_iso[i].noise_curve[j] = pParams->curve[i][j];
    }

    for (i = 0; i < max_iso_step; i++) {
        pSetting->mfnr_iso[i].noise_curve_x00 = pParams->curve_x0[i];
    }


    for (j = 0; j < max_lvl; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].y_lo_noiseprofile[j] = pParams->ci[i][0][j];
            pSetting->mfnr_iso[i].y_hi_noiseprofile[j] = pParams->ci[i][1][j];
            pSetting->mfnr_iso[i].y_lo_bfscale[j] = pParams->scale[i][0][j];
            pSetting->mfnr_iso[i].y_hi_bfscale[j] = pParams->scale[i][1][j];
        }
    }

    for (j = 0; j < lumancurve_step; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].y_lumanrpoint[j] = pParams->lumanrpoint[i][dir_lo][j];
            pSetting->mfnr_iso[i].y_lumanrcurve[j] = pParams->lumanrcurve[i][dir_lo][j];
            pSetting->mfnr_iso[i].y_lumanrpoint[j] = pParams->lumanrpoint[i][dir_hi][j];
            pSetting->mfnr_iso[i].y_lumanrcurve[j] = pParams->lumanrcurve[i][dir_hi][j];
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        pSetting->mfnr_iso[i].y_denoisestrength =  pParams->dnstr[i][dir_lo];
        pParams->dnstr[i][dir_lo] = pParams->dnstr[i][dir_hi];
    }


    for(int j = 0; j < 6; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].y_lo_lvl0_gfdelta[j] = pParams->gfdelta[i][0][0][j];
            pSetting->mfnr_iso[i].y_hi_lvl0_gfdelta[j] = pParams->gfdelta[i][1][0][j];
        }
    }

    for(int j = 0; j < 3; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].y_lo_lvl1_gfdelta[j] = pParams->gfdelta[i][0][1][j];
            pSetting->mfnr_iso[i].y_lo_lvl2_gfdelta[j] = pParams->gfdelta[i][0][2][j];
            pSetting->mfnr_iso[i].y_lo_lvl3_gfdelta[j] = pParams->gfdelta[i][0][3][j];

            pSetting->mfnr_iso[i].y_hi_lvl1_gfdelta[j] = pParams->gfdelta[i][1][1][j];
            pSetting->mfnr_iso[i].y_hi_lvl2_gfdelta[j] = pParams->gfdelta[i][1][2][j];
            pSetting->mfnr_iso[i].y_hi_lvl3_gfdelta[j] = pParams->gfdelta[i][1][3][j];
        }
    }

    for (j = 0; j < max_lvl_uv; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].uv_lo_noiseprofile[j] =  pParams->ci_uv[i][0][j];
            pSetting->mfnr_iso[i].uv_hi_noiseprofile[j] = pParams->ci_uv[i][1][j];
            pSetting->mfnr_iso[i].uv_lo_bfscale[j] = pParams->scale_uv[i][0][j];
            pSetting->mfnr_iso[i].uv_hi_bfscale[j] = pParams->scale_uv[i][1][j];
        }
    }

    for (j = 0; j < lumancurve_step; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].uv_lumanrpoint[j] = pParams->lumanrpoint_uv[i][dir_lo][j];
            pSetting->mfnr_iso[i].uv_lumanrcurve[j] = pParams->lumanrcurve_uv[i][dir_lo][j];
            pSetting->mfnr_iso[i].uv_lumanrpoint[j] = pParams->lumanrpoint_uv[i][dir_hi][j];
            pSetting->mfnr_iso[i].uv_lumanrcurve[j] = pParams->lumanrcurve_uv[i][dir_hi][j];
        }
    }

    for (i = 0; i < max_iso_step; i++) {
        pSetting->mfnr_iso[i].uv_denoisestrength = pParams->dnstr_uv[i][dir_lo];
        pParams->dnstr_uv[i][dir_lo] = pParams->dnstr_uv[i][dir_hi];
    }

    for(int j = 0; j < 6; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].uv_lo_lvl0_gfdelta[j] = pParams->gfdelta_uv[i][0][0][j];
            pSetting->mfnr_iso[i].uv_hi_lvl0_gfdelta[j] = pParams->gfdelta_uv[i][1][0][j];
        }
    }

    for(int j = 0; j < 3; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].uv_lo_lvl1_gfdelta[j] = pParams->gfdelta_uv[i][0][1][j];
            pSetting->mfnr_iso[i].uv_lo_lvl2_gfdelta[j] = pParams->gfdelta_uv[i][0][2][j];
            pSetting->mfnr_iso[i].uv_hi_lvl1_gfdelta[j] = pParams->gfdelta_uv[i][1][1][j];
            pSetting->mfnr_iso[i].uv_hi_lvl2_gfdelta[j] = pParams->gfdelta_uv[i][1][2][j];
        }
    }


    for(int j = 0; j < 6; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].lvl0_gfsigma[j] = pParams->gfsigma[i][0][j];
        }
    }

    for(int j = 0; j < 3; j++) {
        for (i = 0; i < max_iso_step; i++) {
            pSetting->mfnr_iso[i].lvl1_gfsigma[j] = pParams->gfsigma[i][1][j];
            pSetting->mfnr_iso[i].lvl2_gfsigma[j] = pParams->gfsigma[i][2][j];
            pSetting->mfnr_iso[i].lvl3_gfsigma[j] = pParams->gfsigma[i][3][j];
        }
    }

    //dynamic
    RKAnr_Mfnr_Dynamic_t *pDynamic = &pAnrCtx->stAuto.stMfnr_dynamic;
    pCalibdb->mode_cell[mode_idx].dynamic.enable = pDynamic->enable;
    pCalibdb->mode_cell[mode_idx].dynamic.lowth_iso = pDynamic->lowth_iso;
    pCalibdb->mode_cell[mode_idx].dynamic.lowth_time = pDynamic->lowth_time;
    pCalibdb->mode_cell[mode_idx].dynamic.highth_iso = pDynamic->highth_iso;
    pCalibdb->mode_cell[mode_idx].dynamic.highth_time = pDynamic->highth_time;

    //motion
    memcpy(&pCalibdb->mode_cell[mode_idx].motion, &pAnrCtx->stAuto.stMfnr_Motion, sizeof(pAnrCtx->stAuto.stMfnr_Motion));
    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetIQPara(RkAiqAlgoContext *ctx,
                          rk_aiq_nr_IQPara_t *pPara,
                          bool need_sync)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    if(pPara->module_bits & (1 << ANR_MODULE_BAYERNR)) {
        //pAnrCtx->stBayernrCalib = pPara->stBayernrPara;
        pAnrCtx->stBayernrCalib.enable = pPara->stBayernrPara.enable;
        memcpy(pAnrCtx->stBayernrCalib.version, pPara->stBayernrPara.version, sizeof(pPara->stBayernrPara.version));
        for(int i = 0; i < pAnrCtx->stBayernrCalib.mode_num; i++) {
            pAnrCtx->stBayernrCalib.mode_cell[i] = pPara->stBayernrPara.mode_cell[i];
        }
        pAnrCtx->isIQParaUpdate = true;
    }

    if(pPara->module_bits & (1 << ANR_MODULE_MFNR)) {
        //pAnrCtx->stMfnrCalib = pPara->stMfnrPara;
        rk_aiq_mfnr_IQPara_t mfnrPara;
        rk_aiq_uapi_anr_GetMfnrIQPara(ctx, &mfnrPara);

        mfnrPara.stMfnrPara.enable = pPara->stMfnrPara.enable;
        memcpy(pAnrCtx->stMfnrCalib.version, pPara->stMfnrPara.version, sizeof(pPara->stMfnrPara.version));
        mfnrPara.stMfnrPara.local_gain_en = pPara->stMfnrPara.local_gain_en;
        mfnrPara.stMfnrPara.motion_detect_en = pPara->stMfnrPara.motion_detect_en;
        mfnrPara.stMfnrPara.mode_3to1 = pPara->stMfnrPara.mode_3to1;
        mfnrPara.stMfnrPara.max_level = pPara->stMfnrPara.max_level;
        mfnrPara.stMfnrPara.max_level_uv = pPara->stMfnrPara.max_level_uv;
        mfnrPara.stMfnrPara.back_ref_num = pPara->stMfnrPara.back_ref_num;
        for(int i = 0; i < 4; i++) {
            mfnrPara.stMfnrPara.uv_ratio[i] = pPara->stMfnrPara.uv_ratio[i];
        }
        for(int i = 0; i < pAnrCtx->stMfnrCalib.mode_num; i++) {
            memcpy(mfnrPara.stMfnrPara.mode_cell[i].name, pPara->stMfnrPara.mode_cell[i].name, sizeof(pAnrCtx->stMfnrCalib.mode_cell[i].name));
            memcpy(mfnrPara.stMfnrPara.mode_cell[i].setting,  pPara->stMfnrPara.mode_cell[i].setting, sizeof(pAnrCtx->stMfnrCalib.mode_cell[i].setting));
            memcpy(&mfnrPara.stMfnrPara.mode_cell[i].dynamic, &pPara->stMfnrPara.mode_cell[i].dynamic, sizeof(pAnrCtx->stMfnrCalib.mode_cell[i].dynamic));
            mfnrPara.stMfnrPara.mode_cell[i].motion.enable = pPara->stMfnrPara.mode_cell[i].motion.enable;
            for(int j = 0; j < CALIBDB_NR_SHARP_MAX_ISO_LEVEL; j++) {
                mfnrPara.stMfnrPara.mode_cell[i].motion.iso[j] = pPara->stMfnrPara.mode_cell[i].motion.iso[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.sigmaHScale[j]  = pPara->stMfnrPara.mode_cell[i].motion.sigmaHScale[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.sigmaLScale[j]  = pPara->stMfnrPara.mode_cell[i].motion.sigmaLScale[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.lightClp[j]  = pPara->stMfnrPara.mode_cell[i].motion.lightClp[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.uvWeight[j]  = pPara->stMfnrPara.mode_cell[i].motion.uvWeight[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.mfnrSigmaScale[j]  = pPara->stMfnrPara.mode_cell[i].motion.mfnrSigmaScale[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.yuvnrGainScale0[j]  = pPara->stMfnrPara.mode_cell[i].motion.yuvnrGainScale0[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.yuvnrGainScale1[j]  = pPara->stMfnrPara.mode_cell[i].motion.yuvnrGainScale1[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.yuvnrGainScale2[j]  = pPara->stMfnrPara.mode_cell[i].motion.yuvnrGainScale2[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.frame_limit_y[j]  = pPara->stMfnrPara.mode_cell[i].motion.frame_limit_y[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.frame_limit_uv[j]  = pPara->stMfnrPara.mode_cell[i].motion.frame_limit_uv[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved0[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved0[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved1[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved1[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved2[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved2[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved3[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved3[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved4[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved4[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved5[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved5[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved6[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved6[j];
                mfnrPara.stMfnrPara.mode_cell[i].motion.reserved7[j]  = pPara->stMfnrPara.mode_cell[i].motion.reserved7[j];
            }
        }
        rk_aiq_uapi_anr_SetMfnrIQPara(ctx, &mfnrPara, need_sync);
    }

    if(pPara->module_bits & (1 << ANR_MODULE_UVNR)) {
        //pAnrCtx->stUvnrCalib = pPara->stUvnrPara;
        pAnrCtx->stUvnrCalib.enable = pPara->stUvnrPara.enable;
        memcpy(pAnrCtx->stUvnrCalib.version, pPara->stUvnrPara.version, sizeof(pPara->stUvnrPara.version));
        for(int i = 0; i < pAnrCtx->stUvnrCalib.mode_num; i++) {
            pAnrCtx->stUvnrCalib.mode_cell[i] = pPara->stUvnrPara.mode_cell[i];
        }
        pAnrCtx->isIQParaUpdate = true;
    }

    if(pPara->module_bits & (1 << ANR_MODULE_YNR)) {
        //pAnrCtx->stYnrCalib = pPara->stYnrPara;
        pAnrCtx->stYnrCalib.enable = pPara->stYnrPara.enable;
        memcpy(pAnrCtx->stYnrCalib.version, pPara->stYnrPara.version, sizeof(pPara->stYnrPara.version));
        for(int i = 0; i < pAnrCtx->stYnrCalib.mode_num; i++) {
            pAnrCtx->stYnrCalib.mode_cell[i] = pPara->stYnrPara.mode_cell[i];
        }
        pAnrCtx->isIQParaUpdate = true;
    }

    return XCAM_RETURN_NO_ERROR;
}



XCamReturn
rk_aiq_uapi_anr_SetMfnrIQPara(RkAiqAlgoContext *ctx,
                              rk_aiq_mfnr_IQPara_t *pPara,
                              bool need_sync)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    if(pAnrCtx != NULL && pPara != NULL) {
        pAnrCtx->stMfnrCalib.enable = pPara->stMfnrPara.enable;
        memcpy(pAnrCtx->stMfnrCalib.version, pPara->stMfnrPara.version, sizeof(pPara->stMfnrPara.version));
        pAnrCtx->stMfnrCalib.local_gain_en = pPara->stMfnrPara.local_gain_en;
        pAnrCtx->stMfnrCalib.motion_detect_en = pPara->stMfnrPara.motion_detect_en;
        pAnrCtx->stMfnrCalib.mode_3to1 = pPara->stMfnrPara.mode_3to1;
        pAnrCtx->stMfnrCalib.max_level = pPara->stMfnrPara.max_level;
        pAnrCtx->stMfnrCalib.max_level_uv = pPara->stMfnrPara.max_level_uv;
        pAnrCtx->stMfnrCalib.back_ref_num = pPara->stMfnrPara.back_ref_num;
        for(int i = 0; i < 4; i++) {
            pAnrCtx->stMfnrCalib.uv_ratio[i] = pPara->stMfnrPara.uv_ratio[i];
        }
        for(int i = 0; i < pAnrCtx->stMfnrCalib.mode_num; i++) {
            pAnrCtx->stMfnrCalib.mode_cell[i] = pPara->stMfnrPara.mode_cell[i];
        }
        pAnrCtx->isIQParaUpdate = true;
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetIQPara(RkAiqAlgoContext *ctx,
                          rk_aiq_nr_IQPara_t *pPara)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    rk_aiq_bayernr_OverWriteCalibByCurAttr(ctx, &pAnrCtx->stBayernrCalib);
    rk_aiq_mfnr_OverWriteCalibByCurAttr(ctx, &pAnrCtx->stMfnrCalib);
    rk_aiq_ynr_OverWriteCalibByCurAttr(ctx, &pAnrCtx->stYnrCalib);
    rk_aiq_uvnr_OverWriteCalibByCurAttr(ctx, &pAnrCtx->stUvnrCalib);

    //pPara->stBayernrPara = pAnrCtx->stBayernrCalib;
    memset(&pPara->stBayernrPara, 0x00, sizeof(CalibDb_BayerNr_t));
    pPara->stBayernrPara.enable = pAnrCtx->stBayernrCalib.enable;
    memcpy(pPara->stBayernrPara.version, pAnrCtx->stBayernrCalib.version, sizeof(pPara->stBayernrPara.version));
    for(int i = 0; i < pAnrCtx->stBayernrCalib.mode_num; i++) {
        pPara->stBayernrPara.mode_cell[i] = pAnrCtx->stBayernrCalib.mode_cell[i];
    }

    //pPara->stMfnrPara = pAnrCtx->stMfnrCalib;
    rk_aiq_mfnr_IQPara_t mfnrPara;
    rk_aiq_uapi_anr_GetMfnrIQPara(ctx, &mfnrPara);

    memset(&pPara->stMfnrPara, 0x00, sizeof(CalibDb_MFNR_t));
    pPara->stMfnrPara.enable = mfnrPara.stMfnrPara.enable;
    memcpy(pPara->stMfnrPara.version, mfnrPara.stMfnrPara.version, sizeof(pPara->stMfnrPara.version));
    pPara->stMfnrPara.local_gain_en = mfnrPara.stMfnrPara.local_gain_en;
    pPara->stMfnrPara.motion_detect_en = mfnrPara.stMfnrPara.motion_detect_en;
    pPara->stMfnrPara.mode_3to1 = mfnrPara.stMfnrPara.mode_3to1;
    pPara->stMfnrPara.max_level = mfnrPara.stMfnrPara.max_level;
    pPara->stMfnrPara.max_level_uv = mfnrPara.stMfnrPara.max_level_uv;
    pPara->stMfnrPara.back_ref_num = mfnrPara.stMfnrPara.back_ref_num;
    for(int i = 0; i < 4; i++) {
        pPara->stMfnrPara.uv_ratio[i] = mfnrPara.stMfnrPara.uv_ratio[i];
    }
    for(int i = 0; i < pAnrCtx->stMfnrCalib.mode_num; i++) {
        memcpy(pPara->stMfnrPara.mode_cell[i].name, mfnrPara.stMfnrPara.mode_cell[i].name, sizeof(pAnrCtx->stMfnrCalib.mode_cell[i].name));
        memcpy(pPara->stMfnrPara.mode_cell[i].setting,  mfnrPara.stMfnrPara.mode_cell[i].setting, sizeof(pAnrCtx->stMfnrCalib.mode_cell[i].setting));
        memcpy(&pPara->stMfnrPara.mode_cell[i].dynamic, &mfnrPara.stMfnrPara.mode_cell[i].dynamic, sizeof(pAnrCtx->stMfnrCalib.mode_cell[i].dynamic));
        pPara->stMfnrPara.mode_cell[i].motion.enable  = mfnrPara.stMfnrPara.mode_cell[i].motion.enable;
        for(int j = 0; j < CALIBDB_NR_SHARP_MAX_ISO_LEVEL; j++) {
            pPara->stMfnrPara.mode_cell[i].motion.iso[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.iso[j];
            pPara->stMfnrPara.mode_cell[i].motion.sigmaHScale[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.sigmaHScale[j];
            pPara->stMfnrPara.mode_cell[i].motion.sigmaLScale[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.sigmaLScale[j];
            pPara->stMfnrPara.mode_cell[i].motion.lightClp[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.lightClp[j];
            pPara->stMfnrPara.mode_cell[i].motion.uvWeight[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.uvWeight[j];
            pPara->stMfnrPara.mode_cell[i].motion.mfnrSigmaScale[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.mfnrSigmaScale[j];
            pPara->stMfnrPara.mode_cell[i].motion.yuvnrGainScale0[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.yuvnrGainScale0[j];
            pPara->stMfnrPara.mode_cell[i].motion.yuvnrGainScale1[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.yuvnrGainScale1[j];
            pPara->stMfnrPara.mode_cell[i].motion.yuvnrGainScale2[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.yuvnrGainScale2[j];
            pPara->stMfnrPara.mode_cell[i].motion.frame_limit_y[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.frame_limit_y[j];
            pPara->stMfnrPara.mode_cell[i].motion.frame_limit_uv[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.frame_limit_uv[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved0[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved0[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved1[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved1[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved2[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved2[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved3[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved3[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved4[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved4[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved5[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved5[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved6[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved6[j];
            pPara->stMfnrPara.mode_cell[i].motion.reserved7[j] = mfnrPara.stMfnrPara.mode_cell[i].motion.reserved7[j];
        }
    }


    //pPara->stUvnrPara = pAnrCtx->stUvnrCalib;
    memset(&pPara->stUvnrPara, 0x00, sizeof(CalibDb_UVNR_t));
    pPara->stUvnrPara.enable = pAnrCtx->stUvnrCalib.enable;
    memcpy(pPara->stUvnrPara.version, pAnrCtx->stUvnrCalib.version, sizeof(pPara->stUvnrPara.version));
    for(int i = 0; i < pAnrCtx->stUvnrCalib.mode_num; i++) {
        pPara->stUvnrPara.mode_cell[i] = pAnrCtx->stUvnrCalib.mode_cell[i];
    }


    //pPara->stYnrPara = pAnrCtx->stYnrCalib;
    memset(&pPara->stYnrPara, 0x00, sizeof(CalibDb_YNR_t));
    pPara->stYnrPara.enable = pAnrCtx->stYnrCalib.enable;
    memcpy(pPara->stYnrPara.version, pAnrCtx->stYnrCalib.version, sizeof(pPara->stYnrPara.version));
    for(int i = 0; i < pAnrCtx->stYnrCalib.mode_num; i++) {
        pPara->stYnrPara.mode_cell[i] = pAnrCtx->stYnrCalib.mode_cell[i];
    }

    return XCAM_RETURN_NO_ERROR;
}



XCamReturn
rk_aiq_uapi_anr_GetMfnrIQPara(RkAiqAlgoContext *ctx,
                              rk_aiq_mfnr_IQPara_t *pPara)
{

    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    //pPara->stMfnrPara = pAnrCtx->stMfnrCalib;
    memset(&pPara->stMfnrPara, 0x00, sizeof(CalibDb_MFNR_V2_t));
    pPara->stMfnrPara.enable = pAnrCtx->stMfnrCalib.enable;
    memcpy(pPara->stMfnrPara.version, pAnrCtx->stMfnrCalib.version, sizeof(pPara->stMfnrPara.version));
    pPara->stMfnrPara.local_gain_en = pAnrCtx->stMfnrCalib.local_gain_en;
    pPara->stMfnrPara.motion_detect_en = pAnrCtx->stMfnrCalib.motion_detect_en;
    pPara->stMfnrPara.mode_3to1 = pAnrCtx->stMfnrCalib.mode_3to1;
    pPara->stMfnrPara.max_level = pAnrCtx->stMfnrCalib.max_level;
    pPara->stMfnrPara.max_level_uv = pAnrCtx->stMfnrCalib.max_level_uv;
    pPara->stMfnrPara.back_ref_num = pAnrCtx->stMfnrCalib.back_ref_num;
    for(int i = 0; i < 4; i++) {
        pPara->stMfnrPara.uv_ratio[i] = pAnrCtx->stMfnrCalib.uv_ratio[i];
    }
    for(int i = 0; i < pAnrCtx->stMfnrCalib.mode_num; i++) {
        pPara->stMfnrPara.mode_cell[i] = pAnrCtx->stMfnrCalib.mode_cell[i];
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetLumaSFStrength(const RkAiqAlgoContext *ctx,
                                  float fPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0f;
    float fMax = NR_LUMA_SF_STRENGTH_MAX_PERCENT;


    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        fStrength = (fPercent - 0.5) * (fMax - 1) * 2 + 1;
    }

    if(fStrength > 1) {
        pAnrCtx->fRawnr_SF_Strength = fStrength / 2.0;
        pAnrCtx->fLuma_SF_Strength = 1;
    } else {
        pAnrCtx->fRawnr_SF_Strength = fStrength;
        pAnrCtx->fLuma_SF_Strength = fStrength;
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetLumaTFStrength(const RkAiqAlgoContext *ctx,
                                  float fPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0;
    float fMax = NR_LUMA_TF_STRENGTH_MAX_PERCENT;

    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        fStrength = (fPercent - 0.5) * (fMax - 1) * 2 + 1;
    }

    pAnrCtx->fLuma_TF_Strength = fStrength;

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetLumaSFStrength(const RkAiqAlgoContext *ctx,
                                  float *pPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0f;
    float fMax = NR_LUMA_SF_STRENGTH_MAX_PERCENT;


    fStrength = pAnrCtx->fLuma_SF_Strength;

    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        *pPercent = (fStrength - 1) / ((fMax - 1) * 2) + 0.5;
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetLumaTFStrength(const RkAiqAlgoContext *ctx,
                                  float *pPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0;
    float fMax = NR_LUMA_TF_STRENGTH_MAX_PERCENT;

    fStrength = pAnrCtx->fLuma_TF_Strength;

    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        *pPercent = (fStrength - 1) / ((fMax - 1) * 2) + 0.5;
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetChromaSFStrength(const RkAiqAlgoContext *ctx,
                                    float fPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0f;
    float fMax = NR_CHROMA_SF_STRENGTH_MAX_PERCENT;

    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        fStrength = (fPercent - 0.5) * (fMax - 1) * 2 + 1;
    }

    pAnrCtx->fChroma_SF_Strength = fStrength;

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetChromaTFStrength(const RkAiqAlgoContext *ctx,
                                    float fPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0;
    float fMax = NR_CHROMA_TF_STRENGTH_MAX_PERCENT;

    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        fStrength = (fPercent - 0.5) * (fMax - 1) * 2 + 1;
    }

    pAnrCtx->fChroma_TF_Strength = fStrength;

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetChromaSFStrength(const RkAiqAlgoContext *ctx,
                                    float *pPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0f;
    float fMax = NR_CHROMA_SF_STRENGTH_MAX_PERCENT;

    fStrength = pAnrCtx->fChroma_SF_Strength;


    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        *pPercent = (fStrength - 1) / ((fMax - 1) * 2) + 0.5;
    }


    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetChromaTFStrength(const RkAiqAlgoContext *ctx,
                                    float *pPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0;
    float fMax = NR_CHROMA_TF_STRENGTH_MAX_PERCENT;

    fStrength = pAnrCtx->fChroma_TF_Strength;

    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        *pPercent = (fStrength - 1) / ((fMax - 1) * 2) + 0.5;
    }

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_SetRawnrSFStrength(const RkAiqAlgoContext *ctx,
                                   float fPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0;
    float fMax = NR_RAWNR_SF_STRENGTH_MAX_PERCENT;

    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        fStrength = (fPercent - 0.5) * (fMax - 1) * 2 + 1;
    }

    pAnrCtx->fRawnr_SF_Strength = fStrength;

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_anr_GetRawnrSFStrength(const RkAiqAlgoContext *ctx,
                                   float *pPercent)
{
    ANRContext_t* pAnrCtx = (ANRContext_t*)ctx;

    float fStrength = 1.0f;
    float fMax = NR_RAWNR_SF_STRENGTH_MAX_PERCENT;

    fStrength = pAnrCtx->fRawnr_SF_Strength;


    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        *pPercent = (fStrength - 1) / ((fMax - 1) * 2) + 0.5;
    }


    return XCAM_RETURN_NO_ERROR;
}


