/* Source file for the R3 light class */

/* Include files */

#include "R3Graphics.h"

/* Public variables */

R3RectLight R3null_rect_light;



/* Class type definitions */
RN_CLASS_TYPE_DEFINITIONS(R3RectLight);

/* Public functions */
int
R3InitRectLight()
{
    /* Return success */
    return TRUE;
}

void
R3StopRectLight()
{
}

R3RectLight::
R3RectLight(void)
{
}

R3RectLight::
R3RectLight(const R3RectLight& light)
    : R3Light(light),
      pos(light.pos),
      axis1(light.axis1),
      axis2(light.axis2),
      norm(light.norm),
      len1(light.len1),
      len2(light.len2),
      constant_attenuation(light.constant_attenuation),
      linear_attenuation(light.linear_attenuation),
      quadratic_attenuation(light.quadratic_attenuation)
{
}



R3RectLight::
R3RectLight(const R3Point& position, const R3Vector& axis1, const R3Vector& axis2,
    RNLength len1, RNLength len2, const RNRgb& color,
    RNScalar intensity, RNBoolean active, RNScalar ca, RNScalar la, RNScalar qa)
    : R3Light(color, intensity, active),
      pos(position),
      len1(len1),
      len2(len2),
      constant_attenuation(ca),
      linear_attenuation(la),
      quadratic_attenuation(qa)
{
  R3Vector a1 = axis1;
  a1.Normalize();
  R3Vector a2 = axis2;
  a2.Normalize();
  R3Vector n = a1 % a2;
  n.Normalize();

  this->axis1 = a1;
  this->axis2 = a2;
  this->norm = n;
}

void R3RectLight::
SetPosition(const R3Point& position)
{
    // Set position
    this->pos = position;
}

void R3RectLight::
SetPrimaryAxis(const R3Vector& axis)
{
    R3Vector a1 = axis;
    a1.Normalize();
    // Set axis 1
    this->axis1 = a1;

    // Update norm
    R3Vector n = a1 % SecondaryAxis();
    n.Normalize();
    this->norm = n;
}

void R3RectLight::
SetSecondaryAxis(const R3Vector& axis)
{
    R3Vector a2 = axis;
    a2.Normalize();
    // Set axis 2
    this->axis2 = a2;

    // Update norm
    R3Vector n = PrimaryAxis() % a2;
    n.Normalize();
    this->norm = n;
}

void R3RectLight::
SetPrimaryLength(RNLength len)
{
    // Set len 1
    this->len1 = len;
}

void R3RectLight::
SetSecondaryLength(RNLength len)
{
    // Set len 2
    this->len2 = len;
}

void R3RectLight::
SetConstantAttenuation(RNScalar ca)
{
  // Set constant coefficient of attenuation
  this->constant_attenuation = ca;
}



void R3RectLight::
SetLinearAttenuation(RNScalar la)
{
  // Set linear coefficient of attenuation
  this->linear_attenuation = la;
}



void R3RectLight::
SetQuadraticAttenuation(RNScalar qa)
{
  // Set quadratic coefficient of attenuation
  this->quadratic_attenuation = qa;
}

RNRgb R3RectLight::
DiffuseReflection(const R3Brdf& brdf,
    const R3Point& point, const R3Vector& normal, int max_samples) const
{
    max_samples = ceil(max_samples/2);
    // Check if light is active
    if (!IsActive()) return RNblack_rgb;

    // Get material properties
    const RNRgb& Dc = brdf.Diffuse();

    // Get light properties
    const RNRgb& Ic = Color();

    // Get rectangle axes
    R3Vector direction = Direction();
    R3Point center = Position();

    // Check normal direction
    if (direction.Dot(point - center) < 0) {
      return RNblack_rgb;
    }

    R3Vector a1 = PrimaryAxis() * PrimaryLength();
    R3Vector a2 = SecondaryAxis() * SecondaryLength();

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
        // Sample point in square
        r1 = RNThreadableRandomScalar() - 0.5;
        r2 = RNThreadableRandomScalar() - 0.5;
        sample_point = center;
        sample_point += r1 * a1;
        sample_point += r2 * a2;
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
        NL = normal.Dot(L);
        diffuse = (I * abs(NL) / RN_PI) * Dc * Ic;

        // Add to result
        sample_sum += diffuse;
    }

    // Return diffuse reflection from area
    RNArea area = (a1 % a2).Length();
    RNRgb sample_mean = sample_sum;
    if (sample_count > 0) sample_mean /= sample_count;
    return area * sample_mean;
}

RNRgb R3RectLight::
DiffuseReflection(const R3Brdf& brdf, const R3Point& point, const R3Vector& normal) const
{
  return DiffuseReflection(brdf, point, normal, 16);
}

RNRgb R3RectLight::
SpecularReflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal, const int max_samples) const
{
    // Check if light is active
    if (!IsActive()) return RNblack_rgb;

    // Get material properties
    const RNRgb& Sc = brdf.Specular();
    RNScalar s = brdf.Shininess();

    // Get light properties
    const RNRgb& Ic = Color();

    // Get rectangle axes
    R3Vector direction = Direction();
    R3Point center = Position();

    // Check normal direction
    if (direction.Dot(point - center) < 0) {
      return RNblack_rgb;
    }

    R3Vector a1 = PrimaryAxis() * PrimaryLength();
    R3Vector a2 = SecondaryAxis() * SecondaryLength();

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
        // Sample point in square
        r1 = RNThreadableRandomScalar() - 0.5;
        r2 = RNThreadableRandomScalar() - 0.5;
        sample_point = center;
        sample_point += r1 * a1;
        sample_point += r2 * a2;
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
        // NB: (s + 2.0) / (RN_TWO_PI) term is too noisy so it is excluded
        sample_sum += (I * pow(VR,s) * Sc * Ic);
    }

    // Return specular reflection from area
    RNArea area = (a1 % a2).Length();
    RNRgb sample_mean = sample_sum;
    if (sample_count > 0) sample_mean /= sample_count;
    return area * sample_mean;
}

RNRgb R3RectLight::
SpecularReflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal) const
{
  return SpecularReflection(brdf, eye, point, normal, 16);
}

RNRgb R3RectLight::
Reflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal, const int max_samples) const
{
    // Return total reflection
    RNRgb diffuse = DiffuseReflection(brdf, point, normal, max_samples);
    RNRgb specular = SpecularReflection(brdf, eye, point, normal, max_samples);
    return diffuse + specular;
}

RNRgb R3RectLight::
Reflection(const R3Brdf& brdf, const R3Point& eye,
    const R3Point& point, const R3Vector& normal) const
{
    // Return total reflection
    RNRgb diffuse = DiffuseReflection(brdf, point, normal, 16);
    RNRgb specular = SpecularReflection(brdf, eye, point, normal, 16);
    return diffuse + specular;
}

void R3RectLight::
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
