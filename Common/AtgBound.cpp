//-----------------------------------------------------------------------------
// AtgBound.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgCollision.h"
#include "AtgBound.h"

namespace ATG
{

const EnumStringMap Bound::BoundType_StringMap[] =
{
    { Bound::No_Bound, L"NULL" },
    { Bound::Sphere_Bound, L"SPHERE" },
    { Bound::Frustum_Bound, L"FRUSTUM" },
    { Bound::OBB_Bound, L"OBB" },
    { Bound::AABB_Bound, L"AABB" },
    { 0, NULL },
};


//-----------------------------------------------------------------------------
// Name: Bound::operator*
// Desc: transforms the bound by the current matrix
//-----------------------------------------------------------------------------
Bound Bound::operator*( CXMMATRIX World ) const
{
    //$OPTIMIZE: store matrix decomposed    
    XMVECTOR Translation = World.r[3];
    FLOAT Scale = XMVectorGetX( XMVector3Length( World.r[2] ) );
    XMVECTOR Rotation = XMQuaternionNormalize( XMQuaternionRotationMatrix( World ) );

    // switch based off this bounds type and call the correct
    // bound transform function
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
        {
            Sphere WorldSphere = GetSphere();
            TransformSphere( &WorldSphere,
                             &WorldSphere,
                             Scale,
                             Rotation,
                             Translation );
            return Bound( WorldSphere );
        }
        case Bound::Frustum_Bound:
        {
            Frustum WorldFrustum = GetFrustum();
            TransformFrustum( &WorldFrustum,
                              &WorldFrustum,
                              Scale,
                              Rotation,
                              Translation );
            return Bound( WorldFrustum );
        }
        case Bound::OBB_Bound:
        {
            OrientedBox WorldObb = GetObb();
            TransformOrientedBox( &WorldObb,
                                  &WorldObb,
                                  Scale,
                                  Rotation,
                                  Translation );
            return Bound( WorldObb );
        }
        case Bound::AABB_Bound:
        {
            AxisAlignedBox WorldAabb = GetAabb();
            TransformAxisAlignedBox( &WorldAabb,
                                     &WorldAabb,
                                     Scale,
                                     Rotation,
                                     Translation );
            return Bound( WorldAabb );
        }
        case Bound::No_Bound:
            return Bound();
    }

    return Bound();
}


