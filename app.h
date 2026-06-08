#ifndef CLIPWHEEL_APP_H
#define CLIPWHEEL_APP_H

#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include <uxtheme.h>
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
#pragma comment(lib, "uxtheme.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

/* ─── Design Tokens: Surface Elevation ──────────────────────────────────── */
/* 60% dominant: backgrounds, 30% secondary: cards, 10% accent: highlights  */
#define COL_BG_DEEP        RGB(9, 9, 11)       /* deepest layer */
#define COL_BG_SURFACE     RGB(15, 16, 22)      /* page background */
#define COL_BG_CARD        RGB(22, 23, 34)      /* card surfaces */
#define COL_BG_ELEVATED    RGB(30, 31, 44)      /* elevated cards, hover */
#define COL_BG_OVERLAY     RGB(38, 39, 54)      /* overlays, dropdowns */

/* ─── Design Tokens: Accent (Indigo) ───────────────────────────────────── */
#define COL_ACCENT         RGB(99, 102, 241)    /* primary accent */
#define COL_ACCENT_HOVER   RGB(129, 140, 248)   /* accent hover */
#define COL_ACCENT_GLOW    RGB(199, 210, 254)   /* accent glow/ring */
#define COL_ACCENT_DEEP    RGB(67, 70, 200)     /* accent pressed */

/* ─── Design Tokens: Text ──────────────────────────────────────────────── */
#define COL_TEXT_PRIMARY   RGB(250, 250, 250)   /* headings, primary */
#define COL_TEXT_SECONDARY RGB(163, 163, 174)   /* body, descriptions */
#define COL_TEXT_TERTIARY  RGB(113, 113, 122)   /* captions, hints */
#define COL_TEXT_DISABLED  RGB(75, 75, 85)      /* disabled states */
#define COL_WHITE          RGB(255, 255, 255)

/* ─── Design Tokens: Border ────────────────────────────────────────────── */
#define COL_BORDER_SUBTLE  RGB(45, 46, 58)      /* card borders */
#define COL_BORDER_DEFAULT RGB(55, 56, 70)      /* visible borders */
#define COL_BORDER_FOCUS   RGB(129, 140, 248)   /* focus ring */

/* ─── Design Tokens: Status ────────────────────────────────────────────── */
#define COL_SUCCESS        RGB(52, 211, 153)    /* green: auto-paste on */
#define COL_SUCCESS_BG     RGB(28, 70, 52)
#define COL_DANGER         RGB(248, 113, 113)   /* red: errors, cancel */
#define COL_DANGER_BG      RGB(58, 28, 30)
#define COL_DANGER_DEEP    RGB(200, 80, 80)     /* red: cancel zone */

/* ─── Design Tokens: Wheel ─────────────────────────────────────────────── */
#define COL_WHEEL_BG       RGB(15, 15, 18)
#define COL_WHEEL_SECTOR   RGB(35, 35, 40)
#define COL_WHEEL_BORDER   RGB(71, 85, 105)
#define COL_WHEEL_GLOW     COL_ACCENT

/* ─── Cancel zone ──────────────────────────────────────────────────────── */
#define COL_CANCEL_SECTOR_A   RGB(90, 25, 25)
#define COL_CANCEL_SECTOR_B   RGB(200, 55, 55)
#define COL_CANCEL_BORDER_A   RGB(160, 45, 45)
#define COL_CANCEL_BORDER_B   RGB(255, 110, 110)
#define COL_CANCEL_PILL_A1    RGB(50, 15, 15)
#define COL_CANCEL_PILL_A2    RGB(70, 20, 20)
#define COL_CANCEL_PILL_B     RGB(30, 10, 10)
#define COL_CANCEL_PILL_BRD_A RGB(120, 40, 40)
#define COL_CANCEL_PILL_BRD_B RGB(200, 80, 80)
#define COL_CANCEL_TEXT_A     RGB(200, 80, 80)
#define COL_CANCEL_TEXT_B     RGB(255, 130, 130)

