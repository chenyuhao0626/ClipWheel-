#ifndef CLIPWHEEL_WHEEL_PREVIEW_H
#define CLIPWHEEL_WHEEL_PREVIEW_H

#include "app.h"

void preview_calc_radii(int w, int h);
int preview_sector_from_point(int cx, int cy, int px, int py);
int preview_x_hit_test(int cx, int cy, int px, int py, int slot_count);
void preview_do_drop(int target_sector);
void paint_wheel_preview(HWND hwnd);
LRESULT CALLBACK WheelPreviewProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

#endif
