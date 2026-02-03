#ifdef _WIN32
#include <windows.h>
#include "ui.h"
#include "file_io.h"
#include "localization.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
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

    if (!path) {
        path = "notepad.txt";
        printf(loc_str(LOC_NO_FILE), path);
        printf("\n");
    }

    LPWSTR buffer = NULL;
    DWORD size = 0;
    FILE_ENCODING enc = ENC_UTF8;
    if (file_read(NULL, path, &buffer, &size, &enc)) {
        if (size > 0) {
            printf("%s\n", loc_str(LOC_CURRENT_CONTENTS));
            fwrite(buffer, 1, size, stdout);
            printf("\n%s\n", loc_str(LOC_END_OF_FILE));
        }
        free(buffer);
    }

    printf("%s\n", loc_str(LOC_ENTER_NEW_CONTENT));
    size_t cap = 4096;
    size_t len = 0;
    char *input = (char *)malloc(cap);
    if (!input) {
        fprintf(stderr, "%s\n", loc_str(LOC_ALLOC_FAIL));
        return 1;
    }

    int ch;
    while ((ch = getchar()) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *next = (char *)realloc(input, cap);
            if (!next) {
                free(input);
                fprintf(stderr, "%s\n", loc_str(LOC_ALLOC_FAIL));
                return 1;
            }
            input = next;
        }
        input[len++] = (char)ch;
    }
    input[len] = '\0';

    if (!file_write(NULL, path, input, (DWORD)len, ENC_UTF8)) {
        fprintf(stderr, loc_str(LOC_WRITE_FAIL), path);
        fprintf(stderr, "\n");
        free(input);
        return 1;
    }

    printf(loc_str(LOC_SAVED_BYTES), len, path);
    printf("\n");
    free(input);
    return 0;
}
#endif
