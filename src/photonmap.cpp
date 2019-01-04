// Source file for the photon mapping program

// Include files
#include "R3Graphics/R3Graphics.h"
#include "render.h"
#include "photontracer.h"
#include "utils/io_utils.h"
#include "utils/graphics_utils.h"
#include "utils/photon_utils.h"
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Local Parameter Defaults & Variables
////////////////////////////////////////////////////////////////////////

// Input scene & Output file
static char *input_scene_name = NULL;
static char *output_image_name = NULL;

// Image dimensions
static int render_image_width = 1024;
static int render_image_height = 1024;

// Anti-alias by super sampling 4^(aa) eye rays sampled per pixel)
static int aa = 2;

// Normalize material components to 1 when loading brdfs
static bool real_material = false;

////////////////////////////////////////////////////////////////////////
// Global Parameter Defaults (Declared in render.h)
////////////////////////////////////////////////////////////////////////

// Print Rendering Stats
bool VERBOSE = false;
// Number of threads
int THREADS = 1;
// Use fresnel equations to split transmission into refraction and reflection
bool FRESNEL = true;
// Refraction index of air
RNScalar IR_AIR = 1.0;

// Render equation toggles
bool AMBIENT = true;
bool DIRECT_ILLUM = true;
bool TRANSMISSIVE_ILLUM = true;
bool SPECULAR_ILLUM = true;
bool INDIRECT_ILLUM = true;
bool CAUSTIC_ILLUM = true;

// Photon map direct visualization
bool DIRECT_PHOTON_ILLUM = false;
// Forces DIRECT_PHOTON_ILLUM to be true, but does not store photons on first bounce
// Used alongide other illuminations to quickly approximate global illumination with less noise
// than sampling the photon map alone
bool FAST_GLOBAL = false;

// Soft shadows (toggle and number of shadow rays sent per test)
bool SHADOWS = true;
bool SOFT_SHADOWS = true;
int LIGHT_TEST = 128; // Number of direct illumination tests per 2Dlight;
                     // NB: Each illumination test is also a shadow test
int SHADOW_TEST = 128; // Total number of *additional* shadow tests per light
                      // (on top of implicit the shadow tests set via LIGHT_TEST);

// Monte Carlo Raytracing Parameters
bool MONTE_CARLO = true; // Toggles monte carlo path tracing
int MAX_MONTE_DEPTH = 128; // Toggles monte carlo path tracing
RNScalar PROB_ABSORB = 0.005; // Minimal brdf absorption probability (out of one)
bool RECURSIVE_SHADOWS = true; // Test shadows while path tracing
bool DISTRIB_TRANSMISSIVE = true; // Use importance sampling
int TRANSMISSIVE_TEST = 128;
bool DISTRIB_SPECULAR = true; // Use importance sampling
int SPECULAR_TEST = 128;

// Photon Map Tracing Parameters
int GLOBAL_PHOTON_COUNT = 1920; // Number of photons emmitted for global map
int CAUSTIC_PHOTON_COUNT = 300000; // Number of photons emmited for caustic map
int MAX_PHOTON_DEPTH = 128;

// Photon Map Sampling Parameters
int INDIRECT_TEST = 256;
int GLOBAL_ESTIMATE_SIZE = 50;
RNScalar GLOBAL_ESTIMATE_DIST = 2.5;
int CAUSTIC_ESTIMATE_SIZE = 200;
RNScalar CAUSTIC_ESTIMATE_DIST = 1.00;

////////////////////////////////////////////////////////////////////////
// Global Variable Defaults (Declared in render.h)
////////////////////////////////////////////////////////////////////////

// KdTrees for the photon maps
R3Kdtree<Photon *> *GLOBAL_PMAP = NULL;
R3Kdtree<Photon *> *CAUSTIC_PMAP = NULL;

// Memory for photons
RNArray <Photon *> GLOBAL_PHOTONS;
RNArray <Photon *> CAUSTIC_PHOTONS;

// Lookup tables for incident direction
RNScalar PHOTON_X_LOOKUP[65536];
RNScalar PHOTON_Y_LOOKUP[65536];
RNScalar PHOTON_Z_LOOKUP[65536];

// Scene parameters
R3Scene* SCENE = NULL;
RNScalar SCENE_RADIUS = 0;
RNRgb SCENE_AMBIENT = RNblack_rgb;
int SCENE_NLIGHTS = 0;

