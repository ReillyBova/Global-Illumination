////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////
#ifndef RENDER_INC
#define RENDER_INC

#include "R3Graphics/R3Graphics.h"
#include <mutex>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Global struct definitions
////////////////////////////////////////////////////////////////////////

// Photon data structure
struct Photon {
  R3Point position; // Position
  unsigned char rgbe[4];     // compressed RGB values
  unsigned short direction;  // compressed REFLECTION direction
};

////////////////////////////////////////////////////////////////////////
// Global variables/constants
////////////////////////////////////////////////////////////////////////

extern bool VERBOSE;
extern int THREADS;
extern bool FRESNEL;
extern RNScalar IR_AIR;

extern bool AMBIENT;
extern bool DIRECT_ILLUM;
extern bool TRANSMISSIVE_ILLUM;
extern bool SPECULAR_ILLUM;
extern bool INDIRECT_ILLUM;
extern bool CAUSTIC_ILLUM;

extern bool DIRECT_PHOTON_ILLUM;
extern bool FAST_GLOBAL;

extern bool SHADOWS;
extern bool SOFT_SHADOWS;
extern int LIGHT_TEST;
extern int SHADOW_TEST;

extern bool MONTE_CARLO;
extern int MAX_MONTE_DEPTH;
extern RNScalar PROB_ABSORB;
extern bool RECURSIVE_SHADOWS;
extern bool DISTRIB_TRANSMISSIVE;
extern int TRANSMISSIVE_TEST;
extern bool DISTRIB_SPECULAR;
extern int SPECULAR_TEST;

enum Photon_Type {GLOBAL, CAUSTIC};

extern int GLOBAL_PHOTON_COUNT;
extern int CAUSTIC_PHOTON_COUNT;
extern const int SIZE_LOCAL_PHOTON_STORAGE;
extern int MAX_PHOTON_DEPTH;

extern int INDIRECT_TEST;
extern int GLOBAL_ESTIMATE_SIZE;
extern RNScalar GLOBAL_ESTIMATE_DIST;
extern int CAUSTIC_ESTIMATE_SIZE;
extern RNScalar CAUSTIC_ESTIMATE_DIST;

extern R3Kdtree<Photon *> *GLOBAL_PMAP;
extern R3Kdtree<Photon *> *CAUSTIC_PMAP;
extern RNArray <Photon *> GLOBAL_PHOTONS;
extern RNArray <Photon *> CAUSTIC_PHOTONS;

extern RNScalar PHOTON_X_LOOKUP[65536];
extern RNScalar PHOTON_Y_LOOKUP[65536];
extern RNScalar PHOTON_Z_LOOKUP[65536];

extern R3Scene* SCENE;
extern RNScalar SCENE_RADIUS;
extern RNRgb SCENE_AMBIENT;
extern int SCENE_NLIGHTS;

extern const int PROGRESS_BAR_WIDTH;

extern mutex LOCK;

__thread extern int PHOTONS_STORED_COUNT;
__thread extern int TEMPORARY_STORAGE_COUNT;

__thread extern unsigned long long int LOCAL_RAY_COUNT;
__thread extern unsigned long long int LOCAL_SHADOW_RAY_COUNT;
__thread extern unsigned long long int LOCAL_MONTE_RAY_COUNT;
__thread extern unsigned long long int LOCAL_TRANSMISSIVE_RAY_COUNT;
__thread extern unsigned long long int LOCAL_SPECULAR_RAY_COUNT;
__thread extern unsigned long long int LOCAL_INDIRECT_RAY_COUNT;
__thread extern unsigned long long int LOCAL_CAUSTIC_RAY_COUNT;

////////////////////////////////////////////////////////////////////////
// Main Rendering Method
////////////////////////////////////////////////////////////////////////

R2Image *RenderImage(int aa, int width, int height);

#endif
