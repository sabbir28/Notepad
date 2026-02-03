#include "localization.h"

#ifdef _WIN32
#include <windows.h>
#include <stdbool.h>

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
    { LOC_MENU_NEW, L"&New\tCtrl+N", L"&নতুন\tCtrl+N" },
    { LOC_MENU_OPEN, L"&Open...\tCtrl+O", L"&খুলুন...\tCtrl+O" },
    { LOC_MENU_SAVE, L"&Save\tCtrl+S", L"&সংরক্ষণ\tCtrl+S" },
    { LOC_MENU_SAVE_AS, L"Save &As...\tCtrl+Shift+S", L"&নতুন নামে সংরক্ষণ...\tCtrl+Shift+S" },
    { LOC_MENU_EXIT, L"E&xit", L"প্রস্থা&ন" },
    { LOC_MENU_EDIT, L"&Edit", L"&সম্পাদনা" },
    { LOC_MENU_UNDO, L"&Undo\tCtrl+Z", L"&পূর্বাবস্থায়\tCtrl+Z" },
    { LOC_MENU_CUT, L"Cu&t\tCtrl+X", L"কা&টুন\tCtrl+X" },
    { LOC_MENU_COPY, L"&Copy\tCtrl+C", L"&কপি\tCtrl+C" },
    { LOC_MENU_PASTE, L"&Paste\tCtrl+V", L"&পেস্ট\tCtrl+V" },
    { LOC_MENU_SELECT_ALL, L"Select &All\tCtrl+A", L"সব &নির্বাচন\tCtrl+A" },
    { LOC_MENU_VIEW, L"&View", L"&ভিউ" },
    { LOC_MENU_ALWAYS_ON_TOP, L"&Always on Top\tCtrl+T", L"&সবসময় উপরে\tCtrl+T" },
    { LOC_LINUX_CONSOLE_TITLE, L"NotepadLite (Linux console mode)", L"নোটপ্যাড লাইট (লিনাক্স কনসোল মোড)" },
    { LOC_USAGE, L"Usage: %s [file]", L"ব্যবহার: %s [file]" },
    { LOC_USAGE_DETAIL, L"If a file is provided, it will be loaded and then overwritten with new input.", L"একটি ফাইল দিলে সেটি লোড হবে এবং নতুন ইনপুট দিয়ে ওভাররাইট হবে।" },
    { LOC_USAGE_EOF, L"Finish input with Ctrl-D (EOF).", L"Ctrl-D (EOF) দিয়ে ইনপুট শেষ করুন।" },
    { LOC_NO_FILE, L"No file provided. Writing to default: %s", L"কোনো ফাইল দেওয়া হয়নি। ডিফল্টে লেখা হবে: %s" },
    { LOC_CURRENT_CONTENTS, L"Current contents:", L"বর্তমান কনটেন্ট:" },
    { LOC_END_OF_FILE, L"--- End of file ---", L"--- ফাইল শেষ ---" },
    { LOC_ENTER_NEW_CONTENT, L"Enter new content. End with Ctrl-D (EOF):", L"নতুন কনটেন্ট লিখুন। শেষ করতে Ctrl-D (EOF):" },
    { LOC_ALLOC_FAIL, L"Allocation failed.", L"মেমোরি বরাদ্দ ব্যর্থ।" },
    { LOC_WRITE_FAIL, L"Failed to write file: %s", L"ফাইল লেখা ব্যর্থ: %s" },
    { LOC_SAVED_BYTES, L"Saved %zu bytes to %s", L"%zu বাইট সংরক্ষণ হয়েছে: %s" }
};

static bool g_useBangla = false;

static const wchar_t *lookup_entry(LocKey key)
{
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
        if (_wcsicmp(lang, L"bn") == 0 || _wcsicmp(lang, L"bn-BD") == 0 || _wcsicmp(lang, L"bangla") == 0) {
            g_useBangla = true;
        }
    }
}

const wchar_t *loc_wstr(LocKey key)
{
    return lookup_entry(key);
}

#else
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    LocKey key;
    const char *en;
    const char *bn;
} LocEntry;

