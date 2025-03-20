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

#ifndef STORAGE_H_
#define STORAGE_H_

int storage_init(void);
int storage_deinit(void);
int storage_format(void);
int storage_get_status(void);
void storage_nosdcard_dialog_create(void);
void storage_nosdcard_dialog_destroy(void);
void storage_noformat_dialog_destroy(void);
void storage_noformat_dialog_create(void);
void storage_scansdcard_dialog_destroy(void);
void storage_scansdcard_dialog_create(void);
void storage_status_cb_reg(void (*cb)(int));
void storage_status_cb_unreg(void (*cb)(int));

#endif // STORAGE_H_