/* ─── X button (unpin) ──────────────────────────────────────────────────── */
#define COL_X_FILL_HOT    RGB(244, 63, 94)
#define COL_X_FILL        RGB(88, 28, 34)
#define COL_X_BORDER_HOT  RGB(251, 113, 133)
#define COL_X_BORDER      RGB(159, 18, 57)
#define COL_X_TEXT_HOT    RGB(255, 220, 220)
#define COL_X_TEXT        RGB(170, 100, 100)

/* ─── Label pill subtitle ───────────────────────────────────────────────── */
#define COL_LABEL_SUBTLE  RGB(200, 205, 220)

/* ─── Theme mode ──────────────────────────────────────────────────────────── */
typedef enum { THEME_DARK = 0, THEME_LIGHT = 1 } ThemeMode;
extern ThemeMode g_theme;

/* ─── Design Tokens: Light Theme (Claude.com warm editorial) ─────────────── */

/* Surface — warm cream hierarchy */
#define COL_L_CANVAS         RGB(250, 249, 245)  /* #faf9f5 — page floor */
#define COL_L_SURFACE        RGB(239, 233, 222)  /* #efe9de — card surface */
#define COL_L_CARD           RGB(245, 243, 238)  /* elevated card */
#define COL_L_ELEVATED       RGB(232, 225, 213)  /* #e8e1d5 — hover shelf */
#define COL_L_OVERLAY        RGB(255, 255, 255)  /* highest overlay */
#define COL_L_BG_DEEPEST     RGB(242, 239, 233)  /* deepest light bg */

/* Accent — Anthropic coral */
#define COL_L_ACCENT         RGB(204, 120, 92)   /* #cc785c */
#define COL_L_ACCENT_HOVER   RGB(224, 140, 112)  /* lighter coral */
#define COL_L_ACCENT_GLOW    RGB(240, 200, 180)  /* coral glow */
#define COL_L_ACCENT_DEEP    RGB(184, 100, 72)   /* pressed coral */

/* Text — warm ink */
#define COL_L_INK            RGB(20, 20, 19)     /* #141413 — headlines */
#define COL_L_BODY           RGB(61, 61, 58)     /* #3d3d3a — running text */
#define COL_L_MUTED          RGB(90, 88, 83)     /* #5a5853 — captions, 4.8:1 on surface */
#define COL_L_DISABLED       RGB(180, 177, 171)  /* disabled text */
#define COL_L_WHITE          RGB(255, 255, 255)

/* Border */
#define COL_L_BORDER_SUBTLE  RGB(220, 215, 205)
#define COL_L_BORDER_DEFAULT RGB(200, 193, 182)
#define COL_L_BORDER_FOCUS   COL_L_ACCENT

/* Status */
#define COL_L_SUCCESS        RGB(34, 130, 90)
#define COL_L_SUCCESS_BG     RGB(220, 245, 228)
#define COL_L_DANGER         RGB(210, 60, 50)
#define COL_L_DANGER_BG      RGB(250, 228, 225)
#define COL_L_DANGER_DEEP    RGB(185, 40, 30)

/* Wheel */
#define COL_L_WHEEL_BG       RGB(245, 243, 240)
#define COL_L_WHEEL_SECTOR   RGB(250, 249, 246)
#define COL_L_WHEEL_BORDER   RGB(200, 195, 188)
#define COL_L_WHEEL_CANCEL   RGB(220, 80, 70)

/* Cancel zone — light */
#define COL_L_CANCEL_SECTOR_A   RGB(250, 210, 205)
#define COL_L_CANCEL_SECTOR_B   RGB(235, 160, 150)
#define COL_L_CANCEL_BORDER_A   RGB(210, 120, 110)
#define COL_L_CANCEL_BORDER_B   RGB(240, 160, 150)
#define COL_L_CANCEL_PILL_A1    RGB(250, 215, 210)
#define COL_L_CANCEL_PILL_A2    RGB(240, 190, 180)
#define COL_L_CANCEL_PILL_B     RGB(245, 225, 220)
#define COL_L_CANCEL_PILL_BRD_A RGB(220, 140, 130)
#define COL_L_CANCEL_PILL_BRD_B RGB(235, 160, 150)
#define COL_L_CANCEL_TEXT_A     RGB(210, 80, 70)
#define COL_L_CANCEL_TEXT_B     RGB(235, 130, 120)

