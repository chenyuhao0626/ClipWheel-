#ifndef CLIPWHEEL_APP_H
#define CLIPWHEEL_APP_H

#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include "history.h"
#include "resource.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "msimg32.lib")

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* ─── Color system ─────────────────────────────────────────────────────── */
#define COL_WHITE          RGB(255, 255, 255)
#define COL_ACCENT         RGB(99, 102, 241)
#define COL_ACCENT_HOVER   RGB(129, 140, 248)
#define COL_ACCENT_GLOW    RGB(199, 210, 254)
#define COL_TEXT_TERTIARY  RGB(113, 113, 122)

#define COL_WHEEL_BG       RGB(15, 15, 18)
#define COL_WHEEL_SECTOR   RGB(35, 35, 40)
#define COL_WHEEL_BORDER   RGB(71, 85, 105)
#define COL_WHEEL_HIGHLIGHT RGB(167, 139, 250)
#define COL_WHEEL_GLOW     RGB(99, 102, 241)

#define COL_BG_DEEP        RGB(9, 9, 11)
#define COL_BG_SURFACE     RGB(18, 18, 24)
#define COL_BG_CARD        RGB(39, 39, 42)
#define COL_BORDER_SUBTLE  RGB(39, 39, 42)
#define COL_BORDER_FOCUS   RGB(129, 140, 248)
#define COL_TEXT_PRIMARY   RGB(250, 250, 250)
#define COL_TEXT_SECONDARY RGB(163, 163, 174)

/* ─── Layout constants ─────────────────────────────────────────────────── */
#define NSECT               8
#define OUTER_R             250
#define INNER_R             72
#define OUTER_SELECT_MARGIN 72
#define OUTER_STICKY_MARGIN 96
#define FLOATING_OVERLAY_W  720
#define FLOATING_OVERLAY_H  760
#define OVERLAY_KEY_COLOR   RGB(255, 0, 255)
#define ID_TIMER_POLL       1
#define WM_TRAY             (WM_APP + 30)
#define WM_CW_SHOW          (WM_APP + 1)
#define WM_CW_HIDE          (WM_APP + 2)
#define WM_CW_CANCEL        (WM_APP + 3)
#define WM_CW_REFRESH_UI    (WM_APP + 4)
#define TRAY_UID            1

#define BTN_OPEN_CONFIG     1001
#define BTN_RELOAD_CONFIG   1002
#define BTN_PIN_CLIPBOARD   1003
#define BTN_HIDE_TO_TRAY    1004
#define BTN_EXIT_APP        1005
#define CHK_AUTOPASTE       1006
#define CHK_FLOATING_MODE   1007
#define BTN_CLEAR_PINS      1008
#define BTN_RECORD_HOTKEY   1009
#define EDIT_VK             1010
#define CHK_MOD_CTRL        1011
#define CHK_MOD_ALT         1012
#define CHK_MOD_SHIFT       1013
#define LIST_PINS           1101
#define LIST_HISTORY        1102
#define BTN_COPY_PIN        1201
#define BTN_UNPIN           1202
#define BTN_COPY_HISTORY    1203
#define BTN_PIN_HISTORY     1204
#define BTN_DELETE_HISTORY  1205
#define CHK_AUTOSTART       1206
#define BTN_UNDO            1207

#define CONTENT_PAD         24
#define COL_GAP             20

#define CLIPWHEEL_VERSION   L"2.2.0"

/* ─── Card list kind ────────────────────────────────────────────────────── */
#define CARD_KIND_PIN      1
#define CARD_KIND_HISTORY  2

typedef struct CardListState {
    int kind;
    int selected;
    int top_index;
} CardListState;

/* ─── Undo action kinds ────────────────────────────────────────────────── */
#define UNDO_ACTION_NONE       0
#define UNDO_ACTION_DELETE_PIN 1
#define UNDO_ACTION_DELETE_HIST 2
#define UNDO_ACTION_UNPIN      3

typedef struct UndoEntry {
    int  action;
    int  index;
    wchar_t text[CW_MAX_CHARS];
} UndoEntry;

#define UNDO_STACK_SIZE 16

/* ─── Shared window class names ────────────────────────────────────────── */
extern const wchar_t *kOverlayClass;
extern const wchar_t *kMainClass;
extern const wchar_t *kCardListClass;
extern const wchar_t *kWheelPreviewClass;

/* ─── Window handles (extern) ──────────────────────────────────────────── */
extern HINSTANCE g_hinst;
extern HWND g_overlay_hwnd;
extern HWND g_main_hwnd;
extern HWND g_pin_list_hwnd;
extern HWND g_history_list_hwnd;
extern HWND g_auto_paste_hwnd;
extern HWND g_floating_mode_hwnd;
extern HWND g_record_btn_hwnd;
extern HWND g_wheel_preview_hwnd;
extern HWND g_auto_start_hwnd;
extern HWND g_undo_btn_hwnd;

/* ─── Configuration ────────────────────────────────────────────────────── */
extern int g_vk;
extern int g_mod;
extern int g_auto_paste;
extern int g_floating_mode;

/* ─── Wheel state ──────────────────────────────────────────────────────── */
extern volatile int g_wheel_visible;
extern volatile int g_hotkey_suppress;
extern int g_sel;
extern int g_slot_count;
extern wchar_t g_slots[CW_MAX_SLOT][CW_MAX_CHARS];
extern HWND g_target_hwnd;
extern POINT g_original_cursor_pos;
extern POINT g_wheel_center_screen;
extern int g_overlay_vx;
extern int g_overlay_vy;
extern float g_wheel_appear;
extern float g_sector_heat[CW_MAX_SLOT];
extern float g_main_phase;

