#include "app.h"
#include "draw_utils.h"

static const COLORREF kPreviewPal[NSECT] = {
    RGB(91,140,250), RGB(131,108,245), RGB(154,127,255), RGB(76,189,201),
    RGB(71,162,255), RGB(105,136,249), RGB(121,93,236),  RGB(88,196,161)
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

int preview_x_hit_test(int cx, int cy, int px, int py, int slot_count) {
    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / NSECT;
    int x_arm = g_prev_dyn_outer_r - g_prev_dyn_x_btn_r - 2;
    for (int i = 0; i < NSECT; i++) {
        int slot = sector_to_slot(i);
        if (slot < 0 || slot >= slot_count) continue;
        double mid = -pi / 2.0 + i * step;
        int bx = cx + (int)(x_arm * cos(mid));
        int by = cy - (int)(x_arm * sin(mid));
        int dx = px - bx, dy = py - by;
        if (dx * dx + dy * dy <= g_prev_dyn_x_btn_r * g_prev_dyn_x_btn_r) return slot;
    }
    return -1;
}

void preview_do_drop(int target_sector) {
    wchar_t err[256];
    int pin_count;
    if (target_sector < 0) return;
    if (target_sector >= CW_MAX_PIN) target_sector = CW_MAX_PIN - 1;

    if (g_drag_from_kind == CARD_KIND_HISTORY) {
        g_ignore_clip = 1;
        if (!cw_history_pin_text_at(g_drag_text, target_sector, err, ARRAYSIZE(err)) && err[0]) {
            MessageBoxW(g_main_hwnd, err, L"ClipWheel", MB_ICONWARNING);
        }
    } else {
        pin_count = cw_history_pin_count();
        if (pin_count <= 0) return;
        if (target_sector >= pin_count) target_sector = pin_count - 1;
        if (target_sector != g_drag_from_index) {
            cw_history_reorder_pin(g_drag_from_index, target_sector);
        }
    }
    refresh_manager_ui();
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

    fill_vertical_gradient(mem, &rc, RGB(22,24,36), RGB(16,17,25));

    {
        RECT hdr = {0, 0, w, 72};
        fill_vertical_gradient(mem, &hdr, RGB(28,30,42), RGB(20,22,32));
        HPEN sep = CreatePen(PS_SOLID, 1, mix_color(COL_BORDER_SUBTLE, COL_ACCENT, 0.2f));
        HGDIOBJ op = SelectObject(mem, sep);
        MoveToEx(mem, 0, 72, NULL); LineTo(mem, w, 72);
        SelectObject(mem, op); DeleteObject(sep);

        SetBkMode(mem, TRANSPARENT);
        SelectObject(mem, g_font_title);
        SetTextColor(mem, COL_TEXT_PRIMARY);
        RECT tl = {16, 10, w - 130, 40};
        DrawTextW(mem, L"可视轮盘", -1, &tl, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);

        RECT badge = {w - 118, 12, w - 8, 38};
        fill_round_gradient(mem, &badge, RGB(108,116,250), RGB(89,97,231), 12);
        draw_round_border(mem, &badge, COL_ACCENT_GLOW, 12, 1);
        SelectObject(mem, g_font_caption);
        SetTextColor(mem, COL_WHITE);
        DrawTextW(mem, L"拖入固定", -1, &badge, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);

        RECT sub = {16, 42, w - 8, 68};
        SetTextColor(mem, COL_TEXT_SECONDARY);
        DrawTextW(mem, L"拖历史到扇区固定 \xb7 点击 \xd7 取消固定 \xb7 扇区间拖动可排序",
                  -1, &sub, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    }

    preview_calc_radii(w, h);
    int outer_r = g_prev_dyn_outer_r;
    int inner_r = g_prev_dyn_inner_r;
    int x_btn_r = g_prev_dyn_x_btn_r;

    int cx = w / 2;
    int cy = 72 + (h - 72) / 2;

    wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS];
    int slot_count = cw_history_fill_slots(slots);
    int pin_count  = cw_history_pin_count();

    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / NSECT;
    int old_mode = SetBkMode(mem, TRANSPARENT);

    for (int i = 3; i >= 1; i--) {
        int r = outer_r + i * 12;
        COLORREF glow = mix_color(COL_ACCENT_GLOW, COL_BG_DEEP, 0.62f + i * 0.10f);
        HPEN p = CreatePen(PS_SOLID, 1, glow);
        HGDIOBJ op = SelectObject(mem, p);
        HGDIOBJ ob = SelectObject(mem, GetStockObject(NULL_BRUSH));
        Ellipse(mem, cx-r, cy-r, cx+r, cy+r);
        SelectObject(mem, op); SelectObject(mem, ob); DeleteObject(p);
    }

    {
        RECT wr = {cx-outer_r, cy-outer_r, cx+outer_r, cy+outer_r};
        fill_round_gradient(mem, &wr, RGB(28,30,40), RGB(16,17,25), outer_r);
        draw_round_border(mem, &wr, mix_color(COL_WHEEL_BORDER, COL_ACCENT_HOVER, 0.35f), outer_r, 2);
    }

    for (int i = 0; i < NSECT; i++) {
        int slot = sector_to_slot(i);
        int active = (slot >= 0 && slot < slot_count);
        int is_drop_target = (g_drag_active && g_drag_drop_sector == i);
        float heat = g_prev_sector_heat[i];
        if (is_drop_target) heat = 1.0f;
        COLORREF base = kPreviewPal[i];
        COLORREF fill_c;
        if (i == 4) {
            fill_c = mix_color(RGB(80, 20, 20), RGB(200, 40, 40), 0.30f + heat * 0.55f);
        } else {
            fill_c = active
                ? mix_color(COL_WHEEL_SECTOR, base, 0.15f + heat * 0.85f)
                : (is_drop_target
                   ? mix_color(COL_WHEEL_SECTOR, base, 0.60f)
                   : mix_color(COL_WHEEL_SECTOR, COL_BG_DEEP, 0.50f));
        }
        COLORREF border_c = mix_color(COL_WHEEL_BORDER, COL_WHITE, heat * 0.9f);
        if (i == 4) border_c = mix_color(RGB(150, 40, 40), RGB(255, 100, 100), heat * 0.9f);
        HBRUSH br  = CreateSolidBrush(fill_c);
        HPEN   pen = CreatePen(PS_SOLID, (heat > 0.5f || is_drop_target || i == 4) ? 2 : 1, border_c);
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

    {
        RECT hub = {cx-inner_r, cy-inner_r, cx+inner_r, cy+inner_r};
        fill_round_gradient(mem, &hub, RGB(36,38,52), RGB(23,24,34), inner_r);
        draw_round_border(mem, &hub, mix_color(COL_ACCENT, COL_ACCENT_GLOW, 0.35f), inner_r, 2);
        SelectObject(mem, g_font_caption);
        SetTextColor(mem, g_drag_active ? COL_ACCENT_GLOW : mix_color(COL_TEXT_TERTIARY, COL_TEXT_SECONDARY, 0.4f));
        DrawTextW(mem, g_drag_active ? L"\u2191" : L"\u2299", -1, &hub, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    }

    int label_r = (inner_r + outer_r) / 2;
    SelectObject(mem, g_font_caption);
    for (int i = 0; i < NSECT; i++) {
        int slot = sector_to_slot(i);
        double mid = -pi/2.0 + i*step;
        int mx = cx + (int)(label_r * cos(mid));
        int my = cy - (int)(label_r * sin(mid));
        RECT lr = {mx-46, my-12, mx+46, my+12};

        if (i == 4) {
            SetTextColor(mem, mix_color(RGB(200, 80, 80), RGB(255, 130, 130), g_prev_sector_heat[i]));
            DrawTextW(mem, L"\u2715 \u53d6\u6d88", -1, &lr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        } else if (slot >= 0 && slot < slot_count) {
            wchar_t prev[20];
            truncate_preview(prev, ARRAYSIZE(prev), slots[slot], 8);
            int hot = (i == g_prev_hover_sector);
            int drop = (g_drag_active && g_drag_drop_sector == i);
            SetTextColor(mem, drop  ? RGB(160,255,200) :
                              hot   ? COL_WHITE :
                                      mix_color(COL_TEXT_TERTIARY, COL_TEXT_PRIMARY, 0.45f));
            DrawTextW(mem, prev, -1, &lr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);

            if (slot < pin_count) {
                int x_arm = outer_r - x_btn_r - 2;
                int bx = cx + (int)(x_arm * cos(mid));
                int by = cy - (int)(x_arm * sin(mid));
                int xhot = (g_prev_x_hover_sector == i);
                COLORREF xfill  = xhot ? RGB(244, 63, 94) : RGB(88, 28, 34);
                COLORREF xbor   = xhot ? RGB(251, 113, 133) : RGB(159, 18, 57);
                HBRUSH xbr = CreateSolidBrush(xfill);
                HPEN   xp  = CreatePen(PS_SOLID, 1, xbor);
                HGDIOBJ xob = SelectObject(mem, xbr);
                HGDIOBJ xop = SelectObject(mem, xp);
                Ellipse(mem, bx-x_btn_r, by-x_btn_r, bx+x_btn_r, by+x_btn_r);
                SelectObject(mem, xob); SelectObject(mem, xop);
                DeleteObject(xbr); DeleteObject(xp);
                SetTextColor(mem, xhot ? RGB(255,220,220) : RGB(170,100,100));
                RECT xr = {bx-x_btn_r, by-x_btn_r, bx+x_btn_r, by+x_btn_r};
                DrawTextW(mem, L"\xd7", -1, &xr, DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
            }
        } else {
            SetTextColor(mem, g_drag_active
                ? mix_color(COL_TEXT_TERTIARY, COL_ACCENT_GLOW, 0.5f)
                : COL_TEXT_TERTIARY);
            DrawTextW(mem, g_drag_active ? L"+" : L"\xb7", -1, &lr,
                      DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        }
    }

    if (g_drag_active) {
        RECT dr = {0, h-40, w, h};
        fill_vertical_gradient(mem, &dr, RGB(0,0,0), RGB(18,22,36));
        SetTextColor(mem, COL_ACCENT_GLOW);
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
        int slot_count = cw_history_fill_slots(slots);
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
        int slot_count = cw_history_fill_slots(slots);
        int pin_count  = cw_history_pin_count();

        int xslot = preview_x_hit_test(cx, cy, x, y, pin_count);
        if (xslot >= 0) {
            cw_history_unpin_display_index(xslot);
            refresh_manager_ui();
            return 0;
        }

        int sec = preview_sector_from_point(cx, cy, x, y);
        int slot = sector_to_slot(sec);
        if (slot >= 0 && slot < pin_count) {
            g_drag_from_kind  = CARD_KIND_PIN;
            g_drag_from_index = slot;
            wcsncpy_s(g_drag_text, CW_MAX_CHARS, slots[slot], _TRUNCATE);
            g_drag_pending_start.x = x;
            g_drag_pending_start.y = y;
            g_drag_pending = 1;
            SetCapture(hwnd);
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
        fill_vertical_gradient((HDC)wp, &rc, RGB(22, 24, 36), RGB(16, 17, 25));
        return 1;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
