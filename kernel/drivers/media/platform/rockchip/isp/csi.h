/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd. */

#ifndef _RKISP_CSI_H
#define _RKISP_CSI_H

#include <linux/kfifo.h>

#define CSI_DEV_NAME DRIVER_NAME "-csi-subdev"

#define HDR_MAX_DUMMY_BUF 3
/* define max dmatx to use for hdr */
#define HDR_DMA_MAX 3
#define HDR_DMA0 0
#define HDR_DMA1 1
#define HDR_DMA2 2

#define IS_HDR_RDBK(x) ({ \
	typeof(x) __x = (x); \
	(__x == HDR_RDBK_FRAME1 || \
	 __x == HDR_RDBK_FRAME2 || \
	 __x == HDR_RDBK_FRAME3); \
})

enum {
	T_CMD_QUEUE,
	T_CMD_DEQUEUE,
	T_CMD_LEN,
	T_CMD_END,
};

enum hdr_op_mode {
	HDR_NORMAL = 0,
	HDR_RDBK_FRAME1 = 4,
	HDR_RDBK_FRAME2 = 5,
	HDR_RDBK_FRAME3 = 6,
	HDR_FRAMEX2_DDR = 8,
	HDR_LINEX2_DDR = 9,
	HDR_LINEX2_NO_DDR = 10,
	HDR_FRAMEX3_DDR = 12,
	HDR_LINEX3_DDR = 13,
};

enum rkisp_csi_pad {
	CSI_SINK = 0,
	CSI_SRC_CH0,
	CSI_SRC_CH1,
	CSI_SRC_CH2,
	CSI_SRC_CH3,
	CSI_SRC_CH4,
	CSI_PAD_MAX
};

enum rkisp_csi_filt {
	CSI_F_VS,
	CSI_F_RD0,
	CSI_F_RD1,
	CSI_F_RD2,
	CSI_F_MAX
};

struct sink_info {
	u8 index;
	u8 linked;
};

/*
 * struct rkisp_csi_device
 * sink: csi link enable flags
 * rdbk_kfifo: read back event fifo
 * rdbk_lock: lock for read back event
 * mipi_di: Data Identifier (vc[7:6],dt[5:0])
 * filt_state: multiframe read back mode to filt irq event
 * tx_first: flags for dmatx first Y_STATE irq
 */
struct rkisp_csi_device {
	struct rkisp_device *ispdev;
	struct v4l2_subdev sd;
	struct media_pad pads[CSI_PAD_MAX];
	struct sink_info sink[CSI_PAD_MAX - 1];
	struct kfifo rdbk_kfifo;
	spinlock_t rdbk_lock;
	int max_pad;
	int frame_cnt;
	int frame_cnt_x1;
	int frame_cnt_x2;
	int frame_cnt_x3;
	u32 err_cnt;
	u32 irq_cnt;
	u32 rd_mode;
	u8 mipi_di[CSI_PAD_MAX - 1];
	u8 filt_state[CSI_F_MAX];
	u8 tx_first[HDR_DMA_MAX];
};

int rkisp_register_csi_subdev(struct rkisp_device *dev,
			      struct v4l2_device *v4l2_dev);
void rkisp_unregister_csi_subdev(struct rkisp_device *dev);

int rkisp_csi_config_patch(struct rkisp_device *dev);
void rkisp_trigger_read_back(struct rkisp_csi_device *csi, u8 dma2frm, u32 mode, bool is_try);
int rkisp_csi_trigger_event(struct rkisp_device *dev, u32 cmd, void *arg);
void rkisp_csi_sof(struct rkisp_device *dev, u8 id);
#endif
