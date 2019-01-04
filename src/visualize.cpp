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

static RNScalar IR = 1.0;

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
static int show_frame_rate = 0;



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

static void
DrawCustom(R3Scene *scene)
{
  glDisable(GL_LIGHTING);
  glLineWidth(2);
  R3Point camera = scene->Camera().Origin();
  double radius = 0.025;
  R3Sphere(camera, radius).Draw();

  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;
  radius = 0.025;
  for (int i = 0; i < scene->Viewport().Width(); i += 35) {
    for (int j = 0; j < scene->Viewport().Height(); j += 35) {
      R3Ray ray = scene->Viewer().WorldRay(i, j);
      if (scene->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
        const R3Material *material = (element) ? element->Material() : &R3default_material;
        const R3Brdf *brdf = (material) ? material->Brdf() : &R3default_brdf;
        if (brdf && brdf->IsTransparent()) {
          glColor3d(1, 0, 0);
          R3Sphere(point, radius).Draw();
          glColor3d(0.1, 0.1, 0.7);
          R3Span(camera, point).Draw();
          R3Point prev = camera;
          for (int k = 0; k < 5; k++) {
            material = (element) ? element->Material() : &R3default_material;
            brdf = (material) ? material->Brdf() : &R3default_brdf;
            if (brdf) {
              if (!brdf->IsTransparent()) {
                break;
              }
              R3Vector view = point - prev;
              view.Normalize();
              RNScalar cos_theta = normal.Dot(-view);
              RNScalar ir_mat = IR;
              // Get reflection
              R3Vector exact = TransmissiveBounce(normal, view, cos_theta, ir_mat);
              ray = R3Ray(point + exact*RN_EPSILON, exact, true);
              prev = point;
              if (scene->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
                glColor3d(1, 0, 0);
                R3Sphere(point, radius).Draw();
                if (k == 0)
                  glColor3d(0.8, 0.8, 0.8);
                else if (k == 1)
                  glColor3d(0.0, 0.8, 0.2);
                else
                  glColor3d(0.8, 0.0, 0.2);
                R3Span(prev, point).Draw();
              } else {
                glColor3d(1, 1, 1);
                if (k == 0)
                  glColor3d(0.8, 0.8, 0.8);
                else if (k == 1)
                  glColor3d(0.0, 0.8, 0.2);
                else
                  glColor3d(0.8, 0.0, 0.2);
                R3Span(prev, prev + 200 * exact).Draw();
                break;
              }
            }
          }
        }
      }
    }
  }
  /*
  // Ray intersection variables
  glDisable(GL_LIGHTING);
  glLineWidth(2);
  R3SceneNode *node;
  R3SceneElement *element;
  R3Shape *shape;
  R3Point point;
  R3Vector normal;
  RNScalar t;
  // Ray drawing variables
  double radius = 0.025;
  R3Vector beam = R3Vector(cos(0.0872665), sin(0.0872665), 0);
  //R3Vector beam = R3Vector(0, 1, 0);
  beam.Normalize();
  beam *= 1.25;
  R3Point top = beam.Point() + R3Vector(0, 1, 0);
  R3Ray ray = R3Ray(top, -beam);
  if (scene->Intersects(ray, &node, &element, &shape, &point, &normal, &t)) {
      glColor3d(0.7, 0.1, 0.1);
      R3Sphere(point, radius).Draw();
      R3Sphere(top, radius).Draw();
      R3Span(point, top).Draw();
      radius = 0.015;
      R3Vector view = point - top;
      view.Normalize();
      RNScalar cos_theta = abs(normal.Dot(view));
      int n = 1000;
      R3Vector view_flipped_perp = normal * cos_theta;
      R3Vector view_reflection = view + view_flipped_perp*2.0;
      view_reflection.Normalize();
      R3Vector exact = view_reflection;
      for (int i = 0; i < 500; i++) {
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

        // Rotate around axis by phi and normalize
        R3Point end = result + point;
        glColor3d(0, 0, 0);
        R3Sphere(end, radius).Draw();
        glColor3d(0.1, 0.1, 0.7);
        R3Span(point, end).Draw();
      }
  }

  glLineWidth(1);*/
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


  DrawCustom(scene);

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
  case 'O':
  case 'o':
    show_shapes = !show_shapes;
    break;
  case 'i':
    IR += 0.1;
    printf("%f\n", IR);
    break;
  case 'I':
    IR *= 2;
    break;
  case 'a':
    IR = 1;
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
