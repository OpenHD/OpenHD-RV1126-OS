/*
 * eis_algo_service.h
 *
 *  Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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
#include "eis_algo_service.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

#include "rkispp-config.h"
#include "dma_buffer.h"
#include "dvs_app.h"
#include "eis_loader.h"
#include "image_processor.h"
#include "imu_service.h"
#include "rk_aiq_mems_sensor.h"
#include "xcam_log.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define SUCCESS 0
#define ERROR 1

using namespace XCam;

namespace RkCam {

namespace {

template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
    template <typename... Args>
    static Ret callback(Args... args) {
        return func(args...);
    }
    static std::function<Ret(Params...)> func;
};

template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

#ifdef USE_STRUCT_INIT
void setInitialParams(const CalibDb_Eis_t* calib,
                      dvsInitialParams* pinit_params) {
    pinit_params->input_image_height = calib->src_image_height;
    pinit_params->input_image_width = calib->src_image_width;
    pinit_params->output_image_height = calib->dst_image_height;
    pinit_params->output_image_width = calib->dst_image_width;
    pinit_params->image_buffer_num = calib->image_buffer_num;
    pinit_params->clipx_ratio = calib->clip_ratio_x;
    pinit_params->clipy_ratio = calib->clip_ratio_y;

    //读取相机配置参数
    pinit_params->camera_type = static_cast<rk_camera_type>(calib->camera_model_type);
    pinit_params->camera_rate = calib->camera_rate;
    pinit_params->fx = calib->focal_length[0];
    pinit_params->fy = calib->focal_length[1];
    pinit_params->cx = calib->light_center[0];
    pinit_params->cy = calib->light_center[1];
    pinit_params->dt0 = calib->distortion_coeff[0];
    pinit_params->dt1 = calib->distortion_coeff[1];
    pinit_params->dt2 = calib->distortion_coeff[2];
    pinit_params->dt3 = calib->distortion_coeff[3];
    pinit_params->dt4 = calib->distortion_coeff[4];
    pinit_params->Xi = calib->xi; 
    pinit_params->sensor_axes_type = calib->sensor_axes_type;
    //读取陀螺仪配置参数
    pinit_params->imu_rate = calib->imu_rate;
    pinit_params->gbias_x = calib->gbias_x;
    pinit_params->gbias_y = calib->gbias_y;
    pinit_params->gbias_z = calib->gbias_z;
    pinit_params->time_offset = calib->time_offset;

    //算法功能选取
    pinit_params->use_dvs = 1;
    pinit_params->output_undist = 0;
}
#endif

};  // namespace

EisAlgoAdaptor::~EisAlgoAdaptor() {
    Stop();

    if (lib_ != nullptr && engine_ != nullptr) {
        lib_->GetOps()->DeInit(engine_.get());
    }
}

XCamReturn EisAlgoAdaptor::LoadLibrary() {
    lib_ = std::make_shared<DvsLibrary>();

    if (!lib_->Init()) {
        return XCAM_RETURN_ERROR_FAILED;
    }

    if (!lib_->LoadSymbols()) {
        return XCAM_RETURN_ERROR_FAILED;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn EisAlgoAdaptor::CreateImuService(
    const rk_aiq_mems_sensor_intf_t* mems_sensor_intf) {
    if (mems_sensor_intf == nullptr) {
        return XCAM_RETURN_ERROR_PARAM;
    }

    auto adaptor = std::make_shared<EisImuAdaptor>(*mems_sensor_intf);
    if (XCAM_RETURN_NO_ERROR == adaptor->Init(1000.0)) {
        imu_ = std::unique_ptr<ImuService>(new ImuService(
            std::unique_ptr<ImuTask>(new ImuTask(std::move(adaptor))), false, 5,
            std::chrono::milliseconds(10)));
    } else {
        imu_ = nullptr;
        return XCAM_RETURN_ERROR_PARAM;
    }

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn EisAlgoAdaptor::CreateScalerService() {
    std::unique_ptr<ImageProcessor> proc(new ImageProcessor());
    proc->set_operator("rga");
    scl_ = std::unique_ptr<ScalerService>(new ScalerService(
        std::unique_ptr<ScalerTask>(new ScalerTask(std::move(proc))), true, 7));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn EisAlgoAdaptor::CreateFecRemapBackend(
    const FecMeshConfig& config, const isp_drv_share_mem_ops_t* mem_ops) {
    XCAM_ASSERT(mem_ops != nullptr);

    remap_ =
        std::unique_ptr<FecRemapBackend>(new FecRemapBackend(config, mem_ops));

    return XCAM_RETURN_NO_ERROR;
}

int EisAlgoAdaptor::OnMeshCallback(struct dvsEngine* engine,
                                   struct meshxyFEC* mesh) {
    XCAM_ASSERT(mesh != nullptr);

    LOGD_AEIS("OnMeshCallback got img id %d , mesh idx %d, img idx %d",
              mesh->image_index, mesh->mesh_buffer_index,
              mesh->image_buffer_index);
    remap_->Remap(mesh);
    if (lib_ != nullptr) {
        auto* new_mesh = remap_->GetAvailUserBuffer();
        if (new_mesh != nullptr) {
            auto dvs_new_mesh = dvs_meshes_.find(new_mesh->Index);
            if (dvs_new_mesh != dvs_meshes_.end()) {
                LOGD_AEIS("OnMeshCallBack push back available mesh id %d",
                          dvs_new_mesh->second->mesh_buffer_index);
                lib_->GetOps()->PutMesh(engine_.get(),
                                        dvs_new_mesh->second.get());
            }
        }
    }

    auto img = dvs_images_.find(mesh->image_index);
    if (img != dvs_images_.end()) {
        dvs_images_.erase(img);
    } else {
        return 1;
    }
    return 0;
}

XCamReturn EisAlgoAdaptor::Config(const AlgoCtxInstanceCfgInt* config,
                                  const CalibDb_Eis_t* calib) {
    calib_ = calib;
    enable_ = calib_->enable;

    if (config->cfg_com.isp_hw_version == 1) {
        valid_ = false;
        LOGE_AEIS("EIS does not compatible with ISP21");
        return XCAM_RETURN_BYPASS;
    }

    if (XCAM_RETURN_NO_ERROR != LoadLibrary()) {
        LOGE_AEIS("EIS library does not exists");
        valid_ = false;
        return XCAM_RETURN_BYPASS;
    }

    result_callback_ = config->resCallback;

    valid_ = true;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn EisAlgoAdaptor::Prepare(
    const rk_aiq_mems_sensor_intf_t* mems_sensor_intf,
    const isp_drv_share_mem_ops_t* mem_ops) {
    if (!calib_->enable && !enable_) {
        return XCAM_RETURN_NO_ERROR;
    }

    if (!valid_) {
        LOGE_AEIS("EIS Invalid, bypassing!");
        return XCAM_RETURN_BYPASS;
    }

    XCamReturn ret;

    if (calib_->mode == EIS_MODE_IMU_ONLY ||
        calib_->mode == EIS_MODE_IMU_AND_IMG) {
        ret = CreateImuService(mems_sensor_intf);
        if (ret != XCAM_RETURN_NO_ERROR) {
            valid_ = false;
            LOGE_AEIS("EIS IMU interface invalid, bypassing!");
            return XCAM_RETURN_BYPASS;
        }
    }

    if (calib_->mode == EIS_MODE_IMG_ONLY ||
        calib_->mode == EIS_MODE_IMU_AND_IMG) {
        ret = CreateScalerService();
        if (ret != XCAM_RETURN_NO_ERROR) {
            valid_ = false;
            if (calib_->mode == EIS_MODE_IMU_AND_IMG) {
                imu_ = nullptr;
            }
            LOGE_AEIS("EIS scaler interface invalid, bypassing!");
            return XCAM_RETURN_BYPASS;
        }
    }

    int mesh_size;
    lib_->GetOps()->GetMeshSize(calib_->src_image_height,
                                calib_->src_image_width, &mesh_size);
    FecMeshConfig FecCfg;
    FecCfg.Width = calib_->src_image_width;
    FecCfg.Height = calib_->src_image_height;
    if (FecCfg.Width <= 1920)
        FecCfg.MeshDensity = 0;
    else
        FecCfg.MeshDensity = 1;
    FecCfg.MeshSize = mesh_size;

    // TODO(Cody): FEC may support different src and dst image width/height
    ret = CreateFecRemapBackend(FecCfg, mem_ops);
    if (ret != XCAM_RETURN_NO_ERROR) {
        valid_ = false;
        LOGE_AEIS("EIS remap backend invalid, bypassing!");
        return XCAM_RETURN_BYPASS;
    }

    // TODO(Cody): width/height may change during runtime
    engine_ = std::unique_ptr<dvsEngine>(new dvsEngine());

    lib_->GetOps()->Prepare(engine_.get());

#ifdef USE_STRUCT_INIT
    dvsInitialParams pinit_params;
    setInitialParams(calib_, &pinit_params);
    lib_->GetOps()->InitFromStruct(engine_.get(), &pinit_params);
#else
    initialParams init_params;
    init_params.image_buffer_number = FEC_MESH_BUF_NUM;
    init_params.input_image_size.width = calib_->src_image_width;
    init_params.input_image_size.height = calib_->src_image_height;
    init_params.output_image_size.width = calib_->src_image_width;
    init_params.output_image_size.height = calib_->src_image_width;
    init_params.clip_ratio_x = calib_->clip_ratio_x;
    init_params.clip_ratio_y = calib_->clip_ratio_y;
    lib_->GetOps()->InitParams(engine_.get(), &init_params);

    if (!lib_->GetOps()->InitFromXmlFile(engine_.get(),
                                         calib_->debug_xml_path)) {
        valid_ = false;
        LOGE_AEIS("EIS init algo from xml failed, bypassing!");
        return XCAM_RETURN_BYPASS;
    }
#endif // USE_STRUCT_INIT

    for (int i = 0; i < FEC_MESH_BUF_NUM; i++) {
        auto* mesh = remap_->AllocUserBuffer();
        struct meshxyFEC* dvs_mesh = new meshxyFEC;
        dvs_mesh->is_skip = false;
        dvs_mesh->image_buffer_index = mesh->ImageBufferIndex;
        dvs_mesh->image_index = mesh->FrameId;
        dvs_mesh->mesh_buffer_index = mesh->Index;
        dvs_mesh->mesh_size = remap_->GetConfig().MeshSize;
        dvs_mesh->pMeshXF = mesh->MeshXf;
        dvs_mesh->pMeshXI = mesh->MeshXi;
        dvs_mesh->pMeshYF = mesh->MeshYf;
        dvs_mesh->pMeshYI = mesh->MeshYi;
        dvs_meshes_.emplace(dvs_mesh->mesh_buffer_index, dvs_mesh);
        remap_meshes_.emplace(mesh->Index, mesh);
        if (i == FEC_MESH_BUF_NUM - 1) {
            dvs_mesh->image_index = mesh->FrameId = 0;
            lib_->GetOps()->GetOriginalMeshXY(
                calib_->src_image_width, calib_->src_image_height,
                calib_->clip_ratio_x, calib_->clip_ratio_y, dvs_mesh);
            remap_->Remap(dvs_mesh);
            default_mesh_ = mesh;
        } else {
            lib_->GetOps()->PutMesh(engine_.get(), dvs_mesh);
        }
    }

    Callback<int(struct dvsEngine*, struct meshxyFEC*)>::func =
        std::bind(&EisAlgoAdaptor::OnMeshCallback, this, std::placeholders::_1,
                  std::placeholders::_2);
    dvsFrameCallBackFEC mesh_cb = static_cast<dvsFrameCallBackFEC>(
        Callback<int(struct dvsEngine*, struct meshxyFEC*)>::callback);
    lib_->GetOps()->RegisterRemap(engine_.get(), mesh_cb);

    return XCAM_RETURN_NO_ERROR;
}

void EisAlgoAdaptor::Start() {
    if (started_ || !valid_) {
        return;
    }

    if (imu_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        imu_->start();
    }

    if (scl_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        scl_->start();
    }

    if (lib_->GetOps()->Start(engine_.get())) {
        lib_->GetOps()->DeInit(engine_.get());
        goto error_out;
    }

    started_ = true;
    return;

error_out:
    if (imu_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        imu_->stop();
        imu_ = nullptr;
    }

    if (scl_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        scl_->stop();
        scl_ = nullptr;
    }

    started_ = false;
    engine_ = nullptr;
    valid_ = false;
}

void EisAlgoAdaptor::OnFrameEvent(const RkAiqAlgoProcAeisInt* input) {
    auto id = input->common.frame_id;

    LOGV_AEIS("OnFrameEvent %" PRId64 " skew %lf igt %f ag %d fw %u fh %u",
              input->common.sof, input->common.rolling_shutter_skew,
              input->common.integration_time, input->common.analog_gain,
              input->common.frame_width, input->common.frame_height);
    if (imu_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        auto p = imu_->dequeue();
        if (p.state == ParamState::kAllocated) {
            auto& param = p.payload;
            param->frame_id = id;
            p.unique_id = id;
            imu_->enqueue(p);
        } else if (p.state == ParamState::kProcessedSuccess ||
                   p.state == ParamState::kProcessedError) {
            auto& param = p.payload;
            if (param->data != nullptr) {
                LOGV_AEIS("IMU-%d: get data state %d id %d count %d %" PRIu64
                          "",
                          p.unique_id, p.state, param->frame_id,
                          param->data->GetCount(),
                          (param->data->GetData())[param->data->GetCount() - 1]
                              .timestamp_us);
                lib_->GetOps()->PutImuFrame(
                    engine_.get(), (mems_sensor_event_t*)param->data->GetData(),
                    param->data->GetCount());
                param->data.reset();
            }
            param->frame_id = id;
            p.unique_id = id;
            imu_->enqueue(p);
        }
    }

    if (input->nr_buf_info.nr_buf_index < 0 || input->common.frame_id < 0) {
        LOGW_AEIS("Process %d frame has invalid frame idx %d",
                  input->common.frame_id, input->nr_buf_info.nr_buf_index);
        return;
    }

    if (image_indexes_.empty() && input->nr_buf_info.buf_cnt > 0) {
        for (int i = 0; i < input->nr_buf_info.buf_cnt; i++) {
            LOGD_AEIS("Initial nr buffer indexes : %d index %u fd %u size %u",
                      i, input->nr_buf_info.idx_array[i],
                      input->nr_buf_info.fd_array[i],
                      input->nr_buf_info.nr_buf_size);
            image_indexes_.emplace(input->nr_buf_info.idx_array[i],
                                   input->nr_buf_info.nr_buf_size);
        }
    }

    if (scl_ != nullptr && (calib_->mode == EIS_MODE_IMG_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        // TODO(Cody): dequeue and enqueue scaler param
    }

    struct imageData* image = new imageData();
    if (image != nullptr) {
        auto& meta = image->meta_data;
        meta.iso_speed = input->common.analog_gain;
        meta.exp_time = input->common.integration_time;
        meta.rolling_shutter_skew =
            input->common.rolling_shutter_skew / 1000000000;
        meta.zoom_ratio = 1;
        meta.timestamp_sof_us = input->common.sof / 1000;

        image->buffer_index = (int)input->nr_buf_info.nr_buf_index;
        image->frame_index = (int)input->common.frame_id;

        dvs_images_.emplace(input->common.frame_id, image);
    }

    if (image != nullptr) {
        const char* dump_env = std::getenv("eis_dump_imu");
        int dump = 0;
        if (dump_env) {
            dump = atoi(dump_env);
        }
        if (dump > 0) {
            std::ofstream ofs("/data/img.txt", std::ios::app);
            if (ofs.is_open()) {
                ofs << image->frame_index << "," << image->buffer_index << ","
                    << image->meta_data.timestamp_sof_us << std::endl;
            }
            ofs.close();
        }
        LOGV_AEIS("Put img frame id %d idx %d ts %" PRId64 "",
                  image->frame_index, image->buffer_index,
                  image->meta_data.timestamp_sof_us);
        lib_->GetOps()->PutImageFrame(engine_.get(), image);
    }
}

void EisAlgoAdaptor::GetProcResult(RkAiqAlgoProcResAeisInt* output) {
    while (true) {
        auto* mesh = remap_->GetPendingHwResult();
        auto config = remap_->GetConfig();
        if (mesh != nullptr) {
            LOGD_AEIS("Got DVS result : id %d, idx %d, fd %d", mesh->FrameId,
                      mesh->ImageBufferIndex, mesh->Fd);
            output->update = 1;
            // output->frame_id      = mesh->FrameId >= 0 ? mesh->FrameId : 0;
            output->frame_id = mesh->FrameId;
            output->img_buf_index = mesh->ImageBufferIndex;
            output->img_size = image_indexes_.empty()
                                   ? 0
                                   : image_indexes_[mesh->ImageBufferIndex];
            if (mesh->FrameId == 1) remap_->WriteMeshToFile(mesh);
            output->fd = mesh->Fd;
            output->mesh_size = config.MeshSize;
            output->mesh_density = config.MeshDensity;
            if (mesh->Fd >= 0)
                delete mesh;
            else
                remap_->FreeUserBuffer(mesh);
            result_callback_(RK_AIQ_ALGO_TYPE_AEIS, (RkAiqAlgoResCom*)output);
        } else {
            output->update = 0;
            output->fd = -1;
            break;
        }
    }
}

void EisAlgoAdaptor::Stop() {
    if (!started_ || !valid_) {
        return;
    }

    if (imu_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        imu_.reset();
    }

    if (scl_ != nullptr && (calib_->mode == EIS_MODE_IMU_ONLY ||
                            calib_->mode == EIS_MODE_IMU_AND_IMG)) {
        scl_.reset();
    }

    if (engine_ != nullptr) {
        lib_->GetOps()->RequestStop(engine_.get());
    }
    started_ = false;
}

};  // namespace RkCam
