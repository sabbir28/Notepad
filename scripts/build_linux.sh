#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v make >/dev/null 2>&1; then
  echo "Error: 'make' is required but not installed." >&2
  exit 1
fi

if ! command -v gcc >/dev/null 2>&1; then
  echo "Error: 'gcc' is required but not installed." >&2
  exit 1
fi

missing_deps=()
if ! command -v pkg-config >/dev/null 2>&1; then
  missing_deps+=("pkg-config")
fi

if ! pkg-config --exists gtk+-3.0 >/dev/null 2>&1; then
  missing_deps+=("libgtk-3-dev")
fi

if ((${#missing_deps[@]})); then
  echo "Warning: GTK build dependencies not found (${missing_deps[*]})." >&2
  echo "Falling back to console build (USE_GTK=0)." >&2
  make USE_GTK=0
else
  make
fi

echo "Build complete. Binary located at: build/bin/NotepadLite"