static const LocEntry kLocTable[] = {
    { LOC_APP_TITLE, "NotepadLite", "নোটপ্যাড লাইট" },
    { LOC_STATUS_LN_COL, "Ln %d, Col %d", "লাইন %d, কলাম %d" },
    { LOC_STATUS_SIZE, "%.2f %s", "%.2f %s" },
    { LOC_STATUS_ENCODING_UNKNOWN, "Unknown", "অজানা" },
    { LOC_STATUS_ENCODING_UTF8, "UTF-8", "UTF-8" },
    { LOC_STATUS_ENCODING_UTF16_LE, "UTF-16 LE", "UTF-16 LE" },
    { LOC_STATUS_ENCODING_UTF16_BE, "UTF-16 BE", "UTF-16 BE" },
    { LOC_STATUS_ENCODING_ANSI, "ANSI", "ANSI" },
    { LOC_MENU_FILE, "&File", "&ফাইল" },
    { LOC_MENU_NEW, "&New\tCtrl+N", "&নতুন\tCtrl+N" },
    { LOC_MENU_OPEN, "&Open...\tCtrl+O", "&খুলুন...\tCtrl+O" },
    { LOC_MENU_SAVE, "&Save\tCtrl+S", "&সংরক্ষণ\tCtrl+S" },
    { LOC_MENU_SAVE_AS, "Save &As...\tCtrl+Shift+S", "&নতুন নামে সংরক্ষণ...\tCtrl+Shift+S" },
    { LOC_MENU_EXIT, "E&xit", "প্রস্থা&ন" },
    { LOC_MENU_EDIT, "&Edit", "&সম্পাদনা" },
    { LOC_MENU_UNDO, "&Undo\tCtrl+Z", "&পূর্বাবস্থায়\tCtrl+Z" },
    { LOC_MENU_CUT, "Cu&t\tCtrl+X", "কা&টুন\tCtrl+X" },
    { LOC_MENU_COPY, "&Copy\tCtrl+C", "&কপি\tCtrl+C" },
    { LOC_MENU_PASTE, "&Paste\tCtrl+V", "&পেস্ট\tCtrl+V" },
    { LOC_MENU_SELECT_ALL, "Select &All\tCtrl+A", "সব &নির্বাচন\tCtrl+A" },
    { LOC_MENU_VIEW, "&View", "&ভিউ" },
    { LOC_MENU_ALWAYS_ON_TOP, "&Always on Top\tCtrl+T", "&সবসময় উপরে\tCtrl+T" },
    { LOC_LINUX_CONSOLE_TITLE, "NotepadLite (Linux console mode)", "নোটপ্যাড লাইট (লিনাক্স কনসোল মোড)" },
    { LOC_USAGE, "Usage: %s [file]", "ব্যবহার: %s [file]" },
    { LOC_USAGE_DETAIL, "If a file is provided, it will be loaded and then overwritten with new input.", "একটি ফাইল দিলে সেটি লোড হবে এবং নতুন ইনপুট দিয়ে ওভাররাইট হবে।" },
    { LOC_USAGE_EOF, "Finish input with Ctrl-D (EOF).", "Ctrl-D (EOF) দিয়ে ইনপুট শেষ করুন।" },
    { LOC_NO_FILE, "No file provided. Writing to default: %s", "কোনো ফাইল দেওয়া হয়নি। ডিফল্টে লেখা হবে: %s" },
    { LOC_CURRENT_CONTENTS, "Current contents:", "বর্তমান কনটেন্ট:" },
    { LOC_END_OF_FILE, "--- End of file ---", "--- ফাইল শেষ ---" },
    { LOC_ENTER_NEW_CONTENT, "Enter new content. End with Ctrl-D (EOF):", "নতুন কনটেন্ট লিখুন। শেষ করতে Ctrl-D (EOF):" },
    { LOC_ALLOC_FAIL, "Allocation failed.", "মেমোরি বরাদ্দ ব্যর্থ।" },
    { LOC_WRITE_FAIL, "Failed to write file: %s", "ফাইল লেখা ব্যর্থ: %s" },
    { LOC_SAVED_BYTES, "Saved %zu bytes to %s", "%zu বাইট সংরক্ষণ হয়েছে: %s" }
};

static bool g_useBangla = false;

static const char *lookup_entry(LocKey key)
{
    for (size_t i = 0; i < sizeof(kLocTable) / sizeof(kLocTable[0]); ++i) {
        if (kLocTable[i].key == key) {
            return g_useBangla ? kLocTable[i].bn : kLocTable[i].en;
        }
    }
    return "";
}

void localization_init(void)
{
    const char *lang = getenv("NOTEPADLITE_LANG");
    if (!lang) {
        return;
    }
    if (strcmp(lang, "bn") == 0 || strcmp(lang, "bn-BD") == 0 || strcmp(lang, "bangla") == 0) {
        g_useBangla = true;
    }
}

const char *loc_str(LocKey key)
{
    return lookup_entry(key);
}
#endif
