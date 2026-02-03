#ifdef _WIN32
#include <windows.h>
#include "ui.h"
#include "file_io.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
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
    printf("NotepadLite (Linux console mode)\n");
    printf("Usage: %s [file]\n", program);
    printf("If a file is provided, it will be loaded and then overwritten with new input.\n");
    printf("Finish input with Ctrl-D (EOF).\n");
}

int main(int argc, char **argv)
{
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
        printf("No file provided. Writing to default: %s\n", path);
    }

    LPWSTR buffer = NULL;
    DWORD size = 0;
    FILE_ENCODING enc = ENC_UTF8;
    if (file_read(NULL, path, &buffer, &size, &enc)) {
        if (size > 0) {
            printf("Current contents:\n");
            fwrite(buffer, 1, size, stdout);
            printf("\n--- End of file ---\n");
        }
        free(buffer);
    }

    printf("Enter new content. End with Ctrl-D (EOF):\n");
    size_t cap = 4096;
    size_t len = 0;
    char *input = (char *)malloc(cap);
    if (!input) {
        fprintf(stderr, "Allocation failed.\n");
        return 1;
    }

    int ch;
    while ((ch = getchar()) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *next = (char *)realloc(input, cap);
            if (!next) {
                free(input);
                fprintf(stderr, "Allocation failed.\n");
                return 1;
            }
            input = next;
        }
        input[len++] = (char)ch;
    }
    input[len] = '\0';

    if (!file_write(NULL, path, input, (DWORD)len, ENC_UTF8)) {
        fprintf(stderr, "Failed to write file: %s\n", path);
        free(input);
        return 1;
    }

    printf("Saved %zu bytes to %s\n", len, path);
    free(input);
    return 0;
}
#endif
