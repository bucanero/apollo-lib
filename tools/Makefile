TOOLS	=	patcher mdparser
COMMON	=	../source/crc_util.o ../source/list.o ../source/offzip.o ../source/patches.o ../source/decrypt.o \
			../source/loader.o ../source/packzip.o ../source/wildcard.o
DEPS	=	Makefile

CC	=	gcc
CFLAGS	=	-g -O3 -W -I../include -I../oosdk_libraries/polarssl-1.3.9/include -I. -D_GNU_SOURCE
LDFLAGS =	-lz -lpolarssl -L../oosdk_libraries/polarssl-1.3.9/build/library

OBJS	= $(COMMON) $(addsuffix .o, $(TOOLS))

all: $(TOOLS)

$(TOOLS): %: %.o $(COMMON) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(COMMON) $(LDFLAGS)

$(OBJS): %.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(OBJS) $(TOOLS)
