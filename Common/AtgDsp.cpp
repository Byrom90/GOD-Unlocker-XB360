#include "stdafx.h"
#include "ATGDsp.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace ATG
{

void CalcLowpassCoeffs( float Fc, float Q, vector4_out coeffsA, vector4_out coeffsB )
{
    Q = ( float )__fsel( Q, Q, .0001f );
    Fc = ( float )__fsel( Fc, Fc, FLT_EPSILON * 2.0f );

    float w0 = ( float )M_PI * Fc;
    float sin0 = sin( w0 );
    float cos0 = cos( w0 );

    float alpha = sin0 / ( 2.0f * Q );

    float a0 = alpha + 1.0f;
    coeffsA.v[0] = ( -2.0f * cos0 ) / a0;
    coeffsA.v[1] = ( 1.0f - alpha ) / a0;
    coeffsA.v[2] = 0.0f;
    coeffsA.v[3] = 0.0f;

    coeffsB.v[0] = ( ( 1.0f - cos0 ) / 2.0f ) / a0;
    coeffsB.v[1] = ( 1.0f - cos0 ) / a0;
    coeffsB.v[2] = ( ( 1.0f - cos0 ) / 2.0f ) / a0;
    coeffsB.v[3] = 0.0f;
}
void CalcHighpassCoeffs( float Fc, float Q, vector4_out coeffsA, vector4_out coeffsB )
{
    Q = (float)__fsel( Q, Q, .0001f );
    Fc = (float)__fsel( Fc, Fc, FLT_EPSILON * 2.0f );

    float w0 = (float)M_PI*Fc;
    float sin0 = sin(w0);
    float cos0 = cos(w0);

    float alpha = sin0/(2.0f*Q);

    float a0 = alpha + 1.0f;
    coeffsA.v[0] =( -2.0f * cos0)/a0;
    coeffsA.v[1] = (1.0f - alpha)/a0;
    coeffsA.v[2] = 0.0f;
    coeffsA.v[3] = 0.0f;

    coeffsB.v[0] = ((1.0f + cos0)/2.0f)/a0;
    coeffsB.v[1] = (-(1.0f + cos0))/a0;
    coeffsB.v[2] = ((1.0f + cos0)/2.0f)/a0;
    coeffsB.v[3] = 0.0f;
}

void CalcBandpassCoeffs( float Fc, float Q, float gain, vector4_out coeffsA, vector4_out coeffsB )
{
    Q = ( float )__fsel( Q, Q, .0001f );
    Fc = ( float )__fsel( Fc, Fc, FLT_EPSILON * 2.0f );

    float w0 = ( float )M_PI * Fc;
    float sin0 = sin( w0 );
    float cos0 = cos( w0 );

    float alpha = sin0 / ( 2.0f * Q );


    float a0 = alpha + 1.0f;
    coeffsA.v[0] = ( -2.0f * cos0 ) / a0;
    coeffsA.v[1] = ( 1.0f - alpha ) / a0;
    coeffsA.v[2] = 0.0f;
    coeffsA.v[3] = 0.0f;

    coeffsB.v[0] = gain * ( sin0 / 2.0f ) / a0;
    coeffsB.v[1] = 0.0f;
    coeffsB.v[2] = gain * ( -sin0 / 2.0f ) / a0;
    coeffsB.v[3] = 0.0f;
}

//
// Distortion compressor used for Radioize effect
//
__vector4 CalcDistortionCompressorCoeffs( float t, float r, float k )
{
    __vector4 coeffs;

    float denom = -8*k*t - 3*t*t*t + t*t*t*t - 8*t*k*k -3*k*t*t*t + 2*k*k*t*t + 6*k*k +2*t*t + 11*k*t*t;
    coeffs.v[0] = -0.5f * ( ( -3*k + 4*t - 3 + 3*k*r - 4*t*r + 3*r ) / denom ); 
    coeffs.v[1] = ( 2*k*k*r - 2*k*k + 2*k*r - 2*k - 3*t*t*r + 3*t*t + 2*r - 2 ) / denom;
    coeffs.v[2] = -0.5f * ( t * ( -8*k*k + 8*k*k*r - 9*r*k*t + 9*k*t - 8*k + 8*k*r - 9*t*r + 9*t - 8 + 8*r ) / denom );
    coeffs.v[3] = ( 9*k*t*t - 8*t*k*k + 2*t*t*r + t*t*t*t*r - 3*t*t*t*r + 2*t*t*k*k*r + 2*k*t*t*r - 3*t*t*t*k*r - 8*k*t + 6*k*k ) / denom;

    return coeffs;
}

void WINAPI CalcSoftKneeCompressorParams( float t, float k, float r, SoftKneeCompressorParams* pParams )
{
    pParams->ratio.v[0] = r;
    pParams->ratio = __vspltw( pParams->ratio, 0 );

    pParams->threshold1.v[0] = t - k / 2.0f;
    pParams->threshold1 = __vspltw( pParams->threshold1, 0 );

    pParams->threshold2.v[0] = t + k / 2.0f;
    pParams->threshold2 = __vspltw( pParams->threshold2, 0 );

    //
    // Generated from the following Maple code:
    // f := proc (x) options operator, arrow; a*x^4+b*x^3+c*x^2+d*x end proc;
    // s := proc (t, k, r) options operator, arrow; {
    //                  f(t-(1/2)*k) = t-(1/2)*k, eval(diff(f(x), x), x = t-(1/2)*k) = 1, 
    //                  eval(diff(f(x), x), x = t+(1/2)*k) = r, 
    //                  eval(diff(f(x), x), x = t) = r + ( 1- r )/2 } end proc;  
    // solve(s(t,k,r),{a,b,c}) end proc;
    //
    float a = -2 * (r - 1) / (k * k + 4 * t * k + 4 * t * t) / k;
    float b = 8 * (r - 1) / (k * k + 4 * t * k + 4 * t * t) / k * t;
    float c = (1 / k * (-20 * t * t * r + 20 * t * t + 4 * t * k * r - 4 * t * k + 3 * k * k * r - 3 * k * k) / (k * k + 4 * t * k + 4 * t * t)) / (float)0.2e1;
    float d = ((k * k + k * k * r - 4 * t * k * r + 8 * t * k + 4 * t * t * r - 4 * t * t) / k / (2 * t + k)) / (float)0.2e1;

    pParams->coeffs.v[0] = a;
    pParams->coeffs.v[1] = b;
    pParams->coeffs.v[2] = c;
    pParams->coeffs.v[3] = d;

    // evaluate the transfer polynomial at t+ k/2--this gives us the 
    // amplitude at the end of the knee
    if( k != 0 )
    {
        float x = t + k / 2.0f;
        pParams->endOfKneeValue.v[0] =
            a * x * x * x * x
            + b * x * x * x
            + c * x * x
            + d * x;
        pParams->endOfKneeValue = __vspltw( pParams->endOfKneeValue, 0 );
    }
    else
    {
        pParams->endOfKneeValue = pParams->threshold1;
    }
}



}
// namespace ATG
