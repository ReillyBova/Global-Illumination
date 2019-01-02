/* Source file for the R3 model class */



/* Include files */

#include "R3Graphics.h"



// Draw method

#define DRAW_WITH_INDIVIDUAL_TRIANGLES 0
#define DRAW_WITH_VBO 1
#define DRAW_WITH_DISPLAY_LIST 2
// #define DRAW_METHOD DRAW_WITH_INDIVIDUAL_TRIANGLES
// #define DRAW_METHOD DRAW_WITH_VBO
#define DRAW_METHOD DRAW_WITH_DISPLAY_LIST



/* Initialization functions */

int 
R3InitModel()
{
  /* Return success */
  return TRUE;
}



void 
R3StopModel()
{
}



/* Member functions */

R3Model::
R3Model(void)
  : triangles(NULL),
    opengl_id(0),
    opengl_id2(0)
{
}



R3Model::
R3Model(R3TriangleArray *triangles, 
  const RNArray<R3Material *>& materials,
  const RNArray<R3Material *>& triangle_materials,
  const RNArray<RNArray<R3Triangle *> *>& material_triangles)
  : triangles(triangles),
    materials(materials),
    triangle_materials(triangle_materials),
    material_triangles(material_triangles),
    opengl_id(0),
    opengl_id2(0)
{
}



R3Model::
~R3Model(void)
{
  // Delete opengl data
  if (opengl_id > 0) {
#   if (DRAW_METHOD == DRAW_WITH_VBO)
      // Delete vertex buffer object
      glDeleteBuffers(1, &opengl_id);
      if (opengl_id2 > 0) glDeleteBuffers(1, &opengl_id2);
#   elif (DRAW_METHOD == DRAW_WITH_DISPLAY_LIST)
      // Delete display list
      glDeleteLists(opengl_id, 1); 
#   endif
  }

  // Delete material triangle lists
  for (int i =0; i < material_triangles.NEntries(); i++) {
    delete material_triangles.Kth(i);
  }
  
  // Delete triangles
  if (triangles) delete triangles;

  // Delete materials
  // ???
}



RNBoolean R3Model::
Intersects(const R3Ray& ray, 
  R3Point *hit_point, R3Vector *hit_normal, 
  RNScalar *hit_t, int *hit_triangle_index) const
{
  // Check triangles
  if (!triangles) return FALSE;

  // Check bounding box
  if (!R3Intersects(ray, BBox())) return FALSE;

  // Check each triangle for intersection
  RNScalar min_t = FLT_MAX;
  for (int i = 0; i < NTriangles(); i++) {
    const R3Triangle *triangle = Triangle(i);
    R3Point point;
    R3Vector normal;
    RNScalar t;
    if (R3Intersects(ray, *triangle, &point, &normal, &t) == R3_POINT_CLASS_ID) {
      if (t < min_t) {
        if (hit_point) *hit_point = point;
        if (hit_normal) *hit_normal = normal;
        if (hit_triangle_index) *hit_triangle_index = i;
        if (hit_t) *hit_t = t;
        min_t = t;
      }
    }
  }

  // Return whether hit any triangle
  return (min_t == FLT_MAX) ? FALSE : TRUE;
}



struct VBOVertex {
  GLfloat x, y, z, pad1;
  GLfloat s, t, pad2, pad3;
};


