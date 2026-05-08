C = gcc
CFLAGS = -Wall -Iinclude -fPIC -std=gnu99
OBJS = lwp.o rr.o magic64.o

liblwp.so: $(OBJS)
	$(CC) -shared -o liblwp.so $(OBJS)

liblwp.a: $(OBJS)
	ar r liblwp.a $(OBJS)

lwp.o: include/lwp.c
	$(CC) $(CFLAGS) -c include/lwp.c

rr.o: include/rr.c
	$(CC) $(CFLAGS) -c include/rr.c

magic64.o: src/magic64.S
	$(CC) $(CFLAGS) -c src/magic64.S

nums: liblwp.a demos/numbersmain.c
	$(CC) $(CFLAGS) -o nums demos/numbersmain.c -L. -llwp

snakes: liblwp.a demos/randomsnakes.c demos/util.c
	$(CC) $(CFLAGS) -o snakes demos/randomsnakes.c demos/util.c -L. -llwp -Llib64 -lsnakes -lncurses

hungry: liblwp.a demos/hungrysnakes.c demos/util.c
	$(CC) $(CFLAGS) -o hungry demos/hungrysnakes.c demos/util.c -L. -llwp -Llib64 -lsnakes -lncurses

clean:
	rm -f .o.a .so core. .swp DETAILS.bak nums snakes hungry