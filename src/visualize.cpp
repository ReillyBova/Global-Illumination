// Source file for the scene viewer program

// Include files

#include "R3Graphics/R3Graphics.h"
#include "fglut/fglut.h"


// Program variables

static char *input_scene_name = NULL;
static char *output_image_name = NULL;
static char *screenshot_image_name = NULL;
static int render_image_width = 64;
static int render_image_height = 64;
static int print_verbose = 0;

// GLUT variables

static int GLUTwindow = 0;
static int GLUTwindow_height = 900;
static int GLUTwindow_width = 900;
static int GLUTmouse[2] = { 0, 0 };
static int GLUTbutton[3] = { 0, 0, 0 };
static int GLUTmouse_drag = 0;
static int GLUTmodifiers = 0;



// Application variables

static R3Viewer *viewer = NULL;
static R3Scene *scene = NULL;
static R3Point center(0, 0, 0);



// Display variables

static int show_shapes = 1;
static int show_camera = 0;
static int show_lights = 0;
static int show_bboxes = 0;
static int show_rays = 0;
static int show_paths = 0;
static int show_emit = 0;
static int show_global = 0;
static int show_caustic = 0;
static int show_frame_rate = 0;


static bool INDIRECT_ILLUM = false;
static bool CAUSTIC_ILLUM = false;
static int GLOBAL_PHOTON_COUNT = 1920; // Number of photons emmitted for global map
static int CAUSTIC_PHOTON_COUNT = 3000; // Number of photons emmited for caustic map
static int MAX_PHOTON_DEPTH = 10;

static int PHOTONS_STORED_COUNT = 0;
static int TEMPORARY_STORAGE_COUNT = 0;

// Photon data structure
struct Photon {
  R3Point position; // Position
  unsigned char rgbe[4];     // compressed RGB values
  unsigned short direction;  // compressed incident direction
  unsigned short reflection;  // compressed REFLECTION direction
};

// Memory for photons
static RNArray <Photon *> GLOBAL_PHOTONS;
static RNArray <Photon *> CAUSTIC_PHOTONS;

// Memory for emitted photons
static RNArray <Photon *> PHOTONS_EMITTED;

// Lookup tables for incident direction
static RNScalar PHOTON_X_LOOKUP[65536];
static RNScalar PHOTON_Y_LOOKUP[65536];
static RNScalar PHOTON_Z_LOOKUP[65536];

static int global_emitted_count  = 0;
static int caustic_emitted_count = 0;

static int SIZE_LOCAL_PHOTON_STORAGE = 100000;

enum Photon_Type {GLOBAL, CAUSTIC};

////////////////////////////////////////////////////////////////////////
// Draw functions
////////////////////////////////////////////////////////////////////////

static void
LoadLights(R3Scene *scene)
{
  // Load ambient light
  static GLfloat ambient[4];
  ambient[0] = scene->Ambient().R();
  ambient[1] = scene->Ambient().G();
  ambient[2] = scene->Ambient().B();
  ambient[3] = 1;
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

  // Load scene lights
  for (int i = 0; i < scene->NLights(); i++) {
    R3Light *light = scene->Light(i);
    light->Draw(i);
  }
}



#if 0

static void
DrawText(const R3Point& p, const char *s)
{
  // Draw text string s and position p
  glRasterPos3d(p[0], p[1], p[2]);
  while (*s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *(s++));
}

#endif



static void
DrawText(const R2Point& p, const char *s)
{
  // Draw text string s and position p
  R3Ray ray = viewer->WorldRay((int) p[0], (int) p[1]);
  R3Point position = ray.Point(2 * viewer->Camera().Near());
  glRasterPos3d(position[0], position[1], position[2]);
  while (*s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *(s++));
}



static void
DrawCamera(R3Scene *scene)
{
  // Draw view frustum
  const R3Camera& camera = scene->Camera();
  R3Point eye = camera.Origin();
  R3Vector towards = camera.Towards();
  R3Vector up = camera.Up();
  R3Vector right = camera.Right();
  RNAngle xfov = camera.XFOV();
  RNAngle yfov = camera.YFOV();
  double radius = scene->BBox().DiagonalRadius();
  R3Point org = eye + towards * radius;
  R3Vector dx = right * radius * tan(xfov);
  R3Vector dy = up * radius * tan(yfov);
  R3Point ur = org + dx + dy;
  R3Point lr = org + dx - dy;
  R3Point ul = org - dx + dy;
  R3Point ll = org - dx - dy;
  glBegin(GL_LINE_LOOP);
  glVertex3d(ur[0], ur[1], ur[2]);
  glVertex3d(ul[0], ul[1], ul[2]);
  glVertex3d(ll[0], ll[1], ll[2]);
  glVertex3d(lr[0], lr[1], lr[2]);
  glVertex3d(ur[0], ur[1], ur[2]);
  glVertex3d(eye[0], eye[1], eye[2]);
  glVertex3d(lr[0], lr[1], lr[2]);
  glVertex3d(ll[0], ll[1], ll[2]);
  glVertex3d(eye[0], eye[1], eye[2]);
  glVertex3d(ul[0], ul[1], ul[2]);
  glEnd();
}



static void
DrawLights(R3Scene *scene)
{
  // Draw all lights
  double radius = scene->BBox().DiagonalRadius();
  for (int i = 0; i < scene->NLights(); i++) {
    R3Light *light = scene->Light(i);
    RNLoadRgb(light->Color());
    if (light->ClassID() == R3DirectionalLight::CLASS_ID()) {
      R3DirectionalLight *directional_light = (R3DirectionalLight *) light;
      R3Vector direction = directional_light->Direction();

      // Draw direction vector
      glBegin(GL_LINES);
      R3Point centroid = scene->BBox().Centroid();
      R3LoadPoint(centroid - radius * direction);
      R3LoadPoint(centroid - 1.25 * radius * direction);
      glEnd();
    }
    else if (light->ClassID() == R3PointLight::CLASS_ID()) {
      // Draw sphere at point light position
      R3PointLight *point_light = (R3PointLight *) light;
      R3Point position = point_light->Position();

     // Draw sphere at light position
       R3Sphere(position, 0.1 * radius).Draw();
    }
    else if (light->ClassID() == R3SpotLight::CLASS_ID()) {
      R3SpotLight *spot_light = (R3SpotLight *) light;
      R3Point position = spot_light->Position();
      R3Vector direction = spot_light->Direction();

      // Draw sphere at light position
      R3Sphere(position, 0.1 * radius).Draw();

      // Draw direction vector
      glBegin(GL_LINES);
      R3LoadPoint(position);
      R3LoadPoint(position + 0.25 * radius * direction);
      glEnd();
    }
    else {
      fprintf(stderr, "Unrecognized light type: %d\n", light->ClassID());
      return;
    }
  }
}



