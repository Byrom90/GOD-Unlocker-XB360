//--------------------------------------------------------------------------------------
// ATGDsp.h
//
// XNA Developer Connection
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// A set of optimized "building blocks" for DSP work. Common DSP algorithms are
// represented by inline functions whose names end in "Core." Many algorithms also 
// have associated helper functions which are used to calculate parameters. 
//
// In the interests of runtime speed, all core functions are implemented entirely
// in VMX. Unless otherwise noted, each function processes four samples at a time.
//--------------------------------------------------------------------------------------
#pragma once

#include "ATGMetaVMX.h"


namespace ATG
{
//--------------------------------------------------------------------------------------
// DelayLine
//
// Helper class for representing digital delay lines
//
// In digital audio, a delay line represents something like a queue, where samples
// are written at one end and retrieved from elsewhere--either at the other end of
// the queue, or at some intermediate point called a "tap."
//
// Even though delay lines are queuelike in concept, for efficiency they're usually
// implemented as circular buffers. This template implements a statically-sized array
// with a read pointer and a write pointer. To support the concept of taps, the array
// can be accessed via operator []. Since the buffer is circular, an out of bounds 
// index will simply wrap around. Negative indices are also supported, since in DSP
// literature delayed samples are often represented using a negative offset from 
// the current time. However, to keep things from getting too freaky, indices are
// limited to the range {-N...N}
//--------------------------------------------------------------------------------------
template<typename T, long N> class DelayLine
{
public:
                DelayLine()
                {
                    Clear();
                }

    //
    // Const and non-const index operators
    //
    // Note that indices are alwasy calculated as an offset 
    // from the current read pointer.
    //
    const T& operator[]( int idx ) const
    {
        _ASSERT( idx < N && idx > -N );
        return m_data[ ( m_writePtr + idx + N ) % N ];
    }

    T& operator[]( int idx )
    {
        _ASSERT( idx < N && idx > -N );
        return m_data[ ( m_writePtr + idx + N ) % N ];
    }

    //
    // Head/tail access
    //
    // Because the buffer is circular, the "head" is just the
    // item at the current write pointer. The write pointer 
    // increments as items are pushed, so the "tail" is always
    // one greater than the current write pointer.
    //
    const T& Head() const
    {
        return operator[]( 0 );
    }
    T& Head()
    {
        return operator[]( 0 );
    }
    const T& Tail() const
    {
        return operator[]( 1 );
    }
    T& Tail()
    {
        return operator[]( 1 );
    }

    //
    // "Push" increments the read pointer and writes a value
    // to the new head location. 
    //
    // Note that there's no corresponding "Pop" because the
    // queue size is fixed; each push is effectively a pop
    // as well since the new head value overwrites the old
    // tail.
    //
    void        Push( const T& value )
    {
        Head() = value;
        m_writePtr++;
        m_writePtr %= N;
    }

    void        Clear()
    {
        XMemSet( m_data, 0, sizeof( m_data ) ); m_writePtr = 0;
    }
    static long Length()
    {
        return N;
    }

private:
    T           m_data[N];
    int m_writePtr;
};



//--------------------------------------------------------------------------------------
// Root-Mean-Square (RMS)
// 
// RMS is an estimate of the energy present in a signal. The samples are squared and
// then averaged over some period of time, then the root of the average is taken.
// Usually a running average is more useful than a total, so a delay line is used
// to keep track of the window over which the samples are averaged. The windowLen
// value is the number of four-element vectors in the window, NOT the number of samples.
// The number of samples is windowLen * 4.
//
// The accumulator vector is used to maintain the algorithm's internal state. It should
// be initialized to zero.
//
//--------------------------------------------------------------------------------------
template<DWORD windowLen> __vector4 __forceinline RmsCore( vector4_in input, DelayLine<__vector4, windowLen>* __restrict window, vector4_inout accumulator, __vector4 oneOverWindowLen )
 {
    // 
    // Square the four newest input samples
    //
    __vector4 square = __vmulfp( input, input );

    // Grab the four oldest squared samples, then push the four
    // newest
    //
    __vector4 tail = window->Tail();
    window->Push( square );

    //
    // Compute the running average over the four samples. For each 
    // sample we conceptually need to add the new value while
    // simultaneously subtracting the oldest value in the window.
    // For efficiency we'll do the subtractions in parallel, 
    // then accumulate using rotate/inserts and adds.
    //
    __vector4 abcd = __vsubfp( square, tail );

    __vector4 total = __vmsum4fp( abcd, Ones );

    __vector4 _abc = __vrlimi( __vzero(), abcd, 7, 3 );
    __vector4 __ab = __vrlimi( __vzero(), abcd, 3, 2 );
    __vector4 ___a = __vrlimi( __vzero(), abcd, 1, 1 );


    //
    // The output is the accumulator value plus the accumulation
    // of each input sample
    // 
    __vector4 output = accumulator;
    output = __vaddfp( output, __vaddfp( abcd, _abc ) );
    output = __vaddfp( output, __vaddfp( __ab, ___a ) );

    accumulator = __vaddfp( accumulator, total );

    // Compute the average by dividing the accumulated totals by
    // the window length.
    //
    output = __vmulfp( output, oneOverWindowLen );

    // Take the square root of the average. This code uses the VMX
    // reciprocal square root estimate instruction, which is only
    // accurate to about 12 bits. That's probably enough precision
    // for most applications, but for highest fidelity we'd want
    // to refine the result with an iteration or two of Newton's method.
    //
    __vector4 isZero = __vcmpgefp( __vzero(), output );
    output = __vmulfp( output, __vrsqrtefp( output ) );
    output = __vsel( output, __vzero(), isZero );

    return output;
}

//--------------------------------------------------------------------------------------
// Automatic gain control
//
// Varies the gain on an input signal based on how greatly the signal's RMS differs
// from a target value.
//
// If this function is used in conjunction with RMSCore and the window length is long
// enough, the effect will be to keep the output signal fairly close to the input
// signal without overly distorting the audio. Shorter windows, non-RMS values, or
// animated target values can be useful for special effects. 
//
//--------------------------------------------------------------------------------------
__vector4 __forceinline AgcCore( vector4_in input, vector4_out output, vector4_in rms, vector4_in target )
{
    __vector4 absAmp = __vand( rms, vectorNonSignBits.v );
    __vector4 diff = __vsubfp( target, absAmp );
    __vector4 gain = __vaddfp( Ones, __vmulfp( diff, __vrefp( rms ) ) );
    output = __vsel( __vmulfp( input, gain ), __vzero(), __vcmpgefp( __vzero(), rms ) );
}

//--------------------------------------------------------------------------------------
// Filter
//
// This is a standard "Direct Form 1" biquad filter implementation, which can be
// used to represent several different types of filter depending on the coefficients
// used. Helpers to generate a few different filter types are included. 
// 
// Biquad filters are IIR, or Infinite Impulse Response, designs. In practice this 
// means that the filter uses a feedback mechanism: each calculation requires not
// only an input sample but also a number of previous samples from both the input
// and the output. The particular design used here is optimal because it requires 
// exactly four output samples, which means that the output can be recycled one 
// vector at a time.
//--------------------------------------------------------------------------------------
void CalcLowpassCoeffs( float Fc, float Q, vector4_out coeffsA, vector4_out coeffsB );
void CalcHighpassCoeffs( float Fc, float Q, vector4_out coeffsA, vector4_out coeffsB );
void CalcBandpassCoeffs( float Fc, float Q, float gain, vector4_out coeffsA, vector4_out coeffsB );

const static __vector4 DelayLinePermute1 = { vpermY0.f, vpermZ0.f, vpermW0.f, vpermX1.f };
const static __vector4 DelayLinePermute2 = { vpermZ0.f, vpermW0.f, vpermX1.f, vpermY1.f };
const static __vector4 DelayLinePermute3 = { vpermW0.f, vpermX1.f, vpermY1.f, vpermZ1.f };

void __forceinline FilterCore( 
                vector4_in  input,
                vector4_out output,
                vector4_in  prevInput, 
                vector4_in  prevOutput,
                vector4_in  coeffA,
                vector4_in  coeffB )
{
    // Vectors of samples are defined such that X is the earliest in time and W is the
    // latest. This is the reverse of the way the filter coefficients are declared: 
    // A.X should be multipled with the most recent output. We need to reverse one of the
    // two sets of vectors. 
    __vector4 A = __vpermwi( coeffA, 0xe4 );
    __vector4 B = __vpermwi( coeffB, 0xe4 );

    __vector4 xValues = __vperm( prevInput, input, DelayLinePermute1 );
    __vector4 yValues = prevOutput;
    __vector4 outValue = __vsubfp( __vmsum4fp( xValues, B ), __vmsum4fp( yValues, A ) );
    output = outValue;


    xValues = __vperm( prevInput, input, DelayLinePermute2 );
    yValues = __vrlimi( outValue, yValues, 0xE, 1 ); // ( XYZW, OOOO ) -> YZWO
    outValue = __vsubfp( __vmsum4fp( xValues, B ), __vmsum4fp( yValues, A ) );
    output = __vrlimi( output, outValue, 4, 0 );

    xValues = __vperm( prevInput, input, DelayLinePermute3 );
    yValues = __vrlimi( outValue, yValues, 0xE, 1 ); // ( XYZW, OOOO ) -> YZWO
    outValue = __vsubfp( __vmsum4fp( xValues, B ), __vmsum4fp( yValues, A ) );
    output = __vrlimi( output, outValue, 2, 0 );

    xValues = input;
    yValues = __vrlimi( outValue, yValues, 0xE, 1 ); // ( XYZW, OOOO ) -> YZWO
    outValue = __vsubfp( __vmsum4fp( xValues, B ), __vmsum4fp( yValues, A ) );
    output = __vrlimi( output, outValue, 1, 0 );

}


//--------------------------------------------------------------------------------------
// Compressor
//
// Compressors are a more sophisticated version of automatic gain control. Rather 
// than simply adjusting the gain toward a target, they reduce gain on louder 
// material while leaving softer material unchanged. The basic transfer function (plot
// of input vs. output) of a compressor is a diagonal line of slope 1 from the origin
// to a point called the threshold. After that point, the line's slope is less than
// one. The new slope is called the compressor's ratio. For historical reasons the
// ratio is usually written as a reciprocal--for instance, a ratio of .1 would be
// described as 10:1 compression.
//
// A compressor whose transfer function transitions abruptly at the threshold is
// called a "hard-knee" compressor. More sophisticated compressors transition
// gradually, usually preserving C1 or C2 continuity along the transfer function.
// This is called "soft-knee" compression.
//
// This compressor has three important parameters:
//      Ratio       - the compression ratio as described above
//      Knee        - the width of the transition between no compression (1:1) and 
//                    full compression (<1/ratio>:1).
//      Threshold   - the center point of the transition
//
//--------------------------------------------------------------------------------------
struct SoftKneeCompressorParams
{
    __vector4 threshold1;
    __vector4 threshold2;
    __vector4 ratio;
    __vector4 coeffs;
    __vector4 endOfKneeValue;
};
void WINAPI CalcSoftKneeCompressorParams( float threshold, float knee, float ratio,
                                          SoftKneeCompressorParams* out_pParams );

//
// Creating the "soft knee" requires a fourth-order polynomial evaluation.
// This is a useful function in its own right, so it's exposed independently.
//
__vector4 __forceinline EvalPolynomial4( vector4_in coeffs, vector4_in x )
{
    // Splat the coefficients so we can do four samples in parallel
    //
    __vector4 a = __vspltw( coeffs, 0 );
    __vector4 b = __vspltw( coeffs, 1 );
    __vector4 c = __vspltw( coeffs, 2 );
    __vector4 d = __vspltw( coeffs, 3 );

    // Take the square, cube, and fourth power of each sample
    //
    __vector4 x2 = __vmulfp( x, x );
    __vector4 x3 = __vmulfp( x, x2 );
    __vector4 x4 = __vmulfp( x2, x2 );

    // fmadd the coefficients with the corresponding
    // samples
    __vector4 y = __vmaddfp( x, d,
        __vmaddfp( x2, c,
        __vmaddfp( x3, b,
        __vmulfp( x4, a ) ) ) );

    return y;
}

void __forceinline SoftKneeCompressorCore( 
    vector4_in input,
    vector4_out output, 
    vector4_in threshold1,
    vector4_in threshold2,
    vector4_in ratio,
    vector4_in coeffs,
    vector4_in endOfKneeValue )
{
    // 
    // Get the input's absolute value and its reciprocal
    //
    __vector4 x = __vand( input, vectorNonSignBits.v );
    __vector4 oneOverX = __vrefp( x );

    //
    // Two evaluations: t1 is the polynomial at x.
    // t2 is the line that comes after the polynomial,
    // whose slope is the compressor's ratio.
    //
    __vector4 t1 = EvalPolynomial4( coeffs, x );
    __vector4 t2 = __vmaddfp( __vsubfp( x, threshold2 ), ratio, endOfKneeValue );

    //
    // Use vsel to figure out which value is valid: Below threshold1 we
    // return x, in the knee range we return the polynomial at x, above that
    // we return x * the ratio. We clamp to 1.0f.
    //
    __vector4 transfer = __vsel( x, t1, __vcmpgefp( x, threshold1 ) );
    transfer = __vsel( transfer, t2, __vcmpgefp( x, threshold2 ) );
    
    output = __vsel( __vmulfp( transfer, oneOverX ), __vzero(), __vcmpeqfp( x, __vzero() ) );
	output = __vsel(output,Ones,__vcmpgefp(output,Ones));

}

} // Namespace ATG
