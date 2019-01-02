// Source file for photonmap renderer
// This implementation is a simple raycaster.
// Replace it with your own code.



////////////////////////////////////////////////////////////////////////
// Include files
////////////////////////////////////////////////////////////////////////

#include "R3Graphics/R3Graphics.h"



////////////////////////////////////////////////////////////////////////
// Function to render image with photon mapping
////////////////////////////////////////////////////////////////////////

R2Image *
RenderImage(R3Scene *scene, 
  int width, int height, 
  int print_verbose)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();
  int ray_count = 0;

  // Allocate image
  R2Image *image = new R2Image(width, height);
  if (!image) {
    fprintf(stderr, "Unable to allocate image\n");
    return NULL;
  }

  // Convenient variables
  const R3Point& eye = scene->Camera().Origin();
  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;

  // Draw intersection point and normal for some rays
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      R3Ray ray = scene->Viewer().WorldRay(i, j);
      if (scene->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
        // Get intersection information
        const R3Material *material = (element) ? element->Material() : &R3default_material;
        const R3Brdf *brdf = (material) ? material->Brdf() : &R3default_brdf;

        // Compute color
        RNRgb color = scene->Ambient();
        if (brdf) {
          color += brdf->Emission();
          for (int k = 0; k < scene->NLights(); k++) {
            R3Light *light = scene->Light(k);
            color += light->Reflection(*brdf, eye, point, normal);
          }
        }

        // Set pixel color
        image->SetPixelRGB(i, j, color);

        // Update ray count
        ray_count++;
      }
    }
  }

  // Print statistics
  if (print_verbose) {
    printf("Rendered image ...\n");
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Rays = %d\n", ray_count);
    fflush(stdout);
  }

  // Return image
  return image;
}



