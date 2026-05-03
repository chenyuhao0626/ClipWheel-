#ifndef CLIPWHEEL_CARDLIST_H
#define CLIPWHEEL_CARDLIST_H

#include "app.h"

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

#endif
