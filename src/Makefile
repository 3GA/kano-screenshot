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

KS_OBJS=kano-screenshot.o xwindows.o dispmanx_grabber.o
KS_BIN=kano-screenshot

KV_OBJS=encode.o dispmanx_grabber.o
KV_BIN=kano-screenscapture

# By default we enable full code optimization, unless "debug" is requested
CDEBUG="-O3"

CFLAGS+=-Wall -std=c++0x $(shell libpng-config --cflags) -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -U_FORTIFY_SOURCE  -DHAVE_LIBOPENMAX=2 -DOMX -DOMX_SKIP64BIT -ftree-vectorize -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -Wno-psabi -Wwrite-strings -fpermissive

KS_LDFLAGS =-L/opt/vc/lib/ -lbcm_host $(shell libpng-config --ldflags) -lX11
INCLUDES+=-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -Iilclient
KV_LDFLAGS = -L/opt/vc/lib/ -lbcm_host -lopenmaxil  -lvcos  -lpthread 

all: $(KS_BIN) $(KV_BIN)

debug:
	make CDEBUG="-ggdb -DDEBUG" all

%.o: %.cpp
	@rm -f $@ 
	$(CXX) $(CFLAGS) $(INCLUDES) $(CDEBUG) -c $< -o $@ -Wno-deprecated-declarations

$(KS_BIN): $(KS_OBJS)
	$(CXX) -o $@ -Wl,--whole-archive $(KS_OBJS) $(KS_LDFLAGS) -Wl,--no-whole-archive -rdynamic

$(KV_BIN): $(KV_OBJS) ilclient/libilclient.a
	$(CXX) -o $@ -Wl,--whole-archive $(KV_OBJS) ilclient/libilclient.a $(KV_LDFLAGS) -Wl,--no-whole-archive -rdynamic

ilclient/libilclient.a:
	$(MAKE) -C ilclient libilclient.a

clean:
	@rm -f $(KV_OBJS) $(KS_OBJS)
	@rm -f $(KV_BIN) $(KS_BIN)
	make -C ilclient clean
