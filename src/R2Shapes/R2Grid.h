// Header file for GAPS scalar grid class



// Class definition

class R2Grid {
public:
  // Constructors
  R2Grid(int xresolution = 0, int yresolution = 0);
  R2Grid(int xresolution, int yresolution, const R2Box& bbox);
  R2Grid(int xresolution, int yresolution, const R2Affine& world_to_grid);
  R2Grid(const R2Grid& grid, int x1, int y1, int x2, int y2);
  R2Grid(const R2Grid& grid);
  R2Grid(const R2Image& image, int dummy);
  ~R2Grid(void);

  // Grid property functions
  int NEntries() const;
  int XResolution(void) const;
  int YResolution(void) const;
  int Resolution(RNDimension dim) const;
  RNScalar Sum(void) const;
  RNScalar Mean(void) const;
  RNScalar Median(void) const;
  RNScalar Maximum(void) const;
  RNScalar Minimum(void) const;
  RNInterval Range(void) const;
  RNScalar L1Norm(void) const;
  RNScalar L2Norm(void) const;
  RNScalar Area(void) const;
  int Cardinality(void) const;
  R2Box GridBox(void) const;
  R2Box WorldBox(void) const;

  // Transformation property functions
  const R2Affine& WorldToGridTransformation(void) const;
  const R2Affine& GridToWorldTransformation(void) const;
  RNScalar WorldToGridScaleFactor(void) const;
  RNScalar GridToWorldScaleFactor(void) const;

  // Grid value access functions
  RNScalar GridValue(int index) const;
  RNScalar GridValue(int i, int j) const;
  RNScalar GridValue(RNCoord x, RNCoord y) const;
  RNScalar GridValue(const R2Point& grid_point) const;
  RNScalar WorldValue(RNCoord x, RNCoord y) const;
  RNScalar WorldValue(const R2Point& world_point) const;
  RNScalar& operator()(int i, int j);
  RNScalar& operator()(int i);

  // Grid manipulation functions
  void Abs(void);
  void Sqrt(void);
  void Square(void);
  void Negate(void);
  void Invert(void);
  void Transpose(void);
  void Normalize(void);
  void Laplacian(void);
  void Laplacian(RNDimension dim);
  void Sobel(void);
  void GradientAngle(void);
  void GradientMagnitude(void);
  void Gradient(RNDimension dim);
  void Hessian(RNDimension dim1, RNDimension dim2);
  void Clear(RNScalar value = 0);
  void DetectEdges(void);
  void DetectCorners(void);
  void FillHoles(void);
  void FillHoles(int max_hole_size);
  void Dilate(RNScalar grid_distance);
  void Erode(RNScalar grid_distance);
  void Blur(RNScalar grid_sigma = 2);
  void Blur(RNDimension dim, RNScalar grid_sigma);
  void AddNoise(RNScalar sigma_fraction = 0.05);
  void HarrisCornerFilter(int grid_radius = 3, RNScalar kappa = 0.05);
  void BilateralFilter(RNLength grid_sigma = 2, RNScalar value_sigma = -1);
  void AnisotropicDiffusion(RNLength grid_sigma = 2, RNScalar gradient_sigma = -1);
  void PercentileFilter(RNLength grid_radius, RNScalar percentile);
  void MinFilter(RNLength grid_radius);
  void MaxFilter(RNLength grid_radius);
  void MedianFilter(RNLength grid_radius);
  void MaskNonMinima(RNLength grid_radius = 0);
  void MaskNonMaxima(RNLength grid_radius = 0);
  void Convolve(const RNScalar filter[3][3]);
  void Substitute(RNScalar old_value, RNScalar new_value);
  void Add(RNScalar value);
  void Add(const R2Grid& grid);
  void Subtract(RNScalar value);
  void Subtract(const R2Grid& grid);
  void Multiply(RNScalar value);
  void Multiply(const R2Grid& grid);
  void Divide(RNScalar value);
  void Divide(const R2Grid& grid);
  void Pow(RNScalar exponent);
  void Mask(const R2Grid& grid);
  void Overlay(const R2Grid& grid);
  void Threshold(RNScalar threshold, RNScalar low, RNScalar high);
  void Threshold(const R2Grid& threshold, RNScalar low, RNScalar high);
  void SignedDistanceTransform(void);
  void SquaredDistanceTransform(void);
  void Voronoi(R2Grid *squared_distance_grid = NULL);
  void PointSymmetryTransform(int radius = -1);
  void Gauss(RNLength sigma = sqrt(8.0), RNBoolean square = TRUE);
  void Resample(int xres, int yres);
  void PadWithZero(int xres, int yres);
  void SetGridValue(int index, RNScalar value);
  void SetGridValue(int i, int j, RNScalar value);
  void AddGridValue(int i, int j, RNScalar value);

