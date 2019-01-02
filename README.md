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
$ ./photonmap src.scn output.png [-FLAGS]
```

There are also provided shortcuts in the Makefile to expedite the scene-rendering process (as the number of flags can grow rather lengthy).

Here is a breakdown of the meaning of the arguments, as well as the avaliable flags:
* src.scn => file path to input scene image (required)
* output.png => file path to output (required)
* General flag arguments:
  * `-resolution <int X> <int Y>` => Sets output image dimensions to X by Y. Default is X=1024 Y=1024
  * `-v` => Enables verbose output, which prints rendering statistics to the screen. Off by default
  * `-threads <int N>` => Sets the number of threads (including main thread) used to trace photons and render the image. Default is N=1.
  * `-aa <int N>` => Sets how many times the dimensions of the image should be doubled before downsampling (as a form of anti-aliasing) to the output image. To be more precise, there a 4^N rays sampled over an evenly-weighted grid per output pixel. Default is N=2
  * `-real` => Normalize the components of all materials in the scene such that they conserve energy. Off by default
  * `-no_fresnel` => Disables splitting transmissision into specular and refractive components based on angle of incident ray. Fresnel is enabled by default
  * `-ir <float N>` => Sets the refractive index of air. Default is N=1.0
Illumination flags:
  * `-no_ambient` => Disables ambient lighting in scene. Ambient lighting is enabled by default
  * `-no_direct` => Disables direct illumination (immediate illumination of surfaces by a light source) in scene. Direct illumination is enabled by default
  * `-no_transmissive` => Disables transmissive illumination (light carried by refraction through optical media) in scene. Transmissive illumination is enabled by default
  * `-no_specular` => Disables specular illumination (light bouncing off of surface) in scene. Specular illumination is enabled by default
  * `-no_indirect` => Disables indirect illumination (light that bounces diffusely off of at least one surface — this is the primary global illumination component) in scene. Indirect illumination is enabled by default
  * `-no_caustic` => Disables caustic illumination (light that is focused through mirror and optical media) in scene. Caustic illumination is enabled by default
  * `-photon_viz` => Enables direct radiance sampling of the global photon map for vizualization. This layer will (nearly) approach global illumination on its own if given large enough samples. Disabled by default
  * `-fast_global` => Enables a faster estimate of global illumination by combining direct lighting with direct radiance sampling of a version of the global photon map where photons are only stored after their first diffuse bounce. Disabled by default
* Monte Carlo flags:
  * `-no-monte` => Disables Monte Carlo path-tracing (used to compute specular and transmissive illumination). Monte Carlo is path-tracing is enabled by default
  * `-md <int N>` => Sets the max recursion depth of a Monte Carlo path-trace. Default is N=128
  * `-absorb <float N>` => Sets probability of a surface absorbing a photon. Default is N=0.005 (0.5%)
  * `-no_rs` => Disables recursive shadows (shadow sampling from within a specular or transmissive raytrace). Recursive shadows are enabled by default
  * `-no_dt` => Disables distributed importance sampling of transmissive rays based on material shininess. For materials with low shininess but high transmision values, this creates a "frosted glass" effect. Distributed transmissive ray sampling is enabled by default
  * `-tt <int N>` => Sets the number of test rays that should be sent when sampling a transmissive surface. Default is N=128
  * `-no_ds` => Disables distributed importance sampling of specular rays based on material shininess. For materials with low shininess but high specular values, this creates a "glossy surface" effect. Distributed specualar ray sampling is enabled by default
* Photon Mapping flags:
  * `-global <int N>` => Sets the approximate number of photons that should be stored in the global map. Default is N=1920
  * `-caustic <int N>` => Sets the approximate number of photons that should be stored in the caustic map. Default is N=60000
  * `-md <int N>` => Sets the max recursion depth of a Photon trace in the photon mapping step. Default is N=128
  * `-it <int N>` => Sets the number of test rays that should be sent when sampling the indirect illumination of a surface. Default is N=256
  * `-gs <int N>` => Sets the number of photons used in a radiance sample of the global photon map. Default is N=50
  * `-gd <float N>` => Sets the max radius of a radiance sample of the global photon map. Default is N=2.5
  * `-cs <int N>` => Sets the number of photons used in a radiance sample of the caustic photon map. Default is N=60
  * `-cd <float N>` => Sets the max radius of a radiance sample of the caustic photon map. Default is N=0.2
* Shadow Sampling flags:
  * `-no_shadow` => Disables shadows entirely. Shadows are enabled by default
  * `-no_ss` => Disables soft shadows. Soft shadows are enabled by default
  * `-lt <int N>` => Sets the number of occlusion + reflectance rays sent per light per sample. Used to compute both soft shadows and direct illumination by area light. Default is N=128
  * `-s <int N>` => Sets the number of occlusion (only) rays sent per light per sample. Used to take additional soft shadow estimates (on top of the number specified by the `-lt` flag). Default is N=128


# Implementation Details

This section contains descriptions and examples of the rendering program's various features. The feature visualizations below (as opposed to renderings) were made with the provided `visualize` program.

## BRDF Sampling & Lighting Cloning

### The BRDF Function
Before modifications, the provided light classes sample reflectance from the Phong BRDF. These implementations were altered to use a physically-based Phong BRDF suggested by Jason Lawrence in ["Importance Sampling of the Phong Reflectance Model"](http://www.cs.princeton.edu/courses/archive/fall18/cos526/papers/importance.pdf).

Note that the (n + 2) / (2*PI) specular term was dropped for 2D lights because it increased noise too sharply.

### Importance Sampling the BRDF
In order to converge more quickly on the correct solution to the rendering equation, it is necessary to importance sample the BRDF when tracing a ray through a specular or diffuse bounce. In other words, rather than sampling all directions and weighting them according to the probability of a bounce heading in each direction, it is better to sample each direction at a frequency proportional to its probability, and then weight all bounces evenly when averaging.

#### Diffuse Importance Sampling
Under our BRDF model, the outgoing direction of a diffuse bounce is independent of the incident angle of the incoming ray (beyond determining the side of the surface off of which to bounce). Rather, its pdf is determined by a normalized cosine-weighted hemisphere along the surface normal. Using the inverse mapping provided by Lawrence, the outgoing ray

##### Figure 1

| Source | Background Decolorization |
|:--------------:|:----------------:|
| ![Source](/gallery/figures/fig_1a.png?raw=true) | ![Decolorization](/gallery/figures/fig_1b.png?raw=true) |


Poisson cloning is the main workhorse of this program and is run without flags:

```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset
```

#### Explanation
This method of cloning solves a sparse linear system of equations of the form `Ax = b` bound by two contraints: (1) the border of the cloned region must match the border of the region before cloning, and (2) the color gradient-field within the pasted cloned-region must match the gradient-field of the source cloned-region as closely as possible.

In pseudocode, the matrices and vectors for this system of equations are constructed as follows:
```
for each pixel i in the mask:
  Np = 0  // "Number of neighbors"
  for each neighbor j of i:
    set Aij to -1 if j in mask;
    add dest[j] to b_i if j in image but not in mask (=> j is a border pixel)
    add 1 to Np if j is in image
    add guidance (src[i] - src[j]) to b_i if j is in image
  set Aii to num neighbors Np
