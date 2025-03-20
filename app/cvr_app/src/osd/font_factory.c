// Copyright 2021 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "font_factory.h"
#include <pthread.h>

static FT_Library font_library;
static FT_Face font_face;
static FT_GlyphSlot font_slot;
static FT_Vector font_pen;
static double font_angle;
static int font_size;
static unsigned int font_color_osd;
static char *font_path[128];
static unsigned int color_index;
static unsigned int trans_index;
static pthread_mutex_t g_font_mutex = PTHREAD_MUTEX_INITIALIZER;


int create_font(const char *font_path, int font_size) {
    pthread_mutex_lock(&g_font_mutex);
    FT_Init_FreeType(&font_library);
    FT_New_Face(font_library, font_path, 0, &font_face);
    if (!font_face) {
        printf("please check font_path %s\n", font_path);
        pthread_mutex_unlock(&g_font_mutex);
        return -1;
    }

    FT_Set_Pixel_Sizes(font_face, font_size, 0);
    font_size = font_size;
    memcpy(font_path, font_path, strlen(font_path));
    font_slot = font_face->glyph;
    pthread_mutex_unlock(&g_font_mutex);

    return 0;
}

int destroy_font(void) {
    pthread_mutex_lock(&g_font_mutex);
    if (font_face) {
        FT_Done_Face(font_face);
        font_face = NULL;
    }
    if (font_library) {
        FT_Done_FreeType(font_library);
        font_library = NULL;
    }
    pthread_mutex_unlock(&g_font_mutex);

    return 0;
}

int set_font_size(int font_size) {
    pthread_mutex_lock(&g_font_mutex);
    FT_Set_Pixel_Sizes(font_face, font_size, font_size);
    pthread_mutex_unlock(&g_font_mutex);
    font_size = font_size;

    return 0;
}

int get_font_size(void) { return font_size; }

unsigned int set_font_color(unsigned int font_color) {
    pthread_mutex_lock(&g_font_mutex);
    font_color_osd = 0x000000FF;
    font_color_osd |= font_color >> 8 & 0x0000FF00;  // R
    font_color_osd |= font_color << 8 & 0x00FF0000;  // G
    font_color_osd |= font_color << 24 & 0xFF000000; // B
    pthread_mutex_unlock(&g_font_mutex);

    return 0;
}

unsigned int get_font_color() { return font_color_osd; }

static void draw_argb8888_buffer(unsigned int *buffer, int buf_w, int buf_h) {
    int i, j, p, q;
    int left = font_slot->bitmap_left;
    int top = (font_face->size->metrics.ascender >> 6) - font_slot->bitmap_top;
    int right = left + font_slot->bitmap.width;
    int bottom = top + font_slot->bitmap.rows;

    for (j = top, q = 0; j < bottom; j++, q++) {
        int offset = j * buf_w;
        int bmp_offset = q * font_slot->bitmap.width;
        for (i = left, p = 0; i < right; i++, p++) {
            if (i < 0 || j < 0 || i >= buf_w || j >= buf_h)
                continue;
                if (font_slot->bitmap.buffer[bmp_offset + p]) {
                    buffer[offset + i] = font_color_osd;
                } else {
                    buffer[offset + i] = 0x00000000;
            }
        }
    }
}

static void draw_argb8888_wchar(unsigned char *buffer,
    int buf_w, int buf_h, const wchar_t wch) {

    pthread_mutex_lock(&g_font_mutex);
    if (!font_face) {
        printf("please check font_path %s\n", font_path);
        pthread_mutex_unlock(&g_font_mutex);
        return;
    }
    FT_Error error;
    FT_Set_Transform(font_face, NULL, &font_pen);
    error = FT_Load_Char(font_face, wch, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
    FT_Render_Glyph(font_slot, FT_RENDER_MODE_NORMAL); // 8bit per pixel
    if (error) {
        printf("FT_Load_Char error\n");
        return;
    }
    draw_argb8888_buffer((unsigned int *)buffer, buf_w, buf_h);
    pthread_mutex_unlock(&g_font_mutex);
}

void draw_argb8888_text(unsigned char *buffer,
    int buf_w, int buf_h, const wchar_t *wstr) {

    if (wstr == NULL) {
        printf("wstr is NULL\n");
        return;
    }

    int x_offset = 0;
    int len = wcslen(wstr);
    font_pen.x = 0 * 64;
    font_pen.y = 0 * 64;
    for (int i = 0; i < len; i++) {
        draw_argb8888_wchar(buffer, buf_w, buf_h, wstr[i]);
        font_pen.x += font_slot->advance.x;
        font_pen.y += font_slot->advance.y;
    }
}
