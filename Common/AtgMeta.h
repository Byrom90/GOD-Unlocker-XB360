#pragma once

//
// Metaprogramming tools
//
// Template metaprogramming is a way to evaluate expressions
// and generate code at compile time rather than at run time.
// If used correctly, it can pick up where the optimizer leaves
// off and let you unroll loops, hoist variables, and select
// optimized implementations for performance-critical functionality.
// Used incorrectly, it can kill your build with cryptic errors
// or cripple it with astoundingly slow compile times.
//
// Some areas of Xbox 360 code can benefit greatly from judicious
// use of template metaprogramming. It's especially helpful when
// writing VMX128 code, since the 360's vector unit craves huge
// amounts of data and doesn't always play nicely with traditional
// batch processing techniques.
//
// The types, templates and concepts in this header are designed
// for Xbox 360-specific tasks. They are not intended to be an
// all-purpose metaprogramming solution.
//

//
// NOTA BENE:
// Do not make the choice to use these classes lightly! Even when
// used correctly, they will increase compile times by an order
// of magnitude. This is especially true on release builds, where
// their effect is to generate huge working sets for the optimizer.
// Isolate files that include this header into areas of code that
// won't be recompiled often.
//


namespace ATG{
    //
    // CompileTimeInt
    //
    // Although it's possible to use integers as template parameters,
    // there are some important differences between integral parameters
    // and type parameters. This class helps iron out some of those
    // differences by effectively creating a new type for each unique
    // integer value used by the application. 
    //
    // This class conforms to a simple pattern for "meta-values." A
    // meta-value is a compile-time value that can be manipulated by
    // "meta-ops" or compile-time operations. A meta-value in this 
    // system is any type that has a public const static integral 
    // member called "value."
    //
    template< int V >
    struct CompileTimeInt
    {
        typedef CompileTimeInt<V> type;
        enum{ value = V};
            
        operator int() { return value; }
        
    };


    //
    // CompileTimeLoop
    //
    // This template emits multiple calls to the same function. Pass it the
    // name of the function you want to call and a set of parameters. By default,
    // CompileTimeLoop expects the parameters to be arrays (or some class that implements
    // operator []). Each time it calls the function, it increments the array
    // index. This makes it very easy to process an entire batch of data in
    // one call. It's faster in most cases than using a loop for the same 
    // purpose, because the code it emits is automatically "unrolled."
    //
    // CompileTimeLoop works by specializing a template class. The general
    // version of the template takes a starting index and a count. You
    // call the function "unroll." It then calls successive versions of
    // itself recursively, incrementing index and decrementing
    // count each time. The specialization is for count==0. When count
    // drops to zero the specialized version is called. This version
    // of the function is empty and so the recursion halts.
    //
    // In order to make CompileTimeLoop as easy to use as possible,
    // CompileTimeLoop::unroll is overloaded for functions of up
    // to twenty parameters. The overloading is done through a set
    // of macro expansions. Skip past the "helper macros" section to see
    // the class definition of CompileTimeLoop.


    // HELPER MACROS FOR LOOP
    //
    // These macros are primarily intended to make the rest of the file
    // more readable by eliminating redundant code.
    //
    #define DUP1( M, name ) M( name##0 )
    #define DUP2( M, name ) DUP1( M, name ), M( name##1 )
    #define DUP3( M, name ) DUP2( M, name ), M( name##2 )
    #define DUP4( M, name ) DUP3( M, name ), M( name##3 )
    #define DUP5( M, name ) DUP4( M, name ), M( name##4 )
    #define DUP6( M, name ) DUP5( M, name ), M( name##5 )
    #define DUP7( M, name ) DUP6( M, name ), M( name##6 )
    #define DUP8( M, name ) DUP7( M, name ), M( name##7 )
    #define DUP9( M, name ) DUP8( M, name ), M( name##8 )
    #define DUP10( M, name ) DUP9( M, name ), M( name##9 )
    #define DUP11( M, name ) DUP10( M, name ), M( name##10 )
    #define DUP12( M, name ) DUP11( M, name ), M( name##11 )
    #define DUP13( M, name ) DUP12( M, name ), M( name##12 )
    #define DUP14( M, name ) DUP13( M, name ), M( name##13 )
    #define DUP15( M, name ) DUP14( M, name ), M( name##14 )
    #define DUP16( M, name ) DUP15( M, name ), M( name##15 )
    #define DUP17( M, name ) DUP16( M, name ), M( name##16 )
    #define DUP18( M, name ) DUP17( M, name ), M( name##17 )
    #define DUP19( M, name ) DUP18( M, name ), M( name##18 )
    #define DUP20( M, name ) DUP19( M, name ), M( name##19 )

