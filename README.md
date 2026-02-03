# NotepadLite

NotepadLite is a lightweight Windows text editor written in C. It focuses on fast launch, low memory usage, and a clean, familiar editing surface.

## What it does today

**Core editor**
* RichEdit-based multi-line editor with scrollbars and keyboard navigation.
* Undo, Cut, Copy, Paste, and Select All.
* Optional “Always on Top” toggle.
* Drag-and-drop file open support.

**Linux console mode**
* Runs in the terminal (no GUI) and reads/writes UTF-8 text.
* Prints existing file contents, then accepts new input until EOF (Ctrl-D).

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
* GUI is Windows-only (Win32 + RichEdit); Linux uses console mode.

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

## Build workflow (Windows via WSL + Makefile)

Execute a seamless cross-platform build pipeline through WSL:

```bash
# Clone the repository
git clone https://github.com/sabbir28/NotepadLite.git
cd NotepadLite

# Build using the provided Makefile (MinGW cross-compiler required)
make

# Output binary will appear in:
./build/bin/notepad.exe
```

Environment prerequisites:

* WSL (Ubuntu recommended)
* `mingw-w64` toolchain
* `make` build utilities

## Build workflow (Linux)

On Linux, the same source builds a simple console editor:

```bash
make
./build/bin/NotepadLite [file]
```

If no file is supplied, it writes to `notepad.txt` in the current directory.

## Installation

1. Download the latest build or compile via WSL using the Makefile.
2. Place the executable in your preferred working path or integrate it system-wide.
3. Launch and begin executing your editing workflows immediately.

## Contact & engagement

* **Facebook:** [https://fb.com/sabbir28.github.io](https://fb.com/sabbir28.github.io)
* **Email:** [sabbirb28@gmail.com](mailto:sabbirb28@gmail.com)

## License

Distributed under standard open-source licensing terms as defined in the repository.
