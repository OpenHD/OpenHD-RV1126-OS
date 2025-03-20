/*
 * rk_aiq_algo_afec_itf.c
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

#include "rk_aiq_algo_types_int.h"
#include "afec/rk_aiq_algo_afec_itf.h"
#include "afec/rk_aiq_types_afec_algo_prvt.h"
#include "xcam_log.h"

#define EPSINON 0.0000001

RKAIQ_BEGIN_DECLARE

static XCamReturn release_fec_buf(FECContext_t* fecCtx);
static XCamReturn alloc_fec_buf(FECContext_t* fecCtx)
{
    // release-first
    release_fec_buf(fecCtx);
    rk_aiq_share_mem_config_t share_mem_config;
    share_mem_config.alloc_param.width =  fecCtx->dst_width;
    share_mem_config.alloc_param.height = fecCtx->dst_height;
    share_mem_config.alloc_param.reserved[0] = fecCtx->mesh_density;
    share_mem_config.mem_type = MEM_TYPE_FEC;
    fecCtx->share_mem_ops->alloc_mem(fecCtx->share_mem_ops,
                                     &share_mem_config,
                                     &fecCtx->share_mem_ctx);
    fecCtx->hasGenMeshInit = true;
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn release_fec_buf(FECContext_t* fecCtx)
{
    if (fecCtx->share_mem_ctx)
        fecCtx->share_mem_ops->release_mem(fecCtx->share_mem_ctx);

    fecCtx->hasGenMeshInit = false;
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn get_fec_buf(FECContext_t* fecCtx)
{
    fecCtx->fec_mem_info = (rk_aiq_fec_share_mem_info_t *)
            fecCtx->share_mem_ops->get_free_item(fecCtx->share_mem_ctx);
    if (fecCtx->fec_mem_info == NULL) {
        LOGE_AFEC( "%s(%d): no free fec buf", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_MEM;
    } else {
        LOGD_AFEC("get buf fd=%d\n",fecCtx->fec_mem_info->fd);
        fecCtx->meshxi = fecCtx->fec_mem_info->meshxi;
        fecCtx->meshxf = fecCtx->fec_mem_info->meshxf;
        fecCtx->meshyi = fecCtx->fec_mem_info->meshyi;
        fecCtx->meshyf = fecCtx->fec_mem_info->meshyf;
    }
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn rkaiq_gen_fec_mesh_init(FECContext_t* fecCtx)
{
    if (!fecCtx) {
        LOGE_AFEC("input params error!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    if (fecCtx->hasGenMeshInit) {
        return XCAM_RETURN_BYPASS;
    }

    fecCtx->fecParams.isFecOld = 1;
    genFecMeshInit(fecCtx->src_width, fecCtx->src_height, fecCtx->dst_width,
            fecCtx->dst_height, fecCtx->fecParams, fecCtx->camCoeff);
    fecCtx->fec_mesh_h_size = fecCtx->fecParams.meshSizeW;
    fecCtx->fec_mesh_v_size = fecCtx->fecParams.meshSizeH;
    fecCtx->fec_mesh_size = fecCtx->fecParams.meshSize4bin;
    LOGI_AFEC("en: %d, mode(%d), correct_level(%d), direction(%d), dimen(%d-%d), mesh dimen(%d-%d), size(%d)",
              fecCtx->fec_en,
              fecCtx->mode,
              fecCtx->user_config.correct_level,
              fecCtx->correct_direction,
              fecCtx->src_width, fecCtx->src_height,
              fecCtx->fec_mesh_h_size, fecCtx->fec_mesh_v_size,
              fecCtx->fec_mesh_size);

    alloc_fec_buf(fecCtx);
    fecCtx->hasGenMeshInit = true;

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn rkaiq_gen_fec_mesh_deinit(FECContext_t* fecCtx)
{
    if (!fecCtx) {
        LOGE_AFEC("input params error!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    if (!fecCtx->hasGenMeshInit)
        return XCAM_RETURN_BYPASS;

    if (fecCtx->fecParams.pMeshXY) {
        genFecMeshDeInit(fecCtx->fecParams);
        fecCtx->fecParams.pMeshXY = nullptr;
    }

    release_fec_buf(fecCtx);

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn rkaiq_gen_fec_mesh(FECContext_t* fecCtx, int level)
{
    if (!fecCtx) {
        LOGE_AFEC("input params error!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    if (!fecCtx->hasGenMeshInit) {
        LOGW_AFEC("hasn't init gen mesh lib!");
    }

    if (get_fec_buf(fecCtx) != XCAM_RETURN_NO_ERROR) {
        LOGE_AFEC("get fec buf failed!");
        return XCAM_RETURN_ERROR_MEM;
    }

    LOGE_AFEC("gen the mesh of level: %d!", level);
    bool ret = genFECMeshNLevel(fecCtx->fecParams, fecCtx->camCoeff, level, fecCtx->meshxi,
                           fecCtx->meshxf, fecCtx->meshyi, fecCtx->meshyf);
    if (!ret) {
        LOGE_AFEC("afec gen mesh false!");
        return XCAM_RETURN_ERROR_FAILED;
    }

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
create_context(RkAiqAlgoContext **context, const AlgoCtxInstanceCfg* cfg)
{
    RkAiqAlgoContext *ctx = new RkAiqAlgoContext();
    if (ctx == NULL) {
        LOGE_AFEC( "%s: create afec context fail!\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_MEM;
    }
    ctx->hFEC = new FECContext_t();
    if (ctx->hFEC == NULL) {
        LOGE_AFEC( "%s: create afec handle fail!\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_MEM;
    }
    memset((void *)(ctx->hFEC), 0, sizeof(FECContext_t));
    *context = ctx;

    const AlgoCtxInstanceCfgInt* cfg_int = (const AlgoCtxInstanceCfgInt*)cfg;
    FECHandle_t fecCtx = ctx->hFEC;
    CalibDb_FEC_t* calib_fec = &cfg_int->calib->afec;
    CalibDb_Eis_t* calib_eis = &cfg_int->calib->eis_calib;

    if (cfg_int->resCallback)
        fecCtx->resCallback = cfg_int->resCallback;
    else
        fecCtx->resCallback = nullptr;

    memset(&fecCtx->user_config, 0, sizeof(fecCtx->user_config));
    fecCtx->fec_en = fecCtx->user_config.en = calib_eis->enable ? 0 : calib_fec->fec_en;

    memcpy(fecCtx->meshfile, calib_fec->meshfile, sizeof(fecCtx->meshfile));
#if GENMESH_ONLINE
    fecCtx->camCoeff.cx = calib_fec->light_center[0];
    fecCtx->camCoeff.cy = calib_fec->light_center[1];
    fecCtx->camCoeff.a0 = calib_fec->coefficient[0];
    fecCtx->camCoeff.a2 = calib_fec->coefficient[1];
    fecCtx->camCoeff.a3 = calib_fec->coefficient[2];
    fecCtx->camCoeff.a4 = calib_fec->coefficient[3];
    LOGI_AFEC("(%s) len light center(%.16f, %.16f)\n",
            __FUNCTION__,
            fecCtx->camCoeff.cx, fecCtx->camCoeff.cy);
    LOGI_AFEC("(%s) len coefficient(%.16f, %.16f, %.16f, %.16f)\n",
            __FUNCTION__,
            fecCtx->camCoeff.a0, fecCtx->camCoeff.a2,
            fecCtx->camCoeff.a3, fecCtx->camCoeff.a4);
#endif
    fecCtx->correct_level = calib_fec->correct_level;
    fecCtx->correct_level = fecCtx->user_config.correct_level = calib_fec->correct_level;
    fecCtx->correct_direction = fecCtx->user_config.direction = FEC_CORRECT_DIRECTION_XY;
    fecCtx->fecParams.correctX = 1;
    fecCtx->fecParams.correctY = 1;
    fecCtx->fecParams.saveMesh4bin = 0;
    fecCtx->hasGenMeshInit = false;
    atomic_init(&fecCtx->isAttribUpdated, false);
    fecCtx->eis_enable = calib_eis->enable;

    if (fecCtx->fec_en) {
        if (fecCtx->eis_enable) {
            fecCtx->fec_en = fecCtx->user_config.en = 0;
            LOGE_AFEC("FEC diabled because of EIS");
        }
    }

    ctx->hFEC->eState = FEC_STATE_INITIALIZED;

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
destroy_context(RkAiqAlgoContext *context)
{
    FECHandle_t hFEC = (FECHandle_t)context->hFEC;
    FECContext_t* fecCtx = (FECContext_t*)hFEC;
#if GENMESH_ONLINE
    if (fecCtx->fec_en) {
        fecCtx->afecReadMeshThread->triger_stop();
        fecCtx->afecReadMeshThread->stop();
    }
    rkaiq_gen_fec_mesh_deinit(fecCtx);
#else
//    if (fecCtx->meshxi != NULL) {
//        free(fecCtx->meshxi);
//        fecCtx->meshxi = NULL;
//    }
 
//    if (fecCtx->meshyi != NULL) {
//        free(fecCtx->meshyi);
//        fecCtx->meshyi = NULL;
//    }

//    if (fecCtx->meshxf != NULL) {
//        free(fecCtx->meshxf);
//        fecCtx->meshxf = NULL;
//    }
//
//    if (fecCtx->meshyf != NULL) {
//        free(fecCtx->meshyf);
//        fecCtx->meshyf = NULL;
//    }
#endif
    delete context->hFEC;
    context->hFEC = NULL;
    delete context;
    context = NULL;
    return XCAM_RETURN_NO_ERROR;
}

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)      __ALIGN_MASK(x, a-1)

uint32_t cal_fec_mesh(uint32_t width, uint32_t height, uint32_t mode, uint32_t &meshw, uint32_t &meshh)
{
    uint32_t mesh_size, mesh_left_height;
    uint32_t w = ALIGN(width, 32);
    uint32_t h = ALIGN(height, 32);
    uint32_t spb_num = (h + 127) >> 7;
    uint32_t left_height = h & 127;
    uint32_t mesh_width = mode ? (w / 32 + 1) : (w / 16 + 1);
    uint32_t mesh_height = mode ? 9 : 17;

    if (!left_height)
        left_height = 128;
    mesh_left_height = mode ? (left_height / 16 + 1) :
                       (left_height / 8 + 1);
    mesh_size = (spb_num - 1) * mesh_width * mesh_height +
                mesh_width * mesh_left_height;

    meshw = mesh_width;
    meshh = (spb_num - 1) * mesh_height + (spb_num - 1);
    return mesh_size;
}

static XCamReturn
read_mesh_table(FECContext_t* fecCtx, unsigned int correct_level)
{
    bool ret;
#if OPENCV_SUPPORT
    gen_default_mesh_table(fecCtx->src_width, fecCtx->src_height, fecCtx->mesh_density,
                           fecCtx->meshxi, fecCtx->meshyi, fecCtx->meshxf, fecCtx->meshyf);
#else
    FILE* ofp;
    char filename[512];
    sprintf(filename, "%s/%s/meshxi_level.bin",
            fecCtx->resource_path,
            fecCtx->meshfile);
    ofp = fopen(filename, "rb");
    if (ofp != NULL) {
        unsigned int num = fread(fecCtx->meshxi, 1, fecCtx->fec_mesh_size * sizeof(unsigned short), ofp);
        fclose(ofp);

        if (num != fecCtx->fec_mesh_size * sizeof(unsigned short)) {
            fecCtx->fec_en = 0;
            LOGE_AFEC("mismatched mesh XI file");
        }
    } else {
        LOGE_AFEC("mesh XI file %s not exist", filename);
        fecCtx->fec_en = 0;
    }

    sprintf(filename, "%s/%s/meshxf_level.bin",
            fecCtx->resource_path,
            fecCtx->meshfile);
    ofp = fopen(filename, "rb");
    if (ofp != NULL) {
        unsigned int num = fread(fecCtx->meshxf, 1, fecCtx->fec_mesh_size * sizeof(unsigned char), ofp);
        fclose(ofp);
        if (num != fecCtx->fec_mesh_size * sizeof(unsigned char)) {
            fecCtx->fec_en = 0;
            LOGE_AFEC("mismatched mesh XF file");
        }
    } else {
        LOGE_AFEC("mesh XF file %s not exist", filename);
        fecCtx->fec_en = 0;
    }

    sprintf(filename, "%s/%s/meshyi_level.bin",
            fecCtx->resource_path,
            fecCtx->meshfile);
    ofp = fopen(filename, "rb");
    if (ofp != NULL) {
        unsigned int num = fread(fecCtx->meshyi, 1, fecCtx->fec_mesh_size * sizeof(unsigned short), ofp);
        fclose(ofp);
        if (num != fecCtx->fec_mesh_size * sizeof(unsigned short)) {
            fecCtx->fec_en = 0;
            LOGE_AFEC("mismatched mesh YI file");
        }
    } else {
        LOGE_AFEC("mesh YI file %s not exist", filename);
        fecCtx->fec_en = 0;
    }

    sprintf(filename, "%s/%s/meshyf_level.bin",
            fecCtx->resource_path,
            fecCtx->meshfile);
    ofp = fopen(filename, "rb");
    if (ofp != NULL) {
        unsigned int num = fread(fecCtx->meshyf, 1, fecCtx->fec_mesh_size * sizeof(unsigned char), ofp);
        fclose(ofp);
        if (num != fecCtx->fec_mesh_size * sizeof(unsigned char)) {
            fecCtx->fec_en = 0;
            LOGE_AFEC("mismatched mesh YF file");
        }
    } else {
        LOGE_AFEC("mesh YF file %s not exist", filename);
        fecCtx->fec_en = 0;
    }
#endif

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
prepare(RkAiqAlgoCom* params)
{
    FECHandle_t hFEC = (FECHandle_t)params->ctx->hFEC;
    FECContext_t* fecCtx = (FECContext_t*)hFEC;
    RkAiqAlgoConfigAfecInt* rkaiqAfecConfig = (RkAiqAlgoConfigAfecInt*)params;

    fecCtx->share_mem_ops = rkaiqAfecConfig->mem_ops_ptr;
    fecCtx->src_width = params->u.prepare.sns_op_width;
    fecCtx->src_height = params->u.prepare.sns_op_height;
    fecCtx->resource_path = rkaiqAfecConfig->resource_path;
    fecCtx->dst_width = params->u.prepare.sns_op_width;
    fecCtx->dst_height = params->u.prepare.sns_op_height;
    fecCtx->working_mode = params->u.prepare.working_mode;

    if (fecCtx->src_width <= 1920) {
        fecCtx->mesh_density = 0;
    } else {
        fecCtx->mesh_density = 1;
    }

    /*
     * don't reinitialize FEC information in the following cases:
     *  1. Switch between HDR and normal mode
     *  2. update calib
     */
    if (fecCtx->hasGenMeshInit &&
        (fecCtx->working_mode != params->u.prepare.working_mode || \
        !!(params->u.prepare.conf_type & RK_AIQ_ALGO_CONFTYPE_UPDATECALIB))) {
        LOGW_AFEC("don't reinitialize FEC info when switching modes or updating calib\n");
        return XCAM_RETURN_NO_ERROR;
    }

