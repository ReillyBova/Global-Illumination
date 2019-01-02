////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "illumination_utils.h"
#include "graphics_utils.h"
#include "../render.h"
#include "../R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Occlusion Utils
////////////////////////////////////////////////////////////////////////

// Test for the occlusion of a ray. Return true if there are no other intersects
// in scene between provided points
bool RayIlluminationTest(const R3Point& point_in_scene,
  const R3Point& point_on_light)
{
  // Determine distance between point and light
  RNLength unoccluded_len = R3Distance(point_on_light, point_in_scene);

  // Test if there is an intersecting ray that spans this distance
  R3Ray ray = R3Ray(point_on_light, point_in_scene);
  RNLength intersection_len = IntersectionDist(ray, point_on_light);
  LOCAL_SHADOW_RAY_COUNT++;

  if (abs(intersection_len - unoccluded_len) < RN_EPSILON)
    return true;

  return false;
}

// Test if a point intersects a light, and if so return 1 if its on the emmissive
// side, -1 if it's on the non emmissive side, 0 otherwise
int TestLightIntersection(R3Point& point, const R3Point& eye, const R3Light *light)
{
  if (light->ClassID() == R3AreaLight::CLASS_ID()) {
    // Area Light
    R3AreaLight *area_light = (R3AreaLight *) light;

    // Check if point is on circle
    R3Vector v = point - area_light->Position();
    RNLength vLen = v.Length();
    v.Normalize();

    // Circle properties
    R3Vector norm = area_light->Direction();
    RNLength r = area_light->Radius();

    // Test angle and distance from center
    if (abs(v.Dot(norm)) < RN_EPSILON && vLen <= r) {
      // Check if we should still be emmissive
      if (norm.Dot(eye - point) <= 0) {
        return -1;
      }
      return 1;
    }
  } else if (light->ClassID() == R3RectLight::CLASS_ID()) {
    // Area Light
    R3RectLight *rect_light = (R3RectLight *) light;

    // Check if point is on rectangle (or potentially parallelogram)
    R3Vector v = point - rect_light->Position();
    RNLength a1_component = v.Dot(rect_light->PrimaryAxis());
    RNLength a2_component = v.Dot(rect_light->SecondaryAxis());
    v.Normalize();

    // Rectangle properties
    R3Vector norm = rect_light->Direction();
    RNLength l1 = rect_light->PrimaryLength();
    RNLength l2 = rect_light->SecondaryLength();

    // Test angle and distance from center
    if (abs(v.Dot(norm)) < RN_EPSILON && abs(a1_component * 2.0) <= l1 && abs(a2_component * 2.0) <= l2) {
      // Check if we should still be emmissive
      if (norm.Dot(eye - point) <= 0) {
        return -1;
      }
      return 1;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////
// 2D Light Soft Shadow & Reflection Utils
////////////////////////////////////////////////////////////////////////

// Add illumination contribution from area light to color
void ComputeAreaLightReflection(R3AreaLight& area_light, RNRgb& color,
  const R3Brdf& brdf, const R3Point& eye, const R3Point& point_in_scene,
  const R3Vector& normal, int num_light_samples, int num_extra_shadow_samples)
{
  if (!(area_light.IsActive())) return;

  // Get light geometry
  R3Point center = area_light.Position();
  R3Vector light_norm = area_light.Direction();
  RNLength radius = area_light.Radius();
  RNArea area = RN_PI * pow(radius, 2.0);

  // Check normal direction
  if (light_norm.Dot(point_in_scene - center) < 0) {
    return;
  }

  // Find two perpendicular vectors spanning plane
  R3Vector u = R3Vector(light_norm[1], -light_norm[0], 0);
  if (1.0 - abs(light_norm[2]) < 0.1) {
    u = R3Vector(light_norm[2], 0, -light_norm[0]);
  }
  R3Vector v = u % light_norm;
  u.Normalize();
  v.Normalize();

  // Scale u and v
  u *= radius;
  v *= radius;

  // Get color and light sampling properties
  const RNRgb& Dc = brdf.Diffuse();
  const RNRgb& Sc = brdf.Specular();
  const RNScalar n = brdf.Shininess();
  const RNRgb& Ic = area_light.Color();
  const RNScalar constant_attenuation = area_light.ConstantAttenuation();
  const RNScalar linear_attenuation = area_light.LinearAttenuation();
  const RNScalar quadratic_attenuation = area_light.QuadraticAttenuation();
  const RNScalar intensity = area_light.Intensity();

  // Genereal Forward Declarations
  R3Point sample_point;
  RNScalar r1, r2;
  RNScalar I;
  RNLength d;
  RNScalar denom;
  R3Vector L;

  // Weighting parameters
  RNScalar weight;
  int hits;
  int total_num_samples = 0;
  int total_num_hits = 0;

  // Diffuse Sampling
  if (brdf.IsDiffuse()) {
    weight = 0;
    hits = 0;
    for (int i = 0; i < num_light_samples; i++) {
      do {
        // Sample point in circle
        r1 = RNThreadableRandomScalar();
        r2 = RNThreadableRandomScalar();
      } while (r1*r1 + r2*r2 > 1.0);

      // Use values r1, r2 and vectors u, v to find a random point on light
      sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;
      if (RayIlluminationTest(point_in_scene, sample_point)) {
        hits++;
        // Compute intensity at point
        I = intensity;
        d = R3Distance(point_in_scene, sample_point);
        denom = constant_attenuation;
        denom += d * linear_attenuation;
        denom += d * d * quadratic_attenuation;
        if (RNIsPositive(denom)) I /= denom;

        // Compute direction at point
        L = sample_point - point_in_scene;
        L.Normalize();

        // Weight intensity by probability of area light emission direction
        I *= light_norm.Dot(-L) * 2.0;

        // Compute diffuse reflection from sample point
        weight += I * abs(normal.Dot(L));
      }
    }
    // Add diffuse contribution
    if (hits > 0) {
      color += weight * Dc * Ic * area / hits / RN_PI;
    }
    total_num_hits += hits;
    total_num_samples += num_light_samples;
  }

  // Specular Sampling
  if (brdf.IsSpecular()) {
    weight = 0;
    hits = 0;
    num_light_samples *= 2;
    R3Vector R;
    R3Vector V = eye - point_in_scene;
    V.Normalize();
    RNScalar VR;
    RNScalar NL;
    for (int i = 0; i < num_light_samples; i++) {
      do {
        // Sample point in circle
        r1 = RNThreadableRandomScalar();
        r2 = RNThreadableRandomScalar();
      } while (r1*r1 + r2*r2 > 1.0);

      // Use values r1, r2 and vectors u, v to find a random point on light
      sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;
      if (RayIlluminationTest(point_in_scene, sample_point)) {
        hits++;
        // Compute intensity at point
        I = intensity;
        d = R3Distance(point_in_scene, sample_point);
        denom = constant_attenuation;
        denom += d * linear_attenuation;
        denom += d * d * quadratic_attenuation;
        if (RNIsPositive(denom)) I /= denom;

        // Compute direction at point
        L = sample_point - point_in_scene;
        L.Normalize();

        // Weight intensity by probability of area light emission direction
        I *= light_norm.Dot(-L) * 2.0;

        // Compute specular reflection from sample point
        NL = normal.Dot(L);
        R = (2.0 * NL) * normal - L;
        VR = V.Dot(R);
        if (RNIsNegativeOrZero(VR)) continue;

        // Add to result
        weight += (I * pow(VR,n));
      }
    }
    // Add specular contribution
    if (hits > 0) {
      // NB Adding the (n+2)/2pi term heavily increases variance so it is omitted
      color += weight * Sc * Ic * area / hits;
    }
    total_num_hits += hits;
    total_num_samples += num_light_samples;
  }

  // Additional shadow sampling if necessary
  hits = 0;
  for (int i = 0; i < num_extra_shadow_samples; i++) {
    do {
      // Sample point in circle
      r1 = RNThreadableRandomScalar();
      r2 = RNThreadableRandomScalar();
    } while (r1*r1 + r2*r2 > 1.0);

    // Use values r1, r2 and vectors u, v to find a random point on light
    sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;
    if (RayIlluminationTest(point_in_scene, sample_point))
      hits++;
  }
  total_num_hits += hits;
  total_num_samples += num_extra_shadow_samples;

  // Reweight according to shadow hit rate
  if (total_num_samples > 0)
    color *= ((double) total_num_hits) / total_num_samples;
}

// Add illumination contribution from rect light to color
void ComputeRectLightReflection(R3RectLight& rect_light, RNRgb& color,
  const R3Brdf& brdf, const R3Point& eye, const R3Point& point_in_scene,
  const R3Vector& normal, int num_light_samples, int num_extra_shadow_samples)
{
  if (!(rect_light.IsActive())) return;

  // Get light geometry
  R3Point center = rect_light.Position();
  R3Vector light_norm = rect_light.Direction();

  // Check normal direction
  if (light_norm.Dot(point_in_scene - center) < 0) {
    return;
  }

  // Get scaled axes
  R3Vector a1 = rect_light.PrimaryAxis() * rect_light.PrimaryLength();
  R3Vector a2 = rect_light.SecondaryAxis() * rect_light.SecondaryLength();
  RNArea area = (a1 % a2).Length();

  // Get color and light sampling properties
  const RNRgb& Dc = brdf.Diffuse();
  const RNRgb& Sc = brdf.Specular();
  const RNScalar n = brdf.Shininess();
  const RNRgb& Ic = rect_light.Color();
  const RNScalar constant_attenuation = rect_light.ConstantAttenuation();
  const RNScalar linear_attenuation = rect_light.LinearAttenuation();
  const RNScalar quadratic_attenuation = rect_light.QuadraticAttenuation();
  const RNScalar intensity = rect_light.Intensity();

  // Genereal Forward Declarations
  R3Point sample_point;
  RNScalar r1, r2;
  RNScalar I;
  RNLength d;
  RNScalar denom;
  R3Vector L;
  RNScalar NL;

  // Weighting parameters
  RNScalar weight;
  int hits;
  int total_num_samples = 0;
  int total_num_hits = 0;

  // Diffuse Sampling
  if (brdf.IsDiffuse()) {
    weight = 0;
    hits = 0;
    for (int i = 0; i < num_light_samples; i++) {
      r1 = RNThreadableRandomScalar() - 0.5;
      r2 = RNThreadableRandomScalar() - 0.5;

      // Use values r1, r2 and vectors a1, a2 to find a random point on light
      sample_point = r1*a1 + r2*a2 + center + light_norm*RN_EPSILON;
      if (RayIlluminationTest(point_in_scene, sample_point)) {
        hits++;
        // Compute intensity at point
        I = intensity;
        d = R3Distance(point_in_scene, sample_point);
        denom = constant_attenuation;
        denom += d * linear_attenuation;
        denom += d * d * quadratic_attenuation;
        if (RNIsPositive(denom)) I /= denom;

        // Compute direction at point
        L = sample_point - point_in_scene;
        L.Normalize();

        // Weight intensity by probability of area light emission direction
        I *= light_norm.Dot(-L) * 2.0;

        // Compute diffuse reflection from sample point
        weight += I * abs(normal.Dot(L));
      }
    }
    // Add diffuse contribution
    if (hits > 0) {
      color += weight * Dc * Ic * area / hits / RN_PI;
    }
    total_num_hits += hits;
    total_num_samples += num_light_samples;
  }

  // Specular Sampling
  if (brdf.IsSpecular()) {
    weight = 0;
    hits = 0;
    num_light_samples *= 2;
    R3Vector R;
    R3Vector V = eye - point_in_scene;
    V.Normalize();
    RNScalar VR;
    for (int i = 0; i < num_light_samples; i++) {
      r1 = RNThreadableRandomScalar() - 0.5;
      r2 = RNThreadableRandomScalar() - 0.5;

      // Use values r1, r2 and vectors a1, a2 to find a random point on light
      sample_point = r1*a1 + r2*a2 + center + light_norm*RN_EPSILON;
      if (RayIlluminationTest(point_in_scene, sample_point)) {
        hits++;
        // Compute intensity at point
        I = intensity;
        d = R3Distance(point_in_scene, sample_point);
        denom = constant_attenuation;
        denom += d * linear_attenuation;
        denom += d * d * quadratic_attenuation;
        if (RNIsPositive(denom)) I /= denom;

        // Compute direction at point
        L = sample_point - point_in_scene;
        L.Normalize();

        // Weight intensity by probability of area light emission direction
        I *= light_norm.Dot(-L) * 2;

        // Compute specular reflection from sample point
        NL = normal.Dot(L);
        R = (2.0 * NL) * normal - L;
        VR = V.Dot(R);
        if (RNIsNegativeOrZero(VR)) continue;

        // Add to result
        weight += (I * pow(VR,n));
      }
    }
    // Add specular contribution
    if (hits > 0) {
      // NB Adding the (n+2)/2pi term heavily increases variance so it is omitted
      color += weight * Sc * Ic * area / hits;
    }
    total_num_hits += hits;
    total_num_samples += num_light_samples;
  }

  // Additional shadow sampling if necessary
  hits = 0;
  for (int i = 0; i < num_extra_shadow_samples; i++) {
    r1 = RNThreadableRandomScalar() - 0.5;
    r2 = RNThreadableRandomScalar() - 0.5;

    // Use values r1, r2 and vectors a1, a2 to find a random point on light
    sample_point = r1*a1 + r2*a2 + center + light_norm*RN_EPSILON;
    if (RayIlluminationTest(point_in_scene, sample_point)) {
      hits++;
    }
  }
  total_num_hits += hits;
  total_num_samples += num_extra_shadow_samples;

  // Reweight according to shadow hit rate
  if (total_num_samples > 0)
    color *= ((double) total_num_hits) / total_num_samples;
}

////////////////////////////////////////////////////////////////////////
// Illumination Utils
////////////////////////////////////////////////////////////////////////

// Compute illumination (and occlusion if applicable) between points on node and
// light and update color
void ComputeIllumination(RNRgb& color, R3Light* light, const R3Brdf *brdf,
  const R3Point& eye, const R3Point& point_in_scene, const R3Vector& normal,
  const bool inMonteCarlo)
{
  // Determine which boolean we should use
  bool compute_shadows = SHADOWS && (!inMonteCarlo || (RECURSIVE_SHADOWS && inMonteCarlo));

  // Only sample 1 reflection if in a monte carlo trace
  int num_light_samples = LIGHT_TEST;
  int num_extra_shadow_samples = SHADOW_TEST;
  if (inMonteCarlo) {
    num_light_samples = 2;
    num_extra_shadow_samples = 0;
  }

  // Skip extra work if possible
  if (!compute_shadows) {
    color += light->Reflection(*brdf, eye, point_in_scene, normal, num_light_samples);
    return;
  }

  // Shadow + Reflectance computation
  R3Point point_on_light;
  if (light->ClassID() == R3DirectionalLight::CLASS_ID()) {
    // Directional Light
    R3DirectionalLight *directional_light = (R3DirectionalLight *) light;
    point_on_light = point_in_scene - directional_light->Direction() * SCENE_RADIUS * 3.0;
  } else if (light->ClassID() == R3PointLight::CLASS_ID()) {
    // Point Light
    R3PointLight *point_light = (R3PointLight *) light;
    point_on_light = point_light->Position();
  } else if (light->ClassID() == R3SpotLight::CLASS_ID()) {
    // Spot Light
    R3SpotLight *spot_light = (R3SpotLight *) light;
    point_on_light = spot_light->Position();
  } else if (light->ClassID() == R3AreaLight::CLASS_ID()) {
    // Area Light
    R3AreaLight *area_light = (R3AreaLight *) light;
    if (!SOFT_SHADOWS)
      point_on_light = area_light->Position() + RN_EPSILON * area_light->Direction();
    else {
      // Apply soft shadows and distributed illumination
      ComputeAreaLightReflection(*area_light, color, *brdf, eye, point_in_scene, normal,
        num_light_samples, num_extra_shadow_samples);
      return;
    }
  } else if (light->ClassID() == R3RectLight::CLASS_ID()) {
    // Rect Light
    R3RectLight *rect_light = (R3RectLight *) light;
    if (!SOFT_SHADOWS)
      point_on_light = rect_light->Position() + RN_EPSILON * rect_light->Direction();
    else {
      // Apply soft shadows and distributed illumination
      ComputeRectLightReflection(*rect_light, color, *brdf, eye, point_in_scene, normal,
        num_light_samples, num_extra_shadow_samples);
      return;
    }
  } else {
    fprintf(stderr, "Unrecognized light type: %d\n", light->ClassID());
    return;
  }

  // No soft shadows
  if (RayIlluminationTest(point_in_scene, point_on_light))
    color += light->Reflection(*brdf, eye, point_in_scene, normal, num_light_samples);
}