/* ─── Hook / tray ──────────────────────────────────────────────────────── */
extern HHOOK g_kb_hook;
extern NOTIFYICONDATAW g_nid;
extern int g_tray_added;
extern int g_ignore_clip;
extern int g_tray_tip_shown;

/* ─── Config strings ───────────────────────────────────────────────────── */
extern wchar_t g_ini[MAX_PATH];
extern wchar_t g_hotkey_label[64];

/* ─── Theme objects ────────────────────────────────────────────────────── */
extern HFONT g_font_display;
extern HFONT g_font_title;
extern HFONT g_font_body;
extern HFONT g_font_body_bold;
extern HFONT g_font_caption;
extern HBRUSH g_brush_light;
extern HBRUSH g_brush_dark;
extern HBRUSH g_brush_white;

/* ─── Preview state ────────────────────────────────────────────────────── */
extern int   g_prev_hover_sector;
extern int   g_prev_x_hover_sector;
extern float g_prev_sector_heat[CW_MAX_SLOT];
extern float g_prev_cancel_heat;
extern int   g_prev_dyn_outer_r;
extern int   g_prev_dyn_inner_r;
extern int   g_prev_dyn_x_btn_r;

/* ─── Drag-drop ────────────────────────────────────────────────────────── */
extern int     g_drag_active;
extern int     g_drag_pending;
extern int     g_drag_from_kind;
extern int     g_drag_from_index;
extern wchar_t g_drag_text[CW_MAX_CHARS];
extern POINT   g_drag_pending_start;
extern POINT   g_drag_pos_screen;
extern int     g_drag_drop_sector;

/* ─── Cancel zone ──────────────────────────────────────────────────────── */
extern float g_cancel_heat;

/* ─── Hotkey recording ─────────────────────────────────────────────────── */
extern int g_recording;
extern int g_recorded_vk;
extern int g_recorded_mod;

/* ─── Undo stack ───────────────────────────────────────────────────────── */
extern UndoEntry g_undo_stack[UNDO_STACK_SIZE];
extern int g_undo_count;

/* ─── Search ───────────────────────────────────────────────────────────── */
extern int g_manager_search;
extern wchar_t g_manager_search_buf[256];

/* ─── Preview parameters ───────────────────────────────────────────────── */
#define PREV_OUTER_R        175
#define PREV_INNER_R         50
#define PREV_X_BTN_R         10
#define SEL_CANCEL_ZONE     (-2)

/* ─── Navigation ───────────────────────────────────────────────────────── */
#define NAV_H          60
#define QUICK_CARD_H   64
#define SETTINGS_H     100
#define SECTION_GAP    12

/* ─── Function declarations ────────────────────────────────────────────── */

/* draw_utils.c */
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

/* main.c internals */
int sector_to_slot(int sector);
int slot_to_sector(int slot);
void build_slots(void);
int sector_from_point(int cx, int cy, int px, int py);
int copy_text_to_clipboard(HWND owner, const wchar_t *text);
void send_ctrl_v(void);
void focus_target_window(HWND target);
void apply_selection_to_target(void);
void show_wheel(void);
void hide_wheel_commit(void);
void hide_wheel_cancel(void);
void draw_wheel(HDC hdc, int w, int h);
void paint_overlay(void);
void draw_main_window_background(HDC hdc, const RECT *rc);
void paint_main_window(HWND hwnd);
void layout_main_window(HWND hwnd);
void sync_clipboard_now(HWND hwnd);
void tray_add(void);
void tray_remove(void);
void tray_show_balloon(const wchar_t *title, const wchar_t *msg);
void show_about(HWND owner);
void open_config_file(void);
void refresh_manager_ui(void);
void refresh_manager_lists(void);
void copy_pin_selection(void);
void copy_history_selection(void);
void unpin_selected_pin(HWND owner);
void show_startup_splash(HINSTANCE hi);
void enable_dpi_awareness(void);
void format_hotkey_label(void);
void load_config(void);
void save_behavior_config(void);
void save_hotkey_config(void);
int mod_down(void);
int vk_matches(DWORD vk);
int is_our_window(HWND hwnd);
void start_recording(HWND hwnd);
void stop_recording(HWND hwnd, BOOL save);
void sync_settings_controls(void);
void push_undo(int action, int index, const wchar_t *text);
int pop_undo(int *action, int *index, wchar_t *out, size_t cch_out);
void apply_undo(HWND owner);

/* cardlist.c */
CardListState *card_list_state(HWND hwnd);
int card_list_visible_count(HWND hwnd);
void card_list_sync_scroll(HWND hwnd);
int card_list_selected_index(HWND hwnd);
void card_list_set_selected(HWND hwnd, int index);
int card_list_hit_test(HWND hwnd, int y);
int card_list_count_for_kind(int kind);
int card_list_copy_for_kind(int kind, int index, wchar_t *out, size_t cch_out);
void paint_card_list(HWND hwnd);
LRESULT CALLBACK CardListProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/* wheel_preview.c */
void preview_calc_radii(int w, int h);
int preview_sector_from_point(int cx, int cy, int px, int py);
int preview_x_hit_test(int cx, int cy, int px, int py, int slot_count);
void preview_do_drop(int target_sector);
void paint_wheel_preview(HWND hwnd);
LRESULT CALLBACK WheelPreviewProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

#endif /* CLIPWHEEL_APP_H */
