
# assume mingw environement and gnu make

CC=clang
CFLAGS=-O2 -Wall -ansi -pedantic -Wno-unused-function -std=c11
LIBS=-lshlwapi
RM=rm -f

TARGET=wtouch.exe

$(TARGET): wtouch.c
	$(CC) $(CFLAGS) -s -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET)

