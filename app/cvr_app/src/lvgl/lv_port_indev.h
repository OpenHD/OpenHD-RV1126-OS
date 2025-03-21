
/**
 * @file lv_port_indev_templ.h
 *
 */

 /*Copy this file as "lv_port_indev.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_INDEV_TEMPL_H_
#define LV_PORT_INDEV_TEMPL_H_

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void lv_port_indev_init(int rot);
lv_group_t *lv_port_indev_group_create(void);
void lv_port_indev_group_destroy(lv_group_t *group);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_TEMPL_H_*/

#endif /*Disable/Enable content*/
