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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include <linux/netlink.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "lvgl.h"

#include "media/preview.h"
#include "media/recorder.h"
#include "media/vi_isp.h"
#include "ui/main_view.h"
#include "ui/settings.h"
#include "ui/storage.h"

#include "lvgl/lv_port_disp.h"
#include "lvgl/lv_port_indev.h"
#include "lvgl/lv_msg.h"

#define LVGL_TICK 	5

static int g_ui_rotation = 90;

static int quit = 0;
static void sigterm_handler(int sig) {
    fprintf(stderr, "signal %d\n", sig);
    quit = 1;
}

static void lvgl_init(void)
{
    lv_init();
    lv_port_disp_init(g_ui_rotation);
    lv_port_indev_init(g_ui_rotation);
    loading_font();
    loading_string_res();
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigterm_handler);
    lvgl_init();

    lv_msg_init();

    cvr_sdk_param_init();

    storage_init();

    main_view_start();

    while(!quit) {
        lv_tick_inc(LVGL_TICK);
        lv_task_handler();
        usleep(LVGL_TICK * 1000);
    }

    cvr_preview_stop(0);

    cvr_vi_deinit();

    storage_deinit();

    lv_msg_deinit();

    return 0;
}
