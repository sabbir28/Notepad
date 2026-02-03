#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_io.h"
#include "linux_console.h"
#include "localization.h"

int linux_console_run(int argc, char **argv, const char *startup_path)
{
    (void)argc;
    (void)argv;
    const char *path = startup_path;

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
