////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#ifndef ILLUM_INC
#define ILLUM_INC

#include "../R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Occlusion Utils
////////////////////////////////////////////////////////////////////////

// Test for the occlusion of a ray. Return true if there are no other intersects
// in scene between provided points
bool RayIlluminationTest(const R3Point& point_in_scene,
  const R3Point& point_on_light);

  // Test if a point intersects a light, and if so return 1 if its on the emmissive
  // side, -1 if it's on the non emmissive side, 0 otherwise
int TestLightIntersection(R3Point& point, const R3Point& eye, const R3Light *light);

////////////////////////////////////////////////////////////////////////
// 2D Light Soft Shadow & Reflection Utils
////////////////////////////////////////////////////////////////////////

// Add illumination contribution from area light to color
void ComputeAreaLightReflection(R3AreaLight& area_light, RNRgb& color,
  const R3Brdf& brdf, const R3Point& eye, const R3Point& point_in_scene,
  const R3Vector& normal, int num_light_samples, int num_extra_shadow_samples);

// Add illumination contribution from rect light to color
void ComputeRectLightReflection(R3RectLight& rect_light, RNRgb& color,
  const R3Brdf& brdf, const R3Point& eye, const R3Point& point_in_scene,
  const R3Vector& normal, int num_light_samples, int num_extra_shadow_samples);

////////////////////////////////////////////////////////////////////////
// Illumination Utils
////////////////////////////////////////////////////////////////////////

// Compute illumination (and occlusion if applicable) between points on node and
// light and update color
void ComputeIllumination(RNRgb& color, R3Light* light, const R3Brdf *brdf,
  const R3Point& eye, const R3Point& point_in_scene, const R3Vector& normal,
  const bool inMonteCarlo);

#endif
