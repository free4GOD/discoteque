CFLAGS  = -std=c11 -Wall -Os
LDFLAGS = -lSDL2

CFLAGS  += -D_DEFAULT_SOURCE

prog = discoteque
objs = discoteque.o

$(prog): $(objs)
	$(CC) $(LDFLAGS) -o $@ $(objs)

.PHONY: clean
clean:
	rm -f $(prog) $(objs)
