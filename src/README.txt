This directory contains skeleton code as a starting point for 
the photon mapping assignment of COS 526.
You should edit code mainly in render.cpp.


FILE STRUCTURE
==============

There are several files, but you should mainly change photonmap.cpp.

  photonmap.cpp - Interface for photonmapping
  render.cpp - Render function for photonmapping
  kdtview.cpp - Test program for visualizing k-d trees
  R3Graphics/ - A library for many useful things computer graphics 
  R3Shapes/ - A library for 3D shapes
  R2Shapes/ - A library for 2D shapes
  RNBasics/ - A library with basic useful stuff
  fglut/ - A library for creating windows for opengl apps (freeglut)
  jpeg/ - A library for reading/writing JPEG files
  photonmap.[vcproj/sln] - Project file for compiling photomap Visual Studio
  kdtview.[vcproj/sln] - Project file for compiling kdtview in Visual Studio
  Makefile - Unix/Mac makefile for building the project with "make". 


COMPILATION
===========

If you are developing on a Windows machine and have Visual Studio
installed, use the provided project project files (photonmap.vcxproj
and kdtview.vcxproj) to build the program. If you are developing on a
Mac or Linux machine, type "make". In either case, an executable
called photonmap (or photonmap.exe) will be created in the top-level 
directory.
