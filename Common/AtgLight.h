//-----------------------------------------------------------------------------
// AtgLight.h
//
// describes the different lights types
//
// $OPTIMIZE: Pool Allocate
//            Should we cache transformed bounds?
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_LIGHT_H
#define ATG_LIGHT_H

#include <vector>
#include "AtgFrame.h"

namespace ATG
{

//-----------------------------------------------------------------------------
// Name: Light
// Desc: Base Light Type
//-----------------------------------------------------------------------------
class Light : public Frame
{
    DEFINE_TYPE_INFO();

    enum LightFlags
    {
        NoFlags         = 0,
        IsShadowCaster  = 0x1,
        IsDisabled      = 0x2,
        ForceDWORDFlags = 0x7fffffff,
    };

    enum FalloffType
    {
        NoFalloff           = 0,
        LinearFalloff       = 1,
        SquaredFalloff      = 2,
        ForceDWORDFalloff   = 0x7fffffff,
    };

    static CONST EnumStringMap Light::LightFlags_StringMap[];
    static CONST EnumStringMap Light::FalloffType_StringMap[];

public:
    Light()
        : m_Color( XMVectorZero() ),
          m_dwFlags( 0 )
    {
    }

    CONST std::vector<StringID>&    GetLightGroups() { return m_LightGroups; }
    VOID                            AddLightGroup( CONST StringID& ID ) { m_LightGroups.push_back( ID ); }

    XMVECTOR                        GetColor() CONST { return m_Color; };
    VOID                            SetColor( XMVECTOR Color ) { m_Color = Color; }
    D3DCOLOR GetD3DColor() const;

    DWORD                           GetFlags() const { return m_dwFlags; }
    VOID                            ClearFlag( LightFlags dwFlag ) { m_dwFlags &= ~dwFlag; }
    VOID                            SetFlag( LightFlags dwFlag ) { m_dwFlags |= dwFlag; }
    BOOL                            TestFlag( LightFlags dwFlag ) const { return ( m_dwFlags & dwFlag ) == dwFlag; }

protected:
    XMVECTOR m_Color;
    WORD m_dwFlags;
    std::vector <StringID> m_LightGroups;
};


//-----------------------------------------------------------------------------
// Name: AmbientLight
// Desc: Non-bounded ambient light.  Summed if more than one on a model
//-----------------------------------------------------------------------------
class AmbientLight : public Light
{
    DEFINE_TYPE_INFO();
public:
};


//-----------------------------------------------------------------------------
// Name: PointLight
// Desc: Sphere bounded by sphere radius
//-----------------------------------------------------------------------------
class PointLight : public Light
{
    DEFINE_TYPE_INFO();
public:
            PointLight() : m_fRange( 0.0f ),
                           m_FalloffType( Light::LinearFalloff )
            {
            }

    FLOAT                GetLocalRange()
    CONST
    {
        return m_fRange;
    };
    VOID    SetLocalRange( FLOAT fRange );

    FLOAT   GetWorldRange();
    VOID    SetWorldRange( FLOAT fRange );

    FalloffType          GetFalloff()
    CONST
    {
        return m_FalloffType;
    };
    VOID    SetFalloff( FalloffType Falloff )
    {
        m_FalloffType = Falloff;
    }

private:
    FLOAT m_fRange;
    FalloffType m_FalloffType;
};


//-----------------------------------------------------------------------------
// Name: DirectionalLight
// Desc: Directional Light, non-bounded - get Direction with GetWorldForward
//-----------------------------------------------------------------------------
class DirectionalLight : public Light
{
    DEFINE_TYPE_INFO();
public:

    // direction
    VOID        SetLocalDirection( XMVECTOR& Direction );
    VOID        SetWorldDirection( XMVECTOR& Direction );

    VOID        SetLightViewProjection( const DWORD dwIndex, CXMMATRIX matVP )
    {
        XMStoreFloat4x4( &m_matLightViewProj[dwIndex], matVP );
    }
    XMMATRIX    GetLightViewProjection( const DWORD dwIndex ) const
    {
        return XMLoadFloat4x4( &m_matLightViewProj[dwIndex] );
    }

protected:
    XMFLOAT4X4  m_matLightViewProj[2];
};


//-----------------------------------------------------------------------------
// spot light, bounded by a frustum derived from its outer cone
//-----------------------------------------------------------------------------
class SpotLight : public Light
{
    DEFINE_TYPE_INFO();
public:
                SpotLight() : m_fRange( 0.0f ),
                              m_FalloffType( Light::LinearFalloff ),
                              m_SpotFalloffType( Light::LinearFalloff ),
                              m_fInnerAngle( XM_PIDIV4 ),
                              m_fOuterAngle( XM_PIDIV4 )
                {
                }

    // direction
    VOID        SetLocalDirection( XMVECTOR& Direction );
    VOID        SetWorldDirection( XMVECTOR& Direction );

    FLOAT               GetLocalRange()
    CONST
    {
        return m_fRange;
    };
    VOID        SetLocalRange( FLOAT fRange )
    {
        m_fRange = fRange; RecalculateBound();
    }

    FLOAT       GetWorldRange();
    VOID        SetWorldRange( FLOAT fRange );

    FLOAT               GetInnerAngle()
    CONST
    {
        return m_fInnerAngle;
    };
    VOID        SetInnerAngle( FLOAT fInnerAngle )
    {
        m_fInnerAngle = fInnerAngle; RecalculateBound();
    }

    FLOAT               GetOuterAngle()
    CONST
    {
        return m_fOuterAngle;
    };
    VOID        SetOuterAngle( FLOAT fOuterAngle )
    {
        m_fOuterAngle = fOuterAngle; RecalculateBound();
    }

    FalloffType         GetFalloff()
    CONST
    {
        return m_FalloffType;
    };
    VOID        SetFalloff( FalloffType Falloff )
    {
        m_FalloffType = Falloff;
    }

    FalloffType         GetSpotFalloff()
    CONST
    {
        return m_SpotFalloffType;
    };
    VOID        SetSpotFalloff( FalloffType Falloff )
    {
        m_SpotFalloffType = Falloff;
    }

    XMMATRIX    GetLightProjection()
    {
        return XMMatrixPerspectiveFovLH( m_fOuterAngle, 1.0f, 0.1f, GetWorldRange() );
    }
    XMMATRIX    GetLightView()
    {
        XMVECTOR vD; return XMMatrixInverse( &vD, GetWorldTransform() );
    }
    XMMATRIX    GetLightViewProjection()
    {
        return GetLightView() * GetLightProjection();
    }

private:
    VOID        RecalculateBound();

    FLOAT m_fRange;
    FLOAT m_fInnerAngle;
    FLOAT m_fOuterAngle;

    FalloffType m_FalloffType;
    FalloffType m_SpotFalloffType;
};

} // namespace ATG

#endif // ATG_LIGHT_H
