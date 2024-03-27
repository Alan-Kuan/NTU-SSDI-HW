CC = $(CROSS)gcc
CFLAGS = -Wall -static -I $(INCLUDE)
INCLUDE = ../module
EXE = rk

.PHONY: all clean

all: $(EXE)

$(EXE): $(EXE).c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	-rm $(EXE)
