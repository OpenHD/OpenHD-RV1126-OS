#ifndef __EIS_HEAD_H__
#define __EIS_HEAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum eis_mode_e {
    EIS_MODE_IMU_ONLY,
    EIS_MODE_IMG_ONLY,
    EIS_MODE_IMU_AND_IMG,
} eis_mode_t;

typedef struct CalibDb_Eis_s {
    uint8_t enable;
    int mode;
    char debug_xml_path[256];
    int camera_model_type;
    double camera_rate;         // image sensor output fps
    double imu_rate;            // sample rate of imu
    uint32_t image_buffer_num;  // total buffer count of driver
    uint32_t src_image_width;
    uint32_t src_image_height;
    uint32_t dst_image_width;
    uint32_t dst_image_height;
    float clip_ratio_x;
    float clip_ratio_y;

    double focal_length[2];      // focal length x and y
    double light_center[2];      // light center x and y
    double distortion_coeff[5];  // distortion coeff k1 k2 r1 r2 k3
    double xi;       // if camera model is CMAI model, xi MUST be set.  if it is
                     // pinhole model, set to 0.
    double gbias_x;  // offset of gyroscope's x direction
    double gbias_y;  // offset of gyroscope's y direction
    double gbias_z;  // offset of gyroscope's z direction
    double time_offset;    // timestamp offset between gyroscope and image's
                           // timestamp
    int sensor_axes_type;  // coordinate axis system transform type between
                           // gyroscope and camera
    char save_objpts_path[100];  // save the path of world coordinate point.
                                 // it's not used by now.
} CalibDb_Eis_t;

#ifdef __cplusplus
}
#endif

#endif
