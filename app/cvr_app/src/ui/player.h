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

#ifndef PLAYER_H_
#define PLAYER_H_

#include <rkadk/rkadk_storage.h>

typedef struct player_file_info {
    int curposition;
    RKADK_FILE_LIST *list;
} PLAYER_FILE_INFO;

typedef struct player_info_array {
    int totallnum;
    PLAYER_FILE_INFO *replay_list;
} PLAYER_INFO_ARRAY;

extern void player_start(PLAYER_FILE_INFO *filepath, int type);
extern void player_stop(void);

#endif // PLAYER_H_