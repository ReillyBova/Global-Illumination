////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "photontracer.h"
#include "render.h"
#include "utils/graphics_utils.h"
#include "utils/io_utils.h"
#include "utils/photon_utils.h"
#include "R3Graphics/R3Graphics.h"
#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////
// File variables/constants
////////////////////////////////////////////////////////////////////////

const int SIZE_LOCAL_PHOTON_STORAGE = 100000;
__thread int PHOTONS_STORED_COUNT = 0;
__thread int TEMPORARY_STORAGE_COUNT = 0;

////////////////////////////////////////////////////////////////////////
// Photon Tracing Method
////////////////////////////////////////////////////////////////////////

// Monte carlo trace photon, storing at each diffuse intersection
void PhotonTrace(R3Ray ray, RNRgb photon, vector<Photon>& local_photon_storage,
  Photon_Type map_type, int thread_id)
{
  // Intersection variables and forward declarations
  R3SceneElement *element;
  R3Point point;
  R3Vector normal;
  const R3Material *material;
  const R3Brdf *brdf;
  R3Point ray_start = ray.Start();
  R3Vector view;
  RNScalar cos_theta;
  RNScalar R_coeff;

  // Probability values
  RNScalar max_channel;
  RNScalar prob_diffuse;
  RNScalar prob_transmission;
  RNScalar prob_specular;
  RNScalar prob_terminate;
  RNScalar prob_total;
  RNScalar rand;

  // Bouncing geometries
  R3Vector exact_bounce;
  R3Vector sampled_bounce;

  // Storage boolean (used to differentiate map type)
  bool store = false;
  if (map_type == GLOBAL && !FAST_GLOBAL) {
    // Global maps always store
    store = true;
  }

  // Trace the photon through its bounces, storing it at diffuse surfaces
  int iter;
  for (iter = 0; iter < MAX_PHOTON_DEPTH &&
    SCENE->Intersects(ray, NULL, &element, NULL, &point, &normal, NULL); iter++)
  {
    // Get intersection information
    material = (element) ? element->Material() : &R3default_material;
    brdf = (material) ? material->Brdf() : &R3default_brdf;
    if (brdf) {
      // Useful geometric values to precompute
      view = (point - ray_start);
      view.Normalize();
      cos_theta = normal.Dot(-view);

      // Diffuse interaction (store unless first bounce of caustic)
      if (brdf->IsDiffuse() && store) {
        StorePhoton(photon, local_photon_storage, view, point, map_type);
      }

      // Compute Reflection Coefficient, carry reflection portion to Specular
      R_coeff = 0;
      if (FRESNEL && brdf->IsTransparent()) {
        R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());
      }

      // Generate material probabilities of bounce type
      max_channel = MaxChannelVal(photon);
      prob_diffuse = MaxChannelVal(brdf->Diffuse() * photon) / max_channel;
      prob_transmission = MaxChannelVal(brdf->Transmission() * photon) / max_channel;
      prob_specular = (MaxChannelVal(brdf->Specular() * photon) / max_channel)
                        + R_coeff*prob_transmission;
      prob_transmission *= (1.0 - R_coeff);
      prob_terminate = PROB_ABSORB;
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
        // Terminate if caustic trace
        if (map_type == CAUSTIC) {
          break;
        }

        // Fast global maps can store after the first diffuse bounce
        store = true;

        // Otherwise, compute direction of diffuse bounce
        sampled_bounce = Diffuse_ImportanceSample(normal, cos_theta);
        // Update weights
        photon *= brdf->Diffuse() / prob_diffuse;
      } else if (rand < prob_diffuse + prob_transmission) {
        // Compute direction of transmissive bounce
        exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                          brdf->IndexOfRefraction());
        if (DISTRIB_TRANSMISSIVE) {
          // Caustics can now store after non diffuse bounce
          if (map_type == CAUSTIC) {
            store = true;
          }
          // Use importance sampling
          sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
        } else {
          sampled_bounce = exact_bounce;
        }
        // Update weights
        photon *= brdf->Transmission() / prob_transmission;
      } else if (rand < prob_diffuse + prob_transmission + prob_specular) {
        // Compute direction of specular bounce
        exact_bounce = ReflectiveBounce(normal, view, cos_theta);
        if (DISTRIB_SPECULAR) {
          // Caustics can now store after non diffuse bounce
          if (map_type == CAUSTIC) {
            store = true;
          }
          // Use importance sampling
          sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
        } else {
          sampled_bounce = exact_bounce;
        }
        // Update weights
        photon *= brdf->Specular() / prob_specular;
      } else {
        // Photon absorbed; terminate trace
        break;
      }

      ray_start = point + sampled_bounce * RN_EPSILON;
      ray = R3Ray(ray_start, sampled_bounce, TRUE);
    }
  }

  // Print progress if necessary
  if (VERBOSE && thread_id == 0) {
    static int last_value = -1;
    double progress;
    if (map_type == GLOBAL) {
      progress = double(PHOTONS_STORED_COUNT) / GLOBAL_PHOTON_COUNT * THREADS;
    } else {
      progress = double(PHOTONS_STORED_COUNT) / CAUSTIC_PHOTON_COUNT * THREADS;
    }
    int next_value = int(progress * 100.0);
    if (next_value != last_value) {
      PrintProgress(progress, PROGRESS_BAR_WIDTH);
      last_value = next_value;
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Photon Emitting Method (invokes internal photon tracer)
////////////////////////////////////////////////////////////////////////

void EmitPhotons(int num_photons, R3Light* light, Photon_Type map_type, int thread_id)
{
  // Corner cases
  if (!(light->IsActive()) || !num_photons) return;

  // Compute photon power (normalized across lights in scene)
  RNRgb photon = light->Color();
  NormalizeColor(photon);

  // Allocate stack space for fast thread local storage
  vector<Photon> local_photon_storage(SIZE_LOCAL_PHOTON_STORAGE);

  // Reset counts
  TEMPORARY_STORAGE_COUNT = 0;

  // Emit photons based on geometry of light
  if (light->ClassID() == R3DirectionalLight::CLASS_ID()) {
    // Directional Light (emit from a large disk outside scene)
    R3DirectionalLight *directional_light = (R3DirectionalLight *) light;
    R3Vector light_norm = directional_light->Direction();
    R3Point center = SCENE->Centroid() - light_norm * SCENE_RADIUS * 3.0;

    // Find two perpendicular vectors spanning plane of light
    R3Vector u = R3Vector(light_norm[1], -light_norm[0], 0);
    if (1.0 - abs(light_norm[2]) < 0.1) {
      u = R3Vector(light_norm[2], 0, -light_norm[0]);
    }
    R3Vector v = u % light_norm;
    u.Normalize();
    v.Normalize();

    // Scale u and v
    u *= SCENE_RADIUS;
    v *= SCENE_RADIUS;

    // General Forward Declarations
    R3Point sample_point;
    R3Ray ray;
    RNScalar r1, r2;

    // Emit photons
    for (int i = 0; i < num_photons; i++) {
      do {
        // Sample point in circle
        r1 = RNThreadableRandomScalar()*2.0 - 1.0;
        r2 = RNThreadableRandomScalar()*2.0 - 1.0;
      } while (r1*r1 + r2*r2 > 1.0);

      sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;
      ray = R3Ray(sample_point, light_norm, TRUE);
      PhotonTrace(ray, photon,local_photon_storage, map_type, thread_id);
    }
  } else if (light->ClassID() == R3PointLight::CLASS_ID()) {
    // Point Light (use spherical point picking to pick emmission direction)
    R3PointLight *point_light = (R3PointLight *) light;
    R3Point center = point_light->Position();

    // General Forward Declarations
    R3Vector sample_direction;
    R3Ray ray;
    RNScalar x, y, z;

    // Emit photons
    for (int i = 0; i < num_photons; i++) {
      do {
        // Sample direction in sphere
        x = RNThreadableRandomScalar()*2.0 - 1.0;
        y = RNThreadableRandomScalar()*2.0 - 1.0;
        z = RNThreadableRandomScalar()*2.0 - 1.0;
      } while (x*x + y*y + z*z > 1.0);

      sample_direction = R3Vector(x, y, z);
      sample_direction.Normalize();
      ray = R3Ray(center, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type, thread_id);
    }
  } else if (light->ClassID() == R3SpotLight::CLASS_ID()) {
    // Spot Light (use specular importance sampling)
    R3SpotLight *spot_light = (R3SpotLight *) light;
    R3Point center = spot_light->Position();
    R3Vector light_norm = spot_light->Direction();
    RNScalar n = spot_light->DropOffRate();
    RNScalar cutoff = abs(cos(spot_light->CutOffAngle()));

    // General Forward Declarations
    R3Vector sample_direction;
    R3Ray ray;
    int attempts_left;

    // Emit photons
    for (int i = 0; i < num_photons; i++) {
      attempts_left = 20;
      do {
        // Sample perturbation from light direction
        sample_direction = Specular_ImportanceSample(light_norm, n, 1.0);
      } while (sample_direction.Dot(light_norm) < cutoff && attempts_left-- > 0);

      // Cheat the dropoff
      if (attempts_left == 0) {
        sample_direction = Specular_ImportanceSample(light_norm, n, cutoff);
      }

      ray = R3Ray(center, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type, thread_id);
    }
  } else if (light->ClassID() == R3AreaLight::CLASS_ID()) {
    // Area Light (pick from a circle and diffuse direction)
    R3AreaLight *area_light = (R3AreaLight *) light;

    // Get light geometry
    R3Point center = area_light->Position();
    R3Vector light_norm = area_light->Direction();
    RNLength radius = area_light->Radius();

    // Find two perpendicular vectors spanning circle plane
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

    // Genereal Forward Declarations
    R3Point sample_point;
    R3Vector sample_direction;
    R3Ray ray;
    RNScalar r1, r2;

    for (int i = 0; i < num_photons; i++) {
      do {
        // Sample point in circle
        r1 = RNThreadableRandomScalar()*2.0 - 1.0;
        r2 = RNThreadableRandomScalar()*2.0 - 1.0;
      } while (r1*r1 + r2*r2 > 1.0);

      // Use values r1, r2 and vectors u, v to find a random point on light
      sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;

      // Use diffuse importance sampling to pick a direction
      sample_direction = Diffuse_ImportanceSample(light_norm, 1.0);

      ray = R3Ray(sample_point, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type, thread_id);
    }
  } else if (light->ClassID() == R3RectLight::CLASS_ID()) {
    // Rect Light (pick from rectangle and emit in diffuse direction)
    R3RectLight *rect_light = (R3RectLight *) light;

    // Get light geometry
    R3Point center = rect_light->Position();
    R3Vector light_norm = rect_light->Direction();

    // Get scaled axes
    R3Vector a1 = rect_light->PrimaryAxis() * rect_light->PrimaryLength();
    R3Vector a2 = rect_light->SecondaryAxis() * rect_light->SecondaryLength();

    // Genereal Forward Declarations
    R3Point sample_point;
    R3Vector sample_direction;
    R3Ray ray;
    RNScalar r1, r2;

    for (int i = 0; i < num_photons; i++) {
      r1 = RNThreadableRandomScalar() - 0.5;
      r2 = RNThreadableRandomScalar() - 0.5;

      // Use values r1, r2 and vectors a1, a2 to find a random point on light
      sample_point = r1*a1 + r2*a2 + center + light_norm*RN_EPSILON;

      // Use diffuse importance sampling to pick a direction
      sample_direction = Diffuse_ImportanceSample(light_norm, 1.0);
      ray = R3Ray(sample_point, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type, thread_id);
    }
  } else {
    fprintf(stderr, "Unrecognized light type: %d\n", light->ClassID());
  }

  // Flush storage
  int success = FlushPhotonStorage(local_photon_storage, map_type);
  if (!success) {
    fprintf(stderr, "\nError allocating photon memory \n");
    exit(-1);
  }

  return;
}
