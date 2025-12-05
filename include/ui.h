#ifndef UI_H
#define UI_H

#include <windows.h>
#include "resource.h"

/* Starts the main UI loop */
int ui_run(HINSTANCE hInst, int nCmdShow, LPCWSTR startup_path);

/* Global editor handle for future features */
extern HWND g_editor;

#endif /* UI_H */
