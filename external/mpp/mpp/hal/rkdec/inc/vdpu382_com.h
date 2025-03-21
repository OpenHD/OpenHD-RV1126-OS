/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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
 */

#ifndef __VDPU382_COM_H__
#define __VDPU382_COM_H__

#include "mpp_device.h"
#include "mpp_buf_slot.h"
#include "vdpu382.h"

#define OFFSET_COMMON_REGS          (8 * sizeof(RK_U32))
#define OFFSET_CODEC_PARAMS_REGS    (64 * sizeof(RK_U32))
#define OFFSET_COMMON_ADDR_REGS     (128 * sizeof(RK_U32))
#define OFFSET_CODEC_ADDR_REGS      (160 * sizeof(RK_U32))
#define OFFSET_POC_HIGHBIT_REGS     (200 * sizeof(RK_U32))
#define OFFSET_INTERRUPT_REGS       (224 * sizeof(RK_U32))
#define OFFSET_STATISTIC_REGS       (256 * sizeof(RK_U32))

#define RCB_ALLINE_SIZE             (64)

#define MPP_RCB_BYTES(bits)  MPP_ALIGN((bits + 7) / 8, RCB_ALLINE_SIZE)

typedef enum Vdpu382_RCB_TYPE_E {
    RCB_DBLK_ROW,
    RCB_INTRA_ROW,
    RCB_TRANSD_ROW,
    RCB_STRMD_ROW,
    RCB_INTER_ROW,
    RCB_SAO_ROW,
    RCB_FBC_ROW,
    RCB_TRANSD_COL,
    RCB_INTER_COL,
    RCB_FILT_COL,

    RCB_BUF_COUNT,
} Vdpu382RcbType_e;

