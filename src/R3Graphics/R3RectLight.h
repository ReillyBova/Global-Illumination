/* Initialization functions */

int R3InitRectLight();
void R3StopRectLight();



/* Class definition */
class R3RectLight : public R3Light {
    public:
        // Constructor functions
	      R3RectLight(void);
        R3RectLight(const R3RectLight& light);
        R3RectLight(const R3Point& position, const R3Vector& axis1, const R3Vector& axis2,
            RNLength len1, RNLength len2, const RNRgb& color,
            RNScalar intensity = 1.0, RNBoolean active = TRUE,
            RNScalar ca = 0, RNScalar la = 0, RNScalar qa = 1);

      	// Property functions/operators
      	const R3Point& Position(void) const;
      	const R3Vector& Direction(void) const;
        const R3Vector& PrimaryAxis(void) const;
        const R3Vector& SecondaryAxis(void) const;
        const RNLength PrimaryLength(void) const;
        const RNLength SecondaryLength(void) const;
        const RNScalar ConstantAttenuation(void) const;
        const RNScalar LinearAttenuation(void) const;
        const RNScalar QuadraticAttenuation(void) const;

      	// Manipulation functions/operations
      	virtual void SetPosition(const R3Point& position);
        virtual void SetPrimaryAxis(const R3Vector& axis);
        virtual void SetSecondaryAxis(const R3Vector& axis);
      	virtual void SetPrimaryLength(RNLength len);
        virtual void SetSecondaryLength(RNLength len);
        virtual void SetConstantAttenuation(RNScalar ca);
        virtual void SetLinearAttenuation(RNScalar la);
        virtual void SetQuadraticAttenuation(RNScalar qa);

      	// Reflection evaluation functions
      	virtual RNRgb Reflection(const R3Brdf& brdf, const R3Point& eye,
      	    const R3Point& point, const R3Vector& normal, const int max_samples) const;
      	virtual RNRgb DiffuseReflection(const R3Brdf& brdf,
      	    const R3Point& point, const R3Vector& normal, int max_samples) const;
      	virtual RNRgb SpecularReflection(const R3Brdf& brdf, const R3Point& eye,
      	    const R3Point& point, const R3Vector& normal, const int max_samples) const;


        virtual RNRgb Reflection(const R3Brdf& brdf, const R3Point& eye,
      	    const R3Point& point, const R3Vector& normal) const;
      	virtual RNRgb DiffuseReflection(const R3Brdf& brdf,
      	    const R3Point& point, const R3Vector& normal) const;
      	virtual RNRgb SpecularReflection(const R3Brdf& brdf, const R3Point& eye,
      	    const R3Point& point, const R3Vector& normal) const;

      	// Draw functions/operations
        virtual void Draw(int i) const;

      	// Class type definitions
      	RN_CLASS_TYPE_DECLARATIONS(R3RectLight);

    private:
        R3Point  pos;
        R3Vector axis1;
        R3Vector axis2;
        R3Vector norm;
        RNScalar len1;
        RNScalar len2;
        RNScalar constant_attenuation;
        RNScalar linear_attenuation;
        RNScalar quadratic_attenuation;
};



/* Public variables */

extern R3RectLight R3null_rect_light;



/* Inline functions */

inline const R3Point& R3RectLight::
Position(void) const
{
    // Return position
    return pos;
}



inline const R3Vector& R3RectLight::
Direction(void) const
{
    return norm;
}

inline const R3Vector& R3RectLight::
PrimaryAxis(void) const
{
    // Return direction
    return axis1;
}

inline const R3Vector& R3RectLight::
SecondaryAxis(void) const
{
    // Return direction
    return axis2;
}

inline const RNLength R3RectLight::
PrimaryLength(void) const
{
    // Return radius
    return len1;
}

inline const RNLength R3RectLight::
SecondaryLength(void) const
{
    // Return radius
    return len2;
}

inline const RNScalar R3RectLight::
ConstantAttenuation(void) const
{
    // Return constant coefficient of attenuation
    return constant_attenuation;
}



inline const RNScalar R3RectLight::
LinearAttenuation(void) const
{
    // Return linear coefficient of attenuation
    return linear_attenuation;
}



inline const RNScalar R3RectLight::
QuadraticAttenuation(void) const
{
    // Return quadratic coefficient of attenuation
    return quadratic_attenuation;
}
