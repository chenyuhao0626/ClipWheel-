#include "history.h"
#include <shlobj.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <wchar.h>

#define MAGIC 0x43505748u /* 'CPWH' */
#define VER   1

static wchar_t g_pin[CW_MAX_PIN][CW_MAX_CHARS];
static int g_pin_count;

static wchar_t g_hist[CW_MAX_HIST][CW_MAX_CHARS];
static int g_hist_count;

static wchar_t g_data_dir[MAX_PATH];
static wchar_t g_data_file[MAX_PATH];

static int cw_history_save_internal(void);

static void cw_logf(const wchar_t *fmt, ...) {
    wchar_t path[MAX_PATH];
    wchar_t buf[1024];
    DWORD written;
    HANDLE h;
    va_list ap;
    if (!g_data_dir[0]) return;
    swprintf_s(path, MAX_PATH, L"%s\\clipwheel.log", g_data_dir);
    va_start(ap, fmt);
    _vsnwprintf_s(buf, ARRAYSIZE(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    h = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    WriteFile(h, buf, (DWORD)(wcslen(buf) * sizeof(wchar_t)), &written, NULL);
    WriteFile(h, L"\r\n", 2 * sizeof(wchar_t), &written, NULL);
    CloseHandle(h);
}

static void ensure_paths(void) {
    if (g_data_file[0]) return;
    wchar_t base[MAX_PATH];
    DWORD n = GetEnvironmentVariableW(L"CLIPWHEEL_DATA_DIR", base, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
    } else if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, base))) {
        DWORD n2 = GetEnvironmentVariableW(L"USERPROFILE", base, MAX_PATH);
        if (n2 == 0 || n2 >= MAX_PATH) {
            DWORD nt = GetTempPathW(MAX_PATH, base);
            if (nt == 0 || nt >= MAX_PATH) {
                GetCurrentDirectoryW(MAX_PATH, base);
            }
        }
    }
    if (base[0]) {
        SHCreateDirectoryExW(NULL, base, NULL);
    }
    swprintf_s(g_data_dir, MAX_PATH, L"%s\\ClipWheel", base);
    SHCreateDirectoryExW(NULL, g_data_dir, NULL);
    swprintf_s(g_data_file, MAX_PATH, L"%s\\clips.bin", g_data_dir);
}

static int wcs_same(const wchar_t *a, const wchar_t *b) {
    return wcscmp(a, b) == 0;
}

static void push_front_text(wchar_t dest[][CW_MAX_CHARS], int *count, int max_count, const wchar_t *text) {
    if (!text || !text[0] || !count || max_count <= 0) return;
    if (*count >= max_count) {
        for (int i = max_count - 1; i > 0; i--)
            wmemcpy(dest[i], dest[i - 1], CW_MAX_CHARS);
        *count = max_count;
    } else {
        for (int i = *count; i > 0; i--)
            wmemcpy(dest[i], dest[i - 1], CW_MAX_CHARS);
        (*count)++;
    }
    wcsncpy_s(dest[0], CW_MAX_CHARS, text, _TRUNCATE);
}

void cw_history_init(void) {
    ensure_paths();
    cw_history_load();
}

void cw_history_shutdown(void) {
    cw_history_save();
}

void cw_history_on_copy(const wchar_t *text) {
    if (!text || !text[0]) return;
    if (g_hist_count > 0 && wcs_same(text, g_hist[0])) return;
    push_front_text(g_hist, &g_hist_count, CW_MAX_HIST, text);
    cw_history_save();
}

void cw_history_pin_current_clipboard(HWND owner) {
    cw_history_pin_current_clipboard_ex(owner, NULL, 0);
}

void cw_history_pin_text(const wchar_t *text) {
    cw_history_pin_text_ex(text, NULL, 0);
}