void R3Model::
Draw(const R3DrawFlags draw_flags) const
{
  // Check triangles
  if (!triangles) return;
  if (triangles->NTriangles() == 0) return;
  assert(triangle_materials.NEntries() == NTriangles());
  assert(material_triangles.NEntries() == NMaterials());

  // Draw model
  if (draw_flags != 0) {
#   if (DRAW_METHOD == DRAW_WITH_VBO)
      // Initialize material
      R3null_material.Draw(TRUE);

      // Create VBO
      if (opengl_id == 0) {
        // Load materials
        for (int i = 0; i < NMaterials(); i++) {
          R3Material *material = Material(i);
          material->Load();
        }

        // Create VBO vertex array
        VBOVertex *vbo_vertices = new VBOVertex [ NVertices() ];
        for (int i = 0; i < NVertices(); i++) {
          R3TriangleVertex *vertex = Vertex(i);
          const R3Point& position = vertex->Position();
          const R2Point& texcoords = vertex->TextureCoords();
          vbo_vertices[i].x = position.X();
          vbo_vertices[i].y = position.Y();
          vbo_vertices[i].z = position.Z();
          vbo_vertices[i].s = texcoords.X();
          vbo_vertices[i].t = texcoords.Y();
          vertex->SetMark(i);
        }

        // Create VBO triangle array
        GLint *vbo_triangles = new GLint [ 3 * NTriangles() ];
        for (int i = 0; i < NTriangles(); i++) {
          R3Triangle *triangle = Triangle(i);
          R3TriangleVertex *v0 = triangle->Vertex(0);
          R3TriangleVertex *v1 = triangle->Vertex(0);
          R3TriangleVertex *v2 = triangle->Vertex(0);
          vbo_triangles[3*i+0] = v0->Mark();
          vbo_triangles[3*i+1] = v1->Mark();
          vbo_triangles[3*i+2] = v2->Mark();
        }

        // Create VBO for vertices
        glGenBuffers(1, &opengl_id);
        glBindBuffer(GL_ARRAY_BUFFER, opengl_id);
        glBufferData(GL_ARRAY_BUFFER, NVertices() * sizeof(VBOVertex), vbo_vertices, GL_STATIC_DRAW);
        
        // Create VBO for triangles
        glGenBuffers(1, &opengl_id2);
        glBindBuffer(GL_ARRAY_BUFFER, opengl_id2);
        glBufferData(GL_ELEMENT_BUFFER, 3 * NTriangles() * sizeof(GLuint), vbo_triangles, GL_STATIC_DRAW);

        // Delete VBO data
        delete [] vbo_vertices;
        delete [] vbo_triangles;
      }
        
      // Draw VBO
      glBindBuffer(GL_ARRAY_BUFFER, opengl_id);
      glEnableClientState(GL_VERTEX_ARRAY);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glVertexPointer(3, GL_FLOAT, sizeof(VBOVertex), (void *) 0);
      glTexCoordPointer(2, GL_FLOAT, sizeof(VBOVertex), (void *) (3 * sizeof(GLfloat)));
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opengl_id2);  
      glDrawElements(GL_TRIANGLES, NTriangles(), GL_UNSIGNED_INT, (void *) 0);
      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);

      // Reset material
      R3null_material.Draw(TRUE);
#   elif (DRAW_METHOD == DRAW_WITH_DISPLAY_LIST)
      // Initialize material
      R3null_material.Draw(TRUE);

      // Create display list
      if (opengl_id == 0) {
        // Load materials
        for (int i = 0; i < NMaterials(); i++) {
          R3Material *material = Material(i);
          material->Load();
        }
        
        // Begin display list
        R3Model *model = (R3Model *) this;
        model->opengl_id = glGenLists(1);
        glNewList(opengl_id, GL_COMPILE);

        // Draw triangles
        for (int i = 0; i < NMaterials(); i++) {
          R3Material *material = Material(i);
          material->Draw();
          glBegin(GL_TRIANGLES);
          for (int j = 0; j < material_triangles[i]->NEntries(); j++) {
            R3Triangle *triangle = material_triangles[i]->Kth(j);
            R3LoadNormal(triangle->Normal());

            R3TriangleVertex *v0 = triangle->Vertex(0);
            R3LoadTextureCoords(v0->TextureCoords());
            R3LoadPoint(v0->Position());

            R3TriangleVertex *v1 = triangle->Vertex(1);
            R3LoadTextureCoords(v1->TextureCoords());
            R3LoadPoint(v1->Position());

            R3TriangleVertex *v2 = triangle->Vertex(2);
            R3LoadTextureCoords(v2->TextureCoords());
            R3LoadPoint(v2->Position());
          }
          glEnd();
        }
        
        // End display list
        glEndList();
      }

      // Call display list
      glCallList(opengl_id);

      // Reset material
      R3null_material.Draw(TRUE);
#   else
      // Draw individual triangles
      for (int i = 0; i < NMaterials(); i++) {
        R3Material *material = Material(i);
        material->Draw();
        glBegin(GL_TRIANGLES);
        for (int j = 0; j < material_triangles[i]->NEntries(); j++) {
          R3Triangle *triangle = material_triangles[i]->Kth(j);
          R3LoadNormal(triangle->Normal());
          
          R3TriangleVertex *v0 = triangle->Vertex(0);
          R3LoadTextureCoords(v0->TextureCoords());
          R3LoadPoint(v0->Position());
          
          R3TriangleVertex *v1 = triangle->Vertex(1);
          R3LoadTextureCoords(v1->TextureCoords());
          R3LoadPoint(v1->Position());
          
          R3TriangleVertex *v2 = triangle->Vertex(2);
          R3LoadTextureCoords(v2->TextureCoords());
          R3LoadPoint(v2->Position());
        }
        glEnd();
      }
