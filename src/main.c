#include <windows.h>
#include "ui.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR pCmdLine, int nCmdShow)
{
    (void)hPrev;   // suppress unused warning
    (void)pCmdLine;
    return ui_run(hInst, nCmdShow);
}
