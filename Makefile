#
# Makefile - builds kano-screenshot
# 
# Copyright (C) 2014, 2015 Kano Computing Ltd.
#

all: kano-screenshot

kano-screenshot:
	cd src && make all

debug:
	cd src && make debug -B

clean:
	cd src && make clean
