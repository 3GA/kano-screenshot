#
# Makefile to build kano-screenshot and prototype
#
#  "make" or "make all" will build kano-screenshot
#  "make prototype" will build the latter
#

OBJS=kano-screenshot.o
BIN=kano-screenshot

CFLAGS+=-Wall -g -O3 $(shell libpng-config --cflags)
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host $(shell libpng-config --ldflags)
INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

.PHONY: prototype

all: $(BIN)

debug:
	make CDEBUG="-ggdb -O3 -DDEBUG" all

prototype:
	cd prototype && make

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) $(CDEBUG) -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