// Progress bar
const int PROGRESS_BAR_WIDTH = 50;

// Shared synchronization primative
mutex LOCK;

static atomic_int global_emitted_count (0);
static atomic_int caustic_emitted_count (0);

////////////////////////////////////////////////////////////////////////
// Photon Mapping Methods
////////////////////////////////////////////////////////////////////////

// Threadable (parallelizable) photon tracing method
static void Threadable_PhotonTracer(const int num_global_photons,
  const int num_caustic_photons, vector<RNScalar> &light_powers, RNScalar total_power,
  int thread_id)
{
  RNInitThreadRandomness();
  // Global (Indirect) Illumination Photon mapping
  int local_global_emitted_count = 0;
  if (INDIRECT_ILLUM || DIRECT_PHOTON_ILLUM) {
    // Print Info
    if (VERBOSE && (thread_id == 0)) {
      printf("Building global photon map ...\n");
    }

    // Emit photons from each light in rounds (slowly reaching GLOBAL_PHOTON_COUNT)
    PHOTONS_STORED_COUNT = 0;
    RNScalar average_bounce_rate = 4.0; // Init with an overestimate (depends on scene)
    RNScalar slowdown_factor = 1.0;
    int attempts_left = 10;
    while (PHOTONS_STORED_COUNT < num_global_photons && attempts_left > 0) {
      // Approach goal based on how we've done thus far
      int emit_goal = (int) (num_global_photons - PHOTONS_STORED_COUNT)
                      / average_bounce_rate / slowdown_factor + 1;
      int photons_assigned = 0;

      // Emit photons
      for (int i = 0; i < SCENE_NLIGHTS; i++) {
        // Emit photons proportional to light contribution over total power, as well as the
        // assigned total number of photons to emit
        int num_photons = ceil(emit_goal * (light_powers[i] / total_power));
        EmitPhotons(num_photons, SCENE->Light(i), GLOBAL, thread_id);
        photons_assigned += num_photons;
      }
      local_global_emitted_count += photons_assigned;

      // Update average
      if (PHOTONS_STORED_COUNT > 0 && local_global_emitted_count > 0) {
        average_bounce_rate = ((RNScalar) (PHOTONS_STORED_COUNT)) / local_global_emitted_count;

        // Approach slower for first 75% to avoid shooting over
        if ((RNScalar) PHOTONS_STORED_COUNT / local_global_emitted_count < 0.75) {
          slowdown_factor = 2.0;
        } else {
          slowdown_factor = 1.0;
        }
      } else {
        average_bounce_rate /= 2.0;
        attempts_left--;
      }
    }
    if (VERBOSE && (thread_id == 0)) {
      PrintProgress(double(PHOTONS_STORED_COUNT) / GLOBAL_PHOTON_COUNT * THREADS,
        PROGRESS_BAR_WIDTH);
      printf("\n");
    }
  }

  // Caustic Illumination Photon mapping
  int local_caustic_emitted_count = 0;
  if (CAUSTIC_ILLUM) {
    // Print Info
    if (VERBOSE && (thread_id == 0)) {
      printf("Building caustic photon map ...\n");
    }

    // Emit photons from each light
    PHOTONS_STORED_COUNT = 0;
    RNScalar average_bounce_rate = MAX_PHOTON_DEPTH; // Init with an overestimate (depends on scene)
    RNScalar slowdown_factor = 1.0;
    int attempts_left = 10;
    while (PHOTONS_STORED_COUNT < num_caustic_photons && attempts_left > 0) {
      // Approach goal based on how we've done thus far
      int emit_goal = (int) (num_caustic_photons - PHOTONS_STORED_COUNT)
                        / average_bounce_rate / slowdown_factor + 1;
      int photons_assigned = 0;

      // Emit photons
      for (int i = 0; i < SCENE_NLIGHTS; i++) {
        // Emit photons proportional to light contribution to total power and the
        // assigned total number of photons to emit
        int num_photons = ceil(emit_goal * (light_powers[i] / total_power));
        EmitPhotons(num_photons, SCENE->Light(i), CAUSTIC, thread_id);
        photons_assigned += num_photons;
      }
      local_caustic_emitted_count += photons_assigned;

      // Update average
      if (PHOTONS_STORED_COUNT > 0 && local_caustic_emitted_count > 0) {
        average_bounce_rate = ((RNScalar) (PHOTONS_STORED_COUNT)) / local_caustic_emitted_count;

        // Approach slower for first 75% to avoid shooting over
        if ((RNScalar) PHOTONS_STORED_COUNT / num_caustic_photons < 0.75) {
          slowdown_factor = 2.0;
        } else {
          slowdown_factor = 1.0;
        }
      } else {
        average_bounce_rate /= 2.0;
        attempts_left--;
      }
    }
    if (VERBOSE && (thread_id == 0)) {
      PrintProgress(double(PHOTONS_STORED_COUNT) / CAUSTIC_PHOTON_COUNT * THREADS,
        PROGRESS_BAR_WIDTH);
      printf("\n");
    }
  }

  // Update counts
  global_emitted_count += local_global_emitted_count;
  caustic_emitted_count += local_caustic_emitted_count;

  RNClearThreadRandomness();
}

