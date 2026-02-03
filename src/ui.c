// ui.c – NotepadLite – Fixed & Perfectly Compilable Version (MinGW-w64)
#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdbool.h>
#include <wctype.h>
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
static void UpdateRecentFilesMenu(HWND hwnd);
static void ApplySyntaxHighlighting(void);
static void ScheduleSyntaxHighlighting(void);
static BOOL ShowGoToDialog(HWND hwnd);
static void StartNewWindow(HWND hwnd);
static BOOL PrintDocument(HWND hwnd, BOOL toPdf);

typedef enum {
    LINE_ENDING_CRLF,
    LINE_ENDING_LF
} LINE_ENDING;

typedef enum {
    LANGUAGE_MODE_PLAIN,
    LANGUAGE_MODE_C
} LANGUAGE_MODE;

typedef struct {
    WCHAR *text;
    DWORD textLen;
    WCHAR path[MAX_PATH];
    FILE_ENCODING encoding;
    LINE_ENDING lineEnding;
    bool dirty;
} Document;

static Document *g_documents = NULL;
static size_t g_documentCount = 0;
static size_t g_activeDocument = 0;
static bool g_autosaveEnabled = true;
static bool g_syntaxHighlightEnabled = false;
static LANGUAGE_MODE g_languageMode = LANGUAGE_MODE_PLAIN;
static WCHAR g_recentFiles[IDM_RECENT_FILE_MAX - IDM_RECENT_FILE_BASE + 1][MAX_PATH];
static size_t g_recentCount = 0;
static UINT g_findMessage = 0;
static HWND g_findDialog = NULL;
static WCHAR g_findText[128] = {0};
static WCHAR g_replaceText[128] = {0};
static LINE_ENDING g_currentLineEnding = LINE_ENDING_CRLF;
static WCHAR g_autosavePath[MAX_PATH] = {0};
static FINDREPLACEW g_findReplace = {0};

// Global handles
HWND g_hMainWnd   = NULL;
HWND g_hEditor    = NULL;
HWND g_hTab       = NULL;
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

static Document *GetActiveDocument(void)
{
    if (g_documentCount == 0) {
        return NULL;
    }
    return &g_documents[g_activeDocument];
}

static void UpdateCurrentDocumentState(Document *doc)
{
    if (!doc) {
        g_filePath[0] = L'\0';
        g_currentFileEncoding = ENC_UTF8;
        g_currentLineEnding = LINE_ENDING_CRLF;
        return;
    }
    wcscpy_s(g_filePath, _countof(g_filePath), doc->path);
    g_currentFileEncoding = doc->encoding;
    g_currentLineEnding = doc->lineEnding;
}

static void StoreEditorText(Document *doc)
{
    if (!doc || !g_hEditor) {
        return;
    }

    DWORD len = (DWORD)GetWindowTextLengthW(g_hEditor);
    WCHAR *buffer = NULL;
    if (len > 0) {
        buffer = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
        if (!buffer) {
            return;
        }
        GetWindowTextW(g_hEditor, buffer, len + 1);
    } else {
        buffer = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR));
        if (!buffer) {
            return;
        }
        buffer[0] = L'\0';
    }

    if (doc->text) {
        HeapFree(GetProcessHeap(), 0, doc->text);
    }
    doc->text = buffer;
    doc->textLen = len;
}

static void LoadEditorText(const Document *doc)
{
    if (!g_hEditor) {
        return;
    }
    if (doc && doc->text) {
        SetWindowTextW(g_hEditor, doc->text);
    } else {
        SetWindowTextW(g_hEditor, L"");
    }
    SendMessageW(g_hEditor, EM_SETSEL, 0, 0);
    SendMessageW(g_hEditor, EM_SCROLLCARET, 0, 0);
}

static void UpdateTabTitle(size_t index)
{
    if (!g_hTab || index >= g_documentCount) {
        return;
    }

    WCHAR title[MAX_PATH + 32] = {0};
    const Document *doc = &g_documents[index];
    if (doc->path[0]) {
        wcscpy_s(title, _countof(title), GetDisplayNameFromPath(doc->path));
    } else {
        swprintf_s(title, _countof(title), L"Untitled %zu", index + 1);
    }

    TCITEMW item = {0};
    item.mask = TCIF_TEXT;
    item.pszText = title;
    TabCtrl_SetItem(g_hTab, (int)index, &item);
}

static void AddDocument(BOOL select)
{
    Document *newDocs = (Document *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, g_documents,
                                                (g_documentCount + 1) * sizeof(Document));
    if (!newDocs) {
        return;
    }
    g_documents = newDocs;
    Document *doc = &g_documents[g_documentCount];
    ZeroMemory(doc, sizeof(*doc));
    doc->encoding = ENC_UTF8;
    doc->lineEnding = LINE_ENDING_CRLF;
    doc->dirty = false;
    g_documentCount++;

    if (g_hTab) {
        TCITEMW item = {0};
        item.mask = TCIF_TEXT;
        item.pszText = L"Untitled";
        TabCtrl_InsertItem(g_hTab, (int)(g_documentCount - 1), &item);
        UpdateTabTitle(g_documentCount - 1);
    }

    if (select) {
        if (g_documentCount > 1) {
            StoreEditorText(&g_documents[g_activeDocument]);
        }
        g_activeDocument = g_documentCount - 1;
        UpdateCurrentDocumentState(&g_documents[g_activeDocument]);
        LoadEditorText(&g_documents[g_activeDocument]);
        if (g_hTab) {
            TabCtrl_SetCurSel(g_hTab, (int)g_activeDocument);
        }
    }
}

