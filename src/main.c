#include <windows.h>
#include "ui.h"

int WINAPI wWinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    PWSTR     pCmdLine,
    int       nCmdShow
)
{
    (void)hPrev;
    (void)pCmdLine;

    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    LPCWSTR startup_path = NULL;

    if (argv && argc > 1) {
        // take only the first file argument
        startup_path = argv[1];
    }

    int result = ui_run(hInst, nCmdShow, startup_path);

    if (argv) {
        LocalFree(argv);
    }

    return result;
}
