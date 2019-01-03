########################################################################
# This Makefile can be used to execute your program once it is compiled.
# It can contain commands that generate images in your submitted
# output directory from the ones in your input directory.
########################################################################

RESOLUTION= 512 512
CFLAGS= -resolution $(RESOLUTION) -aa 2 -v -lt 64 -ss 0 -tt 64 -st 128 -md 128 -threads 8 -no_caustic -no_indirect -no_transmissive -no_specular
#RESOLUTION= 300 300
#CFLAGS= -resolution $(RESOLUTION) -aa 0 -v -lt 128 -no_rs -no_ss -no_fresnel -no_dt -no_ds -tt 32 -st 32 -md 8 -threads 8
#RESOLUTION= 1024 1024
#CFLAGS= -resolution $(RESOLUTION) -aa 1 -v -real -lt 64 -ss 64 -tt 128 -st 128 -md 64 -threads 8
#RESOLUTION= 4096 4096
#CFLAGS= -resolution $(RESOLUTION) -aa 1 -v -real -lt 128 -ss 128 -tt 256 -st 256 -md 128 -threads 8 -it 512
NICE=0# -20 is highest priority; negative values require root permissions

########################################################################
# Executable name
########################################################################

EXE=src/photonmap

########################################################################
# This command tells the make file how to execute src/lap
# to produce output from basic rules.
########################################################################

output/%.png: input/%.scn
	nice -n $(NICE) $(EXE) $< $(CFLAGS) $@

########################################################################
# "make all" or simply "make" should make all output images
########################################################################

all:
	cd src; make photonmap

visualize:
	cd src; make visualize

clean:
	cd src; make clean

$(EXE): src
	cd src; make photonmap

cornell:
	rm -f output/cornell.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/cornell.png

jensen:
	rm -f output/jensen.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/jensen.png

display:
	rm -f output/display.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/display.png

box:
	rm -f output/box.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/box.png

cos:
	rm -f output/cos526.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/cos526.png

cylinder:
	rm -f output/cylinder.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/cylinder.png

debug:
	rm -f output/debug.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/debug.png

dirlight1:
	rm -f output/dirlight1.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/dirlight1.png

dirlight2:
	rm -f output/dirlight2.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/dirlight2.png

distributed-slow:
	rm -f output/distributed-slow.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/distributed-slow.png

distributed:
	rm -f output/distributed.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/distributed.png

fourspheres:
	rm -f output/fourspheres.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/fourspheres.png

glass_cube_and_sphere:
	rm -f output/glass_cube_and_sphere.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/glass_cube_and_sphere.png

glass_cube:
	rm -f output/glass_cube.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/glass_cube.png

glass_spheres:
	rm -f output/glass_spheres.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/glass_spheres.png

ico:
	rm -f output/ico.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/ico.png

lights:
	rm -f output/lights.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/lights.png

lines:
	rm -f output/lines.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/lines.png

pointlight1:
	rm -f output/pointlight1.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/pointlight1.png

pointlight2:
	rm -f output/pointlight2.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/pointlight2.png

refraction:
	rm -f output/refraction.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/refraction.png

shininess:
	rm -f output/shininess.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/shininess.png

softshadow:
	rm -f output/softshadow.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/softshadow.png

specular:
	rm -f output/specular.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/specular.png

sphere:
	rm -f output/sphere.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/sphere.png

spotlight1:
	rm -f output/spotlight1.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/spotlight1.png

spotlight2:
	rm -f output/spotlight2.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/spotlight2.png

stack:
	rm -f output/stack.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/stack.png

stilllife:
	rm -f output/stilllife.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/stilllife.png

teapot:
	rm -f output/teapot.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/teapot.png

transform:
	rm -f output/transform.png
	cd src; make photonmap
	$(MAKE) $(EXE) \
	      output/transform.png

transmission:
	rm -f output/transmission.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/transmission.png

violin:
	rm -f output/violin.png
	cd src; make photonmap
	$(MAKE) $(EXE) output/violin.png
