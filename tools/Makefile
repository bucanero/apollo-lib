TOOLS	=	patcher parser dumper
COMMON	=	../source/crc_util.o ../source/list.o ../source/offzip.o ../source/patches.o ../source/decrypt.o \
			../source/loader.o ../source/packzip.o ../source/wildcard.o ../source/upy.o compat.o
DEPS	=	Makefile

CC	=	gcc
CFLAGS	=	-g -O3 -Wall -I../include -I../oosdk_libraries/polarssl-1.3.9/include -I. -D_GNU_SOURCE -DAPOLLO_CLI
LDFLAGS =	-lz -lm -lpolarssl -L../oosdk_libraries/polarssl-1.3.9/build/library

ifdef BIGENDIAN
CFLAGS	+=	-D__PS3_PC__
endif

OBJS	= $(COMMON) $(addsuffix .o, $(TOOLS))

all: $(TOOLS)

$(TOOLS): %: %.o $(COMMON) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(COMMON) $(LDFLAGS)

$(OBJS): %.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f $(OBJS) $(TOOLS)
