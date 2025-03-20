#include "rk_aiq_uapi_agamma_int.h"
#include "agamma/rk_aiq_types_agamma_algo_prvt.h"

XCamReturn
rk_aiq_agamma_OverWriteCalibByCurAttr
(
    const RkAiqAlgoContext *ctx,
    CalibDb_Gamma_t *agamma
)
{
    ENTER_ANALYZER_FUNCTION();

    AgammaHandle_t *gamma_handle = (AgammaHandle_t *)ctx;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;


    if(gamma_handle->agammaAttr.mode < RK_AIQ_GAMMA_MODE_TOOL)
        memcpy(agamma, gamma_handle->pCalibDb, sizeof(CalibDb_Gamma_t));
    else if(gamma_handle->agammaAttr.mode == RK_AIQ_GAMMA_MODE_TOOL)
        memcpy(agamma, &gamma_handle->agammaAttr.stTool, sizeof(CalibDb_Gamma_t));

    EXIT_ANALYZER_FUNCTION();
    return (ret);

}

XCamReturn
rk_aiq_uapi_agamma_SetAttrib(RkAiqAlgoContext *ctx,
                             rk_aiq_gamma_attrib_t attr,
                             bool need_sync)
{
    AgammaHandle_t *gamma_handle = (AgammaHandle_t *)ctx;
    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    memcpy(&gamma_handle->agammaAttr, &attr, sizeof(rk_aiq_gamma_attr_t));

    return ret;
}

XCamReturn
rk_aiq_uapi_agamma_GetAttrib(const RkAiqAlgoContext *ctx,
                             rk_aiq_gamma_attrib_t *attr)
{

    AgammaHandle_t* gamma_handle = (AgammaHandle_t*)ctx;

    memcpy(attr, &gamma_handle->agammaAttr, sizeof(rk_aiq_gamma_attr_t));

    return XCAM_RETURN_NO_ERROR;
}