int cw_history_pin_text_ex(const wchar_t *text, wchar_t *err, size_t cch_err) {
    wchar_t pin_backup[CW_MAX_PIN][CW_MAX_CHARS];
    int pin_backup_count;
    if (err && cch_err) err[0] = 0;
    if (!text || !text[0]) {
        if (err && cch_err) wcsncpy_s(err, cch_err, L"剪贴板为空，无法固定。", _TRUNCATE);
        cw_logf(L"[pin] rejected: empty text");
        return 0;
    }
    pin_backup_count = g_pin_count;
    for (int i = 0; i < g_pin_count && i < CW_MAX_PIN; i++) {
        wmemcpy(pin_backup[i], g_pin[i], CW_MAX_CHARS);
    }
    push_front_text(g_pin, &g_pin_count, CW_MAX_PIN, text);
    if (!cw_history_save_internal()) {
        g_pin_count = pin_backup_count;
        for (int k = 0; k < g_pin_count && k < CW_MAX_PIN; k++) wmemcpy(g_pin[k], pin_backup[k], CW_MAX_CHARS);
        if (err && cch_err) wcsncpy_s(err, cch_err, L"固定失败：保存数据失败（可能无权限或磁盘错误）。", _TRUNCATE);
        cw_logf(L"[pin] save failed (count=%d)", g_pin_count);
        return 0;
    }
    cw_logf(L"[pin] ok (count=%d)", g_pin_count);
    return 1;
}

int cw_history_pin_current_clipboard_ex(HWND owner, wchar_t *err, size_t cch_err) {
    wchar_t buf[CW_MAX_CHARS];
    buf[0] = 0;
    if (err && cch_err) err[0] = 0;
    for (int i = 0; i < 6; i++) {
        if (OpenClipboard(owner)) {
            HANDLE h = GetClipboardData(CF_UNICODETEXT);
            if (h) {
                const wchar_t *p = (const wchar_t *)GlobalLock(h);
                if (p) {
                    wcsncpy_s(buf, CW_MAX_CHARS, p, _TRUNCATE);
                    GlobalUnlock(h);
                }
            }
            CloseClipboard();
            break;
        }
        Sleep(15);
    }
    if (!buf[0]) {
        if (err && cch_err) wcsncpy_s(err, cch_err, L"无法读取剪贴板文本（可能被其他程序占用）。", _TRUNCATE);
        cw_logf(L"[pin] failed to read clipboard");
        return 0;
    }
    return cw_history_pin_text_ex(buf, err, cch_err);
}

void cw_history_unpin_display_index(int display_index) {
    if (display_index < 0 || display_index >= g_pin_count) return;
    for (int i = display_index; i < g_pin_count - 1; i++)
        wmemcpy(g_pin[i], g_pin[i + 1], CW_MAX_CHARS);
    g_pin_count--;
    if (g_pin_count < 0) g_pin_count = 0;
    cw_history_save();
    cw_logf(L"[unpin] ok (count=%d)", g_pin_count);
}

void cw_history_delete_history_index(int index) {
    if (index < 0 || index >= g_hist_count) return;
    for (int i = index; i < g_hist_count - 1; i++)
        wmemcpy(g_hist[i], g_hist[i + 1], CW_MAX_CHARS);
    g_hist_count--;
    if (g_hist_count < 0) g_hist_count = 0;
    cw_history_save();
}

void cw_history_clear_all_pins(void) {
    g_pin_count = 0;
    cw_history_save();
    cw_logf(L"[pins-clear] ok");
}

int cw_history_reorder_pin(int from_idx, int to_idx) {
    wchar_t tmp[CW_MAX_CHARS];
    if (from_idx < 0 || from_idx >= g_pin_count) return 0;
    if (to_idx < 0 || to_idx >= g_pin_count) return 0;
    if (from_idx == to_idx) return 1;
    wcsncpy_s(tmp, CW_MAX_CHARS, g_pin[from_idx], _TRUNCATE);
    if (from_idx < to_idx) {
        for (int i = from_idx; i < to_idx; i++)
            wmemcpy(g_pin[i], g_pin[i + 1], CW_MAX_CHARS);
    } else {
        for (int i = from_idx; i > to_idx; i--)
            wmemcpy(g_pin[i], g_pin[i - 1], CW_MAX_CHARS);
    }
    wcsncpy_s(g_pin[to_idx], CW_MAX_CHARS, tmp, _TRUNCATE);
    cw_history_save();
    cw_logf(L"[reorder] from=%d to=%d", from_idx, to_idx);
    return 1;
}

