#include "rk_aiq_uapi_adehaze_int.h"
#include "rk_aiq_types_adehaze_algo_prvt.h"
#include "xcam_log.h"

XCamReturn
rk_aiq_adehaze_OverWriteCalibByCurAttr
(
    const RkAiqAlgoContext *ctx,
    CalibDb_Dehaze_t *adehaze
)
{
    ENTER_ANALYZER_FUNCTION();

    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    int mode = AdehazeHandle->Dehaze_Scene_mode;

    if(AdehazeHandle->AdehazeAtrr.mode != RK_AIQ_DEHAZE_MODE_TOOL || AdehazeHandle->AdehazeAtrr.byPass)
        memcpy(adehaze, &AdehazeHandle->pCalibDb->dehaze, sizeof(CalibDb_Dehaze_t));
    else if(AdehazeHandle->AdehazeAtrr.mode == RK_AIQ_DEHAZE_MODE_TOOL)
        memcpy(adehaze, &AdehazeHandle->AdehazeAtrr.stTool, sizeof(CalibDb_Dehaze_t));

    EXIT_ANALYZER_FUNCTION();
    return (ret);

}

XCamReturn
rk_aiq_uapi_adehaze_SetAttrib(RkAiqAlgoContext *ctx,
                              adehaze_sw_t attr,
                              bool need_sync)
{
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;
    LOGD_ADEHAZE("%s: setMode:%d", __func__, attr.mode);
    AdehazeHandle->AdehazeAtrr.byPass = attr.byPass;
    AdehazeHandle->AdehazeAtrr.mode = attr.mode;

    if(attr.mode == RK_AIQ_DEHAZE_MODE_MANUAL || attr.mode == RK_AIQ_ENHANCE_MODE_MANUAL)
        memcpy(&AdehazeHandle->AdehazeAtrr.stManual, &attr.stManual, sizeof(rk_aiq_dehaze_M_attrib_t));
    else if(attr.mode == RK_AIQ_DEHAZE_MODE_TOOL)
        memcpy(&AdehazeHandle->AdehazeAtrr.stTool, &attr.stTool, sizeof(CalibDb_Dehaze_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_adehaze_GetAttrib(RkAiqAlgoContext *ctx, adehaze_sw_t *attr)
{
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;

    attr->byPass = AdehazeHandle->AdehazeAtrr.byPass;
    attr->mode = AdehazeHandle->AdehazeAtrr.mode;
    memcpy(&attr->stManual, &AdehazeHandle->AdehazeAtrr.stManual, sizeof(rk_aiq_dehaze_M_attrib_t));
    memcpy(&attr->stTool, &AdehazeHandle->AdehazeAtrr.stTool, sizeof(CalibDb_Dehaze_t));

    return XCAM_RETURN_NO_ERROR;
}