#if GENMESH_ONLINE
    int correct_level = fecCtx->correct_level;
    if (fecCtx->isAttribUpdated.load()) {
        fecCtx->fec_en = fecCtx->user_config.en;
        correct_level = fecCtx->user_config.correct_level;
        switch (fecCtx->user_config.direction) {
        case FEC_CORRECT_DIRECTION_X:
            fecCtx->fecParams.correctY = 0;
            fecCtx->correct_direction = fecCtx->user_config.direction;
            break;
        case FEC_CORRECT_DIRECTION_Y:
            fecCtx->fecParams.correctX = 0;
            fecCtx->correct_direction = fecCtx->user_config.direction;
            break;
        default:
            break;
        }

        fecCtx->mode = fecCtx->user_config.mode;
        switch (fecCtx->mode) {
        case FEC_COMPRES_IMAGE_KEEP_FOV:
            fecCtx->fecParams.saveMaxFovX = 1;
            break;
        case FEC_KEEP_ASPECT_RATIO_REDUCE_FOV:
            fecCtx->fecParams.saveMaxFovX = 0;
            break;
        case FEC_ALTER_ASPECT_RATIO_KEEP_FOV:
            break;
        default:
            break;
        }

        if (fecCtx->fecParams.saveMesh4bin)
            sprintf(fecCtx->fecParams.mesh4binPath, "/tmp/");

        fecCtx->isAttribUpdated.store(false);
    }

    if (!fecCtx->fec_en)
        goto out;

    if (rkaiq_gen_fec_mesh_init(fecCtx) < 0) {
        LOGW_AFEC("Failed to init gen fec mesh!");
    }

    if (rkaiq_gen_fec_mesh(fecCtx, correct_level) < 0)
        LOGE_AFEC("gen fec mesh failed!");
