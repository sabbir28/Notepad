#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v make >/dev/null 2>&1; then
  echo "Error: 'make' is required but not installed." >&2
  exit 1
fi

if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  echo "Error: 'x86_64-w64-mingw32-gcc' is required but not installed." >&2
  echo "Install mingw-w64 (WSL/Ubuntu): sudo apt-get install -y mingw-w64" >&2
  exit 1
fi

if ! command -v x86_64-w64-mingw32-windres >/dev/null 2>&1; then
  echo "Error: 'x86_64-w64-mingw32-windres' is required but not installed." >&2
  echo "Install mingw-w64 (WSL/Ubuntu): sudo apt-get install -y mingw-w64" >&2
  exit 1
fi

make windows64

echo "Build complete. Binary located at: build/bin/NotepadLite.exe"
