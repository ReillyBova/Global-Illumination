////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "photon_utils.h"
#include "graphics_utils.h"
#include "../render.h"
#include "../R3Graphics/R3Graphics.h"
#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Storage Utils
////////////////////////////////////////////////////////////////////////

// Flush local memory to global memory using dynamic memory and synchronization
// primatives; returns 1 if successful, 0 otherwise
int FlushPhotonStorage(vector<Photon>& local_photon_storage, Photon_Type map_type)
{
  // Lock for multithreading safety
  lock_guard<mutex> lk(LOCK);

  // Insert photons into global array
  for (int i = 0; i < TEMPORARY_STORAGE_COUNT; i++) {
    Photon* photon = new Photon(local_photon_storage[i]);
    if (map_type == GLOBAL) {
      GLOBAL_PHOTONS.Insert(photon);
    } else if (map_type == CAUSTIC) {
      CAUSTIC_PHOTONS.Insert(photon);
    }
  }

  TEMPORARY_STORAGE_COUNT = 0;
  return 1;
}

// Store Photons locally in vector; if vector is full, safely copy to
// global memory using sychronization (thread safe); uses dynamic memory
void StorePhoton(RNRgb& photon, vector<Photon>& local_photon_storage,
  R3Vector& incident_vector, R3Point& point, Photon_Type map_type)
{
  // Flush buffer if necessary
  if (TEMPORARY_STORAGE_COUNT >= SIZE_LOCAL_PHOTON_STORAGE) {
    int success = FlushPhotonStorage(local_photon_storage, map_type);
    if (!success) {
      fprintf(stderr, "\nError allocating photon memory \n");
      exit(-1);
    }
  }

  // Copy photon
  Photon& photon_target = local_photon_storage[TEMPORARY_STORAGE_COUNT];
  photon_target.position = point;
  RNRgb_to_RGBE(photon, photon_target.rgbe);
  int phi = (unsigned char) (255.0
                      * (atan2(incident_vector[1], incident_vector[0]) + RN_PI)
                      / (RN_TWO_PI));
  int theta = (unsigned char) (255.0 * acos(incident_vector[2]) / RN_PI);
  photon_target.direction = phi*256 + theta;

  TEMPORARY_STORAGE_COUNT++;
  PHOTONS_STORED_COUNT++;
  return;
}

////////////////////////////////////////////////////////////////////////
// Radiance Utils
////////////////////////////////////////////////////////////////////////

// Sample the radiance at a point into the color from the provided photon map
void EstimateRadiance(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, const R3Vector& exact_bounce, RNScalar cos_theta,
  R3Kdtree<Photon*>*  photon_map, int estimate_size,
  RNScalar estimate_dist, Filter_Type filter)
{
  // Find nearby points
  vector<PointAndDistanceSqd<Photon*> > nearby_points;
  photon_map->FindClosestQuick(point, 0, estimate_dist, estimate_size, nearby_points);
  // Compute actual radius of estimate
  int num_nearby = nearby_points.size();
  if (num_nearby == 0) {
    return;
  }
  RNScalar max_dist_sqd = RN_EPSILON;
  if (num_nearby < estimate_size) {
    max_dist_sqd = estimate_dist*estimate_dist;
  } else {
    // Do this seperately because certain filterings need max dist in advance
    for (int i = 0; i < num_nearby; i++) {
      // Update max distances
      if (nearby_points[i].distance_squared > max_dist_sqd) {
        max_dist_sqd = nearby_points[i].distance_squared;
      }
    }
  }

  // Estimate radiance using adjusted Phong brdf
  RNRgb estimate = RNblack_rgb;
  RNScalar fweight_c1 = 1.0;
  RNScalar fweight_c2 = 1.0;
  RNScalar total_fweight = 0;
  if (filter == CONE) {
    fweight_c1 = 1.0 / (FILTER_CONST_K * sqrt(max_dist_sqd));
  } else if (filter == GAUSS) {
    fweight_c1 = pow(RN_E, -FILTER_CONST_B);
    fweight_c2 = 1.0 / (2.0 * max_dist_sqd);
  }
  for (int i = 0; i < num_nearby; i++) {

    // Get photon info
    Photon* photon = nearby_points[i].point;
    int direction = photon->direction;
    RNScalar x = PHOTON_X_LOOKUP[direction];
    RNScalar y = PHOTON_Y_LOOKUP[direction];
    RNScalar z = PHOTON_Z_LOOKUP[direction];
    R3Vector incident_vector = R3Vector(x, y ,z);

    // Check normal
    RNScalar perp_component = normal.Dot(incident_vector);
    if ((cos_theta < 0 && perp_component < 0) || (cos_theta > 0 && perp_component > 0)) {
      continue;
    }

    // Sample flux
    RNRgb photon_color = RGBE_to_RNRgb(photon->rgbe);
    RNScalar cos_alpha = exact_bounce.Dot(-incident_vector);
    if (cos_alpha < 0) {
      // Clamp to pi/2
      cos_alpha = 0;
    }
    // Phong BRDF
    RNScalar n = brdf->Shininess();
    photon_color *= abs(perp_component) * brdf->Diffuse()
                        + pow(cos_alpha, n) * brdf->Specular();

    // Filter
    if (filter == CONE) {
      photon_color *= (1.0 - fweight_c1 * R3Distance(photon->position, point));
    } else if (filter == GAUSS) {
      RNScalar weight = (1.0 - (1.0 - pow(fweight_c1, fweight_c2 * R3SquaredDistance(photon->position, point))) / (1.0 - fweight_c1));
      photon_color *= weight;
      total_fweight += weight;
    }
    estimate += photon_color;
  }

  // Filter
  if (filter == DISK) {
    estimate /= RN_PI * max_dist_sqd;
  } else if (filter == CONE) {
    estimate /= (1.0 - 2.0 / 3.0 / FILTER_CONST_K) * RN_PI * max_dist_sqd;
  } else if (filter == GAUSS) {
    // Scale, Normalize, and Project
    estimate *= FILTER_CONST_A * (num_nearby / total_fweight) / (RN_PI * max_dist_sqd);
  }

  // Save estimate and cleanup
  color += estimate;
}