static void
DrawShapes(R3Scene *scene, R3SceneNode *node, RNFlags draw_flags = R3_DEFAULT_DRAW_FLAGS)
{
  // Push transformation
  node->Transformation().Push();

  // Draw elements
  for (int i = 0; i < node->NElements(); i++) {
    R3SceneElement *element = node->Element(i);
    element->Draw(draw_flags);
  }

  // Draw children
  for (int i = 0; i < node->NChildren(); i++) {
    R3SceneNode *child = node->Child(i);
    DrawShapes(scene, child, draw_flags);
  }

  // Pop transformation
  node->Transformation().Pop();
}



static void
DrawBBoxes(R3Scene *scene, R3SceneNode *node)
{
  // Draw node bounding box
  node->BBox().Outline();

  // Push transformation
  node->Transformation().Push();

  // Draw children bboxes
  for (int i = 0; i < node->NChildren(); i++) {
    R3SceneNode *child = node->Child(i);
    DrawBBoxes(scene, child);
  }

  // Pop transformation
  node->Transformation().Pop();
}



static void
DrawRays(R3Scene *scene)
{
  // Ray intersection variables
  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;

  // Ray generation variables
  int istep = scene->Viewport().Width() / 20;
  int jstep = scene->Viewport().Height() / 20;
  if (istep == 0) istep = 1;
  if (jstep == 0) jstep = 1;

  // Ray drawing variables
  double radius = 0.025 * scene->BBox().DiagonalRadius();

  // Draw intersection point and normal for some rays
  for (int i = istep/2; i < scene->Viewport().Width(); i += istep) {
    for (int j = jstep/2; j < scene->Viewport().Height(); j += jstep) {
      R3Ray ray = scene->Viewer().WorldRay(i, j);
      if (scene->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
        R3Sphere(point, radius).Draw();
        R3Span(point, point + 2 * radius * normal).Draw();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// Geometry and Graphics Functions
////////////////////////////////////////////////////////////////////////

// Use Schlick's approximation to return reflection coefficient between media
RNScalar ComputeReflectionCoeff(RNScalar cos_theta, const RNScalar ir_mat)
{
  // Parallel Light (direction agnostic)
  const RNScalar R_o = pow((1.0 - ir_mat)/(1.0 + ir_mat), 2);
  // Schlick's approximation (need to flip normal if theta is greater than pi)
  return (R_o + (1.0 - R_o) * pow((1.0 -  abs(cos_theta)), 5));
}

// Return the direction of a perfect reflective bounce
R3Vector ReflectiveBounce(R3Vector normal, R3Vector& view, RNScalar cos_theta)
{
  // Flip normal to incident side of surface
  if (cos_theta < 0) {
    normal.Flip();
    cos_theta *= -1.0;
  }

  // Find perpendicular component (recall cos_theta is defined from flip of view)
  R3Vector view_flipped_perp = normal * cos_theta;
  R3Vector view_reflection = view + view_flipped_perp*2.0;
  view_reflection.Normalize();
  return view_reflection;
}

// Return the direction of a perfect transmissive bounce (may return reflection
// if beyond critical angle)
R3Vector TransmissiveBounce(R3Vector normal, R3Vector& view, RNScalar cos_theta,
  const RNScalar ir_mat)
{
  // Compute ir ratio according to whether we are entering or leaving an object
  RNScalar eta;
  if (cos_theta < 0) {
    // Leaving
    eta = ir_mat / 1.0;
    // Need to flip normal (NB: not copy be reference so safe to overwrite)
    normal.Flip();
    cos_theta *= -1.0;
  } else {
    // Entering
    eta = 1.0 / ir_mat;
  }

  // Find relevant angles
  RNAngle theta = acos(cos_theta);
  RNScalar sin_phi = eta * sin(theta);

  // Check for total internal reflection
  if (sin_phi < -1.0 || 1.0 < sin_phi) {
    // Return reflection direction
    return ReflectiveBounce(normal, view, cos_theta);
  }

  // Otherwise return refraction direction
  RNAngle phi = asin(sin_phi);
  R3Vector view_parallel = view + normal*cos_theta;
  view_parallel.Normalize();
  R3Vector view_refraction = view_parallel*tan(phi) - normal;
  view_refraction.Normalize();
  return view_refraction;
}


// Use importance sampling to return a vector sampled from a weighted hemisphere
// around the surface normal (normal is flipped if cos_theta is negative)
R3Vector Diffuse_ImportanceSample(R3Vector normal, const RNScalar cos_theta)
{
  // Check normal direction
  if (cos_theta < 0) {
    normal.Flip();
  }

  // Pick spherical coords
  const RNAngle theta = acos(sqrt(RNRandomScalar()));
  const RNAngle phi = 2*RN_PI*RNRandomScalar();

  // Build a vector with angle alpha relative to the normal direction
  R3Vector perpendicular_direction = R3Vector(normal[1], -normal[0], 0);
  if (1.0 - abs(normal[2]) < 0.1) {
    perpendicular_direction = R3Vector(normal[2], 0, -normal[0]);
  }
  perpendicular_direction.Normalize();
  R3Vector result = perpendicular_direction*sin(theta) + normal*cos(theta);

  // Rotate around axis by phi and normalize
  result.Rotate(normal, phi);
  result.Normalize();
  return result;
}

// Use importance sampling to return a vector offset from a perfect bounce by
// a random variable drawn from the brdf
R3Vector Specular_ImportanceSample(const R3Vector& exact, const RNScalar n,
  const RNScalar cos_theta)
{
  // Get the max value for alpha (becomes increasingly small as cos theta shrinks
  // to prevent the reflection from penetrating the surface; mimics real behavior
  // of increased specularity sharpness near grazing angles)
  // Computed from: 2/pi * (angle between surface and exact) = 2/pi * (pi/2 - acos(N*V))
  //                = 1 - 2/pi * cos(|cos_theta|)
  const RNScalar angle_limit = (1.0 - acos(abs(cos_theta)) * 2.0 / RN_PI);

  // Find axis perturbation values from brdf (see Lafortune & Williams, 1994)
  const RNAngle alpha = acos(pow(RNRandomScalar(), 1.0 / (n + 1.0))) * angle_limit;

  const RNAngle phi = RN_TWO_PI*RNRandomScalar();

  // Build a vector with angle alpha relative to the exact direction
  R3Vector perpendicular_direction = R3Vector(exact[1], -exact[0], 0);
  if (1.0 - abs(exact[2]) < 0.1) {
    perpendicular_direction = R3Vector(exact[2], 0, -exact[0]);
  }
  perpendicular_direction.Normalize();
  R3Vector result = perpendicular_direction*sin(alpha) + exact*cos(alpha);

  // Rotate around axis by phi and normalize
  result.Rotate(exact, phi);
  result.Normalize();
  return result;
}

// Return the max value across all RGB color channels
RNScalar MaxChannelVal(const RNRgb &color)
{
 RNScalar max = 0;
 for (int i = 0; i < 3; i++) {
   if (color[i] > max)
     max = color[i];
 }
 return max;
}

void MonteCarlo_PathTrace(R3Ray& ray, const RNScalar R, const RNScalar G, const RNScalar B)
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
  RNScalar prob_diffuse;
  RNScalar prob_transmission;
  RNScalar prob_specular;
  RNScalar prob_terminate;
  RNScalar prob_total;
  RNScalar rand;

  // Bouncing geometries
  R3Vector exact_bounce;
  R3Vector sampled_bounce;

  for (int iter = 0; iter < 10 &&
    scene->Intersects(ray, NULL, &element, NULL, &point, &normal, NULL); iter++)
  {
    // Draw
    glColor3d(1, 1, 1);
    R3Sphere(point, 0.01).Draw();
    glColor3d(R, G, B);
    R3Span(ray_start, point).Draw();

    // Get intersection information
    material = (element) ? element->Material() : &R3default_material;
    brdf = (material) ? material->Brdf() : &R3default_brdf;
    if (brdf) {
      // Useful geometric values to precompute
      view = (point - ray_start);
      view.Normalize();
      cos_theta = normal.Dot(-view);

      // Bounced sampling (monte carlo) ----------------------------------------

      // Compute Reflection Coefficient, carry reflection portion to Specular
      R_coeff = 0;
      if (brdf->IsTransparent()) {
        R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());
      }

      // Generate material probabilities of bounce type
      prob_diffuse = MaxChannelVal(brdf->Diffuse());
      prob_transmission = MaxChannelVal(brdf->Transmission());
      prob_specular = MaxChannelVal(brdf->Specular()) + R_coeff*prob_transmission;
      prob_transmission *= (1.0 - R_coeff);
      prob_terminate = MaxChannelVal(brdf->Emission());
      prob_total = prob_diffuse + prob_transmission + prob_specular + prob_terminate;

      // Scale down to 1.0 (but never scale up bc of implicit absorption)
      // NB: faster to scale rand up than to normalize; would also need to adjust
      //     brdf values when updating weights (dividing prob_total back out)
      rand = RNRandomScalar();
      if (prob_total > 1.0) {
        rand *= prob_total;
      }

      // Bounce and recur (choose one from distribution)
      if (rand < prob_diffuse) {
        glColor3d(1, 1, 1);
        R3Sphere(point, 0.01).Draw();
        break;
      } else if (rand < prob_diffuse + prob_transmission) {
        // Compute direction of transmissive bounce
        exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                          brdf->IndexOfRefraction());
        // Use importance sampling
        sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
      } else if (rand < prob_diffuse + prob_transmission + prob_specular) {
        // Compute direction of specular bounce
        exact_bounce = ReflectiveBounce(normal, view, cos_theta);
        // Use importance sampling
        sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);
      } else {
        // Photon absorbed; terminate trace
        glColor3d(1, 1, 1);
        R3Sphere(point, 0.01).Draw();
        break;
      }

      // Recur
      ray_start = point + sampled_bounce * RN_EPSILON;
      ray = R3Ray(ray_start, sampled_bounce, TRUE);
    }
  }
}