    #define DUP_ARGNAME( name ) name
    #define DUP_ARGINDEXED( name ) name##[CurrentIndex()]
    #define LOOP_TEMPLATE_ARG( name ) typename name##_t
    #define LOOP_FUNCTION_ARG( name ) name##_t& name

    #define LOOP_UNROLLER_FN( DUP_MACRO )                                           \
        template< typename fn_t, DUP_MACRO( LOOP_TEMPLATE_ARG, arg )>                   \
        static __forceinline void Unroll( fn_t fn, DUP_MACRO( LOOP_FUNCTION_ARG, arg ) )\
    {                                                                               \
        fn( DUP_MACRO( DUP_ARGINDEXED, arg ) );                                      \
        CompileTimeLoop<index + 1, count - 1>::Unroll                                \
        ( fn, DUP_MACRO( DUP_ARGNAME, arg ) );                                 \
    }                                               

    #define LOOP_UNROLLER_FN_EMPTY( DUP_MACRO )                                         \
        template< typename fn_t, DUP_MACRO( LOOP_TEMPLATE_ARG, arg )>                   \
        static __forceinline void Unroll( fn_t fn, DUP_MACRO( LOOP_FUNCTION_ARG, arg ) )\
    {}                                                                                   


    template<int index, int count>
    class CompileTimeLoop
    {
    public:
        typedef CompileTimeInt<index> CurrentIndex;

        template<typename fn_t>
        static __forceinline void Unroll( fn_t fn )
        {
            fn();
            loop<index+1,count-1>::unroll(fn);
        }

        LOOP_UNROLLER_FN( DUP1 );
        LOOP_UNROLLER_FN( DUP2 );
        LOOP_UNROLLER_FN( DUP3 );
        LOOP_UNROLLER_FN( DUP4 );
        LOOP_UNROLLER_FN( DUP5 );
        LOOP_UNROLLER_FN( DUP6 );
        LOOP_UNROLLER_FN( DUP7 );
        LOOP_UNROLLER_FN( DUP8 );
        LOOP_UNROLLER_FN( DUP9 );
        LOOP_UNROLLER_FN( DUP10 );
        LOOP_UNROLLER_FN( DUP11 );
        LOOP_UNROLLER_FN( DUP12 );
        LOOP_UNROLLER_FN( DUP13 );
        LOOP_UNROLLER_FN( DUP14 );
        LOOP_UNROLLER_FN( DUP15 );
        LOOP_UNROLLER_FN( DUP16 );
        LOOP_UNROLLER_FN( DUP17 );
        LOOP_UNROLLER_FN( DUP18 );
        LOOP_UNROLLER_FN( DUP19 );
        LOOP_UNROLLER_FN( DUP20 );

    };


    template<int index>
    class CompileTimeLoop<index, 0>
    {
    public:
        template<typename fn_t>  static __forceinline void Unroll( fn_t fn ){}

        LOOP_UNROLLER_FN_EMPTY( DUP1 );
        LOOP_UNROLLER_FN_EMPTY( DUP2 );
        LOOP_UNROLLER_FN_EMPTY( DUP3 );
        LOOP_UNROLLER_FN_EMPTY( DUP4 );
        LOOP_UNROLLER_FN_EMPTY( DUP5 );
        LOOP_UNROLLER_FN_EMPTY( DUP6 );
        LOOP_UNROLLER_FN_EMPTY( DUP7 );
        LOOP_UNROLLER_FN_EMPTY( DUP8 );
        LOOP_UNROLLER_FN_EMPTY( DUP9 );
        LOOP_UNROLLER_FN_EMPTY( DUP10 );
        LOOP_UNROLLER_FN_EMPTY( DUP11 );
        LOOP_UNROLLER_FN_EMPTY( DUP12 );
        LOOP_UNROLLER_FN_EMPTY( DUP13 );
        LOOP_UNROLLER_FN_EMPTY( DUP14 );
        LOOP_UNROLLER_FN_EMPTY( DUP15 );
        LOOP_UNROLLER_FN_EMPTY( DUP16 );
        LOOP_UNROLLER_FN_EMPTY( DUP17 );
        LOOP_UNROLLER_FN_EMPTY( DUP18 );
        LOOP_UNROLLER_FN_EMPTY( DUP19 );
        LOOP_UNROLLER_FN_EMPTY( DUP20 );
    };


