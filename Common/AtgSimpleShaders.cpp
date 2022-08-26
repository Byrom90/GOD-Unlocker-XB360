//-----------------------------------------------------------------------------
// AtgSimpleShaders.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgSimpleShaders.h"
#include "AtgUtil.h"

namespace ATG
{

extern D3DDevice* g_pd3dDevice;
::D3DDevice* g_pSimpleShadersDevice = NULL;

// effect objects
FXLEffect* g_pSimpleShaderEffect = NULL;
FXLEffectPool* g_pParameterPool = NULL;

// common vertex declarations
D3DVertexDeclaration* g_pVertexDeclPos = NULL;
D3DVertexDeclaration* g_pVertexDeclPosColor = NULL;
D3DVertexDeclaration* g_pVertexDeclPosT = NULL;
D3DVertexDeclaration* g_pVertexDeclPosTexCoord = NULL;
D3DVertexDeclaration* g_pVertexDeclPosColorTexCoord = NULL;

// technique handles
FXLHANDLE g_hTechniqueTransformedDepthOnly = NULL;
FXLHANDLE g_hTechniqueTransformedConstantColor = NULL;
FXLHANDLE g_hTechniqueTransformedVertexColor = NULL;
FXLHANDLE g_hTechniqueTransformedTextured = NULL;
FXLHANDLE g_hTechniqueTransformedTexturedConstantColor = NULL;
FXLHANDLE g_hTechniqueTransformedTexturedVertexColor = NULL;
FXLHANDLE g_hTechniquePreTransformedConstantColor = NULL;
FXLHANDLE g_hTechniquePreTransformedVertexColor = NULL;
FXLHANDLE g_hTechniquePreTransformedTextured = NULL;
FXLHANDLE g_hTechniquePreTransformedDepthTextured = NULL;
FXLHANDLE g_hTechniquePreTransformedTexturedConstantColor = NULL;
FXLHANDLE g_hTechniquePreTransformedTexturedVertexColor = NULL;
FXLHANDLE g_hTechniquePreTransformedDepthOnly = NULL;
FXLHANDLE g_hTechniquePreTransformedDownsampleDepth = NULL;
FXLHANDLE g_hTechniquePreTransformedDownsampleDepthArray = NULL;

// parameter handles
FXLHANDLE g_hWVP = NULL;
FXLHANDLE g_hConstantColor = NULL;
FXLHANDLE g_hSampler = NULL;

// parameter data
D3DXCOLOR g_ConstantColorData;

static const D3DVERTEXELEMENT9 g_PosVertexElements[] =
{
    { 0,     0, D3DDECLTYPE_FLOAT3,     0,  D3DDECLUSAGE_POSITION,  0 },
    D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_PosColorVertexElements[] =
{
    { 0,     0, D3DDECLTYPE_FLOAT3,     0,  D3DDECLUSAGE_POSITION,  0 },
    { 0,    12, D3DDECLTYPE_D3DCOLOR,   0,  D3DDECLUSAGE_COLOR,     0 },
    D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_PosTVertexElements[] =
{
    { 0,     0, D3DDECLTYPE_FLOAT4,     0,  D3DDECLUSAGE_POSITION,  0 },
    D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_PosTexCoordVertexElements[] =
{
    { 0,     0, D3DDECLTYPE_FLOAT3,     0,  D3DDECLUSAGE_POSITION,  0 },
    { 0,    12, D3DDECLTYPE_FLOAT2,     0,  D3DDECLUSAGE_TEXCOORD,  0 },
    D3DDECL_END()
};

static const D3DVERTEXELEMENT9 g_PosColorTexCoordVertexElements[] =
{
    { 0,     0, D3DDECLTYPE_FLOAT3,     0,  D3DDECLUSAGE_POSITION,  0 },
    { 0,    12, D3DDECLTYPE_D3DCOLOR,   0,  D3DDECLUSAGE_COLOR,     0 },
    { 0,    16, D3DDECLTYPE_FLOAT2,     0,  D3DDECLUSAGE_TEXCOORD,  0 },
    D3DDECL_END()
};

VOID SimpleShaders::Initialize( const CHAR* strShaderFileName, FXLEffectPool* pEffectPool )
{
    g_pParameterPool = pEffectPool;

    if( g_pSimpleShadersDevice == NULL )
        BindToNewDevice( g_pd3dDevice );

    // create vertex decls
    CreatePooledVertexDeclaration( g_PosVertexElements, &g_pVertexDeclPos );
    CreatePooledVertexDeclaration( g_PosColorVertexElements, &g_pVertexDeclPosColor );
    CreatePooledVertexDeclaration( g_PosTVertexElements, &g_pVertexDeclPosT );
    CreatePooledVertexDeclaration( g_PosTexCoordVertexElements, &g_pVertexDeclPosTexCoord );
    CreatePooledVertexDeclaration( g_PosColorTexCoordVertexElements, &g_pVertexDeclPosColorTexCoord );

    // create FXLite effect
    if( strShaderFileName == NULL )
        strShaderFileName = "game:\\media\\effects\\simpleshaders.fxobj";
    VOID* pEffectData = NULL;
    ATG::LoadFile( strShaderFileName, &pEffectData, NULL );
    FXLCreateEffect( g_pSimpleShadersDevice, pEffectData, pEffectPool, &g_pSimpleShaderEffect );
    ATG::UnloadFile( pEffectData );

    if( g_pSimpleShaderEffect == NULL )
    {
        FatalError( "Could not initialize SimpleShaders with FX file \"%s\".\n", strShaderFileName );
        return;
    }

    // get technique handles
    g_hTechniqueTransformedDepthOnly = g_pSimpleShaderEffect->GetTechniqueHandle( "Transformed_DepthOnly" );
    g_hTechniqueTransformedConstantColor = g_pSimpleShaderEffect->GetTechniqueHandle( "Transformed_ConstantColor" );
    g_hTechniqueTransformedVertexColor = g_pSimpleShaderEffect->GetTechniqueHandle( "Transformed_VertexColor" );
    g_hTechniqueTransformedTextured = g_pSimpleShaderEffect->GetTechniqueHandle( "Transformed_Textured" );
    g_hTechniqueTransformedTexturedConstantColor = g_pSimpleShaderEffect->GetTechniqueHandle( "Transformed_TextureConstantColor" );
    g_hTechniqueTransformedTexturedVertexColor = g_pSimpleShaderEffect->GetTechniqueHandle( "Transformed_TextureVertexColor" );

    g_hTechniquePreTransformedConstantColor = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_ConstantColor" );
    g_hTechniquePreTransformedVertexColor = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_VertexColor" );
    g_hTechniquePreTransformedTextured = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_Textured" );
    g_hTechniquePreTransformedDepthTextured = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_DepthTextured" );
    g_hTechniquePreTransformedTexturedConstantColor = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_TextureConstantColor" );
    g_hTechniquePreTransformedTexturedVertexColor = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_TextureVertexColor" );
    g_hTechniquePreTransformedDepthOnly = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_DepthOnly" );
    g_hTechniquePreTransformedDownsampleDepth = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_DownsampleDepth" );
    g_hTechniquePreTransformedDownsampleDepthArray = g_pSimpleShaderEffect->GetTechniqueHandle( "PreTransformed_DownsampleDepthArray" );

    // get parameter handles
    g_hWVP = g_pSimpleShaderEffect->GetParameterHandle( "world_view_proj_matrix" );
    g_hConstantColor = g_pSimpleShaderEffect->GetParameterHandle( "simpleshader_constant_color" );
    g_hSampler = g_pSimpleShaderEffect->GetParameterHandle( "simpleshader_sampler" );
}

VOID SimpleShaders::Terminate()
{
    if( g_pSimpleShaderEffect != NULL )
    {
        g_pSimpleShaderEffect->Release();
        g_pSimpleShaderEffect = NULL;
    }

    delete g_pVertexDeclPos;
    delete g_pVertexDeclPosColor;
    delete g_pVertexDeclPosT;
    delete g_pVertexDeclPosTexCoord;
    delete g_pVertexDeclPosColorTexCoord;
}


VOID SimpleShaders::BindToNewDevice( ::D3DDevice* pNewDevice )
{
    g_pSimpleShadersDevice = pNewDevice;
    if( g_pSimpleShaderEffect != NULL )
        g_pSimpleShaderEffect->ChangeDevice( g_pSimpleShadersDevice );
}


VOID SimpleShaders::SetDeclPos()
{
    g_pSimpleShadersDevice->SetVertexDeclaration( g_pVertexDeclPos );
}

VOID SimpleShaders::SetDeclPosColor()
{
    g_pSimpleShadersDevice->SetVertexDeclaration( g_pVertexDeclPosColor );
}

VOID SimpleShaders::SetDeclPosT()
{
    g_pSimpleShadersDevice->SetVertexDeclaration( g_pVertexDeclPosT );
}

VOID SimpleShaders::SetDeclPosTex()
{
    g_pSimpleShadersDevice->SetVertexDeclaration( g_pVertexDeclPosTexCoord );
}

VOID SimpleShaders::SetDeclPosColorTex()
{
    g_pSimpleShadersDevice->SetVertexDeclaration( g_pVertexDeclPosColorTexCoord );
}

inline VOID BeginMaterial( FXLHANDLE hTechnique )
{
    g_pSimpleShaderEffect->BeginTechnique( hTechnique, 0 );
    g_pSimpleShaderEffect->BeginPassFromIndex( 0 );
    g_pSimpleShaderEffect->Commit();
}

VOID SimpleShaders::BeginShader_Transformed_DepthOnly( const XMMATRIX& matWVP )
{
    g_pSimpleShaderEffect->SetMatrixF4x4( g_hWVP, ( FLOAT* )&matWVP );

    BeginMaterial( g_hTechniqueTransformedDepthOnly );
}

VOID SimpleShaders::BeginShader_Transformed_ConstantColor( const XMMATRIX& matWVP, D3DCOLOR Color )
{
    g_pSimpleShaderEffect->SetMatrixF4x4( g_hWVP, ( FLOAT* )&matWVP );
    g_ConstantColorData = D3DXCOLOR( Color );
    g_pSimpleShaderEffect->SetVectorF( g_hConstantColor, ( FLOAT* )&g_ConstantColorData );

    BeginMaterial( g_hTechniqueTransformedConstantColor );
}

VOID SimpleShaders::BeginShader_Transformed_VertexColor( const XMMATRIX& matWVP )
{
    g_pSimpleShaderEffect->SetMatrixF4x4( g_hWVP, ( FLOAT* )&matWVP );
    BeginMaterial( g_hTechniqueTransformedVertexColor );
}

VOID SimpleShaders::BeginShader_Transformed_Textured( const XMMATRIX& matWVP, const D3DBaseTexture* pTexture )
{
    g_pSimpleShaderEffect->SetMatrixF4x4( g_hWVP, ( FLOAT* )&matWVP );
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    BeginMaterial( g_hTechniqueTransformedTextured );
}

VOID SimpleShaders::BeginShader_Transformed_TexturedConstantColor( const XMMATRIX& matWVP,
                                                                   const D3DBaseTexture* pTexture, D3DCOLOR Color )
{
    g_pSimpleShaderEffect->SetMatrixF4x4( g_hWVP, ( FLOAT* )&matWVP );
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    g_ConstantColorData = D3DXCOLOR( Color );
    g_pSimpleShaderEffect->SetVectorF( g_hConstantColor, ( FLOAT* )&g_ConstantColorData );
    BeginMaterial( g_hTechniqueTransformedTexturedConstantColor );
}

VOID SimpleShaders::BeginShader_Transformed_TexturedVertexColor( const XMMATRIX& matWVP,
                                                                 const D3DBaseTexture* pTexture )
{
    g_pSimpleShaderEffect->SetMatrixF4x4( g_hWVP, ( FLOAT* )&matWVP );
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    BeginMaterial( g_hTechniqueTransformedTexturedVertexColor );
}

VOID SimpleShaders::BeginShader_PreTransformed_ConstantColor( D3DCOLOR Color )
{
    g_ConstantColorData = D3DXCOLOR( Color );
    g_pSimpleShaderEffect->SetVectorF( g_hConstantColor, ( FLOAT* )&g_ConstantColorData );

    BeginMaterial( g_hTechniquePreTransformedConstantColor );
}

VOID SimpleShaders::BeginShader_PreTransformed_VertexColor()
{
    BeginMaterial( g_hTechniquePreTransformedVertexColor );
}

VOID SimpleShaders::BeginShader_PreTransformed_Textured( const D3DBaseTexture* pTexture )
{
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    BeginMaterial( g_hTechniquePreTransformedTextured );
}

VOID SimpleShaders::BeginShader_PreTransformed_DepthTextured( const D3DBaseTexture* pTexture )
{
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    BeginMaterial( g_hTechniquePreTransformedDepthTextured );
}

VOID SimpleShaders::BeginShader_PreTransformed_TexturedConstantColor( const D3DBaseTexture* pTexture, D3DCOLOR Color )
{
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    g_ConstantColorData = D3DXCOLOR( Color );
    g_pSimpleShaderEffect->SetVectorF( g_hConstantColor, ( FLOAT* )&g_ConstantColorData );
    BeginMaterial( g_hTechniquePreTransformedTexturedConstantColor );
}

VOID SimpleShaders::BeginShader_PreTransformed_TexturedVertexColor( const D3DBaseTexture* pTexture )
{
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pTexture );
    BeginMaterial( g_hTechniquePreTransformedTexturedVertexColor );
}

VOID SimpleShaders::BeginShader_PreTransformed_DepthOnly()
{
    BeginMaterial( g_hTechniquePreTransformedDepthOnly );
}

VOID SimpleShaders::BeginShader_PreTransformed_DownsampleDepth( const D3DBaseTexture* pDepthTexture )
{
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pDepthTexture );
    BeginMaterial( g_hTechniquePreTransformedDownsampleDepth );
}

VOID SimpleShaders::BeginShader_PreTransformed_DownsampleDepthArray( const D3DBaseTexture* pDepthArrayTexture,
                                                                     DWORD dwSliceIndex )
{
    const DWORD dwMaxSlices = pDepthArrayTexture->Format.Size.Stack.Depth + 1;
    FLOAT fSliceCoord = ( FLOAT )( 2 * dwSliceIndex + 1 ) / ( FLOAT )( dwMaxSlices * 2 );
    XMFLOAT4 vZCoordinate( fSliceCoord, fSliceCoord, fSliceCoord, fSliceCoord );
    g_pSimpleShaderEffect->SetVectorF( g_hConstantColor, ( FLOAT* )&vZCoordinate );
    g_pSimpleShaderEffect->SetSampler( g_hSampler, pDepthArrayTexture );
    BeginMaterial( g_hTechniquePreTransformedDownsampleDepthArray );
}

VOID SimpleShaders::EndShader()
{
    g_pSimpleShaderEffect->EndPass();
    g_pSimpleShaderEffect->EndTechnique();
}

} // namespace ATG