  // Arithmetic operators
  R2Grid& operator=(const R2Grid& grid);
  R2Grid& operator+=(RNScalar scale);
  R2Grid& operator+=(const R2Grid& grid);
  R2Grid& operator-=(RNScalar scale);
  R2Grid& operator-=(const R2Grid& grid);
  R2Grid& operator*=(RNScalar scale);
  R2Grid& operator*=(const R2Grid& grid);
  R2Grid& operator/=(RNScalar scale);
  R2Grid& operator/=(const R2Grid& grid);

  // Rasterization functions
  void RasterizeGridPoint(RNCoord x, RNCoord y, RNScalar value);
  void RasterizeWorldPoint(RNCoord x, RNCoord y, RNScalar value);
  void RasterizeGridPoint(const R2Point& point, RNScalar value);
  void RasterizeWorldPoint(const R2Point& point, RNScalar value);
  void RasterizeGridSpan(const R2Point& p1, const R2Point& p2, RNScalar value);
  void RasterizeWorldSpan(const R2Point& p1, const R2Point& p2, RNScalar value);
  void RasterizeGridSpan(const int p1[3], const int p2[3], RNScalar value1, RNScalar value2);
  void RasterizeGridSpan(const R2Point& p1, const R2Point& p2, RNScalar value1, RNScalar value2);
  void RasterizeWorldSpan(const R2Point& p1, const R2Point& p2, RNScalar value1, RNScalar value2);
  void RasterizeGridBox(const int p1[3], const int p2[3], RNScalar value);
  void RasterizeGridBox(const R2Point& p1, const R2Point& p2, RNScalar value);
  void RasterizeWorldBox(const R2Point& p1, const R2Point& p2, RNScalar value);
  void RasterizeGridTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value);
  void RasterizeWorldTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value);
  void RasterizeGridTriangle(const int p1[3], const int p2[3], const int p3[3], RNScalar value1, RNScalar value2, RNScalar value3);
  void RasterizeGridTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value1, RNScalar value2, RNScalar value3);
  void RasterizeWorldTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value1, RNScalar value2, RNScalar value3);
  void RasterizeGridCircle(const R2Point& center, RNLength radius, RNScalar value);
  void RasterizeWorldCircle(const R2Point& center, RNLength radius, RNScalar value);
  void RasterizeGridPolygon(const R2Polygon& polygon, RNScalar value);
  void RasterizeWorldPolygon(const R2Polygon& polygon, RNScalar value);

  // Relationship functions
  RNScalar Dot(const R2Grid& grid) const;
  RNScalar L1Distance(const R2Grid& grid) const;
  RNScalar L2Distance(const R2Grid& grid) const;
  RNScalar L2DistanceSquared(const R2Grid& grid) const;

  // Transformation manipulation functions
  void SetWorldToGridTransformation(const R2Affine& affine);
  void SetWorldToGridTransformation(const R2Box& world_box);
  void SetWorldToGridTransformation(const R2Point& world_origin, const R2Vector& world_xaxis, RNLength world_radius);

  // Transformation utility functions
  R2Point WorldPosition(const R2Point& grid_point) const;
  R2Point GridPosition(const R2Point& world_point) const;
  R2Point WorldPosition(RNCoord x, RNCoord y) const;
  R2Point GridPosition(RNCoord x, RNCoord y) const;

  // Reading/writing
  int ReadFile(const char *filename);
  int ReadPFMFile(const char *filename);
  int ReadRAWFile(const char *filename);
  int ReadGridFile(const char *filename);
  int ReadPNGFile(const char *filename);
  int ReadImage(const char *filename);
  int WriteFile(const char *filename) const;
  int WritePFMFile(const char *filename) const;
  int WriteRAWFile(const char *filename) const;
  int WriteGridFile(const char *filename) const;
  int WritePNGFile(const char *filename) const;
  int WriteImage(const char *filename) const;
  int ReadGrid(FILE *fp = NULL);
  int WriteGrid(FILE *fp = NULL) const;
  int Print(FILE *fp = NULL) const;
  void Capture(void);

  // Draw functions
  void Draw(void) const;
  void DrawMesh(void) const;
  void DrawImage(int x = 0, int y = 0) const;

  // Utility functions
  RNScalar GridValue(RNCoord x, RNCoord y, RNLength sigma) const;
  void ConnectedComponentLabelFilter(RNScalar isolevel);
  void ConnectedComponentSizeFilter(RNScalar isolevel);
  void ConnectedComponentCentroidFilter(RNScalar isolevel);
  void ConnectedComponentFilter(RNScalar isolevel, RNArea min_grid_area, RNArea max_grid_area, 
    RNScalar under_isolevel_value = 0, RNScalar too_small_value = 0, RNScalar too_large_value = 0);
  int ConnectedComponents(RNScalar isolevel = 0, int max_components = 0, int *seeds = NULL, int *sizes = NULL, int *grid_components = NULL);
  int GenerateIsoContour(RNScalar isolevel, R2Point *points, int max_points) const;

  // Debugging functions
  const RNScalar *GridValues(void) const;
  void IndicesToIndex(int i, int j, int& index) const;
  void IndexToIndices(int index, int& i, int& j) const;

  // Temporary (for backwards compatibility)
  int Read(const char *filename) { return ReadFile(filename); };
  int Write(const char *filename) const { return WriteFile(filename); };

