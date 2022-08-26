//-----------------------------------------------------------------------------
// AtgDebugDraw.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgDebugDraw.h"
#include "AtgSimpleShaders.h"
#include "AtgUtil.h"

namespace ATG
{

extern D3DDevice* g_pd3dDevice;

XMMATRIX g_matViewProjection;

VOID DebugDraw::SetViewProjection( const XMMATRIX& matViewProjection )
{
    g_matViewProjection = matViewProjection;
}

VOID DebugDraw::DrawAxes( const XMMATRIX& matWorld )
{
    SimpleShaders::SetDeclPosColor();
    static const MeshVertexPC verts[] =
    {
        { XMFLOAT3( 0.0f, 0.0f, 0.0f ), 0xFFFF0000 },
        { XMFLOAT3( 1.0f, 0.0f, 0.0f ), 0xFFFF0000 },
        { XMFLOAT3( 0.0f, 0.0f, 0.0f ), 0xFF00FF00 },
        { XMFLOAT3( 0.0f, 1.0f, 0.0f ), 0xFF00FF00 },
        { XMFLOAT3( 0.0f, 0.0f, 0.0f ), 0xFF0000FF },
        { XMFLOAT3( 0.0f, 0.0f, 1.0f ), 0xFF0000FF },
    };
    SimpleShaders::BeginShader_Transformed_VertexColor( matWorld * g_matViewProjection );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 3, ( const VOID* )verts, sizeof( MeshVertexPC ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawCubeWireframe( const XMMATRIX& matWorld, D3DCOLOR Color )
{
    SimpleShaders::SetDeclPos();
    static const MeshVertexP verts[] =
    {
        XMFLOAT3( -1, -1, -1 ),
        XMFLOAT3( 1, -1, -1 ),
        XMFLOAT3( 1, -1, 1 ),
        XMFLOAT3( -1, -1, 1 ),
        XMFLOAT3( -1, 1, -1 ),
        XMFLOAT3( 1, 1, -1 ),
        XMFLOAT3( 1, 1, 1 ),
        XMFLOAT3( -1, 1, 1 ),
    };
    static const WORD indices[] =
    {
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        4, 5,
        5, 6,
        6, 7,
        7, 4,
        0, 4,
        1, 5,
        2, 6,
        3, 7
    };
    SimpleShaders::BeginShader_Transformed_ConstantColor( matWorld * g_matViewProjection, Color );
    g_pd3dDevice->DrawIndexedPrimitiveUP( D3DPT_LINELIST, 0, 8, 12, ( const VOID* )indices,
                                          D3DFMT_INDEX16, ( const VOID* )verts, sizeof( MeshVertexP ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawCubeWireframe( const XMFLOAT3& Center, const XMFLOAT3& HalfSize, D3DCOLOR Color )
{
    XMMATRIX matWorld = XMMatrixIdentity();
    matWorld._11 = HalfSize.x;
    matWorld._22 = HalfSize.y;
    matWorld._33 = HalfSize.z;
    matWorld._41 = Center.x;
    matWorld._42 = Center.y;
    matWorld._43 = Center.z;
    DrawCubeWireframe( matWorld, Color );
}

VOID DebugDraw::DrawLineSegment( const XMFLOAT3& Origin, const XMFLOAT3& End, D3DCOLOR Color )
{
    MeshVertexP verts[2];
    memcpy( &verts[0], &Origin, 3 * sizeof( FLOAT ) );
    memcpy( &verts[1], &End, 3 * sizeof( FLOAT ) );
    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, ( const VOID* )verts, sizeof( MeshVertexP ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawRay( const XMFLOAT3& Origin, const XMFLOAT3& Direction, BOOL bNormalize, D3DCOLOR Color )
{
    MeshVertexP verts[3];
    memcpy( &verts[0], &Origin, 3 * sizeof( FLOAT ) );

    XMVECTOR RayOrigin = XMLoadFloat3( &Origin );
    XMVECTOR RayDirection = XMLoadFloat3( &Direction );
    XMVECTOR NormDirection = XMVector3Normalize( RayDirection );
    if( bNormalize )
        RayDirection = NormDirection;

    XMVECTOR PerpVector;
    XMVECTOR CrossVector = XMVectorSet( 0, 1, 0, 0 );
    PerpVector = XMVector3Cross( NormDirection, CrossVector );

    if( XMVector3Equal( XMVector3LengthSq( PerpVector ), XMVectorSet( 0, 0, 0, 0 ) ) )
    {
        CrossVector = XMVectorSet( 0, 0, 1, 0 );
        PerpVector = XMVector3Cross( NormDirection, CrossVector );
    }
    PerpVector = XMVector3Normalize( PerpVector );

    XMStoreFloat3( ( XMFLOAT3* )&verts[1], XMVectorAdd( RayDirection, RayOrigin ) );
    PerpVector = XMVectorScale( PerpVector, 0.0625f );
    NormDirection = XMVectorScale( NormDirection, -0.25f );
    RayDirection = XMVectorAdd( PerpVector, RayDirection );
    RayDirection = XMVectorAdd( NormDirection, RayDirection );
    XMStoreFloat3( ( XMFLOAT3* )&verts[2], XMVectorAdd( RayDirection, RayOrigin ) );

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, 2, ( const VOID* )verts, sizeof( MeshVertexP ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawRing( const XMFLOAT3& Origin, const XMFLOAT3& MajorAxis, const XMFLOAT3& MinorAxis,
                          D3DCOLOR Color )
{
    static const DWORD dwRingSegments = 32;
    MeshVertexP verts[ dwRingSegments + 1 ];

    XMVECTOR vOrigin = XMLoadFloat3( &Origin );
    XMVECTOR vMajor = XMLoadFloat3( &MajorAxis );
    XMVECTOR vMinor = XMLoadFloat3( &MinorAxis );

    FLOAT fAngleDelta = XM_2PI / ( float )dwRingSegments;
    // Instead of calling cos/sin for each segment we calculate
    // the sign of the angle delta and then incrementally calculate sin
    // and cosine from then on.
    XMVECTOR cosDelta = XMVectorReplicate( cosf( fAngleDelta ) );
    XMVECTOR sinDelta = XMVectorReplicate( sinf( fAngleDelta ) );
    XMVECTOR incrementalSin = XMVectorZero();
    static const XMVECTOR initialCos =
    {
        1.0f, 1.0f, 1.0f, 1.0f
    };
    XMVECTOR incrementalCos = initialCos;
    for( DWORD i = 0; i < dwRingSegments; i++ )
    {
        XMVECTOR Pos;
        Pos = XMVectorMultiplyAdd( vMajor, incrementalCos, vOrigin );
        Pos = XMVectorMultiplyAdd( vMinor, incrementalSin, Pos );
        XMStoreFloat3( ( XMFLOAT3* )&verts[i], Pos );
        // Standard formula to rotate a vector.
        XMVECTOR newCos = incrementalCos * cosDelta - incrementalSin * sinDelta;
        XMVECTOR newSin = incrementalCos * sinDelta + incrementalSin * cosDelta;
        incrementalCos = newCos;
        incrementalSin = newSin;
    }
    verts[ dwRingSegments ] = verts[0];

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, dwRingSegments, ( const VOID* )verts, sizeof( MeshVertexP ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawSphere( const XMFLOAT3& Origin, FLOAT fRadius, D3DCOLOR Color )
{
    DrawRing( Origin, XMFLOAT3( fRadius, 0, 0 ), XMFLOAT3( 0, 0, fRadius ), Color );
    DrawRing( Origin, XMFLOAT3( fRadius, 0, 0 ), XMFLOAT3( 0, fRadius, 0 ), Color );
    DrawRing( Origin, XMFLOAT3( 0, fRadius, 0 ), XMFLOAT3( 0, 0, fRadius ), Color );
}

VOID DebugDraw::DrawGrid( const XMFLOAT3& XAxis, const XMFLOAT3& YAxis, const XMFLOAT3& Origin, INT iXDivisions,
                          INT iYDivisions, D3DCOLOR Color )
{
    iXDivisions = max( 1, iXDivisions );
    iYDivisions = max( 1, iYDivisions );

    // build grid geometry
    INT iLineCount = iXDivisions + iYDivisions + 2;
    XMFLOAT3* pLines = new XMFLOAT3[ 2 * iLineCount ];

    XMVECTOR vX = XMLoadFloat3( &XAxis );
    XMVECTOR vY = XMLoadFloat3( &YAxis );
    XMVECTOR vOrigin = XMLoadFloat3( &Origin );

    for( INT i = 0; i <= iXDivisions; i++ )
    {
        FLOAT fPercent = ( FLOAT )i / ( FLOAT )iXDivisions;
        fPercent = ( fPercent * 2.0f ) - 1.0f;
        XMVECTOR vScale = XMVectorScale( vX, fPercent );
        vScale = XMVectorAdd( vScale, vOrigin );
        XMStoreFloat3( &pLines[ ( i * 2 ) ], XMVectorSubtract( vScale, vY ) );
        XMStoreFloat3( &pLines[ ( i * 2 ) + 1 ], XMVectorAdd( vScale, vY ) );
    }

    INT iStartIndex = ( iXDivisions + 1 ) * 2;
    for( INT i = 0; i <= iYDivisions; i++ )
    {
        FLOAT fPercent = ( FLOAT )i / ( FLOAT )iYDivisions;
        fPercent = ( fPercent * 2.0f ) - 1.0f;
        XMVECTOR vScale = XMVectorScale( vY, fPercent );
        vScale = XMVectorAdd( vScale, vOrigin );
        XMStoreFloat3( &pLines[ ( i * 2 ) + iStartIndex ], XMVectorSubtract( vScale, vX ) );
        XMStoreFloat3( &pLines[ ( i * 2 ) + 1 + iStartIndex ], XMVectorAdd( vScale, vX ) );
    }

    // draw grid
    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, iLineCount, pLines, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();

    delete[] pLines;
}

VOID DebugDraw::DrawTriangle( const XMFLOAT3& PointA, const XMFLOAT3& PointB, const XMFLOAT3& PointC, D3DCOLOR Color )
{
    XMFLOAT3 Verts[4];
    Verts[0] = PointA;
    Verts[1] = PointB;
    Verts[2] = PointC;
    Verts[3] = PointA;

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, 3, &Verts, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawConeWireframe( const XMFLOAT3& CenterBase, const XMFLOAT3& Axis, FLOAT fBaseRadius,
                                   FLOAT fTopRadius, D3DCOLOR Color )
{
    XMVECTOR vAxis = XMLoadFloat3( &Axis );
    XMVECTOR vAxisNorm = XMVector3Normalize( vAxis );

    // compute orthogonal vectors for the base of the cone
    XMVECTOR vBaseVectorX = XMVector3Normalize( XMVector3Orthogonal( vAxisNorm ) );
    XMVECTOR vBaseVectorZ = XMVector3Cross( vAxisNorm, vBaseVectorX );
    XMVECTOR vBaseVectorXScale = XMVectorScale( vBaseVectorX, fBaseRadius );
    XMVECTOR vBaseVectorZScale = XMVectorScale( vBaseVectorZ, fBaseRadius );

    // draw base ring
    if( fBaseRadius != 0 )
    {
        XMFLOAT3 BaseRingAxisX, BaseRingAxisZ;
        XMStoreFloat3( &BaseRingAxisX, vBaseVectorXScale );
        XMStoreFloat3( &BaseRingAxisZ, vBaseVectorZScale );
        DrawRing( CenterBase, BaseRingAxisX, BaseRingAxisZ, Color );
    }

    // compute center top location
    XMVECTOR vCenterTop = XMLoadFloat3( &CenterBase );
    vCenterTop = XMVectorAdd( vCenterTop, vAxis );

    // compute orthogonal vectors for the top of the cone
    XMVECTOR vTopVectorXScale = XMVectorScale( vBaseVectorX, fTopRadius );
    XMVECTOR vTopVectorZScale = XMVectorScale( vBaseVectorZ, fTopRadius );

    // build top ring
    if( fTopRadius != 0 )
    {
        XMFLOAT3 CenterTop;
        XMStoreFloat3( &CenterTop, vCenterTop );
        XMFLOAT3 TopRingAxisX, TopRingAxisZ;
        XMStoreFloat3( &TopRingAxisX, vTopVectorXScale );
        XMStoreFloat3( &TopRingAxisZ, vTopVectorZScale );
        DrawRing( CenterTop, TopRingAxisX, TopRingAxisZ, Color );
    }

    // build walls
    XMVECTOR vCenterBase = XMLoadFloat3( &CenterBase );
    const DWORD dwDivisions = 8;
    XMFLOAT3 WallVerts[ dwDivisions * 2 ];
    for( DWORD i = 0; i < dwDivisions; ++i )
    {
        FLOAT fTheta = ( ( FLOAT )i * XM_2PI ) / ( FLOAT )dwDivisions;
        XMVECTOR vSin, vCos;
        XMVectorSinCos( &vSin, &vCos, XMVectorReplicate( fTheta ) );
        XMVECTOR vBottom = vCenterBase + vBaseVectorXScale * vSin + vBaseVectorZScale * vCos;
        XMVECTOR vTop = vCenterTop + vTopVectorXScale * vSin + vTopVectorZScale * vCos;
        XMStoreFloat3( &WallVerts[ i * 2 ], vBottom );
        XMStoreFloat3( &WallVerts[ i * 2 + 1 ], vTop );
    }

    // draw walls
    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, dwDivisions, WallVerts, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawConeWireframe( const XMMATRIX& matWorld, FLOAT fBaseRadius, FLOAT fTopRadius, D3DCOLOR Color )
{
    XMVECTOR vCenterBase = matWorld.r[3];
    XMVECTOR vAxisY = matWorld.r[1];
    XMFLOAT3 CenterBase, Axis;
    XMStoreFloat3( &CenterBase, vCenterBase );
    XMStoreFloat3( &Axis, vAxisY );
    DrawConeWireframe( CenterBase, Axis, fBaseRadius, fTopRadius, Color );
}

VOID DebugDraw::DrawQuad( const XMFLOAT3& CornerA, const XMFLOAT3& CornerB, const XMFLOAT3& CornerC, D3DCOLOR Color )
{
    XMVECTOR vCornerA = XMLoadFloat3( &CornerA );
    XMVECTOR vCornerB = XMLoadFloat3( &CornerB );
    XMVECTOR vCornerC = XMLoadFloat3( &CornerC );

    XMVECTOR vCornerD = vCornerB + vCornerC - vCornerA;

    XMFLOAT3 vCorners[4];
    XMStoreFloat3( &vCorners[0], vCornerB );
    XMStoreFloat3( &vCorners[1], vCornerA );
    XMStoreFloat3( &vCorners[2], vCornerC );
    XMStoreFloat3( &vCorners[3], vCornerD );

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, vCorners, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawQuad( const XMFLOAT3& CornerA, const XMFLOAT3& CornerB, const XMFLOAT3& CornerC,
                          const XMFLOAT3& CornerD, D3DCOLOR Color )
{
    XMFLOAT3 vCorners[4];
    vCorners[0] = CornerA;
    vCorners[1] = CornerB;
    vCorners[2] = CornerC;
    vCorners[3] = CornerD;

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, vCorners, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();
}


VOID DebugDraw::DrawTexturedQuad( const XMFLOAT3& CornerA, const XMFLOAT3& CornerB, const XMFLOAT3& CornerC,
                                  const XMFLOAT2& UVRepeat, D3DTexture* pTexture )
{
    XMVECTOR vCornerA = XMLoadFloat3( &CornerA );
    XMVECTOR vCornerB = XMLoadFloat3( &CornerB );
    XMVECTOR vCornerC = XMLoadFloat3( &CornerC );

    XMVECTOR vCornerD = vCornerB + vCornerC - vCornerA;

    FLOAT vCorners[4 * 5];
    XMStoreFloat3( ( XMFLOAT3* )&vCorners[0], vCornerB );
    vCorners[3] = UVRepeat.x;
    vCorners[4] = 0.0f;
    XMStoreFloat3( ( XMFLOAT3* )&vCorners[5], vCornerA );
    vCorners[8] = 0.0f;
    vCorners[9] = 0.0f;
    XMStoreFloat3( ( XMFLOAT3* )&vCorners[10], vCornerC );
    vCorners[13] = 0.0f;
    vCorners[14] = UVRepeat.y;
    XMStoreFloat3( ( XMFLOAT3* )&vCorners[15], vCornerD );
    vCorners[18] = UVRepeat.x;
    vCorners[19] = UVRepeat.y;

    SimpleShaders::SetDeclPosTex();
    SimpleShaders::BeginShader_Transformed_Textured( g_matViewProjection, pTexture );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, vCorners, 5 * sizeof( FLOAT ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawCubeQuery( const XMFLOAT3& Center, const XMFLOAT3& HalfSize )
{
    // Index buffer data for a cube (using a quad list).
    static const WORD pIndexData[] =
    {
        3, 2, 0, 1,
        1, 0, 4, 5,
        3, 1, 5, 7,
        2, 3, 7, 6,
        0, 2, 6, 4,
        5, 4, 6, 7
    };
    static const XMFLOAT3 pVertexData[] =
    {
        XMFLOAT3( -1, -1, -1 ),
        XMFLOAT3( 1, -1, -1 ),
        XMFLOAT3( -1, -1, 1 ),
        XMFLOAT3( 1, -1, 1 ),
        XMFLOAT3( -1, 1, -1 ),
        XMFLOAT3( 1, 1, -1 ),
        XMFLOAT3( -1, 1, 1 ),
        XMFLOAT3( 1, 1, 1 )
    };

    XMMATRIX matTranslate = XMMatrixTranslationFromVector( XMLoadFloat3( &Center ) );
    XMMATRIX matScale = XMMatrixScalingFromVector( XMLoadFloat3( &HalfSize ) );
    XMMATRIX matTransform = matScale * matTranslate * g_matViewProjection;

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_DepthOnly( matTransform );
    g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    g_pd3dDevice->DrawIndexedPrimitiveUP( D3DPT_QUADLIST, 0, 8, 6, pIndexData, D3DFMT_INDEX16,
                                          pVertexData, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();
}


VOID DebugDraw::DrawScreenSpaceLine( const XMFLOAT2& Origin, const XMFLOAT2& End, D3DCOLOR Color, FLOAT fLineWidth )
{
    DrawScreenSpaceLine( Origin, Color, End, Color, fLineWidth );
}


VOID DebugDraw::DrawScreenSpaceLine( const XMFLOAT2& Origin, D3DCOLOR OriginColor, const XMFLOAT2& End, D3DCOLOR EndColor, FLOAT fLineWidth )
{
    ATG::MeshVertexPC Vertices[ 2 ];
    Vertices[0].Position = XMFLOAT3( Origin.x, Origin.y, 0 );
    Vertices[0].Color    = OriginColor;
    Vertices[1].Position = XMFLOAT3( End.x, End.y, 0 );
    Vertices[1].Color    = EndColor;


    DWORD dwLineWidthSaved;
    g_pd3dDevice->GetRenderState( D3DRS_LINEWIDTH, &dwLineWidthSaved );
    g_pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, *((DWORD*)&fLineWidth));

    SimpleShaders::SetDeclPosColor();
    SimpleShaders::BeginShader_PreTransformed_VertexColor();
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, 1, Vertices, sizeof( ATG::MeshVertexPC ) );
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );


    g_pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, dwLineWidthSaved );
}


VOID DebugDraw::DrawScreenSpaceLineList( const XMFLOAT2 Points[], UINT PointCount, D3DCOLOR Color, FLOAT fLineWidth )
{
    ATG::MeshVertexPC* Vertices = new ATG::MeshVertexPC[ PointCount ];
    for( UINT i = 0; i < PointCount; ++i )
    {
        Vertices[i].Position = XMFLOAT3( Points[i].x, Points[i].y, 0 );
        Vertices[i].Color    = Color;
    }

    DWORD dwLineWidthSaved;
    g_pd3dDevice->GetRenderState( D3DRS_LINEWIDTH, &dwLineWidthSaved );
    g_pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, *((DWORD*)&fLineWidth));

    SimpleShaders::SetDeclPosColor();
    SimpleShaders::BeginShader_PreTransformed_VertexColor();
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINESTRIP, PointCount - 1, Vertices, sizeof( ATG::MeshVertexPC ) );
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );


    g_pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, dwLineWidthSaved );

    delete[] Vertices;
}


VOID DebugDraw::DrawScreenSpaceRect( const D3DRECT& Rect, FLOAT fLineWidth, D3DCOLOR Color )
{
    XMFLOAT2 Origin( ( FLOAT )Rect.x1, ( FLOAT )Rect.y1 );
    XMFLOAT2 Size( ( FLOAT )( Rect.x2 - Rect.x1 ), ( FLOAT )( Rect.y2 - Rect.y1 ) );
    DrawScreenSpaceRect( Origin, Size, fLineWidth, Color );
}

VOID DebugDraw::DrawScreenSpaceRect( const XMFLOAT2& Origin, const XMFLOAT2& Size, FLOAT fLineWidth, D3DCOLOR Color )
{
    DWORD dwQuadCount = 1;
    XMFLOAT4 Vertices[12];
    if( fLineWidth > 0 )
    {
        dwQuadCount = 4;
        Vertices[0] = XMFLOAT4( Origin.x, Origin.y, 0, 1 );
        Vertices[1] = XMFLOAT4( Origin.x + Size.x, Origin.y, 0, 1 );
        Vertices[2] = XMFLOAT4( Origin.x, Origin.y + fLineWidth, 0, 1 );

        Vertices[3] = XMFLOAT4( Origin.x + Size.x - fLineWidth, Origin.y, 0, 1 );
        Vertices[4] = XMFLOAT4( Origin.x + Size.x, Origin.y, 0, 1 );
        Vertices[5] = XMFLOAT4( Origin.x + Size.x - fLineWidth, Origin.y + Size.y, 0, 1 );

        Vertices[ 6] = XMFLOAT4( Origin.x, Origin.y + Size.y - fLineWidth, 0, 1 );
        Vertices[ 7] = XMFLOAT4( Origin.x + Size.x, Origin.y + Size.y - fLineWidth, 0, 1 );
        Vertices[ 8] = XMFLOAT4( Origin.x, Origin.y + Size.y, 0, 1 );

        Vertices[ 9] = XMFLOAT4( Origin.x, Origin.y, 0, 1 );
        Vertices[10] = XMFLOAT4( Origin.x + fLineWidth, Origin.y, 0, 1 );
        Vertices[11] = XMFLOAT4( Origin.x, Origin.y + Size.y, 0, 1 );
    }
    else
    {
        Vertices[0] = XMFLOAT4( Origin.x, Origin.y, 0, 1 );
        Vertices[1] = XMFLOAT4( Origin.x + Size.x, Origin.y, 0, 1 );
        Vertices[2] = XMFLOAT4( Origin.x, Origin.y + Size.y, 0, 1 );
    }

    SimpleShaders::SetDeclPosT();
    SimpleShaders::BeginShader_PreTransformed_ConstantColor( Color );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_RECTLIST, dwQuadCount, Vertices, sizeof( XMFLOAT4 ) );
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
}

VOID DebugDraw::DrawScreenSpaceTexturedRectPatch( const D3DRECT& Rect, XMFLOAT2 UVTopLeft, XMFLOAT2 UVTopRight, XMFLOAT2 UVBottomLeft, const D3DBaseTexture* pTexture, BOOL bDepthTexture )
{
    SimpleShaders::SetDeclPosTex();
    if( bDepthTexture )
    {
        SimpleShaders::BeginShader_PreTransformed_DepthTextured( pTexture );
    }
    else
    {
        SimpleShaders::BeginShader_PreTransformed_Textured( pTexture );
    }
    MeshVertexPT* pVertexData = NULL;
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    HRESULT hr = g_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( MeshVertexPT ), ( VOID** )&pVertexData );
    // The ring buffer may run out of space when tiling, doing z-prepasses,
    // or using BeginCommandBuffer. If so, make the buffer larger.
    if( FAILED( hr ) )
        FatalError( "Ring buffer out of memory.\n" );
    assert( pVertexData != NULL );
    pVertexData[0].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y1, 0 );
    pVertexData[0].TexCoord = UVTopLeft;
    pVertexData[1].Position = XMFLOAT3( ( FLOAT )Rect.x2, ( FLOAT )Rect.y1, 0 );
    pVertexData[1].TexCoord = UVTopRight;
    pVertexData[2].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y2, 0 );
    pVertexData[2].TexCoord = UVBottomLeft;
    g_pd3dDevice->EndVertices();
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
}

VOID DebugDraw::DrawScreenSpaceTexturedRectPatchPointSampled ( const D3DRECT& Rect, XMFLOAT2 UVTopLeft, XMFLOAT2 UVTopRight, XMFLOAT2 UVBottomLeft, const D3DBaseTexture* pTexture, BOOL bDepthTexture )
{
    SimpleShaders::SetDeclPosTex();
    if( bDepthTexture )
    {
        SimpleShaders::BeginShader_PreTransformed_DepthTextured( pTexture );
    }
    else
    {
        SimpleShaders::BeginShader_PreTransformed_Textured( pTexture );
    }
    MeshVertexPT* pVertexData = NULL;
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    HRESULT hr = g_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( MeshVertexPT ), ( VOID** )&pVertexData );
    // The ring buffer may run out of space when tiling, doing z-prepasses,
    // or using BeginCommandBuffer. If so, make the buffer larger.
    if( FAILED( hr ) )
        FatalError( "Ring buffer out of memory.\n" );
    assert( pVertexData != NULL );
    pVertexData[0].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y1, 0 );
    pVertexData[0].TexCoord = UVTopLeft;
    pVertexData[1].Position = XMFLOAT3( ( FLOAT )Rect.x2, ( FLOAT )Rect.y1, 0 );
    pVertexData[1].TexCoord = UVTopRight;
    pVertexData[2].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y2, 0 );
    pVertexData[2].TexCoord = UVBottomLeft;
    g_pd3dDevice->EndVertices();
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
}

VOID DebugDraw::DrawScreenSpaceTexturedRect( const D3DRECT& Rect, const D3DBaseTexture* pTexture, BOOL bDepthTexture )
{
    SimpleShaders::SetDeclPosTex();
    if( bDepthTexture )
    {
        SimpleShaders::BeginShader_PreTransformed_DepthTextured( pTexture );
    }
    else
    {
        SimpleShaders::BeginShader_PreTransformed_Textured( pTexture );
    }
    MeshVertexPT* pVertexData = NULL;
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    HRESULT hr = g_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( MeshVertexPT ), ( VOID** )&pVertexData );
    // The ring buffer may run out of space when tiling, doing z-prepasses,
    // or using BeginCommandBuffer. If so, make the buffer larger.
    if( FAILED( hr ) )
        FatalError( "Ring buffer out of memory.\n" );
    assert( pVertexData != NULL );
    pVertexData[0].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y1, 0 );
    pVertexData[0].TexCoord = XMFLOAT2( 0, 0 );
    pVertexData[1].Position = XMFLOAT3( ( FLOAT )Rect.x2, ( FLOAT )Rect.y1, 0 );
    pVertexData[1].TexCoord = XMFLOAT2( 1, 0 );
    pVertexData[2].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y2, 0 );
    pVertexData[2].TexCoord = XMFLOAT2( 0, 1 );
    g_pd3dDevice->EndVertices();
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
}

