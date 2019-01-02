// Source file for the KD tree test program



////////////////////////////////////////////////////////////////////////
// Include files 
////////////////////////////////////////////////////////////////////////

#include "R3Graphics/R3Graphics.h"
#include "fglut/fglut.h"



////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////

struct TestPoint {
  R3Point position;
  int id;
};



////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////

// Program variables

static int max_total_points = 1000;
static RNLength min_nearby_distance = 0;
static RNLength max_nearby_distance = 0.5;
static int max_nearby_points = 100;
static int print_debug = 0;
static int print_verbose = 0;



// GLUT variables 

static int GLUTwindow = 0;
static int GLUTwindow_height = 1024;
static int GLUTwindow_width = 1024;
static int GLUTmouse[2] = { 0, 0 };
static int GLUTbutton[3] = { 0, 0, 0 };
static int GLUTmodifiers = 0;



// Application variables

static RNArray<TestPoint *> all_points;
static RNArray<TestPoint *> nearby_points;
static R3Kdtree<TestPoint *> *kdtree = NULL;
static TestPoint *selected_point = NULL;
static TestPoint *closest_point = NULL;
static R3Viewer *viewer = NULL;



// Display variables

static int show_points = 1;
static int show_kdtree = 1;
static int show_constraints = 1;



////////////////////////////////////////////////////////////////////////
// GLUT interface functions
////////////////////////////////////////////////////////////////////////

void GLUTStop(void)
{
  // Destroy window 
  glutDestroyWindow(GLUTwindow);

  // Exit
  exit(0);
}



void GLUTIdle(void)
{
  // Redraw
  glutPostRedisplay();
}



void GLUTRedraw(void)
{
  // Set viewing transformation
  viewer->Camera().Load();

  // Clear window 
  glClearColor(200.0/255.0, 200.0/255.0, 200.0/255.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set lights
  static GLfloat light0_position[] = { 3.0, 4.0, 5.0, 0.0 };
  glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
  static GLfloat light1_position[] = { -3.0, -2.0, -3.0, 0.0 };
  glLightfv(GL_LIGHT1, GL_POSITION, light1_position);

  // Show all points
  if (show_points) {
    glEnable(GL_LIGHTING);
    static GLfloat material[4] = { 0, 0, 1, 1 };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material); 
    for (int i = 0; i < all_points.NEntries(); i++) {
      TestPoint *point = all_points[i];
      R3Sphere(point->position, 0.01).Draw();
    }
  }
    
  // Show nearby points
  if (selected_point && !nearby_points.IsEmpty()) {
    glEnable(GL_LIGHTING);
    static GLfloat material[4] = { 1, 0, 0, 1 };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material); 
    for (int i = 0; i < nearby_points.NEntries(); i++) {
      TestPoint *point = nearby_points.Kth(i);
      R3Sphere(point->position, 0.02).Draw();
    }
  }

  // Show closest point
  if (selected_point && closest_point) {
    glEnable(GL_LIGHTING);
    static GLfloat material[4] = { 0, 1, 0, 1 };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material); 
    R3Sphere(closest_point->position, 0.03).Draw();
  }
    
  // Show selected point
  if (selected_point) {
    glEnable(GL_LIGHTING);
    static GLfloat material[4] = { 1, 1, 1, 1 };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material); 
    R3Sphere(selected_point->position, 0.05).Draw();
  }
    
  // Show constraints
  if (0 && show_constraints) {
    glDisable(GL_LIGHTING);
    glColor3f(0.5, 0, 0);
    R3Sphere(selected_point->position, min_nearby_distance).Outline();
    R3Sphere(selected_point->position, max_nearby_distance).Outline();
  }
    
  // Show KD tree
  if (show_kdtree) {
    glDisable(GL_LIGHTING);
    glColor3f(0, 0, 0);
    kdtree->Outline();
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
  
  // World in hand navigation 
  R3Point origin(0, 0, 0);
  if (GLUTbutton[0]) viewer->RotateWorld(1.0, origin, x, y, dx, dy);
  else if (GLUTbutton[1]) viewer->ScaleWorld(1.0, origin, x, y, dx, dy);
  else if (GLUTbutton[2]) viewer->TranslateWorld(1.0, origin, x, y, dx, dy);
  if (GLUTbutton[0] || GLUTbutton[1] || GLUTbutton[2]) glutPostRedisplay();

  // Remember mouse position 
  GLUTmouse[0] = x;
  GLUTmouse[1] = y;
}



