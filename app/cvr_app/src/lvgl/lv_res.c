/*
 * Copyright (c) 2021 Rockchip, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "lv_res.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <png.h>

typedef struct {
    int width, height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep* row_pointers;
} png_img_t;

static int read_png_file(png_img_t* p, const char* file_name)
{
    char header[8];    // 8 is the maximum size that can be checked

    /*open file and test for it being a png*/
    FILE *fp = fopen(file_name, "rb");
    if (!fp) {
        printf("PNG file %s could not be opened for reading\n", file_name);
        return -1;
    }

    size_t rcnt = fread(header, 1, 8, fp);
    if (rcnt != 8 || png_sig_cmp((png_const_bytep)header, 0, 8)) {
        printf("%s is not recognized as a PNG file\n", file_name);
        fclose(fp);
        return -1;
    }

    /* Initialize stuff */
    p->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!p->png_ptr) {
        printf("png_create_read_struct failed \n");
        fclose(fp);
        return -1;
    }

    p->info_ptr = png_create_info_struct(p->png_ptr);
    if (!p->info_ptr) {
        printf("png_create_info_struct failed \n");
        fclose(fp);
        return -1;
    }
    if (setjmp(png_jmpbuf(p->png_ptr))) {
        printf("Error during init_io\n");
        fclose(fp);
        return -1;
    }
    png_init_io(p->png_ptr, fp);
    png_set_sig_bytes(p->png_ptr, 8);

    png_read_info(p->png_ptr, p->info_ptr);

    p->width = png_get_image_width(p->png_ptr, p->info_ptr);
    p->height = png_get_image_height(p->png_ptr, p->info_ptr);
    p->color_type = png_get_color_type(p->png_ptr, p->info_ptr);
    p->bit_depth = png_get_bit_depth(p->png_ptr, p->info_ptr);

    p->number_of_passes = png_set_interlace_handling(p->png_ptr);
    png_read_update_info(p->png_ptr, p->info_ptr);

    /* Read file */
    if (setjmp(png_jmpbuf(p->png_ptr))) {
        printf("Error during read_image\n");
        fclose(fp);
        return -1;
    }
    p->row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * p->height);

    for (int y = 0; y < p->height; y++) {
    	int row_len = png_get_rowbytes(p->png_ptr,p->info_ptr);
    	//printf("%s %d row_len = %d\n", __func__, y, row_len);
        p->row_pointers[y] = (png_byte*) malloc(row_len);
    }

    png_read_image(p->png_ptr, p->row_pointers);

    fclose(fp);
    return 0;
}

static void png_release(png_img_t* p)
{
    for (int y = 0; y < p->height; y++)
        free(p->row_pointers[y]);

    free(p->row_pointers);
}

lv_img_dsc_t* lv_res_create(char* res_path)
{
    png_img_t p;
    lv_img_dsc_t* img_res = NULL;
    int res = read_png_file(&p, res_path);
    if ((p.color_type == 6) && (p.bit_depth == 8)) {
        int i, j;
        img_res = malloc(sizeof(lv_img_dsc_t));
        memset(img_res, 0, sizeof(lv_img_dsc_t));
        img_res->header.always_zero = 0;
        img_res->header.w = p.width;
        img_res->header.h = p.height;
        img_res->data_size = p.width * p.height * LV_IMG_PX_SIZE_ALPHA_BYTE;
        img_res->header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
        img_res->data = malloc(img_res->data_size);
        for (i = 0; i < p.height; i++) {
            char* img_data = (char*)&img_res->data[p.width * i * LV_IMG_PX_SIZE_ALPHA_BYTE];
            char* row_data = p.row_pointers[i];
            //memcpy(img_data, p.row_pointers[i], p.width * LV_IMG_PX_SIZE_ALPHA_BYTE);
            for (j = 0; j < p.width * 4; j += 4) {
                *img_data++ = row_data[j + 2];
                *img_data++ = row_data[j + 1];
                *img_data++ = row_data[j + 0];
                *img_data++ = row_data[j + 3];
            }
        }
    }
    //printf("%s %s, res = %d, %d, %d, %d, %d\n", __func__, res_path, res, p.width, p.height, p.color_type, p.bit_depth);
    png_release(&p);
    return img_res;
}

int lv_res_destroy(lv_img_dsc_t** res)
{
    if (*res) {
        lv_img_cache_invalidate_src(*res);
        if ((*res)->data) {
            free((void*)(*res)->data);
            (*res)->data = NULL;
        }
        free((void *)(*res));
        *res = NULL;
    }

    return 0;
}