    //
    // Classes for compile-time calculation
    // 
    // These classes define a simple expression tree paradigm for doing
    // compile-time calculations. These classes borrow ideas from boost::mpl
    // and other metaprogramming libraries, but are considerably simpler
    // and less capable. The purpose of these classes is to support fancier
    // looping than the simple monotonically increasing index that 
    // CompileTimeLoop offers as a default. The MetaExpression classes are
    // meant to be used in conjunction with the CompileTimeIndexer classes
    // defined below.
    //
    // In this system an "Expression" is a class that has an embedded template
    // type named "eval." Expressions operate on values, which are classes like
    // CompileTimeInt that have a const static integral member named value.
    // Most expressions evaluate to other expressions, so taken together the 
    // MetaExpressions classes form a tiny little functional language.
    // 
    // eval is always templated on a class which represents the expression's
    // "ambient environment"--that is, data defined outside the expression that
    // is available to the expression and its subexpressions during evaluation.
    // The primary motivator behind the "ambient" idea is to provide a way for
    // expressions to access the current index when evaluating as part of 
    // CompileTimeLoop, but the concept can be extended to provide other ambient
    // data as well.
    //

    //
    // Basic mathematical expressions
    //
    template<class A> struct Negate { template<class ambient> struct eval : CompileTimeInt< -(A::eval<ambient>::value) >{}; };
    template<class A, class B> struct Add { template<class ambient> struct eval : CompileTimeInt< A::eval<ambient>::value + B::eval<ambient>::value >{}; };
    template<class A, class B> struct Sub { template<class ambient> struct eval : CompileTimeInt< A::eval<ambient>::value - B::eval<ambient>::value >{}; };
    template<class A, class B> struct Mul { template<class ambient> struct eval : CompileTimeInt< A::eval<ambient>::value * B::eval<ambient>::value >{}; };
    template<class A, class B> struct Div { template<class ambient> struct eval : CompileTimeInt< A::eval<ambient>::value / B::eval<ambient>::value >{}; };
    template<class A, class B> struct Mod { template<class ambient> struct eval : CompileTimeInt< A::eval<ambient>::value % B::eval<ambient>::value >{}; };
    template<class A, class B, class C> struct Select { template<class ambient> struct eval : CompileTimeInt< A::eval<ambient>::value ? B::eval<ambient>::value : C::eval<ambient>::value >{}; };

    // This is the ambient environment passed to every expression in the tree.
    // It's defined as a struct even though it contains only one value, because
    // defining it this way makes adding new values trivial but defining it as
    // an int would have repercussions all the way through the system.
    //
    template<int index> struct Ambient{ const static int Index = index;};


    //
    // Special compile-time expressions
    //
    // These classes are designed to fill multiple roles. They are "leaf" 
    // expressions, evaluating to value classes rather than to expression
    // classes. They also have overloaded operators that allow them to be
    // used in logical, though somewhat syntactically sketchy, ways when
    // passing them to CompileTimeLoop. For instance, 
    //      CompileTimeLoop<m,n>::Unroll( fn, Index() );
    // will pass the current index value to "fn" every time it's called.
    // Where this is really useful is things like VMX splatting. For
    // instance, here's code to splat each member of a source vector
    // out to one of four different destination vectors:
    //      CompileTimeLoop<0,4>::Unroll( 
    //                              vspltw, 
    //                              destVectors, 
    //                              Indexer<Div<Index,Constant<4> >(src), 
    //                              Index() );
    //
    
    template< int V > struct Constant
        : CompileTimeInt<V>
    { 
        template<class ambient> struct eval : CompileTimeInt<V>{}; 
        
        CompileTimeInt<V> operator[](int){ return CompileTimeInt<V>(); } 
    };

