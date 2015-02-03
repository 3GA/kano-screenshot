#
#  Makefile - builds kano-screenshot
#

all: kano-screenshot

kano-screenshot:
	cd src && make all

debug:
	cd src && make debug -B

clean:
	cd src && make clean
