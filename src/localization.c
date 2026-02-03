#include "localization.h"

static const char *kLocKeyNames[] = {
    "APP_TITLE",
    "STATUS_LN_COL",
    "STATUS_SIZE",
    "STATUS_ENCODING_UNKNOWN",
    "STATUS_ENCODING_UTF8",
    "STATUS_ENCODING_UTF16_LE",
    "STATUS_ENCODING_UTF16_BE",
    "STATUS_ENCODING_ANSI",
    "MENU_FILE",
    "MENU_NEW_TAB",
    "MENU_CLOSE_TAB",
    "MENU_NEW",
    "MENU_OPEN",
    "MENU_SAVE",
    "MENU_SAVE_AS",
    "MENU_PRINT",
    "MENU_EXPORT_PDF",
    "MENU_RECENT_FILES",
    "MENU_EXIT",
    "MENU_EDIT",
    "MENU_UNDO",
    "MENU_FIND",
    "MENU_REPLACE",
    "MENU_GOTO",
    "MENU_CUT",
    "MENU_COPY",
    "MENU_PASTE",
    "MENU_SELECT_ALL",
    "MENU_VIEW",
    "MENU_ALWAYS_ON_TOP",
    "MENU_SYNTAX_HIGHLIGHT",
    "MENU_LANGUAGE_MODE",
    "MENU_LANGUAGE_PLAIN",
    "MENU_LANGUAGE_C",
    "MENU_LANGUAGE",
    "MENU_LANG_ENGLISH",
    "MENU_LANG_BANGLA",
    "MENU_OPTIONS",
    "MENU_AUTOSAVE",
    "MENU_LINE_ENDINGS",
    "MENU_LINE_ENDINGS_CRLF",
    "MENU_LINE_ENDINGS_LF",
    "MENU_ENCODING",
    "MENU_ENCODING_UTF8",
    "MENU_ENCODING_UTF16_LE",
    "MENU_ENCODING_UTF16_BE",
    "MENU_ENCODING_ANSI"
};

#include <windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    LocKey key;
    const wchar_t *en;
    const wchar_t *bn;
} LocEntry;