private:
  R2Affine grid_to_world_transform;
  R2Affine world_to_grid_transform;
  RNScalar world_to_grid_scale_factor;
  RNScalar *grid_values;
  int grid_resolution[2];
  int grid_row_size;
  int grid_size;
};



// Useful constants

const float R2_GRID_KEEP_VALUE = -987;
const float R2_GRID_INPUT_VALUE = -654;
const float R2_GRID_UNKNOWN_VALUE = -321;



// Inline functions

inline int R2Grid::
NEntries(void) const
{
  // Return total number of entries
  return grid_size;
}



inline int R2Grid::
XResolution(void) const
{
  // Return resolution in X dimension
  return grid_resolution[RN_X];
}



inline int R2Grid::
YResolution(void) const
{
  // Return resolution in Y dimension
  return grid_resolution[RN_Y];
}




inline int R2Grid::
Resolution(RNDimension dim) const
{
  // Return resolution in dimension
  assert((0 <= dim) && (dim <= 2));
  return grid_resolution[dim];
}




inline RNScalar R2Grid::
Sum(void) const
{
  // Return sum of all grid values
  return L1Norm();
}



inline RNScalar R2Grid::
Minimum(void) const
{
  // Return smallest value
  return Range().Min();
}



inline RNScalar R2Grid::
Maximum(void) const
{
  // Return largest value
  return Range().Max();
}



inline RNScalar R2Grid::
Area(void) const
{
  // Find volume of non-zero values
  RNScalar scale = 1.0 / world_to_grid_scale_factor;
  return Cardinality() * scale * scale;
}



inline R2Box R2Grid::
GridBox(void) const
{
  // Return bounding box in grid coordinates
  return R2Box(0, 0, grid_resolution[0]-1, grid_resolution[1]-1);
}



inline R2Box R2Grid::
WorldBox(void) const
{
  // Return bounding box in world coordinates
  R2Point p1(0, 0);
  R2Point p2(grid_resolution[0]-1, grid_resolution[1]-1);
  return R2Box(WorldPosition(p1), WorldPosition(p2));
}



inline const R2Affine& R2Grid::
WorldToGridTransformation(void) const
{
  // Return transformation from world coordinates to grid coordinates
  return world_to_grid_transform;
}



inline const R2Affine& R2Grid::
GridToWorldTransformation(void) const
{
  // Return transformation from grid coordinates to world coordinates
  return grid_to_world_transform;
}



inline RNScalar R2Grid::
WorldToGridScaleFactor(void) const
{
  // Return transformation from world coordinates to grid coordinates
  return world_to_grid_scale_factor;
}



inline RNScalar R2Grid::
GridToWorldScaleFactor(void) const
{
  // Return transformation from world coordinates to grid coordinates
  if (RNIsZero(world_to_grid_scale_factor)) return 0.0;
  else return 1.0 / world_to_grid_scale_factor;
}



inline const RNScalar *R2Grid::
GridValues(void) const
{
  // Return pointer to grid values
  return grid_values;
}



inline void R2Grid::
Sobel(void)
{
  // Compute gradient magnitude
  GradientMagnitude();
}



inline void R2Grid::
DetectEdges(void)
{
  // Compute magnitude of gradient (Sobel operator)
  GradientMagnitude();
}



inline void R2Grid::
DetectCorners(void)
{
  // Compute magnitude of corner response
  HarrisCornerFilter();
}



