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

#ifndef _CVR_PLAYER_H_
#define _CVR_PLAYER_H_

typedef enum {
  PLAYER_STOPPED = 0,
  PLAYER_PAUSE,
  PLAYER_PLAY,
} PLAYER_VIDEO_STATUS;

int cvr_player_start(char *filepath);
int cvr_player_stop(void);
int cvr_player_pause(void);
int cvr_player_switch(char *filepath);
int cvr_player_get_duration(int *pDuration);
int cvr_player_play_audio(char *filepath);

#endif //_CVR_PLAYER_H_