static const LocEntry kLocTable[] = {
    { LOC_APP_TITLE, L"NotepadLite", L"নোটপ্যাড লাইট" },
    { LOC_STATUS_LN_COL, L"Ln %d, Col %d", L"লাইন %d, কলাম %d" },
    { LOC_STATUS_SIZE, L"%.2f %s", L"%.2f %s" },
    { LOC_STATUS_ENCODING_UNKNOWN, L"Unknown", L"অজানা" },
    { LOC_STATUS_ENCODING_UTF8, L"UTF-8", L"UTF-8" },
    { LOC_STATUS_ENCODING_UTF16_LE, L"UTF-16 LE", L"UTF-16 LE" },
    { LOC_STATUS_ENCODING_UTF16_BE, L"UTF-16 BE", L"UTF-16 BE" },
    { LOC_STATUS_ENCODING_ANSI, L"ANSI", L"ANSI" },
    { LOC_MENU_FILE, L"&File", L"&ফাইল" },
    { LOC_MENU_NEW_TAB, L"&New Tab\tCtrl+N", L"&নতুন ট্যাব\tCtrl+N" },
    { LOC_MENU_CLOSE_TAB, L"&Close Tab\tCtrl+W", L"&ট্যাব বন্ধ\tCtrl+W" },
    { LOC_MENU_NEW, L"&New Window\tCtrl+Shift+N", L"&নতুন উইন্ডো\tCtrl+Shift+N" },
    { LOC_MENU_OPEN, L"&Open...\tCtrl+O", L"&খুলুন...\tCtrl+O" },
    { LOC_MENU_SAVE, L"&Save\tCtrl+S", L"&সংরক্ষণ\tCtrl+S" },
    { LOC_MENU_SAVE_AS, L"Save &As...\tCtrl+Shift+S", L"&নতুন নামে সংরক্ষণ...\tCtrl+Shift+S" },
    { LOC_MENU_PRINT, L"&Print...\tCtrl+P", L"&প্রিন্ট...\tCtrl+P" },
    { LOC_MENU_EXPORT_PDF, L"Export &PDF...\tCtrl+Shift+P", L"&পিডিএফ রপ্তানি...\tCtrl+Shift+P" },
    { LOC_MENU_RECENT_FILES, L"Recent Files", L"সাম্প্রতিক ফাইলসমূহ" },
    { LOC_MENU_EXIT, L"E&xit", L"প্রস্থা&ন" },
    { LOC_MENU_EDIT, L"&Edit", L"&সম্পাদনা" },
    { LOC_MENU_UNDO, L"&Undo\tCtrl+Z", L"&পূর্বাবস্থায়\tCtrl+Z" },
    { LOC_MENU_FIND, L"&Find...\tCtrl+F", L"&খুঁজুন...\tCtrl+F" },
    { LOC_MENU_REPLACE, L"&Replace...\tCtrl+H", L"&প্রতিস্থাপন...\tCtrl+H" },
    { LOC_MENU_GOTO, L"&Go To...\tCtrl+G", L"&লাইন যান...\tCtrl+G" },
    { LOC_MENU_CUT, L"Cu&t\tCtrl+X", L"কা&টুন\tCtrl+X" },
    { LOC_MENU_COPY, L"&Copy\tCtrl+C", L"&কপি\tCtrl+C" },
    { LOC_MENU_PASTE, L"&Paste\tCtrl+V", L"&পেস্ট\tCtrl+V" },
    { LOC_MENU_SELECT_ALL, L"Select &All\tCtrl+A", L"সব &নির্বাচন\tCtrl+A" },
    { LOC_MENU_VIEW, L"&View", L"&ভিউ" },
    { LOC_MENU_ALWAYS_ON_TOP, L"&Always on Top\tCtrl+T", L"&সবসময় উপরে\tCtrl+T" },
    { LOC_MENU_SYNTAX_HIGHLIGHT, L"Syntax &Highlighting", L"&সিনট্যাক্স হাইলাইটিং" },
    { LOC_MENU_LANGUAGE_MODE, L"Language Mode", L"ভাষা মোড" },
    { LOC_MENU_LANGUAGE_PLAIN, L"&Plain Text", L"&সাধারণ টেক্সট" },
    { LOC_MENU_LANGUAGE_C, L"&C/C++", L"&সি/সি++" },
    { LOC_MENU_LANGUAGE, L"&Language", L"&ভাষা" },
    { LOC_MENU_LANG_ENGLISH, L"&English", L"&ইংরেজি" },
    { LOC_MENU_LANG_BANGLA, L"&Bangla (বাংলা)", L"&বাংলা" },
    { LOC_MENU_OPTIONS, L"&Options", L"&অপশন" },
    { LOC_MENU_AUTOSAVE, L"&Autosave", L"&স্বয়ং-সংরক্ষণ" },
    { LOC_MENU_LINE_ENDINGS, L"Line Endings", L"লাইন শেষ" },
    { LOC_MENU_LINE_ENDINGS_CRLF, L"&CRLF (Windows)", L"&CRLF (উইন্ডোজ)" },
    { LOC_MENU_LINE_ENDINGS_LF, L"&LF (Unix)", L"&LF (ইউনিক্স)" },
    { LOC_MENU_ENCODING, L"Encoding", L"এনকোডিং" },
    { LOC_MENU_ENCODING_UTF8, L"UTF-&8", L"UTF-&8" },
    { LOC_MENU_ENCODING_UTF16_LE, L"UTF-16 &LE", L"UTF-16 &LE" },
    { LOC_MENU_ENCODING_UTF16_BE, L"UTF-16 &BE", L"UTF-16 &BE" },
    { LOC_MENU_ENCODING_ANSI, L"&ANSI", L"&ANSI" }
};

static bool g_useBangla = false;
static wchar_t *g_overrides[LOC_KEY_COUNT] = {0};

