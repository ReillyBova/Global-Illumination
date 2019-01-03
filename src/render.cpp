////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "render.h"
#include "raytracer.h"
#include "utils/io_utils.h"
#include "utils/graphics_utils.h"
#include "R3Graphics/R3Graphics.h"
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <functional>

using namespace std;

////////////////////////////////////////////////////////////////////////
// File variables/constants
////////////////////////////////////////////////////////////////////////

// Progress bar parameters
static atomic_int bars_completed (0);

// Ray counts (thread local)
__thread unsigned long long int LOCAL_RAY_COUNT = 0;
__thread unsigned long long int LOCAL_SHADOW_RAY_COUNT = 0;
__thread unsigned long long int LOCAL_MONTE_RAY_COUNT = 0;
__thread unsigned long long int LOCAL_TRANSMISSIVE_RAY_COUNT = 0;
__thread unsigned long long int LOCAL_SPECULAR_RAY_COUNT = 0;
__thread unsigned long long int LOCAL_INDIRECT_RAY_COUNT = 0;
__thread unsigned long long int LOCAL_CAUSTIC_RAY_COUNT = 0;

// Ray counts (global)
static atomic_ullong ray_count (0);
static atomic_ullong shadow_ray_count (0);
static atomic_ullong monte_ray_count (0);
static atomic_ullong transmissive_ray_count (0);
static atomic_ullong specular_ray_count (0);
static atomic_ullong indirect_ray_count (0);
static atomic_ullong caustic_ray_count (0);

////////////////////////////////////////////////////////////////////////
// Main Rendering Methods
////////////////////////////////////////////////////////////////////////

// Threadable (parallelizable) ray tracing method
static void Threadable_RayTracer(vector<vector<RNRgb> >& image_buffer, int width,
  int height, const R3Point& eye, int id)
{
  RNInitThreadRandomness();
  // Useful values
  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;
  RNRgb color;

  // For progress bar printing
  int last_value = -1;

  // Draw intersection point and normal for some rays
  for (int i = 0; i < width; i++) {
    if (id == 0 && !(i % 2)) {
      double progress = ((double) bars_completed.load()) / width;
      int next_value = int(progress * 100.0);
      if (next_value != last_value) {
        PrintProgress(progress, PROGRESS_BAR_WIDTH);
        last_value = next_value;
      }
    }
    // Each thread does 1/THREADS work
    if (i % THREADS != id) continue;

    for (int j = 0; j < height; j++) {
      R3Ray ray = SCENE->Viewer().WorldRay(i, j);
      if (SCENE->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
        color = RNblack_rgb;
        // Call Raytracer on ray
        RayTrace(element, point, normal, ray, eye, color);

        // Set pixel color
        image_buffer[i][j] = color;

        // Update ray count
        LOCAL_RAY_COUNT++;
      } else {
        image_buffer[i][j] = SCENE->Background();
      }
    }

    bars_completed += 1;
  }

  // Update total ray counts (done at once for speed bc atomic operations are slow)
  ray_count += LOCAL_RAY_COUNT;
  shadow_ray_count += LOCAL_SHADOW_RAY_COUNT;
  monte_ray_count += LOCAL_MONTE_RAY_COUNT;
  transmissive_ray_count += LOCAL_TRANSMISSIVE_RAY_COUNT;
  specular_ray_count += LOCAL_SPECULAR_RAY_COUNT;
  indirect_ray_count += LOCAL_INDIRECT_RAY_COUNT;
  caustic_ray_count += LOCAL_CAUSTIC_RAY_COUNT;

  RNClearThreadRandomness();
}

// Multithreaded function that initializes image raytracing and handles
// anti aliasing. Returns rendered scene as image.
R2Image * RenderImage(int aa, int width, int height)
{
  if (!SCENE) {
    fprintf(stderr, "Renderer requires a scene\n");
    return NULL;
  }

  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Allocate image
  R2Image *image = new R2Image(width, height);
  if (!image) {
    fprintf(stderr, "Unable to allocate image\n");
    return NULL;
  }

  // Anti-aliasing
  int aa_factor = pow(2.0, aa);
  RNScalar axis_scale = 1.0 / aa_factor;
  RNScalar box_weight = 1.0 / aa_factor / aa_factor;
  int scaled_width = width*aa_factor;
  int scaled_height = height*aa_factor;

  if (VERBOSE) {
    printf("Rendering image ...\n");
  }

  // Split off into child threads
  vector<vector<RNRgb> > image_buffer(scaled_width,
         vector<RNRgb> (scaled_height, RNblack_rgb));
  const R3Point& eye = SCENE->Camera().Origin();
  thread *children = new thread[THREADS - 1];
  for (int i = 0; i < THREADS - 1; ++i) {
    children[i] = thread(Threadable_RayTracer, ref(image_buffer), scaled_width, scaled_height, eye, i+1);
  }

  // Use the main thread as well
  Threadable_RayTracer(image_buffer, scaled_width, scaled_height, eye, 0);

  // Join children threads
  for (int i = 0; i < THREADS - 1; i++)
    children[i].join();
  delete [] children;

  PrintProgress(1.0, PROGRESS_BAR_WIDTH);
  cout << endl;

  // Copy to image
  vector<vector<RNRgb> > down_sample_buffer(width,
         vector<RNRgb> (height, RNblack_rgb));
  for (int j = 0; j < scaled_height; j++) {
    for (int i = 0; i < scaled_width; i++) {
      int u = int(i * axis_scale);
      int v = int(j * axis_scale);
      RNRgb color = image_buffer[i][j];
      ClampColor(color);
      down_sample_buffer[u][v] += color;
    }
  }
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      RNRgb color = box_weight*down_sample_buffer[i][j];
      image->SetPixelRGB(i, j, color);
    }
  }

  // Print statistics
  if (VERBOSE) {
    unsigned long long int total_ray_count = ray_count.load();
    printf("Rendered image ...\n");
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Screen Rays = %llu\n", ray_count.load());
    if (SHADOWS) {
      printf("  # Shadow Rays = %llu\n", shadow_ray_count.load());
      total_ray_count += shadow_ray_count.load();
    }
    if (MONTE_CARLO) {
      printf("  # Monte Carlo Rays = %llu\n", monte_ray_count.load());
      total_ray_count += monte_ray_count.load();
    }
    if (TRANSMISSIVE_ILLUM) {
      printf("  # Transmissive Samples = %llu\n", transmissive_ray_count.load());
      total_ray_count += transmissive_ray_count.load();
    }
    if (SPECULAR_ILLUM) {
      printf("  # Specular Samples = %llu\n", specular_ray_count.load());
      total_ray_count += specular_ray_count.load();
    }
    if (INDIRECT_ILLUM) {
      printf("  # Indirect Samples = %llu\n", indirect_ray_count.load());
      total_ray_count += indirect_ray_count.load();
    }
    if (CAUSTIC_ILLUM) {
      printf("  # Caustic Samples = %llu\n", caustic_ray_count.load());
      total_ray_count += caustic_ray_count.load();
    }
    printf("Total Rays: %llu\n", total_ray_count);
    fflush(stdout);
  }

  // Return image
  return image;
}
