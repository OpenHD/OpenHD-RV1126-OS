/*
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

#ifndef _RK_AIQ_TYPES_PRIV_H_
#define _RK_AIQ_TYPES_PRIV_H_

#include <string>
#include <vector>
#include "rk_aiq_types.h"
#include "rk-camera-module.h"

#define RKAIQ_ISP_AEC_ID           (1 << 0)
#define RKAIQ_ISP_HIST_ID          (1 << 1)
#define RKAIQ_ISP_AWB_ID           (1 << 2)
#define RKAIQ_ISP_AWB_GAIN_ID      (1 << 3)
#define RKAIQ_ISP_AHDRMGE_ID       (1 << 4)
#define RKAIQ_ISP_AHDRTMO_ID       (1 << 5)
#define RKAIQ_ISP_AF_ID            (1 << 6)
#define RKAIQ_ISP_BLC_ID           (1 << 7)
#define RKAIQ_ISP_DPCC_ID          (1 << 8)
#define RKAIQ_ISP_RAWNR_ID         (1 << 9)
#define RKAIQ_ISP_GIC_ID           (1 << 10)
#define RKAIQ_ISP_LUT3D_ID         (1 << 11)
#define RKAIQ_ISP_DEHAZE_ID        (1 << 12)
#define RKAIQ_ISP_CCM_ID           (1 << 13)
#define RKAIQ_ISP_GAMMA_ID         (1 << 14)
#define RKAIQ_ISP_LSC_ID           (1 << 15)
#define RKAIQ_ISP_GAIN_ID          (1 << 16)
#define RKAIQ_ISP_DEBAYER_ID       (1 << 17)
#define RKAIQ_ISP_IE_ID            (1 << 18)
#define RKAIQ_ISP_CP_ID            (1 << 19)
#define RKAIQ_ISP_LDCH_ID          (1 << 20)
#define RKAIQ_ISP_DEGAMMA_ID       (1 << 21)
#define RKAIQ_ISP_WDR_ID            (1 << 22)

typedef struct {
    uint32_t update_mask;
    uint32_t module_enable_mask;
    sint32_t frame_id;
    rk_aiq_isp_aec_meas_t   aec_meas;
    rk_aiq_isp_hist_meas_t  hist_meas;
    bool awb_cfg_update;
    rk_aiq_awb_stat_cfg_v200_t   awb_cfg_v200;
    rk_aiq_awb_stat_cfg_v201_t   awb_cfg_v201;
    bool awb_gain_update;
    rk_aiq_wb_gain_t       awb_gain;
    rk_aiq_isp_af_meas_t    af_meas;
    bool af_cfg_update;
    rk_aiq_isp_dpcc_t       dpcc;
    RkAiqAhdrProcResult_t   ahdr_proc_res;//porc data for hw/simulator
    rk_aiq_isp_drc_t        drc;
    rk_aiq_ccm_cfg_t        ccm;
    rk_aiq_lsc_cfg_t        lsc;
    //rk_aiq_isp_goc_t        goc;

    //anr result
    rkaiq_anr_procRes_t     rkaiq_anr_proc_res;
    rkaiq_asharp_procRes_t  rkaiq_asharp_proc_res;
    rk_aiq_isp_ie_t         ie;
#ifdef RK_SIMULATOR_HW
    rk_aiq_isp_blc_t        blc;
    rk_aiq_isp_rawnr_t      rawnr;
    rk_aiq_isp_gic_t        gic;
    rk_aiq_isp_demosaic_t   demosaic;
    rk_aiq_isp_ldch_t       ldch;
    //rk_aiq_isp_fec_t        fec;
    rk_aiq_lut3d_cfg_t      lut3d;
    //rk_aiq_isp_dehaze_t     dehaze;
    rk_aiq_dehaze_cfg_t     adhaz_config;
    AgammaProcRes_t         agamma;
    AdegammaProcRes_t       adegamma;
    rk_aiq_isp_wdr_t        wdr;
    rk_aiq_isp_csm_t        csm;
    rk_aiq_isp_cgc_t        cgc;
    rk_aiq_isp_conv422_t    conv22;
    rk_aiq_isp_yuvconv_t    yuvconv;
    rk_aiq_isp_gain_t       gain_config;
    rk_aiq_acp_params_t     cp;
    rk_aiq_isp_ie_t         ie;
#endif
} rk_aiq_isp_meas_params_t;

typedef struct {
    uint32_t update_mask;
    uint32_t module_enable_mask;
    sint32_t frame_id;
    rk_aiq_isp_rawnr_t      rawnr;
    rk_aiq_isp_gain_t       gain_config;
    ANRMotionParam_t        motion_param;
    rk_aiq_isp_blc_t        blc;
    rk_aiq_isp_gic_t        gic;
    rk_aiq_isp_demosaic_t   demosaic;
    rk_aiq_isp_ldch_t       ldch;
    rk_aiq_lut3d_cfg_t      lut3d;
    rk_aiq_dehaze_cfg_t     adhaz_config;
    AgammaProcRes_t         agamma;
    AdegammaProcRes_t       adegamma;
    rk_aiq_isp_wdr_t        wdr;
    rk_aiq_isp_csm_t        csm;
    rk_aiq_isp_cgc_t        cgc;
    rk_aiq_isp_conv422_t    conv22;
    rk_aiq_isp_yuvconv_t    yuvconv;
    rk_aiq_acp_params_t     cp;
} rk_aiq_isp_other_params_t;

#define RKAIQ_ISPP_TNR_ID           (1 << 0)
#define RKAIQ_ISPP_NR_ID            (1 << 1)
#define RKAIQ_ISPP_SHARP_ID         (1 << 2)
#define RKAIQ_ISPP_FEC_ID           (1 << 3)
#define RKAIQ_ISPP_ORB_ID           (1 << 4)

typedef struct {
    uint32_t update_mask;
    sint32_t frame_id;
#ifdef RK_SIMULATOR_HW
    rk_aiq_isp_tnr_t        tnr;
    rk_aiq_isp_ynr_t        ynr;
    rk_aiq_isp_uvnr_t       uvnr;
    rk_aiq_isp_sharpen_t    sharpen;
    rk_aiq_isp_edgeflt_t    edgeflt;
    rk_aiq_isp_fec_t        fec;
#endif
    rk_aiq_isp_orb_t        orb;
} rk_aiq_ispp_meas_params_t;

typedef struct {
    uint32_t update_mask;
    uint32_t frame_id;
    // rk_aiq_isp_tnr_t        tnr;
    rk_aiq_isp_ynr_t        ynr;
    rk_aiq_isp_uvnr_t       uvnr;
    rk_aiq_isp_sharpen_t    sharpen;
    rk_aiq_isp_edgeflt_t    edgeflt;
    // rk_aiq_isp_fec_t        fec;
} rk_aiq_ispp_other_params_t;

typedef struct {
    uint32_t update_mask;
    sint32_t frame_id;
    rk_aiq_isp_tnr_t        tnr;
} rk_aiq_tnr_params_t;

typedef struct {
    uint32_t update_mask;
    sint32_t frame_id;
    rk_aiq_isp_fec_t        fec;
} rk_aiq_fec_params_t;

typedef enum rk_aiq_drv_share_mem_type_e {
    MEM_TYPE_LDCH,
    MEM_TYPE_FEC,
} rk_aiq_drv_share_mem_type_t;

typedef void (*alloc_mem_t)(void* ops_ctx, void* cfg, void** mem_ctx);
typedef void (*release_mem_t)(void* mem_ctx);
typedef void* (*get_free_item_t)(void* mem_ctx);
typedef struct isp_drv_share_mem_ops_s {
    alloc_mem_t alloc_mem;
    release_mem_t release_mem;
    get_free_item_t get_free_item;
} isp_drv_share_mem_ops_t;

typedef struct rk_aiq_ldch_share_mem_info_s {
    int size;
    void *map_addr;
    void *addr;
    int fd;
    char *state;
} rk_aiq_ldch_share_mem_info_t;

typedef struct rk_aiq_fec_share_mem_info_s {
    int size;
    int fd;
    void *map_addr;
    unsigned char *meshxf;
    unsigned char *meshyf;
    unsigned short *meshxi;
    unsigned short *meshyi;
    char *state;
} rk_aiq_fec_share_mem_info_t;

typedef struct rk_aiq_share_mem_alloc_param_s {
    int width;
    int height;
    char reserved[8];
} rk_aiq_share_mem_alloc_param_t;

typedef struct rk_aiq_share_mem_config_s {
    rk_aiq_drv_share_mem_type_t mem_type;
    rk_aiq_share_mem_alloc_param_t alloc_param;
} rk_aiq_share_mem_config_t;

struct rk_aiq_vbuf_info {
    uint32_t frame_id;
    uint32_t timestamp;
    float    exp_time;
    float    exp_gain;
    uint32_t exp_time_reg;
    uint32_t exp_gain_reg;
    uint32_t data_fd;
    uint8_t *data_addr;
    uint32_t data_length;
    rk_aiq_rawbuf_type_t buf_type;
    bool valid;
};

struct rk_aiq_vbuf {
    void *base_addr;
    uint32_t frame_width;
    uint32_t frame_height;
    struct rk_aiq_vbuf_info buf_info[3];/*index: 0-short,1-medium,2-long*/
};

