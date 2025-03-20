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

#include "lv_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <iostream>
#include <rkadk/rkadk_common.h>
#include <list>

typedef struct lv_msg {
    void *obj;
    int msg;
    void *wparam;
    void *lparam;
} LV_MSG;

typedef struct lv_msg_handler {
    void *obj;
    msg_handler_t handler;
} LV_MSG_HANDLER;

struct msg_manager_t {
    int quit;
    pthread_t msg_tid;
    pthread_mutex_t msg_mutex;
    pthread_cond_t msg_cond;
};

static struct msg_manager_t msg_manager;
static std::list<LV_MSG *> msg_list;
static std::list<LV_MSG_HANDLER *> handler_list;

int lv_msg_register_handler(msg_handler_t handler, void *obj) {
    if (handler == NULL) {
        printf("handler is null\n");
        return -1;
    }
    LV_MSG_HANDLER *handler_elm = NULL;
    handler_elm = (LV_MSG_HANDLER *)malloc(sizeof(LV_MSG_HANDLER));
    if (handler_elm == NULL) {
        printf("handler_elm malloc fail!\n");
        return -1;
    }

    memset(handler_elm, 0, sizeof(LV_MSG_HANDLER));
    handler_elm->handler = handler;
    handler_elm->obj = obj;

    std::list<LV_MSG_HANDLER *>::iterator handler_iter;
    for (handler_iter = handler_list.begin();
        handler_iter != handler_list.end(); ++handler_iter) {

        if ((obj && obj == (*handler_iter)->obj)
            || (!obj && handler == (*handler_iter)->handler)) {
            printf("register handler fail!\n");
            if (handler_elm) {
                free(handler_elm);
                handler_elm = NULL;
            }
            return -1;
        }
    }
    handler_list.push_back(handler_elm);
    printf("register handler sussce,hander_num:%d\n", handler_list.size());
    return 0;
}

static int lv_dispatch_message(void *obj, int msg, void *wparam, void *lparam) {
    int ret;

    pthread_mutex_lock(&msg_manager.msg_mutex);

    printf("msg = %d\n", msg);
    std::list<LV_MSG_HANDLER *>::iterator handler_iter;
    for (handler_iter = handler_list.begin();
        handler_iter != handler_list.end(); ++handler_iter) {
        if (obj) {
            if (obj == (*handler_iter)->obj && (*handler_iter)->handler) {
                ret = (*handler_iter)->handler(msg, wparam, lparam);
                if (ret) {
                    printf("Handle msg fail\n");
                    pthread_mutex_unlock(&msg_manager.msg_mutex);
                    return -1;
                } else {
                    printf("Handle msg sucess\n");
                    pthread_mutex_unlock(&msg_manager.msg_mutex);
                    return 0;
                }
            }
        } else {
            if ((*handler_iter)->handler) {
                ret = (*handler_iter)->handler(msg, wparam, lparam);
                if (!ret) {
                    printf("Obj is null and Handle msg success\n");
                    pthread_mutex_unlock(&msg_manager.msg_mutex);
                    return 0;
                }
            }
        }
    }
    printf("Check the obj is registered or msg is exit\n");
    pthread_mutex_unlock(&msg_manager.msg_mutex);
    return -1;
}

static int lv_put_message(LV_MSG *elm) {
    if (!elm) {
        printf("put elm is null\n");
        return -1;
    }

    pthread_mutex_lock(&msg_manager.msg_mutex);
    msg_list.push_back(elm);
    pthread_cond_signal(&msg_manager.msg_cond);
    pthread_mutex_unlock(&msg_manager.msg_mutex);
    return 0;
}

static LV_MSG *lv_get_message(int s32TimeoutMs) {
    LV_MSG *elm = NULL;
    struct timeval timeNow;
    struct timespec timeout;

    pthread_mutex_lock(&msg_manager.msg_mutex);
    if (msg_list.empty()) {
        gettimeofday(&timeNow, NULL);
        timeout.tv_sec = timeNow.tv_sec + s32TimeoutMs / 1000;
        timeout.tv_nsec = (timeNow.tv_usec + (s32TimeoutMs % 1000) * 1000) * 1000;
        pthread_cond_timedwait(&msg_manager.msg_cond, &msg_manager.msg_mutex, &timeout);
    }
    if (!msg_list.empty()) {
        elm = msg_list.front();
        msg_list.pop_front();
    }

    pthread_mutex_unlock(&msg_manager.msg_mutex);
    return elm;
}

int lv_post_msg(void *obj, int msg, void *wparam, void *lparam) {

    LV_MSG *elm = NULL;

    elm = (LV_MSG *)malloc(sizeof(LV_MSG));
    if (!elm) {
        RKADK_LOGE("elm malloc failed.");
        return -1;
    }

    memset(elm, 0, sizeof(LV_MSG));
    elm->obj = obj;
    elm->msg = msg;
    elm->wparam = wparam;
    elm->lparam = lparam;

    if (lv_put_message(elm)) {
        printf("lv_put_message fail!\n");
        return -1;
    }

    return 0;
}

int lv_send_msg(void *obj, int msg, void *wparam, void *lparam) {
    int ret;
    ret = lv_dispatch_message(obj, msg, wparam, lparam);
    if (ret) {
        printf("Send msg handle fail!\n");
        return -1;
    }
    return 0;
}

static void *lv_msg_recthread(void *arg) {
    int ret;

    while (msg_manager.quit == 0) {
        LV_MSG *elm = lv_get_message(50);

        if (elm) {
            ret = lv_dispatch_message(elm->obj, elm->msg, elm->wparam, elm->lparam);
            if (ret)
                printf("lv_dispatch_message fail\n");
            free(elm);
            elm = NULL;
        }
    }
    return NULL;
}

static int lv_msg_clear_list(void) {
    std::list<LV_MSG *>::iterator msg_iter;
    std::list<LV_MSG_HANDLER *>::iterator handler_iter;

    for (msg_iter = msg_list.begin(); msg_iter != msg_list.end(); ++msg_iter) {
        if (*msg_iter)
            free(*msg_iter);
    }
    msg_list.clear();
    if (!msg_list.empty()) {
        printf("clear msg_list fail!\n");
        return -1;
    }
    for (handler_iter = handler_list.begin();
        handler_iter != handler_list.end(); ++handler_iter) {
        if (*handler_iter)
            free(*handler_iter);
    }
    handler_list.clear();
    if (!handler_list.empty()) {
        printf("clear handler_list fail!\n");
        return -1;
    }
    return 0;
}

int lv_msg_init(void) {
    int ret;

    ret = lv_msg_clear_list();
    if (ret) {
        printf("lv_msg_clear_list fail\n");
        return -1;
    }

    msg_manager.quit = 0;
    pthread_mutex_init(&msg_manager.msg_mutex, NULL);
    pthread_cond_init(&msg_manager.msg_cond, NULL);
    if (pthread_create(&msg_manager.msg_tid, NULL,
        lv_msg_recthread, NULL)) {
        printf("RecMsgThread create failed!");
        return -1;
    }
    return 0;
}

int lv_msg_deinit(void) {
    int ret;

    msg_manager.quit = 1;
    if (msg_manager.msg_tid) {
        pthread_cancel(msg_manager.msg_tid);
        if (pthread_join(msg_manager.msg_tid, NULL)) {
            printf("recmsgthread join failed!");
            return -1;
        }
        msg_manager.msg_tid = 0;
    }

    ret = lv_msg_clear_list();
    if (ret) {
        printf("lv_mag_clear_list fail\n");
        return -1;
    }
    return 0;
}