VOID DebugDraw::DrawScreenSpaceTexturedRectColored( const D3DRECT& Rect, const D3DBaseTexture* pTexture, D3DCOLOR Color )
{
    SimpleShaders::SetDeclPosTex();
    SimpleShaders::BeginShader_PreTransformed_TexturedConstantColor( pTexture, Color );
    MeshVertexPT* pVertexData = NULL;
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    HRESULT hr = g_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( MeshVertexPT ), ( VOID** )&pVertexData );
    // The ring buffer may run out of space when tiling, doing z-prepasses,
    // or using BeginCommandBuffer. If so, make the buffer larger.
    if( FAILED( hr ) )
        FatalError( "Ring buffer out of memory.\n" );
    assert( pVertexData != NULL );
    pVertexData[0].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y1, 0 );
    pVertexData[0].TexCoord = XMFLOAT2( 0, 0 );
    pVertexData[1].Position = XMFLOAT3( ( FLOAT )Rect.x2, ( FLOAT )Rect.y1, 0 );
    pVertexData[1].TexCoord = XMFLOAT2( 1, 0 );
    pVertexData[2].Position = XMFLOAT3( ( FLOAT )Rect.x1, ( FLOAT )Rect.y2, 0 );
    pVertexData[2].TexCoord = XMFLOAT2( 0, 1 );
    g_pd3dDevice->EndVertices();
    SimpleShaders::EndShader();
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
}

