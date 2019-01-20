////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "montecarlo.h"
#include "render.h"
#include "raytracer.h"
#include "utils/graphics_utils.h"
#include "utils/photon_utils.h"
#include "R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Monte Carlo Path-Tracing Method
////////////////////////////////////////////////////////////////////////

void MonteCarlo_PathTrace(R3Ray& ray, RNRgb& color)
{
  // Corner case
  if (!MONTE_CARLO) {
    return;
  }

  // Intersection variables and forward declarations
  RNRgb total_weight = RNwhite_rgb;
  R3SceneElement *element;
  R3Point point;
  R3Vector normal;
  const R3Material *material;
  const R3Brdf *brdf;
  R3Point ray_start = ray.Start();
  R3Vector view;
  RNScalar cos_theta;
  RNScalar R_coeff;
  RNRgb color_buffer;

  // Probability values
  RNScalar prob_diffuse;
  RNScalar prob_transmission;
  RNScalar prob_specular;
  RNScalar prob_terminate;
  RNScalar prob_total;
  RNScalar rand;

  // Bouncing geometries
  R3Vector exact_bounce;
  R3Vector sampled_bounce;

  for (int iter = 0; iter < MAX_MONTE_DEPTH; iter++)
  {
    if (SCENE->Intersects(ray, NULL, &element, NULL, &point, &normal, NULL)) {
      // Book keeping
      LOCAL_MONTE_RAY_COUNT++;

      // Get intersection information
      material = (element) ? element->Material() : &R3default_material;
      brdf = (material) ? material->Brdf() : &R3default_brdf;

      // Color buffer (compute and scale once)
      color_buffer = RNblack_rgb;

      // Compute color
      if (AMBIENT) {
        // Global contribution (ambience from scene)
        color_buffer += SCENE_AMBIENT;
      }
      if (brdf) {
        // Useful geometric values to precompute
        view = (point - ray_start);
        view.Normalize();
        cos_theta = normal.Dot(-view);

        // Immediate sampling (always compute) -----------------------------------
        if (brdf->IsDiffuse() || brdf->IsSpecular()) {
          // Compute contribution from direct illumination
          DirectIllumination(point, normal, ray_start, color_buffer, brdf, cos_theta, true);
        }
        if (CAUSTIC_ILLUM && (brdf->IsDiffuse())) {
          // Compute contribution from direct illumination
          CausticIllumination(point, normal, color_buffer, brdf, view, cos_theta);
        }

        // Store results of sampling
        color += color_buffer * total_weight;

        // Bounced sampling (monte carlo) ----------------------------------------

        // Compute Reflection Coefficient, carry reflection portion to Specular
        R_coeff = 0;
        if (SPECULAR_ILLUM && TRANSMISSIVE_ILLUM && FRESNEL && brdf->IsTransparent()) {
          R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());
        }

        // Generate material probabilities of bounce type
        prob_diffuse = MaxChannelVal(brdf->Diffuse());
        prob_transmission = MaxChannelVal(brdf->Transmission());
        prob_specular = MaxChannelVal(brdf->Specular()) + R_coeff*prob_transmission;
        prob_transmission *= (1.0 - R_coeff);
        prob_terminate = MaxChannelVal(brdf->Emission()) + PROB_ABSORB;
        prob_total = prob_diffuse + prob_transmission + prob_specular + prob_terminate;

        // Scale down to 1.0 (but never scale up bc of implicit absorption)
        // NB: faster to scale rand up than to normalize; would also need to adjust
        //     brdf values when updating weights (dividing prob_total back out)
        rand = RNThreadableRandomScalar();
        if (prob_total > 1.0) {
          rand *= prob_total;
        }

        // Bounce and recur (choose one from distribution)
        if (rand < prob_diffuse) {
          if (INDIRECT_ILLUM) {
            // Sample photon map with diffuse bounce
            color_buffer = RNblack_rgb;
            IndirectIllumination(point, normal, color_buffer, brdf, cos_theta, true);
            color += color_buffer * brdf->Diffuse() * total_weight / prob_diffuse;
          } else if (FAST_GLOBAL) {
            color_buffer = RNblack_rgb;
            EstimateGlobalIllumination(point, normal, color_buffer, brdf, view, cos_theta);
            color += color_buffer * brdf->Diffuse() * total_weight / prob_diffuse;
          }
          break;
        } else if (rand < prob_diffuse + prob_transmission) {
          if (!TRANSMISSIVE_ILLUM)
            break;
          // Compute direction of transmissive bounce
          exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                            brdf->IndexOfRefraction());
          // Compute direction of next ray
          if (DISTRIB_TRANSMISSIVE) {
            // Use importance sampling
            sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
          } else {
            sampled_bounce = exact_bounce;
          }

          LOCAL_TRANSMISSIVE_RAY_COUNT++;
          // Update weights
          total_weight *= (1.0 - R_coeff) * brdf->Transmission() / prob_transmission;
        } else if (rand < prob_diffuse + prob_transmission + prob_specular) {
          if (!SPECULAR_ILLUM)
            break;

          // Compute direction of specular bounce
          exact_bounce = ReflectiveBounce(normal, view, cos_theta);
          // Compute direction of next ray
          if (DISTRIB_SPECULAR) {
            // Use importance sampling
            sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
          } else {
            sampled_bounce = exact_bounce;
          }

          LOCAL_SPECULAR_RAY_COUNT++;
          // Update weights
          total_weight *= (brdf->Specular() +  R_coeff*brdf->Transmission()) / prob_specular;
        } else {
          // Photon absorbed; terminate trace
          break;
        }

        // Recur
        ray_start = point + sampled_bounce * RN_EPSILON;
        ray = R3Ray(ray_start, sampled_bounce, TRUE);
      }
    } else {
      // Intersect with background and break
      color += total_weight * SCENE->Background();
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Indirect Illumination Path-Tracing Method
////////////////////////////////////////////////////////////////////////

void MonteCarlo_IndirectSample(R3Ray& ray, RNRgb& color)
{

  // Intersection variables and forward declarations
  RNRgb total_weight = RNwhite_rgb;
  R3SceneElement *element;
  R3Point point;
  R3Vector normal;
  const R3Material *material;
  const R3Brdf *brdf;
  R3Point ray_start = ray.Start();
  R3Vector view;
  RNScalar cos_theta;
  RNScalar R_coeff;
  RNRgb color_buffer;

  // Probability values
  RNScalar prob_diffuse;
  RNScalar prob_transmission;
  RNScalar prob_specular;
  RNScalar prob_terminate;
  RNScalar prob_total;
  RNScalar rand;

  // Bouncing geometries
  R3Vector exact_bounce;
  R3Vector sampled_bounce;

  // Bounce until diffuse interaction
  for (int iter = 0; iter < MAX_MONTE_DEPTH; iter++) {
    if (SCENE->Intersects(ray, NULL, &element, NULL, &point, &normal, NULL)) {
      // Book keeping
      LOCAL_MONTE_RAY_COUNT++;

      // Get intersection information
      material = (element) ? element->Material() : &R3default_material;
      brdf = (material) ? material->Brdf() : &R3default_brdf;

      // Color buffer (compute and scale once)
      color_buffer = RNblack_rgb;

      if (brdf) {
        // Useful geometric values to precompute
        view = (point - ray_start);
        view.Normalize();
        cos_theta = normal.Dot(-view);

        // Compute Reflection Coefficient, carry reflection portion to Specular
        R_coeff = 0;
        if (FRESNEL && brdf->IsTransparent()) {
          R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());
        }

        // Generate material probabilities of bounce type
        prob_diffuse = MaxChannelVal(brdf->Diffuse());
        prob_transmission = MaxChannelVal(brdf->Transmission());
        prob_specular = MaxChannelVal(brdf->Specular()) + R_coeff*prob_transmission;
        prob_transmission *= (1.0 - R_coeff);
        prob_terminate = MaxChannelVal(brdf->Emission()) + PROB_ABSORB;
        prob_total = prob_diffuse + prob_transmission + prob_specular + prob_terminate;

        // Scale down to 1.0 (but never scale up bc of implicit absorption)
        // NB: faster to scale rand up than to normalize; would also need to adjust
        //     brdf values when updating weights (dividing prob_total back out)
        rand = RNThreadableRandomScalar();
        if (prob_total > 1.0) {
          rand *= prob_total;
        }

        // Bounce and sample or recur (choose one from distribution)
        if (rand < prob_diffuse) {
          // Sample photon map directly
          color_buffer = RNblack_rgb;
          exact_bounce = ReflectiveBounce(normal, view, cos_theta);
          if (IRRADIANCE_CACHE) {
            EstimateCachedRadiance(point, normal, color_buffer, brdf,
              exact_bounce, cos_theta, GLOBAL_PMAP, GLOBAL_ESTIMATE_DIST);
          } else {
            EstimateRadiance(point, normal, color_buffer, brdf,
              exact_bounce, cos_theta, GLOBAL_PMAP, GLOBAL_ESTIMATE_SIZE,
              GLOBAL_ESTIMATE_DIST, GLOBAL_FILTER);
            }
          color += color_buffer * brdf->Diffuse() * total_weight / prob_diffuse;
          break;
        } else if (rand < prob_diffuse + prob_transmission) {
          // Compute direction of transmissive bounce
          exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                            brdf->IndexOfRefraction());
          // Compute direction of next ray
          if (DISTRIB_TRANSMISSIVE) {
            // Use importance sampling
            sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
          } else {
            sampled_bounce = exact_bounce;
          }

          LOCAL_TRANSMISSIVE_RAY_COUNT++;
          // Update weights
          total_weight *= (1.0 - R_coeff) * brdf->Transmission() / prob_transmission;
        } else if (rand < prob_diffuse + prob_transmission + prob_specular) {
          // Compute direction of specular bounce
          exact_bounce = ReflectiveBounce(normal, view, cos_theta);
          // Compute direction of next ray
          if (DISTRIB_SPECULAR) {
            // Use importance sampling
            sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
          } else {
            sampled_bounce = exact_bounce;
          }

          LOCAL_SPECULAR_RAY_COUNT++;
          // Update weights
          total_weight *= (brdf->Specular() +  R_coeff*brdf->Transmission()) / prob_specular;
        } else {
          // Photon absorbed; terminate trace
          break;
        }

        // Recur
        ray_start = point + sampled_bounce * RN_EPSILON;
        ray = R3Ray(ray_start, sampled_bounce, TRUE);
      }
    } else {
      // Intersect with background and break
      color += total_weight * SCENE->Background();
      break;
    }
  }
}
