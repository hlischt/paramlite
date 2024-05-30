.POSIX:
.SUFFIXES:
CFLAGS = -std=c99 -Wall -Werror -Wpedantic -Wextra -Wvla
DEBUGFLAGS = $(CFLAGS) -Og -g -fsanitize=undefined,bounds,address
SQLITEFLAGS = -lsqlite3

all: paramlite

run: paramlite
	./paramlite -h

paramlite: main.c nodes.o escaped_strings.o
	$(CC) $(DEBUGFLAGS) -o $@ $^ $(SQLITEFLAGS)

nodes.o: nodes.c nodes.h
	$(CC) -c $(DEBUGFLAGS) -o $@ $<

escaped_strings.o: escaped_strings.c escaped_strings.h
	$(CC) -c $(DEBUGFLAGS) -o $@ $<