/* base: OFFSET_COMMON_REGS */
typedef struct Vdpu382RegCommon_t {
    struct SWREG8_IN_OUT {
        RK_U32      in_endian               : 1;
        RK_U32      in_swap32_e             : 1;
        RK_U32      in_swap64_e             : 1;
        RK_U32      str_endian              : 1;
        RK_U32      str_swap32_e            : 1;
        RK_U32      str_swap64_e            : 1;
        RK_U32      out_endian              : 1;
        RK_U32      out_swap32_e            : 1;
        RK_U32      out_cbcr_swap           : 1;
        RK_U32      out_swap64_e            : 1;
        RK_U32      reserve                 : 22;
    } reg008;

    struct SWREG9_DEC_MODE {
        RK_U32      dec_mode                : 10;
        RK_U32      reserve                 : 22;
    } reg009;

    struct SWREG10_DEC_E {
        RK_U32      dec_e                   : 1;
        RK_U32      reserve                 : 31;
    } reg010;

    struct SWREG11_IMPORTANT_EN {
        RK_U32      reserver                : 1;
        RK_U32      dec_clkgate_e           : 1;
        RK_U32      reserve1                : 2;

        RK_U32      dec_irq_dis             : 1;
        RK_U32      dec_line_irq_dis        : 1; //change to reg205[9]
        RK_U32      buf_empty_en            : 1;
        RK_U32      reserve2                : 1;

        RK_U32      dec_line_irq_en         : 1;
        RK_U32      reserve3                : 1;
        RK_U32      dec_e_rewrite_valid     : 1;
        RK_U32      reserve4                : 9;

        RK_U32      softrst_en_p            : 1;
        RK_U32      reserve5                : 1; //change to reg205[0]
        RK_U32      err_head_fill_e         : 1;
        RK_U32      err_colmv_fill_e        : 1;
        RK_U32      pix_range_detection_e   : 1;
        RK_U32      reserve6                : 3;
        RK_U32      wlast_match_fail        : 1;
        RK_U32      mmu_wlast_match_fail    : 1;
        RK_U32      reserve7                : 2;
    } reg011;

    struct SWREG12_SENCODARY_EN {
        RK_U32      wr_ddr_align_en         : 1;
        RK_U32      colmv_compress_en       : 1;
        RK_U32      fbc_e                   : 1;
        RK_U32      tile_e                  : 1;

        RK_U32      reserve1                : 1;
        RK_U32      error_info_en           : 1;
        RK_U32      info_collect_en         : 1;
        RK_U32      reserve2                : 1; //change to reg205[4]

        RK_U32      scanlist_addr_valid_en  : 1;
        RK_U32      scale_down_en           : 1;
        RK_U32      reserve3                : 22;
    } reg012;

    struct SWREG13_EN_MODE_SET {
        RK_U32      reserve0                    : 1;
        RK_U32      req_timeout_rst_sel         : 1;
        RK_U32      reserve1                    : 1;
        RK_U32      dec_commonirq_mode          : 1;
        RK_U32      reserve2                    : 2;
        RK_U32      stmerror_waitdecfifo_empty  : 1;
        RK_U32      reserve3                    : 1;
        RK_U32      strmd_zero_rm_en            : 1;
        RK_U32      reserve4                    : 3;
        RK_U32      allow_not_wr_unref_bframe   : 1;
        RK_U32      fbc_output_wr_disable       : 1;

        RK_U32      reserve5            : 4;
        RK_U32      h26x_error_mode     : 1;
        RK_U32      reserve6            : 5;
        RK_U32      cur_pic_is_idr      : 1;
        RK_U32      reserve8            : 6; //change to reg205[5]
        RK_U32      filter_outbuf_mode  : 1;

        /* develop branch */
        // RK_U32      reserve5                    : 2;
        // RK_U32      h26x_error_mode             : 1;
        // RK_U32      reserve6                    : 2;
        // RK_U32      ycacherd_prior              : 1;
        // RK_U32      reserve7                    : 2;
        // RK_U32      cur_pic_is_idr              : 1;
        // RK_U32      reserve8                    : 1;
        // RK_U32      right_auto_rst_disable      : 1;
        // RK_U32      frame_end_err_rst_flag      : 1;
        // RK_U32      rd_prior_mode               : 1;
        // RK_U32      rd_ctrl_prior_mode          : 1;
        // RK_U32      reserved9                   : 1;
        // RK_U32      filter_outbuf_mode          : 1;
    } reg013;

    struct SWREG14_FBC_PARAM_SET {
        RK_U32      fbc_force_uncompress    : 1;

        RK_U32      reserve0                : 2;
        RK_U32      allow_16x8_cp_flag      : 1;
        RK_U32      reserve1                : 2;

        RK_U32      fbc_h264_exten_4or8_flag: 1;
        RK_U32      reserve2                : 25;
    } reg014;

    struct SWREG15_STREAM_PARAM_SET {
        RK_U32      rlc_mode_direct_write   : 1;
        RK_U32      rlc_mode                : 1;
        RK_U32      strmd_ofifo_perf_opt_en : 1;
        RK_U32      reserve0                : 2;

        RK_U32      strm_start_bit          : 7;
        RK_U32      reserve1                : 20;
    } reg015;

    RK_U32  reg016_str_len;

    struct SWREG17_SLICE_NUMBER {
        RK_U32      slice_num           : 25;
        RK_U32      reserve             : 7;
    } reg017;

    struct SWREG18_Y_HOR_STRIDE {
        RK_U32      y_hor_virstride     : 16;
        RK_U32      reserve             : 16;
    } reg018;

    struct SWREG19_UV_HOR_STRIDE {
        RK_U32      uv_hor_virstride    : 16;
        RK_U32      reserve             : 16;
    } reg019;

    union {
        struct SWREG20_Y_STRIDE {
            RK_U32      y_virstride         : 28;
            RK_U32      reserve             : 4;
        } reg020_y_virstride;

        struct SWREG20_FBC_PAYLOAD_OFFSET {
            RK_U32      reserve             : 4;
            RK_U32      payload_st_offset   : 28;
        } reg020_fbc_payload_off;
    };


    struct SWREG21_ERROR_CTRL_SET {
        RK_U32 inter_error_prc_mode          : 1;
        RK_U32 error_intra_mode              : 1;
        RK_U32 error_deb_en                  : 1;
        RK_U32 picidx_replace                : 5;
        RK_U32 error_spread_e                : 1;
        RK_U32                               : 3;
        RK_U32 error_inter_pred_cross_slice  : 1;
        RK_U32 reserve0                      : 11;

        RK_U32 roi_error_ctu_cal_en          : 1;
        RK_U32 reserve1                      : 7;
    } reg021;

    struct SWREG22_ERR_ROI_CTU_OFFSET_START {
        RK_U32      roi_x_ctu_offset_st : 12;
        RK_U32      reserve0            : 4;
        RK_U32      roi_y_ctu_offset_st : 12;
        RK_U32      reserve1            : 4;
    } reg022;

    struct SWREG23_ERR_ROI_CTU_OFFSET_END {
        RK_U32      roi_x_ctu_offset_end    : 12;
        RK_U32      reserve0                : 4;
        RK_U32      roi_y_ctu_offset_end    : 12;
        RK_U32      reserve1                : 4;
    } reg023;

    struct SWREG24_CABAC_ERROR_EN_LOWBITS {
        RK_U32      cabac_err_en_lowbits    : 32;
    } reg024;

    struct SWREG25_CABAC_ERROR_EN_HIGHBITS {
        RK_U32      cabac_err_en_highbits   : 30;
        RK_U32      reserve                 : 2;
    } reg025;

    struct SWREG26_BLOCK_GATING_EN {
        RK_U32      swreg_block_gating_e    : 20;
        RK_U32      reserve                 : 11;
        RK_U32      reg_cfg_gating_en       : 1;
    } reg026;

    /* NOTE: reg027 ~ reg032 are added in vdpu38x at rk3588 */
    struct SW027_CORE_SAFE_PIXELS {
        // colmv and recon report coord x safe pixels
        RK_U32 core_safe_x_pixels           : 16;
        // colmv and recon report coord y safe pixels
        RK_U32 core_safe_y_pixels           : 16;
    } reg027;

    struct SWREG28_MULTIPLY_CORE_CTRL {
        RK_U32      swreg_vp9_wr_prob_idx   : 3;
        RK_U32      reserve0                : 1;
        RK_U32      swreg_vp9_rd_prob_idx   : 3;
        RK_U32      reserve1                : 1;

        RK_U32      swreg_ref_req_advance_flag  : 1;
        RK_U32      sw_colmv_req_advance_flag   : 1;
        RK_U32      sw_poc_only_highbit_flag    : 1;
        RK_U32      sw_poc_arb_flag             : 1;

        RK_U32      reserve2                : 4;
        RK_U32      sw_film_idx             : 10;
        RK_U32      reserve3                : 2;
        RK_U32      sw_pu_req_mismatch_dis  : 1;
        RK_U32      sw_colmv_req_mismatch_dis   : 1;
        RK_U32      reserve4                : 2;
    } reg028;

    struct SWREG29_SCALE_DOWN_CTRL {
        RK_U32      scale_down_y_wratio     : 5;
        RK_U32      reserve0                : 3;
        RK_U32      scale_down_y_hratio     : 5;
        RK_U32      reserve1                : 3;
        RK_U32      scale_down_c_wratio     : 5;
        RK_U32      reserve2                : 3;
        RK_U32      scale_down_c_hratio     : 5;
        RK_U32      reserve3                : 1;
        RK_U32      scale_down_roi_mode     : 1;
        RK_U32      scale_down_tile_mode    : 1;
    } reg029;

    struct SW032_Y_SCALE_DOWN_TILE8x8_HOR_STRIDE {
        RK_U32 y_scale_down_hor_stride      : 20;
        RK_U32                              : 12;
    } reg030;

    struct SW031_UV_SCALE_DOWN_TILE8x8_HOR_STRIDE {
        RK_U32 uv_scale_down_hor_stride     : 20;
        RK_U32                              : 12;
    } reg031;

    /* NOTE: reg027 ~ reg032 are added in vdpu38x at rk3588 */
    /* NOTE: timeout must be config in vdpu38x */
    RK_U32  reg032_timeout_threshold;
} Vdpu382RegCommon;