int cw_history_pin_text_at(const wchar_t *text, int position, wchar_t *err, size_t cch_err) {
    int r = cw_history_pin_text_ex(text, err, cch_err);
    if (!r) return 0;
    int target = position;
    if (target >= g_pin_count) target = g_pin_count - 1;
    if (target > 0) cw_history_reorder_pin(0, target);
    return 1;
}

int cw_history_fill_slots(wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS]) {
    int n = 0;
    for (int i = 0; i < g_pin_count && n < CW_MAX_SLOT; i++) {
        wcsncpy_s(slots[n], CW_MAX_CHARS, g_pin[i], _TRUNCATE);
        n++;
    }
    for (int h = 0; h < g_hist_count && n < CW_MAX_SLOT; h++) {
        int dup = 0;
        for (int p = 0; p < g_pin_count; p++) {
            if (wcs_same(g_hist[h], g_pin[p])) {
                dup = 1;
                break;
            }
        }
        if (dup) continue;
        for (int s = 0; s < n; s++) {
            if (wcs_same(g_hist[h], slots[s])) {
                dup = 1;
                break;
            }
        }
        if (dup) continue;
        wcsncpy_s(slots[n], CW_MAX_CHARS, g_hist[h], _TRUNCATE);
        n++;
    }
    return n;
}

int cw_history_pin_count(void) {
    return g_pin_count;
}

int cw_history_history_count(void) {
    return g_hist_count;
}

int cw_history_copy_pin(int index, wchar_t *out, size_t cch_out) {
    if (!out || cch_out == 0 || index < 0 || index >= g_pin_count) return 0;
    wcsncpy_s(out, cch_out, g_pin[index], _TRUNCATE);
    return 1;
}

int cw_history_copy_history(int index, wchar_t *out, size_t cch_out) {
    if (!out || cch_out == 0 || index < 0 || index >= g_hist_count) return 0;
    wcsncpy_s(out, cch_out, g_hist[index], _TRUNCATE);
    return 1;
}

void cw_history_load(void) {
    ensure_paths();
    g_pin_count = 0;
    g_hist_count = 0;
    FILE *f = NULL;
    wchar_t bak[MAX_PATH];
    swprintf_s(bak, MAX_PATH, L"%s.bak", g_data_file);
    if (_wfopen_s(&f, g_data_file, L"rb") != 0 || !f) {
        if (_wfopen_s(&f, bak, L"rb") != 0 || !f) return;
    }
    uint32_t magic = 0, ver = 0, np = 0, nh = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != MAGIC) {
        fclose(f);
        if (_wfopen_s(&f, bak, L"rb") != 0 || !f) return;
        if (fread(&magic, 4, 1, f) != 1 || magic != MAGIC) {
            fclose(f);
            return;
        }
    }
    if (fread(&ver, 4, 1, f) != 1 || ver != VER) {
        fclose(f);
        if (_wfopen_s(&f, bak, L"rb") != 0 || !f) return;
        if (fread(&magic, 4, 1, f) != 1 || magic != MAGIC) {
            fclose(f);
            return;
        }
        if (fread(&ver, 4, 1, f) != 1 || ver != VER) {
            fclose(f);
            return;
        }
    }
    if (fread(&np, 4, 1, f) != 1 || fread(&nh, 4, 1, f) != 1) {
        fclose(f);
        return;
    }
    if (np > CW_MAX_PIN) np = CW_MAX_PIN;
    if (nh > CW_MAX_HIST) nh = CW_MAX_HIST;
    for (uint32_t i = 0; i < np; i++) {
        uint32_t bl = 0;
        if (fread(&bl, 4, 1, f) != 1) break;
        if (bl == 0 || (bl % (uint32_t)sizeof(wchar_t)) != 0) break;
        if (bl > (CW_MAX_CHARS - 1) * (uint32_t)sizeof(wchar_t)) break;
        if (fread(g_pin[g_pin_count], 1, bl, f) != bl) break;
        size_t nch = bl / sizeof(wchar_t);
        if (nch >= CW_MAX_CHARS) nch = CW_MAX_CHARS - 1;
        g_pin[g_pin_count][nch] = 0;
        g_pin_count++;
    }
    for (uint32_t i = 0; i < nh; i++) {
        uint32_t bl = 0;
        if (fread(&bl, 4, 1, f) != 1) break;
        if (bl == 0 || (bl % (uint32_t)sizeof(wchar_t)) != 0) break;
        if (bl > (CW_MAX_CHARS - 1) * (uint32_t)sizeof(wchar_t)) break;
        if (fread(g_hist[g_hist_count], 1, bl, f) != bl) break;
        size_t nch = bl / sizeof(wchar_t);
        if (nch >= CW_MAX_CHARS) nch = CW_MAX_CHARS - 1;
        g_hist[g_hist_count][nch] = 0;
        g_hist_count++;
    }
    fclose(f);
}

