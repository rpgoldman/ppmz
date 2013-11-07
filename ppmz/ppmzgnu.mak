
From cbloom@mail.utexas.edu Wed Aug 14 15:36:21 1996
Newsgroups: comp.compression
Path: geraldo.cc.utexas.edu!cs.utexas.edu!math.ohio-state.edu!howland.erols.net!spool.mu.edu!usenet.eel.ufl.edu!news.mathworks.com!news.kei.com!news.texas.net!nntp04.primenet.com!news.shkoo.com!nntp.primenet.com!netcom.com!fadden
From: fadden@netcom.com (Andy McFadden)
Subject: Re: ANNC: PPMZ v8.0
Message-ID: <faddenDw2CIB.MIE@netcom.com>
Organization: Lipless Rattling Crankbait
References: <4uankd$k67@geraldo.cc.utexas.edu> <ubbugg7l7l.fsf@jsrs2e.sto.se.ibm.com>
Date: Tue, 13 Aug 1996 06:25:23 GMT
Lines: 48
Sender: fadden@netcom4.netcom.com

In article <ubbugg7l7l.fsf@jsrs2e.sto.se.ibm.com>,
 <jonas@jsrs2e.sto.se.ibm.com> wrote:
>In article <4uankd$k67@geraldo.cc.utexas.edu> cbloom@mail.utexas.edu (Charles B
loom) writes:
>
>   PPMZ v8.0
>
>   Now available on my web page:
>
>   http://wwwvms.utexas.edu/~cbloom/src/indexppm.html
>
>   This version has no known bugs (unlike v7 which does!)
>
>Maybe you should write some "how to compile" instructions. Makefile?

Use something like this:

#
# Quicky makefile (requires GNU make)
#
SHELL           = /bin/sh
CC              = gcc
INCLUDES        = -I. -I../crblib
CFLAGS          = -O3 -Wall
 
HDRS            = $(wildcard *.h)
SRCS            = $(wildcard *.c)
OBJS            = $(notdir $(patsubst %.c, %.o, $(SRCS)))
LIBS            = ../crblib/libcrb.a -lm
 
 
.c.o:
        $(CC) -c $(CFLAGS) $(INCLUDES) $<
 
ppmz: $(OBJS)
        $(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)


For crblib, remove the last target and instead use:

libcrb.a: $(OBJS)
        ar ruv $@ $(OBJS)


-- 
fadden@netcom.com (Andy McFadden)    [These are strictly my opinions.]     PGP

      Anyone can do any amount of work provided it isn't the work he is
        supposed to be doing at the moment.        -- Robert Benchley

