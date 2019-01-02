#
# Package name and list of source files.
#

NAME=R3Shapes
CCSRCS=$(NAME).cpp \
    R3Draw.cpp \
    R3MeshSearchTree.cpp R3MeshPropertySet.cpp R3MeshProperty.cpp \
    R3Isect.cpp R3Cont.cpp R3Dist.cpp R3Parall.cpp R3Perp.cpp R3Relate.cpp R3Align.cpp R3Kdtree.cpp \
    R3CatmullRomSpline.cpp R3Polyline.cpp R3Curve.cpp \
    R3Mesh.cpp R3Ellipse.cpp R3Circle.cpp R3TriangleArray.cpp R3Triangle.cpp R3Surface.cpp \
    R3Ellipsoid.cpp R3Sphere.cpp R3Cone.cpp R3Cylinder.cpp R3OrientedBox.cpp R3Box.cpp R3Solid.cpp \
    R3Shape.cpp \
    R3Affine.cpp R3Xform.cpp R3Crdsys.cpp R3Triad.cpp R3Quaternion.cpp R4Matrix.cpp \
    R3PlanarGrid.cpp R3Grid.cpp \
    R3Halfspace.cpp R3Plane.cpp R3Span.cpp R3Ray.cpp R3Line.cpp R3Point.cpp R3Vector.cpp \
    R3Base.cpp \
    ply.cpp


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





