#######################################################################

# Copyright (c) 1998 Gary William Flake -- Permission granted for any
# use provied that the author's comments are neither modified nor
# removed. No warranty is given or implied.

# Makefile for example code -- Requires GNU make.

# Undefine the line below only if you want the Linux SVGA graphics
# driver builtin.  Note that you will have to make each program
# set-UID root for this to work.

#VGA      = 1
X11      = 1
XINCLUDE = /usr/X11/include
XLIBS    = /usr/X11/lib

COPTS    = -O3  -Wall  -D__dest_os=unix
CC       = gcc

#######################################################################

# Do not edit this section

vpath %.c ../src

ifdef VGA
ifdef X11
# VGA and X11
PLOTOBJS  = vgaplot.o x11plot.o x11cplot.o pgmplot.o psplot.o rawplot.o
PLOTFLAGS = -DPLOTX11 -DPLOTVGA
LIBS      = -lmisc -lm -lX11 -lvga
else
# VGA and !X11
PLOTOBJS  = vgaplot.o pgmplot.o psplot.o rawplot.o
PLOTFLAGS = -DPLOTVGA
LIBS      = -lmisc -lm -lvga
endif
else
ifdef X11
# !VGA and X11
PLOTOBJS  = x11plot.o x11cplot.o pgmplot.o psplot.o rawplot.o
PLOTFLAGS = -DPLOTX11
LIBS      = -lmisc -lm -lX11
else
# !VGA and !X11
PLOTOBJS  = pgmplot.o psplot.o rawplot.o
PLOTFLAGS =
LIBS      = -lmisc -lm
endif
endif

CFLAGS   = $(COPTS) -I$(XINCLUDE) $(PLOTFLAGS)
LDFLAGS  = -L. -L$(XLIBS)

include ../Makefile.inc

PROGS    = $(DEMOS)

#######################################################################

default: $(PROGS)

$(PROGS): % : %.o libmisc.a
	$(CC) -o $@ $< $(LDFLAGS) $(LIBS)

libmisc.a: misc.o plot.o $(PLOTOBJS)
	ar cr $@ $^
	ranlib $@

bifur1d.o phase1d.o: maps1d.c

clean:
	rm -f $(PROGS) *.a *.o

#######################################################################