typedef struct rk_aiq_tx_info_s {
    uint32_t            width;
    uint32_t            height;
    uint8_t             bpp;
    uint8_t             bayer_fmt;
    uint32_t            stridePerLine;
    uint32_t            bytesPerLine;
    bool                storage_type;
    uint32_t            id;
    //get from AE
    bool                IsAeConverged;
    bool                envChange;
    void                *data_addr;
    RKAiqAecExpInfo_t   *aecExpInfo;
} rk_aiq_tx_info_t;

typedef struct rk_aiq_nr_buf_info_s {
    uint32_t    width;
    uint32_t    height;
	uint32_t    nr_buf_size;
	uint32_t    nr_buf_index;

    int32_t     fd_array[16];
    int32_t     idx_array[16];
    int32_t     buf_cnt;
} rk_aiq_nr_buf_info_t;

typedef struct rk_aiq_tnr_buf_info_s {
    uint32_t    frame_id;

    uint32_t    width;
    uint32_t    height;

    uint32_t    kg_buf_fd;
    uint32_t    kg_buf_size;

    uint32_t    wr_buf_index;
    uint32_t    wr_buf_fd;
    uint32_t    wr_buf_size;
} rk_aiq_tnr_buf_info_t;

typedef struct rk_aiq_amd_result_s {
    uint32_t    frame_id;

    uint32_t    wr_buf_index;
    uint32_t    wr_buf_size;
} rk_aiq_amd_result_t;

