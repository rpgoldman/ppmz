#----- makefile for PPMZ -----
#
# Simple UNIX makefile, tweak as necessary.
#
# The "crblib" and "ppmz" sources are expected to be in adjacent
# subdirectories.
#
SHELL		= /bin/sh
CC			= gcc
CFLAGS		= -O3 -Wall

INCLUDES	= -I..

OBJS		= main.o ppmarray.o ppmdet.o ppmz_cfg.o order0.o \
			  ppmcoder.o ppmz.o ppmzhead.o version.o exclude.o ppmzesc.o lzpo12.o lzparray.o
LIBS		= ../crblib/libcrb.a -lm

all: prep ppmz

# create a "crbinc" link so the <crbinc/foo.h> includes will work
prep:
	@if [ -h crbinc ]; then rm -f crbinc; fi
	@ln -s ../crblib crbinc

.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $<

ppmz: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LIBS)

clean:
	rm -f *.o core *~

# simple test
check:
	./ppmz ppmz.c test.ppmz
	./ppmz test.ppmz test.out
	cmp test.out ppmz.c
	@if [ $$? -ne 0 ]; then echo "FAILED"; else echo "Success!"; fi

