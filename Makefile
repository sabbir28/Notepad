# ===============================
# NotepadLite Makefile
# ===============================

# Compiler and Tools
CC      := i686-w64-mingw32-gcc
WINDRES := i686-w64-mingw32-windres

# Directories
SRC_DIR := src
OBJ_DIR := build/obj
BIN_DIR := build/bin
RES_DIR := resources

# Source, Object, and Resource Files
SRC := $(SRC_DIR)/main.c $(SRC_DIR)/ui.c $(SRC_DIR)/file_io.c
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
all: $(BIN_DIR)/NotepadLite.exe

# Compile C source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile resource file
$(RES_OBJ): $(RES) | $(OBJ_DIR)
	$(WINDRES) -Iinclude $< -O coff -o $@

# Link executable
$(BIN_DIR)/NotepadLite.exe: $(OBJ) $(RES_OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) $(RES_OBJ) $(LDFLAGS) -o $@

# Create necessary directories
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Clean build files
clean:
	rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

# Phony targets
.PHONY: all clean
