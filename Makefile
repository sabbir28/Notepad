# ===============================
# NotepadLite Makefile
# ===============================

UNAME_S := $(shell uname -s)
WINDOWS_CC ?= x86_64-w64-mingw32-gcc
WINDOWS_WINDRES ?= x86_64-w64-mingw32-windres

# Compiler and Tools
ifeq ($(UNAME_S),Linux)
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0 2>/dev/null)
CC      := gcc
ifeq ($(strip $(GTK_CFLAGS)),)
USE_GTK ?= 0
else
USE_GTK ?= 1
endif
else
CC      := $(WINDOWS_CC)
WINDRES := $(WINDOWS_WINDRES)
endif

# Directories
SRC_DIR := src
OBJ_DIR := build/obj
BIN_DIR := build/bin
RES_DIR := resources

# Source, Object, and Resource Files
ifeq ($(UNAME_S),Linux)
ifeq ($(USE_GTK),1)
LINUX_UI_SRC := $(SRC_DIR)/linux_ui.c
LINUX_GTK_CFLAGS := $(GTK_CFLAGS) -DUSE_GTK
LINUX_GTK_LDFLAGS := $(GTK_LIBS)
else
LINUX_UI_SRC := $(SRC_DIR)/linux_console.c
LINUX_GTK_CFLAGS :=
LINUX_GTK_LDFLAGS :=
endif
SRC := $(SRC_DIR)/main.c $(LINUX_UI_SRC) $(SRC_DIR)/file_io.c $(SRC_DIR)/localization.c
else
SRC := $(SRC_DIR)/main.c $(SRC_DIR)/ui.c $(SRC_DIR)/file_io.c $(SRC_DIR)/localization.c
endif
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
RES := $(RES_DIR)/notepadlite.rc
RES_OBJ := $(OBJ_DIR)/notepadlite_res.o

# Compiler Flags
ifeq ($(UNAME_S),Linux)
CFLAGS := -Os -Wall -Wextra -Werror \
          -ffunction-sections -fdata-sections \
          -Iinclude $(LINUX_GTK_CFLAGS)
LDFLAGS := -Wl,--gc-sections $(LINUX_GTK_LDFLAGS)
else
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
endif

# ===============================
# Targets
# ===============================

# Default target
ifeq ($(UNAME_S),Linux)
all: $(BIN_DIR)/NotepadLite
else
all: $(BIN_DIR)/NotepadLite.exe
endif

# Compile C source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

ifeq ($(UNAME_S),Linux)
# Link executable
$(BIN_DIR)/NotepadLite: $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
else
# Compile resource file
$(RES_OBJ): $(RES) | $(OBJ_DIR)
	$(WINDRES) -Iinclude $< -O coff -o $@

# Link executable
$(BIN_DIR)/NotepadLite.exe: $(OBJ) $(RES_OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) $(RES_OBJ) $(LDFLAGS) -o $@
endif

# Create necessary directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

# Phony targets
.PHONY: all clean windows64 linux32

windows64:
	$(MAKE) UNAME_S=Windows_NT CC=$(WINDOWS_CC) WINDRES=$(WINDOWS_WINDRES) all

linux32:
	$(MAKE) UNAME_S=Linux CC=gcc USE_GTK=0 \
		CFLAGS='-Os -Wall -Wextra -Werror -m32 -ffunction-sections -fdata-sections -Iinclude' \
		LDFLAGS='-m32 -Wl,--gc-sections' \
		OBJ_DIR=build/obj32 BIN_DIR=build/bin32 all