// Compute transmissive bounce on point
void DrawTransmissive(R3Point& point, R3Vector& normal, const RNScalar R,
  const RNScalar G, const RNScalar B, const R3Brdf *brdf, R3Vector& view,
  RNScalar cos_theta, RNScalar T_coeff)
{
  // Get direction of bounced ray (might be a specular if total internal reflection)
  const R3Vector exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                                  brdf->IndexOfRefraction());

  // Scale number of samples with contribution to final color of pixel
  RNRgb total_weight = T_coeff * (brdf->Transmission());
  RNScalar highest_weight = MaxChannelVal(total_weight);
  const int num_samples = ceil((5*highest_weight + 5) / 2.0);

  // Recursively raytrace the bounce using montecarlo path tracing
  R3Ray ray;
  R3Vector sampled_bounce;
  const RNScalar n = brdf->Shininess();
  for (int i = 0; i < num_samples; i++) {
    // Use importance sampling
    sampled_bounce = Specular_ImportanceSample(exact_bounce, n, cos_theta);
    ray = R3Ray(point + sampled_bounce * RN_EPSILON, sampled_bounce, TRUE);
    MonteCarlo_PathTrace(ray, R, G, B);
  }
}

// Compute specular bounce on point
void DrawSpecular(R3Point& point, R3Vector& normal,
  const RNScalar R, const RNScalar G, const RNScalar B, const R3Brdf *brdf,
  R3Vector& view, RNScalar cos_theta, RNScalar R_coeff)
{
  // Get direction of bounced ray
  const R3Vector exact_bounce = ReflectiveBounce(normal, view, cos_theta);

  // Scale number of samples with contribution to final color of pixel
  RNRgb total_weight = brdf->Transmission()*R_coeff + brdf->Specular();
  RNScalar highest_weight = MaxChannelVal(total_weight);
  const int num_samples = ceil((5*highest_weight + 5) / 2.0);

  // Recursively raytrace the bounce using montecarlo path tracing
  R3Ray ray;
  R3Vector sampled_bounce;
  const RNScalar n = brdf->Shininess();
  for (int i = 0; i < num_samples; i++) {
    // Use importance sampling
    sampled_bounce = Specular_ImportanceSample(exact_bounce, n, cos_theta);
    ray = R3Ray(point + sampled_bounce * RN_EPSILON, sampled_bounce, TRUE);
    MonteCarlo_PathTrace(ray, R, G, B);
  }
}

