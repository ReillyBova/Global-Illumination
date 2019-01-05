////////////////////////////////////////////////////////////////////////
// Directives
////////////////////////////////////////////////////////////////////////

#include "graphics_utils.h"
#include "../render.h"
#include "../R3Graphics/R3Graphics.h"

////////////////////////////////////////////////////////////////////////
// Color Utils
////////////////////////////////////////////////////////////////////////

// Clamp color channels to [0, 1] range
void ClampColor(RNRgb &color)
{
  for (int i = 0; i < 3; i++) {
    if (color[i] < 0)
      color[i] = 0;
    if (color[i] > 1.0)
      color[i] = 1.0;
  }
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

// Convert color from RGB to Ward's packed RGBE format; copies result into char[4]
// array
void RNRgb_to_RGBE(RNRgb& rgb_src, unsigned char* rgbe_target)
{
  RNScalar max = MaxChannelVal(rgb_src);

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

////////////////////////////////////////////////////////////////////////
// Physics & Geometry Utils
////////////////////////////////////////////////////////////////////////

// Return the length of an intersecting ray
RNLength IntersectionDist(const R3Ray& ray, const R3Point& origin)
{
  R3Point intersection_point;
  if (SCENE->Intersects(ray, NULL, NULL, NULL, &intersection_point)) {
    return R3Distance(origin, intersection_point);
  }

  return RN_INFINITY;
}

// Use Schlick's approximation to return reflection coefficient between media
RNScalar ComputeReflectionCoeff(RNScalar cos_theta, const RNScalar ir_mat)
{
  // Parallel Light (direction agnostic)
  const RNScalar R_o = pow((IR_AIR - ir_mat)/(IR_AIR + ir_mat), 2);
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
    eta = ir_mat / IR_AIR;
    // Need to flip normal (NB: not copy be reference so safe to overwrite)
    normal.Flip();
    cos_theta *= -1.0;
  } else {
    // Entering
    eta = IR_AIR / ir_mat;
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

////////////////////////////////////////////////////////////////////////
// Material Utils
////////////////////////////////////////////////////////////////////////

// Use importance sampling to return a vector sampled from a weighted hemisphere
// around the surface normal (normal is flipped if cos_theta is negative)
R3Vector Diffuse_ImportanceSample(R3Vector normal, const RNScalar cos_theta)
{
  // Check normal direction
  if (cos_theta < 0) {
    normal.Flip();
  }

  // Pick spherical coords
  const RNAngle theta = acos(sqrt(RNThreadableRandomScalar()));
  const RNAngle phi = 2*RN_PI*RNThreadableRandomScalar();

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
  const RNAngle alpha = acos(pow(RNThreadableRandomScalar(), 1.0 / (n + 1.0))) * angle_limit;

  const RNAngle phi = RN_TWO_PI*RNThreadableRandomScalar();

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

////////////////////////////////////////////////////////////////////////
// Light Utils
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
    area = RN_PI*pow(SCENE_RADIUS, 2.0);
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
