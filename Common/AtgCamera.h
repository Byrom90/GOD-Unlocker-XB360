//-----------------------------------------------------------------------------
// AtgCamera.h
//
// describes a camera in the scene
//
// $OPTIMIZE: Pool Allocate
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_CAMERA_H
#define ATG_CAMERA_H

#include "AtgFrame.h"

namespace ATG
{

//-----------------------------------------------------------------------------
// Name: Projection
// Desc: simple projection class to handle various projection types.
//-----------------------------------------------------------------------------
class Projection
{
public:

    enum ProjectionType
    {
        Perspective,
        Orthographic,

        ForceDWORD = 0x7fffffff
    };      
    
    static CONST EnumStringMap ProjectionType_StringMap[];

    // set projection
    VOID            SetFovXFovY( FLOAT FovX, FLOAT FovY, FLOAT ZNear, FLOAT ZFar );
    VOID            SetFovYAspect( FLOAT FovY, FLOAT Aspect, FLOAT ZNear, FLOAT ZFar );
    VOID            SetFovXAspect( FLOAT FovX, FLOAT Aspect, FLOAT ZNear, FLOAT ZFar );
    VOID            SetOrthographic( FLOAT Width, FLOAT Height, FLOAT ZNear, FLOAT ZFar );

    // get projection data
    XMMATRIX            GetMatrix() CONST;
    Frustum             GetFrustum() CONST;
    FLOAT               GetFovX() CONST;
    FLOAT               GetFovY() CONST;
    ProjectionType      GetType() CONST { return m_Type; }
    FLOAT               GetZNear() CONST { return m_ZNear; }
    FLOAT               GetZFar() CONST { return m_ZFar; }
    FLOAT               GetWidth() CONST { return m_Width; }
    FLOAT               GetHeight() CONST { return m_Height; }

private:
    ProjectionType m_Type;
    FLOAT m_ZNear;
    FLOAT m_ZFar;
    FLOAT m_Width;
    FLOAT m_Height;
};

//-----------------------------------------------------------------------------
// Name: Camera
// Desc: basic camera class
//-----------------------------------------------------------------------------
class Camera : public Frame
{
    DEFINE_TYPE_INFO();
public:
    //  viewport
    VOID                SetViewport( CONST D3DVIEWPORT9& Viewport ) { m_Viewport = Viewport; }
    CONST D3DVIEWPORT9& GetViewport() CONST { return m_Viewport; }

    // view matrices
    XMMATRIX            GetLocalView() CONST { XMVECTOR pD; return XMMatrixInverse( &pD, GetLocalTransform() ); }
    XMMATRIX            GetWorldView() { XMVECTOR pD; return XMMatrixInverse( &pD, GetWorldTransform() ); }
    XMMATRIX            GetLocalInvView() CONST { return GetLocalTransform(); }
    XMMATRIX            GetWorldInvView() { return GetWorldTransform(); }

    // Focal length
    FLOAT               GetFocalLength() CONST { return m_fFocalLength; }
    VOID                SetFocalLength( FLOAT fFocalLength ) { m_fFocalLength = fFocalLength; }

    // Projection
    CONST Projection&   GetProjection() CONST { return m_Projection; }
    VOID                SetProjection( CONST Projection& Proj ) { m_Projection = Proj; UpdateBound(); }

    // Clear color
    DWORD               GetClearColor() CONST { return m_dwClearColor; }
    VOID                SetClearColor( DWORD ClearColor ) { m_dwClearColor = ClearColor; }

private:
    VOID UpdateBound();

    Projection m_Projection;
    FLOAT m_fFocalLength;
    D3DVIEWPORT9 m_Viewport;
    DWORD m_dwClearColor;
};

} // namespace ATG

#endif // ATG_CAMERA_H
