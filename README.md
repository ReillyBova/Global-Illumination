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
  * `-resolution <int X> <int Y>` => Sets output image dimensions to X by Y. Default is `X=1024 Y=1024`
  * `-v` => Enables verbose output, which prints rendering statistics to the screen. Off by default
  * `-threads <int N>` => Sets the number of threads (including main thread) used to trace photons and render the image. Default is `N=1`
  * `-aa <int N>` => Sets how many times the dimensions of the image should be doubled before downsampling (as a form of anti-aliasing) to the output image. To be more precise, there `4^N` rays sampled over an evenly-weighted grid per output pixel. Default is `N=2`
  * `-real` => Normalize the components of all materials in the scene such that they conserve energy. Off by default
  * `-no_fresnel` => Disables splitting transmissision into specular and refractive components based on angle of incident ray. Fresnel is enabled by default
  * `-ir <float N>` => Sets the refractive index of air. Default is `N=1.0`
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
  * `-md <int N>` => Sets the max recursion depth of a Monte Carlo path-trace. Default is `N=128`
  * `-absorb <float N>` => Sets probability of a surface absorbing a photon. Default is `N=0.005` (0.5%)
  * `-no_rs` => Disables recursive shadows (shadow sampling from within a specular or transmissive raytrace). Recursive shadows are enabled by default
  * `-no_dt` => Disables distributed importance sampling of transmissive rays based on material shininess. For materials with low shininess but high transmision values, this creates a "frosted glass" effect. Distributed transmissive ray sampling is enabled by default
  * `-tt <int N>` => Sets the number of test rays that should be sent when sampling a transmissive surface. Default is `N=128`
  * `-no_ds` => Disables distributed importance sampling of specular rays based on material shininess. For materials with low shininess but high specular values, this creates a "glossy surface" effect. Distributed specualar ray sampling is enabled by default
* Photon Mapping flags:
  * `-global <int N>` => Sets the approximate number of photons that should be stored in the global map. Default is `N=1920`
  * `-caustic <int N>` => Sets the approximate number of photons that should be stored in the caustic map. Default is `N=60000`
  * `-md <int N>` => Sets the max recursion depth of a Photon trace in the photon mapping step. Default is `N=128`
  * `-it <int N>` => Sets the number of test rays that should be sent when sampling the indirect illumination of a surface. Default is `N=256`
  * `-gs <int N>` => Sets the number of photons used in a radiance sample of the global photon map. Default is `N=50`
  * `-gd <float N>` => Sets the max radius of a radiance sample of the global photon map. Default is `N=2.5`
  * `-cs <int N>` => Sets the number of photons used in a radiance sample of the caustic photon map. Default is `N=60`
  * `-cd <float N>` => Sets the max radius of a radiance sample of the caustic photon map. Default is `N=0.2`
* Shadow Sampling flags:
  * `-no_shadow` => Disables shadows entirely. Shadows are enabled by default
  * `-no_ss` => Disables soft shadows. Soft shadows are enabled by default
  * `-lt <int N>` => Sets the number of occlusion + reflectance rays sent per light per sample. Used to compute both soft shadows and direct illumination by area light. Default is `N=128`
  * `-s <int N>` => Sets the number of occlusion (only) rays sent per light per sample. Used to take additional soft shadow estimates (on top of the number specified by the `-lt` flag). Default is `N=128`

## Program Input
### Provided Scenes
Sample scenes for testing out the program can be found in the `input/` folder. To ease commandline headaches, there are also provided `make` rules for almost all of these scenes in the Makefile that lives in the project's root directory.

