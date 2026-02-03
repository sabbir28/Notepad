#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#ifdef _WIN32
#include <windows.h>
#else
#include <stdbool.h>
#endif

typedef enum {
    LOC_APP_TITLE,
    LOC_STATUS_LN_COL,
    LOC_STATUS_SIZE,
    LOC_STATUS_ENCODING_UNKNOWN,
    LOC_STATUS_ENCODING_UTF8,
    LOC_STATUS_ENCODING_UTF16_LE,
    LOC_STATUS_ENCODING_UTF16_BE,
    LOC_STATUS_ENCODING_ANSI,
    LOC_MENU_FILE,
    LOC_MENU_NEW,
    LOC_MENU_OPEN,
    LOC_MENU_SAVE,
    LOC_MENU_SAVE_AS,
    LOC_MENU_EXIT,
    LOC_MENU_EDIT,
    LOC_MENU_UNDO,
    LOC_MENU_CUT,
    LOC_MENU_COPY,
    LOC_MENU_PASTE,
    LOC_MENU_SELECT_ALL,
    LOC_MENU_VIEW,
    LOC_MENU_ALWAYS_ON_TOP,
    LOC_LINUX_CONSOLE_TITLE,
    LOC_USAGE,
    LOC_USAGE_DETAIL,
    LOC_USAGE_EOF,
    LOC_NO_FILE,
    LOC_CURRENT_CONTENTS,
    LOC_END_OF_FILE,
    LOC_ENTER_NEW_CONTENT,
    LOC_ALLOC_FAIL,
    LOC_WRITE_FAIL,
    LOC_SAVED_BYTES,
    LOC_KEY_COUNT
} LocKey;

void localization_init(void);

#ifdef _WIN32
const wchar_t *loc_wstr(LocKey key);
#else
const char *loc_str(LocKey key);
#endif

#endif
