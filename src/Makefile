#
# Makefile
# 
# Copyright (C) 2014, 2015 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
# 
# Builds kano-screenshot, release and debug versions
# 
#  "make" or "make all" will build kano-screenshot release fully optimized
#  "make debug" will bundle symbolic information for debugging purposes with no optimization
#

OBJS=kano-screenshot.o xwindows.o
BIN=kano-screenshot

# By default we enable full code optimization, unless "debug" is requested
CDEBUG="-O3"

CFLAGS+=-Wall -std=c++0x $(shell libpng-config --cflags)
LDFLAGS+=-L/opt/vc/lib/ -lbcm_host $(shell libpng-config --ldflags) -lX11
INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux

all: $(BIN)

debug:
	make CDEBUG="-ggdb -DDEBUG" all

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) $(CDEBUG) -c $< -o $@ -Wno-deprecated-declarations

$(BIN): $(OBJS)
	$(CXX) -o $@ -Wl,--whole-archive $(OBJS) $(LDFLAGS) -Wl,--no-whole-archive -rdynamic

clean:
	@rm -f $(OBJS)
	@rm -f $(BIN)