////////////////////////////////////////////////////////////////////////
// Path Visualizer
////////////////////////////////////////////////////////////////////////

static void
DrawPaths(R3Scene *scene)
{
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  R3Point camera = scene->Camera().Origin();
  double radius = 0.025;
  R3Sphere(camera, radius).Draw();
  radius = 0.01;

  int istep = scene->Viewport().Width() / 25;
  int jstep = scene->Viewport().Height() / 25;
  if (istep == 0) istep = 1;
  if (jstep == 0) jstep = 1;

  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;
  int W = scene->Viewport().Width();
  int H = scene->Viewport().Height();
  for (int i = 0; i < W; i += istep) {
    const RNScalar R = ((RNScalar) i) / W;
    for (int j = 0; j < H; j += jstep) {
      const RNScalar B = ((RNScalar) j) / H;
      const RNScalar G = ((RNScalar) j * i) / (H * W);

      R3Ray ray = scene->Viewer().WorldRay(i, j);
      if (scene->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
        const R3Material *material = (element) ? element->Material() : &R3default_material;
        const R3Brdf *brdf = (material) ? material->Brdf() : &R3default_brdf;

        if (brdf) {
          // Draw 1/4 times if it is absorbed
          if (!brdf->IsTransparent() && !brdf->IsSpecular()) {
            continue;
          }
          glColor3d(1, 1, 1);
          R3Sphere(point, radius).Draw();
          glColor3d(R, G, B);
          R3Span(camera, point).Draw();
          R3Point prev = camera;

          // Useful geometric values to precompute
          R3Vector view = (point - camera);
          view.Normalize();
          RNScalar cos_theta = normal.Dot(-view);

          // Fresnel Reflection Coefficient for transmission (approximated)
          RNScalar R_coeff = 0;
          if (brdf->IsTransparent()) {
            // Compute Reflection Coefficient, carry reflection portion to Specular
            R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());

            // Compute contribution from transmission
            if (R_coeff < 1.0) {
              DrawTransmissive(point, normal, R, G, B, brdf, view, cos_theta,
                1.0 - R_coeff);
            }
          }
          if (brdf->IsSpecular() || R_coeff > 0) {
            // Compute contribution from transmission
            DrawSpecular(point, normal, R, G, B, brdf, view, cos_theta,
              R_coeff);
          }
        }
      }
    }
  }
  glLineWidth(1);
}

////////////////////////////////////////////////////////////////////////
// Photon Tools
////////////////////////////////////////////////////////////////////////

// Return total power of light (sum of RGB channels and scaled by area)
RNScalar LightPower(R3Light* light)
{
  // Flux into scene computation
  const RNRgb& color = light->Color();
  RNArea area = 1.0;
  // Flux through closed gaussian surface is 4pi
  RNScalar flux = 4.0 * RN_PI;
  if (light->ClassID() == R3DirectionalLight::CLASS_ID()) {
    // Area of Directional Light
    area = RN_PI*pow(scene->BBox().DiagonalRadius(), 2.0);
    flux = 1.0;
  } else if (light->ClassID() == R3AreaLight::CLASS_ID()) {
    // Area of Circular Light
    R3AreaLight *area_light = (R3AreaLight *) light;
    area = RN_PI*pow(area_light->Radius(), 2.0);
    // Flux through hemisphere is 2pi
    flux /= 2.0;
  } else if (light->ClassID() == R3RectLight::CLASS_ID()) {
    // Area of Parallelogram Light
    R3RectLight *rect_light = (R3RectLight *) light;
    R3Vector a1 = rect_light->PrimaryAxis() * rect_light->PrimaryLength();
    R3Vector a2 = rect_light->SecondaryAxis() * rect_light->SecondaryLength();
    area = (a1 % a2).Length();
    // Flux through hemisphere is 2pi
    flux /= 2.0;
  } else if (light->ClassID() == R3SpotLight::CLASS_ID()) {
    // Get Flux of cone (tricky!)
    R3SpotLight *spot_light = (R3SpotLight *) light;
    RNScalar s = spot_light->DropOffRate();
    RNAngle c = spot_light->CutOffAngle();
    flux = RN_TWO_PI / (s + 1.0) * (1.0 - pow(cos(c), s + 1.0));
  }

  // Return sum of color powers scaled by area
  return (color[0] + color[1] + color[2]) * area * flux;
}

// Normalize Color channels so they sum to 1
void NormalizeColor(RNRgb &color) {
  // Compute L1 magnitude
  RNScalar total = 0;
  for (int i = 0; i < 3; i++) {
    total += color[i];
  }

  if (total > 0) {
    color /= total;
  }
}

// Convert color from RGB to Ward's packed RGBE format; copies result into char[4]
// array
void RNRgb_to_RGBE(RNRgb& rgb_src, unsigned char* rgbe_target)
{
  RNScalar max = MaxChannelVal(rgb_src);

  // Corner case for black
  if (max < RN_EPSILON) {
    rgbe_target[0] = 0;
    rgbe_target[1] = 0;
    rgbe_target[2] = 0;
    rgbe_target[3] = 0;
    return;
  }

  int exponent;
  RNScalar mantissa = frexp(max, &exponent);
  rgbe_target[0] = (unsigned char) (256.0 * rgb_src[0]/max * mantissa);
  rgbe_target[1] = (unsigned char) (256.0 * rgb_src[1]/max * mantissa);
  rgbe_target[2] = (unsigned char) (256.0 * rgb_src[2]/max * mantissa);
  rgbe_target[3] = (unsigned char) (exponent + 128);
}

// Convert color from Ward's packed char[4] RGBE format to an RGB class
RNRgb RGBE_to_RNRgb(unsigned char* rgbe_src)
{
  // Corner case for black
  if (!rgbe_src[3]) {
    return RNblack_rgb;
  }

  // Find inverse
  RNScalar inverse = ldexp(1.0, ((int) rgbe_src[3]) - 128 - 8);

  // Copy, scale, and return
  RNRgb color = RNRgb(rgbe_src[0], rgbe_src[1], rgbe_src[2]);
  color *= inverse;
  return color;
}



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

