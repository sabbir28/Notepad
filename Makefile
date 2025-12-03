CC      = x86_64-w64-mingw32-gcc
LD      = x86_64-w64-mingw32-ld
WINDRES = x86_64-w64-mingw32-windres

CFLAGS  = -O2 -Wall -Wextra -DUNICODE -D_UNICODE -municode -mwindows
LDFLAGS = -mwindows -nostartfiles -e _WinMain@16

LIBS    = -luser32 -lgdi32 -lcomctl32 -lcomdlg32 -ladvapi32 -lshell32 -lkernel32 -lmsvcrt

SRC = dialog.c main.c printing.c settings.c text.c
OBJ = $(SRC:.c=.o)

TARGET = notepad.exe

all: $(TARGET)

# THIS IS THE KEY LINE — bypass the broken crtexewin.o completely
$(TARGET): $(OBJ) rsrc.o
	$(CC) $(OBJ) rsrc.o -o $@ $(LDFLAGS) $(LIBS)

rsrc.o: rsrc.rc notepad.h
	$(WINDRES) $< -O coff -o $@

%.o: %.c notepad.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) rsrc.o $(TARGET)

strip: $(TARGET)
	x86_64-w64-mingw32-strip $(TARGET)

.PHONY: all clean strip