inline void R2Grid::
MinFilter(RNLength grid_radius)
{
  // Set each pixel to be min of neighborhood
  PercentileFilter(grid_radius, 0);
}



inline void R2Grid::
MaxFilter(RNLength grid_radius)
{
  // Set each pixel to be max of neighborhood
  PercentileFilter(grid_radius, 1);
}



inline void R2Grid::
MedianFilter(RNLength grid_radius)
{
  // Set each pixel to be median of neighborhood
  PercentileFilter(grid_radius, 0.5);
}



inline RNScalar& R2Grid::
operator()(int i, int j) 
{
  // Return value at grid point
  assert((0 <= i) && (i < XResolution()));
  assert((0 <= j) && (j < YResolution()));
  return grid_values[j * grid_row_size + i];
}



inline RNScalar& R2Grid::
operator()(int i) 
{
  // Return value at grid point
  assert((0 <= i) && (i < NEntries()));
  return grid_values[i];
}



inline RNScalar R2Grid::
GridValue(int index) const
{
  // Return value at grid point
  assert((0 <= index) && (index < grid_size));
  return grid_values[index];
}



inline RNScalar R2Grid::
GridValue(int i, int j) const
{
  // Return value at grid point
  assert((0 <= i) && (i < XResolution()));
  assert((0 <= j) && (j < YResolution()));
  return grid_values[j * grid_row_size + i];
}



inline RNScalar R2Grid::
GridValue(const R2Point& point) const
{
  // Return value at grid point
  return GridValue(point[0], point[1]);
}



inline RNScalar R2Grid::
WorldValue(const R2Point& point) const
{
  // Return value at world point
  return GridValue(GridPosition(point));
}



inline RNScalar R2Grid::
WorldValue(RNCoord x, RNCoord y) const
{
  // Return value at world point
  return GridValue(GridPosition(x, y));
}



inline R2Grid& R2Grid::
operator+=(RNScalar value) 
{
  // Add value to all grid values 
  Add(value);
  return *this;
}



inline R2Grid& R2Grid::
operator+=(const R2Grid& grid) 
{
  // Add passed grid values to corresponding entries of this grid
  Add(grid);
  return *this;
}



inline R2Grid& R2Grid::
operator-=(RNScalar value) 
{
  // Subtract value from all grid values 
  Subtract(value);
  return *this;
}



inline R2Grid& R2Grid::
operator-=(const R2Grid& grid) 
{
  // Subtract passed grid values from corresponding entries of this grid
  Subtract(grid);
  return *this;
}



inline R2Grid& R2Grid::
operator*=(RNScalar value) 
{
  // Multiply grid values by value
  Multiply(value);
  return *this;
}



inline R2Grid& R2Grid::
operator*=(const R2Grid& grid) 
{
  // Multiply passed grid values by corresponding entries of this grid
  Multiply(grid);
  return *this;
}



inline R2Grid& R2Grid::
operator/=(RNScalar value) 
{
  // Divide grid values by value
  Divide(value);
  return *this;
}



inline R2Grid& R2Grid::
operator/=(const R2Grid& grid) 
{
  // Divide passed grid values by corresponding entries of this grid
  Divide(grid);
  return *this;
}



inline void R2Grid::
SetGridValue(int index, RNScalar value)
{
  // Set value at grid point
  assert((0 <= index) && (index < grid_size));
  grid_values[index] = value;
}



inline void R2Grid::
SetGridValue(int i, int j, RNScalar value)
{
  // Set value at grid point
  assert((0 <= i) && (i < XResolution()));
  assert((0 <= j) && (j < YResolution()));
  (*this)(i, j) = value;
}



inline void R2Grid::
AddGridValue(int i, int j, RNScalar value)
{
  // Add value at grid point
  assert((0 <= i) && (i < XResolution()));
  assert((0 <= j) && (j < YResolution()));
  if ((*this)(i, j) == R2_GRID_UNKNOWN_VALUE) (*this)(i, j) = value;
  else (*this)(i, j) += value;
}



inline void R2Grid::
RasterizeGridPoint(const R2Point& point, RNScalar value)
{
  // Splat value at grid point
  RasterizeGridPoint(point[0], point[1], value);
}



inline void R2Grid::
RasterizeWorldPoint(RNCoord x, RNCoord y, RNScalar value)
{
  // Splat value at world point
  RasterizeGridPoint(GridPosition(x, y), value);
}



inline void R2Grid::
RasterizeWorldPoint(const R2Point& world_point, RNScalar value)
{
  // Splat value at world point
  RasterizeGridPoint(GridPosition(world_point), value);
}



