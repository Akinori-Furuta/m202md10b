CC ?= gcc
CFLAGS ?= -Wall -O2

all: m202md10b

m202md10b: m202md10b.c
	$(CC) $(CFLAGS) -o $@ $<
