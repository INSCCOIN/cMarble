CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
OBJS = game.o phys.o draw.o fb.o scene.o

cMarble: $(OBJS)
	$(CC) $(CFLAGS) -o cMarble $(OBJS) -lm

clean:
	rm -f cMarble $(OBJS)
