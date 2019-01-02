#
# Package name and list of source files.
#

NAME=RNBasics
CCSRCS=$(NAME).cpp \
	RNTime.cpp \
        RNGrfx.cpp RNRgb.cpp \
        RNHeap.cpp RNQueue.cpp RNArray.cpp \
	RNSvd.cpp RNIntval.cpp RNScalar.cpp \
 	RNType.cpp \
 	RNFlags.cpp \
	RNFile.cpp RNMem.cpp \
	RNError.cpp \
	RNBase.cpp 



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
