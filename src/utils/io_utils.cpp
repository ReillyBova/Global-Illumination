////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "io_utils.h"
#include "../R3Graphics/R3Graphics.h"
#include "../render.h"
#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Program argument parsing
////////////////////////////////////////////////////////////////////////

int ParseArgs(int argc, char **argv, char*& input_scene_name,
  char*& output_image_name, int& width, int& height, int& aa, bool& real_material)
{
  // Parse arguments
  argc--; argv++;
  while (argc > 0) {
    if ((*argv)[0] == '-') {
      // Misc Args
      if (!strcmp(*argv, "-v")) {
        VERBOSE = 1;
      } else if (!strcmp(*argv, "-threads")) {
        argc--; argv++; THREADS = atof(*argv);
        if (THREADS <= 0)
          THREADS = 1;
      } else if (!strcmp(*argv, "-aa")) {
        argc--; argv++; aa = atoi(*argv);
        if (aa < 0)
          aa *= -1;
      } else if (!strcmp(*argv, "-real")) {
        real_material = true;
      } else if (!strcmp(*argv, "-no_fresnel")) {
        FRESNEL = false;
      } else if (!strcmp(*argv, "-ir")) {
        argc--; argv++; IR_AIR = atof(*argv);
        if (IR_AIR <= 0)
          IR_AIR = RN_EPSILON;
      }
      // Render Equation Toggles
      else if (!strcmp(*argv, "-no_ambient")) {
        AMBIENT = false;
      } else if (!strcmp(*argv, "-no_direct")) {
        DIRECT_ILLUM = false;
      } else if (!strcmp(*argv, "-no_transmissive")) {
        TRANSMISSIVE_ILLUM = false;
      } else if (!strcmp(*argv, "-no_specular")) {
        SPECULAR_ILLUM = false;
      } else if (!strcmp(*argv, "-no_indirect")) {
        INDIRECT_ILLUM = false;
      } else if (!strcmp(*argv, "-no_caustic")) {
        CAUSTIC_ILLUM = false;
      } else if (!strcmp(*argv, "-photon_viz")) {
        DIRECT_PHOTON_ILLUM = true;
      } else if (!strcmp(*argv, "-fast_global")) {
        FAST_GLOBAL = true;
        DIRECT_PHOTON_ILLUM = true; // Needs to be on for fast_global to work
      }
      // Monte Carlo Toggles
      else if (!strcmp(*argv, "-no_monte")) {
        MONTE_CARLO = false;
      } else if (!strcmp(*argv, "-md")) {
        argc--; argv++; MAX_MONTE_DEPTH = atoi(*argv);
        if (MAX_MONTE_DEPTH < 1)
          MAX_MONTE_DEPTH = 1;
      } else if (!strcmp(*argv, "-absorb")) {
        argc--; argv++; PROB_ABSORB = atof(*argv);
        if (PROB_ABSORB < 0)
          PROB_ABSORB = 0;
      } else if (!strcmp(*argv, "-no_rs")) {
        RECURSIVE_SHADOWS = false;
      } else if (!strcmp(*argv, "-no_dt")) {
        DISTRIB_TRANSMISSIVE = false;
      } else if (!strcmp(*argv, "-tt")) {
        argc--; argv++; TRANSMISSIVE_TEST = atoi(*argv);
        if (TRANSMISSIVE_TEST < 1)
          TRANSMISSIVE_TEST = 1;
      } else if (!strcmp(*argv, "-no_ds")) {
        DISTRIB_SPECULAR = false;
      } else if (!strcmp(*argv, "-st")) {
        argc--; argv++; SPECULAR_TEST = atoi(*argv);
        if (SPECULAR_TEST < 1)
          SPECULAR_TEST = 1;
      }
      // Photons
      else if (!strcmp(*argv, "-global")) {
        argc--; argv++; GLOBAL_PHOTON_COUNT = atoi(*argv);
        if (GLOBAL_PHOTON_COUNT < 1)
          GLOBAL_PHOTON_COUNT = 1;
      } else if (!strcmp(*argv, "-caustic")) {
        argc--; argv++; CAUSTIC_PHOTON_COUNT = atoi(*argv);
        if (CAUSTIC_PHOTON_COUNT < 1)
          CAUSTIC_PHOTON_COUNT = 1;
      } else if (!strcmp(*argv, "-pd")) {
        argc--; argv++; MAX_PHOTON_DEPTH = atoi(*argv);
        if (MAX_PHOTON_DEPTH < 1)
          MAX_PHOTON_DEPTH = 1;
      } else if (!strcmp(*argv, "-it")) {
        argc--; argv++; INDIRECT_TEST = atoi(*argv);
        if (INDIRECT_TEST < 1)
          INDIRECT_TEST = 1;
      } else if (!strcmp(*argv, "-gs")) {
        argc--; argv++; GLOBAL_ESTIMATE_SIZE = atoi(*argv);
        if (GLOBAL_ESTIMATE_SIZE < 1)
          GLOBAL_ESTIMATE_SIZE = 1;
      } else if (!strcmp(*argv, "-gd")) {
        argc--; argv++; GLOBAL_ESTIMATE_DIST = atof(*argv);
        if (GLOBAL_ESTIMATE_DIST < 0.0)
          GLOBAL_ESTIMATE_DIST = RN_EPSILON;
      } else if (!strcmp(*argv, "-cs")) {
        argc--; argv++; CAUSTIC_ESTIMATE_SIZE = atoi(*argv);
        if (CAUSTIC_ESTIMATE_SIZE < 1)
          CAUSTIC_ESTIMATE_SIZE = 1;
      } else if (!strcmp(*argv, "-cd")) {
        argc--; argv++; CAUSTIC_ESTIMATE_DIST = atof(*argv);
        if (CAUSTIC_ESTIMATE_DIST < 0.0)
          CAUSTIC_ESTIMATE_DIST = RN_EPSILON;
      }
      // Shadows
      else if (!strcmp(*argv, "-no_shadow")) {
        SHADOWS = false;
      } else if (!strcmp(*argv, "-no_ss")) {
        SOFT_SHADOWS = false;
      } else if (!strcmp(*argv, "-lt")) {
        argc--; argv++; LIGHT_TEST = atoi(*argv);
        if (LIGHT_TEST < 1)
          LIGHT_TEST = 1;
      } else if (!strcmp(*argv, "-ss")) {
        argc--; argv++; SHADOW_TEST = atoi(*argv);
        if (SHADOW_TEST < 0)
          SHADOW_TEST = 0;
      }
      // Image Resolution
      else if (!strcmp(*argv, "-resolution")) {
        argc--; argv++; width = atoi(*argv);
        argc--; argv++; height = atoi(*argv);
        if (width < 0)
          width *= -1;
        if (height < 0)
          height *= -1;
      } else {
        fprintf(stderr, "Invalid program argument: %s", *argv);
        exit(1);
      }
      argv++; argc--;
    } else {
      if (!input_scene_name) input_scene_name = *argv;
      else if (!output_image_name) output_image_name = *argv;
      else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
      argv++; argc--;
    }
  }

  // Check scene filename
  if (!input_scene_name || !output_scene_name) {
    fprintf(stderr, "Usage: photonmap inputscenefile outputimagefile [-FLAGS]\n");
    return 0;
  }

  // Return OK status
  return 1;
}