#else
    if (fecCtx->isAttribUpdated.load()){
        fecCtx->fec_en = fecCtx->user_config.en;
        fecCtx->isAttribUpdated.store(false);
    }

    fecCtx->fec_mesh_size =
        cal_fec_mesh(fecCtx->src_width, fecCtx->src_height, fecCtx->mesh_density,
                     fecCtx->fec_mesh_h_size, fecCtx->fec_mesh_v_size);

    LOGI_AFEC("(%s) en: %d, user_en: %d, correct_level: %d, dimen: %d-%d, mesh dimen: %d-%d, size: %d",
              fecCtx->meshfile, fecCtx->fec_en,
              fecCtx->user_config.en, fecCtx->user_config.correct_level,
              fecCtx->src_width, fecCtx->src_height,
              fecCtx->fec_mesh_h_size, fecCtx->fec_mesh_v_size,
              fecCtx->fec_mesh_size);
    if (!fecCtx->fec_en)
        goto out;

    read_mesh_table(fecCtx, fecCtx->user_config.correct_level);
#endif

out:
    fecCtx->eState = FEC_STATE_STOPPED;

    if (fecCtx->fec_en && !fecCtx->afecReadMeshThread.ptr()) {
        fecCtx->afecReadMeshThread = new RKAiqAfecThread(fecCtx);
        if (fecCtx->afecReadMeshThread.ptr()) {
            fecCtx->afecReadMeshThread->triger_start();
            fecCtx->afecReadMeshThread->start();
        }
    }

    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
