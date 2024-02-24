CC = gcc
CFLAGS = -Wall
EXE = cs5374_sh

.PHONY: all clean

all: $(EXE)

$(EXE): src/main.c
	$(CC) -o $@ $(CFLAGS) $<

clean:
	-rm $(EXE)
