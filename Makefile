# ===============================
# NotepadLite Makefile
# ===============================

WINDOWS_CC ?= x86_64-w64-mingw32-gcc
WINDOWS_WINDRES ?= x86_64-w64-mingw32-windres
WINDOWS32_CC ?= i686-w64-mingw32-gcc
WINDOWS32_WINDRES ?= i686-w64-mingw32-windres
BUILD_ARCH ?= x64
EXE_NAME ?= NotepadLite-$(BUILD_ARCH).exe
ALIAS_NAME ?=

# Compiler and Tools
CC      := $(WINDOWS_CC)
WINDRES := $(WINDOWS_WINDRES)

# Directories
SRC_DIR := src
OBJ_DIR := build/obj/$(BUILD_ARCH)
BIN_DIR := build/bin
RES_DIR := resources

# Source, Object, and Resource Files
SRC := $(SRC_DIR)/main.c $(SRC_DIR)/ui.c $(SRC_DIR)/file_io.c $(SRC_DIR)/localization.c
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
RES := $(RES_DIR)/notepadlite.rc
RES_OBJ := $(OBJ_DIR)/notepadlite_res.o

# Compiler Flags
CFLAGS := -Os -s -Wall -Wextra -Werror \
          -DUNICODE -D_UNICODE -municode \
          -ffunction-sections -fdata-sections \
          -fno-ident -fno-asynchronous-unwind-tables \
          -Iinclude

# Linker Flags
LDFLAGS := -s -municode -mwindows \
           -Wl,--gc-sections \
           -Wl,--strip-all \
           -Wl,--build-id=none \
           -Wl,--no-insert-timestamp \
           -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -lshell32

# ===============================
# Targets
# ===============================

# Default target
all: $(BIN_DIR)/$(EXE_NAME)

# Compile C source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile resource file
$(RES_OBJ): $(RES) | $(OBJ_DIR)
	$(WINDRES) -Iinclude $< -O coff -o $@

# Link executable
$(BIN_DIR)/$(EXE_NAME): $(OBJ) $(RES_OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) $(RES_OBJ) $(LDFLAGS) -o $@
ifneq ($(strip $(ALIAS_NAME)),)
	cp $@ $(BIN_DIR)/$(ALIAS_NAME)
endif

# Create necessary directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

# Phony targets
.PHONY: all clean windows32 windows64 windows

windows64:
	$(MAKE) CC=$(WINDOWS_CC) WINDRES=$(WINDOWS_WINDRES) BUILD_ARCH=x64 EXE_NAME=NotepadLite.exe ALIAS_NAME=NotepadLite-x64.exe all

windows32:
	$(MAKE) CC=$(WINDOWS32_CC) WINDRES=$(WINDOWS32_WINDRES) BUILD_ARCH=x86 EXE_NAME=NotepadLite-x86.exe all

windows: windows64 windows32
