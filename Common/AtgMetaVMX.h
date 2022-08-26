#pragma once

#include "AtgMeta.h"
#include <vectorintrinsics.h>

namespace ATG{

    //
    // Recommended types for passing __vector4
    //
    typedef const __vector4 __declspec(passinreg) vector4_in;
    typedef __vector4 __declspec(passinreg)& vector4_out;
    typedef __vector4 __declspec(passinreg)& vector4_inout;
    typedef __vector4* __restrict vector4_ptr;

    //
    // Wrappers for VMX compiler intrinsics
    // 
    // VMX intrinsics can't be used as functors--they look like functions
    // but they have no storage, which makes things all confused. Plus, they
    // are mostly defined with return types, which makes them harder to use
    // with CompileTimeLoop. Fortunately they can be wrapped in functions
    // with no loss of efficiency as long as the functions are inlined.
    //
    void __forceinline vor( vector4_out result, vector4_in a, vector4_in b ){ result = __vor( a, b ); }
    void __forceinline vand( vector4_out result, vector4_in a, vector4_in b ){ result = __vand( a, b ); }

    void __forceinline vcmpgefp( vector4_out result, vector4_in a, vector4_in b ){ result = __vcmpgefp( a, b ); }
    void __forceinline vcmpgtfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vcmpgtfp( a, b ); }

    void __forceinline vcmpgefp_vor( vector4_out result, vector4_in a, vector4_in b ){ result = __vor( result, __vcmpgefp( a, b ) ); }
    void __forceinline vcmpgtfp_vor( vector4_out result, vector4_in a, vector4_in b ){ result = __vor( result, __vcmpgtfp( a, b ) ); }
    void __forceinline vcmpgefp_vand( vector4_out result, vector4_in a, vector4_in b ){ result = __vand( result, __vcmpgefp( a, b ) ); }
    void __forceinline vcmpgtfp_vand( vector4_out result, vector4_in a, vector4_in b ){ result = __vand( result, __vcmpgtfp( a, b ) ); }

    void __forceinline vmulfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vmulfp( a, b ); }
    void __forceinline vaddfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vaddfp( a, b ); }
    void __forceinline vsubfp( vector4_out result, vector4_in a, vector4_in b ){ result = __vsubfp( a, b ); }
    void __forceinline vmaddfp( vector4_out result, vector4_in mul1, vector4_in mul2, vector4_in add ){ result = __vmaddfp( mul1, mul2, add ); }
    void __forceinline vmsum4fp( vector4_out result, vector4_in mul1, vector4_in mul2 ){ result = __vmsum4fp( mul1, mul2 ); }
    void __forceinline vmsum3fp( vector4_out result, vector4_in mul1, vector4_in mul2 ){ result = __vmsum3fp( mul1, mul2 ); }


    //
    // Things do get a little tricky when the intrinsic takes a constant
    // parameter, because the value for the parameter has to be known at
    // compile time in order for the compiler to emit the right machine code.
    // This could be solved in many cases by wrapping the intrinsic in 
    // a function template, but you can't pass a template function to another
    // template function if the second function isn't expecting a template.
    // We get around this problem by using functor structs--structures that
    // overload operator() and templatize the overload. The struct object
    // is what gets passed to CompileTimeLoop, which sees it as just another
    // function. When the operator actually gets called, the compiler 
    // deduces the correct integer to call it with.
    //
    struct vspltw_t
    {
        template<int element>
        void __forceinline operator()(vector4_out result, vector4_in src, CompileTimeInt<element> e ){ result = __vspltw( src, element ); }
    };
    __declspec(selectany) vspltw_t vspltw;

    struct vinsert_t
    {
        template<int srcE, int destE>
        void __forceinline operator()(vector4_out result, vector4_in src, CompileTimeInt<srcE> srcElement, CompileTimeInt<destE> destElement )
        { 
            result = __vrlimi( result, src, 8>>destE, ((destE-srcE)+3)%3);
        }
    };
    __declspec(selectany) vinsert_t vinsert;

    union fdw{ DWORD dw; float f; };
    const static fdw vpermX0 = { 0x00010203 };
    const static fdw vpermY0 = { 0x04050607 };
    const static fdw vpermZ0 = { 0x08090a0b };
    const static fdw vpermW0 = { 0x0c0d0e0f };
    const static fdw vpermX1 = { 0x10111213 };
    const static fdw vpermY1 = { 0x14151617 };
    const static fdw vpermZ1 = { 0x18191a1b };
    const static fdw vpermW1 = { 0x1c1d1e1f };

    union __declspec(align(16)) vdw{ DWORD dw[4]; __vector4 v; };
    const static __vector4 Ones = { 1.0f, 1.0f, 1.0f, 1.0f };
    const static vdw vectorSignBits = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
    const static vdw vectorNonSignBits = { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff };
} // namespace ATG