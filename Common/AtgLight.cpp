//-----------------------------------------------------------------------------
// AtgLight.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgCollision.h"
#include "AtgBound.h"
#include "AtgLight.h"

namespace ATG
{

CONST StringID Light::TypeID( L"Light" );
CONST StringID AmbientLight::TypeID( L"AmbientLight" );
CONST StringID PointLight::TypeID( L"PointLight" );
CONST StringID SpotLight::TypeID( L"SpotLight" );
CONST StringID DirectionalLight::TypeID( L"DirectionalLight" );

CONST EnumStringMap Light::FalloffType_StringMap[] =
{
    { Light::LinearFalloff, L"LINEAR" },
    { Light::SquaredFalloff, L"SQUARED" },
    { Light::NoFalloff, L"NONE" },
    { 0, NULL },
};

CONST EnumStringMap Light::LightFlags_StringMap[] =
{
    { Light::NoFlags, L"NONE" },
    { Light::IsShadowCaster, L"SHADOWCASTER" },
    { 0, NULL },
};

D3DCOLOR Light::GetD3DColor() const
{
    XMVECTOR vColor = GetColor();
    vColor = XMVectorClamp( vColor, XMVectorZero(), XMVectorReplicate( 1.0f ) );

    XMFLOAT4A vec;
    XMStoreFloat4A( &vec, vColor );

    D3DCOLOR Color = D3DCOLOR_COLORVALUE( vec.x, vec.y, vec.z, vec.w );
    return Color;
}

//-----------------------------------------------------------------------------
// Name: PointLight::SetLocalRange
//-----------------------------------------------------------------------------
VOID PointLight::SetLocalRange( FLOAT fRange )
{
    Sphere sphere;

    m_fRange = fRange;

    // build new bound
    XMStoreFloat3( &( sphere.Center ), XMVectorZero() );
    sphere.Radius = m_fRange;

    SetLocalBound( Bound( sphere ) );
}

//-----------------------------------------------------------------------------
// Name: PointLight::GetWorldRange
//-----------------------------------------------------------------------------
FLOAT PointLight::GetWorldRange()
{
    // assume uniform scale.  get scale from length of x axis
    XMVECTOR Scale = XMVector3Length( GetWorldTransform().r[0] );
    return GetLocalRange() * XMVectorGetX( Scale );
}


//--------------------------------------------------------------------------------------
// Name: PointLight::SetWorldRange()
//--------------------------------------------------------------------------------------
VOID PointLight::SetWorldRange( FLOAT fRange )
{
    // assume uniform scale.  get scale from length of x axis
    XMVECTOR Scale = XMVector3Length( GetWorldTransform().r[0] );
    SetLocalRange( fRange / XMVectorGetX( Scale ) );
}


//--------------------------------------------------------------------------------------
// Name: SpotLight::SetWorldRange()
//--------------------------------------------------------------------------------------
VOID SpotLight::SetWorldRange( FLOAT fRange )
{
    // assume uniform scale.  get scale from length of x axis
    XMVECTOR Scale = XMVector3Length( GetWorldTransform().r[0] );
    SetLocalRange( fRange / XMVectorGetX( Scale ) );
}


//-----------------------------------------------------------------------------
// Name: SpotLight::RecalculateBound
//-----------------------------------------------------------------------------
VOID SpotLight::RecalculateBound()
{
    Frustum frustum;

    /*
      FLOAT FovY = m_fOuterAngle;
      FLOAT ZNear = m_fRange * 1e-4f;
      FLOAT ZFar = m_fRange;
      FLOAT h = ( 2.0f * ZNear) / ( 1.0f/tanf(FovY/2.0f)) ;
      
      //$OPTIMIZE: store matrix in decomposed form
      XMMATRIX Projection = XMMatrixPerspectiveLH( h, h, ZNear, ZFar );
     */
    XMMATRIX Projection = GetLightProjection();

    ComputeFrustumFromProjection( &frustum, &Projection );

    SetLocalBound( Bound( frustum ) );
}


//-----------------------------------------------------------------------------
// Name: PointLight::GetWorldRange
//-----------------------------------------------------------------------------
FLOAT SpotLight::GetWorldRange()
{
    // assume uniform scale.  get scale from length of x axis
    XMVECTOR Scale = XMVector3Length( GetWorldTransform().r[0] );
    return GetLocalRange() * XMVectorGetX( Scale );
}

} // namespace ATG
