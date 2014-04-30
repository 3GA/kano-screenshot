#
# Makefile to build kano-screenshot
#

OBJS=kano-screenshot.o
BIN=kano-screenshot

CFLAGS+=-Wall -g -O3 $(shell libpng-config --cflags)
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host $(shell libpng-config --ldflags)

INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: $(BIN) haw

# haw is a POC to make sure we can crop a screenshot as fast as we can
haw.o: haw.c
	$(CC) $(CFLAGS) $(INCLUDES) -g -c haw.c -o haw.o -Wno-deprecated-declarations

haw: haw.o
	$(CC) -o $@ -Wl,--whole-archive haw.o $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

%.o: %.c
	@rm -f $@ 
	$(CC) $(CFLAGS) $(INCLUDES) -g -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CC) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
