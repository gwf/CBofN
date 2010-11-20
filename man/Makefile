
.SUFFIXES: .1

vpath %.c ../src

include ../Makefile.inc

SRC     = $(DEMOS:%=%.c)
MANS    = $(SRC:%.c=%.1)

.c.1:
	perl ../scripts/cbn2man < $< > $@

default: $(MANS)

clean:
	rm -f *.1