////////////////////////////////////////////////////////////////////////
// Input
////////////////////////////////////////////////////////////////////////

// Read scene from file
R3Scene * ReadScene(char *filename, bool real_material)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Allocate scene
  R3Scene *scene = new R3Scene();

  if (!scene) {
    fprintf(stderr, "Unable to allocate scene for %s\n", filename);
    return NULL;
  }

  // Read scene from file
  if (!scene->ReadFile(filename, real_material)) {
    delete scene;
    return NULL;
  }

  // Print statistics
  if (VERBOSE) {
    printf("Read scene from %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Nodes = %d\n", scene->NNodes());
    printf("  # Lights = %d\n", scene->NLights());
    fflush(stdout);
  }

  // Return scene
  return scene;
}

////////////////////////////////////////////////////////////////////////
// Ouput
////////////////////////////////////////////////////////////////////////

// Print progress to stdout (From https://stackoverflow.com/q/14539867)
void PrintProgress(const double progress, const int width)
{
  cout << "[";
  int pos = width * progress;
  for (int j = 0; j < width; j++) {
    if (j < pos) cout << "=";
    else if (j == pos) cout << ">";
    else cout << " ";
  }
  cout << "] " << int(progress * 100.0) << "%\r";
  cout.flush();
}

// Write image to file
int WriteImage(R2Image *image, const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Write image to file
  if (!image->Write(filename)) return 0;

  // Print statistics
  if (VERBOSE) {
    printf("Wrote image to %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  Width = %d\n", image->Width());
    printf("  Height = %d\n", image->Height());
    fflush(stdout);
  }

  // Return success
  return 1;
}
