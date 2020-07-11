.POSIX:

PREFIX = /usr/local

ALL_WARN    = -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes
ALL_CFLAGS  = $(CFLAGS) $(CPPFLAGS) -std=c99 $(ALL_WARN)
ALL_LDFLAGS = $(LDFLAGS) -lxcb

CC = cc

all: sowm

sowm: sowm.c Makefile
	$(CC) -O3 $(ALL_CFLAGS) -o $@ $< $(ALL_LDFLAGS)

install: all
	mkdir -p $(DESTDIR)/bin
	cp sowm $(DESTDIR)/bin/sowm

uninstall:
	rm -f $(DESTDIR)/bin/sowm

clean:
	rm -f sowm *.o

.PHONY: all install uninstall clean
