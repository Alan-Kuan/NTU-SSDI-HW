MAKE = make

.PHONY: all clean

all:
	$(MAKE) -C module
	$(MAKE) -C cli
	cp module/rootkit.ko cli/rk ../shared

clean:
	$(MAKE) -C module clean
	$(MAKE) -C cli clean
