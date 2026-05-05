CC=gcc
CFLAGS=-Wall -Wextra -pedantic -pthread

all: czytelnicy_pierwsi pisarze_pierwsi sprawiedliwe

czytelnicy_pierwsi: czytelnicy_pierwsi.c
	$(CC) $(CFLAGS) czytelnicy_pierwsi.c -o czytelnicy_pierwsi

pisarze_pierwsi: pisarze_pierwsi.c
	$(CC) $(CFLAGS) pisarze_pierwsi.c -o pisarze_pierwsi

sprawiedliwe: sprawiedliwe.c
	$(CC) $(CFLAGS) sprawiedliwe.c -o sprawiedliwe

clean:
	rm -f czytelnicy_pierwsi pisarze_pierwsi sprawiedliwe
