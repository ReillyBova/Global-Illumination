/* Source file for the R3 light class */

/* Include files */

#include "R3Graphics.h"

/* Public variables */

R3AreaLight R3null_area_light;

/* Class type definitions */

RN_CLASS_TYPE_DEFINITIONS(R3AreaLight);



/* Public functions */

int
R3InitAreaLight()
{
    /* Return success */
    return TRUE;
}



void
R3StopAreaLight()
{
}



R3AreaLight::
R3AreaLight(void)
{
}



R3AreaLight::
R3AreaLight(const R3AreaLight& light)
    : R3Light(light),
      circle(light.circle),
      constant_attenuation(light.constant_attenuation),
      linear_attenuation(light.linear_attenuation),
      quadratic_attenuation(light.quadratic_attenuation)
{
}



R3AreaLight::
R3AreaLight(const R3Point& position, RNLength radius, const R3Vector& direction, const RNRgb& color,
	     RNScalar intensity, RNBoolean active,
             RNScalar ca, RNScalar la, RNScalar qa)
    : R3Light(color, intensity, active),
      circle(position, radius, direction),
      constant_attenuation(ca),
      linear_attenuation(la),
      quadratic_attenuation(qa)
{
}




void R3AreaLight::
SetPosition(const R3Point& position)
{
    // Set position
    this->circle.Reset(position, Radius(), Direction());
}



void R3AreaLight::
SetDirection(const R3Vector& direction)
{
    // Set direction
    this->circle.Reset(Position(), Radius(), direction);
}



void R3AreaLight::
SetRadius(RNLength radius)
{
    // Set radius
    this->circle.Reset(Position(), radius, Direction());
}



void R3AreaLight::
SetConstantAttenuation(RNScalar ca)
{
  // Set constant coefficient of attenuation
  this->constant_attenuation = ca;
}



void R3AreaLight::
SetLinearAttenuation(RNScalar la)
{
  // Set linear coefficient of attenuation
  this->linear_attenuation = la;
}



void R3AreaLight::
SetQuadraticAttenuation(RNScalar qa)
{
  // Set quadratic coefficient of attenuation
  this->quadratic_attenuation = qa;
}

RNRgb R3AreaLight::
DiffuseReflection(const R3Brdf& brdf,
    const R3Point& point, const R3Vector& normal, const int max_samples) const
{
    // Check if light is active
    if (!IsActive()) return RNblack_rgb;

    // Get material properties
    const RNRgb& Dc = brdf.Diffuse();

    // Get light properties
    const RNRgb& Ic = Color();

    // Get circle axes
    R3Vector direction = circle.Normal();
    R3Point center = Position();

    // Check normal direction
    if (direction.Dot(point - center) < 0) {
      return RNblack_rgb;
    }

    RNDimension dim = direction.MinDimension();
    R3Vector axis1 = direction % R3xyz_triad[dim];
    axis1.Normalize();
    R3Vector axis2 = direction % axis1;
    axis2.Normalize();

    // Scale axes
    axis1 *= Radius();
    axis2 *= Radius();

    // Sample points on light source
    int sample_count = 0;
    RNRgb sample_sum = RNblack_rgb;
    RNScalar r1;
    RNScalar r2;
    R3Point sample_point;
    RNScalar I;
    RNLength d;
    RNScalar denom;
    R3Vector L;
    RNScalar NL;
    RNRgb diffuse;
    while (sample_count < max_samples) {
        // Sample point in circle
        r1 = (RNThreadableRandomScalar()*2.0) - 1.0;
        r2 = (RNThreadableRandomScalar()*2.0) - 1.0;
        if (r1*r1 + r2*r2 > 1) continue;

        sample_point = center;
        sample_point += r1 * axis1;
        sample_point += r2 * axis2;
        sample_count++;

        // Compute intensity at point
        I = Intensity();
        d = R3Distance(point, sample_point);
        denom = constant_attenuation;
        denom += d * linear_attenuation;
        denom += d * d * quadratic_attenuation;
        if (RNIsPositive(denom)) I /= denom;

        // Compute direction at point
        L = sample_point - point;
        L.Normalize();

        // Weight intensity by probability of area light emission direction
        I *= direction.Dot(-L) * 2.0;

        // Compute diffuse reflection from sample point
        NL = normal.Dot(L);
        diffuse = (I * abs(NL)) * Dc * Ic;

        // Add to result
        sample_sum += diffuse;
    }

    // Return diffuse reflection from area
    RNArea area = circle.Area();
    RNRgb sample_mean = sample_sum;
    if (sample_count > 0) sample_mean /= sample_count;
    return area * sample_mean;
}

RNRgb R3AreaLight::
DiffuseReflection(const R3Brdf& brdf, const R3Point& point, const R3Vector& normal) const
{
  return DiffuseReflection(brdf, point, normal, 16);
}