static int cw_history_save_internal(void) {
    ensure_paths();
    wchar_t tmp[MAX_PATH];
    wchar_t bak[MAX_PATH];
    swprintf_s(tmp, MAX_PATH, L"%s.%lu.tmp", g_data_file, (unsigned long)GetCurrentProcessId());
    swprintf_s(bak, MAX_PATH, L"%s.bak", g_data_file);
    DeleteFileW(tmp);
    FILE *f = NULL;
    if (_wfopen_s(&f, tmp, L"wb") != 0 || !f) {
        int e = errno;
        cw_logf(L"[save] open tmp failed errno=%d path=%s", e, tmp);
        return 0;
    }
    uint32_t magic = MAGIC, ver = VER;
    uint32_t np = (uint32_t)g_pin_count, nh = (uint32_t)g_hist_count;
    if (fwrite(&magic, 4, 1, f) != 1 || fwrite(&ver, 4, 1, f) != 1 || fwrite(&np, 4, 1, f) != 1 ||
        fwrite(&nh, 4, 1, f) != 1) {
        fclose(f);
        DeleteFileW(tmp);
        cw_logf(L"[save] header write failed");
        return 0;
    }
    for (int i = 0; i < g_pin_count; i++) {
        uint32_t bl = (uint32_t)((wcslen(g_pin[i]) + 1) * sizeof(wchar_t));
        if (fwrite(&bl, 4, 1, f) != 1 || fwrite(g_pin[i], 1, bl, f) != bl) {
            fclose(f);
            DeleteFileW(tmp);
            cw_logf(L"[save] pin write failed");
            return 0;
        }
    }
    for (int i = 0; i < g_hist_count; i++) {
        uint32_t bl = (uint32_t)((wcslen(g_hist[i]) + 1) * sizeof(wchar_t));
        if (fwrite(&bl, 4, 1, f) != 1 || fwrite(g_hist[i], 1, bl, f) != bl) {
            fclose(f);
            DeleteFileW(tmp);
            cw_logf(L"[save] hist write failed");
            return 0;
        }
    }
    fflush(f);
    fclose(f);
    if (!ReplaceFileW(g_data_file, tmp, bak, REPLACEFILE_WRITE_THROUGH, NULL, NULL)) {
        DWORD e = GetLastError();
        if (!MoveFileExW(tmp, g_data_file, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(tmp);
            cw_logf(L"[save] MoveFileEx failed=%lu", (unsigned long)GetLastError());
            return 0;
        }
        cw_logf(L"[save] ReplaceFile failed=%lu", (unsigned long)e);
    }
    return 1;
}

void cw_history_save(void) {
    (void)cw_history_save_internal();
}

int cw_history_selftest(void) {
    ensure_paths();
    g_pin_count = 0;
    g_hist_count = 0;
    DeleteFileW(g_data_file);
    {
        wchar_t err[256];
        if (!cw_history_pin_text_ex(L"AAA", err, ARRAYSIZE(err))) return 0;
        if (g_pin_count != 1) return 0;
        if (wcscmp(g_pin[0], L"AAA") != 0) return 0;
        if (!cw_history_pin_text_ex(L"BBB", err, ARRAYSIZE(err))) return 0;
        if (g_pin_count != 2) return 0;
        if (wcscmp(g_pin[0], L"BBB") != 0) return 0;
        if (!cw_history_pin_text_ex(L"AAA", err, ARRAYSIZE(err))) return 0;
        if (g_pin_count != 3) return 0;
        if (wcscmp(g_pin[0], L"AAA") != 0) return 0;
    }
    cw_history_save();
    g_pin_count = 0;
    g_hist_count = 0;
    cw_history_load();
    if (g_pin_count != 3) return 0;
    if (wcscmp(g_pin[0], L"AAA") != 0) return 0;
    if (wcscmp(g_pin[1], L"BBB") != 0) return 0;
    if (wcscmp(g_pin[2], L"AAA") != 0) return 0;

    cw_history_unpin_display_index(1);
    if (g_pin_count != 2) return 0;
    if (wcscmp(g_pin[0], L"AAA") != 0) return 0;
    if (wcscmp(g_pin[1], L"AAA") != 0) return 0;

    cw_history_clear_all_pins();
    {
        wchar_t err[256];
        if (!cw_history_pin_text_at(L"ONE", 0, err, ARRAYSIZE(err))) return 0;
        if (!cw_history_pin_text_at(L"TWO", 0, err, ARRAYSIZE(err))) return 0;
        if (!cw_history_pin_text_at(L"THREE", 2, err, ARRAYSIZE(err))) return 0;
        if (g_pin_count != 3) return 0;
        if (wcscmp(g_pin[0], L"TWO") != 0) return 0;
        if (wcscmp(g_pin[1], L"ONE") != 0) return 0;
        if (wcscmp(g_pin[2], L"THREE") != 0) return 0;
        if (!cw_history_reorder_pin(2, 0)) return 0;
        if (wcscmp(g_pin[0], L"THREE") != 0) return 0;
        if (wcscmp(g_pin[1], L"TWO") != 0) return 0;
        if (wcscmp(g_pin[2], L"ONE") != 0) return 0;
    }

    cw_history_clear_all_pins();
    if (g_pin_count != 0) return 0;

    cw_history_on_copy(L"hist-alpha");
    cw_history_on_copy(L"hist-beta");
    cw_history_on_copy(L"hist-gamma");
    if (g_hist_count != 3) return 0;
    if (wcscmp(g_hist[0], L"hist-gamma") != 0) return 0;
    if (wcscmp(g_hist[1], L"hist-beta") != 0) return 0;

    cw_history_on_copy(L"hist-gamma");
    if (g_hist_count != 3) return 0;

    {
        wchar_t slots[CW_MAX_SLOT][CW_MAX_CHARS];
        int n = cw_history_fill_slots(slots);
        if (n != 3) return 0;
        if (wcscmp(slots[0], L"hist-gamma") != 0) return 0;
        if (wcscmp(slots[1], L"hist-beta") != 0) return 0;
        if (wcscmp(slots[2], L"hist-alpha") != 0) return 0;
    }

    cw_history_delete_history_index(1);
    if (g_hist_count != 2) return 0;
    if (wcscmp(g_hist[0], L"hist-gamma") != 0) return 0;
    if (wcscmp(g_hist[1], L"hist-alpha") != 0) return 0;

    cw_history_delete_history_index(0);
    cw_history_delete_history_index(0);
    if (g_hist_count != 0) return 0;

    return 1;
}
