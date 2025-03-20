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

#ifndef _LV_MSG_H_
#define _LV_MSG_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*msg_handler_t)(int, void *, void *);

int lv_msg_init(void);
int lv_msg_deinit(void);
int lv_msg_register_handler(msg_handler_t handler, void *obj);
int lv_send_msg(void *obj, int msg, void *wparam, void *lparam);
int lv_post_msg(void *obj, int msg, void *wparam, void *lparam);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif //_LV_MSG_H_