// Flush local memory to global memory using dynamic memory
// primatives; returns 1 if successful, 0 otherwise
int FlushPhotonStorage(vector<Photon>& local_photon_storage, Photon_Type map_type)
{
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
  R3Vector& incident_vector, R3Vector& reflection_vector, R3Point& point, Photon_Type map_type)
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

  phi = (unsigned char) (255.0
                      * (atan2(reflection_vector[1], reflection_vector[0]) + RN_PI)
                      / (RN_TWO_PI));
  theta = (unsigned char) (255.0 * acos(reflection_vector[2]) / RN_PI);
  photon_target.reflection = phi*256 + theta;

  TEMPORARY_STORAGE_COUNT++;
  PHOTONS_STORED_COUNT++;
  return;
}

////////////////////////////////////////////////////////////////////////
// Photon Tracing Method
////////////////////////////////////////////////////////////////////////

// Monte carlo trace photon, storing at each diffuse intersection
void PhotonTrace(R3Ray ray, RNRgb photon, vector<Photon>& local_photon_storage,
  Photon_Type map_type)
{
  // Store emitted photons
  if (map_type == GLOBAL) {
    // Copy photon
    Photon *photon_emitted = new Photon();
    photon_emitted->position = ray.Start();
    RNRgb_to_RGBE(photon, photon_emitted->rgbe);
    R3Vector v = ray.Vector();
    int phi = (unsigned char) (255.0
                        * (atan2(v[1], v[0]) + RN_PI)
                        / (RN_TWO_PI));
    int theta = (unsigned char) (255.0 * acos(v[2]) / RN_PI);
    photon_emitted->direction = phi*256 + theta;
    PHOTONS_EMITTED.Insert(photon_emitted);
  }
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

  // Trace the photon through its bounces, storing it at diffuse surfaces
  int iter;
  for (iter = 0; iter < MAX_PHOTON_DEPTH &&
    scene->Intersects(ray, NULL, &element, NULL, &point, &normal, NULL); iter++)
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
      if (brdf->IsDiffuse() && (iter > 0 || (map_type == GLOBAL))) {
        R3Vector reflection = ReflectiveBounce(normal, view, cos_theta);
        StorePhoton(photon, local_photon_storage, view, reflection, point, map_type);
      }

      // Compute Reflection Coefficient, carry reflection portion to Specular
      R_coeff = 0;
      if (brdf->IsTransparent()) {
        R_coeff = ComputeReflectionCoeff(cos_theta, brdf->IndexOfRefraction());
      }

      // Generate material probabilities of bounce type
      max_channel = MaxChannelVal(photon);
      prob_diffuse = MaxChannelVal(brdf->Diffuse() * photon) / max_channel;
      prob_transmission = MaxChannelVal(brdf->Transmission() * photon) / max_channel;
      prob_specular = (MaxChannelVal(brdf->Specular() * photon) / max_channel)
                        + R_coeff*prob_transmission;
      prob_transmission *= (1.0 - R_coeff);
      prob_terminate = 0.005;
      prob_total = prob_diffuse + prob_transmission + prob_specular + prob_terminate;

      // Scale down to 1.0 (but never scale up bc of implicit absorption)
      // NB: faster to scale rand up than to normalize; would also need to adjust
      //     brdf values when updating weights (dividing prob_total back out)
      rand = RNRandomScalar();
      if (prob_total > 1.0) {
        rand *= prob_total;
      }

      // Bounce and recur (choose one from distribution)
      if (rand < prob_diffuse) {
        // Terminate if caustic trace
        if (map_type == CAUSTIC) {
          break;
        }

        // Otherwise, compute direction of diffuse bounce
        sampled_bounce = Diffuse_ImportanceSample(normal, cos_theta);
        // Update weights
        photon *= brdf->Diffuse() / prob_diffuse;
      } else if (rand < prob_diffuse + prob_transmission) {
        // Compute direction of transmissive bounce
        exact_bounce = TransmissiveBounce(normal, view, cos_theta,
                                          brdf->IndexOfRefraction());
        // Use importance sampling
        sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);

        // Update weights
        photon *= brdf->Transmission() / prob_transmission;
      } else if (rand < prob_diffuse + prob_transmission + prob_specular) {
        // Compute direction of specular bounce
        exact_bounce = ReflectiveBounce(normal, view, cos_theta);
        // Use importance sampling
        sampled_bounce = Specular_ImportanceSample(exact_bounce, brdf->Shininess(), cos_theta);

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
}

////////////////////////////////////////////////////////////////////////
// Photon Emitting Method (invokes internal photon tracer)
////////////////////////////////////////////////////////////////////////

void EmitPhotons(int num_photons, R3Light* light, Photon_Type map_type)
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
    R3Point center = scene->Centroid() - light_norm * scene->BBox().DiagonalRadius() * 3.0;

    // Find two perpendicular vectors spanning plane of light
    R3Vector u = R3Vector(light_norm[1], -light_norm[0], 0);
    if (1.0 - abs(light_norm[2]) < 0.1) {
      u = R3Vector(light_norm[2], 0, -light_norm[0]);
    }
    R3Vector v = u % light_norm;
    u.Normalize();
    v.Normalize();

    // Scale u and v
    u *= scene->BBox().DiagonalRadius();
    v *= scene->BBox().DiagonalRadius();

    // General Forward Declarations
    R3Point sample_point;
    R3Ray ray;
    RNScalar r1, r2;

    // Emit photons
    for (int i = 0; i < num_photons; i++) {
      do {
        // Sample point in circle
        r1 = RNRandomScalar()*2.0 - 1.0;
        r2 = RNRandomScalar()*2.0 - 1.0;
      } while (r1*r1 + r2*r2 > 1.0);

      sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;
      ray = R3Ray(sample_point, light_norm, TRUE);
      PhotonTrace(ray, photon,local_photon_storage, map_type);
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
        x = RNRandomScalar()*2.0 - 1.0;
        y = RNRandomScalar()*2.0 - 1.0;
        z = RNRandomScalar()*2.0 - 1.0;
      } while (x*x + y*y + z*z > 1.0);

      sample_direction = R3Vector(x, y, z);
      sample_direction.Normalize();
      ray = R3Ray(center, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type);
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
      PhotonTrace(ray, photon, local_photon_storage, map_type);
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
        r1 = (RNRandomScalar() * 2.0) - 1.0;
        r2 = (RNRandomScalar() * 2.0) - 1.0;
      } while (r1*r1 + r2*r2 > 1.0);

      // Use values r1, r2 and vectors u, v to find a random point on light
      sample_point = r1*u + r2*v + center + light_norm*RN_EPSILON;

      // Use diffuse importance sampling to pick a direction
      sample_direction = Diffuse_ImportanceSample(light_norm, 1.0);

      ray = R3Ray(sample_point, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type);
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
      r1 = RNRandomScalar() - 0.5;
      r2 = RNRandomScalar() - 0.5;

      // Use values r1, r2 and vectors a1, a2 to find a random point on light
      sample_point = r1*a1 + r2*a2 + center + light_norm*RN_EPSILON;

      // Use diffuse importance sampling to pick a direction
      sample_direction = Diffuse_ImportanceSample(light_norm, 1.0);
      ray = R3Ray(sample_point, sample_direction, TRUE);
      PhotonTrace(ray, photon, local_photon_storage, map_type);
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

