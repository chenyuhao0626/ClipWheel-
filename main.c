#include "app.h"
#include "draw_utils.h"

/* ─── Global definitions ──────────────────────────────────────────────────── */
const wchar_t *kOverlayClass = L"ClipWheelOverlay";
const wchar_t *kMainClass = L"ClipWheelMain";
const wchar_t *kCardListClass = L"ClipWheelCardList";
const wchar_t *kWheelPreviewClass = L"ClipWheelPreview";

HINSTANCE g_hinst;
HWND g_overlay_hwnd;
HWND g_main_hwnd;
HWND g_pin_list_hwnd;
HWND g_history_list_hwnd;
HWND g_auto_paste_hwnd;
HWND g_floating_mode_hwnd;
HWND g_record_btn_hwnd;
HWND g_wheel_preview_hwnd;
static HWND g_auto_start_hwnd = NULL;
static int g_side_w_left = 260;
static int g_side_w_right = 260;
static int g_dragging_splitter = 0; // 0: none, 1: left, 2: right
#define SPLITTER_HIT_W 8
#define MIN_SIDE_W 180
#define MIN_CENTER_W 280
HWND g_undo_btn_hwnd;

int g_vk = VK_OEM_3;
int g_mod;
int g_auto_paste = 1;
int g_floating_mode = 0;

volatile int g_wheel_visible;
volatile int g_hotkey_suppress;
int g_sel = -1;
int g_slot_count;
wchar_t g_slots[CW_MAX_SLOT][CW_MAX_CHARS];
HWND g_target_hwnd;
POINT g_original_cursor_pos;
POINT g_wheel_center_screen;
int g_overlay_vx;
int g_overlay_vy;
float g_wheel_appear = 1.0f;
float g_sector_heat[CW_MAX_SLOT];
float g_main_phase = 0.0f;

HHOOK g_kb_hook;
NOTIFYICONDATAW g_nid;
int g_tray_added;
int g_ignore_clip;
int g_tray_tip_shown;

wchar_t g_ini[MAX_PATH];
wchar_t g_hotkey_label[64];

HFONT g_font_display;
HFONT g_font_title;
HFONT g_font_body;
HFONT g_font_body_bold;
HFONT g_font_caption;
HBRUSH g_brush_light;
HBRUSH g_brush_dark;
HBRUSH g_brush_white;

int   g_prev_hover_sector   = -1;
int   g_prev_x_hover_sector = -1;
float g_prev_sector_heat[CW_MAX_SLOT];
float g_prev_cancel_heat;
int   g_prev_dyn_outer_r = PREV_OUTER_R;
int   g_prev_dyn_inner_r = PREV_INNER_R;
int   g_prev_dyn_x_btn_r = PREV_X_BTN_R;

int     g_drag_active;
int     g_drag_pending;
int     g_drag_from_kind;
int     g_drag_from_index;
wchar_t g_drag_text[CW_MAX_CHARS];
POINT   g_drag_pending_start;
POINT   g_drag_pos_screen;
int     g_drag_drop_sector = -1;

float g_cancel_heat;

int g_recording = 0;
int g_recorded_vk = 0;
int g_recorded_mod = 0;

UndoEntry g_undo_stack[UNDO_STACK_SIZE];
int g_undo_count = 0;

int g_manager_search = 0;
wchar_t g_manager_search_buf[256] = L"";

/* ─── Forward declarations ────────────────────────────────────────────────── */
static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam);

/* ─── Utility ─────────────────────────────────────────────────────────────── */
void enable_dpi_awareness(void) {
    HMODULE user32 = LoadLibraryW(L"user32.dll");
    if (user32) {
        typedef BOOL(WINAPI *SetProcessDpiAwarenessContextFn)(HANDLE);
        SetProcessDpiAwarenessContextFn fn =
            (SetProcessDpiAwarenessContextFn)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        if (fn) {
            fn((HANDLE)-4);
        } else {
            typedef BOOL(WINAPI *SetProcessDPIAwareFn)(void);
            SetProcessDPIAwareFn fn2 = (SetProcessDPIAwareFn)GetProcAddress(user32, "SetProcessDPIAware");
            if (fn2) fn2();
        }
        FreeLibrary(user32);
    }
}

void format_hotkey_label(void) {
    wchar_t key_name[64] = L"";
    wchar_t mods[64] = L"";
    UINT scan = MapVirtualKeyW((UINT)g_vk, MAPVK_VK_TO_VSC);
    LONG lparam = (LONG)(scan << 16);
    if (!GetKeyNameTextW(lparam, key_name, ARRAYSIZE(key_name))) {
        swprintf_s(key_name, ARRAYSIZE(key_name), L"VK %d", g_vk);
    }
    if (g_mod & MOD_CONTROL) wcscat_s(mods, ARRAYSIZE(mods), L"Ctrl + ");
    if (g_mod & MOD_ALT) wcscat_s(mods, ARRAYSIZE(mods), L"Alt + ");
    if (g_mod & MOD_SHIFT) wcscat_s(mods, ARRAYSIZE(mods), L"Shift + ");
    swprintf_s(g_hotkey_label, ARRAYSIZE(g_hotkey_label), L"%s%s", mods, key_name);
}

void load_config(void) {
    wchar_t exe[MAX_PATH];
    wchar_t *slash;
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    slash = wcsrchr(exe, L'\\');
    if (slash) *(slash + 1) = 0;
    swprintf_s(g_ini, MAX_PATH, L"%sclipwheel.ini", exe);
    g_vk = GetPrivateProfileIntW(L"Hotkey", L"VK", VK_OEM_3, g_ini);
    g_mod = GetPrivateProfileIntW(L"Hotkey", L"Mod", 0, g_ini);
    g_auto_paste = GetPrivateProfileIntW(L"Behavior", L"AutoPaste", 1, g_ini) ? 1 : 0;
    g_floating_mode = GetPrivateProfileIntW(L"Behavior", L"FloatingMode", 0, g_ini) ? 1 : 0;
    format_hotkey_label();
}

void save_behavior_config(void) {
    WritePrivateProfileStringW(L"Behavior", L"AutoPaste", g_auto_paste ? L"1" : L"0", g_ini);
    WritePrivateProfileStringW(L"Behavior", L"FloatingMode", g_floating_mode ? L"1" : L"0", g_ini);
}

void save_hotkey_config(void) {
    wchar_t buf[32];
    swprintf_s(buf, ARRAYSIZE(buf), L"%d", g_vk);
    WritePrivateProfileStringW(L"Hotkey", L"VK", buf, g_ini);
    swprintf_s(buf, ARRAYSIZE(buf), L"%d", g_mod);
    WritePrivateProfileStringW(L"Hotkey", L"Mod", buf, g_ini);
}

int mod_down(void) {
    int alt_down   = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
    int ctrl_down  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    int shift_down = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
    int want_alt   = (g_mod & MOD_ALT)     != 0;
    int want_ctrl  = (g_mod & MOD_CONTROL) != 0;
    int want_shift = (g_mod & MOD_SHIFT)   != 0;
    if (alt_down   != want_alt)   return 0;
    if (ctrl_down  != want_ctrl)  return 0;
    if (shift_down != want_shift) return 0;
    return 1;
}

int vk_matches(DWORD vk) {
    return (int)vk == g_vk;
}

int is_our_window(HWND hwnd) {
    return hwnd && (hwnd == g_overlay_hwnd || hwnd == g_main_hwnd);
}

int sector_to_slot(int sector) {
    if (sector < 4) return sector;
    if (sector == 4) return -1;
    return sector - 1;
}

int slot_to_sector(int slot) {
    if (slot < 4) return slot;
    return slot + 1;
}

void build_slots(void) {
    g_slot_count = cw_history_fill_slots(g_slots);
    if (g_sel != 4) {
        if (g_slot_count <= 0) g_sel = -1;
        else {
            int slot = sector_to_slot(g_sel);
            if (slot >= g_slot_count) g_sel = slot_to_sector(g_slot_count - 1);
        }
    }
}

int sector_from_point(int cx, int cy, int px, int py) {
    double dx, dy, dist, outer_select, outer_sticky;
    dx = px - cx;
    dy = py - cy;
    dist = hypot(dx, dy);
    outer_select = (double)OUTER_R + (double)OUTER_SELECT_MARGIN;
    outer_sticky = (double)OUTER_R + (double)OUTER_STICKY_MARGIN;
    if (dist < (double)INNER_R) return -1;
    if (dist > outer_sticky) return -1;
    if (dist > outer_select && g_sel < 0) return -1;
    {
        const double pi = 3.14159265358979323846;
        double step = 2.0 * pi / (double)NSECT;
        double ang = atan2(-dy, dx);
        double shifted = ang + pi / 2.0 + step / 2.0;
        while (shifted < 0) shifted += 2.0 * pi;
        while (shifted >= 2.0 * pi) shifted -= 2.0 * pi;
        {
            int idx = (int)floor(shifted / step);
            if (idx < 0) idx = 0;
            if (idx >= NSECT) idx = NSECT - 1;
            if (idx == 4) return idx;
            int slot = sector_to_slot(idx);
            if (slot < 0 || slot >= g_slot_count) return -1;
            return idx;
        }
    }
}

int copy_text_to_clipboard(HWND owner, const wchar_t *text) {
    size_t n;
    HGLOBAL hg;
    wchar_t *p;
    if (!text || !text[0]) return 0;
    n = wcslen(text) + 1;
    hg = GlobalAlloc(GMEM_MOVEABLE, n * sizeof(wchar_t));
    if (!hg) return 0;
    p = (wchar_t *)GlobalLock(hg);
    if (!p) { GlobalFree(hg); return 0; }
    memcpy(p, text, n * sizeof(wchar_t));
    GlobalUnlock(hg);
    g_ignore_clip = 1;
    if (!OpenClipboard(owner)) { GlobalFree(hg); g_ignore_clip = 0; return 0; }
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hg);
    CloseClipboard();
    return 1;
}