// Sample the radiance from the irradiance cache
void EstimateCachedRadiance(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, const R3Vector& exact_bounce, RNScalar cos_theta,
  R3Kdtree<Photon*>*  photon_map, RNScalar estimate_dist)
{
  // Find nearest point
  Photon* closest_photon;
  RNLength closest_dist = 0;
  int direction;
  RNScalar x;
  RNScalar y;
  RNScalar z;
  R3Vector incident_vector;
  RNScalar perp_component;

  do {
    closest_photon = photon_map->FindClosest(point, closest_dist + RN_EPSILON, estimate_dist, &closest_dist);
    direction = closest_photon->direction;
    if (closest_photon == NULL)
      return;
    x = PHOTON_X_LOOKUP[direction];
    y = PHOTON_Y_LOOKUP[direction];
    z = PHOTON_Z_LOOKUP[direction];
    incident_vector = R3Vector(x, y ,z);
    // Check normal
    perp_component = normal.Dot(incident_vector);
  } while ((cos_theta < 0 && perp_component < 0) || (cos_theta > 0 && perp_component > 0));

  // Sample flux
  RNRgb photon_color = RGBE_to_RNRgb(closest_photon->rgbe);
  RNScalar cos_alpha = exact_bounce.Dot(-incident_vector);
  if (cos_alpha < 0) {
    // Clamp to pi/2
    cos_alpha = 0;
  }
  // Phong BRDF
  RNScalar n = brdf->Shininess();
  photon_color *= abs(perp_component) * brdf->Diffuse()
                      + pow(cos_alpha, n) * brdf->Specular();

  color += photon_color;
}


// Roughly sample the irradiance at a point
void EstimateIrradiance(R3Point& point, RNRgb& color, R3Kdtree<Photon*>* photon_map,
    int estimate_size, RNScalar estimate_dist)
{
  // Find nearby points
  vector<PointAndDistanceSqd<Photon*> > nearby_points;
  photon_map->FindClosestQuick(point, 0, estimate_dist, estimate_size, nearby_points);
  // Compute actual radius of estimate
  int num_nearby = nearby_points.size();
  if (num_nearby == 0) {
    return;
  }
  RNScalar max_dist_sqd = RN_EPSILON;
  if (num_nearby < estimate_size) {
    max_dist_sqd = estimate_dist*estimate_dist;
  } else {
    // Do this seperately for style
    for (int i = 0; i < num_nearby; i++) {
      // Update max distances
      if (nearby_points[i].distance_squared > max_dist_sqd) {
        max_dist_sqd = nearby_points[i].distance_squared;
      }
    }
  }

  // Estimate irradiance using disk
  RNRgb estimate = RNblack_rgb;
  for (int i = 0; i < num_nearby; i++) {
    // Get photon info
    Photon* photon = nearby_points[i].point;
    // Sample flux
    estimate += RGBE_to_RNRgb(photon->rgbe);
  }

  // Filter
  estimate /= RN_PI * max_dist_sqd;
  // Save estimate
  color += estimate;
}

////////////////////////////////////////////////////////////////////////
// Efficiency Utils
////////////////////////////////////////////////////////////////////////

// Build mapping from spherical coordinates to xyz coordinates for fast lookup
void BuildDirectionLookupTable(void)
{
  for (int phi = 0; phi < 256; phi++) {
    for (int theta = 0; theta < 256; theta++) {
      // Convert to cartesian
      RNAngle true_phi = (phi * RN_TWO_PI / 255.0) - RN_PI;
      RNAngle true_theta = (theta * RN_PI / 255.0);
      RNScalar x = sin(true_theta) * cos(true_phi);
      RNScalar y = sin(true_theta) * sin(true_phi);
      RNScalar z = cos(true_theta);
      // Normalize (might be slightly off)
      R3Vector norm = R3Vector(x, y, z);
      norm.Normalize();
      // Store
      PHOTON_X_LOOKUP[256*phi + theta] = norm[0];
      PHOTON_Y_LOOKUP[256*phi + theta] = norm[1];
      PHOTON_Z_LOOKUP[256*phi + theta] = norm[2];
    }
  }
}
