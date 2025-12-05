#pragma once
#include <windows.h>

#define MAX_PATH_LEN 1024

typedef enum {
    ENC_ANSI = 0,
    ENC_UTF8,
    ENC_UTF16_LE,
    ENC_UTF16_BE
} FILE_ENCODING;

// Open a file and return its content as UTF-16, also outputs detected encoding
BOOL file_read(HWND hwnd, LPCWSTR filename, LPWSTR *outBuffer, DWORD *outSize, FILE_ENCODING *detectedEncoding);

// Write UTF-16 buffer to file with specified encoding
BOOL file_write(HWND hwnd, LPCWSTR filename, LPCWSTR buffer, DWORD size, FILE_ENCODING encoding);

BOOL file_open_dialog(HWND hwnd, LPWSTR outPath, DWORD maxLen);
BOOL file_save_dialog(HWND hwnd, LPWSTR outPath, DWORD maxLen);


FILE_ENCODING file_detect_encoding(LPCWSTR filePath);