inline void R2Grid::
RasterizeGridSpan(const R2Point& p1, const R2Point& p2, RNScalar value1, RNScalar value2)
{
  // Splat value everywhere inside grid triangle
  int i1[2] = { (int) (p1[0] + 0.5), (int) (p1[1] + 0.5) };
  int i2[2] = { (int) (p2[0] + 0.5), (int) (p2[1] + 0.5) };
  RasterizeGridSpan(i1, i2, value1, value2);
}



inline void R2Grid::
RasterizeGridSpan(const R2Point& p1, const R2Point& p2, RNScalar value)
{
  // Splat value everywhere inside grid triangle
  RasterizeGridSpan(p1, p2, value, value);
}



inline void R2Grid::
RasterizeWorldSpan(const R2Point& p1, const R2Point& p2, RNScalar value1, RNScalar value2)
{
  // Splat value everywhere inside world triangle
  RasterizeGridSpan(GridPosition(p1), GridPosition(p2), value1, value2);
}



inline void R2Grid::
RasterizeWorldSpan(const R2Point& p1, const R2Point& p2, RNScalar value)
{
  // Splat value everywhere inside world triangle
  RasterizeGridSpan(GridPosition(p1), GridPosition(p2), value, value);
}



inline void R2Grid::
RasterizeGridBox(const R2Point& p1, const R2Point& p2, RNScalar value)
{
  // Splat value everywhere inside grid triangle
  int i1[2] = { (int) (p1[0] + 0.5), (int) (p1[1] + 0.5) };
  int i2[2] = { (int) (p2[0] + 0.5), (int) (p2[1] + 0.5) };
  RasterizeGridBox(i1, i2, value);
}



inline void R2Grid::
RasterizeWorldBox(const R2Point& p1, const R2Point& p2, RNScalar value)
{
  // Splat value everywhere inside world triangle
  RasterizeGridBox(GridPosition(p1), GridPosition(p2), value);
}



inline void R2Grid::
RasterizeGridTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value1, RNScalar value2, RNScalar value3)
{
  // Splat value everywhere inside grid triangle
  int i1[2] = { (int) (p1[0] + 0.5), (int) (p1[1] + 0.5) };
  int i2[2] = { (int) (p2[0] + 0.5), (int) (p2[1] + 0.5) };
  int i3[2] = { (int) (p3[0] + 0.5), (int) (p3[1] + 0.5) };
  RasterizeGridTriangle(i1, i2, i3, value1, value2, value3);
}



inline void R2Grid::
RasterizeGridTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value)
{
  // Splat value everywhere inside world triangle
  RasterizeGridTriangle(p1, p2, p3, value, value, value);
}



inline void R2Grid::
RasterizeWorldTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value1, RNScalar value2, RNScalar value3)
{
  // Splat value everywhere inside world triangle
  RasterizeGridTriangle(GridPosition(p1), GridPosition(p2), GridPosition(p3), value1, value2, value3);
}



inline void R2Grid::
RasterizeWorldTriangle(const R2Point& p1, const R2Point& p2, const R2Point& p3, RNScalar value)
{
  // Splat value everywhere inside world triangle
  RasterizeGridTriangle(GridPosition(p1), GridPosition(p2), GridPosition(p3), value, value, value);
}



inline void R2Grid::
RasterizeWorldCircle(const R2Point& center, RNLength radius, RNScalar value)
{
  // Splat value everywhere inside world circle
  RasterizeGridCircle(GridPosition(center), radius * WorldToGridScaleFactor(), value);
}



inline RNScalar R2Grid::
L2Distance(const R2Grid& grid) const
{
  // Return L2 distance between this and grid
  return sqrt(L2DistanceSquared(grid));
}



inline R2Point R2Grid::
WorldPosition(const R2Point& grid_point) const
{
  // Transform point from grid coordinates to world coordinates
  return WorldPosition(grid_point[0], grid_point[1]);
}



inline R2Point R2Grid::
GridPosition(const R2Point& world_point) const
{
  // Transform point from world coordinates to grid coordinates
  return GridPosition(world_point[0], world_point[1]);
}



inline void R2Grid::
IndicesToIndex(int i, int j, int& index) const
{
  // Set index of grid value at (i, j) 
  index = j * grid_row_size + i;
}


inline void R2Grid::
IndexToIndices(int index, int& i, int& j) const
{
  // Set indices of grid value at index
  j = index / grid_row_size;
  i = index % grid_row_size;
}



inline void R2Grid::
Draw(void) const
{
  // Draw image
  DrawImage();
}









