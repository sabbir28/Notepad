#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v make >/dev/null 2>&1; then
  echo "Error: 'make' is required but not installed." >&2
  exit 1
fi

missing=0

if ! command -v i686-w64-mingw32-gcc >/dev/null 2>&1; then
  echo "Error: 'i686-w64-mingw32-gcc' is required but not installed." >&2
  missing=1
fi

if ! command -v i686-w64-mingw32-windres >/dev/null 2>&1; then
  echo "Error: 'i686-w64-mingw32-windres' is required but not installed." >&2
  missing=1
fi

if [ "$missing" -ne 0 ]; then
  echo "Install mingw-w64 (WSL/Ubuntu): sudo apt-get install -y mingw-w64" >&2
  exit 1
fi

make windows32

echo "Build complete. Binaries located at:"
echo "  build/bin/NotepadLite-x86.exe"
