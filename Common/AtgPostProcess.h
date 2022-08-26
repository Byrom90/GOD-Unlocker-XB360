//--------------------------------------------------------------------------------------
// AtgPostProcess.h
//
// Commonly used post-processing effects (like bloom, blur, etc.)
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGPOSTPROCESS_H
#define ATGPOSTPROCESS_H

#include <xtl.h>
#include <xboxmath.h>
#include <stack>
#include "AtgDevice.h"

namespace ATG
{

// External access to the global D3D device
extern D3DDevice* g_pd3dDevice;

// Special formats to keep full range of 16-bit fixed point render targets.  
// Textures using these formats should ideally use exp biases as follows:
//  1) -15 on sampling (Format.ExpAdjust in texture header)
//  2) +5 on write to render target (pParameters->ColorExpBias in CreateRenderTarget)
//  3) +10 on resolve (D3DRESOLVE_EXPONENTBIAS Flags in Resolve)
// With this setup you can losslessly traverse a full loop from EDRAM to texture to EDRAM,
// without making any shader changes.
static const D3DFORMAT D3DFMT_G16R16_SIGNED_INTEGER = ( D3DFORMAT )MAKED3DFMT2(
    GPUTEXTUREFORMAT_16_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_SIGNED,
    GPUSIGN_SIGNED, GPUSIGN_SIGNED, GPUSIGN_SIGNED, GPUNUMFORMAT_INTEGER,
    GPUSWIZZLE_X, GPUSWIZZLE_Y, GPUSWIZZLE_0, GPUSWIZZLE_0 );
static const D3DFORMAT D3DFMT_A16B16G16R16_SIGNED_INTEGER = ( D3DFORMAT )MAKED3DFMT2(
    GPUTEXTUREFORMAT_16_16_16_16, GPUENDIAN_8IN16, TRUE, GPUSIGN_SIGNED,
    GPUSIGN_SIGNED, GPUSIGN_SIGNED, GPUSIGN_SIGNED, GPUNUMFORMAT_INTEGER,
    GPUSWIZZLE_X, GPUSWIZZLE_Y, GPUSWIZZLE_Z, GPUSWIZZLE_W );

//-----------------------------------------------------------------------------
// Name: CreateRenderTarget()
// Desc: Helper function to set a render target for a texture
//-----------------------------------------------------------------------------
inline LPDIRECT3DSURFACE9 CreateRenderTarget( const LPDIRECT3DTEXTURE9 pTexture,
                                              D3DSURFACE_PARAMETERS* pSurfaceParams = NULL )
{
    XGTEXTURE_DESC Desc;
    XGGetTextureDesc( pTexture, 0, &Desc );

    D3DSURFACE_PARAMETERS SurfaceParams = {0};
    if( NULL == pSurfaceParams )
    {
        pSurfaceParams = &SurfaceParams;
    }

    if( Desc.Format == D3DFMT_G16R16_SIGNED_INTEGER )               pSurfaceParams->ColorExpBias += 5;
    else if( Desc.Format == D3DFMT_A16B16G16R16_SIGNED_INTEGER )    pSurfaceParams->ColorExpBias += 5;

    // Some texture formats can't be used as render target formats.  Special
    // case the ones we use
    if( Desc.Format == D3DFMT_R5G6B5 )                         Desc.Format = D3DFMT_X8R8G8B8;
    else if( Desc.Format == D3DFMT_G16R16 )                         Desc.Format = D3DFMT_G16R16_EDRAM;
    else if( Desc.Format == D3DFMT_G16R16_SIGNED_INTEGER )          Desc.Format = D3DFMT_G16R16_EDRAM;
    else if( Desc.Format == D3DFMT_A16B16G16R16 )                   Desc.Format = D3DFMT_A16B16G16R16_EDRAM;
    else if( Desc.Format == D3DFMT_A16B16G16R16_SIGNED_INTEGER )    Desc.Format = D3DFMT_A16B16G16R16_EDRAM;
    else if( Desc.Format == D3DFMT_R16F_EXPAND )                    Desc.Format = D3DFMT_R32F;
    else if( Desc.Format == D3DFMT_R16F )                           Desc.Format = D3DFMT_R32F;
    else if( Desc.Format == D3DFMT_G16R16F_EXPAND )                 Desc.Format = D3DFMT_G16R16F;
    else if( Desc.Format == D3DFMT_A16B16G16R16F_EXPAND )           Desc.Format = D3DFMT_A16B16G16R16F;

    LPDIRECT3DSURFACE9 pRenderTarget;
    g_pd3dDevice->CreateRenderTarget( Desc.Width, Desc.Height, Desc.Format,
                                      D3DMULTISAMPLE_NONE, 0, FALSE,
                                      &pRenderTarget, pSurfaceParams );
    return pRenderTarget;
}


//-----------------------------------------------------------------------------
// Name: PushRenderTarget()/PopRenderTarget()
// Desc: Helper functions for setting/restoring render targets
//-----------------------------------------------------------------------------
extern std::stack <LPDIRECT3DSURFACE9> g_pRenderTargetStack;


// Pushes a render target onto a stack of saved states
inline VOID PushRenderTarget( DWORD dwRenderTargetID,
                              const LPDIRECT3DSURFACE9 pNewRenderTarget = NULL )
{
    LPDIRECT3DSURFACE9 pOldRenderTarget;
    g_pd3dDevice->GetRenderTarget( dwRenderTargetID, &pOldRenderTarget );
    if( pNewRenderTarget )
        g_pd3dDevice->SetRenderTarget( dwRenderTargetID, pNewRenderTarget );

    g_pRenderTargetStack.push( pOldRenderTarget );
    g_pRenderTargetStack.push( pNewRenderTarget );
}


// Pops a saved render target from a stack of saved states
inline LPDIRECT3DSURFACE9 PopRenderTarget( DWORD dwRenderTargetID )
{
    LPDIRECT3DSURFACE9 pNewRenderTarget = g_pRenderTargetStack.top(); g_pRenderTargetStack.pop();
    LPDIRECT3DSURFACE9 pOldRenderTarget = g_pRenderTargetStack.top(); g_pRenderTargetStack.pop();

    g_pd3dDevice->SetRenderTarget( dwRenderTargetID, pOldRenderTarget );
    if( pOldRenderTarget )
        pOldRenderTarget->Release();

    return pNewRenderTarget;
}


//-----------------------------------------------------------------------------
// Name: PushRenderState()/PopRenderState()
// Desc: Helper functions for setting/restoring render targets
//-----------------------------------------------------------------------------
struct RENDERSTATE
{
    D3DRENDERSTATETYPE dwRenderState;
    DWORD dwValue;
};

extern std::stack <RENDERSTATE> g_dwRenderStateStack;

// Pushes a render state onto a stack of saved states
inline VOID PushRenderState( DWORD dwRenderState, DWORD dwValue )
{
    DWORD dwOriginalValue;
    g_pd3dDevice->GetRenderState( ( D3DRENDERSTATETYPE )dwRenderState, &dwOriginalValue );

    RENDERSTATE rs;
    rs.dwRenderState = ( D3DRENDERSTATETYPE )dwRenderState;
    rs.dwValue = dwOriginalValue;
    g_dwRenderStateStack.push( rs );

    g_pd3dDevice->SetRenderState( ( D3DRENDERSTATETYPE )dwRenderState, dwValue );
}


// Pops a saved render state from a stack of saved states
inline VOID PopRenderStates()
{
    for( int i = 0; !g_dwRenderStateStack.empty(); ++i, g_dwRenderStateStack.pop() )
    {
        RENDERSTATE rs = g_dwRenderStateStack.top();

        g_pd3dDevice->SetRenderState( rs.dwRenderState, rs.dwValue );
    }
}


//--------------------------------------------------------------------------------------
// Name: class PostProcess
// Desc: Library of commonly used post-processing effects
//--------------------------------------------------------------------------------------
class PostProcess
{
    LPDIRECT3DVERTEXBUFFER9 m_pQuadVB;
    LPDIRECT3DVERTEXDECLARATION9 m_pQuadVtxDecl;
    LPDIRECT3DVERTEXSHADER9 m_pScreenSpaceVS;
    LPDIRECT3DPIXELSHADER9 m_pCopyTexturePS;
    LPDIRECT3DPIXELSHADER9 m_pBloomPS;
    LPDIRECT3DPIXELSHADER9 m_pStarPS;
    LPDIRECT3DPIXELSHADER9 m_pSampleLumInitialPS;
    LPDIRECT3DPIXELSHADER9 m_pSampleLumFinalPS;
    LPDIRECT3DPIXELSHADER9 m_pCalculateAdaptedLumPS;
    LPDIRECT3DPIXELSHADER9 m_pDownScale4x4PS;
    LPDIRECT3DPIXELSHADER9 m_pDownScale2x2PS;
    LPDIRECT3DPIXELSHADER9 m_pGaussBlur5x5PS;
    LPDIRECT3DPIXELSHADER9 m_pBrightPassFilterPS;
    LPDIRECT3DPIXELSHADER9  m_pMergeTexturesPS[9];

