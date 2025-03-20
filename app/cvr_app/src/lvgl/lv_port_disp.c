/**
 * @file lv_port_disp_templ.c
 *
 */

 /*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

#include <rga/rga.h>
#include <rga/RgaApi.h>

#include "lv_port_disp.h"
#include "lvgl.h"

#include "hal/drm_display.h"

static int rot_disp = 90;
static int lcd_w;
static int lcd_h;
static char* drm_buff;
static lv_color_t *buf_1;

//static struct timeval tvCbBegin;
//static struct timeval tvCbEnd;
static int quit = 0;
static pthread_t disp_thread_pid;
static pthread_mutex_t draw_mutex;
static int draw_update = 0;

static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

static void *disp_thread(void *arg)
{
    while (!quit) {
        pthread_mutex_lock(&draw_mutex);
        if (draw_update) {
            struct drm_bo *bo = getdrmdisp();
            draw_update = 0;
            if (rot_disp == 90 || rot_disp == 270) {
                rga_info_t src;
                rga_info_t dst;
                memset(&src, 0, sizeof(rga_info_t));
                memset(&dst, 0, sizeof(rga_info_t));
                src.virAddr = drm_buff;
                src.mmuFlag = 1;
                dst.virAddr = getdrmdispbuff();
                dst.mmuFlag = 1;
                rga_set_rect(&src.rect, 0, 0, lcd_h, lcd_w, lcd_h, lcd_w, RK_FORMAT_BGRA_8888);
                src.rotation = HAL_TRANSFORM_ROT_90;
                rga_set_rect(&dst.rect, 0, 0, lcd_w, lcd_h, lcd_w, lcd_h, RK_FORMAT_BGRA_8888);

                int ret = c_RkRgaBlit(&src, &dst, NULL);
                if (ret)
                    printf("c_RkRgaBlit2 error : %s\n", strerror(errno));
            } else {
                memcpy(getdrmdispbuff(), drm_buff, lcd_w * lcd_h * 4);
            }
            pthread_mutex_unlock(&draw_mutex);
            setdrmdisp(bo);
        } else {
            pthread_mutex_unlock(&draw_mutex);
        }
        usleep(34000);
    }
    return NULL;
}

void lv_port_disp_init(int rot)
{
    rot_disp = rot;
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    buf_1 = malloc(lcd_w * lcd_h * 4);

    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, lcd_w * lcd_h);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    if (rot_disp == 0) {
        disp_drv.hor_res = lcd_w;
        disp_drv.ver_res = lcd_h;

        disp_drv.sw_rotate = 0;
        disp_drv.rotated = LV_DISP_ROT_NONE;
    } else if (rot_disp == 180) {
        disp_drv.hor_res = lcd_w;
        disp_drv.ver_res = lcd_h;

        disp_drv.sw_rotate = 1;
        disp_drv.rotated = LV_DISP_ROT_180;
    } else if (rot_disp == 90) {
        disp_drv.hor_res = lcd_h;
        disp_drv.ver_res = lcd_w;

        disp_drv.sw_rotate = 0;
        disp_drv.rotated = LV_DISP_ROT_NONE;
    } else if (rot_disp == 270) {
        disp_drv.hor_res = lcd_h;
        disp_drv.ver_res = lcd_w;

        disp_drv.sw_rotate = 1;
        disp_drv.rotated = LV_DISP_ROT_180;
    }
    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
    pthread_mutex_init(&draw_mutex, NULL);
    pthread_create(&disp_thread_pid, NULL, disp_thread, NULL);
}

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
	drm_init(32);
	getdrmresolve(&lcd_w, &lcd_h);
    //struct drm_bo *bo = getdrmdisp();
    //drm_buff = getdrmdispbuff();
    //setdrmdisp(bo);
    drm_buff = malloc(lcd_w * lcd_h * 4);
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
    int32_t x;
    int32_t y;

    //printf("%s, x1 = %d, y1 = %d, x2 = %d, y2 = %d, %d, %d\n", __func__, area->x1, area->y1, area->x2, area->y2, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
    //gettimeofday(&tvCbBegin, NULL);
    pthread_mutex_lock(&draw_mutex);
    if (rot_disp == 90 || rot_disp == 270) {
        for(y = area->y1; y <= area->y2; y++) {
            int area_w = area->x2 - area->x1 + 1;
            lv_color_t *disp = (lv_color_t*)(drm_buff + (y * lcd_h + area->x1) * 4);
            memcpy(disp, color_p, area_w * 4);
            color_p += area_w;
        }
    } else {
        for(y = area->y1; y <= area->y2; y++) {
            int area_w = area->x2 - area->x1 + 1;
            lv_color_t *disp = (lv_color_t*)(drm_buff + (y * lcd_w + area->x1) * 4);
            memcpy(disp, color_p, area_w * 4);
            color_p += area_w;
        }
    }
    draw_update = 1;
    pthread_mutex_unlock(&draw_mutex);
    //gettimeofday(&tvCbEnd, NULL);
    //double dDuration = 1000 * (tvCbEnd.tv_sec - tvCbBegin.tv_sec) + ((tvCbEnd.tv_usec - tvCbBegin.tv_usec) / 1000.0);
    //printf("%s dDuration = %f\n", __func__, dDuration);
    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}