#   endif
  }
  else {
    // Draw triangles
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < NTriangles(); i++) {
      R3Triangle *triangle = Triangle(i);
      R3LoadNormal(triangle->Normal());
      R3TriangleVertex *v0 = triangle->Vertex(0);
      R3LoadPoint(v0->Position());
      R3TriangleVertex *v1 = triangle->Vertex(1);
      R3LoadPoint(v1->Position());
      R3TriangleVertex *v2 = triangle->Vertex(2);
      R3LoadPoint(v2->Position());
    }
    glEnd();
  }
}



////////////////////////////////////////////////////////////////////////
// I/O FUNCTIONS
////////////////////////////////////////////////////////////////////////

int R3Model::
ReadFile(const char *filename)
{
  // Parse input filename extension
  const char *extension;
  if (!(extension = strrchr(filename, '.'))) {
    printf("Filename %s has no extension (e.g., .obj)\n", filename);
    return 0;
  }

  // Read file of appropriate type
  if (!strncmp(extension, ".obj", 4)) {
    if (!ReadObjFile(filename)) return 0;
  }
  else {
    RNFail("Unable to read file %s (unrecognized extension: %s)\n", filename, extension);
    return 0;
  }

  // Return success
  return 1;
}



int R3Model::
WriteFile(const char *filename) const
{
  // Parse input filename extension
  const char *extension;
  if (!(extension = strrchr(filename, '.'))) {
    printf("Filename %s has no extension (e.g., .obj)\n", filename);
    return 0;
  }

  // Write file of appropriate type
  if (!strncmp(extension, ".obj", 4)) {
    if (!WriteObjFile(filename)) return 0;
  }
  else {
    RNFail("Unable to write file %s (unrecognized extension: %s)\n", filename, extension);
    return 0;
  }

  // Return success
  return 1;
}



////////////////////////////////////////////////////////////////////////
// OBJ FILE FUNCTIONS
////////////////////////////////////////////////////////////////////////

static int
FindMaterialIndex(const RNArray<R3Material *>& materials, const char *name)
{
  // Return material with matching name
  for (int i = 0; i < materials.NEntries(); i++) {
    R3Material *material = materials.Kth(i);
    if (!strcmp(name, material->Name())) return i;
  }

  // No matching material found
  return -1;
}



