/* Include file for the R3 model class */



/* Initialization functions */

int R3InitModel();
void R3StopModel();



/* Class definitions */

class R3Model {
public:
  // Constructor functions
  R3Model(void);
  R3Model(R3TriangleArray *triangles, 
    const RNArray<R3Material *>& materials,
    const RNArray<R3Material *>& triangle_materials,
    const RNArray<RNArray<R3Triangle *> *>& material_triangles);
  ~R3Model(void);

  // Properties
  R3Box BBox(void) const;
  R3Point Centroid(void) const;

  // Access functions
  int NVertices(void) const;
  R3TriangleVertex *Vertex(int k) const;
  int NTriangles(void) const;
  R3Triangle *Triangle(int k) const;
  int NMaterials(void) const;
  R3Material *Material(int k) const;

  // Triangle property functions
  R3Material *TriangleMaterial(int triangle_index) const;

  // Ray intersection functions
  RNBoolean Intersects(const R3Ray& ray, 
    R3Point *hit_point = NULL, R3Vector *hit_normal = NULL, 
    RNScalar *hit_t = NULL, int *hit_triangle_index = NULL) const;

  // Draw functions
  void Draw(const R3DrawFlags draw_flags = R3_DEFAULT_DRAW_FLAGS) const;

  // I/O functions
  int ReadFile(const char *filename);
  int ReadObjFile(const char *filename);
  int WriteFile(const char *filename) const;
  int WriteObjFile(const char *filename) const;

private:
  // Internal functions
  int ReadObjMtlFile(const char *dirname, const char *filename);

 private:
  R3TriangleArray *triangles;
  RNArray<R3Material *> materials; // List of materials (each material appears once)
  RNArray<R3Material *> triangle_materials; // Mapping from triangle index to material
  RNArray<RNArray<R3Triangle *> *> material_triangles; // List of triangles for each material
  unsigned int opengl_id, opengl_id2;
};



/* Inline functions */

inline R3Box R3Model::
BBox(void) const
{
  // Return bounding box
  if (!triangles) return R3null_box;
  return triangles->BBox();
}



inline R3Point R3Model::
Centroid(void) const
{
  // Return centroid
  if (!triangles) return R3zero_point;
  return triangles->Centroid();
}



inline int R3Model::
NVertices(void) const
{
  // Return number of vertices
  if (!triangles) return 0;
  return triangles->NVertices();
}



inline R3TriangleVertex *R3Model::
Vertex(int k) const
{
  // Return Kth vertex
  if (!triangles) return NULL;
  return triangles->Vertex(k);
}



inline int R3Model::
NTriangles(void) const
{
  // Return number of triangles
  if (!triangles) return 0;
  return triangles->NTriangles();
}



inline R3Triangle *R3Model::
Triangle(int k) const
{
  // Return Kth triangle
  if (!triangles) return NULL;
  return triangles->Triangle(k);
}



inline int R3Model::
NMaterials(void) const
{
  // Return number of materials
  return materials.NEntries();
}


inline R3Material *R3Model::
Material(int k) const
{
  // Return Kth material
  return materials.Kth(k);
}



inline R3Material *R3Model::
TriangleMaterial(int triangle_index) const
{
  // Return material for triangle
  if ((triangle_index < 0) || (triangle_index >= triangle_materials.NEntries())) return NULL;
  return triangle_materials.Kth(triangle_index);
}