RNRgb R3AreaLight::
SpecularReflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal, int max_samples) const
{
    max_samples *= 2;
    // Check if light is active
    if (!IsActive()) return RNblack_rgb;

    // Get material properties
    const RNRgb& Sc = brdf.Specular();
    RNScalar s = brdf.Shininess();

    // Get light properties
    const RNRgb& Ic = Color();

    // Get circle axes
    R3Vector direction = circle.Normal();
    R3Point center = Position();

    // Check normal direction
    if (direction.Dot(point - center) < 0) {
      return RNblack_rgb;
    }

    RNDimension dim = direction.MinDimension();
    R3Vector axis1 = direction % R3xyz_triad[dim];
    axis1.Normalize();
    R3Vector axis2 = direction % axis1;
    axis2.Normalize();

    // Scale axes
    axis1 *= Radius();
    axis2 *= Radius();

    // Sample points on light source
    int sample_count = 0;
    RNRgb sample_sum = RNblack_rgb;
    RNScalar r1;
    RNScalar r2;
    R3Point sample_point;
    RNScalar I;
    RNLength d;
    RNScalar denom;
    R3Vector L;
    RNScalar NL;
    R3Vector R;
    R3Vector V;
    RNScalar VR;
    for (int i = 0; i < max_samples; i++) {
        // Sample point in circle
        r1 = (RNThreadableRandomScalar()*2.0) - 1.0;
        r2 = (RNThreadableRandomScalar()*2.0) - 1.0;
        if (r1*r1 + r2*r2 > 1) continue;
        sample_point = center;
        sample_point += r1 * axis1;
        sample_point += r2 * axis2;
        sample_count++;

        // Compute intensity at point
        I = Intensity();
        d = R3Distance(point, sample_point);
        denom = constant_attenuation;
        denom += d * linear_attenuation;
        denom += d * d * quadratic_attenuation;
        if (RNIsPositive(denom)) I /= denom;

        // Compute direction at point
        L = sample_point - point;
        L.Normalize();

        // Weight intensity by probability of area light emission direction
        I *= direction.Dot(-L) * 2.0;

        // Compute specular reflection from sample_point
        NL = normal.Dot(L);
        R = (2.0 * NL) * normal - L;
        V = eye - point;
        V.Normalize();
        VR = V.Dot(R);
        if (RNIsNegativeOrZero(VR)) continue;

        // Return specular component of reflection
        sample_sum += (I * pow(VR,s) * Sc * Ic);
    }

    // Return specular reflection from area
    RNArea area = circle.Area();
    RNRgb sample_mean = sample_sum;
    if (sample_count > 0) sample_mean /= sample_count;
    return area * sample_mean;
}

RNRgb R3AreaLight::
SpecularReflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal) const
{
  return SpecularReflection(brdf, eye, point, normal, 16);
}

RNRgb R3AreaLight::
Reflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal, int max_samples) const
{
    // Return total reflection
    RNRgb diffuse = DiffuseReflection(brdf, point, normal, max_samples);
    RNRgb specular = SpecularReflection(brdf, eye, point, normal, max_samples);
    return diffuse + specular;
}

RNRgb R3AreaLight::
Reflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal) const
{
    // Return total reflection
    RNRgb diffuse = DiffuseReflection(brdf, point, normal, 16);
    RNRgb specular = SpecularReflection(brdf, eye, point, normal, 16);
    return diffuse + specular;
}

void R3AreaLight::
Draw(int i) const
{
    // Draw light
    GLenum index = (GLenum) (GL_LIGHT2 + i);
    if (index > GL_LIGHT7) return;
    GLfloat buffer[4];
    buffer[0] = Intensity() * Color().R();
    buffer[1] = Intensity() * Color().G();
    buffer[2] = Intensity() * Color().B();
    buffer[3] = 1.0;
    glLightfv(index, GL_DIFFUSE, buffer);
    glLightfv(index, GL_SPECULAR, buffer);
    buffer[0] = Position().X();
    buffer[1] = Position().Y();
    buffer[2] = Position().Z();
    buffer[3] = 1.0;
    glLightfv(index, GL_POSITION, buffer);
    buffer[0] = 90.0;
    glLightf(index, GL_SPOT_CUTOFF, buffer[0]);
    glEnable(index);
    buffer[0] = ConstantAttenuation();
    buffer[1] = LinearAttenuation();
    buffer[2] = QuadraticAttenuation();
    glLightf(index, GL_CONSTANT_ATTENUATION, buffer[0]);
    glLightf(index, GL_LINEAR_ATTENUATION, buffer[1]);
    glLightf(index, GL_QUADRATIC_ATTENUATION, buffer[2]);
}