static void CloseActiveDocument(void)
{
    if (g_documentCount == 0) {
        return;
    }
    if (g_documentCount == 1) {
        Document *doc = &g_documents[0];
        if (doc->text) {
            HeapFree(GetProcessHeap(), 0, doc->text);
        }
        ZeroMemory(doc, sizeof(*doc));
        doc->encoding = ENC_UTF8;
        doc->lineEnding = LINE_ENDING_CRLF;
        doc->dirty = false;
        LoadEditorText(doc);
        UpdateTabTitle(0);
        UpdateCurrentDocumentState(doc);
        UpdateStatusBar();
        return;
    }

    Document *doc = &g_documents[g_activeDocument];
    if (doc->text) {
        HeapFree(GetProcessHeap(), 0, doc->text);
    }

    for (size_t i = g_activeDocument; i + 1 < g_documentCount; ++i) {
        g_documents[i] = g_documents[i + 1];
    }
    g_documentCount--;
    Document *newDocs = (Document *)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, g_documents,
                                                g_documentCount * sizeof(Document));
    if (newDocs || g_documentCount == 0) {
        g_documents = newDocs;
    }

    if (g_hTab) {
        TabCtrl_DeleteItem(g_hTab, (int)g_activeDocument);
    }
    if (g_activeDocument >= g_documentCount) {
        g_activeDocument = g_documentCount - 1;
    }
    UpdateCurrentDocumentState(&g_documents[g_activeDocument]);
    LoadEditorText(&g_documents[g_activeDocument]);
    if (g_hTab) {
        TabCtrl_SetCurSel(g_hTab, (int)g_activeDocument);
    }
    UpdateStatusBar();
}

static WCHAR *GetEditorTextWithLineEnding(LINE_ENDING lineEnding, DWORD *outLenChars)
{
    DWORD len = (DWORD)GetWindowTextLengthW(g_hEditor);
    WCHAR *buffer = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!buffer) {
        return NULL;
    }
    if (len > 0) {
        GetWindowTextW(g_hEditor, buffer, len + 1);
    } else {
        buffer[0] = L'\0';
    }

    if (lineEnding == LINE_ENDING_CRLF) {
        if (outLenChars) {
            *outLenChars = len;
        }
        return buffer;
    }

    WCHAR *converted = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!converted) {
        HeapFree(GetProcessHeap(), 0, buffer);
        return NULL;
    }

    DWORD outLen = 0;
    for (DWORD i = 0; i < len; ++i) {
        if (buffer[i] == L'\r' && i + 1 < len && buffer[i + 1] == L'\n') {
            continue;
        }
        converted[outLen++] = buffer[i];
    }
    converted[outLen] = L'\0';
    HeapFree(GetProcessHeap(), 0, buffer);
    if (outLenChars) {
        *outLenChars = outLen;
    }
    return converted;
}

static void AddRecentFile(LPCWSTR path)
{
    if (!path || !path[0]) {
        return;
    }
    for (size_t i = 0; i < g_recentCount; ++i) {
        if (_wcsicmp(g_recentFiles[i], path) == 0) {
            for (size_t j = i; j > 0; --j) {
                wcscpy_s(g_recentFiles[j], _countof(g_recentFiles[j]), g_recentFiles[j - 1]);
            }
            wcscpy_s(g_recentFiles[0], _countof(g_recentFiles[0]), path);
            return;
        }
    }

    size_t insertAt = g_recentCount < _countof(g_recentFiles) ? g_recentCount : _countof(g_recentFiles) - 1;
    if (g_recentCount < _countof(g_recentFiles)) {
        g_recentCount++;
    }
    for (size_t i = insertAt; i > 0; --i) {
        wcscpy_s(g_recentFiles[i], _countof(g_recentFiles[i]), g_recentFiles[i - 1]);
    }
    wcscpy_s(g_recentFiles[0], _countof(g_recentFiles[0]), path);
}

static BOOL SaveActiveDocument(HWND hwnd, BOOL saveAs)
{
    Document *doc = GetActiveDocument();
    if (!doc) {
        return FALSE;
    }

    WCHAR targetPath[MAX_PATH] = {0};
    if (saveAs || doc->path[0] == L'\0') {
        if (!file_save_dialog(hwnd, targetPath, MAX_PATH)) {
            return FALSE;
        }
        wcscpy_s(doc->path, _countof(doc->path), targetPath);
    } else {
        wcscpy_s(targetPath, _countof(targetPath), doc->path);
    }

    DWORD lenChars = 0;
    WCHAR *text = GetEditorTextWithLineEnding(doc->lineEnding, &lenChars);
    if (!text) {
        return FALSE;
    }
    BOOL ok = file_write(hwnd, targetPath, text, lenChars * sizeof(WCHAR), doc->encoding);
    HeapFree(GetProcessHeap(), 0, text);

    if (ok) {
        doc->dirty = false;
        UpdateCurrentDocumentState(doc);
        SetWindowTextW(hwnd, GetDisplayNameFromPath(doc->path));
        UpdateTabTitle(g_activeDocument);
        AddRecentFile(doc->path);
        UpdateRecentFilesMenu(hwnd);
        if (g_autosavePath[0]) {
            DeleteFileW(g_autosavePath);
        }
    }
    UpdateStatusBar();
    return ok;
}