//-----------------------------------------------------------------------------
// Name: Bound::Collide
// Desc: collides this bound with an obb
//-----------------------------------------------------------------------------
BOOL Bound::Collide( const OrientedBox& Obb ) const
{
    // switch on bound type and call the correct intersection function
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
            return IntersectSphereOrientedBox( &GetSphere(), &Obb );
        case Bound::Frustum_Bound:
            return ( BOOL )IntersectOrientedBoxFrustum( &Obb, &GetFrustum() );
        case Bound::OBB_Bound:
            return IntersectOrientedBoxOrientedBox( &GetObb(), &Obb );
        case Bound::AABB_Bound:
            return IntersectAxisAlignedBoxOrientedBox( &GetAabb(), &Obb );
        case Bound::No_Bound:
            return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Bound::Collide
// Desc: collides this bound with a sphere
//-----------------------------------------------------------------------------
BOOL Bound::Collide( const Sphere& Sphere ) const
{
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
            return IntersectSphereSphere( &GetSphere(), &Sphere );
        case Bound::Frustum_Bound:
            return ( BOOL )IntersectSphereFrustum( &Sphere, &GetFrustum() );
        case Bound::OBB_Bound:
            return IntersectSphereOrientedBox( &Sphere, &GetObb() );
        case Bound::AABB_Bound:
            return IntersectSphereAxisAlignedBox( &Sphere, &GetAabb() );
        case Bound::No_Bound:
            return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Bound::Collide
// Desc: collides this bound with an axis aligned box
//-----------------------------------------------------------------------------
BOOL Bound::Collide( const AxisAlignedBox& Aabb ) const
{
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
            return IntersectSphereAxisAlignedBox( &GetSphere(), &Aabb );
        case Bound::Frustum_Bound:
            return ( BOOL )IntersectAxisAlignedBoxFrustum( &Aabb, &GetFrustum() );
        case Bound::OBB_Bound:
            return IntersectAxisAlignedBoxOrientedBox( &Aabb, &GetObb() );
        case Bound::AABB_Bound:
            return IntersectAxisAlignedBoxAxisAlignedBox( &GetAabb(), &Aabb );
        case Bound::No_Bound:
            return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Bound::Collide
// Desc: collides this bound with a frustum
//-----------------------------------------------------------------------------
BOOL Bound::Collide( const Frustum& Frustum ) const
{
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
            return ( BOOL )IntersectSphereFrustum( &GetSphere(), &Frustum );
        case Bound::Frustum_Bound:
            return ( BOOL )IntersectFrustumFrustum( &GetFrustum(), &Frustum );
        case Bound::OBB_Bound:
            return ( BOOL )IntersectOrientedBoxFrustum( &GetObb(), &Frustum );
        case Bound::AABB_Bound:
            return ( BOOL )IntersectAxisAlignedBoxFrustum( &GetAabb(), &Frustum );
        case Bound::No_Bound:
            return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Bound::Collide
// Desc: collides this bound with another bound
//-----------------------------------------------------------------------------
BOOL Bound::Collide( const Bound& Other ) const
{
    switch( Other.m_Type )
    {
        case Bound::Sphere_Bound:
            return Collide( Other.GetSphere() );
        case Bound::Frustum_Bound:
            return Collide( Other.GetFrustum() );
        case Bound::OBB_Bound:
            return Collide( Other.GetObb() );
        case Bound::AABB_Bound:
            return Collide( Other.GetAabb() );
        case Bound::No_Bound:
            return TRUE;
    }

    return FALSE;
}


VOID Bound::Merge( const Bound& Other )
{
    Sphere OtherSphere;
    OtherSphere.Center = Other.GetCenter();
    OtherSphere.Radius = Other.GetMaxRadius();

    if( m_Type == Bound::No_Bound )
    {
        SetSphere( OtherSphere );
        return;
    }

    Sphere ThisSphere;
    if( m_Type != Bound::Sphere_Bound )
    {
        // convert this bound into a sphere
        ThisSphere.Center = GetCenter();
        ThisSphere.Radius = GetMaxRadius();
    }
    else
    {
        ThisSphere = GetSphere();
    }

    XMVECTOR vThisCenter = XMLoadFloat3( &ThisSphere.Center );
    XMVECTOR vOtherCenter = XMLoadFloat3( &OtherSphere.Center );
    XMVECTOR vThisToOther = XMVectorSubtract( vOtherCenter, vThisCenter );
    XMVECTOR vDistance = XMVector3LengthEst( vThisToOther );

    FLOAT fCombinedDiameter = XMVectorGetX( vDistance ) + ThisSphere.Radius + OtherSphere.Radius;
    if( fCombinedDiameter <= ( ThisSphere.Radius * 2 ) )
    {
        SetSphere( ThisSphere );
        return;
    }
    if( fCombinedDiameter <= ( OtherSphere.Radius * 2 ) )
    {
        SetSphere( OtherSphere );
        return;
    }

    XMVECTOR vDirectionNorm = XMVector3Normalize( vThisToOther );

    XMVECTOR vRadius = XMVectorSet( ThisSphere.Radius, OtherSphere.Radius, 0, 0 );
    XMVECTOR vThisRadius = XMVectorSplatX( vRadius );
    XMVECTOR vOtherRadius = XMVectorSplatY( vRadius );
    XMVECTOR vCombinedDiameter = vThisRadius + vDistance + vOtherRadius;
    XMVECTOR vMaxDiameter = XMVectorMax( vCombinedDiameter, vThisRadius * 2 );
    vMaxDiameter = XMVectorMax( vMaxDiameter, vOtherRadius * 2 );
    XMVECTOR vMaxRadius = vMaxDiameter * 0.5f;
    ThisSphere.Radius = XMVectorGetX( vMaxRadius );
    vMaxRadius -= vThisRadius;
    XMVECTOR vCombinedCenter = vThisCenter + vMaxRadius * vDirectionNorm;
    XMStoreFloat3( &ThisSphere.Center, vCombinedCenter );
    SetSphere( ThisSphere );
}


//-----------------------------------------------------------------------------
// Name: Bound::SetSphere
// Desc: sets this bound to a sphere
//-----------------------------------------------------------------------------
VOID Bound::SetSphere( const Sphere& Sphere )
{
    m_Type = Bound::Sphere_Bound;
    memcpy( m_Data, &Sphere, sizeof( Sphere ) );
}


//-----------------------------------------------------------------------------
// Name: Bound::GetSphere
// Desc: Gets the current bound as a sphere.
//-----------------------------------------------------------------------------
const Sphere& Bound::GetSphere() const
{
    assert( m_Type == Bound::Sphere_Bound );
    return *( Sphere* )m_Data;
}


//-----------------------------------------------------------------------------
// Name: Bound::SetAabb
// Desc: sets this bound to an axis aligned box
//-----------------------------------------------------------------------------
VOID Bound::SetAabb( const AxisAlignedBox& Aabb )
{
    m_Type = Bound::AABB_Bound;
    memcpy( m_Data, &Aabb, sizeof( Aabb ) );
}


//-----------------------------------------------------------------------------
// Name: Bound::GetAabb
// Desc: Gets the current bound as an axis aligned box.
//-----------------------------------------------------------------------------
const AxisAlignedBox& Bound::GetAabb() const
{
    assert( m_Type == Bound::AABB_Bound );
    return *( AxisAlignedBox* )m_Data;
}


//-----------------------------------------------------------------------------
// Name: Bound::SetObb
// Desc: sets this bound to an oriented box
//-----------------------------------------------------------------------------
VOID Bound::SetObb( const OrientedBox& Obb )
{
    m_Type = Bound::OBB_Bound;
    memcpy( m_Data, &Obb, sizeof( Obb ) );
}


//-----------------------------------------------------------------------------
// Name: Bound::GetObb
// Desc: Gets this bound as an oriented box
//-----------------------------------------------------------------------------
const OrientedBox& Bound::GetObb() const
{
    assert( m_Type == Bound::OBB_Bound );
    return *( OrientedBox* )m_Data;
}


//-----------------------------------------------------------------------------
// Name: Bound::SetFrustum
// Desc: sets this bound to a frustum
//-----------------------------------------------------------------------------
VOID Bound::SetFrustum( const Frustum& Frustum )
{
    m_Type = Bound::Frustum_Bound;
    memcpy( m_Data, &Frustum, sizeof( Frustum ) );
}


//-----------------------------------------------------------------------------
// Name: Bound::GetFrustum
// Desc: Gets this bound as a frustum
//-----------------------------------------------------------------------------
const Frustum& Bound::GetFrustum() const
{
    assert( m_Type == Bound::Frustum_Bound );
    return *( Frustum* )m_Data;
}


//-----------------------------------------------------------------------------
// Name: Bound::GetCenter
// Desc: Gets the center of the bound.
//-----------------------------------------------------------------------------
XMFLOAT3 Bound::GetCenter() const
{
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
            return *( ( XMFLOAT3* )&GetSphere().Center );
        case Bound::Frustum_Bound:
            return *( ( XMFLOAT3* )&GetFrustum().Origin );
        case Bound::OBB_Bound:
            return *( ( XMFLOAT3* )&GetObb().Center );
        case Bound::AABB_Bound:
            return *( ( XMFLOAT3* )&GetAabb().Center );
        case Bound::No_Bound:
            break;
    }

    return XMFLOAT3( 0.0f, 0.0f, 0.0f );
}


//-----------------------------------------------------------------------------
// Name: Bound::GetMaxRadius
// Desc: Computes the maximum radius of the bound.
//-----------------------------------------------------------------------------
FLOAT Bound::GetMaxRadius() const
{
    switch( m_Type )
    {
        case Bound::Sphere_Bound:
        {
            float Radius = GetSphere().Radius;
            return Radius;
        }

        case Bound::Frustum_Bound:
        {
            FLOAT MaxZ = abs( GetFrustum().Far - GetFrustum().Near );
            FLOAT MaxX = abs( GetFrustum().LeftSlope * GetFrustum().Far - GetFrustum().RightSlope * GetFrustum().Far );
            FLOAT MaxY = abs( GetFrustum().TopSlope * GetFrustum().Far - GetFrustum().BottomSlope * GetFrustum().Far );
            return max( MaxZ, max( MaxX, MaxY ) );
        }

        case Bound::OBB_Bound:
        {
            XMVECTOR v = XMVector3Length( XMLoadFloat3( &( GetObb().Extents ) ) );
            return XMVectorGetX( v );
        }
        case Bound::AABB_Bound:
        {
            XMVECTOR v = XMVector3Length( XMLoadFloat3( &( GetAabb().Extents ) ) );
            return XMVectorGetX( v );
        }
        case Bound::No_Bound:
            break;
    }

    return 0.0f;
}

} // namespace ATG