/* X button — light */
#define COL_L_X_FILL_HOT    RGB(210, 80, 85)
#define COL_L_X_FILL        RGB(245, 210, 208)
#define COL_L_X_BORDER_HOT  RGB(220, 110, 110)
#define COL_L_X_BORDER      RGB(190, 85, 85)
#define COL_L_X_TEXT_HOT    RGB(180, 60, 60)
#define COL_L_X_TEXT        RGB(150, 90, 90)

/* Label subtitle — light */
#define COL_L_LABEL_SUBTLE  RGB(80, 78, 72)

/* ─── Theme-Aware Proxy Macros ────────────────────────────────────────────── */
/* Drawing code uses TC_* exclusively; the proxy selects light/dark at compile
   time based on g_theme. No per-function if/else needed. */

#define TC_BG_DEEP      (g_theme == THEME_LIGHT ? COL_L_CANVAS      : COL_BG_DEEP)
#define TC_BG_SURFACE   (g_theme == THEME_LIGHT ? COL_L_SURFACE     : COL_BG_SURFACE)
#define TC_BG_CARD      (g_theme == THEME_LIGHT ? COL_L_CARD        : COL_BG_CARD)
#define TC_BG_ELEVATED  (g_theme == THEME_LIGHT ? COL_L_ELEVATED    : COL_BG_ELEVATED)
#define TC_BG_OVERLAY   (g_theme == THEME_LIGHT ? COL_L_OVERLAY     : COL_BG_OVERLAY)
#define TC_BG_DEEPEST   (g_theme == THEME_LIGHT ? COL_L_BG_DEEPEST  : COL_BG_DEEP)

#define TC_ACCENT       (g_theme == THEME_LIGHT ? COL_L_ACCENT       : COL_ACCENT)
#define TC_ACCENT_HOVER (g_theme == THEME_LIGHT ? COL_L_ACCENT_HOVER : COL_ACCENT_HOVER)
#define TC_ACCENT_GLOW  (g_theme == THEME_LIGHT ? COL_L_ACCENT_GLOW  : COL_ACCENT_GLOW)
#define TC_ACCENT_DEEP  (g_theme == THEME_LIGHT ? COL_L_ACCENT_DEEP  : COL_ACCENT_DEEP)

#define TC_TEXT_PRIMARY   (g_theme == THEME_LIGHT ? COL_L_INK      : COL_TEXT_PRIMARY)
#define TC_TEXT_SECONDARY (g_theme == THEME_LIGHT ? COL_L_BODY     : COL_TEXT_SECONDARY)
#define TC_TEXT_TERTIARY  (g_theme == THEME_LIGHT ? COL_L_MUTED    : COL_TEXT_TERTIARY)
#define TC_TEXT_DISABLED  (g_theme == THEME_LIGHT ? COL_L_DISABLED : COL_TEXT_DISABLED)
#define TC_WHITE          (g_theme == THEME_LIGHT ? COL_L_WHITE    : COL_WHITE)

#define TC_BORDER_SUBTLE  (g_theme == THEME_LIGHT ? COL_L_BORDER_SUBTLE  : COL_BORDER_SUBTLE)
#define TC_BORDER_DEFAULT (g_theme == THEME_LIGHT ? COL_L_BORDER_DEFAULT : COL_BORDER_DEFAULT)
#define TC_BORDER_FOCUS   (g_theme == THEME_LIGHT ? COL_L_BORDER_FOCUS   : COL_BORDER_FOCUS)