// Multithreading method that populates the photon maps as necessarys
static void MapPhotons(void)
{
  // Corner case
  if (SCENE_NLIGHTS <= 0) {
    return;
  }

  // Children threads
  thread *children = new thread[THREADS - 1];

  // Start statistics
  RNTime total_start_time, photon_time, kd_time;
  RNScalar photon_dur, kd_dur;
  total_start_time.Read();

  // Compute power distribution of lights
  vector<RNScalar> light_powers(SCENE_NLIGHTS, 0.0);
  RNScalar total_power = 0;
  for (int i = 0; i < SCENE_NLIGHTS; i++) {
    R3Light *light = SCENE->Light(i);
    if (!(light->IsActive())) continue;
    RNScalar light_power = LightPower(light);
    light_powers[i] = light_power;
    total_power += light_power;
  }

  // Corner case
  if (total_power <= 0) {
    return;
  }

  // Build compressed spherical coordinates mapping for fast lookup
  BuildDirectionLookupTable();

  // Divide work among threads
  int global_photons_remaining = 0;
  int caustic_photons_remaining = 0;

  // For global photons
  if (INDIRECT_ILLUM || DIRECT_PHOTON_ILLUM) {
    // Compute how many photons to distribute to each thread
    global_photons_remaining = GLOBAL_PHOTON_COUNT;
  }
  int global_photons_per_thread = (int) global_photons_remaining / THREADS;

  // For caustic photons
  if (CAUSTIC_ILLUM) {
    // Compute how many photons to distribute to each thread
    caustic_photons_remaining = CAUSTIC_PHOTON_COUNT;
  }
  int caustic_photons_per_thread = (int) caustic_photons_remaining / THREADS;

  // Split off into threads
  photon_time.Read();
  for (int i = 0; i < THREADS - 1; ++i) {
    // Launch thread and book-keep
    children[i] = thread(Threadable_PhotonTracer, global_photons_per_thread,
                      caustic_photons_per_thread, ref(light_powers), total_power, i+1);
    global_photons_remaining -= global_photons_per_thread;
    caustic_photons_remaining -= caustic_photons_per_thread;
  }

  // Use the main thread as well
  Threadable_PhotonTracer(global_photons_remaining, caustic_photons_remaining,
                          light_powers, total_power, 0);

  // Join children threads
  for (int i = 0; i < THREADS - 1; i++)
    children[i].join();
  delete [] children;

  photon_dur = photon_time.Elapsed();

  // Build kd trees
  if (VERBOSE)
    printf("Building kdtrees ...\n");
  kd_time.Read();

  // First update globals and scale by power
  if ((INDIRECT_ILLUM || DIRECT_PHOTON_ILLUM) && GLOBAL_PHOTONS.NEntries()) {
    GLOBAL_PHOTON_COUNT = GLOBAL_PHOTONS.NEntries();
    RNScalar photon_power = (RNScalar) total_power / global_emitted_count.load();
    for (int i = 0; i < GLOBAL_PHOTON_COUNT; i++) {
      RNRgb color = RGBE_to_RNRgb(GLOBAL_PHOTONS[i]->rgbe);
      color *= photon_power;
      RNRgb_to_RGBE(color, GLOBAL_PHOTONS[i]->rgbe);
    }
  } else if ((INDIRECT_ILLUM || DIRECT_PHOTON_ILLUM) && GLOBAL_PHOTONS.NEntries() == 0) {
    INDIRECT_ILLUM = false;
    DIRECT_PHOTON_ILLUM = false;
  }
  if (CAUSTIC_ILLUM && CAUSTIC_PHOTONS.NEntries()) {
    CAUSTIC_PHOTON_COUNT = CAUSTIC_PHOTONS.NEntries();
    RNScalar photon_power = total_power / caustic_emitted_count.load();
    for (int i = 0; i < CAUSTIC_PHOTON_COUNT; i++) {
      RNRgb color = RGBE_to_RNRgb(CAUSTIC_PHOTONS[i]->rgbe);
      color *= photon_power;
      RNRgb_to_RGBE(color, CAUSTIC_PHOTONS[i]->rgbe);
    }
  } else if (CAUSTIC_ILLUM && CAUSTIC_PHOTONS.NEntries() == 0) {
    CAUSTIC_ILLUM = false;
  }

  // Now build...
  if (INDIRECT_ILLUM || DIRECT_PHOTON_ILLUM) {
    GLOBAL_PMAP = new R3Kdtree<Photon *>(GLOBAL_PHOTONS, offsetof(struct Photon, position));
    if (!GLOBAL_PMAP) {
      fprintf(stderr, ("Unable to create global photon map\n"));
      exit(-1);
    }
  }
  if (CAUSTIC_ILLUM) {
    CAUSTIC_PMAP = new R3Kdtree<Photon *>(CAUSTIC_PHOTONS, offsetof(struct Photon, position));
    if (!CAUSTIC_PMAP) {
      fprintf(stderr, ("Unable to create caustic photon map\n"));
      exit(-1);
    }
  }
  kd_dur = kd_time.Elapsed();

  // Print statistics
  if (VERBOSE) {
    int total_photon_count = 0;
    printf("Built photon map ...\n");
    printf("  Total Time = %.2f seconds\n", total_start_time.Elapsed());
    printf("  Photon Tracing = %.2f seconds\n", photon_dur);
    printf("  KdTree Construction = %.2f seconds\n", kd_dur);
    if (INDIRECT_ILLUM || DIRECT_PHOTON_ILLUM) {
      printf("  # Global Photons Stored = %u\n", GLOBAL_PHOTONS.NEntries());
      total_photon_count += GLOBAL_PHOTONS.NEntries();
    }
    if (CAUSTIC_ILLUM) {
      printf("  # Caustic Photons Stored = %u\n", CAUSTIC_PHOTONS.NEntries());
      total_photon_count += CAUSTIC_PHOTONS.NEntries();
    }
    printf("Total Photons Stored: %u\n", total_photon_count);
    fflush(stdout);
  }
}