```

#### Results

|  Direct Cloning | Poisson Cloning | 
|:--------------:|:----------------:|
| ![Direct Cloning](/results/battleOfPrinceton_direct.png?raw=true) | ![Poisson Cloning](/results/battleOfPrinceton_poisson.png?raw=true) |
| ![Direct Cloning](/results/fig3a_direct.png?raw=true) | ![Poisson Cloning](/results/fig3a_poisson.png?raw=true) |
| ![Direct Cloning](/results/fig3b_direct.png?raw=true) | ![Poisson Cloning](/results/fig3b_poisson.png?raw=true) |
| ![Direct Cloning](/results/fig4_direct.png?raw=true) | ![Poisson Cloning](/results/fig4_poisson.png?raw=true) |


### Direct Cloning
#### Usage
Direct cloning is the naive (seamed) implementation of cloning and requires a `-d` or `-direct` flag. It takes no further parameters:

```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset -d
```
or
```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset -direct
```


### Monochromatic Poisson Cloning
#### Usage
Monochromatic cloning requires the `-mono` or `-monochrome` flag and takes no further parameters:

```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset -mono
```
or
```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset -monochrome
```

#### Explanation
First monochromatic cloning converts the source image to greyscale through luminance, and it then applies Poisson cloning on using the black and white source image. This is useful when the chromacity of the cloned region needs to remain relatively constant, as the default (polychromatic) Poisson cloning will allow for changes in color within the cloned region that are somewhat independent of the destination's border constraints.

#### Results

| Source | Destination | Polychromatic Poisson Cloning | Monochromatic Poisson Cloning | 
|:--------------:|:----------------:|:----------------:|:----------------:|
| ![Source](/test_images/perez-fig5-src.png?raw=true) | ![Destination](/test_images/perez-fig5-dst.png?raw=true) |![Poisson Cloning](/results/fig5_poisson.png?raw=true) | ![Monochromatic Cloning](/results/fig5_mono.png?raw=true) |


### Mixed Poisson Cloning
#### Usage
Mixed cloning requires the `-mx` or `-mixed` flag and takes no further parameters:

```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset -mx
```
or
```
$ ./poisson_clone src.png mask.png dest.png out.png xOffset yOffset -mixed
```

#### Explanation
Sometimes there is detail in the target region of the destination that needs to be preserved (e.g. a brick wall, or a sharp edge in the background). This is accomplished by adjusting the guidance function between two pixels (i, j) such that if the gradient between pixels i and j are greater (by magnitude) in the destitiation image than in the source, this larger gradient will be used for the system of equations

#### Results

| Source | Destination | Poisson Cloning | Mixed Poisson Cloning | 
|:--------------:|:----------------:|:----------------:|:----------------:|
| ![Source](/test_images/perez-fig6-src.png?raw=true) | ![Destination](/test_images/perez-fig6-dst.png?raw=true) |![Poisson Cloning](/results/fig6_poisson.png?raw=true) | ![Mixed Cloning](/results/fig6_mixed.png?raw=true) |
| ![Source](/test_images/perez-fig7-src.png?raw=true) | ![Destination](/test_images/perez-fig7-dst.png?raw=true) |![Poisson Cloning](/results/fig7_poisson.png?raw=true) | ![Mixed Cloning](/results/fig7_mixed.png?raw=true) |
| ![Source](/test_images/perez-fig8-src.png?raw=true) | ![Destination](/test_images/perez-fig8-dst.png?raw=true) |![Poisson Cloning](/results/fig8_poisson.png?raw=true) | ![Mixed Cloning](/results/fig8_mixed.png?raw=true) |


### Image Flattening
#### Usage
Image flattening requires the `-f` or `-flat` flag along with `threshold` and `factor` values. It is recommended that the source and destination arguments point to the same image, and that the offsets are set to `0`:

```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -f threshold factor
```
or
```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 - flat threshold factor
```

#### Explanation
Image flattening throws away gradients in the source that do not exceed the threshold value in magnitude. As such, only the sharper edges and features of the image are preserved, giving a flattening feel. The preserved gradients are either compressed or heightened depending on whether `factor` is greater than or less than `1` respectively.

#### Results

| Source | Image Flattening |
|:--------------:|:----------------:|
| ![Source](/test_images/perez-fig9-src.png?raw=true) | ![Flat](/results/fig9_flat.png?raw=true) |


### Local Illumination Changes
#### Usage
Local illumination requires the `-il` or `-illumination` flag along with `alpha` and `beta` values. It is recommended that the source and destination arguments point to the same image, and that the offsets are set to `0`:

```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -il alpha beta
```
or
```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -illumination alpha beta
```

#### Explanation
Nonlinear expansion and compression of the source image gradient (applied through alpha and beta) can be used to fix locally underexposed (e.g. shadows) and overexposed (e.g. specular highlights) areas in images. It is recommended that `alpha` is set to a value in the vicinity of `0.2` times the average gradient of the image, and that `beta` be set to a value simply within the neighborhood of `0.2`.

#### Results

| Source | Local Illumination |
|:--------------:|:----------------:|
| ![Source](/test_images/perez-fig10a-src.png?raw=true) | ![Illuminated](/results/fig10a_illum.png?raw=true) |
| ![Source](/test_images/perez-fig10b-src.png?raw=true) | ![Illuminated](/results/fig10b_illum.png?raw=true) |


### Background Decolorization
#### Usage
Background decolorization requires the `-dec` or `-decolor` flag and takes no further parameters. It is recommended that the source and destination arguments point to the same image, and that the offsets are set to `0`:

```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -dec
```
or
```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -decolor
```

#### Explanation
If there is a particularly colorful area within an image (more specifically, that the region has a distinct color from its immediate surroundings), then if the source is Poisson-cloned onto a greyscale version of itself, the colorful region should still retain its color through the cloning process.

#### Results

| Source | Background Decolorization |
|:--------------:|:----------------:|
| ![Source](/test_images/perez-fig11-src.png?raw=true) | ![Decolorization](/results/fig11_dec.png?raw=true) |


### Local Recolorization
#### Usage
Local recolorization requires the `-rec` or `-recolor` flag as well as three additional parameters that coorespond to the scaling of RGB in recolorization. It is recommended that the source and destination arguments point to the same image, and that the offsets are set to `0`:

```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -rec scaleR scaleG scaleB
```
or
```
$ ./poisson_clone src.png mask.png src.png out.png 0 0 -recolor scaleR scaleG scaleB
```

#### Explanation
Recoloring works by scaling RGB values in the source image by the specified amounts before applying Poisson cloning. The bordering region around the object of interest in the mask will return to its original color when the boundary constraints of the cloning process are applied, the but object of interest itself will retain its new color.

#### Results

| Source | Background Local Recolorization |
|:--------------:|:----------------:|
| ![Source](/test_images/perez-fig11-src.png?raw=true) | ![Recolorization](/results/fig11_rec.png?raw=true) |


### Texture Preserving Poisson Cloning
#### Usage
Texture preserving poisson cloning requires the `-tex` or `-texture` flag as well as one addition `threshold` argument:

```
$ ./poisson_clone src.png mask.png src.png out.png xOffset yOffset -tex threshold
```
or
```
$ ./poisson_clone src.png mask.png src.png out.png xOffset yOffset -texture threshold
```

#### Explanation
This is an experimental extension that was not proposed in the original 2003 paper. Although Poisson cloning is excellent at removing seams, differences between cloned textures and the destination textures (e.g. grain) can still leave an apparent seam around the cloning region. In an effort to preserve grain and other small details of the destination, gradients in the destination below the threshold are added onto the source gradient in this mode. Unfortunately, this method is not always successful as discoloration occasionally pops up, and it is not able to remove any grain from the source (so it effectively only adds grain and other nearly imperceptible details).

#### Results

| Poisson Cloning vs. Poisson Cloning with Texture Preservation |
|:--------------:|
| ![Texture Comparison](/results/texture_comparison.png?raw=true) |


## Authors

* **Reilly Bova** - *Cloning Program and Examples* - [ReillyBova](https://github.com/ReillyBova)
* **Szymon Rusinkiewicz** - *Basic C++ and C Image I/O files*

See also the list of [contributors](https://github.com/ReillyBova/poisson/contributors) who participated in this project.

## References
[1] Pérez, Patrick, Michel Gangnet, and Andrew Blake. "Poisson image editing." ACM Transactions on Graphics (TOG). Vol. 22. No. 3. ACM, 2003.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

Thank you to Professor Szymon Rusinkiewicz for teaching and assigning this brilliant technique in the Fall 2018 semester of COS 526: Advanced Computer Graphics