static LocKey key_from_name(const char *name)
{
    for (size_t i = 0; i < LOC_KEY_COUNT; ++i) {
        if (strcmp(kLocKeyNames[i], name) == 0) {
            return (LocKey)i;
        }
    }
    return LOC_KEY_COUNT;
}

static bool is_bangla_locale(const wchar_t *lang)
{
    if (!lang || !*lang) {
        return false;
    }
    if (_wcsnicmp(lang, L"bn", 2) == 0) {
        return true;
    }
    return _wcsnicmp(lang, L"bangla", 6) == 0;
}

static char *trim_whitespace(char *text)
{
    char *start = text;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }
    if (*start == '\0') {
        return start;
    }
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        --end;
    }
    end[1] = '\0';
    return start;
}

static void set_override(LocKey key, const char *value)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, value, -1, NULL, 0);
    if (len <= 0) {
        return;
    }
    wchar_t *wide = (wchar_t *)calloc((size_t)len, sizeof(wchar_t));
    if (!wide) {
        return;
    }
    MultiByteToWideChar(CP_UTF8, 0, value, -1, wide, len);
    free(g_overrides[key]);
    g_overrides[key] = wide;
}

static void load_locale_file(const wchar_t *path)
{
    FILE *file = _wfopen(path, L"rb");
    if (!file) {
        return;
    }
    char line[1024];
    bool first_line = true;
    while (fgets(line, (int)sizeof(line), file)) {
        char *cursor = line;
        if (first_line) {
            if ((unsigned char)cursor[0] == 0xEF &&
                (unsigned char)cursor[1] == 0xBB &&
                (unsigned char)cursor[2] == 0xBF) {
                cursor += 3;
            }
            first_line = false;
        }
        char *trimmed = trim_whitespace(cursor);
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';') {
            continue;
        }
        char *equals = strchr(trimmed, '=');
        if (!equals) {
            continue;
        }
        *equals = '\0';
        char *key_name = trim_whitespace(trimmed);
        char *value = trim_whitespace(equals + 1);
        if (*key_name == '\0' || *value == '\0') {
            continue;
        }
        LocKey key = key_from_name(key_name);
        if (key == LOC_KEY_COUNT) {
            continue;
        }
        set_override(key, value);
    }
    fclose(file);
}

static const wchar_t *lookup_entry(LocKey key)
{
    if (key < LOC_KEY_COUNT && g_overrides[key]) {
        return g_overrides[key];
    }
    for (size_t i = 0; i < sizeof(kLocTable) / sizeof(kLocTable[0]); ++i) {
        if (kLocTable[i].key == key) {
            return g_useBangla ? kLocTable[i].bn : kLocTable[i].en;
        }
    }
    return L"";
}

void localization_init(void)
{
    wchar_t lang[16] = {0};
    DWORD len = GetEnvironmentVariableW(L"NOTEPADLITE_LANG", lang, (DWORD)(sizeof(lang) / sizeof(lang[0])));
    if (len > 0) {
        g_useBangla = is_bangla_locale(lang);
    } else {
        wchar_t locale_name[LOCALE_NAME_MAX_LENGTH] = {0};
        if (GetUserDefaultLocaleName(locale_name, LOCALE_NAME_MAX_LENGTH)) {
            g_useBangla = is_bangla_locale(locale_name);
        }
    }

    wchar_t locale_file[MAX_PATH] = {0};
    DWORD file_len = GetEnvironmentVariableW(L"NOTEPADLITE_LOCALE_FILE", locale_file,
                                             (DWORD)(sizeof(locale_file) / sizeof(locale_file[0])));
    if (file_len > 0) {
        load_locale_file(locale_file);
    }
}

const wchar_t *loc_wstr(LocKey key)
{
    return lookup_entry(key);
}

void localization_set_bangla(bool enable)
{
    g_useBangla = enable;
}

bool localization_is_bangla(void)
{
    return g_useBangla;
}