typedef struct rk_aiq_eis_input_info_s {
    uint32_t                id;
    uint64_t                timestamp;

    rk_aiq_nr_buf_info_t    nr_buf_info;

    RKAiqAecExpInfo_t       exp_Info;
    uint32_t                sensor_readout_linecnt_perline;
} rk_aiq_eis_input_info;

#define MAX_MEDIA_INDEX               16
#define DEV_PATH_LEN                  64
#define SENSOR_ATTACHED_FLASH_MAX_NUM 2
#define MAX_CAM_NUM                   4

#define ISP_TX_BUF_NUM 4
#define VICAP_TX_BUF_NUM 4

typedef struct {
    int  model_idx: 3;
    int  linked_sensor: 3;
    bool linked_dvp;
    bool valid;
    char media_dev_path[DEV_PATH_LEN];
    char isp_dev_path[DEV_PATH_LEN];
    char csi_dev_path[DEV_PATH_LEN];
    char mpfbc_dev_path[DEV_PATH_LEN];
    char main_path[DEV_PATH_LEN];
    char self_path[DEV_PATH_LEN];
    char rawwr0_path[DEV_PATH_LEN];
    char rawwr1_path[DEV_PATH_LEN];
    char rawwr2_path[DEV_PATH_LEN];
    char rawwr3_path[DEV_PATH_LEN];
    char dma_path[DEV_PATH_LEN];
    char rawrd0_m_path[DEV_PATH_LEN];
    char rawrd1_l_path[DEV_PATH_LEN];
    char rawrd2_s_path[DEV_PATH_LEN];
    char stats_path[DEV_PATH_LEN];
    char input_params_path[DEV_PATH_LEN];
    char mipi_luma_path[DEV_PATH_LEN];
    char mipi_dphy_rx_path[DEV_PATH_LEN];
    char linked_vicap[DEV_PATH_LEN];
} rk_aiq_isp_t;