    // Sample offset calculation. These offsets are passed to corresponding pixel shaders.
    VOID    GetSampleOffsets_GaussBlur5x5( DWORD dwTextureWidth, DWORD dwTextureHeight,
                                           OUT XMVECTOR* pvTexCoordOffsets,
                                           OUT XMVECTOR* vSampleWeights,
                                           OPTIONAL FLOAT fMultiplier=1.0f );
    VOID    GetSampleOffsets_Bloom( DWORD dwTextureWidth, DWORD dwTextureHeight,
                                    FLOAT fAngle, OUT XMVECTOR* pvTexCoordOffsets,
                                    OUT XMVECTOR* pvColorWeights, FLOAT fDeviation,
                                    FLOAT OPTIONAL fMultiplier=1.0f );
    VOID    GetSampleOffsets_Star( DWORD dwTextureSize,
                                   OUT XMVECTOR* pvTexCoordOffsets,
                                   OUT XMVECTOR* pvColorWeights, FLOAT fDeviation );
    VOID    GetSampleOffsets_DownScale4x4( DWORD dwWidth, DWORD dwHeight,
                                           OUT XMVECTOR* pvSampleOffsets );
    VOID    GetSampleOffsets_DownScale3x3( DWORD dwWidth, DWORD dwHeight,
                                           OUT XMVECTOR* pvSampleOffsets );
    VOID    GetSampleOffsets_DownScale2x2( DWORD dwWidth, DWORD dwHeight,
                                           OUT XMVECTOR* pvSampleOffsets );

public:
    HRESULT Initialize();