/* base: OFFSET_COMMON_ADDR_REGS */
typedef struct Vdpu382RegCommonAddr_t {
    /* offset 128 */
    RK_U32  reg128_rlc_base;

    RK_U32  reg129_rlcwrite_base;

    RK_U32  reg130_decout_base;

    RK_U32  reg131_colmv_cur_base;

    RK_U32  reg132_error_ref_base;

    RK_U32  reg133_rcb_intra_base;

    RK_U32  reg134_rcb_transd_row_base;

    RK_U32  reg135_rcb_transd_col_base;

    RK_U32  reg136_rcb_streamd_row_base;

    RK_U32  reg137_rcb_inter_row_base;

    RK_U32  reg138_rcb_inter_col_base;

    RK_U32  reg139_rcb_dblk_base;

    RK_U32  reg140_rcb_sao_base;

    RK_U32  reg141_rcb_fbc_base;

    RK_U32  reg142_rcb_filter_col_base;
} Vdpu382RegCommonAddr;

/* base: OFFSET_COMMON_ADDR_REGS */
typedef struct Vdpu382RegIrqStatus_t {
    struct SWREG224_STA_INT {
        RK_U32      dec_irq         : 1;
        RK_U32      dec_irq_raw     : 1;

        RK_U32      dec_rdy_sta     : 1;
        RK_U32      dec_bus_sta     : 1;
        RK_U32      dec_error_sta   : 1;
        RK_U32      dec_timeout_sta : 1;
        RK_U32      buf_empty_sta   : 1;
        RK_U32      colmv_ref_error_sta : 1;
        RK_U32      cabu_end_sta    : 1;

        RK_U32      softreset_rdy   : 1;

        RK_U32      dec_line_irq    : 1;
        RK_U32      dec_line_irq_raw : 1;
        RK_U32      ltb_pause_sta   : 1;
        RK_U32      mmureset_rdy    : 1;
        RK_U32      ltb_end_sta     : 1;

        RK_U32      reserve         : 17;
    } reg224;

    struct SWREG225_STA_SLICE_BYTE_OFFSET {
        RK_U32      strmd_slice_byte_offset : 32;
    } reg225;

    struct SWREG226_STA_CABAC_ERROR_STATUS {
        RK_U32      strmd_error_status      : 28;
        RK_U32      strmd_detect_error_flag : 3;
        RK_U32      all_frame_error_flag    : 1;
    } reg226;

    struct SWREG227_STA_COLMV_ERROR_REF_PICIDX {
        RK_U32      colmv_error_ref_picidx  : 4;
        RK_U32      reserve                 : 28;
    } reg227;

    struct SWREG228_STA_CABAC_ERROR_CTU_OFFSET {
        RK_U32 cabac_error_ctu_offset_x     : 12;
        RK_U32                              : 4;
        RK_U32 cabac_error_ctu_offset_y     : 12;
        RK_U32                              : 4;
    } reg228;

    struct SWREG229_STA_SAOWR_CTU_OFFSET {
        RK_U32      saowr_xoffset : 16;
        RK_U32      saowr_yoffset : 16;
    } reg229;

    struct SWREG230_STA_SLICE_DEC_NUM {
        RK_U32      slicedec_num : 25;
        RK_U32      reserve      : 7;
    } reg230;

    struct SWREG231_STA_FRAME_ERROR_CTU_NUM {
        RK_U32      frame_ctu_err_num : 32;
    } reg231;

    struct SWREG232_STA_ERROR_PACKET_NUM {
        RK_U32      packet_err_num  : 16;
        RK_U32      reserve         : 16;
    } reg232;

    struct SWREG233_STA_ERR_CTU_NUM_IN_RO {
        RK_U32      error_ctu_num_in_roi : 24;
        RK_U32      reserve              : 8;
    } reg233;

    struct SWREG234_BUF_EMPTY_OFFSET {
        RK_U32      coord_report_offset_x : 16;
        RK_U32      coord_report_offset_y : 16;
    } reg234;

    struct SWREG235_COORD_REPORT_OUTBUF_HEIGHT {
        RK_U32      coord_report_output_height : 16;
        RK_U32      reserve                    : 16;
    } reg235;

    RK_U32  reserve_reg236_237[2];
} Vdpu382RegIrqStatus;

