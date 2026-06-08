#include "app.h"
#include "draw_utils.h"

static const COLORREF kPreviewPal[NSECT] = {
    RGB(148,163,184), RGB(196,167,144), RGB(134,183,152), RGB(176,162,198),
    RGB(211,135,135), RGB(128,184,184), RGB(204,186,132), RGB(200,152,176)
};

static const COLORREF kPreviewPalLight[NSECT] = {
    RGB(99, 130, 180),  RGB(200, 130, 90),  RGB(60, 160, 120),  RGB(150, 110, 190),
    RGB(210, 100, 100), RGB(60, 160, 160),  RGB(200, 160, 60),  RGB(200, 110, 150)
};

void preview_calc_radii(int w, int h) {
    int avail_h = (h - 72 - 20) / 2;
    int avail_w = (w - 60) / 2;
    int avail = (avail_h < avail_w) ? avail_h : avail_w;
    if (avail < 28) avail = 28;
    g_prev_dyn_outer_r = (avail < PREV_OUTER_R) ? avail : PREV_OUTER_R;
    g_prev_dyn_inner_r = g_prev_dyn_outer_r * PREV_INNER_R / PREV_OUTER_R;
    if (g_prev_dyn_inner_r < 16) g_prev_dyn_inner_r = 16;
    g_prev_dyn_x_btn_r = g_prev_dyn_outer_r * PREV_X_BTN_R / PREV_OUTER_R;
    if (g_prev_dyn_x_btn_r < 6) g_prev_dyn_x_btn_r = 6;
}

int preview_sector_from_point(int cx, int cy, int px, int py) {
    double dx = px - cx, dy = py - cy;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < g_prev_dyn_inner_r || dist > g_prev_dyn_outer_r + 20) return -1;
    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / NSECT;
    double ang = atan2(-dy, dx);
    double shifted = ang + pi / 2.0 + step / 2.0;
    while (shifted < 0) shifted += 2.0 * pi;
    while (shifted >= 2.0 * pi) shifted -= 2.0 * pi;
    int idx = (int)(shifted / step);
    if (idx < 0) idx = 0;
    if (idx >= NSECT) idx = NSECT - 1;
    return idx;
}

int preview_x_hit_test(int cx, int cy, int px, int py, int pin_count) {
    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / NSECT;
    (void)pin_count; /* use sector-based lookup instead */
    int x_arm = g_prev_dyn_outer_r - g_prev_dyn_x_btn_r - 2;
    for (int i = 0; i < NSECT; i++) {
        if (sector_to_slot(i) < 0) continue;
        if (cw_history_find_pin_at_sector(i) < 0) continue;
        double mid = -pi / 2.0 + i * step;
        int bx = cx + (int)(x_arm * cos(mid));
        int by = cy - (int)(x_arm * sin(mid));
        int dx = px - bx, dy = py - by;
        if (dx * dx + dy * dy <= g_prev_dyn_x_btn_r * g_prev_dyn_x_btn_r) return i;
    }
    return -1;
}

void preview_do_drop(int target_slot) {
    wchar_t err[256];
    int pin_count, target_sector;
    if (target_slot < 0) return;

    target_sector = slot_to_sector(target_slot);

    if (g_drag_from_kind == CARD_KIND_HISTORY) {
        g_ignore_clip = 1;
        if (!cw_history_pin_text_at(g_drag_text, target_sector, err, ARRAYSIZE(err)) && err[0]) {
            MessageBoxW(g_main_hwnd, err, L"ClipWheel", MB_ICONWARNING);
        }
    } else {
        pin_count = cw_history_pin_count();
        if (pin_count <= 0) return;
        if (target_slot >= pin_count) target_slot = pin_count - 1;
        if (target_slot != g_drag_from_index) {
            cw_history_reorder_pin(g_drag_from_index, target_slot);
        }
    }
    refresh_manager_ui();
}