static BOOL FindNextText(DWORD flags)
{
    if (!g_findText[0]) {
        return FALSE;
    }
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(g_hEditor, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);

    FINDTEXTEXW ft = {0};
    ft.lpstrText = g_findText;
    ft.chrg.cpMin = (LONG)end;
    ft.chrg.cpMax = -1;

    LRESULT res = SendMessageW(g_hEditor, EM_FINDTEXTEXW, (WPARAM)flags, (LPARAM)&ft);
    if (res == -1) {
        ft.chrg.cpMin = 0;
        ft.chrg.cpMax = (LONG)start;
        res = SendMessageW(g_hEditor, EM_FINDTEXTEXW, (WPARAM)flags, (LPARAM)&ft);
    }

    if (res == -1) {
        MessageBeep(MB_ICONWARNING);
        return FALSE;
    }

    SendMessageW(g_hEditor, EM_SETSEL, ft.chrgText.cpMin, ft.chrgText.cpMax);
    SendMessageW(g_hEditor, EM_SCROLLCARET, 0, 0);
    return TRUE;
}

static BOOL SelectionMatchesFind(DWORD flags)
{
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(g_hEditor, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    if (start == end) {
        return FALSE;
    }
    DWORD len = end - start;
    WCHAR *selText = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!selText) {
        return FALSE;
    }
    SendMessageW(g_hEditor, EM_GETSELTEXT, 0, (LPARAM)selText);
    selText[len] = L'\0';

    BOOL match = FALSE;
    if (flags & FR_MATCHCASE) {
        match = (wcscmp(selText, g_findText) == 0);
    } else {
        match = (_wcsicmp(selText, g_findText) == 0);
    }
    HeapFree(GetProcessHeap(), 0, selText);
    return match;
}

static void ReplaceSelectionAndFindNext(DWORD flags)
{
    if (SelectionMatchesFind(flags)) {
        SendMessageW(g_hEditor, EM_REPLACESEL, TRUE, (LPARAM)g_replaceText);
    }
    FindNextText(flags);
}

static void ReplaceAllText(DWORD flags)
{
    SendMessageW(g_hEditor, EM_SETSEL, 0, 0);
    while (FindNextText(flags)) {
        SendMessageW(g_hEditor, EM_REPLACESEL, TRUE, (LPARAM)g_replaceText);
    }
}

static LINE_ENDING DetectLineEnding(LPCWSTR text)
{
    if (!text) {
        return LINE_ENDING_CRLF;
    }
    const WCHAR *crlf = wcsstr(text, L"\r\n");
    if (crlf) {
        return LINE_ENDING_CRLF;
    }
    const WCHAR *lf = wcschr(text, L'\n');
    if (lf) {
        return LINE_ENDING_LF;
    }
    return LINE_ENDING_CRLF;
}

