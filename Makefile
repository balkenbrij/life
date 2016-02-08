CFLAGS = -std=c99 -O2 -pipe -Wall -Wextra -pedantic `sdl2-config --cflags`
LFLAGS = `sdl2-config --libs`
PROG   = life

$(PROG): $(PROG).c
	$(CC) $(CFLAGS) -o $(PROG) $(PROG).c $(LFLAGS)
	$(CC) $(CFLAGS) -o /dev/null -g -Wa,-alhdn=$(PROG).c.s $(PROG).c $(LFLAGS)

profile: $(PROG).c
	$(CC) $(CFLAGS) -pg -o $(PROG) $(PROG).c $(LFLAGS)

indent: $(PROG).c
	indent -kr -di 12 $(PROG).c

strip: $(PROG)
	strip -s -R .note -R .comment $(PROG)

clean:
	rm $(PROG)
