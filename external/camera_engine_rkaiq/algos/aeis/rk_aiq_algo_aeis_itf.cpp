/*
 * rk_aiq_algo_aeis_itf.c
 *
 *  Copyright (c) 2021 Rockchip Electronics Co., Ltd
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
 * Author: Cody Xie <cody.xie@rock-chips.com>
 */

#include "aeis/rk_aiq_algo_aeis_itf.h"

#include <memory>
#include <fstream>

#include "aeis/rk_aiq_types_aeis_algo_prvt.h"
#include "drm_device.h"
#include "drm_buffer.h"
//#include "dma_buffer.h"
#include "dma_video_buffer.h"
#include "dvs_app.h"
#include "remap_backend.h"
#include "rk_aiq_algo_types_int.h"
#include "unistd.h"
#include "xcam_common.h"
#include "xcam_log.h"
#include "imu_service.h"
#include "video_buffer.h"
#include "image_processor.h"
#include "scaler_service.h"
#include "eis_algo_service.h"

using namespace RkCam;
using namespace XCam;

RKAIQ_BEGIN_DECLARE

static XCamReturn create_context(RkAiqAlgoContext** context, const AlgoCtxInstanceCfg* cfg) {
    RkAiqAlgoContext* ctx = new RkAiqAlgoContext();
    if (ctx == nullptr) {
        LOGE_AEIS("create aeis context fail!");
        return XCAM_RETURN_ERROR_MEM;
    }

    auto adaptor = new EisAlgoAdaptor();
    if (adaptor == nullptr) {
        LOGE_AEIS("create aeis handle fail!");
        delete ctx;
        return XCAM_RETURN_ERROR_MEM;
    }

    const AlgoCtxInstanceCfgInt* cfg_int = (const AlgoCtxInstanceCfgInt*)cfg;
    CamCalibDbContext_t* pcalib = cfg_int->calib;
    CalibDb_Eis_t* calib_eis = &pcalib->eis_calib;
    adaptor->Config(cfg_int, calib_eis);

    ctx->handle = static_cast<void*>(adaptor);
    *context = ctx;

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn destroy_context(RkAiqAlgoContext* context) {
    if (context) {
        if (context->handle) {
            EisAlgoAdaptor* adaptor = static_cast<EisAlgoAdaptor*>(context->handle);
            delete adaptor;
            context->handle = nullptr;
        }
        delete context;
    }
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn prepare(RkAiqAlgoCom* params) {
    EisAlgoAdaptor* adaptor = static_cast<EisAlgoAdaptor*>(params->ctx->handle);
    RkAiqAlgoConfigAeisInt* config = (RkAiqAlgoConfigAeisInt*)params;

    return adaptor->Prepare(config->common.mems_sensor_intf, config->mem_ops);
}

static XCamReturn pre_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams) {
    RkAiqAlgoPreAeisInt* input = (RkAiqAlgoPreAeisInt*)(inparams);
    EisAlgoAdaptor* adaptor = static_cast<EisAlgoAdaptor*>(inparams->ctx->handle);

    if (!adaptor->IsEnabled()) {
        return XCAM_RETURN_NO_ERROR;
    }

    if (inparams->u.proc.init) {
        adaptor->Start();
    }

    if (!adaptor->IsValid()) {
        return XCAM_RETURN_BYPASS;
    }


    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams) {
    RkAiqAlgoProcAeisInt* input = (RkAiqAlgoProcAeisInt*)(inparams);
    RkAiqAlgoProcResAeisInt* output = (RkAiqAlgoProcResAeisInt*)outparams;
    EisAlgoAdaptor* adaptor = static_cast<EisAlgoAdaptor*>(inparams->ctx->handle);

    if (!adaptor->IsEnabled()) {
        return XCAM_RETURN_NO_ERROR;
    }

    if (!adaptor->IsValid()) {
        return XCAM_RETURN_BYPASS;
    }

    output->fec_en = true;
#if 0
    // Anyway adaptor will prepare default mesh from file in prepare state
    if (inparams->u.proc.init) {
        output->update = 1;
        output->fd = -1;
        return XCAM_RETURN_NO_ERROR;
    }
#endif

    adaptor->GetProcResult(output);

    adaptor->OnFrameEvent(input);

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams) {
    EisAlgoAdaptor* adaptor = static_cast<EisAlgoAdaptor*>(inparams->ctx->handle);

    if (!adaptor->IsEnabled()) {
        return XCAM_RETURN_NO_ERROR;
    }

    if (!adaptor->IsValid()) {
        return XCAM_RETURN_BYPASS;
    }

    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAeis = {
    .common =
        {
            .version         = RKISP_ALGO_AEIS_VERSION,
            .vendor          = RKISP_ALGO_AEIS_VENDOR,
            .description     = RKISP_ALGO_AEIS_DESCRIPTION,
            .type            = RK_AIQ_ALGO_TYPE_AEIS,
            .id              = 0,
            .create_context  = create_context,
            .destroy_context = destroy_context,
        },
    .prepare      = prepare,
    .pre_process  = pre_process,
    .processing   = processing,
    .post_process = post_process,
};

RKAIQ_END_DECLARE
