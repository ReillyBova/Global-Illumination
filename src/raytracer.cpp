////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "raytracer.h"
#include "render.h"
#include "montecarlo.h"
#include "utils/graphics_utils.h"
#include "utils/photon_utils.h"
#include "utils/illumination_utils.h"
#include "R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Illumination Sampling Functions (From Rendering Equation)
////////////////////////////////////////////////////////////////////////

// Compute Direct Illumination on point
void DirectIllumination(R3Point& point, R3Vector& normal, const R3Point& eye,
  RNRgb& color, const R3Brdf *brdf, const bool inMonteCarlo)
{
  // Check for single emmissive side of area lights
  bool should_emit = true;

  // Compute illumination from lights
  for (int k = 0; k < SCENE_NLIGHTS; k++) {
    R3Light *light = SCENE->Light(k);

    // Check that point is not on an area or rect light (i.e. on itself)
    int light_intersection = TestLightIntersection(point, eye, light);
    if (light_intersection != 0) {
      if (light_intersection == -1) {
        should_emit = false;
      }
      continue;
    }

    ComputeIllumination(color, light, brdf, eye, point, normal, inMonteCarlo);
  }

  // Account for emission
  if (should_emit) {
    color += brdf->Emission();
  }
}

// Compute transmissive bounce on point
void TransmissiveIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta, RNScalar T_coeff)
{
  // Get direction of bounced ray (might be a specular if total internal reflection)
  const R3Vector exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                                  brdf->IndexOfRefraction());

  // Scale number of samples with contribution to final color of pixel
  RNRgb total_weight = T_coeff * (brdf->Transmission());
  RNScalar highest_weight = MaxChannelVal(total_weight);
  const int num_samples = ceil((TRANSMISSIVE_TEST*highest_weight + TRANSMISSIVE_TEST) / 2.0);

  // Recursively raytrace the bounce using montecarlo path tracing
  RNRgb color_buffer = RNblack_rgb;
  R3Ray ray;
  R3Vector sampled_bounce;
  const RNScalar n = brdf->Shininess();
  for (int i = 0; i < num_samples; i++) {
    if (DISTRIB_TRANSMISSIVE) {
      // Use importance sampling
      sampled_bounce = Specular_ImportanceSample(exact_bounce, n, cos_theta);
    } else {
      sampled_bounce = exact_bounce;
    }
    ray = R3Ray(point + sampled_bounce * RN_EPSILON, sampled_bounce, TRUE);
    MonteCarlo_PathTrace(ray, color_buffer);
    LOCAL_TRANSMISSIVE_RAY_COUNT++;
  }
  // Normalize average and add contribution
  color += (color_buffer / (RNScalar) num_samples) * total_weight;
}

// Compute specular bounce on point
void SpecularIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta, RNScalar R_coeff)
{
  // Get direction of bounced ray
  const R3Vector exact_bounce = ReflectiveBounce(normal, view, cos_theta);

  // Scale number of samples with contribution to final color of pixel
  RNRgb total_weight = brdf->Transmission()*R_coeff + brdf->Specular();
  RNScalar highest_weight = MaxChannelVal(total_weight);
  const int num_samples = ceil((SPECULAR_TEST*highest_weight + SPECULAR_TEST) / 2.0);

  // Recursively raytrace the bounce using montecarlo path tracing
  RNRgb color_buffer = RNblack_rgb;
  R3Ray ray;
  R3Vector sampled_bounce;
  const RNScalar n = brdf->Shininess();
  for (int i = 0; i < num_samples; i++) {
    if (DISTRIB_SPECULAR) {
      // Use importance sampling
      sampled_bounce = Specular_ImportanceSample(exact_bounce, n, cos_theta);
    } else {
      sampled_bounce = exact_bounce;
    }
    ray = R3Ray(point + sampled_bounce * RN_EPSILON, sampled_bounce, TRUE);
    MonteCarlo_PathTrace(ray, color_buffer);
    LOCAL_SPECULAR_RAY_COUNT++;
  }
  // Normalize average and add contribution
  color += (color_buffer / (RNScalar) num_samples) * total_weight;
}

