# Makefile
# 
# Copyright (C) 2014 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
# 
# bare-screenshot is a compact POC that takes a screenshot and shows how to crop it into a PPM graphics file


APP=bare-screenshot

CFLAGS+=-Wall -ggdb -O3 -DDEBUG
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host
INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: $(APP)

$(APP).o: $(APP).c
	$(CC) $(CFLAGS) $(INCLUDES) -c $(APP).c -o $(APP).o -Wno-deprecated-declarations

$(APP): $(APP).o
	$(CC) -o $@ -Wl,--whole-archive $(APP).o $(LDFLAGS) -Wl,--no-whole-archive -rdynamic
