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

OBJS=kano-screenshot.o xwindows.o encode.o
BIN=kano-screenshot

# By default we enable full code optimization, unless "debug" is requested
CDEBUG="-O3"

CFLAGS+=-Wall -std=c++0x $(shell libpng-config --cflags) -fpermissive
LDFLAGS+=-L/opt/vc/lib/ -L./ilclient -lbcm_host $(shell libpng-config --ldflags) -lX11 -lopenmaxil -lbcm_host -lvcos -lpthread -lilclient
INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/src/hello_pi/libs/ilclient

CFLAGS+=-DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE -Wall -g -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi

all: $(BIN) ilclient/libilclient.a

ilclient/libilclient.a:
	make -C ilclient 

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