////////////////////////////////////////////////////////////////////////
// Main program
////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  // Parse program arguments
  if (!ParseArgs(argc, argv, input_scene_name, output_image_name, render_image_width,
    render_image_height, aa, real_material)) exit(-1);

  // Read scene
  SCENE = ReadScene(input_scene_name, real_material);
  if (!SCENE) exit(-1);

  // Check output image file
  if (output_image_name) {
    // Set useful constants
    SCENE_RADIUS = SCENE->BBox().DiagonalRadius();
    SCENE_AMBIENT = SCENE->Ambient();
    SCENE_NLIGHTS = SCENE->NLights();

    // Generate Photon Map if necessary
    if (INDIRECT_ILLUM || CAUSTIC_ILLUM || DIRECT_PHOTON_ILLUM) {
      MapPhotons();
    }

    // Scale for anti-aliasing
    int aa_factor = pow(2.0, aa);

    // Set scene viewport (scaled for anti aliasing)
    SCENE->SetViewport(R2Viewport(0, 0, render_image_width*aa_factor, render_image_height*aa_factor));

    // Render image
    R2Image *image = RenderImage(aa, render_image_width, render_image_height);

    // Cleanup Photon Map Memory
    for (int i = 0; i < GLOBAL_PHOTONS.NEntries(); i++) {
      delete GLOBAL_PHOTONS[i];
    }
    for (int i = 0; i < CAUSTIC_PHOTONS.NEntries(); i++) {
      delete CAUSTIC_PHOTONS[i];
    }
    if (GLOBAL_PMAP) {
      delete GLOBAL_PMAP;
    }
    if (CAUSTIC_PMAP) {
      delete CAUSTIC_PMAP;
    }

    // Error Check
    if (!image) exit(-1);

    // Write image
    if (!WriteImage(image, output_image_name)) exit(-1);

    // Delete image
    delete image;
  }

  // Return success
  return 0;
}
