//-----------------------------------------------------------------------------
// AtgBound.h
//
// A bound in a union of four basic bound types: frustum, AAbb, Obb, and
// sphere.  Bounds can be collided against one another.  
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_BOUND_H
#define ATG_BOUND_H

namespace ATG
{

struct AxisAlignedBox;
struct OrientedBox;
struct Frustum;
struct Sphere;

//-----------------------------------------------------------------------------
// Name: EnumStringMap
// Desc: Maps values to strings
//-----------------------------------------------------------------------------    
#ifndef ENUMSTRINGMAP_DEFINED
struct EnumStringMap
{
    DWORD Value;
    const WCHAR* szName;
};
#define ENUMSTRINGMAP_DEFINED
#endif

//$TODO: try to get the bounds in a union, once xboxmath.h has had empty
//       constructors removed
//-----------------------------------------------------------------------------
class Bound
{
public:
    enum BoundType
    {
        No_Bound,
        Sphere_Bound,
        Frustum_Bound,
        OBB_Bound,
        AABB_Bound,
        ForceDWORD = 0x7FFFFFFF
    };

    static const EnumStringMap  BoundType_StringMap[];

    // construct/destruct a bound
    Bound() { Clear(); }
    Bound( const Sphere& Sphere ) { SetSphere( Sphere ); }
    Bound( const Frustum& Frustum ){ SetFrustum( Frustum ); }
    Bound( const AxisAlignedBox& Aabb ){ SetAabb( Aabb ); }
    Bound( const OrientedBox& Obb ){ SetObb( Obb ); }
    Bound( const Bound& Other ){ *this = Other; }

    VOID Clear() { m_Type = Bound::No_Bound; }

    // collision with other primitives
    BOOL            Collide( const Bound& Other ) const;
    BOOL            Collide( const Frustum& Frustum ) const;
    BOOL            Collide( const Sphere& Sphere ) const;
    BOOL            Collide( const AxisAlignedBox& Aabb ) const;
    BOOL            Collide( const OrientedBox& pObb ) const;

    // merge with another bound
    VOID            Merge( const Bound& Other );

    // transformation
    Bound           operator*( CXMMATRIX Matrix ) const;

    // get-sets
    // Note that the get functions will assert if you try to
    // get a type not equal to the bounds current BoundType
    BoundType                   GetType() const { return m_Type; }
    const Sphere&           GetSphere() const;
    VOID                        SetSphere( const Sphere& Sphere );
    const Frustum&          GetFrustum() const;
    VOID                        SetFrustum( const Frustum& Frustum );
    const AxisAlignedBox&   GetAabb() const;
    VOID                        SetAabb( const AxisAlignedBox& Aabb );
    const OrientedBox&      GetObb() const;
    VOID                        SetObb( const OrientedBox& Obb );

    // gets the center of the bound. 
    XMFLOAT3       GetCenter() const;

    // gets the bound's maximum radius
    FLOAT          GetMaxRadius() const;

private:
    BoundType       m_Type;
    
    // Data must be big enough to hold the union of all the bound types.
    BYTE            m_Data[ sizeof( FLOAT ) * 13 ];

};

} // namespace ATG

#endif // ATG_BOUND_H
