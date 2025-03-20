// Copyright 2021 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _CVR_FONT_FACTORY_H_
#define _CVR_FONT_FACTORY_H_

#include "ft2build.h"
#include <freetype2/freetype/freetype.h>
#include <freetype2/freetype/ftimage.h>

int create_font(const char *font_path, int font_size);
int destroy_font(void);
int set_font_size(int font_size);
int get_font_size(void);
unsigned int set_font_color(unsigned int font_color);
unsigned int get_font_color();
void draw_argb8888_text(unsigned char *buffer, int buf_w, int buf_h, const wchar_t *wstr);

#endif //_CVR_FONT_FACTORY_H_