#define TC_SUCCESS        (g_theme == THEME_LIGHT ? COL_L_SUCCESS     : COL_SUCCESS)
#define TC_SUCCESS_BG     (g_theme == THEME_LIGHT ? COL_L_SUCCESS_BG  : COL_SUCCESS_BG)
#define TC_DANGER         (g_theme == THEME_LIGHT ? COL_L_DANGER      : COL_DANGER)
#define TC_DANGER_BG      (g_theme == THEME_LIGHT ? COL_L_DANGER_BG   : COL_DANGER_BG)
#define TC_DANGER_DEEP    (g_theme == THEME_LIGHT ? COL_L_DANGER_DEEP : COL_DANGER_DEEP)

#define TC_WHEEL_BG       (g_theme == THEME_LIGHT ? COL_L_WHEEL_BG     : COL_WHEEL_BG)
#define TC_WHEEL_SECTOR   (g_theme == THEME_LIGHT ? COL_L_WHEEL_SECTOR : COL_WHEEL_SECTOR)
#define TC_WHEEL_BORDER   (g_theme == THEME_LIGHT ? COL_L_WHEEL_BORDER : COL_WHEEL_BORDER)

#define TC_CANCEL_SECTOR_A   (g_theme == THEME_LIGHT ? COL_L_CANCEL_SECTOR_A   : COL_CANCEL_SECTOR_A)
#define TC_CANCEL_SECTOR_B   (g_theme == THEME_LIGHT ? COL_L_CANCEL_SECTOR_B   : COL_CANCEL_SECTOR_B)
#define TC_CANCEL_BORDER_A   (g_theme == THEME_LIGHT ? COL_L_CANCEL_BORDER_A   : COL_CANCEL_BORDER_A)
#define TC_CANCEL_BORDER_B   (g_theme == THEME_LIGHT ? COL_L_CANCEL_BORDER_B   : COL_CANCEL_BORDER_B)
#define TC_CANCEL_PILL_A1    (g_theme == THEME_LIGHT ? COL_L_CANCEL_PILL_A1    : COL_CANCEL_PILL_A1)
#define TC_CANCEL_PILL_A2    (g_theme == THEME_LIGHT ? COL_L_CANCEL_PILL_A2    : COL_CANCEL_PILL_A2)
#define TC_CANCEL_PILL_B     (g_theme == THEME_LIGHT ? COL_L_CANCEL_PILL_B     : COL_CANCEL_PILL_B)
#define TC_CANCEL_PILL_BRD_A (g_theme == THEME_LIGHT ? COL_L_CANCEL_PILL_BRD_A : COL_CANCEL_PILL_BRD_A)
#define TC_CANCEL_PILL_BRD_B (g_theme == THEME_LIGHT ? COL_L_CANCEL_PILL_BRD_B : COL_CANCEL_PILL_BRD_B)
#define TC_CANCEL_TEXT_A     (g_theme == THEME_LIGHT ? COL_L_CANCEL_TEXT_A     : COL_CANCEL_TEXT_A)
#define TC_CANCEL_TEXT_B     (g_theme == THEME_LIGHT ? COL_L_CANCEL_TEXT_B     : COL_CANCEL_TEXT_B)

#define TC_X_FILL_HOT   (g_theme == THEME_LIGHT ? COL_L_X_FILL_HOT   : COL_X_FILL_HOT)
#define TC_X_FILL       (g_theme == THEME_LIGHT ? COL_L_X_FILL       : COL_X_FILL)
#define TC_X_BORDER_HOT (g_theme == THEME_LIGHT ? COL_L_X_BORDER_HOT : COL_X_BORDER_HOT)
#define TC_X_BORDER     (g_theme == THEME_LIGHT ? COL_L_X_BORDER     : COL_X_BORDER)
#define TC_X_TEXT_HOT   (g_theme == THEME_LIGHT ? COL_L_X_TEXT_HOT   : COL_X_TEXT_HOT)
#define TC_X_TEXT       (g_theme == THEME_LIGHT ? COL_L_X_TEXT       : COL_X_TEXT)

