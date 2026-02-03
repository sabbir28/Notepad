// ui.c – NotepadLite – Fixed & Perfectly Compilable Version (MinGW-w64)
#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include "ui.h"
#include "file_io.h"
#include "localization.h"

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void CreateStatusBarParts(void);
static void UpdateStatusBar(void);
static void ResizeControls(HWND hwnd);
static const WCHAR *GetDisplayNameFromPath(LPCWSTR path);
static void OpenFileFromPath(HWND hwnd, LPCWSTR path);
static void LocalizeMenu(HWND hwnd);

// Global handles
HWND g_hMainWnd   = NULL;
HWND g_hEditor    = NULL;
HWND g_hStatusBar = NULL;

FILE_ENCODING g_currentFileEncoding = ENC_UTF8;
WCHAR g_filePath[MAX_PATH] = {0};

int ui_run(HINSTANCE hInst, int nCmdShow, LPCWSTR startup_path)
{
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    LoadLibraryW(L"Msftedit.dll");  // RichEdit 4.1+

    const WCHAR CLASS_NAME[] = L"NotepadLite";

    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCEW(IDR_MAINICON));
    wc.hIconSm       = wc.hIcon;

    if (!RegisterClassExW(&wc)) return -1;

    g_hMainWnd = CreateWindowExW(
        0, CLASS_NAME, loc_wstr(LOC_APP_TITLE),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        NULL, NULL, hInst, (LPVOID)startup_path);

    if (!g_hMainWnd) return -1;

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    HACCEL hAccel = LoadAcceleratorsW(hInst, MAKEINTRESOURCEW(IDR_ACCELERATORS));

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!hAccel || !TranslateAcceleratorW(g_hMainWnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}

static const WCHAR *GetDisplayNameFromPath(LPCWSTR path)
{
    if (!path || !path[0])
        return loc_wstr(LOC_APP_TITLE);

    const WCHAR *slash = wcsrchr(path, L'\\');
    const WCHAR *altSlash = wcsrchr(path, L'/');
    const WCHAR *sep = slash;
    if (altSlash && (!sep || altSlash > sep))
        sep = altSlash;

    return sep ? (sep + 1) : path;
}

