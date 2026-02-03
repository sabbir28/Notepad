#ifdef _WIN32
#include <windows.h>
#include "ui.h"
#include "file_io.h"
#include "localization.h"
#else
#include <stdio.h>
#include <string.h>
#include "linux_console.h"
#include "linux_ui.h"
#include "localization.h"
#endif

#ifdef _WIN32
int WINAPI wWinMain(
    HINSTANCE hInst,
    HINSTANCE hPrev,
    PWSTR     pCmdLine,
    int       nCmdShow
)
{
    (void)hPrev;
    (void)pCmdLine;

    localization_init();
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
#else
static void print_usage(const char *program)
{
    printf("%s\n", loc_str(LOC_LINUX_CONSOLE_TITLE));
    printf(loc_str(LOC_USAGE), program);
    printf("\n%s\n", loc_str(LOC_USAGE_DETAIL));
    printf("%s\n", loc_str(LOC_USAGE_EOF));
}

int main(int argc, char **argv)
{
    localization_init();
    const char *path = NULL;
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        path = argv[1];
    }

#ifdef USE_GTK
    return linux_ui_run(argc, argv, path);
#else
    return linux_console_run(argc, argv, path);
#endif
}
#endif
