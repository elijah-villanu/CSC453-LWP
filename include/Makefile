CC 	= gcc

CFLAGS  = -Wall -g -I .

LD 	= gcc

LDFLAGS  = -Wall -g 

PROGS	= snakes nums hungry

SNAKEOBJS  = randomsnakes.o 

HUNGRYOBJS = hungrysnakes.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) 

SRCS	= randomsnakes.c numbersmain.c hungrysnakes.c

HDRS	= 

EXTRACLEAN = core $(PROGS)

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) lwp.o rr.o util.o magic64.o liblwp.a liblwp.so core.* *~ TAGS

snakes: randomsnakes.o liblwp.a libsnakes.a
	$(LD) $(LDFLAGS) -o snakes randomsnakes.o -L. -lncurses -lsnakes -lLWP

hungry: hungrysnakes.o liblwp.a libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrysnakes.o -L. -lncurses -lsnakes -lLWP

nums: numbersmain.o liblwp.a 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -lLWP

hungrysnakes.o: lwp.h snakes.h

randomsnakes.o: lwp.h snakes.h

numbermain.o: lwp.h

liblwp.a: lwp.c rr.c util.c magic64.S
	$(CC) $(CFLAGS) -c rr.c util.c lwp.c magic64.S
	ar r liblwp.a util.o lwp.o rr.o magic64.o
	rm lwp.o

liblwp.so: lwp.c rr.c util.c magic64.S
	$(CC) $(CFLAGS) -fPIC -c rr.c util.c lwp.c magic64.S
	$(CC) -shared -o liblwp.so lwp.o rr.o util.o magic64.o

submission: lwp.c rr.c util.c Makefile README
	tar -cf project2_submission.tar lwp.c rr.c Makefile README
	gzip project2_submission.tar