typedef struct Vdpu382RegStatistic_t {
    struct SWREG256_DEBUG_PERF_LATENCY_CTRL0 {
        RK_U32      axi_perf_work_e     : 1;
        RK_U32      axi_perf_clr_e      : 1;
        RK_U32      reserve0            : 1;
        RK_U32      axi_cnt_type        : 1;
        RK_U32      rd_latency_id       : 4;
        RK_U32      rd_latency_thr      : 12;
        RK_U32      reserve1            : 12;
    } reg256;

    struct SWREG257_DEBUG_PERF_LATENCY_CTRL1 {
        RK_U32      addr_align_type     : 2;
        RK_U32      ar_cnt_id_type      : 1;
        RK_U32      aw_cnt_id_type      : 1;
        RK_U32      ar_count_id         : 4;
        RK_U32      aw_count_id         : 4;
        RK_U32      rd_band_width_mode  : 1;
        RK_U32      reserve             : 19;
    } reg257;

    struct SWREG258_DEBUG_PERF_RD_MAX_LATENCY_NUM {
        RK_U32      rd_max_latency_num  : 16;
        RK_U32      reserve             : 16;
    } reg258;

    RK_U32          reg259_rd_latency_thr_num_ch0;
    RK_U32          reg260_rd_latency_acc_sum;
    RK_U32          reg261_perf_rd_axi_total_byte;
    RK_U32          reg262_perf_wr_axi_total_byte;
    RK_U32          reg263_perf_working_cnt;

    RK_U32          reserve_reg264;

    struct SWREG265_DEBUG_PERF_SEL {
        RK_U32      perf_cnt0_sel               : 6;
        RK_U32      reserve0                    : 2;
        RK_U32      perf_cnt1_sel               : 6;
        RK_U32      reserve1                    : 2;
        RK_U32      perf_cnt2_sel               : 6;
        RK_U32      reserve2                    : 10;
    } reg265;

    RK_U32          reg266_perf_cnt0;
    RK_U32          reg267_perf_cnt1;
    RK_U32          reg268_perf_cnt2;

    RK_U32          reserve_reg269;

    struct SWREG270_DEBUG_QOS_CTRL {
        RK_U32      bus2mc_buffer_qos_level     : 8;
        RK_U32      reserve0                    : 8;
        RK_U32      axi_rd_hurry_level          : 2;
        RK_U32      reserve1                    : 2;
        RK_U32      axi_wr_qos                  : 2;
        RK_U32      reserve2                    : 2;
        RK_U32      axi_wr_hurry_level          : 2;
        RK_U32      reserve3                    : 2;
        RK_U32      axi_rd_qos                  : 2;
        RK_U32      reserve4                    : 2;
    } reg270;

    RK_U32          reg271_wr_wait_cycle_qos;

    struct SWREG272_DEBUG_INT {
        RK_U32      busidle_flag                : 1;
        RK_U32      reserved                    : 4;
        RK_U32      mmu_busidle_flag            : 1;
        RK_U32      wr_tansfer_cnt              : 8;
        RK_U32      reserved1                   : 2;
        RK_U32      Sw_streamfifo_space2full    : 7;
        RK_U32      reserved2                   : 1;
        RK_U32      mmu_wr_transer_cnt          : 8;
    } reg272;

    struct SWREG273 {
        RK_U32      bus_status_flag             : 19;
        RK_U32      reserve0                    : 12;
        RK_U32      pps_no_ref_bframe_dec_r     : 1;
    } reg273;

    RK_U16          reg274_y_min_value;
    RK_U16          reg274_y_max_value;
    RK_U16          reg275_u_min_value;
    RK_U16          reg275_u_max_value;
    RK_U16          reg276_v_min_value;
    RK_U16          reg276_v_max_value;

    struct SWREG277_ERROR_SPREAD_NUM {
        RK_U32 err_spread_cnt_sum           : 24;
        RK_U32                              : 8;
    } reg277;

    // RK_U32          reg277_err_spread_num;
    // struct SWREG278_DEC_LINE_OFFSET_Y {
    //     RK_U32      dec_line_offset_y   : 16;
    //     RK_U32      reserve             : 16;
    // } reg278;

} Vdpu382RegStatistic;

typedef struct vdpu382_rcb_info_t {
    RK_S32              reg;
    RK_S32              size;
    RK_S32              offset;
} Vdpu382RcbInfo;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 vdpu382_get_rcb_buf_size(Vdpu382RcbInfo *info, RK_S32 width, RK_S32 height);
void vdpu382_setup_rcb(Vdpu382RegCommonAddr *reg, MppDev dev, MppBuffer buf, Vdpu382RcbInfo *info);
RK_S32 vdpu382_compare_rcb_size(const void *a, const void *b);
void vdpu382_setup_statistic(Vdpu382RegCommon *com, Vdpu382RegStatistic *sta);
void vdpu382_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand);
void vdpu382_setup_down_scale(MppFrame frame, MppDev dev, Vdpu382RegCommon *com);

#ifdef  __cplusplus
}
#endif

#endif /* __VDPU382_COM_H__ */