pre_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
processing(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    FECHandle_t hFEC = (FECHandle_t)inparams->ctx->hFEC;
    FECContext_t* fecCtx = (FECContext_t*)hFEC;
    RkAiqAlgoProcAfecInt* fecProcParams = (RkAiqAlgoProcAfecInt*)inparams;
    RkAiqAlgoProcResAfecInt* fecPreOut = (RkAiqAlgoProcResAfecInt*)outparams;

    fecPreOut->afec_result.sw_fec_en = fecCtx->fec_en;
    fecPreOut->afec_result.crop_en = 0;
    fecPreOut->afec_result.crop_width = 0;
    fecPreOut->afec_result.crop_height = 0;
    fecPreOut->afec_result.mesh_density = fecCtx->mesh_density;
    fecPreOut->afec_result.mesh_size = fecCtx->fec_mesh_size;
    // TODO: should check the fec mode,
    // if mode == RK_AIQ_ISPP_STATIC_FEC_WORKING_MODE_STABLIZATION
    // params may be changed

    if (inparams->u.proc.init && fecCtx->eState == FEC_STATE_STOPPED) {
        fecPreOut->afec_result.update = 1;
    } else {
        if (fecCtx->isAttribUpdated.load()) {
            fecCtx->isAttribUpdated.store(false);
            fecPreOut->afec_result.update = 1;
            LOGV_AFEC("en(%d), level(%d), direction(%d), result update(%d)\n",
                    fecCtx->fec_en,
                    fecCtx->correct_level,
                    fecCtx->correct_direction,
                    fecPreOut->afec_result.update);
        } else {
            fecPreOut->afec_result.update = 0;
            LOG1_AFEC("en(%d) level(%d), direction(%d), result update(%d)\n",
                    fecCtx->fec_en,
                    fecCtx->correct_level,
                    fecCtx->correct_direction,
                    fecPreOut->afec_result.update);
        }

    }

    if (fecPreOut->afec_result.update) {
        //memcpy(fecPreOut->afec_result.meshxi, fecCtx->meshxi,
        //       fecCtx->fec_mesh_size * sizeof(unsigned short));
        //memcpy(fecPreOut->afec_result.meshxf, fecCtx->meshxf,
        //       fecCtx->fec_mesh_size * sizeof(unsigned char));
        //memcpy(fecPreOut->afec_result.meshyi, fecCtx->meshyi,
        //       fecCtx->fec_mesh_size * sizeof(unsigned short));
        //memcpy(fecPreOut->afec_result.meshyf, fecCtx->meshyf,
        //       fecCtx->fec_mesh_size * sizeof(unsigned char));

        if (fecCtx->fec_mem_info == NULL) {
            LOGE_AFEC("%s: no available fec buf!", __FUNCTION__);
            fecPreOut->afec_result.update = 0;
            goto out;
        }
        fecPreOut->afec_result.mesh_buf_fd = fecCtx->fec_mem_info->fd;
        fecCtx->fec_mem_info->state[0] = 1; //mark that this buf is using.

        if (fecCtx->resCallback)
            fecCtx->resCallback(RK_AIQ_ALGO_TYPE_AFEC, (RkAiqAlgoResCom*)fecPreOut);
    }

out:
    fecCtx->eState = FEC_STATE_RUNNING;
    return XCAM_RETURN_NO_ERROR;
}

