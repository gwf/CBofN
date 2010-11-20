#######################################################################

# Copyright (c) 1998 Gary William Flake -- Permission granted for any
# use provied that the author's comments are neither modified nor
# removed. No warranty is given or implied.

#######################################################################

default: progs

all: progs man html

docs: man html

progs: FORCE
	cd bin; make

man: FORCE
	cd man; make

html: FORCE
	cd html; make

FORCE:

#######################################################################

clean:
	cd bin; make clean

veryclean: clean
	cd man; make clean
	cd html; make clean

tar:
	make veryclean
	n=`pwd`; n=`basename $$n`; cd ..; tar cfz $$n.tgz $$n

../code.tgz: tar

dotar: ../code.tgz
	mcopy -o ../code.tgz a:

#######################################################################

all-dists: all-linux-dists
	make clean; make docs
	#
	rm -f ../html/cbn-win-bin.zip
	cp ../arch/win/*.exe bin
	cd ../..; zip -r cbn/html/cbn-win-bin.zip cbn/code
	rm bin/*.exe
	#
	cp ../arch/sun/* bin
	cd ../..; tar cvvfz cbn/html/cbn-sunos-bin.tgz cbn/code
	make clean


all-linux-dists:
	make veryclean
	cd ../..; tar cvvfz cbn/html/cbn-noarch-src.tgz cbn/code
	make docs
	cd ../..; tar cvvfz cbn/html/cbn-noarch-src+docs.tgz cbn/code
	rm ../html/cbn-noarch-src+docs.zip
	cd ../..; zip -r cbn/html/cbn-noarch-src+docs.zip cbn/code
	make all
	rm bin/*.[oa]
	cd ../..; tar cvvfz cbn/html/cbn-linux-bin.tgz cbn/code

cbn-noarch-src.tgz:
	make veryclean
	cd ../..; tar cvvfz cbn/html/$@ cbn/code

cbn-noarch-src+docs.tgz:
	make veryclean; make docs
	cd ../..; tar cvvfz cbn/html/$@ cbn/code

cbn-noarch-src+docs.zip:
	make clean; make docs
	rm -f ../html/$@
	cd ../..; zip -r cbn/html/$@ cbn/code

cbn-linux-bin.tgz:
	make all
	rm bin/*.[oa]
	cd ../..; tar cvvfz cbn/html/$@ cbn/code

cbn-win-bin.zip:
	make clean; make docs
	rm -f ../html/$@
	cp ../arch/win/*.exe bin
	cd ../..; zip -r cbn/html/$@ cbn/code
	rm bin/*.exe

cbn-sunos-bin.tgz:
	make clean; make docs
	cp ../arch/sun/* bin
	cd ../..; tar cvvfz cbn/html/$@ cbn/code
	make clean

#######################################################################

