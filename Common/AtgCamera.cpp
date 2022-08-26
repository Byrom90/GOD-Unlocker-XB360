//-----------------------------------------------------------------------------
// AtgCamera.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgCollision.h"
#include "AtgCamera.h"

namespace ATG
{

CONST StringID Camera::TypeID( L"Camera" );
CONST EnumStringMap Projection::ProjectionType_StringMap[] =
{
    { Projection::Perspective, L"Perspective" },
    { Projection::Orthographic, L"Orthographic" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
// Name: Projection::GetFrustum
//-----------------------------------------------------------------------------
Frustum Projection::GetFrustum() CONST
{
    Frustum Temp;
    XMMATRIX Proj = GetMatrix();
    ComputeFrustumFromProjection( &Temp, &Proj );
    return Temp;
}


//-----------------------------------------------------------------------------
// Name: Projection::GetMatrix
//-----------------------------------------------------------------------------
XMMATRIX Projection::GetMatrix() CONST
{
    XMMATRIX Proj;
    if( m_Type == Projection::Perspective )
    {
        Proj = XMMatrixPerspectiveLH( m_Width, m_Height, m_ZNear, m_ZFar );
    }
    else if( m_Type == Projection::Orthographic )
    {
        Proj = XMMatrixOrthographicLH( m_Width, m_Height, m_ZNear, m_ZFar );
    }
    else
        assert( FALSE );

    return Proj;
}


//-----------------------------------------------------------------------------
// Name: Projection::SetFovXFovY
//-----------------------------------------------------------------------------
VOID Projection::SetFovXFovY( FLOAT FovX, FLOAT FovY, FLOAT ZNear, FLOAT ZFar )
{
    m_ZNear = ZNear;
    m_ZFar = ZFar;
    m_Type = Projection::Perspective;

    m_Width = ( 2.0f * m_ZNear ) * tanf( FovX / 2.0f );
    m_Height = ( 2.0f * m_ZNear ) * tanf( FovY / 2.0f );
}


//-----------------------------------------------------------------------------
// Name: Projection::SetFovYAspect
//-----------------------------------------------------------------------------
VOID Projection::SetFovYAspect( FLOAT FovY, FLOAT Aspect, FLOAT ZNear, FLOAT ZFar )
{
    m_ZNear = ZNear;
    m_ZFar = ZFar;
    m_Type = Projection::Perspective;


    m_Height = 2.0f * m_ZNear * tanf( FovY / 2 );
    m_Width = m_Height * Aspect;
}


//-----------------------------------------------------------------------------
// Name: Projection::SetFovXAspect
//-----------------------------------------------------------------------------
VOID Projection::SetFovXAspect( FLOAT FovX, FLOAT Aspect, FLOAT ZNear, FLOAT ZFar )
{
    m_ZNear = ZNear;
    m_ZFar = ZFar;
    m_Type = Projection::Perspective;


    m_Width = 2.0f * m_ZNear * tanf( FovX / 2 );
    m_Height = m_Width / Aspect;

}


//-----------------------------------------------------------------------------
// Name: Projection::SetOrthographic
//-----------------------------------------------------------------------------
VOID Projection::SetOrthographic( FLOAT Width, FLOAT Height, FLOAT ZNear, FLOAT ZFar )
{
    m_ZNear = ZNear;
    m_ZFar = ZFar;
    m_Type = Projection::Orthographic;
    m_Width = Width;
    m_Height = Height;
}


//-----------------------------------------------------------------------------
// Name: Projection::GetFovX
//-----------------------------------------------------------------------------
FLOAT Projection::GetFovX() CONST
{
    assert( m_Type == Projection::Perspective );
    return atanf( m_Width / ( 2.0f * m_ZNear ) ) * 2.0f;
}


//----------------------------------------------------------------------------
// Name: Projection::GetFovY
//-----------------------------------------------------------------------------
FLOAT Projection::GetFovY() CONST
{
    assert( m_Type == Projection::Perspective );
    return atanf( m_Height / ( 2.0f * m_ZNear ) ) * 2.0f;
}


//-----------------------------------------------------------------------------
// Name: Camera::UpdateBounds
//-----------------------------------------------------------------------------
VOID Camera::UpdateBound()
{
    SetLocalBound( Bound( m_Projection.GetFrustum() ) );
}

} // namespace ATG
