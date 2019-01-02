#
# Package name and list of source files.
#

NAME=R2Shapes
CCSRCS=R2Shapes.cpp \
    R2Draw.cpp R2Io.cpp R2Kdtree.cpp \
    R2Dist.cpp R2Cont.cpp R2Isect.cpp R2Parall.cpp R2Perp.cpp R2Relate.cpp R2Align.cpp \
    R2Grid.cpp \
    R2Polyline.cpp R2Arc.cpp R2Curve.cpp \
    R2Polygon.cpp R2Circle.cpp R2Box.cpp R2Solid.cpp \
    R2Shape.cpp \
    R2Affine.cpp R2Xform.cpp R2Crdsys.cpp R2Diad.cpp R3Matrix.cpp \
    R2Halfspace.cpp R2Span.cpp R2Ray.cpp R2Line.cpp R2Point.cpp R2Vector.cpp \
    R2Image.cpp



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