static void OpenFileFromPath(HWND hwnd, LPCWSTR path)
{
    if (!path || !path[0])
        return;

    wcscpy_s(g_filePath, _countof(g_filePath), path);
    FILE_ENCODING enc = file_detect_encoding(path);
    LPWSTR buffer = NULL;
    DWORD size = 0;
    if (file_read(hwnd, path, &buffer, &size, &enc)) {
        SetWindowTextW(g_hEditor, buffer);
        g_currentFileEncoding = enc;
        HeapFree(GetProcessHeap(), 0, buffer);
        SetWindowTextW(hwnd, GetDisplayNameFromPath(path));
    } else {
        g_filePath[0] = L'\0';
    }
    UpdateStatusBar();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)lParam;
        LPCWSTR startupFile = (LPCWSTR)pcs->lpCreateParams;

        // RichEdit control
        g_hEditor = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            MSFTEDIT_CLASS,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
            ES_NOHIDESEL | ES_WANTRETURN,
            0, 0, 0, 0,
            hwnd, NULL, pcs->hInstance, NULL);

        CHARFORMATW cf = {0};
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_FACE | CFM_SIZE;
        cf.yHeight = 220;                       // 11 pt
        wcscpy_s(cf.szFaceName, LF_FACESIZE, L"Consolas");
        SendMessageW(g_hEditor, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

        SendMessageW(g_hEditor, EM_EXLIMITTEXT, 0, (LPARAM)0x7FFFFFFE);
        SendMessageW(g_hEditor, EM_SETEVENTMASK, 0, ENM_SELCHANGE | ENM_CHANGE);

        // Status bar
        g_hStatusBar = CreateWindowExW(
            0, STATUSCLASSNAMEW, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hwnd, (HMENU)ID_STATUSBAR, pcs->hInstance, NULL);

        CreateStatusBarParts();

        // Menu
        HMENU hMenu = LoadMenuW(pcs->hInstance, MAKEINTRESOURCEW(IDR_MYMENU));
        SetMenu(hwnd, hMenu);
        LocalizeMenu(hwnd);

        DragAcceptFiles(hwnd, TRUE);

        // Open file passed on command line
        if (startupFile && startupFile[0]) {
            OpenFileFromPath(hwnd, startupFile);
        }

        UpdateStatusBar();
        return 0;
    }

    case WM_NOTIFY:
        if (((NMHDR*)lParam)->hwndFrom == g_hEditor &&
            ((NMHDR*)lParam)->code == EN_SELCHANGE)
            UpdateStatusBar();
        return 0;

    case WM_SIZE:
        ResizeControls(hwnd);
        return 0;

    case WM_DROPFILES:
    {
        HDROP drop = (HDROP)wParam;
        WCHAR path[MAX_PATH] = {0};
        if (DragQueryFileW(drop, 0, path, _countof(path))) {
            OpenFileFromPath(hwnd, path);
        }
        DragFinish(drop);
        return 0;
    }

    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 500;
        ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 380;
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_FILE_NEW:
            SetWindowTextW(g_hEditor, L"");
            g_filePath[0] = L'\0';
            g_currentFileEncoding = ENC_UTF8;
            SetWindowTextW(hwnd, loc_wstr(LOC_APP_TITLE));
            UpdateStatusBar();
            break;

        case IDM_FILE_OPEN:
            if (file_open_dialog(hwnd, g_filePath, MAX_PATH)) {
                OpenFileFromPath(hwnd, g_filePath);
            }
            UpdateStatusBar();
            break;

        case IDM_FILE_SAVE:
            if (g_filePath[0] == L'\0')
                goto do_save_as;
            // fall through
        {
            DWORD len = (DWORD)GetWindowTextLengthW(g_hEditor);
            LPWSTR text = (len == 0) ? NULL : HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
            if (len == 0 || text) {
                if (text) GetWindowTextW(g_hEditor, text, len + 1);
                file_write(hwnd, g_filePath, text ? text : L"", len * sizeof(WCHAR), g_currentFileEncoding);
                if (text) HeapFree(GetProcessHeap(), 0, text);
            }
            UpdateStatusBar();
            break;
        }

        case IDM_FILE_SAVEAS:
        do_save_as:
        {
            WCHAR newPath[MAX_PATH] = {0};
            if (file_save_dialog(hwnd, newPath, MAX_PATH)) {
                DWORD len = (DWORD)GetWindowTextLengthW(g_hEditor);
                LPWSTR buf = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
                if (buf) {
                    GetWindowTextW(g_hEditor, buf, len + 1);
                    if (file_write(hwnd, newPath, buf, len * sizeof(WCHAR), g_currentFileEncoding)) {
                        wcscpy_s(g_filePath, _countof(g_filePath), newPath);
                        SetWindowTextW(hwnd, GetDisplayNameFromPath(newPath));
                    }
                    HeapFree(GetProcessHeap(), 0, buf);
                }
            }
            UpdateStatusBar();
            break;
        }

        case IDM_TOGGLE_ALWAYSONTOP:
        {
            BOOL topmost = (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;

            SetWindowPos(hwnd, topmost ? HWND_NOTOPMOST : HWND_TOPMOST,0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            CheckMenuItem(GetMenu(hwnd), IDM_TOGGLE_ALWAYSONTOP,topmost ? MF_UNCHECKED : MF_CHECKED);
            break;
        }

        case IDM_LANG_ENGLISH:
            localization_set_bangla(false);
            LocalizeMenu(hwnd);
            if (g_filePath[0] == L'\0') {
                SetWindowTextW(hwnd, loc_wstr(LOC_APP_TITLE));
            }
            UpdateStatusBar();
            break;

        case IDM_LANG_BANGLA:
            localization_set_bangla(true);
            LocalizeMenu(hwnd);
            if (g_filePath[0] == L'\0') {
                SetWindowTextW(hwnd, loc_wstr(LOC_APP_TITLE));
            }
            UpdateStatusBar();
            break;

        case IDM_FILE_EXIT:
            DestroyWindow(hwnd);
            break;

        case IDM_EDIT_UNDO:       SendMessageW(g_hEditor, EM_UNDO, 0, 0); break;
        case IDM_EDIT_CUT:        SendMessageW(g_hEditor, WM_CUT, 0, 0); break;
        case IDM_EDIT_COPY:       SendMessageW(g_hEditor, WM_COPY, 0, 0); break;
        case IDM_EDIT_PASTE:      SendMessageW(g_hEditor, WM_PASTE, 0, 0); break;
        case IDM_EDIT_SELECT:  SendMessageW(g_hEditor, EM_SETSEL, 0, -1); break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void LocalizeMenu(HWND hwnd)
{
    HMENU menu = GetMenu(hwnd);
    if (!menu) {
        return;
    }

    MENUITEMINFOW mii = {0};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_FILE);
    SetMenuItemInfoW(menu, 0, TRUE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_EDIT);
    SetMenuItemInfoW(menu, 1, TRUE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_VIEW);
    SetMenuItemInfoW(menu, 2, TRUE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_NEW);
    SetMenuItemInfoW(menu, IDM_FILE_NEW, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_OPEN);
    SetMenuItemInfoW(menu, IDM_FILE_OPEN, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_SAVE);
    SetMenuItemInfoW(menu, IDM_FILE_SAVE, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_SAVE_AS);
    SetMenuItemInfoW(menu, IDM_FILE_SAVEAS, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_EXIT);
    SetMenuItemInfoW(menu, IDM_FILE_EXIT, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_UNDO);
    SetMenuItemInfoW(menu, IDM_EDIT_UNDO, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_CUT);
    SetMenuItemInfoW(menu, IDM_EDIT_CUT, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_COPY);
    SetMenuItemInfoW(menu, IDM_EDIT_COPY, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_PASTE);
    SetMenuItemInfoW(menu, IDM_EDIT_PASTE, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_SELECT_ALL);
    SetMenuItemInfoW(menu, IDM_EDIT_SELECT, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_ALWAYS_ON_TOP);
    SetMenuItemInfoW(menu, IDM_TOGGLE_ALWAYSONTOP, FALSE, &mii);

    HMENU viewMenu = GetSubMenu(menu, 2);
    if (viewMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANGUAGE);
        SetMenuItemInfoW(viewMenu, 2, TRUE, &mii);
    }

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANG_ENGLISH);
    SetMenuItemInfoW(menu, IDM_LANG_ENGLISH, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANG_BANGLA);
    SetMenuItemInfoW(menu, IDM_LANG_BANGLA, FALSE, &mii);

    if (localization_is_bangla()) {
        CheckMenuItem(menu, IDM_LANG_BANGLA, MF_CHECKED);
        CheckMenuItem(menu, IDM_LANG_ENGLISH, MF_UNCHECKED);
    } else {
        CheckMenuItem(menu, IDM_LANG_BANGLA, MF_UNCHECKED);
        CheckMenuItem(menu, IDM_LANG_ENGLISH, MF_CHECKED);
    }
}

