//-----------------------------------------------------------------------------
// AtgDebugDraw.h
//
// A rendering library for drawing simple primitives.  These are not optimized for
// speed - just ease of use.
//
// You must initialize the SimpleShaders class in AtgSimpleShaders.h/.cpp before using
// the debug primitives.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATGDEBUGDRAW_H
#define ATGDEBUGDRAW_H

#include <xboxmath.h>
#include "AtgBound.h"
#include "AtgCollision.h"

namespace ATG
{

class DebugDraw
{
public:
    // Call SetViewProjection with your camera matrix before using the draw calls.
    static VOID SetViewProjection( const XMMATRIX& matViewProjection );

    // Debug draw commands.  All vectors and sizes are in world space.
    static VOID DrawAxes( const XMMATRIX& matWorld );
    static VOID DrawCubeWireframe( const XMFLOAT3& Center, const XMFLOAT3& HalfSize, D3DCOLOR Color );
    static VOID DrawCubeWireframe( const XMMATRIX& matWorld, D3DCOLOR Color );
    static VOID DrawLineSegment( const XMFLOAT3& Origin, const XMFLOAT3& End, D3DCOLOR Color );
    static VOID DrawRay( const XMFLOAT3& Origin, const XMFLOAT3& Direction, BOOL bNormalize, D3DCOLOR Color );
    static VOID DrawRing( const XMFLOAT3& Origin, const XMFLOAT3& MajorAxis, const XMFLOAT3& MinorAxis,
                          D3DCOLOR Color );
    static VOID DrawSphere( const XMFLOAT3& Origin, FLOAT fRadius, D3DCOLOR Color );
    static VOID DrawGrid( const XMFLOAT3& XAxis, const XMFLOAT3& YAxis, const XMFLOAT3& Origin, INT iXDivisions,
                          INT iYDivisions, D3DCOLOR Color );
    static VOID DrawTriangle( const XMFLOAT3& PointA, const XMFLOAT3& PointB, const XMFLOAT3& PointC, D3DCOLOR Color );
    static VOID DrawConeWireframe( const XMFLOAT3& CenterBase, const XMFLOAT3& Axis, FLOAT fBaseRadius,
                                   FLOAT fTopRadius, D3DCOLOR Color );
    static VOID DrawConeWireframe( const XMMATRIX& matWorld, FLOAT fBaseRadius, FLOAT fTopRadius, D3DCOLOR Color );
    static VOID DrawQuad( const XMFLOAT3& CornerA, const XMFLOAT3& CornerB, const XMFLOAT3& CornerC, D3DCOLOR Color );
    static VOID DrawQuad( const XMFLOAT3& CornerA, const XMFLOAT3& CornerB, const XMFLOAT3& CornerC, const XMFLOAT3& CornerD, D3DCOLOR Color );
    static VOID DrawTexturedQuad( const XMFLOAT3& CornerA, const XMFLOAT3& CornerB, const XMFLOAT3& CornerC,
                                  const XMFLOAT2& UVRepeat, D3DTexture* pTexture );
    static VOID DrawCubeQuery( const XMFLOAT3& Center, const XMFLOAT3& HalfSize );

    // Screen space debug draw primitives.  The viewport is disabled when drawing these primitives.
    // If the line width is less than 0, the screen space rect is drawn solid.
    static VOID DrawScreenSpaceLine( const XMFLOAT2& Origin, const XMFLOAT2& End, D3DCOLOR Color, FLOAT fLineWidth = 1 );
    static VOID DrawScreenSpaceLine( const XMFLOAT2& Origin, D3DCOLOR OriginColor, const XMFLOAT2& End, D3DCOLOR EndColor, FLOAT fLineWidth = 1 );
    static VOID DrawScreenSpaceLineList( const XMFLOAT2 Points[], UINT PointCount, D3DCOLOR Color, FLOAT fLineWidth = 1 );
    static VOID DrawScreenSpaceRect( const XMFLOAT2& Origin, const XMFLOAT2& Size, FLOAT fLineWidth, D3DCOLOR Color );
    static VOID DrawScreenSpaceRect( const D3DRECT& Rect, FLOAT fLineWidth, D3DCOLOR Color );
    static VOID DrawScreenSpaceTexturedRect( const D3DRECT& Rect, const D3DBaseTexture* pTexture, BOOL bDepthTexture =
                                             FALSE );
    static VOID DrawScreenSpaceTexturedRectColored( const D3DRECT& Rect, const D3DBaseTexture* pTexture, D3DCOLOR Color );

    static VOID DrawScreenSpaceTexturedRectPatch( const D3DRECT& Rect, XMFLOAT2 UVTopLeft, XMFLOAT2 UVTopRight, XMFLOAT2 UVBottomLeft, const D3DBaseTexture* pTexture, BOOL bDepthTexture =
                                             FALSE );
    static VOID DrawScreenSpaceTexturedRectPatchPointSampled( const D3DRECT& Rect, XMFLOAT2 UVTopLeft, XMFLOAT2 UVTopRight, XMFLOAT2 UVBottomLeft, const D3DBaseTexture* pTexture, BOOL bDepthTexture =
                                             FALSE );

    // Debug bounds drawing.
    static VOID DrawBound( const Bound& bound, D3DCOLOR Color );
    static VOID DrawSphere( const Sphere& sphere, D3DCOLOR Color );
    static VOID DrawFrustum( const Frustum& frustum, D3DCOLOR Color );
    static VOID DrawObb( const OrientedBox& obb, D3DCOLOR Color );
    static VOID DrawAabb( const AxisAlignedBox& box, D3DCOLOR Color );
};

} // namespace ATG

#endif