    // Post-processing source textures creation
    VOID    ClearTexture( LPDIRECT3DTEXTURE9 pTexture,
                          OPTIONAL DWORD dwClearColor = 0x00000000 );
    VOID    CopyTexture( LPDIRECT3DTEXTURE9 pSrcTexture,
                         LPDIRECT3DTEXTURE9 pDstTexture,
                         LPDIRECT3DPIXELSHADER9 pPixelShader = NULL,
                         OPTIONAL DWORD         dwEdramOffset = 0 );
    VOID    BuildMipMaps( LPDIRECT3DTEXTURE9 pTexture );
    VOID    Downsample2x2Texture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                  LPDIRECT3DTEXTURE9 pDstTexture,
                                  OPTIONAL DWORD     dwEdramOffset = 0 );
    VOID    Downsample4x4Texture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                  LPDIRECT3DTEXTURE9 pDstTexture );
    VOID    GaussBlur5x5Texture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                 LPDIRECT3DTEXTURE9 pDstTexture,
                                 OPTIONAL DWORD     dwEdramOffset = 0 );
    VOID    BrightPassFilterTexture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                     LPDIRECT3DTEXTURE9 pAdaptedLuminanceTexture, 
                                     FLOAT              fMiddleGrayKeyValue,
                                     LPDIRECT3DTEXTURE9 pDstTexture );
    VOID    AddTextures( LPDIRECT3DTEXTURE9* ppSrcTextures,
                         DWORD dwNumSrcTextures,
                         LPDIRECT3DTEXTURE9 pDstTexture );
    VOID    MergeTextures( LPDIRECT3DTEXTURE9* ppSrcTextures,
                           DWORD dwNumSrcTextures,
                           LPDIRECT3DTEXTURE9 pDstTexture );
    VOID    AdaptLuminance( LPDIRECT3DTEXTURE9 pAdaptedLuminanceTexture, 
                            LPDIRECT3DTEXTURE9 pToneMapTexture, 
                            FLOAT fElapsedTime,
                            LPDIRECT3DTEXTURE9 pDstTexture );
    VOID    SampleLuminance( LPDIRECT3DTEXTURE9 pSrcTexture, BOOL bInitial,
                             LPDIRECT3DTEXTURE9 pDstTexture );
    VOID    BloomTexture( LPDIRECT3DTEXTURE9 pSrcTexture, BOOL bBloomAcrossWidth,
                          LPDIRECT3DTEXTURE9 pDstTexture,
                          FLOAT fSize = 3.0f, FLOAT fBrightness = 2.0f );
    VOID    RenderStarLine( LPDIRECT3DTEXTURE9 pSrcTexture, DWORD dwNumSamples,
                            FLOAT fAttenuation, FLOAT fAttnPowScale,
                            XMVECTOR* colors, DWORD pass,
                            FLOAT fStepU, FLOAT fStepV, 
                            LPDIRECT3DTEXTURE9 pDstTexture );

    VOID    DrawScreenSpaceQuad( FLOAT fWidth, FLOAT fHeight,
                                 FLOAT fMaxU=1.0f, FLOAT fMaxV=1.0f );
    VOID    DrawFullScreenQuad();
};

}
// namespace ATG

#endif // ATGPOSTPROCESS_H
