#include "app.h"
#include "draw_utils.h"

#define CW_CARD_H  DPISC(82)
#define CW_CARD_GAP DPISC(12)
#define CW_CARD_TOP DPISC(10)

int card_list_count_for_kind(int kind) {
    return kind == CARD_KIND_PIN ? cw_history_pin_count() : cw_history_history_count();
}

int card_list_copy_for_kind(int kind, int index, wchar_t *out, size_t cch_out) {
    return kind == CARD_KIND_PIN ? cw_history_copy_pin(index, out, cch_out)
                                 : cw_history_copy_history(index, out, cch_out);
}

CardListState *card_list_state(HWND hwnd) {
    return (CardListState *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
}

int card_list_visible_count(HWND hwnd) {
    RECT rc;
    int card_h = CW_CARD_H;
    GetClientRect(hwnd, &rc);
    return max(1, (rc.bottom - rc.top - CW_CARD_TOP) / (card_h + CW_CARD_GAP));
}

void card_list_sync_scroll(HWND hwnd) {
    CardListState *state = card_list_state(hwnd);
    SCROLLINFO si;
    int count;
    int page;
    if (!state) return;
    count = card_list_count_for_kind(state->kind);
    page = card_list_visible_count(hwnd);
    if (state->selected >= count) state->selected = count - 1;
    if (count <= 0) state->selected = -1;
    if (state->top_index > max(0, count - page)) state->top_index = max(0, count - page);
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nMin = 0;
    si.nMax = max(0, count - 1);
    si.nPage = (UINT)page;
    si.nPos = state->top_index;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

int card_list_selected_index(HWND hwnd) {
    CardListState *state = card_list_state(hwnd);
    return state ? state->selected : -1;
}

void card_list_set_selected(HWND hwnd, int index) {
    CardListState *state = card_list_state(hwnd);
    int count;
    int page;
    if (!state) return;
    count = card_list_count_for_kind(state->kind);
    if (index < -1) index = -1;
    if (index >= count) index = count - 1;
    state->selected = index;
    if (index >= 0) {
        page = card_list_visible_count(hwnd);
        if (index < state->top_index) state->top_index = index;
        if (index >= state->top_index + page) state->top_index = index - page + 1;
    }
    card_list_sync_scroll(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

int card_list_hit_test(HWND hwnd, int y) {
    CardListState *state = card_list_state(hwnd);
    int card_h = CW_CARD_H;
    int gap = CW_CARD_GAP;
    int rel;
    if (!state) return -1;
    rel = y - CW_CARD_TOP;
    if (rel < 0) return -1;
    rel /= (card_h + gap);
    if (rel < 0) return -1;
    rel += state->top_index;
    if (rel >= card_list_count_for_kind(state->kind)) return -1;
    return rel;
}

void paint_card_list(HWND hwnd) {
    CardListState *state = card_list_state(hwnd);
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc = BeginPaint(hwnd, &ps);
    GetClientRect(hwnd, &rc);

    fill_vertical_gradient(hdc, &rc, TC_BG_CARD, TC_BG_SURFACE);

    if (state) {
        int count = card_list_count_for_kind(state->kind);
        int y = CW_CARD_TOP;
        int card_h = CW_CARD_H;
        int gap = CW_CARD_GAP;
        int visible = card_list_visible_count(hwnd);
        float pulse = 0.5f + 0.5f * (float)sin(g_main_phase);
        for (int i = 0; i < visible && state->top_index + i < count; i++) {
            int data_index = state->top_index + i;
            RECT card = {12, y, rc.right - 12, y + card_h};
            RECT inner;
            wchar_t text[CW_MAX_CHARS];
            wchar_t preview[220];
            wchar_t meta[64];

            COLORREF border;
            if (data_index == state->selected) {
                COLORREF top = mix_color(TC_ACCENT, TC_ACCENT_HOVER, pulse * 0.6f);
                COLORREF bot = mix_color(TC_ACCENT_DEEP, TC_ACCENT, pulse * 0.4f);
                fill_round_gradient(hdc, &card, top, bot, RADIUS_LG);
                border = TC_ACCENT_GLOW;
            } else {
                fill_round_gradient(hdc, &card, TC_BG_ELEVATED, TC_BG_CARD, RADIUS_LG);
                border = TC_BORDER_SUBTLE;
            }

            if (!card_list_copy_for_kind(state->kind, data_index, text, ARRAYSIZE(text))) break;

            draw_round_border(hdc, &card, border, RADIUS_LG, 1);

            if (data_index == state->selected) {
                HPEN glow = CreatePen(PS_SOLID, 2, TC_ACCENT_GLOW);
                HGDIOBJ old_pen = SelectObject(hdc, glow);
                HGDIOBJ old_br = SelectObject(hdc, GetStockObject(NULL_BRUSH));
                RECT gr = {card.left + 2, card.top + 2, card.right - 2, card.bottom - 2};
                RoundRect(hdc, gr.left, gr.top, gr.right, gr.bottom, (RADIUS_LG - 2) * 2, (RADIUS_LG - 2) * 2);
                SelectObject(hdc, old_pen);
                SelectObject(hdc, old_br);
                DeleteObject(glow);
            }
            inner = card;
            inner.left += 18;
            inner.top += 14;
            inner.right -= 18;
            inner.bottom = inner.top + 26;
            truncate_preview(preview, ARRAYSIZE(preview), text, 72);
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_font_body_bold);

            if (data_index == state->selected) {
                SetTextColor(hdc, TC_WHITE);
            } else {
                SetTextColor(hdc, TC_TEXT_PRIMARY);
            }

            DrawTextW(hdc, preview, -1, &inner, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);

            inner.top = card.top + 44;
            inner.bottom = card.bottom - 12;
            SelectObject(hdc, g_font_caption);

            if (data_index == state->selected) {
                SetTextColor(hdc, TC_ACCENT_GLOW);
            } else {
                SetTextColor(hdc, TC_TEXT_SECONDARY);
            }

            swprintf_s(meta, ARRAYSIZE(meta), state->kind == CARD_KIND_PIN ? L"\u56fa\u5b9a\u9879 #%d" : L"\u5386\u53f2\u8bb0\u5f55 #%d", data_index + 1);
            DrawTextW(hdc, meta, -1, &inner, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

            /* Draw edit icon for pin items */
            if (state->kind == CARD_KIND_PIN) {
                int icon_size = DPISC(28);
                int icon_x = card.right - DPISC(40);
                int icon_y = card.top + (card_h - icon_size) / 2;
                int icon_hot = (g_rename_pin_index == data_index);
                COLORREF icon_col = icon_hot ? TC_ACCENT_HOVER : TC_TEXT_TERTIARY;
                /* Pen icon: simple "✎" character */
                SelectObject(hdc, g_font_body_bold);
                SetTextColor(hdc, icon_col);
                RECT icon_rc = {icon_x, icon_y, icon_x + icon_size, icon_y + icon_size};
                DrawTextW(hdc, L"\u270e", -1, &icon_rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }

            y += card_h + gap;
        }
    }
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK CardListProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    CardListState *state = card_list_state(hwnd);
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lp;
        CardListState *new_state = (CardListState *)calloc(1, sizeof(CardListState));
        if (!new_state) return -1;
        new_state->kind = (int)(INT_PTR)cs->lpCreateParams;
        new_state->selected = -1;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)new_state);
        card_list_sync_scroll(hwnd);
        return 0;
    }
    case WM_SIZE:
        card_list_sync_scroll(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_VSCROLL:
        if (state) {
            int max_top = max(0, card_list_count_for_kind(state->kind) - card_list_visible_count(hwnd));
            switch (LOWORD(wp)) {
            case SB_LINEUP: state->top_index--; break;
            case SB_LINEDOWN: state->top_index++; break;
            case SB_PAGEUP: state->top_index -= card_list_visible_count(hwnd); break;
            case SB_PAGEDOWN: state->top_index += card_list_visible_count(hwnd); break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION: state->top_index = HIWORD(wp); break;
            }
            if (state->top_index < 0) state->top_index = 0;
            if (state->top_index > max_top) state->top_index = max_top;
            card_list_sync_scroll(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    case WM_MOUSEWHEEL:
        SendMessageW(hwnd, WM_VSCROLL, GET_WHEEL_DELTA_WPARAM(wp) > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
        return 0;
    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lp);
        int my = GET_Y_LPARAM(lp);
        int index = card_list_hit_test(hwnd, my);
        /* Check if click is on the edit icon (pin list only) */
        if (index >= 0 && state && state->kind == CARD_KIND_PIN) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int icon_x = rc.right - 44;
            int card_y = CW_CARD_TOP + (index - state->top_index) * (CW_CARD_H + CW_CARD_GAP);
            if (mx >= icon_x && mx < rc.right - DPISC(12) && my >= card_y && my < card_y + CW_CARD_H) {
                card_list_start_rename(hwnd, index, card_y);
                return 0;
            }
        }
        card_list_set_selected(hwnd, index);
        SetFocus(hwnd);
        if (index >= 0 && state) {
            g_drag_pending_start.x = mx;
            g_drag_pending_start.y = my;
            g_drag_from_kind  = state->kind;
            g_drag_from_index = index;
            card_list_copy_for_kind(state->kind, index, g_drag_text, CW_MAX_CHARS);
            g_drag_pending = 1;
        }
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
            if (g_wheel_preview_hwnd) InvalidateRect(g_wheel_preview_hwnd, NULL, FALSE);
            SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        g_drag_pending = 0;
        if (g_drag_active && GetCapture() == hwnd) {
            ReleaseCapture();
            if (g_wheel_preview_hwnd) {
                POINT pt;
                GetCursorPos(&pt);
                RECT wrc;
                GetWindowRect(g_wheel_preview_hwnd, &wrc);
                if (pt.x >= wrc.left && pt.x < wrc.right &&
                    pt.y >= wrc.top  && pt.y < wrc.bottom) {
                    POINT cpt = pt;
                    ScreenToClient(g_wheel_preview_hwnd, &cpt);
                    RECT prc;
                    GetClientRect(g_wheel_preview_hwnd, &prc);
                    int pcx = (prc.right-prc.left)/2;
                    int pcy = 72 + (prc.bottom-prc.top-72)/2;
                    preview_calc_radii(prc.right - prc.left, prc.bottom - prc.top);
                    int target = preview_sector_from_point(pcx, pcy, cpt.x, cpt.y);
                    if (target == 4 && g_drag_from_kind == CARD_KIND_PIN) {
                        cw_history_unpin_display_index(g_drag_from_index);
                        refresh_manager_ui();
                    } else if (target >= 0) {
                        int target_slot = sector_to_slot(target);
                        if (target_slot >= 0 && !(g_drag_from_kind == CARD_KIND_PIN && target_slot == g_drag_from_index)) {
                            preview_do_drop(target_slot);
                        }
                    }
                }
            }
            g_drag_active      = 0;
            g_drag_drop_sector = -1;
            if (g_wheel_preview_hwnd) InvalidateRect(g_wheel_preview_hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_CAPTURECHANGED:
        g_drag_pending = 0;
        if (g_drag_active) {
            g_drag_active      = 0;
            g_drag_drop_sector = -1;
            if (g_wheel_preview_hwnd) InvalidateRect(g_wheel_preview_hwnd, NULL, FALSE);
        }
        return 0;
    case WM_LBUTTONDBLCLK: {
        int index = card_list_hit_test(hwnd, GET_Y_LPARAM(lp));
        card_list_set_selected(hwnd, index);
        if (state && index >= 0) {
            SendMessageW(GetParent(hwnd), WM_COMMAND,
                         MAKEWPARAM(state->kind == CARD_KIND_PIN ? BTN_COPY_PIN : BTN_COPY_HISTORY, BN_CLICKED),
                         0);
        }
        return 0;
    }
    case WM_RBUTTONDOWN: {
        int index = card_list_hit_test(hwnd, GET_Y_LPARAM(lp));
        if (index >= 0) {
            card_list_set_selected(hwnd, index);
        }
        if (state && index >= 0) {
            POINT pt;
            HMENU m = CreatePopupMenu();
            GetCursorPos(&pt);
            wchar_t text_buf[CW_MAX_CHARS];
            if (card_list_copy_for_kind(state->kind, index, text_buf, ARRAYSIZE(text_buf))) {
            wchar_t label[CW_MAX_CHARS + 64];
                swprintf_s(label, ARRAYSIZE(label), L"编辑: %s", text_buf);
                AppendMenuW(m, MF_STRING, 1, L"复制到剪贴板");
                if (state->kind == CARD_KIND_HISTORY) AppendMenuW(m, MF_STRING, 2, L"固定此项");
                if (state->kind == CARD_KIND_PIN) {
                    AppendMenuW(m, MF_STRING, 3, label);
                    AppendMenuW(m, MF_STRING, 6, L"\u91cd\u547d\u540d");
                    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(m, MF_STRING, 4, L"\u53d6\u6d88\u56fa\u5b9a");
                }
                if (state->kind == CARD_KIND_HISTORY) {
                    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(m, MF_STRING, 5, L"删除此记录");
                }
            }
            int cmd = (int)TrackPopupMenu(m, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
            if (cmd == 1) {
                SendMessageW(GetParent(hwnd), WM_COMMAND,
                             MAKEWPARAM(state->kind == CARD_KIND_PIN ? BTN_COPY_PIN : BTN_COPY_HISTORY, BN_CLICKED), 0);
            } else if (cmd == 2) {
                SendMessageW(GetParent(hwnd), WM_COMMAND,
                             MAKEWPARAM(BTN_PIN_HISTORY, BN_CLICKED), 0);
            } else if (cmd == 3) {
                wchar_t out[CW_MAX_CHARS];
                if (card_list_copy_for_kind(state->kind, index, out, ARRAYSIZE(out))) {
                    copy_text_to_clipboard(g_main_hwnd, out);
                    MessageBoxW(g_main_hwnd, L"已复制到剪贴板，请在外部编辑后重新固定。", L"编辑固定项", MB_ICONINFORMATION);
                }
            } else if (cmd == 4) {
                SendMessageW(GetParent(hwnd), WM_COMMAND,
                             MAKEWPARAM(BTN_UNPIN, BN_CLICKED), 0);
            } else if (cmd == 5) {
                SendMessageW(GetParent(hwnd), WM_COMMAND,
                             MAKEWPARAM(BTN_DELETE_HISTORY, BN_CLICKED), 0);
            } else if (cmd == 6) {
                /* Start inline rename */
                int card_y = CW_CARD_TOP + (index - state->top_index) * (CW_CARD_H + CW_CARD_GAP);
                card_list_start_rename(hwnd, index, card_y);
            }
            DestroyMenu(m);
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (state) {
            if (wp == 'F' && (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
                g_manager_search = 1;
                g_manager_search_buf[0] = 0;
                InvalidateRect(g_main_hwnd, NULL, TRUE);
                if (g_main_hwnd) SetFocus(g_main_hwnd);
                return 0;
            }
            switch (wp) {
            case VK_UP: card_list_set_selected(hwnd, state->selected - 1); return 0;
            case VK_DOWN: card_list_set_selected(hwnd, state->selected + 1); return 0;
            case VK_RETURN:
                if (state->selected >= 0) {
                    SendMessageW(GetParent(hwnd), WM_COMMAND,
                                 MAKEWPARAM(state->kind == CARD_KIND_PIN ? BTN_COPY_PIN : BTN_COPY_HISTORY, BN_CLICKED),
                                 0);
                }
                return 0;
            }
        }
        break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    case WM_PAINT:
        paint_card_list(hwnd);
        return 0;
    case WM_ERASEBKGND: {
        RECT rc;
        GetClientRect(hwnd, &rc);
        fill_vertical_gradient((HDC)wp, &rc, TC_BG_CARD, TC_BG_SURFACE);
        return 1;
    }
    case WM_DESTROY:
        free(state);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
