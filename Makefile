CC      = x86_64-w64-mingw32-gcc
LD      = x86_64-w64-mingw32-ld
WINDRES = x86_64-w64-mingw32-windres

CFLAGS  = -O2 -Wall -Wextra -DUNICODE -D_UNICODE -municode -mwindows
LDFLAGS = -mwindows -nostartfiles -e _WinMain@16

LIBS    = -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -ladvapi32 -lshell32 -lkernel32 -lmsvcrt

SRC = src/file_io.c src/main.c src/ui.c
OBJ = $(SRC:.c=.o)

TARGET = build/bin/notepad.exe

all: $(TARGET)

$(TARGET): $(OBJ) rsrc.o
	@mkdir -p build/bin
	$(CC) $(OBJ) rsrc.o -o $@ $(LDFLAGS) $(LIBS)

rsrc.o: resources/notepadlite.rc include/notepad.h
	$(WINDRES) $< -O coff -o $@

%.o: %.c include/notepad.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) rsrc.o $(TARGET)

strip: $(TARGET)
	x86_64-w64-mingw32-strip $(TARGET)

.PHONY: all clean strip