VOID DebugDraw::DrawBound( const Bound& bound, D3DCOLOR Color )
{
    switch( bound.GetType() )
    {
        case Bound::Sphere_Bound:
            DrawSphere( bound.GetSphere(), Color );
            return;
        case Bound::Frustum_Bound:
            DrawFrustum( bound.GetFrustum(), Color );
            return;
        case Bound::AABB_Bound:
            DrawAabb( bound.GetAabb(), Color );
            return;
        case Bound::OBB_Bound:
            DrawObb( bound.GetObb(), Color );
            return;
    }
}

VOID DebugDraw::DrawSphere( const Sphere& sphere, D3DCOLOR Color )
{
    DrawSphere( sphere.Center, sphere.Radius, Color );
}

VOID DebugDraw::DrawFrustum( const Frustum& frustum, D3DCOLOR Color )
{
    // compute corner points

    XMVECTOR Origin = XMVectorSet( frustum.Origin.x, frustum.Origin.y, frustum.Origin.z, 0 );
    FLOAT Near = frustum.Near;
    FLOAT Far = frustum.Far;
    FLOAT RightSlope = frustum.RightSlope;
    FLOAT LeftSlope = frustum.LeftSlope;
    FLOAT TopSlope = frustum.TopSlope;
    FLOAT BottomSlope = frustum.BottomSlope;

    XMFLOAT3 CornerPoints[8];
    CornerPoints[0] = XMFLOAT3( RightSlope * Near, TopSlope * Near, Near );
    CornerPoints[1] = XMFLOAT3( LeftSlope * Near, TopSlope * Near, Near );
    CornerPoints[2] = XMFLOAT3( LeftSlope * Near, BottomSlope * Near, Near );
    CornerPoints[3] = XMFLOAT3( RightSlope * Near, BottomSlope * Near, Near );

    CornerPoints[4] = XMFLOAT3( RightSlope * Far, TopSlope * Far, Far );
    CornerPoints[5] = XMFLOAT3( LeftSlope * Far, TopSlope * Far, Far );
    CornerPoints[6] = XMFLOAT3( LeftSlope * Far, BottomSlope * Far, Far );
    CornerPoints[7] = XMFLOAT3( RightSlope * Far, BottomSlope * Far, Far );

    XMVECTOR Orientation = XMLoadFloat4( &frustum.Orientation );
    XMMATRIX Mat = XMMatrixRotationQuaternion( Orientation );
    for( UINT i = 0; i < 8; i++ )
    {
        XMVECTOR Result = XMVector3Transform( XMLoadFloat3( &CornerPoints[i] ), Mat );
        Result = XMVectorAdd( Result, Origin );
        XMStoreFloat3( &CornerPoints[i], Result );
    }

    XMFLOAT3 Lines[12 * 2];

    Lines[0] = CornerPoints[0];
    Lines[1] = CornerPoints[1];
    Lines[2] = CornerPoints[1];
    Lines[3] = CornerPoints[2];
    Lines[4] = CornerPoints[2];
    Lines[5] = CornerPoints[3];
    Lines[6] = CornerPoints[3];
    Lines[7] = CornerPoints[0];

    Lines[8] = CornerPoints[0];
    Lines[9] = CornerPoints[4];
    Lines[10] = CornerPoints[1];
    Lines[11] = CornerPoints[5];
    Lines[12] = CornerPoints[2];
    Lines[13] = CornerPoints[6];
    Lines[14] = CornerPoints[3];
    Lines[15] = CornerPoints[7];

    Lines[16] = CornerPoints[4];
    Lines[17] = CornerPoints[5];
    Lines[18] = CornerPoints[5];
    Lines[19] = CornerPoints[6];
    Lines[20] = CornerPoints[6];
    Lines[21] = CornerPoints[7];
    Lines[22] = CornerPoints[7];
    Lines[23] = CornerPoints[4];

    // draw frustum

    SimpleShaders::SetDeclPos();
    SimpleShaders::BeginShader_Transformed_ConstantColor( g_matViewProjection, Color );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 12, Lines, sizeof( XMFLOAT3 ) );
    SimpleShaders::EndShader();
}

VOID DebugDraw::DrawAabb( const AxisAlignedBox& box, D3DCOLOR Color )
{
    XMMATRIX matWorld = XMMatrixScaling( box.Extents.x, box.Extents.y, box.Extents.z );
    XMVECTOR position = XMLoadFloat3( &box.Center );
    matWorld.r[3] = XMVectorSelect( matWorld.r[3], position, XMVectorSelectControl( 1, 1, 1, 0 ) );

    DrawCubeWireframe( matWorld, Color );
}

VOID DebugDraw::DrawObb( const OrientedBox& obb, D3DCOLOR Color )
{
    XMMATRIX matWorld = XMMatrixRotationQuaternion( XMLoadFloat4( &obb.Orientation ) );
    XMMATRIX matScale = XMMatrixScaling( obb.Extents.x, obb.Extents.y, obb.Extents.z );
    matWorld = XMMatrixMultiply( matScale, matWorld );
    XMVECTOR position = XMLoadFloat3( &obb.Center );
    matWorld.r[3] = XMVectorSelect( matWorld.r[3], position, XMVectorSelectControl( 1, 1, 1, 0 ) );

    DrawCubeWireframe( matWorld, Color );
}

} // namespace ATG
