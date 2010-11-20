
.SUFFIXES: .1 .html

vpath %.1 ../man

include ../Makefile.inc

MANS = $(wildcard ../man/*.1)
HTML = $(MANS:../man/%.1=%.html)

.1.html:
	nroff -man < $< \
	  | perl ../scripts/man2html -compress -botm 6 -topm 5 -pgsize 66 \
	  | sed 's/H2/H4/g' > $@

default: $(HTML)

clean:
	rm -f *.html