#define TC_LABEL_SUBTLE (g_theme == THEME_LIGHT ? COL_L_LABEL_SUBTLE : COL_LABEL_SUBTLE)

/* ─── Font faces ──────────────────────────────────────────────────────── */
#define FONT_DISPLAY_DARK   L"Microsoft YaHei UI"
#define FONT_DISPLAY_LIGHT  L"Georgia"
#define FONT_BODY_DARK      L"Microsoft YaHei UI"
#define FONT_BODY_LIGHT     L"Segoe UI"

/* ─── DPI scaling ─────────────────────────────────────────────────────── */
extern float g_dpi_scale;
#define DPISC(px)  ((int)((px) * g_dpi_scale + 0.5f))

/* ─── Font sizes (logical px, scaled by g_dpi_scale at create time) ───── */
#define FONT_SIZE_DISPLAY  -44
#define FONT_SIZE_TITLE     -22
#define FONT_SIZE_BODY      -18
#define FONT_SIZE_RENAME    -16
#define FONT_SIZE_CAPTION   -14

/* ─── Design Tokens: Spacing (base-4 grid) ─────────────────────────────── */
#define SPACE_1   4
#define SPACE_2   8
#define SPACE_3   12
#define SPACE_4   16
#define SPACE_5   20
#define SPACE_6   24
#define SPACE_8   32
#define SPACE_10  40
#define SPACE_12  48

/* ─── Design Tokens: Radius ────────────────────────────────────────────── */
#define RADIUS_SM   6
#define RADIUS_MD  10
#define RADIUS_LG  14
#define RADIUS_XL  20
#define RADIUS_2XL 24

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

/* Direction ball constants */
#define BALL_RADIUS      14
#define BALL_ORBIT_R     (OUTER_R - 20)
#define BALL_INNER_DEAD  (INNER_R + 10)
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
#define CHK_DARK_MODE       1208

#define CONTENT_PAD         SPACE_6
#define COL_GAP             SPACE_3

#define CLIPWHEEL_VERSION   L"2.3.0"

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
extern HWND g_dark_mode_hwnd;

/* ─── Configuration ────────────────────────────────────────────────────── */
extern float g_dpi_scale;
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
extern float g_fan_phase;
extern float g_fan_open[NSECT];
extern float g_sector_heat[CW_MAX_SLOT];
extern float g_main_phase;
extern float g_ball_angle;
extern int   g_ball_active;
extern float g_ball_x;
extern float g_ball_y;
extern float g_ball_tx;
extern float g_ball_ty;

/* ─── Hook / tray ──────────────────────────────────────────────────────── */
extern HHOOK g_kb_hook;
extern NOTIFYICONDATAW g_nid;
extern int g_tray_added;
extern int g_ignore_clip;
extern int g_tray_tip_shown;

extern wchar_t g_ini[MAX_PATH];
extern wchar_t g_hotkey_label[64];

/* Inline rename state */
extern HWND g_rename_edit;
extern int g_rename_pin_index;
void card_list_start_rename(HWND parent, int pin_index, int card_y);

/* ─── Theme objects ────────────────────────────────────────────────────── */
extern HFONT g_font_display;
extern HFONT g_font_title;
extern HFONT g_font_body;
extern HFONT g_font_body_bold;
extern HFONT g_font_caption;
extern HBRUSH g_brush_bg_deep;
extern HBRUSH g_brush_bg_surface;
extern HBRUSH g_brush_bg_card;

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
#define SEL_CANCEL_ZONE     4

/* ─── Navigation ───────────────────────────────────────────────────────── */
#define NAV_H          60
#define QUICK_CARD_H   64
#define SETTINGS_H     130
#define SECTION_GAP    SPACE_3

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

#define INPUTBOX_CANCEL 0
#define INPUTBOX_OK     1
int input_box_show(HWND owner, const wchar_t *title, const wchar_t *prompt,
                   const wchar_t *initial, wchar_t *out, size_t cch_out);

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
