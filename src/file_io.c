#include "file_io.h"
#include <string.h>

// ------------------------------
// Detect BOM
// ------------------------------
static FILE_ENCODING detect_bom(const BYTE* data, DWORD size)
{
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        return ENC_UTF8;
    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
        return ENC_UTF16_LE;
    if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF)
        return ENC_UTF16_BE;
    return ENC_ANSI; // fallback
}

// ------------------------------
// Convert raw bytes to UTF-16
// ------------------------------
static BOOL convert_to_utf16(const BYTE* data, DWORD size, FILE_ENCODING encoding, LPWSTR *outBuffer, DWORD *outSize)
{
    if (!data || !outBuffer) return FALSE;

    LPWSTR wbuf = NULL;
    DWORD wsize = 0;

    switch (encoding)
    {
        case ENC_UTF8:
        {
            // skip BOM if present
            if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
            {
                data += 3;
                size -= 3;
            }
            int chars = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)data, size, NULL, 0);
            if (chars <= 0) return FALSE;
            wbuf = (LPWSTR)GlobalAlloc(GPTR, (chars + 1) * sizeof(WCHAR));
            if (!wbuf) return FALSE;
            MultiByteToWideChar(CP_UTF8, 0, (LPCCH)data, size, wbuf, chars);
            wbuf[chars] = L'\0';
            wsize = chars * sizeof(WCHAR);
            break;
        }

        case ENC_UTF16_LE:
        case ENC_UTF16_BE:
        {
            // handle BOM
            if (size >= 2 && ((data[0] == 0xFF && data[1] == 0xFE) || (data[0] == 0xFE && data[1] == 0xFF)))
            {
                data += 2;
                size -= 2;
            }

            if (encoding == ENC_UTF16_BE)
            {
                // swap bytes to little-endian
                wbuf = (LPWSTR)GlobalAlloc(GPTR, size + sizeof(WCHAR));
                if (!wbuf) return FALSE;
                for (DWORD i = 0; i < size / 2; ++i)
                {
                    ((BYTE*)&wbuf[i])[0] = data[i*2+1];
                    ((BYTE*)&wbuf[i])[1] = data[i*2];
                }
            }
            else
            {
                wbuf = (LPWSTR)GlobalAlloc(GPTR, size + sizeof(WCHAR));
                if (!wbuf) return FALSE;
                memcpy(wbuf, data, size);
            }
            wbuf[size/2] = L'\0';
            wsize = size;
            break;
        }

        case ENC_ANSI:
        default:
        {
            int chars = MultiByteToWideChar(CP_ACP, 0, (LPCCH)data, size, NULL, 0);
            if (chars <= 0) return FALSE;
            wbuf = (LPWSTR)GlobalAlloc(GPTR, (chars + 1) * sizeof(WCHAR));
            if (!wbuf) return FALSE;
            MultiByteToWideChar(CP_ACP, 0, (LPCCH)data, size, wbuf, chars);
            wbuf[chars] = L'\0';
            wsize = chars * sizeof(WCHAR);
            break;
        }
    }

    *outBuffer = wbuf;
    if (outSize) *outSize = wsize;
    return TRUE;
}

// ------------------------------
// Read file with auto-detect encoding
// ------------------------------
BOOL file_read(HWND hwnd, LPCWSTR filename, LPWSTR *outBuffer, DWORD *outSize, FILE_ENCODING *detectedEncoding)
{
    (void) hwnd;

    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    BYTE* buffer = (BYTE*)GlobalAlloc(GPTR, fileSize);
    if (!buffer)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buffer, fileSize, &bytesRead, NULL))
    {
        GlobalFree(buffer);
        CloseHandle(hFile);
        return FALSE;
    }

    FILE_ENCODING enc = detect_bom(buffer, bytesRead);
    if (detectedEncoding) *detectedEncoding = enc;

    BOOL ok = convert_to_utf16(buffer, bytesRead, enc, outBuffer, outSize);

    GlobalFree(buffer);
    CloseHandle(hFile);
    return ok;
}