int R3Model::
ReadObjMtlFile(const char *dirname, const char *mtlname)
{
  // Open file
  char filename[1024];
  sprintf(filename, "%s/%s", dirname, mtlname);
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    RNFail("Unable to open file %s", filename);
    return 0;
  }

  // Parse file
  char buffer[1024];
  int line_count = 0;
  R3Brdf *brdf = NULL;
  R2Texture *texture = NULL;
  R3Material *material = NULL;
  while (fgets(buffer, 1023, fp)) {
    // Increment line counter
    line_count++;

    // Skip white space
    char *bufferp = buffer;
    while (isspace(*bufferp)) bufferp++;

    // Skip blank lines and comments
    if (*bufferp == '#') continue;
    if (*bufferp == '\0') continue;

    // Get keyword
    char keyword[80];
    if (sscanf(bufferp, "%s", keyword) != 1) {
      RNFail("Syntax error on line %d in file %s", line_count, filename);
      return 0;
    }

    // Check keyword
    if (!strcmp(keyword, "newmtl")) {
      // Parse line
      char name[1024];
      if (sscanf(bufferp, "%s%s", keyword, name) != (unsigned int) 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Create new material
      texture = NULL;
      brdf = new R3Brdf();
      material = new R3Material(brdf, texture, name);
      materials.Insert(material);
      RNArray<R3Triangle *> *mat_tris = new RNArray<R3Triangle *>();
      material_triangles.Insert(mat_tris);
    }
    else if (!strcmp(keyword, "Ka")) {
      // Parse line
      double r, g, b;
      if (sscanf(bufferp, "%s%lf%lf%lf", keyword, &r, &g, &b) != (unsigned int) 4) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set ambient reflectance 
      if (material && brdf) {
        brdf->SetAmbient(RNRgb(r, g, b));
        material->Update();
      }
    }
    else if (!strcmp(keyword, "Kd")) {
      // Parse line
      double r, g, b;
      if (sscanf(bufferp, "%s%lf%lf%lf", keyword, &r, &g, &b) != (unsigned int) 4) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set diffuse reflectance 
      if (material && brdf) {
        brdf->SetDiffuse(RNRgb(r, g, b));
        material->Update();
      }
    }
    else if (!strcmp(keyword, "Ks")) {
      // Parse line
      double r, g, b;
      if (sscanf(bufferp, "%s%lf%lf%lf", keyword, &r, &g, &b) != (unsigned int) 4) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set specular reflectance 
      if (material && brdf) {
        brdf->SetSpecular(RNRgb(r, g, b));
        material->Update();
      }
    }
    else if (!strcmp(keyword, "Ns")) {
      // Parse line
      double ns;
      if (sscanf(bufferp, "%s%lf", keyword, &ns) != (unsigned int) 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set shininess
      if (material && brdf) {
        brdf->SetShininess(ns);
        material->Update();
      }
    }
    else if (!strcmp(keyword, "Ni")) {
      // Parse line
      double index_of_refraction;
      if (sscanf(bufferp, "%s%lf", keyword, &index_of_refraction) != (unsigned int) 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set index of refraction
      if (material && brdf) {
        brdf->SetIndexOfRefraction(index_of_refraction);
        material->Update();
      }
    }
    else if (!strcmp(keyword, "d")) {
      // Parse line
      double transparency;
      if (sscanf(bufferp, "%s%lf", keyword, &transparency) != (unsigned int) 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set opacity
      if (material && brdf) {
        brdf->SetOpacity(1 - transparency);
        material->Update();
      }
    }
    else if (!strcmp(keyword, "map_Kd")) {
      // Parse line
      char texture_name[1024];
      if (sscanf(bufferp, "%s%s", keyword, texture_name) != (unsigned int) 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Set texture
      if (material) {
        char texture_filename[1024];
        sprintf(texture_filename, "%s/%s", dirname, texture_name);
        R2Image *image = new R2Image();
        if (!image->Read(texture_filename)) return 0;
        R2Texture *texture = new R2Texture(image);
        material->SetTexture(texture);
        material->Update();
      }
    }
  }

  // Close file
  fclose(fp);

  // Return success
  return 1;
}



int R3Model::
ReadObjFile(const char *filename)
{
  // Open file
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    RNFail("Unable to open file %s", filename);
    return 0;
  }

  // Determine directory name (for texture image files)
  char dirname[1024];
  strncpy(dirname, filename, 1024);
  char *endp = strrchr(dirname, '/');
  if (!endp) endp = strrchr(dirname, '\\');
  if (!endp) strcpy(dirname, ".");
  else *endp = '\0';

  // Read body
  char buffer[1024];
  int line_count = 0;
  int material_index =-1;
  RNArray<R2Point *> texture_coords;
  RNArray<R3TriangleVertex *> verts;
  RNArray<R3Triangle *> tris;
  while (fgets(buffer, 1023, fp)) {
    // Increment line counter
    line_count++;

    // Skip white space
    char *bufferp = buffer;
    while (isspace(*bufferp)) bufferp++;

    // Skip blank lines and comments
    if (*bufferp == '#') continue;
    if (*bufferp == '\0') continue;

    // Get keyword
    char keyword[80];
    if (sscanf(bufferp, "%s", keyword) != 1) {
      RNFail("Syntax error on line %d in file %s", line_count, filename);
      return 0;
    }

    // Check keyword
    if (!strcmp(keyword, "v")) {
      // Read vertex coordinates
      double x, y, z;
      if (sscanf(bufferp, "%s%lf%lf%lf", keyword, &x, &y, &z) != 4) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Create vertex
      R3TriangleVertex *vertex = new R3TriangleVertex(R3Point(x, y, z));
      verts.Insert(vertex);
    }
    else if (!strcmp(keyword, "vt")) {
      // Read texture coordinates
      double u, v;
      if (sscanf(bufferp, "%s%lf%lf", keyword, &u, &v) != 3) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Create texture coordinates
      R2Point *vt = new R2Point(u, v);
      texture_coords.Insert(vt);
    }
    else if (!strcmp(keyword, "f")) {
      // Read vertex indices
      int quad = 1;
      char s1[128], s2[128], s3[128], s4[128] = { '\0' };
      if (sscanf(bufferp, "%s%s%s%s%s", keyword, s1, s2, s3, s4) != 5) {
        quad = 0;;
        if (sscanf(bufferp, "%s%s%s%s", keyword, s1, s2, s3) != 4) {
          RNFail("Syntax error on line %d in file %s", line_count, filename);
          return 0;
        }
      }

      // Parse vertex indices
      int vi1 = -1, vi2 = -1, vi3 = -1, vi4 = -1;
      int ti1 = -1, ti2 = -1, ti3 = -1, ti4 = -1;
      char *p1 = strchr(s1, '/'); 
      if (p1) { *p1 = 0; vi1 = atoi(s1); p1++; if (*p1) ti1 = atoi(p1); }
      else { vi1 = atoi(s1); ti1 = vi1; }
      char *p2 = strchr(s2, '/'); 
      if (p2) { *p2 = 0; vi2 = atoi(s2); p2++; if (*p2) ti2 = atoi(p2); }
      else { vi2 = atoi(s2); ti2 = vi2; }
      char *p3 = strchr(s3, '/'); 
      if (p3) { *p3 = 0; vi3 = atoi(s3); p3++; if (*p3) ti3 = atoi(p3); }
      else { vi3 = atoi(s3); ti3 = vi3; }
      if (quad) {
        char *p4 = strchr(s4, '/'); 
        if (p4) { *p4 = 0; vi4 = atoi(s4); p4++; if (*p4) ti4 = atoi(p4); }
        else { vi4 = atoi(s4); ti4 = vi4; }
      }

      // Get vertices
      R3TriangleVertex *v1 = verts.Kth(vi1-1);
      R3TriangleVertex *v2 = verts.Kth(vi2-1);
      R3TriangleVertex *v3 = verts.Kth(vi3-1);
      R3TriangleVertex *v4 = (quad) ? verts.Kth(vi4-1) : NULL;
      
      // Assign texture coordinates
      if ((ti1 >= 0) && (ti1 < texture_coords.NEntries())) v1->SetTextureCoords(*(texture_coords.Kth(ti1-1)));
      if ((ti2 >= 0) && (ti2 < texture_coords.NEntries())) v2->SetTextureCoords(*(texture_coords.Kth(ti2-1)));
      if ((ti3 >= 0) && (ti3 < texture_coords.NEntries())) v3->SetTextureCoords(*(texture_coords.Kth(ti3-1)));
      if (quad) {
        if ((ti4 >= 0) && (ti4 < texture_coords.NEntries())) v4->SetTextureCoords(*(texture_coords.Kth(ti4-1)));
      }

      // Check vertices
      if ((v1 == v2) || (v2 == v3) || (v1 == v3)) continue;
      if ((quad) && ((v4 == v1) || (v4 == v2) || (v4 == v3))) quad = 0;

      // Create default material, if needed
      if (material_index == -1) {
        R3Brdf *brdf = new R3Brdf(RNRgb(0.2, 0.2, 0.2), RNRgb(0.8, 0.8, 0.8), 
          RNRgb(0.0, 0.0, 0.0), RNRgb(0.0, 0.0, 0.0), 0.2, 1.0, 1.0);
        R3Material *material = new R3Material(brdf, "Default");
        materials.Insert(material);
        RNArray<R3Triangle *> *mat_tris = new RNArray<R3Triangle *>();
        material_triangles.Insert(mat_tris);
        material_index = 0;
      }

      // Get material
      assert(material_index >= 0);
      R3Material *material = materials.Kth(material_index);

      // Create first triangle
      R3Triangle *triangle = new R3Triangle(v1, v2, v3);
      tris.Insert(triangle);
      triangle_materials.Insert(material);
      material_triangles[material_index]->Insert(triangle);

      // Create second triangle
      if (quad) {
        R3Triangle *triangle = new R3Triangle(v1, v3, v4);
        tris.Insert(triangle);
        triangle_materials.Insert(material);
        material_triangles[material_index]->Insert(triangle);
      }
    }
    else if (!strcmp(keyword, "mtllib")) {
      // Read fields
      char mtlname[1024];
      if (sscanf(bufferp, "%s%s", keyword, mtlname) != 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Read materials
      if (!ReadObjMtlFile(dirname, mtlname)) return 0;
    }
    else if (!strcmp(keyword, "usemtl")) {
      // Read fields
      char mtlname[1024];
      if (sscanf(bufferp, "%s%s", keyword, mtlname) != 2) {
        RNFail("Syntax error on line %d in file %s", line_count, filename);
        return 0;
      }

      // Find material
      material_index = FindMaterialIndex(materials, mtlname);
      if (material_index == -1) {
        fprintf(stderr, "Unable to find material %s at on line %d in file %s", mtlname, line_count, filename);
        return 0;
      }
    }
  }

  // Create triangle array
  triangles = new R3TriangleArray(verts, tris);

  // Delete texture coordinates
  for (int i = 0; i < texture_coords.NEntries(); i++) {
    R2Point *vt = texture_coords.Kth(i);
    delete vt;
  }

  // Close file
  fclose(fp);

  // Return success
  return 1;
}



int R3Model::
WriteObjFile(const char *filename) const
{
  // Open file
  FILE *fp;
  if (!(fp = fopen(filename, "w"))) {
    RNFail("Unable to open file %s", filename);
    return 0;
  }

  RNAbort("Not implemented");

  // Close file
  fclose(fp);

  // Return success
  return 1;
}