// photon tracing method caller
static void Threadable_PhotonTracer(const int num_global_photons,
  const int num_caustic_photons, vector<RNScalar> &light_powers, RNScalar total_power)
{
  // Global (Indirect) Illumination Photon mapping
  int local_global_emitted_count = 0;
  if (INDIRECT_ILLUM) {
    // Print Info
    if (print_verbose) {
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
      for (int i = 0; i < scene->NLights(); i++) {
        // Emit photons proportional to light contribution over total power, as well as the
        // assigned total number of photons to emit
        int num_photons = ceil(emit_goal * (light_powers[i] / total_power));
        EmitPhotons(num_photons, scene->Light(i), GLOBAL);
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
  }

  // Caustic Illumination Photon mapping
  int local_caustic_emitted_count = 0;
  if (CAUSTIC_ILLUM) {
    // Print Info
    if (print_verbose) {
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
      for (int i = 0; i < scene->NLights(); i++) {
        // Emit photons proportional to light contribution to total power and the
        // assigned total number of photons to emit
        int num_photons = ceil(emit_goal * (light_powers[i] / total_power));
        EmitPhotons(num_photons, scene->Light(i), CAUSTIC);
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
  }

  // Update counts
  global_emitted_count += local_global_emitted_count;
  caustic_emitted_count += local_caustic_emitted_count;
}

// method that populates the photon maps as necessary
static void MapPhotons(void)
{
  // Corner case
  if (scene->NLights() <= 0) {
    return;
  }

  // Start statistics
  RNTime total_start_time;
  total_start_time.Read();

  // Compute power distribution of lights
  vector<RNScalar> light_powers(scene->NLights(), 0.0);
  RNScalar total_power = 0;
  for (int i = 0; i < scene->NLights(); i++) {
    R3Light *light = scene->Light(i);
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

  // For global photons
  int global_photons_remaining = 0;
  if (INDIRECT_ILLUM) {
    // Compute how many photons to distribute to each thread
    global_photons_remaining = GLOBAL_PHOTON_COUNT;
  }

  // For caustic photons
  int caustic_photons_remaining = 0;
  if (CAUSTIC_ILLUM) {
    // Compute how many photons to distribute to each thread
    caustic_photons_remaining = CAUSTIC_PHOTON_COUNT;
  }

  Threadable_PhotonTracer(global_photons_remaining, caustic_photons_remaining,
                          light_powers, total_power);

  // Update globals based on sucess
  if ((INDIRECT_ILLUM) && GLOBAL_PHOTONS.NEntries() == 0) {
    INDIRECT_ILLUM = false;
  }
  if (CAUSTIC_ILLUM && CAUSTIC_PHOTONS.NEntries() == 0) {
    CAUSTIC_ILLUM = false;
  }

  // Print statistics
  if (print_verbose) {
    int total_photon_count = 0;
    printf("Built photon map ...\n");
    printf("  Total Time = %.2f seconds\n", total_start_time.Elapsed());
    if (INDIRECT_ILLUM) {
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
// Photon Viz
////////////////////////////////////////////////////////////////////////

static void DrawEmitted(void)
{
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  int len = PHOTONS_EMITTED.NEntries();
  double radius = 0.01;
  double norm_len = 50.0*radius;
  for (int i = 0; i < len; i++) {
    Photon p = *(PHOTONS_EMITTED[i]);
    // Get photon info
    RNRgb p_color = RGBE_to_RNRgb(p.rgbe);
    R3Point p_pos = p.position;
    int direction = p.direction;
    RNScalar x = PHOTON_X_LOOKUP[direction];
    RNScalar y = PHOTON_Y_LOOKUP[direction];
    RNScalar z = PHOTON_Z_LOOKUP[direction];
    R3Vector incident_vector = R3Vector(x, y ,z);

    // Draw
    glColor3d(p_color[0], p_color[1], p_color[2]);
    R3Sphere(p_pos, radius).Draw();
    R3Span(p_pos, norm_len*incident_vector + p_pos).Draw();
  }
  glLineWidth(1);
}


static void DrawStoredGlobal(void)
{
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  int len = GLOBAL_PHOTONS.NEntries();
  double radius = 0.01;
  double norm_len = 2.0*radius;
  for (int i = 0; i < len; i++) {
    Photon p = *(GLOBAL_PHOTONS[i]);
    // Get photon info
    RNRgb p_color = RGBE_to_RNRgb(p.rgbe);
    R3Point p_pos = p.position;
    int direction = p.direction;
    RNScalar x = PHOTON_X_LOOKUP[direction];
    RNScalar y = PHOTON_Y_LOOKUP[direction];
    RNScalar z = PHOTON_Z_LOOKUP[direction];
    R3Vector incident_vector = R3Vector(x, y ,z);

    int reflection = p.reflection;
    x = PHOTON_X_LOOKUP[reflection];
    y = PHOTON_Y_LOOKUP[reflection];
    z = PHOTON_Z_LOOKUP[reflection];
    R3Vector reflection_vector = R3Vector(x, y ,z);

    // Draw
    glColor3d(p_color[0], p_color[1], p_color[2]);
    R3Sphere(p_pos, radius).Draw();
    R3Span(p_pos, -norm_len*incident_vector + p_pos).Draw();
    R3Span(p_pos, 2*norm_len*reflection_vector + p_pos).Draw();
  }
  glLineWidth(1);
}


static void DrawStoredCaustic(void)
{
  glDisable(GL_LIGHTING);
  glLineWidth(1);
  int len = CAUSTIC_PHOTONS.NEntries();
  double radius = 0.01;
  double norm_len = 10.0*radius;
  for (int i = 0; i < len; i++) {
    Photon p = *(CAUSTIC_PHOTONS[i]);
    // Get photon info
    RNRgb p_color = RGBE_to_RNRgb(p.rgbe);
    R3Point p_pos = p.position;
    int direction = p.direction;
    RNScalar x = PHOTON_X_LOOKUP[direction];
    RNScalar y = PHOTON_Y_LOOKUP[direction];
    RNScalar z = PHOTON_Z_LOOKUP[direction];
    R3Vector incident_vector = R3Vector(x, y ,z);

    // Draw
    glColor3d(p_color[0], p_color[1], p_color[2]);
    R3Sphere(p_pos, radius).Draw();
    R3Span(p_pos, norm_len*incident_vector + p_pos).Draw();
  }
  glLineWidth(1);
}

////////////////////////////////////////////////////////////////////////
// Glut user interface functions
////////////////////////////////////////////////////////////////////////

void GLUTStop(void)
{
  // Destroy window
  glutDestroyWindow(GLUTwindow);

  // Exit
  exit(0);
}



void GLUTRedraw(void)
{
  // Check scene
  if (!scene) return;

  // Set viewing transformation
  viewer->Camera().Load();

  // Clear window
  RNRgb background = scene->Background();
  glClearColor(background.R(), background.G(), background.B(), 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Load lights
  LoadLights(scene);

  // Draw camera
  if (show_camera) {
    glDisable(GL_LIGHTING);
    glColor3d(1.0, 1.0, 1.0);
    glLineWidth(5);
    DrawCamera(scene);
    glLineWidth(1);
  }

  // Draw lights
  if (show_lights) {
    glDisable(GL_LIGHTING);
    glColor3d(1.0, 1.0, 1.0);
    glLineWidth(5);
    DrawLights(scene);
    glLineWidth(1);
  }

  // Draw rays
  if (show_rays) {
    glDisable(GL_LIGHTING);
    glColor3d(0.0, 1.0, 0.0);
    glLineWidth(3);
    DrawRays(scene);
    glLineWidth(1);
  }

  // Draw paths
  if (show_paths) {
    DrawPaths(scene);
  }

  // Draw emitted photons
  if (show_emit && INDIRECT_ILLUM) {
    DrawEmitted();
  }

  if (show_global && INDIRECT_ILLUM) {
    DrawStoredGlobal();
  }

  if (show_caustic && CAUSTIC_ILLUM) {
    DrawStoredCaustic();
  }

  // Draw scene nodes
  if (show_shapes) {
    glEnable(GL_LIGHTING);
    R3null_material.Draw();
    DrawShapes(scene, scene->Root());
    R3null_material.Draw();
  }

  // Draw bboxes
  if (show_bboxes) {
    glDisable(GL_LIGHTING);
    glColor3d(1.0, 0.0, 0.0);
    DrawBBoxes(scene, scene->Root());
  }

  // Draw frame time
  if (show_frame_rate) {
    char buffer[128];
    static RNTime last_time;
    double frame_time = last_time.Elapsed();
    last_time.Read();
    if ((frame_time > 0) && (frame_time < 10)) {
      glDisable(GL_LIGHTING);
      glColor3d(1.0, 1.0, 1.0);
      sprintf(buffer, "%.1f fps", 1.0 / frame_time);
      DrawText(R2Point(100, 100), buffer);
    }
  }

  // Capture screenshot image
  if (screenshot_image_name) {
    if (print_verbose) printf("Creating image %s\n", screenshot_image_name);
    R2Image image(GLUTwindow_width, GLUTwindow_height, 3);
    image.Capture();
    image.Write(screenshot_image_name);
    screenshot_image_name = NULL;
  }

  // Swap buffers
  glutSwapBuffers();
}



void GLUTResize(int w, int h)
{
  // Resize window
  glViewport(0, 0, w, h);

  // Resize viewer viewport
  viewer->ResizeViewport(0, 0, w, h);

  // Resize scene viewport
  scene->SetViewport(viewer->Viewport());

  // Remember window size
  GLUTwindow_width = w;
  GLUTwindow_height = h;

  // Redraw
  glutPostRedisplay();
}



void GLUTMotion(int x, int y)
{
  // Invert y coordinate
  y = GLUTwindow_height - y;

  // Compute mouse movement
  int dx = x - GLUTmouse[0];
  int dy = y - GLUTmouse[1];

  // Update mouse drag
  GLUTmouse_drag += dx*dx + dy*dy;

  // World in hand navigation
  if (GLUTbutton[0]) viewer->RotateWorld(1.0, center, x, y, dx, dy);
  else if (GLUTbutton[1]) viewer->ScaleWorld(1.0, center, x, y, dx, dy);
  else if (GLUTbutton[2]) viewer->TranslateWorld(1.0, center, x, y, dx, dy);
  if (GLUTbutton[0] || GLUTbutton[1] || GLUTbutton[2]) glutPostRedisplay();

  // Remember mouse position
  GLUTmouse[0] = x;
  GLUTmouse[1] = y;
}



void GLUTMouse(int button, int state, int x, int y)
{
  // Invert y coordinate
  y = GLUTwindow_height - y;

  // Mouse is going down
  if (state == GLUT_DOWN) {
    // Reset mouse drag
    GLUTmouse_drag = 0;
  }
  else {
    // Check for double click
    static RNBoolean double_click = FALSE;
    static RNTime last_mouse_up_time;
    double_click = (!double_click) && (last_mouse_up_time.Elapsed() < 0.4);
    last_mouse_up_time.Read();

    // Check for click (rather than drag)
    if (GLUTmouse_drag < 100) {
      // Check for double click
      if (double_click) {
        // Set viewing center point
        R3Ray ray = viewer->WorldRay(x, y);
        R3Point intersection_point;
        if (scene->Intersects(ray, NULL, NULL, NULL, &intersection_point)) {
          center = intersection_point;
        }
      }
    }
  }

  // Remember button state
  int b = (button == GLUT_LEFT_BUTTON) ? 0 : ((button == GLUT_MIDDLE_BUTTON) ? 1 : 2);
  GLUTbutton[b] = (state == GLUT_DOWN) ? 1 : 0;

  // Remember modifiers
  GLUTmodifiers = glutGetModifiers();

   // Remember mouse position
  GLUTmouse[0] = x;
  GLUTmouse[1] = y;

  // Redraw
  glutPostRedisplay();
}



void GLUTSpecial(int key, int x, int y)
{
  // Invert y coordinate
  y = GLUTwindow_height - y;

  // Process keyboard button event

  // Remember mouse position
  GLUTmouse[0] = x;
  GLUTmouse[1] = y;

  // Remember modifiers
  GLUTmodifiers = glutGetModifiers();

  // Redraw
  glutPostRedisplay();
}



void GLUTKeyboard(unsigned char key, int x, int y)
{
  // Process keyboard button event
  switch (key) {
  case '~': {
    // Dump screen shot to file iX.jpg
    static char buffer[64];
    static int image_count = 1;
    sprintf(buffer, "i%d.jpg", image_count++);
    screenshot_image_name = buffer;
    break; }

  case 'B':
  case 'b':
    show_bboxes = !show_bboxes;
    break;

  case 'C':
  case 'c':
    show_camera = !show_camera;
    break;
  case 'L':
  case 'l':
    show_lights = !show_lights;
    break;
  case 'W':
  case 'w':
    viewer->ScaleWorld(1.0, center, 0, 0, 5.0, 0.0);
    break;
  case 'S':
  case 's':
    viewer->ScaleWorld(1.0, center, 0, 0, -5.0, 0.0);
    break;
  case 'E':
  case 'e':
    viewer->RotateCameraRoll(-.01);
    break;
  case 'Q':
  case 'q':
    viewer->RotateCameraRoll(.01);
    break;
  case 'R':
  case 'r':
    show_rays = !show_rays;
    break;
  case 'F':
  case 'f':
    show_emit = !show_emit;
    break;
  case 'G':
  case 'g':
    show_global = !show_global;
    break;
  case 'H':
  case 'h':
    show_caustic = !show_caustic;
    break;
  case 'O':
  case 'o':
    show_shapes = !show_shapes;
    break;
  case 'M':
  case 'm':
    show_paths = !show_paths;
    break;
  case 'T':
  case 't':
    show_frame_rate = !show_frame_rate;
    break;

  case ' ':
    viewer->SetCamera(scene->Camera());
    break;

  case 27: // ESCAPE
    GLUTStop();
    break;
  }

  // Remember mouse position
  GLUTmouse[0] = x;
  GLUTmouse[1] = GLUTwindow_height - y;

  // Remember modifiers
  GLUTmodifiers = glutGetModifiers();

  // Redraw
  glutPostRedisplay();
}




void GLUTInit(int *argc, char **argv)
{
  // Open window
  glutInit(argc, argv);
  glutInitWindowPosition(100, 100);
  glutInitWindowSize(GLUTwindow_width, GLUTwindow_height);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); // | GLUT_STENCIL
  GLUTwindow = glutCreateWindow("Scene Visualization");

  // Initialize lighting
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  static GLfloat lmodel_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
  glEnable(GL_NORMALIZE);
  glEnable(GL_LIGHTING);

  // Initialize headlight
  // static GLfloat light0_diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
  // static GLfloat light0_position[] = { 0.0, 0.0, 1.0, 0.0 };
  // glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  // glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  // glEnable(GL_LIGHT0);

  // Initialize graphics modes
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // Initialize GLUT callback functions
  glutDisplayFunc(GLUTRedraw);
  glutReshapeFunc(GLUTResize);
  glutKeyboardFunc(GLUTKeyboard);
  glutSpecialFunc(GLUTSpecial);
  glutMouseFunc(GLUTMouse);
  glutMotionFunc(GLUTMotion);
}



void GLUTMainLoop(void)
{
  // Initialize viewing center
  if (scene) center = scene->BBox().Centroid();

  // Run main loop -- never returns
  glutMainLoop();
}



////////////////////////////////////////////////////////////////////////
// Input/output
////////////////////////////////////////////////////////////////////////

static R3Scene *
ReadScene(char *filename)
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
  if (!scene->ReadFile(filename)) {
    delete scene;
    return NULL;
  }

  // Print statistics
  if (print_verbose) {
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
// Program argument parsing
////////////////////////////////////////////////////////////////////////

static int
ParseArgs(int argc, char **argv)
{
  // Parse arguments
  argc--; argv++;
  while (argc > 0) {
    if ((*argv)[0] == '-') {
      if (!strcmp(*argv, "-v")) {
        print_verbose = 1;
      }
      else if (!strcmp(*argv, "-resolution")) {
        argc--; argv++; render_image_width = atoi(*argv);
        argc--; argv++; render_image_height = atoi(*argv);
      } else if (!strcmp(*argv, "-global")) {
        INDIRECT_ILLUM = true;
        argc--; argv++; GLOBAL_PHOTON_COUNT = atoi(*argv);
        if (GLOBAL_PHOTON_COUNT < 1)
          GLOBAL_PHOTON_COUNT = 1;
      } else if (!strcmp(*argv, "-caustic")) {
        CAUSTIC_ILLUM = true;
        argc--; argv++; CAUSTIC_PHOTON_COUNT = atoi(*argv);
        if (CAUSTIC_PHOTON_COUNT < 1)
          CAUSTIC_PHOTON_COUNT = 1;
      }
      else {
        fprintf(stderr, "Invalid program argument: %s", *argv);
        exit(1);
      }
      argv++; argc--;
    }
    else {
      if (!input_scene_name) input_scene_name = *argv;
      else if (!output_image_name) output_image_name = *argv;
      else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
      argv++; argc--;
    }
  }

  // Check scene filename
  if (!input_scene_name) {
    fprintf(stderr, "Usage: visualize inputscenefile [-v]\n");
    return 0;
  }

  // Return OK status
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Main program
////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  // Parse program arguments
  if (!ParseArgs(argc, argv)) exit(-1);

  // Read scene
  scene = ReadScene(input_scene_name);
  if (!scene) exit(-1);

  // Map Photons if necessary
  if (INDIRECT_ILLUM || CAUSTIC_ILLUM) {
    MapPhotons();
  }

  // Initialize GLUT
  GLUTInit(&argc, argv);

  // Create viewer
  viewer = new R3Viewer(scene->Viewer());
  if (!viewer) exit(-1);
  // Run GLUT interface
  GLUTMainLoop();

  // Delete viewer (doesn't ever get here)
  delete viewer;

  // Return success
  return 0;
}