void GLUTMouse(int button, int state, int x, int y)
{
  // Invert y coordinate
  y = GLUTwindow_height - y;
  
  // Process mouse button event
  if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {
    // Check for double click
    static RNTime click_time;
    const RNScalar max_double_click_elapsed = 0.5;
    RNBoolean double_click = (click_time.Elapsed() < max_double_click_elapsed);
    click_time.Read();

    // Select closest point to cursor
    if (double_click) {
      selected_point = NULL;
      RNLength closest_distance = 10;
      R2Point cursor_position(x, y);
      for (int i = 0; i < all_points.NEntries(); i++) {
        TestPoint *point = all_points[i];
        const R3Point& world_position = point->position;
        R2Point screen_position = viewer->ViewportPoint(world_position);
        RNLength distance = R2Distance(screen_position, cursor_position);
        if (distance < closest_distance) {
          selected_point = point;
          closest_distance = distance;
        }
      }

      // Find closest points
      closest_point = NULL;
      nearby_points.Empty();
      if (selected_point) {
        if (max_nearby_points > 0) {
          kdtree->FindClosest(selected_point, min_nearby_distance, max_nearby_distance, max_nearby_points, nearby_points);
          closest_point = (nearby_points.NEntries() > 0) ? nearby_points.Head() : NULL;
          if (print_debug) printf("Found %d points\n", nearby_points.NEntries());
        }
        else {
          kdtree->FindAll(selected_point, min_nearby_distance, max_nearby_distance, nearby_points);
          closest_point = kdtree->FindClosest(selected_point);
          if (print_debug) printf("Found %d points\n", nearby_points.NEntries());
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
  case 'P':
  case 'p':
    show_points = !show_points;
    break;

  case 'K':
  case 'k':
    show_kdtree = !show_kdtree;
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
  GLUTwindow = glutCreateWindow("KD tree test program");

  // Initialize background color 
  glClearColor(200.0/255.0, 200.0/255.0, 200.0/255.0, 1.0);

  // Initialize lights
  static GLfloat lmodel_ambient[] = { 0.2, 0.2, 0.2, 1.0 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
  static GLfloat light0_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
  glEnable(GL_LIGHT0);
  static GLfloat light1_diffuse[] = { 0.5, 0.5, 0.5, 1.0 };
  glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
  glEnable(GL_LIGHT1);
  glEnable(GL_NORMALIZE);
  glEnable(GL_LIGHTING);

  // Initialize graphics modes 
  glEnable(GL_DEPTH_TEST);
  glPointSize(3); 

  // Initialize GLUT callback functions 
  glutDisplayFunc(GLUTRedraw);
  glutReshapeFunc(GLUTResize);
  glutKeyboardFunc(GLUTKeyboard);
  glutSpecialFunc(GLUTSpecial);
  glutMouseFunc(GLUTMouse);
  glutMotionFunc(GLUTMotion);

  // Initialize font
#if (RN_OS == RN_WINDOWSNT)
  int font = glGenLists(256);
  wglUseFontBitmaps(wglGetCurrentDC(), 0, 256, font); 
  glListBase(font);
#endif
}



void GLUTMainLoop(void)
{
  // Run main loop -- never returns 
  glutMainLoop();
}

 

////////////////////////////////////////////////////////////////////////
// Data creation
////////////////////////////////////////////////////////////////////////

static R3Point 
GetTestPointPosition(TestPoint *point, void *dummy)
{
  // Return point position (used for Kdtree)
  return point->position;
}



static int
CreateKdtree(void)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

#if 0
  // Create KD tree (method 1)
  TestPoint point;
  int position_offset = (unsigned char *) &(point.position) - (unsigned char *) &point;
  kdtree = new R3Kdtree<TestPoint *>(all_points, position_offset);
  if (!kdtree) {
    fprintf(stderr, ("Unable to create KD tree\n");
    return 0;
  }
#else
  // Create KD tree (method 2)
  kdtree = new R3Kdtree<TestPoint *>(all_points, GetTestPointPosition);
  if (!kdtree) {
    fprintf(stderr, "Unable to create KD tree\n");
    return 0;
  }
#endif

  // Print statistics
  if (print_verbose) {
    printf("Created KD tree ...\n");
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Nodes = %d\n", kdtree->NNodes());
    fflush(stdout);
  }

  // Return success
  return 1;
}



static int
CreatePoints(void)
{
  // Start statistics
  RNTime start_time;
  start_time.Read();

  // Create points at random positions in unit box
  RNSeedRandomScalar();
  for (int i = 0; i < max_total_points; i++) {
    TestPoint *point = new TestPoint();
    double x = 2.0 * RNRandomScalar() - 1.0;
    double y = 2.0 * RNRandomScalar() - 1.0;
    double z = 2.0 * RNRandomScalar() - 1.0;
    point->position = R3Point(x, y, z);
    point->id = i;
    all_points.Insert(point);
  }

  // Print statistics
  if (print_verbose) {
    printf("Created points ...\n");
    printf("  Time = %.2f seconds\n", start_time.Elapsed());
    printf("  # Points = %d\n", all_points.NEntries());
    fflush(stdout);
  }

  // Return success
  return 1;
}



static int
CreateViewer(void)
{
  // Setup default camera view looking down the Z axis
  R3Point origin = R3unit_box.Centroid() - 4 * R3negz_vector;;
  R3Camera camera(origin, R3negz_vector, R3posy_vector, 0.4, 0.4, 0.1, 1000.0);
  R2Viewport viewport(0, 0, GLUTwindow_width, GLUTwindow_height);
  viewer = new R3Viewer(camera, viewport);

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
    if (!strcmp(*argv, "-v")) print_verbose = 1; 
    else if (!strcmp(*argv, "-debug")) print_debug = 1; 
    else if (!strcmp(*argv, "-max_total_points")) { argv++; argc--; max_total_points = atoi(*argv); }
    else if (!strcmp(*argv, "-min_nearby_distance")) { argv++; argc--; min_nearby_distance = atof(*argv); }
    else if (!strcmp(*argv, "-max_nearby_distance")) { argv++; argc--; max_nearby_distance = atof(*argv); }
    else if (!strcmp(*argv, "-max_nearby_points")) { argv++; argc--; max_nearby_points = atoi(*argv); }
    else { fprintf(stderr, "Invalid program argument: %s", *argv); exit(1); }
    argv++; argc--;
  }

  // Return OK status 
  return 1;
}



////////////////////////////////////////////////////////////////////////
// Main program
////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
  // Initialize GLUT
  GLUTInit(&argc, argv);

  // Parse program arguments
  if (!ParseArgs(argc, argv)) exit(-1);

  // Create everything points
  if (!CreatePoints()) exit(-1);
  if (!CreateKdtree()) exit(-1);
  if (!CreateViewer()) exit(-1);

  // Run GLUT interface
  GLUTMainLoop();

  // Return success 
  return 0;
}



