.POSIX:
prefix     = /usr/local
# DEBUGFLAGS = -O2
DEBUGFLAGS = -Og -g -fsanitize=undefined,bounds,address
CFLAGS     = -std=c99 -Wall -Werror -Wpedantic -Wextra -Wvla $(DEBUGFLAGS)
LDLIBS     = -lsqlite3
PREREQS    = main.c nodes.o escaped_strings.o

all: paramlite

paramlite: $(PREREQS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(PREREQS) $(LDLIBS)

nodes.o: nodes.h
escaped_strings.o: escaped_strings.h

install: paramlite
	mkdir -p $(DESTDIR)$(prefix)/bin
	cp -p paramlite $(DESTDIR)$(prefix)/bin
	chmod 755 $(DESTDIR)$(prefix)/bin

uninstall:
	rm -f $(DESTDIR)$(prefix)/bin/paramlite

clean:
	rm -f escaped_strings.o nodes.o main.o
	rm -f paramlite
