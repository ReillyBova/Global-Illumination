# Global Illumination Renderer
This repository contains C++ program that uses multithreaded raytracing and photon mapping to render photorealistic images of 3D environments. The photon mapping technique was described in Henrik Wann Jensen ["Global Illumination using Photon Maps"](http://graphics.ucsd.edu/~henrik/papers/photon_map/global_illumination_using_photon_maps_egwr96.pdf).

## Getting Started

Follow these instructions in order to run this program on your local machine.

### A Note on Portability

This program was only tested on Mac OS Mojave 10.14.2. Because C++ multithreading is not especially portable, it is possible that `photonmap` will not compile on Linux or Windows operating systems. If so, the author suggests replacing all instances of `__thread` with `thread_local`. If this does not work, next try removing the code that splits execution among child threads in both `src/render.cpp` and `src/photonmap.cpp` as well as any keywords that the compiler does not recognize (leaving a single threaded program).

### Installing

Change the directory to the root folder of the program and enter `make all` into the command line. If the program fails to compile, try changing the directory to `/src/` and enter `make clean && make all`.

### Running the Program

Once `photonmap` has been compiled, run it in the command line using the following arguments:

```
$ ./photonmap src.scn [output.png] [-FLAGS]
```

Here is a breakdown of the meaning of the arguments and avaliable flags:
* src.scn => file path to input scene image (required)
* output.png => file path output; if not provided, program will launch OpenGL viewer
* General flag arguments:
  * "-resolution <int X> <int Y>" => Sets output image dimensions to X by Y. Default is X=1024 Y=1024
  * "-v" => Enables verbose output, which prints rendering statistics to the screen. Off by default
  * "-threads <int N>" => Sets the number of threads (including main thread) used to trace photons and render the image. Default is N=1.
  * "-aa <int N>" => Sets how many times the dimensions of the image should be doubled before downsampling (as a form of anti-aliasing) to the output image. To be more precise, there a 4^N rays sampled over an evenly-weighted grid per output pixel. Default is N=2
  * "-real" => Normalize the components of all materials in the scene such that they conserve energy. Off by default
  * "-no_fresnel" => Disables splitting transmissision into specular and refractive components based on angle of incident ray. Fresnel is enabled by default
  * "-ir <float N>" => Sets the refractive index of air. Default is N=1.0
Illumination flags:
  * "-no_ambient" => Disables ambient lighting in scene. Ambient lighting is enabled by default
  * "-no_direct" => Disables direct illumination (immediate illumination of surfaces by a light source) in scene. Direct illumination is enabled by default
  * "-no_transmissive" => Disables transmissive illumination (light carried by refraction through optical media) in scene. Transmissive illumination is enabled by default
  * "-no_specular" => Disables specular illumination (light bouncing off of surface) in scene. Specular illumination is enabled by default
  * "-no_indirect" => Disables indirect illumination (light that bounces diffusely off of at least one surface — this is the primary global illumination component) in scene. Indirect illumination is enabled by default
  * "-no_caustic" => Disables caustic illumination (light that is focused through mirror and optical media) in scene. Caustic illumination is enabled by default
  * "-photon_viz" => Enables direct radiance sampling of the global photon map for vizualization. This layer will (nearly) approach global illumination on its own if given large enough samples. Disabled by default
  * "-photon_viz" => Enables a faster estimate of global illumination by combining direct lighting with direct radiance sampling of a version of the global photon map where photons are only stored after their first diffuse bounce. Disabled by default
Monte Carlo flags:
  * "-no-monte" => Disables Monte Carlo path-tracing (used to compute specular and transmissive illumination). Monte Carlo is path-tracing is enabled by default
  * "-md <int N>" => Sets the max recursion depth of a Monte Carlo path-trace. Default is N=128
  * "-absorb <float N>" => Sets probability of a surface absorbing a photon. Default is N=0.005 (0.5%)
  * "-no_rs" => Disables recursive shadows (shadow sampling from within a specular or transmissive raytrace). Recursive shadows are enabled by default
  * "-no_dt" => Disables distributed importance sampling of transmissive rays based on material shininess. For materials with low shininess but high transmision values, this creates a "frosted glass" effect. Distributed transmissive ray sampling is enabled by default
 * "-tt <int N>" => Sets the number of test rays that should be sent when sampling a transmissive surface. Default is N=128
 * "-no_ds" => Disables distributed importance sampling of specular rays based on material shininess. For materials with low shininess but high specular values, this creates a "glossy surface" effect. Distributed specualar ray sampling is enabled by default
Photon Mapping flags:
 * "-global <int N>" => Sets the approximate number of photons that should be stored in the global map. Default is N=1920
 * "-caustic <int N>" => Sets the approximate number of photons that should be stored in the caustic map. Default is N=60000
 * "-md <int N>" => Sets the max recursion depth of a Photon trace in the photon mapping step. Default is N=128
 * "-it <int N>" => Sets the number of test rays that should be sent when sampling the indirect illumination of a surface. Default is N=256
 * "-gs <int N>" => Sets the number of photons used in a radiance sample of the global photon map. Default is N=50
 * "-gd <float N>" => Sets the max radius of a radiance sample of the global photon map. Default is N=2.5
 * "-cs <int N>" => Sets the number of photons used in a radiance sample of the caustic photon map. Default is N=60
 * "-cd <float N>" => Sets the max radius of a radiance sample of the caustic photon map. Default is N=0.2
Shadow Sampling flags:
 * "-no_shadow" => Disables shadows entirely. Shadows are enabled by default
 * "-no_ss" => Disables soft shadows. Soft shadows are enabled by default
 * "-lt <int N>" => Sets the number of occlusion + reflectance rays sent per light per sample. Used to compute both soft shadows and direct illumination by area light. Default is N=128
 * "-s <int N>" => Sets the number of occlusion (only) rays sent per light per sample. Used to take additional soft shadow estimates (on top of the number specified by the `-lt` flag). Default is N=128

