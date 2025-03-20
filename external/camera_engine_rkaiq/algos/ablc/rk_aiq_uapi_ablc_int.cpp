#include "rk_aiq_uapi_ablc_int.h"
#include "ablc/rk_aiq_types_ablc_algo_prvt.h"
#include "rk_aiq_ablc_algo.h"

XCamReturn
rk_aiq_uapi_ablc_SetAttrib(RkAiqAlgoContext *ctx,
                           rk_aiq_blc_attrib_t *attr,
                           bool need_sync)
{

    AblcContext_t* pAblcCtx = (AblcContext_t*)ctx;
    pAblcCtx->eMode = attr->eMode;
    pAblcCtx->stAuto = attr->stAuto;
    pAblcCtx->stManual = attr->stManual;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_ablc_GetAttrib(const RkAiqAlgoContext *ctx,
                           rk_aiq_blc_attrib_t *attr)
{

    AblcContext_t* pAblcCtx = (AblcContext_t*)ctx;

    attr->eMode = pAblcCtx->eMode;
    memcpy(&attr->stAuto, &pAblcCtx->stAuto, sizeof(AblcAutoAttr_t));
    memcpy(&attr->stManual, &pAblcCtx->stManual, sizeof(AblcManualAttr_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_ablc_OverWriteCalibByCurAttr(RkAiqAlgoContext *ctx,
                                    CalibDb_Blc_t *pBlcCalib)
{
    if(ctx == NULL || pBlcCalib == NULL) {
        LOGE_ABLC("%s(%d): error!!!	\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    AblcContext_t* pAblcCtx = (AblcContext_t*)ctx;
    AblcResult_t res = ABLC_RET_SUCCESS;
    int mode_idx = 0;
    AblcParams_t *pParams = &pAblcCtx->stAuto.stParams;

    res = Ablc_get_mode_cell_idx_by_name(pBlcCalib, pAblcCtx->mode_name, &mode_idx);
    if(res != ABLC_RET_SUCCESS) {
        LOGE_ABLC("%s(%d): error!!!  can't find mode name in iq files, return\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_NO_ERROR;
    }

    pBlcCalib->enable = pParams->enable;


    for(int i = 0; i < BLC_MAX_ISO_LEVEL; i++) {
        pBlcCalib->mode_cell[mode_idx].iso[i] = pParams->iso[i];
        pBlcCalib->mode_cell[mode_idx].level[0][i] = pParams->blc_r[i];
        pBlcCalib->mode_cell[mode_idx].level[1][i] = pParams->blc_gr[i];
        pBlcCalib->mode_cell[mode_idx].level[2][i] = pParams->blc_gb[i];
        pBlcCalib->mode_cell[mode_idx].level[3][i] = pParams->blc_b[i];
    }

    return XCAM_RETURN_NO_ERROR;
}