static void draw_preview_header(HDC mem, int w) {
    RECT hdr = {0, 0, w, 72};
    fill_vertical_gradient(mem, &hdr, TC_BG_ELEVATED, TC_BG_CARD);
    HPEN sep = CreatePen(PS_SOLID, 1, TC_BORDER_DEFAULT);
    HGDIOBJ op = SelectObject(mem, sep);
    MoveToEx(mem, 0, 72, NULL); LineTo(mem, w, 72);
    SelectObject(mem, op); DeleteObject(sep);

    SetBkMode(mem, TRANSPARENT);
    SelectObject(mem, g_font_title);
    SetTextColor(mem, TC_TEXT_PRIMARY);
    RECT tl = {16, 10, w - 130, 40};
    DrawTextW(mem, L"\u53ef\u89c6\u8f6e\u76d8", -1, &tl, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);

    RECT badge = {w - 118, 12, w - 8, 38};
    fill_round_gradient(mem, &badge, TC_ACCENT, TC_ACCENT_DEEP, RADIUS_LG);
    draw_round_border(mem, &badge, TC_ACCENT_GLOW, RADIUS_LG, 1);
    SelectObject(mem, g_font_caption);
    SetTextColor(mem, TC_WHITE);
    DrawTextW(mem, L"\u62d6\u5165\u56fa\u5b9a", -1, &badge, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);

    RECT sub = {16, 42, w - 8, 68};
    SetTextColor(mem, TC_TEXT_SECONDARY);
    DrawTextW(mem, L"\u62d6\u5386\u53f2\u5230\u6247\u533a\u56fa\u5b9a \xb7 \u70b9\u51fb \xd7 \u53d6\u6d88\u56fa\u5b9a \xb7 \u6247\u533a\u95f4\u62d6\u52a8\u53ef\u6392\u5e8f",
              -1, &sub, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}

static void draw_preview_wheel_body(HDC mem, int cx, int cy, int outer_r, int inner_r,
                                     const wchar_t (*slots)[CW_MAX_CHARS]) {
    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / NSECT;

    /* Glow rings */
    for (int i = 3; i >= 1; i--) {
        int r = outer_r + i * 12;
        COLORREF glow = mix_color(TC_ACCENT_GLOW, TC_BG_DEEP, 0.62f + i * 0.10f);
        HPEN p = CreatePen(PS_SOLID, 1, glow);
        HGDIOBJ op = SelectObject(mem, p);
        HGDIOBJ ob = SelectObject(mem, GetStockObject(NULL_BRUSH));
        Ellipse(mem, cx-r, cy-r, cx+r, cy+r);
        SelectObject(mem, op); SelectObject(mem, ob); DeleteObject(p);
    }

    /* Wheel base */
    {
        RECT wr = {cx-outer_r, cy-outer_r, cx+outer_r, cy+outer_r};
        fill_round_gradient(mem, &wr, TC_WHEEL_SECTOR, TC_BG_DEEP, outer_r);
        draw_round_border(mem, &wr, mix_color(TC_WHEEL_BORDER, TC_ACCENT_HOVER, 0.35f), outer_r, 2);
    }

    /* Sectors */
    const COLORREF *pal = g_theme == THEME_LIGHT ? kPreviewPalLight : kPreviewPal;
    for (int i = 0; i < NSECT; i++) {
        int slot = sector_to_slot(i);
        int active = (slot >= 0 && slots[slot][0]);
        int is_drop_target = (g_drag_active && g_drag_drop_sector == i);
        float heat = g_prev_sector_heat[i];
        if (is_drop_target) heat = 1.0f;
        COLORREF base = pal[i];
        COLORREF fill_c;
        if (i == 4) {
            fill_c = mix_color(TC_CANCEL_SECTOR_A, TC_CANCEL_SECTOR_B, 0.25f + heat * 0.60f);
        } else {
            fill_c = active
                ? mix_color(mix_color(TC_WHEEL_SECTOR, base, 0.30f), base, 0.45f + heat * 0.50f)
                : (is_drop_target
                   ? mix_color(TC_WHEEL_SECTOR, base, 0.60f)
                   : mix_color(TC_WHEEL_SECTOR, TC_WHEEL_BG, 0.5f));
        }
        COLORREF border_c = mix_color(TC_WHEEL_BORDER, TC_ACCENT_GLOW, heat * 0.70f);
        if (i == 4) {
            border_c = mix_color(TC_CANCEL_BORDER_A, TC_CANCEL_BORDER_B, heat * 0.80f);
        }
        HBRUSH br  = CreateSolidBrush(fill_c);
        HPEN   pen = CreatePen(PS_SOLID, (heat > 0.5f || is_drop_target || i == 4) ? 1 : 0, border_c);
        HGDIOBJ ob = SelectObject(mem, br);
        HGDIOBJ op = SelectObject(mem, pen);
        double t0 = -pi/2.0 + i*step - step/2.0;
        double t1 = t0 + step;
        Pie(mem, cx-outer_r, cy-outer_r, cx+outer_r, cy+outer_r,
            cx+(int)(outer_r*cos(t0)), cy-(int)(outer_r*sin(t0)),
            cx+(int)(outer_r*cos(t1)), cy-(int)(outer_r*sin(t1)));
        SelectObject(mem, ob); SelectObject(mem, op);
        DeleteObject(br); DeleteObject(pen);
    }

    /* Divider lines */
    {
        HPEN div = CreatePen(PS_SOLID, 1, mix_color(TC_BORDER_DEFAULT, TC_BG_DEEP, 0.5f));
        HGDIOBJ op = SelectObject(mem, div);
        for (int i = 0; i < NSECT; i++) {
            double angle = -pi/2.0 + i*step - step/2.0;
            int x1 = cx + (int)(inner_r * cos(angle));
            int y1 = cy - (int)(inner_r * sin(angle));
            int x2 = cx + (int)(outer_r * cos(angle));
            int y2 = cy - (int)(outer_r * sin(angle));
            MoveToEx(mem, x1, y1, NULL);
            LineTo(mem, x2, y2);
        }
        SelectObject(mem, op); DeleteObject(div);
    }
}

static void draw_preview_hub(HDC mem, int cx, int cy, int inner_r) {
    RECT hub = {cx-inner_r, cy-inner_r, cx+inner_r, cy+inner_r};
    fill_round_gradient(mem, &hub, TC_BG_ELEVATED, TC_BG_CARD, inner_r);
    draw_round_border(mem, &hub, mix_color(TC_ACCENT, TC_ACCENT_GLOW, 0.35f), inner_r, 2);
    SelectObject(mem, g_font_caption);
    SetTextColor(mem, g_drag_active ? TC_ACCENT_GLOW : mix_color(TC_TEXT_TERTIARY, TC_TEXT_SECONDARY, 0.4f));
    DrawTextW(mem, g_drag_active ? L"\u2191" : L"\u2299", -1, &hub, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
}

static void draw_preview_labels(HDC mem, int cx, int cy, int outer_r, int inner_r, int x_btn_r,
                                 const wchar_t (*slots)[CW_MAX_CHARS],
                                 const wchar_t (*slot_disp)[64], int pin_count) {
    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / NSECT;
    int label_r = (inner_r + outer_r) / 2;
    SelectObject(mem, g_font_caption);
    for (int i = 0; i < NSECT; i++) {
        int slot = sector_to_slot(i);
        double mid = -pi/2.0 + i*step;
        int mx = cx + (int)(label_r * cos(mid));
        int my = cy - (int)(label_r * sin(mid));
        RECT lr = {mx-46, my-12, mx+46, my+12};

        if (i == 4) {
            SetTextColor(mem, mix_color(TC_CANCEL_TEXT_A, TC_CANCEL_TEXT_B, g_prev_sector_heat[i]));
            DrawTextW(mem, L"\u2715 \u53d6\u6d88", -1, &lr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        } else if (slot >= 0 && slots[slot][0]) {
            wchar_t prev[64];
            truncate_preview(prev, ARRAYSIZE(prev), slot_disp[slot], 8);
            int hot = (i == g_prev_hover_sector);
            int drop = (g_drag_active && g_drag_drop_sector == i);
            SetTextColor(mem, drop  ? TC_SUCCESS :
                              hot   ? TC_WHITE :
                                      mix_color(TC_TEXT_TERTIARY, TC_TEXT_PRIMARY, 0.45f));
            DrawTextW(mem, prev, -1, &lr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);

            if (cw_history_find_pin_at_sector(i) >= 0) {
                int x_arm = outer_r - x_btn_r - 2;
                int bx = cx + (int)(x_arm * cos(mid));
                int by = cy - (int)(x_arm * sin(mid));
                int xhot = (g_prev_x_hover_sector == i);
                COLORREF xfill  = xhot ? TC_X_FILL_HOT : TC_X_FILL;
                COLORREF xbor   = xhot ? TC_X_BORDER_HOT : TC_X_BORDER;
                HBRUSH xbr = CreateSolidBrush(xfill);
                HPEN   xp  = CreatePen(PS_SOLID, 1, xbor);
                HGDIOBJ xob = SelectObject(mem, xbr);
                HGDIOBJ xop = SelectObject(mem, xp);
                Ellipse(mem, bx-x_btn_r, by-x_btn_r, bx+x_btn_r, by+x_btn_r);
                SelectObject(mem, xob); SelectObject(mem, xop);
                DeleteObject(xbr); DeleteObject(xp);
                SetTextColor(mem, xhot ? TC_X_TEXT_HOT : TC_X_TEXT);
                RECT xr = {bx-x_btn_r, by-x_btn_r, bx+x_btn_r, by+x_btn_r};
                DrawTextW(mem, L"\xd7", -1, &xr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
            }
        } else {
            SetTextColor(mem, g_drag_active
                ? mix_color(TC_TEXT_TERTIARY, TC_ACCENT_GLOW, 0.5f)
                : TC_TEXT_TERTIARY);
            DrawTextW(mem, g_drag_active ? L"+" : L"\xb7", -1, &lr,
                      DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        }
    }
}

void paint_wheel_preview(HWND hwnd) {
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc = BeginPaint(hwnd, &ps);
    HDC mem = CreateCompatibleDC(hdc);
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP old_bmp = (HBITMAP)SelectObject(mem, bmp);

    fill_vertical_gradient(mem, &rc, TC_BG_CARD, TC_BG_SURFACE);

    draw_preview_header(mem, w);

    preview_calc_radii(w, h);
    int outer_r = g_prev_dyn_outer_r;
    int inner_r = g_prev_dyn_inner_r;
    int x_btn_r = g_prev_dyn_x_btn_r;
    int cx = w / 2;
    int cy = 72 + (h - 72) / 2;

    wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS];
    wchar_t slot_disp[CW_MAX_SLOT][64];
    ZeroMemory(slots, sizeof(slots));
    ZeroMemory(slot_disp, sizeof(slot_disp));
    int slot_count = cw_history_fill_slots(slots);
    cw_history_fill_slot_display(slot_disp);
    int pin_count  = cw_history_pin_count();

    int old_mode = SetBkMode(mem, TRANSPARENT);

    draw_preview_wheel_body(mem, cx, cy, outer_r, inner_r, slots);
    draw_preview_hub(mem, cx, cy, inner_r);
    draw_preview_labels(mem, cx, cy, outer_r, inner_r, x_btn_r, slots, slot_disp, pin_count);

    if (g_drag_active) {
        RECT dr = {0, h-40, w, h};
        fill_vertical_gradient(mem, &dr, TC_BG_DEEP, TC_BG_SURFACE);
        SetTextColor(mem, TC_ACCENT_GLOW);
        SelectObject(mem, g_font_caption);
        DrawTextW(mem, L"\u677e\u5f00\u9f20\u6807\u5b8c\u6210\u56fa\u5b9a  \xb7  \u62d6\u81f3\u6700\u4e0b\u65b9\u53d6\u6d88",
                  -1, &dr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    }

    SetBkMode(mem, old_mode);
    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old_bmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WheelPreviewProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        SetTimer(hwnd, 3, 16, NULL);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 3);
        return 0;
    case WM_TIMER: {
        if (wp != 3) break;
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        int cx = w / 2, cy = 72 + (h - 72) / 2;
        preview_calc_radii(w, h);

        wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS];
        wchar_t slot_disp[CW_MAX_SLOT][64];
        ZeroMemory(slots, sizeof(slots));
        ZeroMemory(slot_disp, sizeof(slot_disp));
        int slot_count = cw_history_fill_slots(slots);
        cw_history_fill_slot_display(slot_disp);
        int pin_count  = cw_history_pin_count();

        POINT pt;
        GetCursorPos(&pt);
        RECT wrc;
        GetWindowRect(hwnd, &wrc);
        BOOL cursor_in_window = (pt.x >= wrc.left && pt.x < wrc.right &&
                                 pt.y >= wrc.top  && pt.y < wrc.bottom);

        POINT cpt = pt;
        ScreenToClient(hwnd, &cpt);

        int new_hover  = -1;
        int new_x_hover = -1;
        int new_drop   = -1;

        if (cursor_in_window && !g_drag_active) {
            new_hover   = preview_sector_from_point(cx, cy, cpt.x, cpt.y);
            new_x_hover = preview_x_hit_test(cx, cy, cpt.x, cpt.y, pin_count);
        }

        if (g_drag_active) {
            POINT dpt = g_drag_pos_screen;
            POINT dcpt = dpt;
            ScreenToClient(hwnd, &dcpt);
            RECT cr;
            GetClientRect(hwnd, &cr);
            if (dcpt.x >= 0 && dcpt.x < cr.right && dcpt.y >= 0 && dcpt.y < cr.bottom) {
                new_drop = preview_sector_from_point(cx, cy, dcpt.x, dcpt.y);
            }
        }

        int need = 0;
        for (int i = 0; i < NSECT; i++) {
            float target = (i == new_hover) ? 1.0f : 0.0f;
            float prev_h = g_prev_sector_heat[i];
            g_prev_sector_heat[i] += (target - g_prev_sector_heat[i]) * 0.22f;
            if (fabs(g_prev_sector_heat[i] - prev_h) > 0.005f) need = 1;
        }
        if (new_hover   != g_prev_hover_sector)   { g_prev_hover_sector   = new_hover;   need = 1; }
        if (new_x_hover != g_prev_x_hover_sector) { g_prev_x_hover_sector = new_x_hover; need = 1; }
        if (new_drop    != g_drag_drop_sector)     { g_drag_drop_sector    = new_drop;    need = 1; }

        if (need || g_drag_active) InvalidateRect(hwnd, NULL, FALSE);
        (void)slot_count; (void)pin_count;
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int cx = (rc.right-rc.left)/2, cy = 72 + (rc.bottom-rc.top-72)/2;
        preview_calc_radii(rc.right - rc.left, rc.bottom - rc.top);

        wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS];
        ZeroMemory(slots, sizeof(slots));
        int slot_count = cw_history_fill_slots(slots);
        int pin_count  = cw_history_pin_count();

        int xsec = preview_x_hit_test(cx, cy, x, y, pin_count);
        if (xsec >= 0) {
            int pin_idx = cw_history_find_pin_at_sector(xsec);
            if (pin_idx >= 0) {
                cw_history_unpin_display_index(pin_idx);
                refresh_manager_ui();
            }
            return 0;
        }

        int sec = preview_sector_from_point(cx, cy, x, y);
        int slot = sector_to_slot(sec);
        int pin_idx = cw_history_find_pin_at_sector(sec);
        if (slot >= 0 && pin_idx >= 0) {
            g_drag_from_kind  = CARD_KIND_PIN;
            g_drag_from_index = pin_idx;
            wcsncpy_s(g_drag_text, CW_MAX_CHARS, slots[slot], _TRUNCATE);
            g_drag_pending_start.x = x;
            g_drag_pending_start.y = y;
            g_drag_pending = 1;
        }
        (void)slot_count;
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (g_drag_pending && (wp & MK_LBUTTON)) {
            int dx = GET_X_LPARAM(lp) - g_drag_pending_start.x;
            int dy = GET_Y_LPARAM(lp) - g_drag_pending_start.y;
            int thresh = GetSystemMetrics(SM_CXDRAG);
            if (dx*dx + dy*dy > thresh*thresh) {
                g_drag_pending = 0;
                g_drag_active  = 1;
                SetCapture(hwnd);
            }
        }
        if (g_drag_active && GetCapture() == hwnd) {
            GetCursorPos(&g_drag_pos_screen);
            SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        g_drag_pending = 0;
        if (g_drag_active && GetCapture() == hwnd) {
            ReleaseCapture();
            POINT pt;
            GetCursorPos(&pt);
            RECT wrc;
            GetWindowRect(hwnd, &wrc);
            if (pt.x >= wrc.left && pt.x < wrc.right && pt.y >= wrc.top && pt.y < wrc.bottom) {
                POINT cpt = pt;
                ScreenToClient(hwnd, &cpt);
                RECT rc;
                GetClientRect(hwnd, &rc);
                int cx = (rc.right-rc.left)/2, cy = 72 + (rc.bottom-rc.top-72)/2;
                preview_calc_radii(rc.right - rc.left, rc.bottom - rc.top);
                int target = preview_sector_from_point(cx, cy, cpt.x, cpt.y);
                if (target == 4 && g_drag_from_kind == CARD_KIND_PIN) {
                    cw_history_unpin_display_index(g_drag_from_index);
                    refresh_manager_ui();
                } else if (target >= 0) {
                    int target_slot = sector_to_slot(target);
                    if (target_slot >= 0 && !(g_drag_from_kind == CARD_KIND_PIN && target_slot == g_drag_from_index)) {
                        preview_do_drop(target_slot);
                    }
                }
            } else {
                if (g_drag_from_kind == CARD_KIND_PIN) {
                    cw_history_unpin_display_index(g_drag_from_index);
                    refresh_manager_ui();
                }
            }
            g_drag_active     = 0;
            g_drag_drop_sector = -1;
            g_prev_cancel_heat = 0.0f;
            if (g_wheel_preview_hwnd) InvalidateRect(g_wheel_preview_hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_CAPTURECHANGED:
        g_drag_pending = 0;
        if (g_drag_active) {
            g_drag_active      = 0;
            g_drag_drop_sector = -1;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    case WM_PAINT:
        paint_wheel_preview(hwnd);
        return 0;
    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        fill_vertical_gradient((HDC)wp, &rc, TC_BG_CARD, TC_BG_SURFACE);
        return 1;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