void send_ctrl_v(void) {
    INPUT in[4];
    ZeroMemory(in, sizeof(in));
    in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = VK_CONTROL;
    in[1].type = INPUT_KEYBOARD; in[1].ki.wVk = 'V';
    in[2].type = INPUT_KEYBOARD; in[2].ki.wVk = 'V'; in[2].ki.dwFlags = KEYEVENTF_KEYUP;
    in[3].type = INPUT_KEYBOARD; in[3].ki.wVk = VK_CONTROL; in[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(ARRAYSIZE(in), in, sizeof(INPUT));
}

void focus_target_window(HWND target) {
    DWORD current_thread, target_thread;
    if (!target || !IsWindow(target)) return;
    current_thread = GetCurrentThreadId();
    target_thread = GetWindowThreadProcessId(target, NULL);
    if (target_thread != 0 && target_thread != current_thread) {
        AttachThreadInput(current_thread, target_thread, TRUE);
        BringWindowToTop(target);
        SetForegroundWindow(target);
        SetActiveWindow(target);
        AttachThreadInput(current_thread, target_thread, FALSE);
    } else {
        BringWindowToTop(target);
        SetForegroundWindow(target);
        SetActiveWindow(target);
    }
}

void apply_selection_to_target(void) {
    if (g_sel < 0) return;
    if (g_sel == 4) return;
    int slot = sector_to_slot(g_sel);
    if (slot < 0 || slot >= g_slot_count) return;
    if (!copy_text_to_clipboard(g_overlay_hwnd, g_slots[slot])) return;
    if (g_auto_paste && g_target_hwnd && IsWindow(g_target_hwnd) && !is_our_window(g_target_hwnd)) {
        focus_target_window(g_target_hwnd);
        Sleep(40);
        send_ctrl_v();
    }
}

/* ─── Overlay wheel ────────────────────────────────────────────────────────── */

static const COLORREF kSectorPalette[NSECT] = {
    RGB(91, 140, 250), RGB(131, 108, 245), RGB(154, 127, 255), RGB(76, 189, 201),
    RGB(71, 162, 255), RGB(105, 136, 249), RGB(121, 93, 236), RGB(88, 196, 161)
};

void draw_wheel(HDC hdc, int w, int h) {
    RECT rc = {0, 0, w, h};
    RECT wheel_rc, center_card, brand_rect, hint_rect;
    int cx, cy;
    int old_mode, outer_r, inner_r, card_w, card_h;
    float appear = ease_out_backf(g_wheel_appear);
    float t_text = ease_out_cubicf(g_wheel_appear);
    wchar_t preview[CW_MAX_CHARS];
    const double pi = 3.14159265358979323846;
    double step = 2.0 * pi / (double)NSECT;

    /* CRITICAL: Clear any existing clipping region from the HDC immediately.
       In Floating Mode or during certain OS-triggered partial repaints, the HDC
       may carry a stale clipping region that cuts off sectors (like the bottom-left). */
    SelectClipRgn(hdc, NULL);

    if (g_floating_mode) {
        cx = w / 2;
        cy = h / 2;
    } else {
        cx = g_wheel_center_screen.x - g_overlay_vx;
        cy = g_wheel_center_screen.y - g_overlay_vy;
        if (cx <= 0 || cx >= w || cy <= 0 || cy >= h) { cx = w / 2; cy = h / 2; }
    }

    outer_r = (int)(OUTER_R * (0.6f + 0.4f * appear));
    inner_r = (int)(INNER_R * (0.6f + 0.4f * appear));

    if (g_floating_mode) {
        HBRUSH key_br = CreateSolidBrush(OVERLAY_KEY_COLOR);
        FillRect(hdc, &rc, key_br);
        DeleteObject(key_br);
    } else {
        fill_vertical_gradient(hdc, &rc, RGB(7, 8, 14), RGB(10, 10, 14));
    }

    old_mode = SetBkMode(hdc, TRANSPARENT);

    /* 1. Draw Outer Glow Rings */
    {
        int s1 = SaveDC(hdc);
        for (int i = 4; i >= 1; --i) {
            int r = outer_r + i * 18;
            COLORREF glow = mix_color(COL_ACCENT_GLOW, COL_BG_DEEP, 0.55f + (float)i * 0.1f);
            HPEN p = CreatePen(PS_SOLID, 2, glow);
            HGDIOBJ op = SelectObject(hdc, p);
            HGDIOBJ ob = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
            SelectObject(hdc, op); SelectObject(hdc, ob);
            DeleteObject(p);
        }
        RestoreDC(hdc, s1);
    }

    /* 2. Draw Wheel Base Circle */
    wheel_rc.left = cx - outer_r; wheel_rc.top = cy - outer_r;
    wheel_rc.right = cx + outer_r; wheel_rc.bottom = cy + outer_r;
    fill_round_gradient(hdc, &wheel_rc, RGB(28, 30, 40), RGB(16, 17, 25), outer_r);
    draw_round_border(hdc, &wheel_rc, mix_color(COL_WHEEL_BORDER, COL_ACCENT_HOVER, 0.35f), outer_r, 2);

    /* 3. Draw Sectors (Pie) - Use a fresh clip state */
    {
        int s2 = SaveDC(hdc);
        SelectClipRgn(hdc, NULL); /* Force clear before Pie loop */
        for (int i = 0; i < NSECT; ++i) {
            int slot = sector_to_slot(i);
            int active = (slot >= 0 && slot < g_slot_count);
            float heat = g_sector_heat[i];
            COLORREF base = kSectorPalette[i];
            COLORREF fill, border;
            
            /* Enhanced color mixing: ensure min brightness */
            if (i == 4) {
                fill = mix_color(RGB(100, 30, 30), RGB(220, 50, 50), 0.30f + heat * 0.55f);
            } else {
                fill = active ? mix_color(mix_color(COL_WHEEL_SECTOR, base, 0.25f), base, 0.40f + heat * 0.50f)
                              : mix_color(RGB(32, 34, 46), RGB(24, 26, 36), 0.5f);
            }
            border = mix_color(COL_WHEEL_BORDER, COL_ACCENT_GLOW, heat * 0.75f);
            if (i == 4) border = mix_color(RGB(180, 50, 50), RGB(255, 120, 120), heat * 0.75f);

            HBRUSH br = CreateSolidBrush(fill);
            HPEN pen = CreatePen(PS_SOLID, (active || i == 4) ? 1 : 0, border);
            HGDIOBJ ob = SelectObject(hdc, br);
            HGDIOBJ op = SelectObject(hdc, pen);
            double t0 = -pi / 2.0 + i * step - step / 2.0;
            double t1 = t0 + step;
            Pie(hdc, cx - outer_r, cy - outer_r, cx + outer_r, cy + outer_r,
                cx + (int)(outer_r * cos(t0)), cy - (int)(outer_r * sin(t0)),
                cx + (int)(outer_r * cos(t1)), cy - (int)(outer_r * sin(t1)));
            SelectObject(hdc, ob); SelectObject(hdc, op);
            DeleteObject(br); DeleteObject(pen);
        }
        RestoreDC(hdc, s2);
    }

    /* 4. Draw Inner Hub and Center Card */
    {
        int s3 = SaveDC(hdc);
        RECT hub = {cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r};
        fill_round_gradient(hdc, &hub, RGB(36, 38, 52), RGB(23, 24, 34), inner_r);
        draw_round_border(hdc, &hub, mix_color(COL_ACCENT, COL_ACCENT_GLOW, 0.35f), inner_r, 2);

        card_w = (int)(360.0f * (0.6f + 0.4f * appear));
        card_h = (int)(122.0f * (0.6f + 0.4f * appear));
        center_card.left = cx - card_w / 2; center_card.top = cy - card_h / 2;
        center_card.right = center_card.left + card_w;
        center_card.bottom = center_card.top + card_h;
        fill_round_gradient(hdc, &center_card, RGB(33, 35, 48), RGB(22, 24, 33), 24);
        draw_round_border(hdc, &center_card, mix_color(COL_ACCENT, COL_ACCENT_GLOW, 0.45f), 24, 2);

        SelectObject(hdc, g_font_body);
        SetTextColor(hdc, mix_color(COL_TEXT_SECONDARY, COL_TEXT_PRIMARY, 0.35f + 0.65f * t_text));
        if (g_sel == 4) {
            SetTextColor(hdc, RGB(255, 120, 120));
            DrawTextW(hdc, L"松开取消", -1, &center_card, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        } else {
            int sel_slot = sector_to_slot(g_sel);
            if (g_slot_count == 0) {
                DrawTextW(hdc, L"还没有可用内容", -1, &center_card, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            } else if (sel_slot >= 0 && sel_slot < g_slot_count) {
                truncate_preview(preview, ARRAYSIZE(preview), g_slots[sel_slot], 88);
                DrawTextW(hdc, preview, -1, &center_card, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
            } else {
                DrawTextW(hdc, L"移动鼠标选择，松开热键粘贴", -1, &center_card, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
        }
        RestoreDC(hdc, s3);
    }

    /* 5. Draw Labels and Branding */
    {
        int s4 = SaveDC(hdc);
        SelectObject(hdc, g_font_caption);
        for (int i = 0; i < NSECT; ++i) {
            int slot = sector_to_slot(i);
            double mid = -pi / 2.0 + i * step;
            int mx = cx + (int)((inner_r + 96) * cos(mid));
            int my = cy - (int)((inner_r + 96) * sin(mid));
            RECT label_rc = {mx - 78, my - 16, mx + 78, my + 16};
            if (i == 4) {
                SetTextColor(hdc, mix_color(RGB(200, 80, 80), RGB(255, 130, 130), g_sector_heat[i]));
                DrawTextW(hdc, L"\u2715 \u53d6\u6d88", -1, &label_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            } else if (slot >= 0 && slot < g_slot_count) {
                truncate_preview(preview, ARRAYSIZE(preview), g_slots[slot], 26);
                if (i == g_sel) {
                    RECT chip = {mx - 72, my - 14, mx + 72, my + 14};
                    fill_round_gradient(hdc, &chip, RGB(102, 112, 250), RGB(84, 95, 232), 12);
                    draw_round_border(hdc, &chip, COL_ACCENT_GLOW, 12, 1);
                    SetTextColor(hdc, COL_WHITE);
                } else {
                    SetTextColor(hdc, mix_color(COL_TEXT_TERTIARY, COL_TEXT_SECONDARY, 0.45f + 0.55f * g_sector_heat[i]));
                }
                DrawTextW(hdc, preview, -1, &label_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            }
        }

        brand_rect.left = cx - 128; brand_rect.top = cy - outer_r - 72;
        brand_rect.right = cx + 128; brand_rect.bottom = cy - outer_r - 30;
        fill_round_gradient(hdc, &brand_rect, RGB(33, 34, 47), RGB(21, 22, 30), 14);
        draw_round_border(hdc, &brand_rect, mix_color(COL_ACCENT, COL_ACCENT_GLOW, 0.40f), 14, 1);
        SetTextColor(hdc, mix_color(COL_ACCENT_GLOW, COL_WHITE, 0.35f));
        SelectObject(hdc, g_font_title);
        DrawTextW(hdc, L"ClipWheel", -1, &brand_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        hint_rect.left = cx - 180; hint_rect.top = cy + outer_r + 24;
        hint_rect.right = cx + 180; hint_rect.bottom = cy + outer_r + 52;
        /* For the hint text, ensure the text background is rendered nicely to avoid pink edges */
        SetBkColor(hdc, RGB(12, 12, 18)); /* Dark background fallback for anti-aliasing */
        SetTextColor(hdc, mix_color(COL_TEXT_TERTIARY, COL_TEXT_SECONDARY, t_text));
        SelectObject(hdc, g_font_caption);
        DrawTextW(hdc, L"ESC \u6216\u6ed1\u81f3\u5e95\u90e8\u53d6\u6d88 \xb7 \u677e\u5f00\u7c98\u8d34 \xb7 \u2190\u2191\u2193\u2192\u9009\u6247\u533a", -1, &hint_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        RestoreDC(hdc, s4);
    }

    SetBkMode(hdc, old_mode);
}

void paint_overlay(void) {
    PAINTSTRUCT ps; RECT rc;
    HDC hdc = BeginPaint(g_overlay_hwnd, &ps);
    /* CRITICAL FIX: Create memory DC and Bitmap compatible with the SCREEN, 
       not the window DC. If the window is partially off-screen (top 1/3rd),
       the window DC might have a clipping region that corrupts the memory DC. */
    HDC hdc_screen = GetDC(NULL);
    HDC mem = CreateCompatibleDC(hdc_screen);
    HBITMAP bmp; HBITMAP old_bmp;
    GetClientRect(g_overlay_hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    bmp = CreateCompatibleBitmap(hdc_screen, w, h);
    old_bmp = (HBITMAP)SelectObject(mem, bmp);
    
    draw_wheel(mem, w, h);
    
    /* Copy the fully rendered buffer to the window */
    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    
    SelectObject(mem, old_bmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc_screen);
    EndPaint(g_overlay_hwnd, &ps);
}

void show_wheel(void) {
    int vx, vy, vw, vh, cx, cy, ovx, ovy, ovw, ovh;
    HMONITOR mon; MONITORINFO mi; POINT verify;
    HWND foreground = GetForegroundWindow();
    if (g_wheel_visible) return;
    g_target_hwnd = is_our_window(foreground) ? NULL : foreground;

    GetCursorPos(&g_original_cursor_pos);
    build_slots();
    g_wheel_visible = 1;
    g_sel = -1;
    g_wheel_appear = 0.0f;
    ZeroMemory(g_sector_heat, sizeof(g_sector_heat));
    vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    cx = vx + vw / 2; cy = vy + vh / 2;
    mon = MonitorFromPoint(g_original_cursor_pos, MONITOR_DEFAULTTONEAREST);
    ZeroMemory(&mi, sizeof(mi)); mi.cbSize = sizeof(mi);
    if (mon && GetMonitorInfoW(mon, &mi)) {
        cx = (mi.rcMonitor.left + mi.rcMonitor.right) / 2;
        cy = (mi.rcMonitor.top + mi.rcMonitor.bottom) / 2;
    }
    if (g_floating_mode) { cx = g_original_cursor_pos.x; cy = g_original_cursor_pos.y; }
    g_wheel_center_screen.x = cx; g_wheel_center_screen.y = cy;

    if (g_floating_mode) {
        ovw = FLOATING_OVERLAY_W; ovh = FLOATING_OVERLAY_H;
        ovx = cx - ovw / 2; ovy = cy - ovh / 2;

        /* Clamp overlay to current monitor so it never extends off-screen.
           If the overlay is partially off-screen, BeginPaint clips the DC
           to the visible region, which causes GDI Pie/Ellipse calls to be
           culled — producing the black/missing sectors. */
        {
            RECT mr = mi.rcMonitor;
            if (ovx < mr.left)  ovx = mr.left;
            if (ovy < mr.top)   ovy = mr.top;
            if (ovx + ovw > mr.right)  ovx = mr.right - ovw;
            if (ovy + ovh > mr.bottom) ovy = mr.bottom - ovh;
        }

        /* Recompute wheel center from the clamped overlay position.
           This ensures the wheel is always at the overlay center (cx in
           draw_wheel = ovw/2), which keeps the cursor aligned with the
           center card rather than drifting off to the edge. */
        g_wheel_center_screen.x = ovx + ovw / 2;
        g_wheel_center_screen.y = ovy + ovh / 2;
    } else {
        ovx = vx - 200; ovy = vy - 200;
        ovw = vw + 400; ovh = vh + 400;
    }

    SetWindowPos(g_overlay_hwnd, HWND_TOPMOST, ovx, ovy, ovw, ovh, SWP_SHOWWINDOW);

    /* Read the CLIENT area origin — this is where GDI drawing starts.
       Using GetWindowRect can differ by invisible borders on WS_POPUP
       layered windows, causing draw_wheel's coordinate conversion to drift. */
    {
        POINT origin = {0, 0};
        ClientToScreen(g_overlay_hwnd, &origin);
        g_overlay_vx = origin.x;
        g_overlay_vy = origin.y;
        RECT crc;
        GetClientRect(g_overlay_hwnd, &crc);
        ovw = crc.right;
        ovh = crc.bottom;
    }

    /* Clamp wheel center so full visual fits inside overlay (fullscreen only).
       Above: glow(72) + brand_gap(72) + brand_h(42) = ~186, round up to OUTER_R+86=336.
       Below: glow(72) + hint_gap(52) = ~124, round up to OUTER_R+54=304. */
    if (!g_floating_mode) {
        int margin_x = OUTER_R + 54;
        int margin_top = OUTER_R + 86;
        int margin_bot = OUTER_R + 54;
        int lx = g_wheel_center_screen.x - g_overlay_vx;
        int ly = g_wheel_center_screen.y - g_overlay_vy;
        if (lx < margin_x)           g_wheel_center_screen.x = g_overlay_vx + margin_x;
        if (lx > ovw - margin_x)     g_wheel_center_screen.x = g_overlay_vx + ovw - margin_x;
        if (ly < margin_top)         g_wheel_center_screen.y = g_overlay_vy + margin_top;
        if (ly > ovh - margin_bot)   g_wheel_center_screen.y = g_overlay_vy + ovh - margin_bot;
    }

    /* Sync local cx/cy with the (possibly clamped) wheel center.
       draw_wheel and SetCursorPos both use these values. */
    cx = g_wheel_center_screen.x;
    cy = g_wheel_center_screen.y;

    ShowWindow(g_overlay_hwnd, SW_SHOW);
    SetForegroundWindow(g_overlay_hwnd);
    SetLayeredWindowAttributes(g_overlay_hwnd, g_floating_mode ? OVERLAY_KEY_COLOR : 0, 0,
                                g_floating_mode ? (LWA_ALPHA | LWA_COLORKEY) : LWA_ALPHA);

    if (!g_floating_mode) {
        SetCursorPos(cx, cy);
        GetCursorPos(&verify);
        if (abs(verify.x - cx) > 1 || abs(verify.y - cy) > 1) SetCursorPos(cx, cy);
    }
    RedrawWindow(g_overlay_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

void hide_wheel_commit(void) {
    if (!g_wheel_visible) return;
    if (g_sel == SEL_CANCEL_ZONE) { hide_wheel_cancel(); return; }
    g_wheel_visible = 0;
    g_wheel_appear = 1.0f;
    g_cancel_heat = 0.0f;
    ShowWindow(g_overlay_hwnd, SW_HIDE);
    SetCursorPos(g_original_cursor_pos.x, g_original_cursor_pos.y);
    apply_selection_to_target();
    g_sel = -1;
}

void hide_wheel_cancel(void) {
    if (!g_wheel_visible) return;
    g_wheel_visible = 0;
    g_wheel_appear = 1.0f;
    g_cancel_heat = 0.0f;
    g_sel = -1;
    g_hotkey_suppress = 1;
    ShowWindow(g_overlay_hwnd, SW_HIDE);
    SetCursorPos(g_original_cursor_pos.x, g_original_cursor_pos.y);
}

/* ─── Clipboard / Tray ─────────────────────────────────────────────────────── */

void sync_clipboard_now(HWND hwnd) {
    if (!OpenClipboard(hwnd)) return;
    HANDLE h = GetClipboardData(CF_UNICODETEXT);
    if (h) {
        const wchar_t *s = (const wchar_t *)GlobalLock(h);
        if (s) {
            wchar_t tmp[CW_MAX_CHARS];
            wcsncpy_s(tmp, ARRAYSIZE(tmp), s, _TRUNCATE);
            GlobalUnlock(h);
            cw_history_on_copy(tmp);
        }
    }
    CloseClipboard();
}

void tray_add(void) {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_overlay_hwnd;
    g_nid.uID = TRAY_UID;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    g_nid.uCallbackMessage = WM_TRAY;
    g_nid.hIcon = (HICON)LoadImageW(g_hinst, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    wcscpy_s(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"ClipWheel \u526a\u8d34\u677f\u8f6e\u76d8");
    if (Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        g_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
        g_tray_added = 1;
    }
}

void tray_remove(void) {
    if (g_tray_added) { Shell_NotifyIconW(NIM_DELETE, &g_nid); g_tray_added = 0; }
}

void tray_show_balloon(const wchar_t *title, const wchar_t *msg) {
    NOTIFYICONDATAW tip = g_nid;
    if (!g_tray_added) return;
    tip.uFlags = NIF_INFO;
    wcsncpy_s(tip.szInfoTitle, ARRAYSIZE(tip.szInfoTitle), title ? title : L"ClipWheel", _TRUNCATE);
    wcsncpy_s(tip.szInfo, ARRAYSIZE(tip.szInfo), msg ? msg : L"", _TRUNCATE);
    tip.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY, &tip);
}

void show_about(HWND owner) {
    MessageBoxW(owner,
                L"ClipWheel " CLIPWHEEL_VERSION
                L"\n\n\u6309\u4f4f\u4f60\u8bbe\u7f6e\u7684\u70ed\u952e\u5373\u53ef\u547c\u51fa\u526a\u8d34\u677f\u8f6e\u76d8\u3002"
                L"\n\u5c06\u9f20\u6807\u79fb\u52a8\u5230\u76ee\u6807\u5206\u533a\u540e\u677e\u5f00\uff0c\u53ef\u81ea\u52a8\u56de\u5230\u4e0a\u4e00\u4e2a\u7a97\u53e3\u5e76\u7c98\u8d34\u3002"
                L"\n\u6309 Esc \u4f1a\u53d6\u6d88\u672c\u6b21\u64cd\u4f5c\uff0c\u4e0d\u4fee\u6539\u526a\u8d34\u677f\u3002",
                L"\u5173\u4e8e ClipWheel", MB_ICONINFORMATION);
}

void open_config_file(void) {
    ShellExecuteW(NULL, L"open", g_ini, NULL, NULL, SW_SHOWNORMAL);
}

/* ─── Undo ─────────────────────────────────────────────────────────────────── */

void push_undo(int action, int index, const wchar_t *text) {
    if (g_undo_count >= UNDO_STACK_SIZE) {
        for (int i = 0; i < UNDO_STACK_SIZE - 1; i++)
            g_undo_stack[i] = g_undo_stack[i + 1];
        g_undo_count = UNDO_STACK_SIZE - 1;
    }
    g_undo_stack[g_undo_count].action = action;
    g_undo_stack[g_undo_count].index  = index;
    wcsncpy_s(g_undo_stack[g_undo_count].text, CW_MAX_CHARS, text ? text : L"", _TRUNCATE);
    g_undo_count++;
}

int pop_undo(int *action, int *index, wchar_t *out, size_t cch_out) {
    if (g_undo_count <= 0) return 0;
    g_undo_count--;
    *action = g_undo_stack[g_undo_count].action;
    *index  = g_undo_stack[g_undo_count].index;
    if (out && cch_out) wcsncpy_s(out, cch_out, g_undo_stack[g_undo_count].text, _TRUNCATE);
    return 1;
}

void apply_undo(HWND owner) {
    int action, index;
    wchar_t text[CW_MAX_CHARS];
    if (!pop_undo(&action, &index, text, ARRAYSIZE(text))) {
        MessageBoxW(owner, L"没有可撤销的操作。", L"ClipWheel", MB_ICONINFORMATION);
        return;
    }
    wchar_t err[256];
    if (action == UNDO_ACTION_DELETE_HIST) {
        cw_history_on_copy(text);
    } else if (action == UNDO_ACTION_DELETE_PIN || action == UNDO_ACTION_UNPIN) {
        cw_history_pin_text_at(text, index, err, ARRAYSIZE(err));
    }
    refresh_manager_ui();
}

/* ─── Manager UI ───────────────────────────────────────────────────────────── */

void refresh_manager_lists(void) {
    if (g_pin_list_hwnd) { card_list_sync_scroll(g_pin_list_hwnd); InvalidateRect(g_pin_list_hwnd, NULL, TRUE); }
    if (g_history_list_hwnd) { card_list_sync_scroll(g_history_list_hwnd); InvalidateRect(g_history_list_hwnd, NULL, TRUE); }
    if (g_wheel_preview_hwnd) InvalidateRect(g_wheel_preview_hwnd, NULL, FALSE);
}

void sync_settings_controls(void) {
    if (!g_main_hwnd) return;
    if (g_record_btn_hwnd && !g_recording)
        SetWindowTextW(g_record_btn_hwnd, L"点击开始录制热键");
    if (g_undo_btn_hwnd)
        EnableWindow(g_undo_btn_hwnd, g_undo_count > 0);
}

void refresh_manager_ui(void) {
    refresh_manager_lists();
    sync_settings_controls();
    if (g_auto_paste_hwnd) SendMessageW(g_auto_paste_hwnd, BM_SETCHECK, g_auto_paste ? BST_CHECKED : BST_UNCHECKED, 0);
    if (g_floating_mode_hwnd) SendMessageW(g_floating_mode_hwnd, BM_SETCHECK, g_floating_mode ? BST_CHECKED : BST_UNCHECKED, 0);
    if (g_main_hwnd) InvalidateRect(g_main_hwnd, NULL, TRUE);
}

void copy_pin_selection(void) {
    int idx = card_list_selected_index(g_pin_list_hwnd);
    wchar_t text[CW_MAX_CHARS];
    if (!cw_history_copy_pin(idx, text, ARRAYSIZE(text))) return;
    copy_text_to_clipboard(g_main_hwnd, text);
}

void copy_history_selection(void) {
    int idx = card_list_selected_index(g_history_list_hwnd);
    wchar_t text[CW_MAX_CHARS];
    if (!cw_history_copy_history(idx, text, ARRAYSIZE(text))) return;
    copy_text_to_clipboard(g_main_hwnd, text);
}

void unpin_selected_pin(HWND owner) {
    int idx = card_list_selected_index(g_pin_list_hwnd);
    HWND msg_owner = owner ? owner : g_main_hwnd;
    if (idx < 0) {
        MessageBoxW(msg_owner, L"请先在\"固定内容\"中选中一项，再执行删除。", L"ClipWheel", MB_ICONINFORMATION);
        if (g_main_hwnd && !IsWindowVisible(g_main_hwnd)) {
            ShowWindow(g_main_hwnd, SW_SHOW);
            SetForegroundWindow(g_main_hwnd);
        }
        if (g_pin_list_hwnd) SetFocus(g_pin_list_hwnd);
        return;
    }
    wchar_t text[CW_MAX_CHARS];
    if (cw_history_copy_pin(idx, text, ARRAYSIZE(text)))
        push_undo(UNDO_ACTION_UNPIN, idx, text);
    cw_history_unpin_display_index(idx);
    refresh_manager_ui();
    if (g_pin_list_hwnd) SetFocus(g_pin_list_hwnd);
}

void start_recording(HWND hwnd) {
    g_recording = 1;
    g_recorded_vk = 0;
    g_recorded_mod = 0;
    SetWindowTextW(g_record_btn_hwnd, L"请按下热键组合...");
    SetFocus(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

void stop_recording(HWND hwnd, BOOL save) {
    g_recording = 0;
    if (save && g_recorded_vk > 0) {
        g_vk = g_recorded_vk;
        g_mod = g_recorded_mod;
        format_hotkey_label();
        save_hotkey_config();
    }
    g_recorded_vk = 0;
    g_recorded_mod = 0;
    if (g_record_btn_hwnd) SetWindowTextW(g_record_btn_hwnd, L"点击开始录制热键");
    refresh_manager_ui();
}

/* ─── Auto-start ───────────────────────────────────────────────────────────── */

static int is_auto_start_enabled(void) {
    HKEY hkey;
    wchar_t path[MAX_PATH];
    DWORD type, sz;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hkey) != ERROR_SUCCESS)
        return 0;
    sz = sizeof(path);
    if (RegQueryValueExW(hkey, L"ClipWheel", NULL, &type, (BYTE *)path, &sz) == ERROR_SUCCESS && type == REG_SZ)
        { RegCloseKey(hkey); return 1; }
    RegCloseKey(hkey);
    return 0;
}

static void set_auto_start(int enable) {
    HKEY hkey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey) != ERROR_SUCCESS)
        return;
    if (enable) {
        wchar_t exe[MAX_PATH];
        GetModuleFileNameW(NULL, exe, MAX_PATH);
        RegSetValueExW(hkey, L"ClipWheel", 0, REG_SZ, (BYTE *)exe, (DWORD)((wcslen(exe) + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hkey, L"ClipWheel");
    }
    RegCloseKey(hkey);
}

/* ─── Layout ───────────────────────────────────────────────────────────────── */

void layout_main_window(HWND hwnd) {
    RECT rc; int width, height;
    int btn_h = 34, btn_w = 110, btn_gap = 8;
    int col_pad = 12, x, y, list_top, footer_y, list_h;
    int content_left = CONTENT_PAD, content_right;
    GetClientRect(hwnd, &rc);
    width = rc.right - rc.left; height = rc.bottom - rc.top;
    content_right = width - CONTENT_PAD;

    /* Nav bar: right-aligned system buttons */
    x = content_right - btn_w;
    MoveWindow(GetDlgItem(hwnd, BTN_EXIT_APP),      x, 13, btn_w, btn_h, TRUE); x -= btn_w + btn_gap;
    MoveWindow(GetDlgItem(hwnd, BTN_HIDE_TO_TRAY),  x, 13, btn_w, btn_h, TRUE); x -= btn_w + btn_gap;
    MoveWindow(GetDlgItem(hwnd, BTN_RELOAD_CONFIG),  x, 13, btn_w, btn_h, TRUE); x -= btn_w + btn_gap;
    MoveWindow(GetDlgItem(hwnd, BTN_OPEN_CONFIG),   x, 13, btn_w, btn_h, TRUE);
    MoveWindow(GetDlgItem(hwnd, BTN_UNDO), -200, -200, 1, 1, FALSE);

    /* Quick action card */
    y = NAV_H + SECTION_GAP;
    MoveWindow(GetDlgItem(hwnd, BTN_PIN_CLIPBOARD), content_left,       y + 13, 160, 36, TRUE);
    MoveWindow(GetDlgItem(hwnd, BTN_CLEAR_PINS),    content_left + 172, y + 13, 140, 36, TRUE);
    MoveWindow(g_undo_btn_hwnd,                     content_left + 324, y + 13, 110, 36, TRUE);

    /* Settings row */
    y += QUICK_CARD_H + SECTION_GAP;
    MoveWindow(g_auto_paste_hwnd,    content_left + 16,  y + 12, 260, 26, TRUE);
    MoveWindow(g_floating_mode_hwnd, content_left + 290, y + 12, 240, 26, TRUE);
    MoveWindow(g_auto_start_hwnd,    content_left + 16,  y + 42, 260, 26, TRUE);
    MoveWindow(g_record_btn_hwnd,    content_right - 220, y + 20, 220, 38, TRUE);

    /* 3-column content area */
    y += SETTINGS_H + SECTION_GAP;
    {
        int total_w  = content_right - content_left;
        int cg = 12;
        /* Ensure minimum center width by clamping side widths */
        if (g_side_w_left + g_side_w_right + 2 * cg > total_w - MIN_CENTER_W) {
             /* Basic balancing if total width is too small */
             float ratio = (float)g_side_w_left / (g_side_w_left + g_side_w_right);
             int available = total_w - MIN_CENTER_W - 2 * cg;
             if (available < MIN_SIDE_W * 2) available = MIN_SIDE_W * 2;
             g_side_w_left = (int)(available * ratio);
             g_side_w_right = available - g_side_w_left;
        }
        
        int center_w = total_w - g_side_w_left - g_side_w_right - 2 * cg;
        int pin_left     = content_left;
        int preview_left = pin_left + g_side_w_left + cg;
        int hist_left    = preview_left + center_w + cg;
        
        list_top = y + 76;
        footer_y = height - CONTENT_PAD - 40;
        list_h   = footer_y - 8 - list_top;
        if (list_h < 120) list_h = 120;

        MoveWindow(g_pin_list_hwnd,  pin_left + col_pad, list_top, g_side_w_left - col_pad * 2, list_h, TRUE);
        MoveWindow(GetDlgItem(hwnd, BTN_COPY_PIN), pin_left + 8,   footer_y, 104, 36, TRUE);
        MoveWindow(GetDlgItem(hwnd, BTN_UNPIN),    pin_left + 120, footer_y, 120, 36, TRUE);

        if (g_wheel_preview_hwnd)
            MoveWindow(g_wheel_preview_hwnd, preview_left, y, center_w, footer_y + 36 - y, TRUE);

        MoveWindow(g_history_list_hwnd, hist_left + col_pad, list_top, g_side_w_right - col_pad * 2, list_h, TRUE);
        MoveWindow(GetDlgItem(hwnd, BTN_COPY_HISTORY),   hist_left + 8,   footer_y, 100, 36, TRUE);
        MoveWindow(GetDlgItem(hwnd, BTN_PIN_HISTORY),    hist_left + 116, footer_y, 100, 36, TRUE);
        MoveWindow(GetDlgItem(hwnd, BTN_DELETE_HISTORY), hist_left + 224, footer_y, 100, 36, TRUE);
    }
}

static void add_list_strings(HWND list_hwnd, int (*copy_fn)(int, wchar_t *, size_t), int count) {
    for (int i = 0; i < count; i++) {
        wchar_t text[CW_MAX_CHARS], line[CW_MAX_CHARS + 16];
        if (!copy_fn(i, text, ARRAYSIZE(text))) continue;
        swprintf_s(line, ARRAYSIZE(line), L"%d. %s", i + 1, text);
        SendMessageW(list_hwnd, LB_ADDSTRING, 0, (LPARAM)line);
    }
}

void draw_main_window_background(HDC hdc, const RECT *rc) {
    fill_vertical_gradient(hdc, rc, RGB(8, 10, 16), RGB(11, 11, 16));
    HBRUSH blob_a = CreateSolidBrush(RGB(21, 26, 45));
    HBRUSH blob_b = CreateSolidBrush(RGB(16, 18, 34));
    HGDIOBJ old_br = SelectObject(hdc, blob_a);
    HGDIOBJ old_pen = SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, rc->right - 420, -120, rc->right + 120, 290);
    SelectObject(hdc, blob_b);
    Ellipse(hdc, -200, rc->bottom - 360, 340, rc->bottom + 120);
    SelectObject(hdc, old_br); SelectObject(hdc, old_pen);
    DeleteObject(blob_a); DeleteObject(blob_b);
}

void paint_main_window(HWND hwnd) {
    PAINTSTRUCT ps; RECT rc, nav, card, left_card, right_card, badge, line;
    HDC hdc_screen = BeginPaint(hwnd, &ps);
    int width, height, y;
    GetClientRect(hwnd, &rc);
    width = rc.right - rc.left; height = rc.bottom - rc.top;
    HDC hdc = CreateCompatibleDC(hdc_screen);
    HBITMAP bmp = CreateCompatibleBitmap(hdc_screen, width, height);
    HBITMAP old_bmp = (HBITMAP)SelectObject(hdc, bmp);

    int total_w  = width - 2 * CONTENT_PAD;
    int cg = 12;
    int center_w = total_w - g_side_w_left - g_side_w_right - 2 * cg;
    int right_x  = CONTENT_PAD + g_side_w_left + cg + center_w + cg;

    float pulse = 0.5f + 0.5f * (float)sin(g_main_phase);
    COLORREF accent_live = mix_color(COL_ACCENT, COL_ACCENT_HOVER, pulse * 0.85f);

    /* Background */
    fill_vertical_gradient(hdc, &rc, RGB(8, 10, 16), RGB(11, 11, 16));
    { HBRUSH ba = CreateSolidBrush(RGB(21,26,45)), bb = CreateSolidBrush(RGB(16,18,34));
      HGDIOBJ obr = SelectObject(hdc, ba), opn = SelectObject(hdc, GetStockObject(NULL_PEN));
      Ellipse(hdc, width-380,-80,width+80,240); SelectObject(hdc, bb);
      Ellipse(hdc,-160,height-300,300,height+100);
      SelectObject(hdc,obr); SelectObject(hdc,opn); DeleteObject(ba); DeleteObject(bb); }

    /* Compact nav bar */
    nav.left=0; nav.top=0; nav.right=width; nav.bottom=NAV_H;
    fill_vertical_gradient(hdc, &nav, RGB(20,22,34), RGB(14,15,24));
    { HPEN lp = CreatePen(PS_SOLID, 2, accent_live); HGDIOBJ op = SelectObject(hdc, lp);
      MoveToEx(hdc, 0, NAV_H-1, NULL); LineTo(hdc, 80+(int)(50*pulse), NAV_H-1);
      SelectObject(hdc,op); DeleteObject(lp); }
    { HPEN dp = CreatePen(PS_SOLID, 1, mix_color(COL_BORDER_SUBTLE,COL_ACCENT,0.15f));
      HGDIOBJ op = SelectObject(hdc,dp);
      MoveToEx(hdc,0,NAV_H-1,NULL); LineTo(hdc,width,NAV_H-1);
      SelectObject(hdc,op); DeleteObject(dp); }
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, g_font_body_bold); SetTextColor(hdc, COL_TEXT_PRIMARY);
    line.left=CONTENT_PAD+8; line.top=0; line.right=CONTENT_PAD+160; line.bottom=NAV_H;
    DrawTextW(hdc, L"ClipWheel", -1, &line, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    SelectObject(hdc, g_font_caption); SetTextColor(hdc, COL_TEXT_TERTIARY);
    line.left=CONTENT_PAD+118;
    DrawTextW(hdc, L"控制中心", -1, &line, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    /* Hotkey pill in nav */
    { RECT hk={CONTENT_PAD+220,10,CONTENT_PAD+470,NAV_H-10};
      fill_round_gradient(hdc,&hk,RGB(34,36,52),RGB(24,26,40),10);
      draw_round_border(hdc,&hk,mix_color(COL_BORDER_SUBTLE,COL_ACCENT,0.3f),10,1);
      SelectObject(hdc,g_font_caption);
      SetTextColor(hdc,mix_color(COL_ACCENT_GLOW,COL_WHITE,0.3f+0.4f*pulse));
      RECT hki={hk.left+10,hk.top,hk.right-6,hk.bottom};
      wchar_t hkl[96]; swprintf_s(hkl,ARRAYSIZE(hkl),L"热键: %s",g_hotkey_label);
      DrawTextW(hdc,hkl,-1,&hki,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX); }

    /* Quick action card */
    y = NAV_H + SECTION_GAP;
    card.left=CONTENT_PAD; card.top=y;
    card.right=width-CONTENT_PAD; card.bottom=y+QUICK_CARD_H;
    fill_round_gradient(hdc,&card,RGB(48,51,68),RGB(36,38,54),12);
    draw_round_border(hdc,&card,mix_color(COL_BORDER_SUBTLE,COL_ACCENT_GLOW,0.2f),12,1);
    SelectObject(hdc,g_font_caption); SetTextColor(hdc,COL_TEXT_TERTIARY);
    line.left=card.left+16; line.top=y; line.right=card.left+160; line.bottom=y+QUICK_CARD_H;
    DrawTextW(hdc,L"快速操作",-1,&line,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    { RECT pill={width-CONTENT_PAD-210,y+14,width-CONTENT_PAD-16,y+QUICK_CARD_H-14};
      COLORREF pc=g_auto_paste?RGB(28,70,52):RGB(58,28,30);
      COLORREF pb=g_auto_paste?RGB(52,211,153):RGB(248,113,113);
      fill_round_gradient(hdc,&pill,pc,mix_color(pc,RGB(0,0,0),0.3f),10);
      draw_round_border(hdc,&pill,pb,10,1); SetTextColor(hdc,pb);
      SelectObject(hdc,g_font_caption);
      DrawTextW(hdc,g_auto_paste?L"\u81ea\u52a8\u7c98\u8d34 \u2713":L"\u81ea\u52a8\u7c98\u8d34 \u2717",
                -1,&pill,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX); }

    /* Settings card */
    y += QUICK_CARD_H + SECTION_GAP;
    card.top=y; card.bottom=y+SETTINGS_H;
    fill_round_gradient(hdc,&card,RGB(42,44,60),RGB(30,32,48),12);
    draw_round_border(hdc,&card,mix_color(COL_BORDER_SUBTLE,COL_ACCENT,0.18f),12,1);
    
    SelectObject(hdc,g_font_title); SetTextColor(hdc,COL_TEXT_PRIMARY);
    line.left=card.left+16; line.top=y+10; line.right=card.left+220; line.bottom=y+40;
    DrawTextW(hdc,L"\u70ed\u952e\u4e0e\u884c\u4e3a",-1,&line,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);

    if (g_recording) {
        SelectObject(hdc,g_font_caption); SetTextColor(hdc,accent_live);
        line.left=card.right-380; line.right=card.right-16; line.top=y+10; line.bottom=y+40;
        DrawTextW(hdc,L"\u25cf \u6b63\u5728\u5f55\u5236\u2026 \u8bf7\u6309\u4e0b\u70ed\u952e\u7ec4\u5408",-1,&line,DT_RIGHT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    } else {
        wchar_t cur[128]; swprintf_s(cur,ARRAYSIZE(cur),L"\u5f53\u524d: %s",g_hotkey_label);
        SelectObject(hdc,g_font_caption); SetTextColor(hdc,COL_TEXT_SECONDARY);
        line.left=card.right-320; line.right=card.right-16; line.top=y+10; line.bottom=y+40;
        DrawTextW(hdc,cur,-1,&line,DT_RIGHT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    }

    /* 3-column cards */
    y += SETTINGS_H + SECTION_GAP;
    left_card.left=CONTENT_PAD; left_card.top=y;
    left_card.right=CONTENT_PAD+g_side_w_left; left_card.bottom=height-CONTENT_PAD;
    right_card.left=right_x; right_card.top=y;
    right_card.right=width-CONTENT_PAD; right_card.bottom=height-CONTENT_PAD;
    fill_round_gradient(hdc,&left_card,RGB(36,38,54),RGB(24,26,40),14);
    fill_round_gradient(hdc,&right_card,RGB(36,38,54),RGB(24,26,40),14);
    draw_round_border(hdc,&left_card,mix_color(COL_BORDER_SUBTLE,COL_ACCENT,0.14f),14,1);
    draw_round_border(hdc,&right_card,mix_color(COL_BORDER_SUBTLE,COL_ACCENT,0.14f),14,1);

    SelectObject(hdc, g_font_title); SetTextColor(hdc, COL_TEXT_PRIMARY);
    line.left=left_card.left+14; line.top=y+14; line.right=left_card.right-100; line.bottom=y+44;
    DrawTextW(hdc,L"\u56fa\u5b9a\u5185\u5bb9",-1,&line,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    badge.left=left_card.right-94; badge.top=y+16; badge.right=left_card.right-10; badge.bottom=y+42;
    fill_round_gradient(hdc,&badge,RGB(108,116,250),RGB(89,97,231),10);
    draw_round_border(hdc,&badge,COL_ACCENT_GLOW,10,1);
    SetTextColor(hdc,COL_WHITE); SelectObject(hdc,g_font_caption);
    DrawTextW(hdc,L"\u8f6e\u76d8\u4f18\u5148",-1,&badge,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(hdc,g_font_caption); SetTextColor(hdc,COL_TEXT_TERTIARY);
    line.top=y+46; line.bottom=y+68; line.right=left_card.right-10;
    DrawTextW(hdc,L"\u56fa\u5b9a\u9879\u4f18\u5148\u586b\u5145\u8f6e\u76d8\u6d4e\u533a",-1,&line,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);

    SelectObject(hdc,g_font_title); SetTextColor(hdc,COL_TEXT_PRIMARY);
    line.left=right_card.left+14; line.top=y+14; line.right=right_card.right-100; line.bottom=y+44;
    DrawTextW(hdc,L"\u6700\u8fd1\u5386\u53f2",-1,&line,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
    badge.left=right_card.right-94; badge.top=y+16; badge.right=right_card.right-10; badge.bottom=y+42;
    fill_round_gradient(hdc,&badge,RGB(108,116,250),RGB(89,97,231),10);
    draw_round_border(hdc,&badge,COL_ACCENT_GLOW,10,1);
    SetTextColor(hdc,COL_WHITE); SelectObject(hdc,g_font_caption);
    DrawTextW(hdc,L"\u81ea\u52a8\u8bb0\u5f55",-1,&badge,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    SelectObject(hdc,g_font_caption); SetTextColor(hdc,COL_TEXT_TERTIARY);
    line.left=right_card.left+14; line.top=y+46; line.bottom=y+68; line.right=right_card.right-10;
    DrawTextW(hdc,L"\u62d6\u5165\u4e2d\u95f4\u8f6e\u76d8\u53ef\u56fa\u5b9a",-1,&line,DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);

    /* Draw splitters */
    {
        HPEN split_pen = CreatePen(PS_SOLID, 1, mix_color(COL_BORDER_SUBTLE, COL_ACCENT, 0.4f));
        HGDIOBJ op = SelectObject(hdc, split_pen);
        int sy = y, sh = height - CONTENT_PAD - y;
        int sx1 = CONTENT_PAD + g_side_w_left + cg / 2;
        int sx2 = right_x - cg / 2;
        MoveToEx(hdc, sx1, sy + 20, NULL); LineTo(hdc, sx1, sy + sh - 20);
        MoveToEx(hdc, sx2, sy + 20, NULL); LineTo(hdc, sx2, sy + sh - 20);
        SelectObject(hdc, op); DeleteObject(split_pen);
    }

    if (g_manager_search) {
        int sy=right_card.top+72;
        RECT sr={right_card.left+14,sy,right_card.right-14,sy+30};
        fill_round_gradient(hdc,&sr,RGB(58,60,78),RGB(46,48,64),8);
        draw_round_border(hdc,&sr,COL_ACCENT,8,1);
        SelectObject(hdc,g_font_caption); SetTextColor(hdc,COL_TEXT_PRIMARY);
        wchar_t disp[280]; swprintf_s(disp,ARRAYSIZE(disp),L"\u641c\u7d22: %s_",g_manager_search_buf);
        DrawTextW(hdc,disp,-1,&sr,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
    }

    BitBlt(hdc_screen,0,0,width,height,hdc,0,0,SRCCOPY);
    SelectObject(hdc,old_bmp); DeleteObject(bmp); DeleteDC(hdc);
    EndPaint(hwnd,&ps);
}


/* ─── Splash ───────────────────────────────────────────────────────────────── */

static LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; RECT rc, title, sub, brand_bar;
        HDC hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(COL_BG_DEEP); FillRect(hdc, &rc, bg); DeleteObject(bg);
        brand_bar.left = 0; brand_bar.top = 0; brand_bar.right = rc.right; brand_bar.bottom = 4;
        HBRUSH brand = CreateSolidBrush(COL_ACCENT); FillRect(hdc, &brand_bar, brand); DeleteObject(brand);
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, COL_TEXT_PRIMARY);
        SelectObject(hdc, g_font_display);
        title.left = 24; title.top = 20; title.right = rc.right - 24; title.bottom = 66;
        DrawTextW(hdc, L"ClipWheel", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(hdc, g_font_caption); SetTextColor(hdc, COL_TEXT_SECONDARY);
        sub.left = 24; sub.top = 70; sub.right = rc.right - 24; sub.bottom = 96;
        DrawTextW(hdc, L"启动中...", -1, &sub, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        HPEN decor = CreatePen(PS_SOLID, 2, COL_ACCENT); HGDIOBJ op = SelectObject(hdc, decor);
        MoveToEx(hdc, 24, rc.bottom - 8, NULL); LineTo(hdc, 80, rc.bottom - 8);
        SelectObject(hdc, op); DeleteObject(decor);
        EndPaint(hwnd, &ps);
        return 0;
    }}
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void show_startup_splash(HINSTANCE hi) {
    WNDCLASSEXW sc = {0}; HWND splash; RECT wa;
    int w = 360, h = 130;
    sc.cbSize = sizeof(sc); sc.lpfnWndProc = SplashProc; sc.hInstance = hi;
    sc.lpszClassName = L"ClipWheelSplash";
    sc.hCursor = LoadCursor(NULL, IDC_ARROW);
    sc.hIcon = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR);
    RegisterClassExW(&sc);
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &wa, 0);
    splash = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, sc.lpszClassName, L"ClipWheel",
                              WS_POPUP, wa.left + ((wa.right - wa.left - w) / 2),
                              wa.top + ((wa.bottom - wa.top - h) / 2), w, h, NULL, NULL, hi, NULL);
    if (splash) { ShowWindow(splash, SW_SHOW); UpdateWindow(splash); Sleep(680); DestroyWindow(splash); }
}

/* ─── Window procedures ────────────────────────────────────────────────────── */

static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        AddClipboardFormatListener(hwnd);
        SetTimer(hwnd, ID_TIMER_POLL, 16, NULL);
        SetLayeredWindowAttributes(hwnd, 0, (BYTE)242, LWA_ALPHA);
        sync_clipboard_now(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER_POLL);
        RemoveClipboardFormatListener(hwnd);
        tray_remove();
        PostQuitMessage(0);
        return 0;
    case WM_CW_SHOW: show_wheel(); return 0;
    case WM_CW_HIDE: hide_wheel_commit(); return 0;
    case WM_CW_CANCEL: hide_wheel_cancel(); return 0;
    case WM_CW_REFRESH_UI:
        refresh_manager_ui();
        if (g_wheel_visible) { build_slots(); InvalidateRect(hwnd, NULL, FALSE); }
        return 0;
    case WM_CLIPBOARDUPDATE:
        if (g_ignore_clip) { g_ignore_clip = 0; return 0; }
        sync_clipboard_now(hwnd);
        PostMessageW(hwnd, WM_CW_REFRESH_UI, 0, 0);
        return 0;
    case WM_TIMER:
        if (wp == ID_TIMER_POLL) {
            g_main_phase += 0.016f;
            if (g_wheel_visible) {
                POINT pt; RECT rc; int cx, cy, next_sel, i, need_repaint = 0, alpha_changed = 0;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                GetClientRect(hwnd, &rc);
                if (g_floating_mode) {
                    cx = (rc.right - rc.left) / 2;
                    cy = (rc.bottom - rc.top) / 2;
                } else {
                    cx = g_wheel_center_screen.x - g_overlay_vx;
                    cy = g_wheel_center_screen.y - g_overlay_vy;
                    if (cx <= 0 || cx >= rc.right || cy <= 0 || cy >= rc.bottom)
                        { cx = (rc.right - rc.left) / 2; cy = (rc.bottom - rc.top) / 2; }
                }
                next_sel = sector_from_point(cx, cy, pt.x, pt.y);
                if (next_sel < 0) {
                    int outer_r_cur = (int)(OUTER_R * (0.6f + 0.4f * ease_out_backf(g_wheel_appear)));
                    int cancel_y = cy + outer_r_cur + 74;
                    int dcx = pt.x - cx, dcy = pt.y - cancel_y;
                    if (dcx*dcx + dcy*dcy <= 26*26) next_sel = SEL_CANCEL_ZONE;
                }
                if (next_sel != g_sel) { g_sel = next_sel; need_repaint = 1; }
                { float ct = (g_sel == SEL_CANCEL_ZONE) ? 1.0f : 0.0f;
                  float prev_ch = g_cancel_heat;
                  g_cancel_heat += (ct - g_cancel_heat) * 0.22f;
                  if (fabs(g_cancel_heat - prev_ch) > 0.005f) need_repaint = 1; }
                if (g_wheel_appear < 1.0f) {
                    g_wheel_appear += 0.05f;
                    if (g_wheel_appear > 1.0f) g_wheel_appear = 1.0f;
                    need_repaint = 1;
                    SetLayeredWindowAttributes(hwnd, g_floating_mode ? OVERLAY_KEY_COLOR : 0,
                        (BYTE)((g_floating_mode ? 255.0f : 242.0f) * ease_out_cubicf(g_wheel_appear)),
                        g_floating_mode ? (LWA_ALPHA | LWA_COLORKEY) : LWA_ALPHA);
                    alpha_changed = 1;
                }
                for (i = 0; i < NSECT; ++i) {
                    float target = (i == g_sel) ? 1.0f : 0.0f;
                    float prev = g_sector_heat[i];
                    g_sector_heat[i] += (target - g_sector_heat[i]) * 0.22f;
                    if (fabs(g_sector_heat[i] - prev) > 0.01f) need_repaint = 1;
                }
                if (need_repaint || alpha_changed) {
                    if (g_floating_mode) {
                        /* Small window — just invalidate the whole thing. */
                        InvalidateRect(hwnd, NULL, FALSE);
                    } else {
                        /* Full-screen overlay: always repaint the entire window so
                           that areas outside the wheel (e.g. bottom-left corner)
                           are never left stale / black after a partial OS-triggered
                           paint. The double-buffer in paint_overlay keeps this fast. */
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
            } else if (g_main_phase > 999.0f) {
                g_main_phase = 0.0f;
            }
        }
        return 0;
    case WM_PAINT: paint_overlay(); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_TRAY:
        if (LOWORD(lp) == WM_LBUTTONDBLCLK) { ShowWindow(g_main_hwnd, SW_SHOW); SetForegroundWindow(g_main_hwnd); return 0; }
        if (LOWORD(lp) == WM_RBUTTONUP || LOWORD(lp) == WM_LBUTTONUP) {
            POINT pt; HMENU m = CreatePopupMenu();
            GetCursorPos(&pt);
            AppendMenuW(m, MF_STRING, 201, L"打开控制中心");
            AppendMenuW(m, MF_STRING, 202, L"固定当前剪贴板");
            AppendMenuW(m, MF_STRING, 206, L"删除选中固定项");
            AppendMenuW(m, MF_STRING, 203, L"重新加载配置");
            AppendMenuW(m, MF_SEPARATOR, 0, NULL);
            AppendMenuW(m, MF_STRING, 204, L"关于");
            AppendMenuW(m, MF_STRING, 205, L"退出");
            SetForegroundWindow(hwnd);
            switch (TrackPopupMenu(m, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL)) {
            case 201: ShowWindow(g_main_hwnd, SW_SHOW); SetForegroundWindow(g_main_hwnd); break;
            case 202: { wchar_t err[256]; int r = cw_history_pin_current_clipboard_ex(hwnd, err, ARRAYSIZE(err));
                if (r == 0) MessageBoxW(hwnd, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING);
                PostMessageW(hwnd, WM_CW_REFRESH_UI, 0, 0); } break;
            case 206: unpin_selected_pin(hwnd); break;
            case 203: load_config(); refresh_manager_ui(); break;
            case 204: show_about(hwnd); break;
            case 205: DestroyWindow(hwnd); break;
            } DestroyMenu(m);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_pin_list_hwnd = CreateWindowExW(0, kCardListClass, NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU)LIST_PINS, g_hinst, (LPVOID)(INT_PTR)CARD_KIND_PIN);
        g_history_list_hwnd = CreateWindowExW(0, kCardListClass, NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 0,0,0,0, hwnd, (HMENU)LIST_HISTORY, g_hinst, (LPVOID)(INT_PTR)CARD_KIND_HISTORY);
        CreateWindowW(L"BUTTON", L"打开配置", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_OPEN_CONFIG,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"重载配置", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_RELOAD_CONFIG,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"固定选中项 / 剪贴板", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_PIN_CLIPBOARD,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"隐藏到托盘", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_HIDE_TO_TRAY,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"退出应用", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_EXIT_APP,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"删除选中固定项", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_CLEAR_PINS,g_hinst,NULL);
        g_auto_paste_hwnd = CreateWindowW(L"BUTTON", L"松开后自动回到上一个窗口并粘贴",
            WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|WS_TABSTOP, 0,0,0,0,hwnd,(HMENU)CHK_AUTOPASTE,g_hinst,NULL);
        g_floating_mode_hwnd = CreateWindowW(L"BUTTON", L"浮动模式（跟随鼠标，不全屏）",
            WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|WS_TABSTOP, 0,0,0,0,hwnd,(HMENU)CHK_FLOATING_MODE,g_hinst,NULL);
        g_auto_start_hwnd = CreateWindowW(L"BUTTON", L"开机自动启动", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|WS_TABSTOP,
            0,0,0,0,hwnd,(HMENU)CHK_AUTOSTART,g_hinst,NULL);
        g_record_btn_hwnd = CreateWindowW(L"BUTTON", L"点击开始录制热键", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
            0,0,0,0,hwnd,(HMENU)BTN_RECORD_HOTKEY,g_hinst,NULL);
        g_undo_btn_hwnd = CreateWindowW(L"BUTTON", L"撤销", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW|WS_TABSTOP,
            0,0,0,0,hwnd,(HMENU)BTN_UNDO,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"复制", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_COPY_PIN,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"取消固定", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_UNPIN,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"复制", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_COPY_HISTORY,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"固定", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_PIN_HISTORY,g_hinst,NULL);
        CreateWindowW(L"BUTTON", L"删除", WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 0,0,0,0,hwnd,(HMENU)BTN_DELETE_HISTORY,g_hinst,NULL);
        g_wheel_preview_hwnd = CreateWindowExW(0, kWheelPreviewClass, NULL,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS, 0,0,0,0,hwnd,NULL,g_hinst,NULL);
        SendMessageW(g_auto_paste_hwnd, WM_SETFONT, (WPARAM)g_font_body, TRUE);
        SendMessageW(g_floating_mode_hwnd, WM_SETFONT, (WPARAM)g_font_body, TRUE);
        SendMessageW(g_auto_start_hwnd, WM_SETFONT, (WPARAM)g_font_body, TRUE);
        SendMessageW(g_record_btn_hwnd, WM_SETFONT, (WPARAM)g_font_body, TRUE);
        EnableWindow(g_undo_btn_hwnd, FALSE);
        layout_main_window(hwnd);
        refresh_manager_ui();
        SendMessageW(g_auto_start_hwnd, BM_SETCHECK, is_auto_start_enabled() ? BST_CHECKED : BST_UNCHECKED, 0);
        return 0;
    case WM_GETMINMAXINFO: { MINMAXINFO *mmi = (MINMAXINFO *)lp; mmi->ptMinTrackSize.x = 1100; mmi->ptMinTrackSize.y = 700; return 0; }
    case WM_SIZE:
        if (wp == SIZE_MINIMIZED) {
            ShowWindow(hwnd, SW_HIDE);
            if (!g_tray_tip_shown) { tray_show_balloon(L"ClipWheel", L"应用已最小化到托盘，双击托盘图标可恢复。"); g_tray_tip_shown = 1; }
        } else {
            SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
            layout_main_window(hwnd);
            SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
            RECT rc; int width, content_left = CONTENT_PAD, cg = 12;
            GetClientRect(hwnd, &rc); width = rc.right - rc.left;
            int y_start = NAV_H + SECTION_GAP + QUICK_CARD_H + SECTION_GAP + SETTINGS_H + SECTION_GAP;
            if (pt.y > y_start) {
                int x1 = content_left + g_side_w_left + cg / 2;
                int x2 = width - CONTENT_PAD - g_side_w_right - cg / 2;
                if (abs(pt.x - x1) < SPLITTER_HIT_W) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); return TRUE; }
                if (abs(pt.x - x2) < SPLITTER_HIT_W) { SetCursor(LoadCursor(NULL, IDC_SIZEWE)); return TRUE; }
            }
        }
        break;
    case WM_LBUTTONDOWN: {
        POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
        RECT rc; int width, content_left = CONTENT_PAD, cg = 12;
        GetClientRect(hwnd, &rc); width = rc.right - rc.left;
        int y_start = NAV_H + SECTION_GAP + QUICK_CARD_H + SECTION_GAP + SETTINGS_H + SECTION_GAP;
        if (pt.y > y_start) {
            int x1 = content_left + g_side_w_left + cg / 2;
            int x2 = width - CONTENT_PAD - g_side_w_right - cg / 2;
            if (abs(pt.x - x1) < SPLITTER_HIT_W) { g_dragging_splitter = 1; SetCapture(hwnd); return 0; }
            if (abs(pt.x - x2) < SPLITTER_HIT_W) { g_dragging_splitter = 2; SetCapture(hwnd); return 0; }
        }
        break;
    }
    case WM_MOUSEMOVE:
        if (g_dragging_splitter) {
            POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
            RECT rc; int width, cg = 12;
            GetClientRect(hwnd, &rc); width = rc.right - rc.left;
            int total_w = width - 2 * CONTENT_PAD;
            if (g_dragging_splitter == 1) {
                int new_w = pt.x - CONTENT_PAD - cg / 2;
                if (new_w < MIN_SIDE_W) new_w = MIN_SIDE_W;
                if (new_w > total_w - MIN_CENTER_W - g_side_w_right - 2 * cg)
                    new_w = total_w - MIN_CENTER_W - g_side_w_right - 2 * cg;
                g_side_w_left = new_w;
            }
            else if (g_dragging_splitter == 2) {
                int new_w = (width - CONTENT_PAD) - pt.x - cg / 2;
                if (new_w < MIN_SIDE_W) new_w = MIN_SIDE_W;
                if (new_w > total_w - MIN_CENTER_W - g_side_w_left - 2 * cg)
                    new_w = total_w - MIN_CENTER_W - g_side_w_left - 2 * cg;
                g_side_w_right = new_w;
            }
            layout_main_window(hwnd);
            /* Force immediate update of the wheel preview during drag */
            if (g_wheel_preview_hwnd) {
                InvalidateRect(g_wheel_preview_hwnd, NULL, FALSE);
                UpdateWindow(g_wheel_preview_hwnd);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (g_dragging_splitter) { g_dragging_splitter = 0; ReleaseCapture(); return 0; }
        break;
    case WM_DRAWITEM:
        draw_button((const DRAWITEMSTRUCT *)lp);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case BTN_UNDO: apply_undo(hwnd); return 0;
        case BTN_OPEN_CONFIG: open_config_file(); return 0;
        case BTN_RELOAD_CONFIG: load_config(); refresh_manager_ui(); return 0;
        case BTN_PIN_CLIPBOARD: {
            int idx = card_list_selected_index(g_history_list_hwnd);
            if (idx >= 0) { wchar_t text[CW_MAX_CHARS];
                if (cw_history_copy_history(idx, text, ARRAYSIZE(text))) {
                    wchar_t err[256]; int r = cw_history_pin_text_ex(text, err, ARRAYSIZE(err));
                    if (r == 0) MessageBoxW(hwnd, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING);
                    refresh_manager_ui();
                    if (r != 0) { card_list_set_selected(g_history_list_hwnd, idx + 1); SetFocus(g_history_list_hwnd); }
                } return 0; }
            wchar_t err[256]; int r = cw_history_pin_current_clipboard_ex(hwnd, err, ARRAYSIZE(err));
            if (r == 0) MessageBoxW(hwnd, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING);
            refresh_manager_ui(); return 0; }
        case BTN_RECORD_HOTKEY:
            if (!g_recording) start_recording(hwnd); else stop_recording(hwnd, FALSE); return 0;
        case BTN_CLEAR_PINS: unpin_selected_pin(hwnd); return 0;
        case BTN_HIDE_TO_TRAY: ShowWindow(hwnd, SW_HIDE);
            if (!g_tray_tip_shown) { tray_show_balloon(L"ClipWheel", L"应用已隐藏到托盘，双击托盘图标可恢复。"); g_tray_tip_shown = 1; }
            return 0;
        case BTN_EXIT_APP: DestroyWindow(g_overlay_hwnd); return 0;
        case CHK_AUTOPASTE: g_auto_paste = (SendMessageW(g_auto_paste_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
            save_behavior_config(); InvalidateRect(hwnd, NULL, TRUE); return 0;
        case CHK_FLOATING_MODE: g_floating_mode = (SendMessageW(g_floating_mode_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);
            save_behavior_config(); return 0;
        case CHK_AUTOSTART:
            set_auto_start(SendMessageW(g_auto_start_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED); return 0;
        case BTN_COPY_PIN: copy_pin_selection(); return 0;
        case BTN_UNPIN: unpin_selected_pin(hwnd); return 0;
        case BTN_COPY_HISTORY: copy_history_selection(); return 0;
        case BTN_PIN_HISTORY: {
            int idx = card_list_selected_index(g_history_list_hwnd);
            wchar_t text[CW_MAX_CHARS]; if (cw_history_copy_history(idx, text, ARRAYSIZE(text))) {
                wchar_t err[256]; int r = cw_history_pin_text_ex(text, err, ARRAYSIZE(err));
                if (r == 0) MessageBoxW(hwnd, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING);
                refresh_manager_ui();
                if (r != 0) { card_list_set_selected(g_history_list_hwnd, idx + 1); SetFocus(g_history_list_hwnd); } }
            return 0; }
        case BTN_DELETE_HISTORY: {
            int idx = card_list_selected_index(g_history_list_hwnd);
            wchar_t text[CW_MAX_CHARS];
            if (cw_history_copy_history(idx, text, ARRAYSIZE(text)))
                push_undo(UNDO_ACTION_DELETE_HIST, idx, text);
            cw_history_delete_history_index(idx);
            refresh_manager_ui(); return 0; }
        }
        break;
    case WM_KEYDOWN: {
        if (wp == 'F' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
            g_manager_search = 1; g_manager_search_buf[0] = 0;
            InvalidateRect(hwnd, NULL, TRUE); return 0;
        }
        if (g_manager_search && wp == VK_ESCAPE) {
            g_manager_search = 0; g_manager_search_buf[0] = 0;
            InvalidateRect(hwnd, NULL, TRUE); return 0;
        }
        break;
    }
    case WM_CHAR: {
        if (g_manager_search) {
            if (wp == VK_BACK) {
                size_t len = wcslen(g_manager_search_buf);
                if (len > 0) { g_manager_search_buf[len - 1] = 0; InvalidateRect(hwnd, NULL, TRUE); }
                return 0;
            }
            if (wp == VK_RETURN) {
                g_manager_search = 0;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            if (wp >= 32 && wp < 256 && wcslen(g_manager_search_buf) + 1 < ARRAYSIZE(g_manager_search_buf)) {
                size_t len = wcslen(g_manager_search_buf);
                g_manager_search_buf[len] = (wchar_t)wp;
                g_manager_search_buf[len + 1] = 0;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
        }
        break;
    }
    case WM_CTLCOLORSTATIC: case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wp; HWND ctl = (HWND)lp;
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, COL_TEXT_PRIMARY);
        if (ctl == g_auto_paste_hwnd || ctl == g_floating_mode_hwnd || ctl == g_auto_start_hwnd)
            return (LRESULT)g_brush_dark;
        return (LRESULT)g_brush_light; }
    case WM_PAINT: paint_main_window(hwnd); return 0;
    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        fill_vertical_gradient((HDC)wp, &rc, RGB(8, 10, 16), RGB(11, 11, 16));
        return 1;
    }
    case WM_CLOSE: ShowWindow(hwnd, SW_HIDE);
        if (!g_tray_tip_shown) { tray_show_balloon(L"ClipWheel", L"应用仍在后台运行，可在托盘中退出。"); g_tray_tip_shown = 1; }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    KBDLLHOOKSTRUCT *k;
    static int s_recording_mods = 0;
    if (code != HC_ACTION) return CallNextHookEx(g_kb_hook, code, wParam, lParam);
    k = (KBDLLHOOKSTRUCT *)lParam;

    if (g_recording && GetForegroundWindow() == g_main_hwnd) {
        int vk = (int)k->vkCode;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) s_recording_mods |= MOD_CONTROL;
            else if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) s_recording_mods |= MOD_ALT;
            else if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) s_recording_mods |= MOD_SHIFT;
            else if (vk == VK_ESCAPE) { s_recording_mods = 0; stop_recording(g_main_hwnd, FALSE); return 1; }
            else { g_recorded_vk = vk; g_recorded_mod = s_recording_mods; s_recording_mods = 0; stop_recording(g_main_hwnd, TRUE); return 1; }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) s_recording_mods &= ~MOD_CONTROL;
            else if (vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) s_recording_mods &= ~MOD_ALT;
            else if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) s_recording_mods &= ~MOD_SHIFT;
        }
        return 1;
    }

    if (g_wheel_visible) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (k->vkCode == VK_ESCAPE) { PostMessageW(g_overlay_hwnd, WM_CW_CANCEL, 0, 0); return 1; }
            if (k->vkCode == VK_UP)    { int s = sector_to_slot(0); if (s >= 0 && s < g_slot_count) g_sel = 0; return 1; }
            if (k->vkCode == VK_RIGHT) { int s = sector_to_slot(5); if (s >= 0 && s < g_slot_count) g_sel = 5; return 1; }
            if (k->vkCode == VK_DOWN)  { g_sel = 4; return 1; }
            if (k->vkCode == VK_LEFT)  { int s = sector_to_slot(2); if (s >= 0 && s < g_slot_count) g_sel = 2; return 1; }
            if (k->vkCode == VK_NEXT)  { int s = sector_to_slot(6); if (s >= 0 && s < g_slot_count) g_sel = 6; return 1; }
            if (k->vkCode == VK_PRIOR) { int s = sector_to_slot(1); if (s >= 0 && s < g_slot_count) g_sel = 1; return 1; }
            if (k->vkCode == VK_HOME)  { int s = sector_to_slot(3); if (s >= 0 && s < g_slot_count) g_sel = 3; return 1; }
            if (k->vkCode == VK_END)   { int s = sector_to_slot(7); if (s >= 0 && s < g_slot_count) g_sel = 7; return 1; }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            BOOL hide = FALSE;
            if (vk_matches(k->vkCode)) hide = TRUE;
            else if ((g_mod & MOD_CONTROL) && (k->vkCode == VK_CONTROL || k->vkCode == VK_LCONTROL || k->vkCode == VK_RCONTROL)) hide = TRUE;
            else if ((g_mod & MOD_ALT) && (k->vkCode == VK_MENU || k->vkCode == VK_LMENU || k->vkCode == VK_RMENU)) hide = TRUE;
            else if ((g_mod & MOD_SHIFT) && (k->vkCode == VK_SHIFT || k->vkCode == VK_LSHIFT || k->vkCode == VK_RSHIFT)) hide = TRUE;
            if (hide) { PostMessageW(g_overlay_hwnd, WM_CW_HIDE, 0, 0); return 1; }
        }
        return 1;
    }

    if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
        if (vk_matches(k->vkCode) && mod_down()) {
            if (g_hotkey_suppress) return 1;
            PostMessageW(g_overlay_hwnd, WM_CW_SHOW, 0, 0);
            return 1;
        }
    } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
        if (vk_matches(k->vkCode)) g_hotkey_suppress = 0;
    }
    return CallNextHookEx(g_kb_hook, code, wParam, lParam);
}

/* ─── Entry point ──────────────────────────────────────────────────────────── */

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE prev, LPWSTR cmd, int show) {
    HANDLE one; WNDCLASSEXW wc; MSG msg; RECT work;
    (void)prev; (void)show;

    g_hinst = hi;
    enable_dpi_awareness();
    init_theme_objects();
    show_startup_splash(hi);

    if (cmd && wcsstr(cmd, L"--pins-clear")) { cw_history_init(); cw_history_clear_all_pins(); return 0; }
    if (cmd) {
        const wchar_t *pm = wcsstr(cmd, L"--pin-many=");
        if (pm) { wchar_t err[256], buf[CW_MAX_CHARS]; const wchar_t *t = pm + 11; size_t n = 0;
            cw_history_init();
            while (*t && n + 1 < ARRAYSIZE(buf)) {
                if (*t == L',') { buf[n] = 0;
                    if (buf[0]) { if (!cw_history_pin_text_ex(buf, err, ARRAYSIZE(err))) { MessageBoxW(NULL, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING); return 1; } }
                    n = 0; t++; continue; }
                buf[n++] = *t++; }
            buf[n] = 0; if (buf[0]) { if (!cw_history_pin_text_ex(buf, err, ARRAYSIZE(err))) { MessageBoxW(NULL, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING); return 1; } }
            return 0; }
        const wchar_t *p = wcsstr(cmd, L"--pin=");
        if (p) { wchar_t err[256]; const wchar_t *t = p + 6;
            cw_history_init();
            if (!cw_history_pin_text_ex(t, err, ARRAYSIZE(err))) { MessageBoxW(NULL, err[0] ? err : L"固定失败。", L"ClipWheel", MB_ICONWARNING); return 1; }
            return 0; } }

    if (cmd && (wcsstr(cmd, L"--selftest") || wcsstr(cmd, L"--selftest-noui"))) {
        wchar_t tmp[MAX_PATH], base[MAX_PATH]; int noui = wcsstr(cmd, L"--selftest-noui") != NULL;
        DWORD n = GetTempPathW(MAX_PATH, base);
        if (n == 0 || n >= MAX_PATH) wcsncpy_s(base, ARRAYSIZE(base), L".\\", _TRUNCATE);
        swprintf_s(tmp, ARRAYSIZE(tmp), L"%sClipWheelSelfTest", base);
        SetEnvironmentVariableW(L"CLIPWHEEL_DATA_DIR", tmp);
        if (cw_history_selftest()) { if (!noui) MessageBoxW(NULL, L"固定/保存/加载自测通过。", L"ClipWheel SelfTest", MB_ICONINFORMATION); return 0; }
        if (!noui) MessageBoxW(NULL, L"固定/保存/加载自测失败。", L"ClipWheel SelfTest", MB_ICONERROR); return 1; }

    one = CreateMutexW(NULL, FALSE, L"Local\\ClipWheelSingleInstance");
    if (!one) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(one); MessageBoxW(NULL, L"ClipWheel 已经在运行中。", L"ClipWheel", MB_ICONINFORMATION); return 0; }

    load_config();
    cw_history_init();

    ZeroMemory(&wc, sizeof(wc)); wc.cbSize = sizeof(wc); wc.hInstance = hi;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    wc.hIconSm = (HICON)LoadImageW(hi, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

    wc.lpfnWndProc = OverlayProc; wc.lpszClassName = kOverlayClass;
    RegisterClassExW(&wc);
    wc.lpfnWndProc = MainProc; wc.lpszClassName = kMainClass; wc.hbrBackground = g_brush_light;
    RegisterClassExW(&wc);
    wc.lpfnWndProc = CardListProc; wc.lpszClassName = kCardListClass; wc.hbrBackground = g_brush_dark;
    wc.style = CS_DBLCLKS;
    RegisterClassExW(&wc);
    wc.lpfnWndProc = WheelPreviewProc; wc.lpszClassName = kWheelPreviewClass; wc.hbrBackground = g_brush_light;
    wc.style = 0;
    RegisterClassExW(&wc);

    g_overlay_hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        kOverlayClass, L"ClipWheel Overlay", WS_POPUP,
        GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN),
        NULL, NULL, hi, NULL);
    if (!g_overlay_hwnd) { cw_history_shutdown(); CloseHandle(one); destroy_theme_objects(); return 1; }

    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    g_main_hwnd = CreateWindowExW(0, kMainClass, L"ClipWheel 控制中心",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, work.left + 80, work.top + 48, 1180, 760,
        NULL, NULL, hi, NULL);
    if (!g_main_hwnd) { DestroyWindow(g_overlay_hwnd); cw_history_shutdown(); CloseHandle(one); destroy_theme_objects(); return 1; }

    tray_add();
    g_kb_hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hi, 0);
    if (!g_kb_hook) { MessageBoxW(NULL, L"无法安装全局键盘钩子。", L"ClipWheel", MB_ICONERROR);
        DestroyWindow(g_main_hwnd); DestroyWindow(g_overlay_hwnd); cw_history_shutdown(); CloseHandle(one); destroy_theme_objects(); return 1; }

    ShowWindow(g_main_hwnd, SW_SHOW);
    UpdateWindow(g_main_hwnd);
    refresh_manager_ui();

    while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }

    UnhookWindowsHookEx(g_kb_hook);
    cw_history_shutdown();
    CloseHandle(one);
    destroy_theme_objects();
    return (int)msg.wParam;
}
