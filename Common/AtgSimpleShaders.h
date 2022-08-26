//-----------------------------------------------------------------------------
// AtgSimpleShaders.h
//
// A library of standard shaders, vertex structs, and vertex declarations for use in
// simple rendering.  In a way, this is a replacement for what the fixed function
// pipeline provided.
//
// This code is dependent on a piece of content - media/simpleshaders.fx
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATGSIMPLESHADERS_H
#define ATGSIMPLESHADERS_H

#include <xboxmath.h>
#include <fxl.h>

namespace ATG
{

struct MeshVertexP
{
public:
    XMFLOAT3 Position;
    static size_t Size()
    {
        return sizeof( MeshVertexP );
    }
};

struct MeshVertexPTransformed
{
public:
    XMFLOAT4 PositionT;
    static size_t Size()
    {
        return sizeof( MeshVertexPTransformed );
    }
};

struct MeshVertexPC
{
public:
    XMFLOAT3 Position;
    D3DCOLOR Color;
    static size_t Size()
    {
        return sizeof( MeshVertexPC );
    }
};

struct MeshVertexPT
{
public:
    XMFLOAT3 Position;
    XMFLOAT2 TexCoord;
    static size_t Size()
    {
        return sizeof( MeshVertexPT );
    }
};

struct MeshVertexPCT
{
public:
    XMFLOAT3 Position;
    D3DCOLOR Color;
    XMFLOAT2 TexCoord;
    static size_t Size()
    {
        return sizeof( MeshVertexPCT );
    }
};

class SimpleShaders
{
public:
    static VOID Initialize( const CHAR* strShaderFileName, FXLEffectPool* pEffectPool );
    static VOID Terminate();

    static VOID BindToNewDevice( ::D3DDevice* pNewDevice );

    static VOID SetDeclPos();
    static VOID SetDeclPosT();
    static VOID SetDeclPosColor();
    static VOID SetDeclPosTex();
    static VOID SetDeclPosColorTex();

    static VOID BeginShader_Transformed_ConstantColor( const XMMATRIX& matWVP, D3DCOLOR Color );
    static VOID BeginShader_Transformed_VertexColor( const XMMATRIX& matWVP );
    static VOID BeginShader_Transformed_Textured( const XMMATRIX& matWVP, const D3DBaseTexture* pTexture );
    static VOID BeginShader_Transformed_TexturedConstantColor( const XMMATRIX& matWVP, const D3DBaseTexture* pTexture, D3DCOLOR Color );
    static VOID BeginShader_Transformed_TexturedVertexColor( const XMMATRIX& matWVP, const D3DBaseTexture* pTexture );
    static VOID BeginShader_Transformed_DepthOnly( const XMMATRIX& matWVP );

    static VOID BeginShader_PreTransformed_ConstantColor( D3DCOLOR Color );
    static VOID BeginShader_PreTransformed_VertexColor();
    static VOID BeginShader_PreTransformed_Textured( const D3DBaseTexture* pTexture );
    static VOID BeginShader_PreTransformed_DepthTextured( const D3DBaseTexture* pDepthTexture );
    static VOID BeginShader_PreTransformed_TexturedConstantColor( const D3DBaseTexture* pTexture, D3DCOLOR Color );
    static VOID BeginShader_PreTransformed_TexturedVertexColor( const D3DBaseTexture* pTexture );
    static VOID BeginShader_PreTransformed_DepthOnly();
    static VOID BeginShader_PreTransformed_DownsampleDepth( const D3DBaseTexture* pDepthTexture );
    static VOID BeginShader_PreTransformed_DownsampleDepthArray( const D3DBaseTexture* pDepthArrayTexture, DWORD dwSliceIndex );

    static VOID EndShader();
};

} // namespace ATG

#endif
