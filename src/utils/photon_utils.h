////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#ifndef PHOTON_UTILS_INC
#define PHOTON_UTILS_INC

#include "../render.h"
#include "../R3Graphics/R3Graphics.h"
#include <vector>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Storage Utils
////////////////////////////////////////////////////////////////////////

// Flush local memory to global memory using dynamic memory and synchronization
// primatives; returns 1 if successful; 0 otherwise
int FlushPhotonStorage(vector<Photon>& local_photon_storage, Photon_Type map_type);

// Store Photons locally in vector; if vector is full, safely copy to
// global memory using sychronization (thread safe); uses dynamic memory
void StorePhoton(RNRgb& photon, vector<Photon>& local_photon_storage,
  R3Vector& incident_vector, R3Point& point, Photon_Type map_type);

////////////////////////////////////////////////////////////////////////
// Radiance Utils
////////////////////////////////////////////////////////////////////////

// Sample the radiance at a point into the color from the provided photon map
void EstimateRadiance(R3Point& point, R3Vector& normal, RNRgb& color,
  const R3Brdf *brdf, const R3Vector& exact_bounce, RNScalar cos_theta,
  R3Kdtree<Photon*>* photon_map, int estimate_size,
  RNScalar estimate_dist);

////////////////////////////////////////////////////////////////////////
// Efficiency Utils
////////////////////////////////////////////////////////////////////////

// Build mapping from spherical coordinates to xyz coordinates for fast lookup
void BuildDirectionLookupTable(void);

#endif
