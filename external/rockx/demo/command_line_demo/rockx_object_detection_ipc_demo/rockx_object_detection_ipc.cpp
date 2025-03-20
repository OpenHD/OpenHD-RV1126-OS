/****************************************************************************
*
*    Copyright (c) 2017 - 2019 by Rockchip Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Rockchip Corporation. This is proprietary information owned by
*    Rockchip Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Rockchip Corporation.
*
*****************************************************************************/
#include <stdio.h>
#include <memory.h>
#include <sys/time.h>
#include <iostream>

#include "rockx.h"

int main(int argc, char** argv) {

    rockx_ret_t ret;
    rockx_handle_t ipcobj_det_handle;
    struct timeval tv1, tv2;
    const char *licence_path = nullptr;

    const char *img_path = argv[1];
    if (argc == 3) {
        licence_path = argv[2];
    }

    rockx_config_t *config = rockx_create_config();
    if (licence_path != nullptr) {
        rockx_add_config(config, ROCKX_CONFIG_LICENCE_KEY_PATH, licence_path);
    }

    ret = rockx_create(&ipcobj_det_handle, ROCKX_MODULE_OBJECT_DETECTION_IPC, config, sizeof(rockx_config_t));
    if (ret != ROCKX_RET_SUCCESS) {
        printf("init rockx module ROCKX_MODULE_OBJECT_DETECTION_IPC error %d\n", ret);
        return -1;
    }

    //attribute
    rockx_handle_t obj_attribute_handle;
    ret = rockx_create(&obj_attribute_handle, ROCKX_MODULE_OBJECT_ATTRIBUTE, config, sizeof(rockx_config_t));
    if (ret != ROCKX_RET_SUCCESS) {
        printf("init rockx module ROCKX_MODULE_OBJECT_ATTRIBUTE error %d\n", ret);
        return -1;
    }

    // read image
    rockx_image_t input_image;
    rockx_image_read(img_path, &input_image, 1);
    uint32_t img_width = input_image.width;
    uint32_t img_height = input_image.height;

    // create for store result
    rockx_object_array_t obj_array;
    memset(&obj_array, 0, sizeof(rockx_object_array_t));

    // detect
    ret = rockx_object_detect_ipc(ipcobj_det_handle, &input_image, &obj_array, nullptr);
    if (ret != ROCKX_RET_SUCCESS) {
        printf("rockx_ipcobj_detect error %d\n", ret);
        return -1;
    }

    // process result
    for (int i = 0; i < obj_array.count; i++)
     {
        int left = obj_array.object[i].box.left;
        int top = obj_array.object[i].box.top;
        int right = obj_array.object[i].box.right;
        int bottom = obj_array.object[i].box.bottom;
        float score = obj_array.object[i].score;
        int cls_idx = obj_array.object[i].cls_idx;
        printf("cls_idx=%d box=(%d %d %d %d) score=%f\n", cls_idx, left, top, right, bottom, score);

        //attribute
        left = (left >= 0) ? left : 0;
        top = (top >= 0) ? top : 0;
        right = (right >= img_width) ? (img_width - 1) : right;
        bottom = (bottom >= img_height) ? (img_height - 1) : bottom;
        obj_array.object[i].box.left = left;
        obj_array.object[i].box.top = top;
        obj_array.object[i].box.right = right;
        obj_array.object[i].box.bottom = bottom;

        rockx_color_attribute_array_t color_array;
        memset(&color_array, 0, sizeof(rockx_color_attribute_array_t));
        //gettimeofday(&tv1,NULL);
        rockx_object_attribute_color(obj_attribute_handle, &input_image, &obj_array.object[i], &color_array);
        //gettimeofday(&tv2,NULL);
        //printf("rockx_object_attribute_color time = %.fms\n",(tv2.tv_usec-tv1.tv_usec)*1.0/1000.0);

        printf("demo color_attribute: output\n");

        for(int k = 0; k < color_array.count; k++)
        {
            rockx_color_attribute_t color_attribute = color_array.color_attribute[k];
            int category = color_attribute.category;
            int color_index = color_attribute.result_idx;
            float color_score = color_attribute.attribute_score;

            //printf("category: %d, result_idx: %d, attribute_score: %.3f\n", color_attribute.category, color_attribute.result_idx, color_attribute.attribute_score);

            char category_str[64];
            char color_str[64];

            switch(category)
            {
                case COAT_COLOR:
                    strcpy(category_str, "coat"); break;
                case TROUSERS_COLOR:
                    strcpy(category_str, "trousers"); break;
                case CAR_COLOR:
                    strcpy(category_str, "car"); break;
                default:
                    printf("error category\n");
            }
            switch(color_index)
            {
                case BLACK:
                    strcpy(color_str, "black"); break;
                case WHITE:
                    strcpy(color_str, "white"); break;
                case RED:
                    strcpy(color_str, "red"); break;
                case GRAY:
                    strcpy(color_str, "gray"); break;
                case PURPLE:
                    strcpy(color_str, "purple"); break;
                case YELLOW:
                    strcpy(color_str, "yellow"); break;
                case BLUE:
                    strcpy(color_str, "blue"); break;
                case GREEN:
                    strcpy(color_str, "green"); break;
                default:
                    printf("error color\n");
            }

            printf("category: %s, color_str: %s\n", category_str, color_str);

		}
		//attribute

        // draw
        char score_str[8];
        memset(score_str, 0, 8);
        snprintf(score_str, 8, "%.3f", score);
        char cls_name[1];
        memset(cls_name, 0, 1);
        snprintf(cls_name, 1, "%d", cls_idx);

        rockx_image_draw_rect(&input_image, {left, top}, {right, bottom}, {255, 0, 0}, 3);
        rockx_image_draw_text(&input_image, score_str, {left, top-12}, {255, 0, 0}, 3);
        rockx_image_draw_text(&input_image, cls_name, {left, top+12}, {255, 0, 0}, 3);
    }

    // save image
    rockx_image_write("./out.jpg", &input_image);

    // release
    rockx_image_release(&input_image);
    rockx_destroy(ipcobj_det_handle);
    rockx_destroy(obj_attribute_handle);
}
