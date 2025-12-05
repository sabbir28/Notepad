// ui.c
#include <windows.h>
#include <richedit.h>
#include "ui.h"
#include "file_io.h"

static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
static void ui_add_editor(HWND parent);

HWND g_editor                       = NULL;
FILE_ENCODING g_currentFileEncoding = ENC_ANSI;
WCHAR text_path[MAX_PATH_LEN]       = {0};

int ui_run(HINSTANCE hInst, int show)
{
    LoadLibraryW(L"Msftedit.dll");  // RICHEDIT50W

    WNDCLASSEXW wc   = {0};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NotepadLiteWnd";

    wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCE(IDR_MAINICON));
    wc.hIconSm       = LoadIconW(hInst, MAKEINTRESOURCE(IDR_MAINICON));

    if (!RegisterClassExW(&wc)) return -1;

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"NotepadLite",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 650,
        NULL, NULL, hInst, NULL
    );
    if (!hwnd) return -1;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    HACCEL hAccel = LoadAcceleratorsW(hInst, MAKEINTRESOURCE(IDR_ACCELERATORS));

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!TranslateAcceleratorW(hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        ui_add_editor(hwnd);
        {
            HMENU hMenu = LoadMenuW(((LPCREATESTRUCT)lp)->hInstance, MAKEINTRESOURCE(IDR_MYMENU));
            if (hMenu) SetMenu(hwnd, hMenu);
        }
        return 0;

    case WM_SIZE:
        if (g_editor) MoveWindow(g_editor, 0, 0, LOWORD(lp), HIWORD(lp), TRUE);
        return 0;

    case WM_GETMINMAXINFO:
        ((MINMAXINFO*)lp)->ptMinTrackSize.x = 400;
        ((MINMAXINFO*)lp)->ptMinTrackSize.y = 300;
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDM_FILE_NEW:
        {
            SetWindowTextW(g_editor, L"");
            g_currentFileEncoding = ENC_UTF8;
            ZeroMemory(text_path, sizeof(text_path));
            break;
        }

        case IDM_FILE_OPEN:
        {
            if (file_open_dialog(hwnd, text_path, MAX_PATH_LEN))
            {
                LPWSTR fileBuffer = NULL;
                DWORD fileSize    = 0;
                FILE_ENCODING enc = file_detect_encoding(text_path);

                if (file_read(hwnd, text_path, &fileBuffer, &fileSize, &enc))
                {
                    SetWindowTextW(g_editor, fileBuffer);
                    g_currentFileEncoding = enc;

                    // Free fileBuffer after use
                    HeapFree(GetProcessHeap(), 0, fileBuffer);
                }
                else
                {
                    MessageBoxW(hwnd,L"Failed to read file.",L"Error",MB_OK | MB_ICONERROR);
                }
            }
            break;
        }

        case IDM_FILE_SAVE:
        {
            if (text_path[0] == L'\0')
            {
                if (!file_save_dialog(hwnd, text_path, MAX_PATH_LEN)) 
                {
                    break;
                }
            }

            DWORD textLen = (DWORD)GetWindowTextLengthW(g_editor);
            LPWSTR buffer = (LPWSTR)HeapAlloc(
                GetProcessHeap(),
                HEAP_ZERO_MEMORY,
                (textLen + 1) * sizeof(WCHAR)
            );

            if (!buffer) break;

            GetWindowTextW(g_editor, buffer, textLen + 1);

            if (!file_write(hwnd, text_path, buffer, textLen * sizeof(WCHAR), g_currentFileEncoding))
            {
                MessageBoxW(hwnd,L"Failed to save file.",L"NotepadLite",MB_ICONERROR);
            }

            HeapFree(GetProcessHeap(), 0, buffer);
            break;
        }
        case IDM_FILE_SAVEAS:
        {
            WCHAR newPath[MAX_PATH_LEN] = {0};

            if (file_save_dialog(hwnd, newPath, MAX_PATH_LEN))
            {
                DWORD textLen = (DWORD)GetWindowTextLengthW(g_editor);
                LPWSTR buffer = (LPWSTR)HeapAlloc(
                    GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    (textLen+ 1) * sizeof(WCHAR)
                );

                if (buffer)
                {
                    GetWindowTextW(g_editor,buffer,textLen + 1);

                    // save to new path
                    if (!file_write(hwnd, newPath, buffer, textLen * sizeof(WCHAR), g_currentFileEncoding))
                    {
                        MessageBoxW(hwnd, L"Failed to save file.", L"NotepadLite", MB_ICONERROR);
                    }
                    else
                    {
                        // update that curent path !
                        wcscpy_s(text_path, MAX_PATH_LEN, newPath);
                    }

                    HeapFree(GetProcessHeap(),0, buffer);
                }
            }
            break;
        }

        case IDM_TOGGLE_ALWAYSONTOP:
        {
            HMENU hMenue = GetMenu(hwnd);
            if(hMenue)
            {
                UINT state = GetMenuState(hMenue, IDM_TOGGLE_ALWAYSONTOP, MF_BYCOMMAND);
                BOOL makeTopmost = !(state & MF_CHECKED);
                SetWindowPos(
                    hwnd,
                    makeTopmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                    0,0,0,0,
                    SWP_NOMOVE | SWP_NOSIZE
                );

                // update the cheack manue 

                CheckMenuItem(
                    hMenue,
                    IDM_TOGGLE_ALWAYSONTOP,
                    makeTopmost ? MF_CHECKED : MF_UNCHECKED
                );
            }

            break;
        }

        case IDM_FILE_EXIT:    PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
        case IDM_EDIT_UNDO:    SendMessageW(g_editor, EM_UNDO, 0, 0); break;
        case IDM_EDIT_CUT:     SendMessageW(g_editor, WM_CUT, 0, 0); break;
        case IDM_EDIT_COPY:    SendMessageW(g_editor, WM_COPY, 0, 0); break;
        case IDM_EDIT_PASTE:   SendMessageW(g_editor, WM_PASTE, 0, 0); break;
        case IDM_EDIT_SELECT:  SendMessageW(g_editor, EM_SETSEL, 0, -1); break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ui_add_editor(HWND parent)
{
    g_editor = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"RICHEDIT50W",
        NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | ES_WANTRETURN,
        0, 0, 0, 0,
        parent, NULL,
        (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE),
        NULL
    );

    CHARFORMATW cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE;
    cf.yHeight = 220;  // ~11pt
    wcscpy_s(cf.szFaceName, LF_FACESIZE, L"Consolas");
    SendMessageW(g_editor, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);

    SendMessageW(g_editor, EM_EXLIMITTEXT, 0, (LPARAM)-1);
}
