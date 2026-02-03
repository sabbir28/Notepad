#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <stdbool.h>
#include <windows.h>

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
    LOC_MENU_LANGUAGE,
    LOC_MENU_LANG_ENGLISH,
    LOC_MENU_LANG_BANGLA,
    LOC_KEY_COUNT
} LocKey;

void localization_init(void);
void localization_set_bangla(bool enable);
bool localization_is_bangla(void);

const wchar_t *loc_wstr(LocKey key);

#endif