### Adding a Custom Scene
It is easy to provide the rendering program with a scene of your own design. Currently, this program is only able to read a scenes in a very simple (custom) format. Documentation on the syntax of this format may be found [here](https://www.cs.princeton.edu/courses/archive/fall18/cos526/a3/scnformat.html).

Note that textures are not yet implemented in the program.

# Implementation Details
This section contains descriptions and examples of the rendering program's various features. The feature visualizations below (as opposed to renderings) were made with the provided `visualize` program.

## BRDF Sampling & Lighting
### The BRDF Function
Before modifications, the provided light classes sample reflectance from the Phong BRDF. These implementations were altered to use a physically-based Phong BRDF suggested by Jason Lawrence in ["Importance Sampling of the Phong Reflectance Model"](http://www.cs.princeton.edu/courses/archive/fall18/cos526/papers/importance.pdf).

Note that the `(n + 2) / (2π)` specular term was dropped for 2D lights because it increased noise too sharply.

### Importance Sampling the BRDF
In order to converge more quickly on the correct solution to the rendering equation, it is necessary to importance sample the BRDF when tracing a ray through a specular or diffuse bounce. In other words, rather than sampling all directions and weighting them according to the probability of a bounce heading in each direction, it is better to sample each direction at a frequency proportional to its probability, and then weight all bounces evenly when averaging.

#### Diffuse Importance Sampling
Under our BRDF model, the outgoing direction of a diffuse bounce is independent of the incident angle of the incoming ray (beyond determining the side of the surface off of which to bounce). Rather, its pdf is determined by a normalized cosine-weighted hemisphere along the surface normal. Borrowing the inverse mapping provided by Lawrence, the direction of the outgoing ray is sampled in spherical coordinates as `(θ, φ) = (arccos(sqrt(u)), 2πv)`, where `(u, v)` are uniformly-distributed random variables in the range `[0, 1)`, `θ` is the angle between the outgoing ray and the surface normal, and `φ` is the angle around the plane perpendicular to the surface normal.

##### Figure 1: Diffuse importance sampling of 500 rays at three angles and two viewpoints.

|| Viewpoint A | Viewpoint B |
|:----------------:|:----------------:|:----------------:|
| 90° | ![Fig 1a.i](/gallery/figures/fig_1a-i.png?raw=true) | ![Fig 1a.ii](/gallery/figures/fig_1a-ii.png?raw=true) |
| 45° | ![Fig 1b.i](/gallery/figures/fig_1b-i.png?raw=true) | ![Fig 1b.ii](/gallery/figures/fig_1b-ii.png?raw=true) |
| 5° | ![Fig 1c.i](/gallery/figures/fig_1c-i.png?raw=true) | ![Fig 1c.ii](/gallery/figures/fig_1c-ii.png?raw=true) |

#### Specular Importance Sampling
For specular importance sampling, the outgoing direction is sampled as a perturbance from the direction of perfect reflection of the incident ray. Again referencing Lawrence's note, we initially sample this direction as `(α, φ) = (arccos(pow(u,1/(n+1))), 2πv)`, where `(u, v)` are uniformly-distributed random variables in the range `[0, 1)`, `α` is the angle between the outgoing ray and the direction of perfect reflection, and `φ` is the angle around the plane perpendicular to the direction of perfect reflection. Finally, although this is not mentioned in the notes, in order to ensure the sampled outgoing ray is on the same side of the surface as the incoming ray, it is necessary to scale alpha from the range `[0, pi/2)` to `[0, θ)`, where `θ` is the angle between the direction of perfect reflection and the plane of the surface. Note that this rescaling is not a perfectly accurate model and somewhat inconsistent with our BRDF (rejection sampling would be a more accurate approach, but significantly more inefficient and not worth the cost), but it is still has a physical basis since glossy reflections become significantly sharper at increasingly grazing angles.

##### Figure 2: Specular importance sampling of 500 rays at three angles and two viewpoints for two materials with shininess of n = 100 and n = 1000 respectively.

|| Viewpoint A, n = 100| Viewpoint B, n = 100 | Viewpoint A, n = 1000 | Viewpoint B, n = 1000 |
|:----------------:|:----------------:|:----------------:|:----------------:|:----------------:|
| 90° | ![Fig 2a.i](/gallery/figures/fig_2a-i.png?raw=true) | ![Fig 2a.ii](/gallery/figures/fig_2a-ii.png?raw=true) | ![Fig 2a.iii](/gallery/figures/fig_2a-iii.png?raw=true) | ![Fig 2a.iv](/gallery/figures/fig_2a-iv.png?raw=true) |
| 45° | ![Fig 2b.i](/gallery/figures/fig_2b-i.png?raw=true) | ![Fig 2b.ii](/gallery/figures/fig_2b-ii.png?raw=true) | ![Fig 2b.iii](/gallery/figures/fig_2b-iii.png?raw=true) | ![Fig 2b.iv](/gallery/figures/fig_2b-iv.png?raw=true) |
| 5° | ![Fig 2c.i](/gallery/figures/fig_2c-i.png?raw=true) | ![Fig 2c.ii](/gallery/figures/fig_2c-ii.png?raw=true) | ![Fig 2c.iii](/gallery/figures/fig_2c-iii.png?raw=true) | ![Fig 2c.iv](/gallery/figures/fig_2c-iv.png?raw=true) |

### 2D Lights
The only physically-plausible light in the R3Graphics codebase is a circular area light. Because all example scenes in Jensen's Photon Mapping paper use a rectangular area light, the we added an R3RectLight class. Additionally, the original R3AreaLight class required modifications to its reflectance function implementation in order to more accurately reflect how light is emitted through diffuse surfaces.

#### R3RectLight
The syntax of the a rectanglar light object is as follows:
```
rect_light r g b    px py pz    a1x a1y a1z    a2x a2y a2z    len1 len2    ca la qa
```
This defines a rectangular light with radiance `r g b` (in Watts/sr/m^2) centered at `px py pz`. The surface of the light is defined by axes `a1` and `a2`, with lengths `len1` and `len2` respectively. Note that if `a1` and `a2` are not perpendicular, the light will be a parallelogram. Light is only emitted in the `a1 x a2` direction, and `ca la qa` define the light's attenuation constants.

##### Figure 3: A comparison of area lights. In Figure (3a) we see a Cornell Box illuminated by a circular area light. In Figure (3b) we see a Cornell Box illuminated by a rectangular area light.

| Circular Area Light | Rectangular Area Light |
|:----------------:|:----------------:|
| ![Fig 3a](/gallery/figures/fig_3a.png?raw=true) | ![Fig 3b](/gallery/figures/fig_3b.png?raw=true) |

#### Weighted Area Light Reflectance
Since area lights emit light diffusely — that is, according to the distribution of a cosine-weighted hemisphere — it was necessary to modify the area light reflectance implementation provided in R3AreaLight. When computing the illumination of a surface due to an area light (circular or rectangular), the intensity of the illumination doubled and then scaled by the cosine of the angle between the light normal and the vector spanning from the light to the surface. The doubling is necessary since we want to keep the power of the light consistent with the original implementation (the flux through an evenly-weighted hemisphere is 2π, whereas the flux through a cosine-weighted hemisphere is π).

##### Figure 4: A comparison of area light falloff. In Figure (4a) we see a Cornell Box illuminated by an area light that emits light evenly in all directions (the provided implemention). In Figure (4b) we see a Cornell Box illuminated by an area light that emits light according to a cosine-weighted hemisphere. Notice that this improvement alone already makes the box appear far more realistic and natural.

| No Light Falloff | Cosine Light Falloff |
|:----------------:|:----------------:|
| ![Fig 4a](/gallery/figures/fig_4a.png?raw=true) | ![Fig 4b](/gallery/figures/fig_4b.png?raw=true) |

## Direct Illumination
### Shadows
When sampling the illumination of a surface by a particular light, it is necessary to test if there are any objects in the scene that occlude the surface from that light (thereby casting a shadow). Details on how this is computed for each light follow.

#### Point Light & Spotlight Shadows
For point lights and spotlights, a ray is cast from the light's position to the surface sample. Then the first intersection of the ray with an object in the scene is computed, and if the distance between this intersection point and the light does not match up with the distance between the light and the surface sample, the surface sample is taken to be in shadow.

##### Figure 5: A demonstration of how point lights illuminate surfaces and cast shadows. In (5a) a sphere is illuminated on a box by a bright blue point light outside of the camera's view. In (5b) a sphere is illuminated by three brightly-colored point lights outside of the camera's view. Notice how the colors of the lights mix to form new colors.

| Figure 5a | Figure 5b |
|:----------------:|:----------------:|
| ![Fig 5a](/gallery/figures/fig_5a.png?raw=true) | ![Fig 5b](/gallery/figures/fig_5b.png?raw=true) |

##### Figure 6: A demonstration of how spotlights illuminate surfaces and cast shadows. In (6a) a sphere is illuminated on a box by a bright white spotlight (similar to a studio light) outside of the camera's view. In (6b) a sphere is illuminated by three brightly-colored spotlights outside of the camera's view that are focused on the sphere. Notice how only very faint shadows are cast in the scene because the cutoff angles of the spotlights are set such that the sphere "eclipses" the cones of light.

| Figure 6a | Figure 6b |
|:----------------:|:----------------:|
| ![Fig 6a](/gallery/figures/fig_6a.png?raw=true) | ![Fig 6b](/gallery/figures/fig_6b.png?raw=true) |

#### Directional Light Shadows
For directional lights, it is first necessary to find a point far outside the scene such that the vector from this point to the surface sample is colinear with the direction of the the directional light. Then a ray is cast from this point to the surface sample. The rest of the shadowing computation continues as before.

##### Figure 7: A demonstration of how directional lights illuminate surfaces and cast shadows. In (7a) a sphere is illuminated on a box by a bright blue directional light. In (7b) a sphere is illuminated by three brightly-colored directional lights. Notice the coloring of the shadows is subtractive (whereas the coloring on the ball is additive) because of the sharpness of directional light (i.e. there is no bleeding).

| Figure 7a | Figure 7b |
|:----------------:|:----------------:|
| ![Fig 7a](/gallery/figures/fig_7a.png?raw=true) | ![Fig 7b](/gallery/figures/fig_7b.png?raw=true) |

### Soft Shadows
For area lights, it is possible for a surface to be partially occluded from a light, causing a penumbra. In order to compute soft shadows, many random points on the surface of the light are sampled, and then a shadow ray (i.e. occlusion test) is sent from each of them to the potentially-occluded surface and the results are averaged to estimate the degree to which the object sits in shadow relative to the area light source.

#### An Illumination Optimization
Note that if the ray can make it all the way to the surface, then as an optimization, the surface illumination due to the ray is also sampled. For the R3AreaLight, this requires rewriting the provided reflection code to sit within the soft shadow loop. Note that this optimization is, remarkably, also a more accurate model since the illumination of a surface is now only sampled from unoccluded portions of area lights.

##### Figure 8: A comparison of soft shadow quality based the number of shadow rays sent per sample.

| No Soft Shadows | 1 Sample | 4 Samples | 16 Samples | 64 Samples | 256 Samples |
|:---:|:---:|:---:|:---:|:---:|:---:|
| ![Fig 8a](/gallery/figures/fig_8a.png?raw=true) | ![Fig 8b](/gallery/figures/fig_8b.png?raw=true) | ![Fig 8c](/gallery/figures/fig_8c.png?raw=true) | ![Fig 8d](/gallery/figures/fig_8d.png?raw=true) | ![Fig 8e](/gallery/figures/fig_8e.png?raw=true) | ![Fig 8f](/gallery/figures/fig_8f.png?raw=true) |

##### Figure 9: A comparison of hard shadowing to soft shadowing in two different scenes. Note that certain features that have not yet been discussed are disabled, hence the black spheres in (9b). For both scenes, which measure 512x512
| Hard Shadows | Soft Shadows |
|:----------------:|:----------------:|
| ![Fig 9a-a](/gallery/figures/fig_9a-i.png?raw=true) | ![Fig 9a-ii](/gallery/figures/fig_9a-ii.png?raw=true) |
| ![Fig 9b-i](/gallery/figures/fig_9b-i.png?raw=true) | ![Fig 9b-ii](/gallery/figures/fig_9b-ii.png?raw=true) |

## Transmission, Reflection, & Monte Carlo Path-Tracing
###
# Credits
## Authors
* **Reilly Bova** - *Rendering Program and Examples* - [ReillyBova](https://github.com/ReillyBova)
* **Tom Funkhouser** - *Basic C++ and C Image I/O files*

See also the list of [contributors](https://github.com/ReillyBova/poisson/contributors) who participated in this project.

## References
* [Henrik Wann Jensen, Frank Suykens, and Per Christensen. "A Practical Guide to Global Illumination using Photon Mapping" SIGGRAPH'2001 Course 38, Los Angeles, August 2001.](http://www.cs.princeton.edu/courses/archive/fall18/cos526/papers/jensen01.pdf)
* [Jason Lawrence. "Importance Sampling of the Phong Reflectance Model" COS 526 Course Notes.](http://www.cs.princeton.edu/courses/archive/fall18/cos526/papers/importance.pdf)

## License
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments
Thank you to Professor Szymon Rusinkiewicz adn the rest of the Computer Graphics faculty at Princeton University for teaching me all the techniques I needed to champion this assignment in the Fall 2018 semester of COS 526: Advanced Computer Graphics.