    struct Index
    { 
        template< class ambient > struct eval : CompileTimeInt< ambient::Index >{}; 
        template<int index> CompileTimeInt<index> operator[](CompileTimeInt<index> n){ return CompileTimeInt<index>(); } 
    };


    //
    // Indexers
    //
    // CompileTimeLoop's default functionality is great if you want to process
    // every array member identically and in order. But sometimes you need 
    // more flexibility than that. To handle more complex processing we 
    // introduce the concept of "indexers." These are distinct from
    // "iterators," although the two concepts are very similar. The reason 
    // for the distinction is to emphasize the random-access nature of the 
    // indexing.
    //
    // Indexers are classes that implement operator[]. The default indexers 
    // defined here are parameterized using the MetaExpression templates, 
    // so they should be flexible enough for most purposes.
    //

    // Helper class--encapsulates the MetaExpression part of things
    template< class OperationT >
    class IndexTransform
    {
        template< int E >
        int operator[]( CompileTimeInt<E> element ) const { return OperationT::eval< Ambient<E> >(); }
    };

    //
    // Indexer for constant arrays
    //
    template< typename SequenceT, typename ElementT, class TransformT >
    class ConstCompileTimeIndex
    {
    public:
        typedef SequenceT SequenceType;
        typedef SequenceT ElementType;

        ConstCompileTimeIndex( SequenceT sequence ): m_sequence( sequence ){}

        template< int E >
        const ElementT& operator[]( CompileTimeInt<E> element ) const { return m_sequence[m_transform[element]]; }

    private:
        SequenceT m_sequence;
        TransformT m_transform;
    };

    //
    // Indexer for non-const arrays
    //
    template< typename SequenceT, typename ElementT, class TransformT >
    class CompileTimeIndex
    {
    public:
        CompileTimeIndex( SequenceT sequence ): m_sequence(sequence){}

        template< int E >
        ElementT& operator[]( CompileTimeInt<E> element ) { return m_sequence[m_transform[element]]; }

        template< int E >
        const ElementT& operator[]( CompileTimeInt<E> element ) const { return m_sequence[m_transform[element]]; }

    private:
        SequenceT m_sequence;
        TransformT m_transform;
    };

    //
    // Wraps a uniform value so that it can be used in CompileTimeLoop 
    // and anywhere else that expects an array. The [] operator returns 
    // the same value no matter what the index is.
    //
    template< typename T >
    class IdentityIndexer
    {
    public:
        IdentityIndexer( T& value ): m_value(value){}

        T& operator[]( int ) { return m_value; }

        const T& operator[]( int ) const { return m_value; }

    private:
        T& m_value;
    };

    //
    // const-only version of the above
    //
    template< typename T >
    class ConstIdentityIndexer
    {
    public:
        ConstIdentityIndexer( const T& value ): m_value(value){}

        const T& operator[]( int ) const { return m_value; }

    private:
        const T& m_value;
    };

    //
    // Type switch for indexers, to select the correct
    // type based on the constness and indirection level
    // of the type parameter.
    //
    template< class ExpressionT >
    struct CompileTimeIndexSwitch
    {
        typedef IndexTransform<ExpressionT> TransformT;

        template<typename T> struct specialize{
            typedef IdentityIndexer<T> type; };

        template<typename T> struct specialize<const T>{
            typedef ConstIdentityIndexer<T> type; };

        template<typename T> struct specialize< const T* >{ 
            typedef ConstCompileTimeIndex<const T*, T, TransformT> type; };
        
        template<typename T> struct specialize< T* >{ 
            typedef CompileTimeIndex<T*, T, TransformT> type; };

    };

    //
    // Template factory for indexers. Implemented as a template function to
    // make the compiler deduce the type of sequence and make it easier to
    // support types other than vanilla C arrays.
    //
    template< class ExpressionT, typename T >
    typename CompileTimeIndexSwitch<ExpressionT>::specialize<T>::type Indexer( T in )
    { 
        return typename CompileTimeIndexSwitch<ExpressionT>::specialize<T>::type( in ); 
    }

    template<typename T>
    typename CompileTimeIndexSwitch<Constant<0> >::specialize<T>::type Uniform( T& in )
    {
        return typename CompileTimeIndexSwitch<Constant<0> >::specialize<T>::type( in ); 
    }


} // namespace ATG














