/*
 * Float-precision wrappers calling double-precision libm.
 * Provides sinf, cosf, sqrtf, etc. by calling the double impls.
 */
extern double sin(double);
extern double cos(double);
extern double tan(double);
extern double atan(double);
extern double atan2(double, double);
extern double sqrt(double);
extern double exp(double);
extern double log(double);
extern double log10(double);
extern double pow(double, double);
extern double fmod(double, double);
extern double fabs(double);
extern double floor(double);

float sinf(float x)  { return (float)sin((double)x); }
float cosf(float x)  { return (float)cos((double)x); }
float tanf(float x)  { return (float)tan((double)x); }
float atanf(float x) { return (float)atan((double)x); }
float atan2f(float y, float x) { return (float)atan2((double)y, (double)x); }
float sqrtf(float x) { return (float)sqrt((double)x); }
float expf(float x)  { return (float)exp((double)x); }
float logf(float x)  { return (float)log((double)x); }
float log10f(float x) { return (float)log10((double)x); }
float powf(float x, float y) { return (float)pow((double)x, (double)y); }
float fmodf(float x, float y) { return (float)fmod((double)x, (double)y); }
float fabsf(float x) { return (float)fabs((double)x); }
float floorf(float x) { return (float)floor((double)x); }
