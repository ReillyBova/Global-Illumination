////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#ifndef GRAPHICS_INC
#define GRAPHICS_INC

#include "../R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Color Utils
////////////////////////////////////////////////////////////////////////

// Clamp color channels to [0, 1] range
void ClampColor(RNRgb &color);

// Normalize Color channels to 1
void NormalizeColor(RNRgb &color);

// Return the max value across all RGB color channels
RNScalar MaxChannelVal(const RNRgb &color);

// Convert color from RGB to Ward's packed RGBE format; copies result into char[4]
// array
void RNRgb_to_RGBE(RNRgb& rgb_src, unsigned char* rgbe_target);

// Convert color from Ward's packed char[4] RGBE format to an RGB class
RNRgb RGBE_to_RNRgb(unsigned char* rgbe_src);

////////////////////////////////////////////////////////////////////////
// Physics & Geometry Utils
////////////////////////////////////////////////////////////////////////

// Return the length of an intersecting ray
RNLength IntersectionDist(const R3Ray& ray, const R3Point& origin);

// Use Schlick's approximation to return reflection coefficient between media
RNScalar ComputeReflectionCoeff(RNScalar cos_theta, const RNScalar ir_mat);

// Return the direction of a perfect reflective bounce
R3Vector ReflectiveBounce(R3Vector normal, R3Vector& view, RNScalar cos_theta);

// Return the direction of a perfect transmissive bounce (may return reflection
// if beyond critical angle)
R3Vector TransmissiveBounce(R3Vector normal, R3Vector& view, RNScalar cos_theta,
  const RNScalar ir_mat);

////////////////////////////////////////////////////////////////////////
// Material Utils
////////////////////////////////////////////////////////////////////////

// Use importance sampling to return a vector sampled from a weighted hemisphere
// around the surface normal (normal is flipped if cos_theta is negative)
R3Vector Diffuse_ImportanceSample(R3Vector normal, const RNScalar cos_theta);

// Use importance sampling to return a vector offset from a perfect bounce by
// a random variable drawn from the brdf
R3Vector Specular_ImportanceSample(const R3Vector& exact, const RNScalar n,
  const RNScalar cos_theta);

////////////////////////////////////////////////////////////////////////
// Light Utils
////////////////////////////////////////////////////////////////////////

RNScalar LightPower(R3Light* light);

#endif
