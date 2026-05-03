#ifndef CLIPWHEEL_DRAW_UTILS_H
#define CLIPWHEEL_DRAW_UTILS_H

#include "app.h"

HFONT create_app_font(int height, int weight, const wchar_t *face);
void init_theme_objects(void);
void destroy_theme_objects(void);
float clamp01f(float v);
float ease_out_cubicf(float t);
float ease_out_backf(float t);
COLORREF mix_color(COLORREF a, COLORREF b, float t);
void fill_vertical_gradient(HDC hdc, const RECT *rc, COLORREF top, COLORREF bottom);
void fill_round_gradient(HDC hdc, const RECT *rc, COLORREF top, COLORREF bottom, int radius);
void draw_rounded_rect(HDC hdc, const RECT *rc, COLORREF fill, COLORREF border, int radius);
void draw_round_border(HDC hdc, const RECT *rc, COLORREF border, int radius, int pen_width);
void truncate_preview(wchar_t *dest, size_t cch_dest, const wchar_t *src, size_t max_chars);
int button_is_primary(int id);
void draw_button(const DRAWITEMSTRUCT *di);

#endif