// ------------------------------------------------------------------
// Status bar helpers
// ------------------------------------------------------------------
static void CreateStatusBarParts(void)
{
    int parts[] = { 300, 500, -1 };
    SendMessageW(g_hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
}

static void ResizeControls(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    if (g_hStatusBar) {
        SendMessageW(g_hStatusBar, WM_SIZE, 0, 0);
        RECT rcSB;
        GetWindowRect(g_hStatusBar, &rcSB);
        int sbHeight = rcSB.bottom - rcSB.top;
        rc.bottom -= sbHeight;
    }

    if (g_hEditor)
        SetWindowPos(g_hEditor, NULL, rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top,
                     SWP_NOZORDER);
}

static void UpdateStatusBar(void)
{
    if (!g_hEditor || !g_hStatusBar) return;

    // --- Part 0: Line & Column ---
    DWORD start, end;
    SendMessageW(g_hEditor, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    int line = (int)SendMessageW(g_hEditor, EM_EXLINEFROMCHAR, 0, (LPARAM)start) + 1;
    int col  = (int)(start - SendMessageW(g_hEditor, EM_LINEINDEX, line - 1, 0)) + 1;

    WCHAR buf[128];
    swprintf_s(buf, _countof(buf), loc_wstr(LOC_STATUS_LN_COL), line, col);
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 0, (LPARAM)buf);

    // --- Part 1: File size ---
    DWORD len = GetWindowTextLengthW(g_hEditor);
    double size = (double)len;
    const WCHAR *unit = L"B";
    if (size >= 1024*1024)      { size /= (1024*1024); unit = L"MB"; }
    else if (size >= 1024)      { size /= 1024;       unit = L"KB"; }

    swprintf_s(buf, _countof(buf), loc_wstr(LOC_STATUS_SIZE), size, unit);
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 1, (LPARAM)buf);

    // --- Part 2: Encoding ---
    const WCHAR *enc = loc_wstr(LOC_STATUS_ENCODING_UNKNOWN);
    switch (g_currentFileEncoding) {
        case ENC_UTF8:      enc = loc_wstr(LOC_STATUS_ENCODING_UTF8);      break;
        case ENC_UTF16_LE:  enc = loc_wstr(LOC_STATUS_ENCODING_UTF16_LE);  break;
        case ENC_UTF16_BE:  enc = loc_wstr(LOC_STATUS_ENCODING_UTF16_BE);  break;
        case ENC_ANSI:      enc = loc_wstr(LOC_STATUS_ENCODING_ANSI);      break;
    }
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 2, (LPARAM)enc);
}