static void OpenFileFromPath(HWND hwnd, LPCWSTR path)
{
    if (!path || !path[0])
        return;

    FILE_ENCODING enc = file_detect_encoding(path);
    LPWSTR buffer = NULL;
    DWORD size = 0;
    if (file_read(hwnd, path, &buffer, &size, &enc)) {
        Document *doc = GetActiveDocument();
        if (doc && (doc->textLen > 0 || doc->dirty || doc->path[0])) {
            AddDocument(TRUE);
            doc = GetActiveDocument();
        }
        if (!doc) {
            if (buffer) {
                GlobalFree(buffer);
            }
            return;
        }

        wcscpy_s(doc->path, _countof(doc->path), path);
        doc->encoding = enc;
        doc->lineEnding = DetectLineEnding(buffer);
        doc->dirty = false;
        UpdateCurrentDocumentState(doc);
        LocalizeMenu(hwnd);
        SetWindowTextW(g_hEditor, buffer);
        StoreEditorText(doc);
        GlobalFree(buffer);
        SetWindowTextW(hwnd, GetDisplayNameFromPath(path));
        UpdateTabTitle(g_activeDocument);
    } else {
        g_filePath[0] = L'\0';
    }
    UpdateStatusBar();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == g_findMessage) {
        LPFINDREPLACEW fr = (LPFINDREPLACEW)lParam;
        if (fr->Flags & FR_DIALOGTERM) {
            g_findDialog = NULL;
            return 0;
        }
        if (fr->Flags & FR_FINDNEXT) {
            FindNextText(fr->Flags);
        } else if (fr->Flags & FR_REPLACE) {
            ReplaceSelectionAndFindNext(fr->Flags);
        } else if (fr->Flags & FR_REPLACEALL) {
            ReplaceAllText(fr->Flags);
        }
        return 0;
    }

    switch (msg)
    {
    case WM_CREATE:
    {
        LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)lParam;
        LPCWSTR startupFile = (LPCWSTR)pcs->lpCreateParams;

        g_findMessage = RegisterWindowMessageW(FINDMSGSTRING);

        g_hTab = CreateWindowExW(
            0,
            WC_TABCONTROLW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 0, 0,
            hwnd, NULL, pcs->hInstance, NULL);

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

        AddDocument(TRUE);

        if (GetTempPathW(_countof(g_autosavePath), g_autosavePath) > 0) {
            wcscat_s(g_autosavePath, _countof(g_autosavePath), L"NotepadLite.autosave.txt");
        }

        // Open file passed on command line
        if (startupFile && startupFile[0]) {
            OpenFileFromPath(hwnd, startupFile);
        } else if (g_autosavePath[0]) {
            DWORD attrs = GetFileAttributesW(g_autosavePath);
            if (attrs != INVALID_FILE_ATTRIBUTES) {
                if (MessageBoxW(hwnd, L"An autosave file was found. Restore it?", L"NotepadLite",
                                MB_ICONQUESTION | MB_YESNO) == IDYES) {
                    FILE_ENCODING enc = ENC_UTF8;
                    LPWSTR buffer = NULL;
                    DWORD size = 0;
                    if (file_read(hwnd, g_autosavePath, &buffer, &size, &enc)) {
                        Document *doc = GetActiveDocument();
                        if (doc) {
                            doc->encoding = enc;
                            doc->lineEnding = DetectLineEnding(buffer);
                            doc->dirty = true;
                            UpdateCurrentDocumentState(doc);
                            SetWindowTextW(g_hEditor, buffer);
                            StoreEditorText(doc);
                            UpdateTabTitle(g_activeDocument);
                        }
                        GlobalFree(buffer);
                    }
                }
            }
        }

        if (g_autosaveEnabled) {
            SetTimer(hwnd, 1, 60000, NULL);
        }

        UpdateStatusBar();
        return 0;
    }

    case WM_NOTIFY:
    {
        NMHDR *hdr = (NMHDR *)lParam;
        if (hdr->hwndFrom == g_hEditor) {
            if (hdr->code == EN_SELCHANGE) {
                UpdateStatusBar();
            } else if (hdr->code == EN_CHANGE) {
                Document *doc = GetActiveDocument();
                if (doc) {
                    doc->dirty = true;
                }
                ScheduleSyntaxHighlighting();
            }
        } else if (hdr->hwndFrom == g_hTab && hdr->code == TCN_SELCHANGE) {
            int sel = TabCtrl_GetCurSel(g_hTab);
            if (sel >= 0 && (size_t)sel < g_documentCount) {
                StoreEditorText(&g_documents[g_activeDocument]);
                g_activeDocument = (size_t)sel;
                UpdateCurrentDocumentState(&g_documents[g_activeDocument]);
                LoadEditorText(&g_documents[g_activeDocument]);
                UpdateStatusBar();
                LocalizeMenu(hwnd);
            }
        }
        return 0;
    }

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

    case WM_TIMER:
        if (wParam == 1 && g_autosaveEnabled && g_autosavePath[0]) {
            Document *doc = GetActiveDocument();
            if (doc && doc->dirty) {
                DWORD lenChars = 0;
                WCHAR *text = GetEditorTextWithLineEnding(doc->lineEnding, &lenChars);
                if (text) {
                    file_write(hwnd, g_autosavePath, text, lenChars * sizeof(WCHAR), doc->encoding);
                    HeapFree(GetProcessHeap(), 0, text);
                }
            }
            return 0;
        }
        if (wParam == 2) {
            KillTimer(hwnd, 2);
            ApplySyntaxHighlighting();
            return 0;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_FILE_NEW_TAB:
            AddDocument(TRUE);
            SetWindowTextW(hwnd, loc_wstr(LOC_APP_TITLE));
            UpdateStatusBar();
            break;

        case IDM_FILE_CLOSE_TAB:
            CloseActiveDocument();
            SetWindowTextW(hwnd, g_filePath[0] ? GetDisplayNameFromPath(g_filePath) : loc_wstr(LOC_APP_TITLE));
            break;

        case IDM_FILE_NEW:
            StartNewWindow(hwnd);
            break;

        case IDM_FILE_OPEN:
            if (file_open_dialog(hwnd, g_filePath, MAX_PATH)) {
                OpenFileFromPath(hwnd, g_filePath);
                AddRecentFile(g_filePath);
                UpdateRecentFilesMenu(hwnd);
            }
            UpdateStatusBar();
            break;

        case IDM_FILE_SAVE:
            SaveActiveDocument(hwnd, FALSE);
            break;

        case IDM_FILE_SAVEAS:
            SaveActiveDocument(hwnd, TRUE);
            break;

        case IDM_FILE_PRINT:
            PrintDocument(hwnd, FALSE);
            break;

        case IDM_FILE_EXPORT_PDF:
            PrintDocument(hwnd, TRUE);
            break;

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

        case IDM_OPTIONS_AUTOSAVE:
        {
            g_autosaveEnabled = !g_autosaveEnabled;
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_AUTOSAVE, g_autosaveEnabled ? MF_CHECKED : MF_UNCHECKED);
            if (g_autosaveEnabled) {
                SetTimer(hwnd, 1, 60000, NULL);
            } else {
                KillTimer(hwnd, 1);
            }
            break;
        }

        case IDM_OPTIONS_LINE_CRLF:
        case IDM_OPTIONS_LINE_LF:
        {
            Document *doc = GetActiveDocument();
            if (doc) {
                doc->lineEnding = (LOWORD(wParam) == IDM_OPTIONS_LINE_LF) ? LINE_ENDING_LF : LINE_ENDING_CRLF;
                g_currentLineEnding = doc->lineEnding;
            }
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_LINE_CRLF,
                          (g_currentLineEnding == LINE_ENDING_CRLF) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_LINE_LF,
                          (g_currentLineEnding == LINE_ENDING_LF) ? MF_CHECKED : MF_UNCHECKED);
            UpdateStatusBar();
            break;
        }

        case IDM_OPTIONS_ENC_UTF8:
        case IDM_OPTIONS_ENC_UTF16LE:
        case IDM_OPTIONS_ENC_UTF16BE:
        case IDM_OPTIONS_ENC_ANSI:
        {
            Document *doc = GetActiveDocument();
            if (doc) {
                switch (LOWORD(wParam)) {
                    case IDM_OPTIONS_ENC_UTF8: doc->encoding = ENC_UTF8; break;
                    case IDM_OPTIONS_ENC_UTF16LE: doc->encoding = ENC_UTF16_LE; break;
                    case IDM_OPTIONS_ENC_UTF16BE: doc->encoding = ENC_UTF16_BE; break;
                    case IDM_OPTIONS_ENC_ANSI: doc->encoding = ENC_ANSI; break;
                }
                g_currentFileEncoding = doc->encoding;
            }
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_ENC_UTF8,
                          (g_currentFileEncoding == ENC_UTF8) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_ENC_UTF16LE,
                          (g_currentFileEncoding == ENC_UTF16_LE) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_ENC_UTF16BE,
                          (g_currentFileEncoding == ENC_UTF16_BE) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), IDM_OPTIONS_ENC_ANSI,
                          (g_currentFileEncoding == ENC_ANSI) ? MF_CHECKED : MF_UNCHECKED);
            UpdateStatusBar();
            break;
        }

        case IDM_FILE_EXIT:
            DestroyWindow(hwnd);
            break;

        case IDM_EDIT_FIND:
        case IDM_EDIT_REPLACE:
        {
            ZeroMemory(&g_findReplace, sizeof(g_findReplace));
            g_findReplace.lStructSize = sizeof(g_findReplace);
            g_findReplace.hwndOwner = hwnd;
            g_findReplace.lpstrFindWhat = g_findText;
            g_findReplace.wFindWhatLen = (WORD)_countof(g_findText);
            g_findReplace.lpstrReplaceWith = g_replaceText;
            g_findReplace.wReplaceWithLen = (WORD)_countof(g_replaceText);
            g_findReplace.Flags = FR_DOWN;

            if (LOWORD(wParam) == IDM_EDIT_FIND) {
                g_findDialog = FindTextW(&g_findReplace);
            } else {
                g_findDialog = ReplaceTextW(&g_findReplace);
            }
            break;
        }

        case IDM_EDIT_GOTO:
            ShowGoToDialog(hwnd);
            break;

        case IDM_VIEW_SYNTAX_TOGGLE:
            g_syntaxHighlightEnabled = !g_syntaxHighlightEnabled;
            CheckMenuItem(GetMenu(hwnd), IDM_VIEW_SYNTAX_TOGGLE,
                          g_syntaxHighlightEnabled ? MF_CHECKED : MF_UNCHECKED);
            ApplySyntaxHighlighting();
            break;

        case IDM_VIEW_LANG_PLAIN:
        case IDM_VIEW_LANG_C:
            g_languageMode = (LOWORD(wParam) == IDM_VIEW_LANG_C) ? LANGUAGE_MODE_C : LANGUAGE_MODE_PLAIN;
            CheckMenuItem(GetMenu(hwnd), IDM_VIEW_LANG_PLAIN,
                          (g_languageMode == LANGUAGE_MODE_PLAIN) ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem(GetMenu(hwnd), IDM_VIEW_LANG_C,
                          (g_languageMode == LANGUAGE_MODE_C) ? MF_CHECKED : MF_UNCHECKED);
            ApplySyntaxHighlighting();
            break;

        default:
            if (LOWORD(wParam) >= IDM_RECENT_FILE_BASE &&
                LOWORD(wParam) <= IDM_RECENT_FILE_MAX) {
                int index = (int)(LOWORD(wParam) - IDM_RECENT_FILE_BASE);
                if ((size_t)index < g_recentCount) {
                    OpenFileFromPath(hwnd, g_recentFiles[index]);
                    AddRecentFile(g_recentFiles[index]);
                    UpdateRecentFilesMenu(hwnd);
                }
            }
            break;

        case IDM_EDIT_UNDO:       SendMessageW(g_hEditor, EM_UNDO, 0, 0); break;
        case IDM_EDIT_CUT:        SendMessageW(g_hEditor, WM_CUT, 0, 0); break;
        case IDM_EDIT_COPY:       SendMessageW(g_hEditor, WM_COPY, 0, 0); break;
        case IDM_EDIT_PASTE:      SendMessageW(g_hEditor, WM_PASTE, 0, 0); break;
        case IDM_EDIT_SELECT:  SendMessageW(g_hEditor, EM_SETSEL, 0, -1); break;
        }
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        KillTimer(hwnd, 2);
        if (g_documents) {
            for (size_t i = 0; i < g_documentCount; ++i) {
                if (g_documents[i].text) {
                    HeapFree(GetProcessHeap(), 0, g_documents[i].text);
                }
            }
            HeapFree(GetProcessHeap(), 0, g_documents);
            g_documents = NULL;
            g_documentCount = 0;
        }
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

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_OPTIONS);
    SetMenuItemInfoW(menu, 3, TRUE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_NEW_TAB);
    SetMenuItemInfoW(menu, IDM_FILE_NEW_TAB, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_CLOSE_TAB);
    SetMenuItemInfoW(menu, IDM_FILE_CLOSE_TAB, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_NEW);
    SetMenuItemInfoW(menu, IDM_FILE_NEW, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_OPEN);
    SetMenuItemInfoW(menu, IDM_FILE_OPEN, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_SAVE);
    SetMenuItemInfoW(menu, IDM_FILE_SAVE, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_SAVE_AS);
    SetMenuItemInfoW(menu, IDM_FILE_SAVEAS, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_PRINT);
    SetMenuItemInfoW(menu, IDM_FILE_PRINT, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_EXPORT_PDF);
    SetMenuItemInfoW(menu, IDM_FILE_EXPORT_PDF, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_EXIT);
    SetMenuItemInfoW(menu, IDM_FILE_EXIT, FALSE, &mii);

    HMENU fileMenu = GetSubMenu(menu, 0);
    if (fileMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_RECENT_FILES);
        SetMenuItemInfoW(fileMenu, 11, TRUE, &mii);
    }

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_UNDO);
    SetMenuItemInfoW(menu, IDM_EDIT_UNDO, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_FIND);
    SetMenuItemInfoW(menu, IDM_EDIT_FIND, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_REPLACE);
    SetMenuItemInfoW(menu, IDM_EDIT_REPLACE, FALSE, &mii);

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_GOTO);
    SetMenuItemInfoW(menu, IDM_EDIT_GOTO, FALSE, &mii);

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

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_SYNTAX_HIGHLIGHT);
    SetMenuItemInfoW(menu, IDM_VIEW_SYNTAX_TOGGLE, FALSE, &mii);

    HMENU viewMenu = GetSubMenu(menu, 2);
    if (viewMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANGUAGE_MODE);
        SetMenuItemInfoW(viewMenu, 2, TRUE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANGUAGE);
        SetMenuItemInfoW(viewMenu, 4, TRUE, &mii);
    }

    HMENU languageModeMenu = viewMenu ? GetSubMenu(viewMenu, 2) : NULL;
    if (languageModeMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANGUAGE_PLAIN);
        SetMenuItemInfoW(languageModeMenu, IDM_VIEW_LANG_PLAIN, FALSE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANGUAGE_C);
        SetMenuItemInfoW(languageModeMenu, IDM_VIEW_LANG_C, FALSE, &mii);
    }

    HMENU languageMenu = viewMenu ? GetSubMenu(viewMenu, 4) : NULL;
    if (languageMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANG_ENGLISH);
        SetMenuItemInfoW(languageMenu, IDM_LANG_ENGLISH, FALSE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LANG_BANGLA);
        SetMenuItemInfoW(languageMenu, IDM_LANG_BANGLA, FALSE, &mii);
    }

    HMENU optionsMenu = GetSubMenu(menu, 3);
    if (optionsMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LINE_ENDINGS);
        SetMenuItemInfoW(optionsMenu, 1, TRUE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_ENCODING);
        SetMenuItemInfoW(optionsMenu, 2, TRUE, &mii);
    }

    mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_AUTOSAVE);
    SetMenuItemInfoW(menu, IDM_OPTIONS_AUTOSAVE, FALSE, &mii);

    HMENU lineMenu = optionsMenu ? GetSubMenu(optionsMenu, 1) : NULL;
    if (lineMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LINE_ENDINGS_CRLF);
        SetMenuItemInfoW(lineMenu, IDM_OPTIONS_LINE_CRLF, FALSE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_LINE_ENDINGS_LF);
        SetMenuItemInfoW(lineMenu, IDM_OPTIONS_LINE_LF, FALSE, &mii);
    }

    HMENU encMenu = optionsMenu ? GetSubMenu(optionsMenu, 2) : NULL;
    if (encMenu) {
        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_ENCODING_UTF8);
        SetMenuItemInfoW(encMenu, IDM_OPTIONS_ENC_UTF8, FALSE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_ENCODING_UTF16_LE);
        SetMenuItemInfoW(encMenu, IDM_OPTIONS_ENC_UTF16LE, FALSE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_ENCODING_UTF16_BE);
        SetMenuItemInfoW(encMenu, IDM_OPTIONS_ENC_UTF16BE, FALSE, &mii);

        mii.dwTypeData = (LPWSTR)loc_wstr(LOC_MENU_ENCODING_ANSI);
        SetMenuItemInfoW(encMenu, IDM_OPTIONS_ENC_ANSI, FALSE, &mii);
    }

    if (localization_is_bangla()) {
        CheckMenuItem(menu, IDM_LANG_BANGLA, MF_CHECKED);
        CheckMenuItem(menu, IDM_LANG_ENGLISH, MF_UNCHECKED);
    } else {
        CheckMenuItem(menu, IDM_LANG_BANGLA, MF_UNCHECKED);
        CheckMenuItem(menu, IDM_LANG_ENGLISH, MF_CHECKED);
    }

    CheckMenuItem(menu, IDM_TOGGLE_ALWAYSONTOP,
                  (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_VIEW_SYNTAX_TOGGLE, g_syntaxHighlightEnabled ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_VIEW_LANG_PLAIN, (g_languageMode == LANGUAGE_MODE_PLAIN) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_VIEW_LANG_C, (g_languageMode == LANGUAGE_MODE_C) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_AUTOSAVE, g_autosaveEnabled ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_LINE_CRLF, (g_currentLineEnding == LINE_ENDING_CRLF) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_LINE_LF, (g_currentLineEnding == LINE_ENDING_LF) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_ENC_UTF8, (g_currentFileEncoding == ENC_UTF8) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_ENC_UTF16LE, (g_currentFileEncoding == ENC_UTF16_LE) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_ENC_UTF16BE, (g_currentFileEncoding == ENC_UTF16_BE) ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(menu, IDM_OPTIONS_ENC_ANSI, (g_currentFileEncoding == ENC_ANSI) ? MF_CHECKED : MF_UNCHECKED);

    UpdateRecentFilesMenu(hwnd);
}

static void UpdateRecentFilesMenu(HWND hwnd)
{
    HMENU menu = GetMenu(hwnd);
    if (!menu) {
        return;
    }
    HMENU fileMenu = GetSubMenu(menu, 0);
    if (!fileMenu) {
        return;
    }
    HMENU recentMenu = GetSubMenu(fileMenu, 11);
    if (!recentMenu) {
        return;
    }

    for (UINT i = 0; i < _countof(g_recentFiles); ++i) {
        MENUITEMINFOW mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING | MIIM_STATE;
        WCHAR label[MAX_PATH + 8] = {0};
        if (i < g_recentCount) {
            swprintf_s(label, _countof(label), L"%u. %s", i + 1, GetDisplayNameFromPath(g_recentFiles[i]));
            mii.fState = MFS_ENABLED;
        } else {
            wcscpy_s(label, _countof(label), L"(Empty)");
            mii.fState = MFS_GRAYED;
        }
        mii.dwTypeData = label;
        SetMenuItemInfoW(recentMenu, i, TRUE, &mii);
    }
}

static void ScheduleSyntaxHighlighting(void)
{
    if (!g_syntaxHighlightEnabled || !g_hMainWnd) {
        return;
    }
    SetTimer(g_hMainWnd, 2, 400, NULL);
}

static void ApplySyntaxHighlighting(void)
{
    if (!g_hEditor) {
        return;
    }

    DWORD selStart = 0;
    DWORD selEnd = 0;
    SendMessageW(g_hEditor, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);

    CHARFORMAT2W base = {0};
    base.cbSize = sizeof(base);
    base.dwMask = CFM_COLOR;
    base.crTextColor = RGB(0, 0, 0);
    SendMessageW(g_hEditor, EM_SETSEL, 0, -1);
    SendMessageW(g_hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&base);

    if (!g_syntaxHighlightEnabled || g_languageMode == LANGUAGE_MODE_PLAIN) {
        SendMessageW(g_hEditor, EM_SETSEL, selStart, selEnd);
        return;
    }

    static const WCHAR *kKeywords[] = {
        L"auto", L"break", L"case", L"char", L"const", L"continue", L"default",
        L"do", L"double", L"else", L"enum", L"extern", L"float", L"for",
        L"goto", L"if", L"inline", L"int", L"long", L"register", L"restrict",
        L"return", L"short", L"signed", L"sizeof", L"static", L"struct",
        L"switch", L"typedef", L"union", L"unsigned", L"void", L"volatile",
        L"while", L"class", L"namespace", L"template", L"typename", L"bool"
    };

    DWORD len = (DWORD)GetWindowTextLengthW(g_hEditor);
    if (len == 0) {
        SendMessageW(g_hEditor, EM_SETSEL, selStart, selEnd);
        return;
    }

    WCHAR *text = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!text) {
        SendMessageW(g_hEditor, EM_SETSEL, selStart, selEnd);
        return;
    }
    GetWindowTextW(g_hEditor, text, len + 1);

    CHARFORMAT2W keyword = {0};
    keyword.cbSize = sizeof(keyword);
    keyword.dwMask = CFM_COLOR;
    keyword.crTextColor = RGB(0, 0, 180);

    for (DWORD i = 0; i < len; ++i) {
        if (!iswalpha(text[i]) && text[i] != L'_') {
            continue;
        }
        DWORD start = i;
        DWORD end = i + 1;
        while (end < len && (iswalnum(text[end]) || text[end] == L'_')) {
            end++;
        }
        DWORD wordLen = end - start;
        for (size_t k = 0; k < _countof(kKeywords); ++k) {
            size_t keyLen = wcslen(kKeywords[k]);
            if (keyLen == wordLen && _wcsnicmp(&text[start], kKeywords[k], keyLen) == 0) {
                SendMessageW(g_hEditor, EM_SETSEL, start, end);
                SendMessageW(g_hEditor, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&keyword);
                break;
            }
        }
        i = end - 1;
    }

    HeapFree(GetProcessHeap(), 0, text);
    SendMessageW(g_hEditor, EM_SETSEL, selStart, selEnd);
}

