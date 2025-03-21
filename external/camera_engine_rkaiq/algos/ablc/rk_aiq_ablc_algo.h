
#ifndef _RKAIQ_ABLC_H_
#define _RKAIQ_ABLC_H_

#include "stdio.h"
#include "string.h"
#include "math.h"
#include "base/xcam_log.h"
#include "rk_aiq_comm.h"
#include "RkAiqCalibDbTypes.h"
#include "ablc/rk_aiq_types_ablc_algo_prvt.h"



AblcResult_t AblcInit(AblcContext_t **ppAblcCtx, CamCalibDbContext_t *pCalibDb);

AblcResult_t AblcRelease(AblcContext_t *pAblcCtx);

AblcResult_t AblcConfig(AblcContext_t *pAblcCtx, AblcConfig_t* pAblcConfig);

AblcResult_t AblcReConfig(AblcContext_t *pAblcCtx, AblcConfig_t* pAblcConfig);

AblcResult_t AblcPreProcess(AblcContext_t *pAblcCtx);

AblcResult_t AblcProcess(AblcContext_t *pAblcCtx, AblcExpInfo_t *pExpInfo);

AblcResult_t AblcGetProcResult(AblcContext_t *pAblcCtx, AblcProcResult_t* pAblcResult);

AblcResult_t Ablc_config_mode_param(AblcContext_t *pAblcCtx, CalibDb_Blc_t* pBlcCalib, AblcParamMode_t eParamMode);

AblcResult_t Ablc_get_mode_cell_idx_by_name(CalibDb_Blc_t *pCalibdb, char *name, int *mode_idx);


#endif

