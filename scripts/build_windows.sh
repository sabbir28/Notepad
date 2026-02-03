#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

if ! command -v make >/dev/null 2>&1; then
  echo "Error: 'make' is required but not installed." >&2
  exit 1
fi

has_32=1
has_64=1
has_llvm=1

if ! command -v i686-w64-mingw32-gcc >/dev/null 2>&1; then
  has_32=0
fi

if ! command -v i686-w64-mingw32-windres >/dev/null 2>&1; then
  has_32=0
fi

if ! command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
  has_64=0
fi

if ! command -v x86_64-w64-mingw32-windres >/dev/null 2>&1; then
  has_64=0
fi

if ! command -v clang >/dev/null 2>&1; then
  has_llvm=0
fi

if ! command -v llvm-rc >/dev/null 2>&1; then
  if ! command -v llvm-windres >/dev/null 2>&1; then
    has_llvm=0
  else
    export WINDRES=llvm-windres
  fi
else
  export WINDRES=llvm-rc
fi

if [ "$has_32" -eq 1 ]; then
  make windows32
elif [ "$has_64" -eq 1 ]; then
  make windows64
elif [ "$has_llvm" -eq 1 ]; then
  make windows64 TOOLCHAIN=llvm CC=clang
else
  echo "Error: MinGW-w64 toolchains not found (missing i686-w64-mingw32-gcc/windres and x86_64-w64-mingw32-gcc/windres)." >&2
  echo "LLVM option: install clang + llvm-rc (or llvm-windres) and run 'make windows64 TOOLCHAIN=llvm'." >&2
  echo "Install mingw-w64 (WSL/Ubuntu): sudo apt-get install -y mingw-w64" >&2
  exit 1
fi

echo "Build complete. Binaries located at:"
ls -1 build/bin/*.exe