static INT_PTR CALLBACK GoToDialogProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        SetWindowLongPtrW(dlg, GWLP_USERDATA, (LONG_PTR)lParam);
        SetDlgItemInt(dlg, IDC_GOTO_EDIT, 1, FALSE);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            BOOL ok = FALSE;
            UINT line = GetDlgItemInt(dlg, IDC_GOTO_EDIT, &ok, FALSE);
            int maxLine = (int)GetWindowLongPtrW(dlg, GWLP_USERDATA);
            if (!ok || line == 0 || (int)line > maxLine) {
                MessageBoxW(dlg, L"Line number is out of range.", L"Go To Line", MB_ICONWARNING);
                return TRUE;
            }
            EndDialog(dlg, (INT_PTR)line);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(dlg, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL ShowGoToDialog(HWND hwnd)
{
    int lineCount = (int)SendMessageW(g_hEditor, EM_GETLINECOUNT, 0, 0);
    INT_PTR result = DialogBoxParamW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDD_GOTO_DIALOG),
                                     hwnd, GoToDialogProc, (LPARAM)lineCount);
    if (result > 0) {
        int line = (int)result;
        int index = (int)SendMessageW(g_hEditor, EM_LINEINDEX, line - 1, 0);
        SendMessageW(g_hEditor, EM_SETSEL, index, index);
        SendMessageW(g_hEditor, EM_SCROLLCARET, 0, 0);
        return TRUE;
    }
    return FALSE;
}