// ------------------------------
// Save UTF-16 buffer to file with chosen encoding
// ------------------------------
BOOL file_write(HWND hwnd, LPCWSTR filename, LPCWSTR buffer, DWORD size, FILE_ENCODING encoding)
{
    (void) hwnd;

    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    BOOL success = FALSE;

    switch (encoding)
    {
        case ENC_UTF8:
        {
            BYTE bom[] = {0xEF,0xBB,0xBF};
            WriteFile(hFile, bom, sizeof(bom), NULL, NULL);

            int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);
            if (bytesNeeded > 0)
            {
                BYTE* outBuf = (BYTE*)GlobalAlloc(GPTR, bytesNeeded);
                if (outBuf)
                {
                    WideCharToMultiByte(CP_UTF8, 0, buffer, -1, (LPSTR)outBuf, bytesNeeded, NULL, NULL);
                    DWORD written;
                    success = WriteFile(hFile, outBuf, bytesNeeded-1, &written, NULL);
                    GlobalFree(outBuf);
                }
            }
            break;
        }

        case ENC_UTF16_LE:
        {
            BYTE bom[] = {0xFF, 0xFE};
            WriteFile(hFile, bom, sizeof(bom), NULL, NULL);
            DWORD written;
            success = WriteFile(hFile, buffer, size, &written, NULL) && (written == size);
            break;
        }

        case ENC_UTF16_BE:
        {
            BYTE bom[] = {0xFE, 0xFF};
            WriteFile(hFile, bom, sizeof(bom), NULL, NULL);
            DWORD written;
            for (DWORD i=0; i<size/2; ++i)
            {
                BYTE b[2];
                b[0] = ((BYTE*)&buffer[i])[1];
                b[1] = ((BYTE*)&buffer[i])[0];
                success = WriteFile(hFile, b, 2, &written, NULL);
                if (!success) break;
            }
            break;
        }

        case ENC_ANSI:
        default:
        {
            int bytesNeeded = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);
            if (bytesNeeded > 0)
            {
                BYTE* outBuf = (BYTE*)GlobalAlloc(GPTR, bytesNeeded);
                if (outBuf)
                {
                    WideCharToMultiByte(CP_ACP, 0, buffer, -1, (LPSTR)outBuf, bytesNeeded, NULL, NULL);
                    DWORD written;
                    success = WriteFile(hFile, outBuf, bytesNeeded-1, &written, NULL);
                    GlobalFree(outBuf);
                }
            }
            break;
        }
    }

    CloseHandle(hFile);
    return success;
}

// ------------------------------
// Open / Save Dialogs
// ------------------------------
BOOL file_open_dialog(HWND hwnd, LPWSTR outPath, DWORD maxLen)
{
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFile   = outPath;
    ofn.nMaxFile    = maxLen;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";

    return GetOpenFileNameW(&ofn);
}

BOOL file_save_dialog(HWND hwnd, LPWSTR outPath, DWORD maxLen)
{
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFile   = outPath;
    ofn.nMaxFile    = maxLen;
    ofn.Flags       = OFN_OVERWRITEPROMPT;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";

    return GetSaveFileNameW(&ofn);
}


// ----------------------------------------------------------------------
// Detect encoding from file path only (no full file load)
// ----------------------------------------------------------------------
FILE_ENCODING file_detect_encoding(LPCWSTR filePath)
{
    HANDLE hFile = CreateFileW(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return ENC_ANSI; // default fallback
    }

    BYTE bom[3] = {0};
    DWORD bytesRead = 0;

    ReadFile(hFile, bom, sizeof(bom), &bytesRead, NULL);
    CloseHandle(hFile);

    // UTF-8 BOM: EF BB BF
    if (bytesRead >= 3 &&
        bom[0] == 0xEF &&
        bom[1] == 0xBB &&
        bom[2] == 0xBF)
    {
        return ENC_UTF8;
    }

    // UTF-16 LE BOM: FF FE
    if (bytesRead >= 2 &&
        bom[0] == 0xFF &&
        bom[1] == 0xFE)
    {
        return ENC_UTF16_LE;
    }

    // UTF-16 BE BOM: FE FF
    if (bytesRead >= 2 &&
        bom[0] == 0xFE &&
        bom[1] == 0xFF)
    {
        return ENC_UTF16_BE;
    }

    // Default: ANSI
    return ENC_ANSI;
}