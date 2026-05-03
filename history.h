/* Clipboard history + pinned slots (UTF-16). */
#ifndef CLIPWHEEL_HISTORY_H
#define CLIPWHEEL_HISTORY_H

#include <windows.h>

#define CW_MAX_SLOT   7
#define CW_MAX_HIST   32
#define CW_MAX_PIN    7
#define CW_MAX_CHARS  4096

void cw_history_init(void);
void cw_history_shutdown(void);

void cw_history_on_copy(const wchar_t *text);

void cw_history_pin_current_clipboard(HWND owner);
void cw_history_pin_text(const wchar_t *text);
int cw_history_pin_current_clipboard_ex(HWND owner, wchar_t *err, size_t cch_err);
int cw_history_pin_text_ex(const wchar_t *text, wchar_t *err, size_t cch_err);
void cw_history_unpin_display_index(int display_index);
void cw_history_delete_history_index(int index);
void cw_history_clear_all_pins(void);
int  cw_history_reorder_pin(int from_idx, int to_idx);
int  cw_history_pin_text_at(const wchar_t *text, int position, wchar_t *err, size_t cch_err);

int cw_history_fill_slots(wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS]);
int cw_history_pin_count(void);
int cw_history_history_count(void);
int cw_history_copy_pin(int index, wchar_t *out, size_t cch_out);
int cw_history_copy_history(int index, wchar_t *out, size_t cch_out);

void cw_history_load(void);
void cw_history_save(void);

int cw_history_selftest(void);

#endif
