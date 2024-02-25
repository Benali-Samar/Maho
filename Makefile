EXEC=maho
CFLAGS=-Wall -Wextra -pedantic -std=c99
LDFLAGS=

$(EXEC): maho.c
	$(CC) $^ -o $@ $(CFLAGS)
