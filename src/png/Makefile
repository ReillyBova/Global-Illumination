#
# Package name
#

NAME=png



#
# List of source files
#

# JPEG library source files
PNG_CSRCS = \
  png.c \
  pngset.c \
  pngget.c \
  pngrutil.c \
  pngtrans.c \
  pngwutil.c \
  pngread.c \
  pngrio.c \
  pngwio.c \
  pngwrite.c \
  pngrtran.c \
  pngwtran.c \
  pngmem.c \
  pngerror.c \
  pngpread.c 

ZLIB_CSRCS = \
  adler32.c \
  compress.c \
  crc32.c \
  deflate.c \
  gzclose.c \
  gzlib.c \
  gzread.c \
  gzwrite.c \
  infback.c \
  inffast.c \
  inflate.c \
  inftrees.c \
  trees.c \
  uncompr.c \
  zutil.c

CSRCS = $(PNG_CSRCS) $(ZLIB_CSRCS)



#
# Targets
#

OBJS=$(CCSRCS:.cpp=.o) $(CSRCS:.c=.o) 
INCS=$(HSRCS) $(CCSRCS:.cpp=.h) $(CSRCS:.c=.h)



#
# C flags
#

CC=g++
BASE_CFLAGS=$(USER_CFLAGS) -Wall -I. -I..
DEBUG_CFLAGS=$(BASE_CFLAGS) -g
OPT_CFLAGS=$(BASE_CFLAGS) -O3 -DNDEBUG
CFLAGS=$(DEBUG_CFLAGS)



#
# Default rules
#

.SUFFIXES: .c .cpp .o

.cpp.o:
	$(CC) $(CFLAGS) -c $<

.c.o:
	gcc $(CFLAGS) -c $<



#
# Set up target name
#

LIB=./lib$(NAME).a



#
# Make targets
#

opt:
	    $(MAKE) $(LIB) "CFLAGS=$(OPT_CFLAGS)" 

debug:
	    $(MAKE) $(LIB) "CFLAGS=$(DEBUG_CFLAGS)" 

$(LIB):     $(CCSRCS) $(CSRCS) $(OSRCS) $(OBJS) 
	    rm -f $(LIB)
	    ar ur $(LIB) $(OBJS) $(USER_OBJS)

clean:
	    - rm -f *~ *.o $(LIB)
