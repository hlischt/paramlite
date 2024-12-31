.POSIX:
prefix     = /usr/local
DEBUGFLAGS = -O2
# DEBUGFLAGS = -Og -g -fsanitize=undefined,bounds,address
CFLAGS     = -std=c99 -Wall -Werror -Wpedantic -Wextra -Wvla $(DEBUGFLAGS)
LDLIBS     = -lsqlite3
PREREQS    = main.o nodes.o escaped_strings.o

all: paramlite

paramlite: $(PREREQS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(PREREQS) $(LDLIBS)

main.o:
nodes.o: nodes.h
escaped_strings.o: escaped_strings.h

install: paramlite paramlite.1
	mkdir -p $(DESTDIR)$(prefix)/bin
	cp -p paramlite $(DESTDIR)$(prefix)/bin/
	chmod 755 $(DESTDIR)$(prefix)/bin/paramlite
	mkdir -p $(DESTDIR)$(prefix)/share/man/man1
	cp -p paramlite.1 $(DESTDIR)$(prefix)/share/man/man1/

uninstall:
	rm -f $(DESTDIR)$(prefix)/bin/paramlite
	rm -f $(DESTDIR)$(prefix)/share/man/man1/paramlite.1

clean:
	rm -f escaped_strings.o nodes.o main.o
	rm -f paramlite
