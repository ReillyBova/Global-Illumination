////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////
#ifndef RAYTRACE_INC
#define RAYTRACE_INC

#include "R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Illumination Sampling Functions (From Rendering Equation)
////////////////////////////////////////////////////////////////////////

// Compute Direct Illumination on point
void DirectIllumination(R3Point& point, R3Vector& normal, const R3Point& eye,
  RNRgb& color, const R3Brdf *brdf, const bool inMonteCarlo);

// Compute transmissive bounce on point
void TransmissiveIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta, RNScalar T_coeff);

// Compute specular bounce on point
void SpecularIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta, RNScalar R_coeff);

// Compute indirect illumination at point
void IndirectIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, const RNScalar cos_theta, const bool inMonteCarlo);

// Compute caustic radiance at point
void CausticIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta);

// Sample the global photon map directly for global illumination estimation
void EstimateGlobalIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta);

////////////////////////////////////////////////////////////////////////
// Main Raytracing Method
////////////////////////////////////////////////////////////////////////

// Sample Ray from eye
void RayTrace(R3SceneElement* element, R3Point& point, R3Vector& normal,
  R3Ray& ray, const R3Point& eye, RNRgb& color);

#endif