static void StartNewWindow(HWND hwnd)
{
    WCHAR modulePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, modulePath, _countof(modulePath));

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    if (!CreateProcessW(modulePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBoxW(hwnd, L"Unable to open a new window.", L"NotepadLite", MB_ICONERROR);
        return;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static BOOL PrintDocument(HWND hwnd, BOOL toPdf)
{
    PRINTDLGW pd = {0};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = hwnd;
    pd.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION;
    if (!PrintDlgW(&pd)) {
        return FALSE;
    }

    DOCINFOW di = {0};
    di.cbSize = sizeof(di);
    di.lpszDocName = toPdf ? L"NotepadLite Export" : L"NotepadLite Document";

    if (StartDocW(pd.hDC, &di) <= 0) {
        DeleteDC(pd.hDC);
        return FALSE;
    }
    StartPage(pd.hDC);

    int pageWidth = GetDeviceCaps(pd.hDC, HORZRES);
    int pageHeight = GetDeviceCaps(pd.hDC, VERTRES);
    int marginX = GetDeviceCaps(pd.hDC, LOGPIXELSX);
    int marginY = GetDeviceCaps(pd.hDC, LOGPIXELSY);

    FORMATRANGE fr = {0};
    fr.hdc = pd.hDC;
    fr.hdcTarget = pd.hDC;
    fr.rcPage.left = 0;
    fr.rcPage.top = 0;
    fr.rcPage.right = pageWidth;
    fr.rcPage.bottom = pageHeight;
    fr.rc.left = marginX;
    fr.rc.top = marginY;
    fr.rc.right = pageWidth - marginX;
    fr.rc.bottom = pageHeight - marginY;
    fr.chrg.cpMin = 0;
    fr.chrg.cpMax = -1;

    SendMessageW(g_hEditor, EM_FORMATRANGE, TRUE, (LPARAM)&fr);
    SendMessageW(g_hEditor, EM_FORMATRANGE, FALSE, 0);

    EndPage(pd.hDC);
    EndDoc(pd.hDC);
    DeleteDC(pd.hDC);

    if (pd.hDevMode) {
        GlobalFree(pd.hDevMode);
    }
    if (pd.hDevNames) {
        GlobalFree(pd.hDevNames);
    }
    return TRUE;
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

    if (g_hTab) {
        int tabHeight = 28;
        SetWindowPos(g_hTab, NULL, rc.left, rc.top, rc.right - rc.left, tabHeight, SWP_NOZORDER);
        rc.top += tabHeight;
    }

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
    const WCHAR *lineEnding = (g_currentLineEnding == LINE_ENDING_LF) ? L"LF" : L"CRLF";
    swprintf_s(buf, _countof(buf), L"%s (%s)", enc, lineEnding);
    SendMessageW(g_hStatusBar, SB_SETTEXTW, 2, (LPARAM)buf);
}