static XCamReturn
post_process(const RkAiqAlgoCom* inparams, RkAiqAlgoResCom* outparams)
{
    return XCAM_RETURN_NO_ERROR;
}

RkAiqAlgoDescription g_RkIspAlgoDescAfec = {
    .common = {
        .version = RKISP_ALGO_AFEC_VERSION,
        .vendor  = RKISP_ALGO_AFEC_VENDOR,
        .description = RKISP_ALGO_AFEC_DESCRIPTION,
        .type    = RK_AIQ_ALGO_TYPE_AFEC,
        .id      = 0,
        .create_context  = create_context,
        .destroy_context = destroy_context,
    },
    .prepare = prepare,
    .pre_process = pre_process,
    .processing = processing,
    .post_process = post_process,
};

RKAIQ_END_DECLARE

bool RKAiqAfecThread::loop()
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    ENTER_ANALYZER_FUNCTION();

    const static int32_t timeout = -1;
    SmartPtr<rk_aiq_fec_cfg_t> attrib = mAttrQueue.pop (timeout);

    if (!attrib.ptr())
        return true;

    if (attrib->en &&
        hFEC->correct_level != attrib->correct_level) {
        ret = rkaiq_gen_fec_mesh_init(hFEC);
        if (ret >= 0) {
            if (rkaiq_gen_fec_mesh(hFEC, attrib->correct_level) < 0)
                LOGE_AFEC("gen fec mesh failed!");
        }
    }

    if (ret >= 0) {
        hFEC->fec_en = attrib->en;
        hFEC->correct_level = attrib->correct_level;
        hFEC->correct_direction = attrib->direction;
        hFEC->mode = attrib->mode;
        hFEC->isAttribUpdated.store(true);
        LOGD_AFEC("update attrib: en: %d, mode: %d, direction: %d",
                attrib->en, attrib->correct_level,
                attrib->mode, attrib->direction);
        return true;
    }

    EXIT_ANALYZER_FUNCTION();

    return true;
}
