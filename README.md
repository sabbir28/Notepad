# NotepadLite

NotepadLite is a lightweight Windows text editor written in C. It focuses on fast launch, low memory usage, and a clean, familiar editing surface.

## What it does today

**Core editor**
* RichEdit-based multi-line editor with scrollbars and keyboard navigation.
* Undo, Cut, Copy, Paste, and Select All.
* Optional “Always on Top” toggle.
* Drag-and-drop file open support.

**File handling**
* New, Open, Save, and Save As.
* Encoding detection on open (UTF-8 BOM, UTF-16 LE/BE, ANSI fallback).
* Writes files as UTF-8 (with BOM), UTF-16 LE/BE, or ANSI.

**Status bar**
* Line/column indicator.
* Document size (B/KB/MB).
* Current encoding label.

## Current limitations (what’s missing)

These are intentional gaps or not-yet-implemented areas:
* No search, replace, or go-to-line dialog.
* No syntax highlighting or language modes.
* No tabs or multi-document UI.
* No autosave, crash recovery, or recent files list.
* No print/export or PDF support.
* No configurable line endings (CRLF/LF) or encoding picker UI.

## Localization & language support

Runtime language selection is supported for English (default) and Bangla (বাংলা):

* Set `NOTEPADLITE_LANG=bn` (or `bn-BD`, `bangla`) to enable Bangla strings.
* If `NOTEPADLITE_LANG` is not set, the app falls back to the OS locale (e.g., `bn` will auto-enable Bangla).
* To provide your own translations, point `NOTEPADLITE_LOCALE_FILE` at a UTF-8 `key=value` file using keys like `APP_TITLE`, `MENU_FILE`, and `STATUS_LN_COL` (see `src/localization.c` for the full list).
* In the Windows UI, use **View → Language** to switch between English and Bangla at runtime.
* All UI labels, system messages, and console prompts are localized via a shared string table.

For operational details, see `docs/operations.md`.

## Suggested future features

If you want to extend NotepadLite, these are practical next steps:
* **Find/Replace** dialog with incremental search.
* **Recent files** and **last session restore**.
* **Encoding & line-ending selector** in the status bar.
* **Preferences** (font, theme, tab width, wrap).
* **Search highlighting** and **matched bracket/quote** helpers.
* **Export/Print** workflows.
* **Diagnostics/Analysis panel** if advanced text insights are desired.

## Latest build

Download the most recent release package:
👉 **Download Latest Build:**  
[https://github.com/sabbir28/NotepadLite/releases/latest](https://github.com/sabbir28/Notepad/releases)

## Build workflow (Windows 7–11)

NotepadLite targets Windows 7 through Windows 11. Build on Windows using MinGW-w64 (MSYS2) or a compatible MinGW toolchain that provides `make`, `gcc`, and `windres`.

```bash
# Clone the repository
git clone https://github.com/sabbir28/NotepadLite.git
cd NotepadLite

# Build using the provided Makefile (MinGW cross-compiler required)
make windows64

# Output binary will appear in:
./build/bin/NotepadLite.exe
```

### Windows helper script

If you prefer a single command that validates dependencies and builds the Windows binary, use:

```bash
./scripts/build_windows.sh
```

Environment prerequisites:

* MinGW-w64 toolchain (via MSYS2 or similar)
* `make` build utilities

## Operations & deployment

Operational expectations for localization, update scheduling (20-minute incremental uploads), and platform deployment targets are documented in `docs/operations.md`.

## Installation

1. Download the latest build or compile on Windows using the Makefile.
2. Place the executable in your preferred working path or integrate it system-wide.
3. Launch and begin executing your editing workflows immediately.

## Contact & engagement

* **Facebook:** [https://fb.com/sabbir28.github.io](https://fb.com/sabbir28.github.io)
* **Email:** [sabbirb28@gmail.com](mailto:sabbirb28@gmail.com)

## License

Distributed under standard open-source licensing terms as defined in the repository.