typedef struct {
    int  model_idx;
    char media_dev_path[DEV_PATH_LEN];
    char pp_input_image_path[DEV_PATH_LEN];
    char pp_m_bypass_path[DEV_PATH_LEN];
    char pp_scale0_path[DEV_PATH_LEN];
    char pp_scale1_path[DEV_PATH_LEN];
    char pp_scale2_path[DEV_PATH_LEN];
    char pp_input_params_path[DEV_PATH_LEN];
    char pp_stats_path[DEV_PATH_LEN];
    char pp_dev_path[DEV_PATH_LEN];

    char pp_tnr_params_path[DEV_PATH_LEN];
    char pp_tnr_stats_path[DEV_PATH_LEN];
    char pp_nr_params_path[DEV_PATH_LEN];
    char pp_nr_stats_path[DEV_PATH_LEN];
    char pp_fec_params_path[DEV_PATH_LEN];

} rk_aiq_ispp_t;

typedef struct {
    int isp_ver;
    int awb_ver;
    int aec_ver;
    int afc_ver;
    int ahdr_ver;
    int blc_ver;
    int dpcc_ver;
    int anr_ver;
    int debayer_ver;
    int lsc_ver;
    int ccm_ver;
    int gamma_ver;
    int gic_ver;
    int sharp_ver;
    int dehaze_ver;
} rk_aiq_hw_ver_t;

typedef struct {
    rk_aiq_isp_t isp_info[MAX_CAM_NUM];
    rk_aiq_ispp_t ispp_info[MAX_CAM_NUM];
    rk_aiq_hw_ver_t hw_ver_info;
} rk_aiq_isp_hw_info_t;

typedef struct {
    int  model_idx: 3;
    char media_dev_path[DEV_PATH_LEN];
    char mipi_id0[DEV_PATH_LEN];
    char mipi_id1[DEV_PATH_LEN];
    char mipi_id2[DEV_PATH_LEN];
    char mipi_id3[DEV_PATH_LEN];
    char dvp_id0[DEV_PATH_LEN];
    char dvp_id1[DEV_PATH_LEN];
    char dvp_id2[DEV_PATH_LEN];
    char dvp_id3[DEV_PATH_LEN];
    char mipi_dphy_rx_path[DEV_PATH_LEN];
    char mipi_csi2_sd_path[DEV_PATH_LEN];
    char lvds_sd_path[DEV_PATH_LEN];
    char mipi_luma_path[DEV_PATH_LEN];
    char stream_cif_path[DEV_PATH_LEN];
    char dvp_sof_sd_path[DEV_PATH_LEN];
    char model_str[DEV_PATH_LEN];
} rk_aiq_cif_info_t;

typedef struct {
    rk_aiq_cif_info_t cif_info[MAX_CAM_NUM];
    rk_aiq_hw_ver_t hw_ver_info;
} rk_aiq_cif_hw_info_t;

typedef struct {
    /* sensor entity name format:
     * m01_b_ov13850 1-0010, where 'm01' means module index number
     * 'b' meansback or front, 'ov13850' is real sensor name
     * '1-0010' means the i2c bus and sensor i2c slave address
     */
    std::string sensor_name;
    std::string device_name;
    std::string len_name;
    std::string parent_media_dev;
    int media_node_index;
    int csi_port;
    std::string module_lens_dev_name; // matched using mPhyModuleIndex
    std::string module_ircut_dev_name;
    int flash_num;
    std::string module_flash_dev_name[SENSOR_ATTACHED_FLASH_MAX_NUM]; // matched using mPhyModuleIndex
    bool fl_strth_adj_sup;
    int flash_ir_num;
    std::string module_flash_ir_dev_name[SENSOR_ATTACHED_FLASH_MAX_NUM];
    bool fl_ir_strth_adj_sup;
    std::string module_real_sensor_name; //parsed frome sensor entity name
    std::string module_index_str; // parsed from sensor entity name
    char phy_module_orient; // parsed from sensor entity name
    std::vector<rk_frame_fmt_t>  frame_size;
    rk_aiq_isp_t *isp_info;
    rk_aiq_cif_info_t *cif_info;
    rk_aiq_ispp_t *ispp_info;
    bool linked_to_isp;
    bool dvp_itf;
    struct rkmodule_inf mod_info;
} rk_sensor_full_info_t;

#endif