// Compute indirect illumination at point
void IndirectIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, const RNScalar cos_theta, const bool inMonteCarlo)
{
  if (!brdf->IsDiffuse()) return;
  // Scale number of samples with contribution to final color of pixel
  RNRgb total_weight = brdf->Diffuse();
  RNScalar highest_weight = MaxChannelVal(total_weight);
  int num_samples = ceil((INDIRECT_TEST*highest_weight + INDIRECT_TEST) / 2.0);
  if (inMonteCarlo) num_samples = 1;

  // Recursively raytrace the bounce using montecarlo path tracing
  RNRgb color_buffer = RNblack_rgb;
  R3Ray ray;
  R3Vector sampled_bounce;
  for (int i = 0; i < num_samples; i++) {
    // Diffuse importance sample
    sampled_bounce = Diffuse_ImportanceSample(normal, cos_theta);
    ray = R3Ray(point + sampled_bounce * RN_EPSILON, sampled_bounce, TRUE);
    MonteCarlo_IndirectSample(ray, color_buffer);
    LOCAL_INDIRECT_RAY_COUNT++;
  }
  // Normalize average and add contribution
  color += (color_buffer / (RNScalar) num_samples) * total_weight;
}

// Compute caustic radiance at point
void CausticIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta)
{
  if (!brdf->IsDiffuse()) return;
  // Get direction of bounced ray
  const R3Vector exact_bounce = ReflectiveBounce(normal, view, cos_theta);

  // Estimate radiance at point
  EstimateRadiance(point, normal, color, brdf, exact_bounce, cos_theta,
    CAUSTIC_PMAP, CAUSTIC_ESTIMATE_SIZE, CAUSTIC_ESTIMATE_DIST, CAUSTIC_FILTER);
  LOCAL_CAUSTIC_RAY_COUNT++;
}

void EstimateGlobalIllumination(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, R3Vector& view, RNScalar cos_theta)
{
  if (!brdf->IsDiffuse()) return;
  // Get direction of bounced ray
  const R3Vector exact_bounce = ReflectiveBounce(normal, view, cos_theta);

  // Estimate radiance at point
  if (IRRADIANCE_CACHE) {
    EstimateCachedRadiance(point, normal, color, brdf, exact_bounce, cos_theta,
      GLOBAL_PMAP, GLOBAL_ESTIMATE_DIST);
    } else {
    EstimateRadiance(point, normal, color, brdf, exact_bounce, cos_theta,
      GLOBAL_PMAP, GLOBAL_ESTIMATE_SIZE, GLOBAL_ESTIMATE_DIST, GLOBAL_FILTER);
    LOCAL_INDIRECT_RAY_COUNT++;
  }
}

////////////////////////////////////////////////////////////////////////
// Main Raytracing Method
////////////////////////////////////////////////////////////////////////

// Sample Ray from eye
void RayTrace(R3SceneElement* element, R3Point& point, R3Vector& normal,
  R3Ray& ray, const R3Point& eye, RNRgb& color)
{
  // Get intersection information
  const R3Material *material = (element) ? element->Material() : &R3default_material;
  const R3Brdf *brdf = (material) ? material->Brdf() : &R3default_brdf;

  if (AMBIENT) {
    // Global ambient contribution (ambience from scene)
    color += SCENE_AMBIENT;
  }
  if (brdf) {
    // Material dependent contribution

    // Useful geometric values to precompute
    R3Vector view = (point - eye);
    view.Normalize();
    RNScalar cos_theta = normal.Dot(-view);

    // Fresnel Reflection Coefficient for transmission (approximated)
    RNScalar R_coeff = 0;

    if (AMBIENT && brdf->IsAmbient()) {
      // Local ambient contribution (ambience from material)
      color += brdf->Ambient();
    }
    if (DIRECT_ILLUM && (brdf->IsDiffuse() || brdf->IsSpecular())) {
      // Compute contribution from direct illumination
      DirectIllumination(point, normal, eye, color, brdf, false);
    }
    if (TRANSMISSIVE_ILLUM && brdf->IsTransparent()) {
      // Compute Reflection Coefficient, carry reflection portion to Specular
      if (SPECULAR_ILLUM && FRESNEL) {
        R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());
      }
      // Compute contribution from transmission
      if (R_coeff < 1.0) {
        TransmissiveIllumination(point, normal, color, brdf, view, cos_theta,
          1.0 - R_coeff);
      }
    }
    if (SPECULAR_ILLUM && (brdf->IsSpecular() || R_coeff > 0)) {
      // Compute contribution from transmission
      SpecularIllumination(point, normal, color, brdf, view, cos_theta,
        R_coeff);
    }
    if (INDIRECT_ILLUM && (brdf->IsDiffuse())) {
      // Compute contribution from indirect illumination
      IndirectIllumination(point, normal, color, brdf, cos_theta, false);
    }
    if (CAUSTIC_ILLUM && brdf->IsDiffuse()) {
      // Compute contribution from caustic illumination
      CausticIllumination(point, normal, color, brdf, view, cos_theta);
    }
    if (DIRECT_PHOTON_ILLUM && brdf->IsDiffuse()) {
      // Sample the global photon map directly for global illumination estimation
      EstimateGlobalIllumination(point, normal, color, brdf, view, cos_theta);
    }
  }
}
