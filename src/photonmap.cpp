// Source file for the scene viewer program



// Include files 

#include "R3Graphics/R3Graphics.h"
#include "fglut/fglut.h"
#include "render.h"



// Program variables

static char *input_scene_name = NULL;
static char *output_image_name = NULL;
static char *screenshot_image_name = NULL;
static int render_image_width = 64;
static int render_image_height = 64;
static int print_verbose = 0;



// GLUT variables 

static int GLUTwindow = 0;
static int GLUTwindow_height = 800;
static int GLUTwindow_width = 800;
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

  case 'R':
  case 'r':
    show_rays = !show_rays;
    break;

  case 'S':
  case 's':
    show_shapes = !show_shapes;
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
  GLUTwindow = glutCreateWindow("Property Viewer");

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



static int
WriteImage(R2Image *image, const char *filename)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Write image to file
  if (!image->Write(filename)) return 0;

  // Print statistics
  if (print_verbose) {
    printf("Wrote image to %s ...\n", filename);
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  Width = %d\n", image->Width());
    printf("  Height = %d\n", image->Height());
    fflush(stdout);
  }

  // Return success
  return 1;
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
    fprintf(stderr, "Usage: photonmap inputscenefile [outputimagefile] [-resolution <int> <int>] [-v]\n");
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

  // Check output image file
  if (output_image_name) {
    // Set scene viewport
    scene->SetViewport(R2Viewport(0, 0, render_image_width, render_image_height));

    // Render image
    R2Image *image = RenderImage(scene, render_image_width, render_image_height, print_verbose);
    if (!image) exit(-1);

    // Write image
    if (!WriteImage(image, output_image_name)) exit(-1);
    
    // Delete image
    delete image;
  }
  else {
    // Initialize GLUT
    GLUTInit(&argc, argv);

    // Create viewer
    viewer = new R3Viewer(scene->Viewer());
    if (!viewer) exit(-1);
    
    // Run GLUT interface
    GLUTMainLoop();

    // Delete viewer (doesn't ever get here)
    delete viewer;
  }

  // Return success 
  return 0;
}

















