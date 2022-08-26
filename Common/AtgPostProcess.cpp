//--------------------------------------------------------------------------------------
// AtgPostProcess.cpp
//
// Commonly used post-processing effects (like bloom, blur, etc.)
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgPostProcess.h"
#include "AtgUtil.h"


namespace ATG
{

//--------------------------------------------------------------------------------------
// Mappings to shader constants that are used in the HLSL shaders
//--------------------------------------------------------------------------------------
const DWORD PSCONST_fMiddleGray = 5;
const DWORD PSCONST_fElapsedTime = 7;

const DWORD PSCONST_avSampleOffsets = 0;
const DWORD PSCONST_avSampleWeights = 16;

//--------------------------------------------------------------------------------------
// Constants and external variables
//--------------------------------------------------------------------------------------

// When drawing a full-screen quad, trial-n-error shows that using a grid can be better
// due to the GPU's rasterization rules and minimizing texture cache misses
static const DWORD g_dwQuadGridSizeX = 8; // 1280 / 8 = 160
static const DWORD g_dwQuadGridSizeY = 1; //  720 / 1 = 720
static const DWORD g_dwNumQuadsInGrid = g_dwQuadGridSizeX * g_dwQuadGridSizeY;

const DWORD MAX_SAMPLES = 16;      // Maximum number of texture grabs

// A stack of render targets used for convenient pushing/popping of render targets
std::stack <LPDIRECT3DSURFACE9> g_pRenderTargetStack;

// A stack of render states used for convenient pushing/popping of render state
std::stack <RENDERSTATE> g_dwRenderStateStack;


//--------------------------------------------------------------------------------------
// Name: GaussianDistribution()
// Desc: Computes a two-parameter (x,y) Gaussian distrubution using the given
//       standard deviation (rho)
//--------------------------------------------------------------------------------------
inline FLOAT GaussianDistribution( FLOAT x, FLOAT y, FLOAT rho )
{
    return expf( -( x * x + y * y ) / ( 2 * rho * rho ) ) / sqrtf( 2 * XM_PI * rho * rho );
}


//--------------------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale4x4()
// Desc: Get the texcoord offsets to be used inside the DownScale4x4 pixel shader.
//--------------------------------------------------------------------------------------
VOID PostProcess::GetSampleOffsets_DownScale4x4( DWORD dwTexWidth, DWORD dwTexHeight,
                                                 XMVECTOR* pvSampleOffsets )
{
    FLOAT tu = 1.0f / ( FLOAT )dwTexWidth;
    FLOAT tv = 1.0f / ( FLOAT )dwTexHeight;

    // Sample from the 16 surrounding points. Since the center point will be in the
    // exact center of 16 texels, a 1.5f offset is needed to specify a texel center.
    for( int y = 0; y < 4; y++ )
    {
        for( int x = 0; x < 4; x++ )
        {
            pvSampleOffsets->x = ( ( FLOAT )x - 1.5f ) * tu;
            pvSampleOffsets->y = ( ( FLOAT )y - 1.5f ) * tv;
            pvSampleOffsets++;
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale3x3()
// Desc: Get the texcoord offsets to be used inside the DownScale3x3 pixel shader.
//--------------------------------------------------------------------------------------
VOID PostProcess::GetSampleOffsets_DownScale3x3( DWORD dwTexWidth, DWORD dwTexHeight,
                                                 XMVECTOR* pvSampleOffsets )
{
    FLOAT tu = 1.0f / ( FLOAT )dwTexWidth;
    FLOAT tv = 1.0f / ( FLOAT )dwTexHeight;

    // Sample from the 9 surrounding points. Since the center point will be in the exact
    // center of 4 texels, a 1.0f offset is needed to specify a texel center.
    for( int x = 0; x < 3; x++ )
    {
        for( int y = 0; y < 3; y++ )
        {
            pvSampleOffsets->x = ( x - 1.0f ) * tu;
            pvSampleOffsets->y = ( y - 1.0f ) * tv;
            pvSampleOffsets++;
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale2x2()
// Desc: Get the texcoord offsets to be used inside the DownScale2x2 pixel shader.
//--------------------------------------------------------------------------------------
VOID PostProcess::GetSampleOffsets_DownScale2x2( DWORD dwTexWidth, DWORD dwTexHeight,
                                                 XMVECTOR* pvSampleOffsets )
{
    FLOAT tu = 1.0f / ( FLOAT )dwTexWidth;
    FLOAT tv = 1.0f / ( FLOAT )dwTexHeight;

    // Sample from the 4 surrounding points. Since the center point will be in the exact
    // center of 4 texels, a 0.5f offset is needed to specify a texel center.
    for( int y = 0; y < 2; y++ )
    {
        for( int x = 0; x < 2; x++ )
        {
            pvSampleOffsets->x = ( ( FLOAT )x - 0.5f ) * tu;
            pvSampleOffsets->y = ( ( FLOAT )y - 0.5f ) * tv;
            pvSampleOffsets++;
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: GetSampleOffsets_GaussBlur5x5()
// Desc: Get the texcoord offsets to be used inside the GaussBlur5x5 pixel shader.
//--------------------------------------------------------------------------------------
VOID PostProcess::GetSampleOffsets_GaussBlur5x5( DWORD dwTexWidth, DWORD dwTexHeight,
                                                 XMVECTOR* pvTexCoordOffsets,
                                                 XMVECTOR* pvSampleWeights,
                                                 FLOAT fMultiplier )
{
    FLOAT tu = 1.0f / ( FLOAT )dwTexWidth;
    FLOAT tv = 1.0f / ( FLOAT )dwTexHeight;

    XMVECTOR vWhite = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );

    FLOAT fTotalWeight = 0.0f;
    DWORD index = 0;
    for( int x = -2; x <= 2; x++ )
    {
        for( int y = -2; y <= 2; y++ )
        {
            // Exclude pixels with a block distance greater than 2. This will
            // create a kernel which approximates a 5x5 kernel using only 13
            // sample points instead of 25; this is necessary since 2.0 shaders
            // only support 16 texture grabs.
            if( fabs( ( FLOAT )x ) + fabs( ( FLOAT )y ) > 2.0f )
                continue;

            // Get the unscaled Gaussian intensity for this offset
            pvTexCoordOffsets[index].x = ( FLOAT )x * tu;
            pvTexCoordOffsets[index].y = ( FLOAT )y * tv;
            pvTexCoordOffsets[index].z = 0.0f;
            pvTexCoordOffsets[index].w = 0.0f;

            pvSampleWeights[index] = vWhite * GaussianDistribution( ( FLOAT )x, ( FLOAT )y, 1.0f );

            fTotalWeight += pvSampleWeights[index].x;

            index++;
        }
    }

    // Divide the current weight by the total weight of all the samples; Gaussian
    // blur kernels add to 1.0f to ensure that the intensity of the image isn't
    // changed when the blur occurs. An optional multiplier variable is used to
    // add or remove image intensity during the blur.
    for( DWORD i = 0; i < index; i++ )
    {
        pvSampleWeights[i] /= fTotalWeight;
        pvSampleWeights[i] *= fMultiplier;
    }
}


//--------------------------------------------------------------------------------------
// Name: GetSampleOffsets_Bloom()
// Desc: Get the texcoord offsets to be used inside the Bloom pixel shader.
//--------------------------------------------------------------------------------------
VOID PostProcess::GetSampleOffsets_Bloom( DWORD dwTextureWidth, DWORD dwTextureHeight,
                                          FLOAT fAngle, XMVECTOR* pvTexCoordOffsets,
                                          XMVECTOR* pvColorWeights, FLOAT fDeviation,
                                          FLOAT fMultiplier )
{
    FLOAT tu = cosf( fAngle ) / ( FLOAT )dwTextureWidth;
    FLOAT tv = sinf( fAngle ) / ( FLOAT )dwTextureHeight;

    // Fill the center texel
    FLOAT fWeight = fMultiplier * GaussianDistribution( 0, 0, fDeviation );
    pvColorWeights[0] = XMVectorSet( fWeight, fWeight, fWeight, 1.0f );
    pvTexCoordOffsets[0] = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );

    // Fill the first half
    for( DWORD i = 1; i < 8; i++ )
    {
        // Get the Gaussian intensity for this offset
        fWeight = fMultiplier * GaussianDistribution( ( FLOAT )i, 0, fDeviation );
        pvColorWeights[i] = XMVectorSet( fWeight, fWeight, fWeight, 1.0f );
        pvTexCoordOffsets[i] = XMVectorSet( i * tu, i * tv, 0.0f, 0.0f );
    }

    // Mirror to the second half
    for( DWORD i = 8; i < 15; i++ )
    {
        pvColorWeights[i] = pvColorWeights[i - 7];
        pvTexCoordOffsets[i] = -pvTexCoordOffsets[i - 7];
    }
}


//--------------------------------------------------------------------------------------
// Name: GetSampleOffsets_Star()
// Desc: Get the texcoord offsets to be used inside the Star pixel shader.
//--------------------------------------------------------------------------------------
VOID PostProcess::GetSampleOffsets_Star( DWORD dwTexSize, XMVECTOR* pvTexCoordOffsets,
                                         XMVECTOR* pvColorWeights, FLOAT fDeviation )
{
    FLOAT tu = 1.0f / ( FLOAT )dwTexSize;

    // Fill the center texel
    FLOAT fWeight = 1.0f * GaussianDistribution( 0, 0, fDeviation );
    pvColorWeights[0] = XMVectorSet( fWeight, fWeight, fWeight, 1.0f );
    pvTexCoordOffsets[0] = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );

    // Fill the first half
    for( DWORD i = 1; i < 8; i++ )
    {
        // Get the Gaussian intensity for this offset
        fWeight = 1.0f * GaussianDistribution( ( FLOAT )i, 0, fDeviation );
        pvColorWeights[i] = XMVectorSet( fWeight, fWeight, fWeight, 1.0f );
        pvTexCoordOffsets[i] = XMVectorSet( i * tu, 0.0f, 0.0f, 0.0f );
    }

    // Mirror to the second half
    for( DWORD i = 8; i < 15; i++ )
    {
        pvColorWeights[i] = pvColorWeights[i - 7];
        pvTexCoordOffsets[i] = -pvTexCoordOffsets[i - 7];
    }
}


//-----------------------------------------------------------------------------
// Name: ClearTexture()
// Desc: Helper function for RestoreDeviceObjects to clear a texture surface
//-----------------------------------------------------------------------------
VOID PostProcess::ClearTexture( LPDIRECT3DTEXTURE9 pTexture, DWORD dwClearColor )
{
    // Make sure that the required shaders and objects exist
    assert( pTexture );

    DWORD OldFormat = pTexture->Format.DataFormat;
    if( pTexture->Format.DataFormat == GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16 )
    {
        pTexture->Format.DataFormat = GPUTEXTUREFORMAT_8_8_8_8;
    }

    // Create and set a render target for the texture
    PushRenderTarget( 0, CreateRenderTarget( pTexture ) );

    g_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, dwClearColor, 1.0f, 0L );

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pTexture, NULL,
                           0, 0, NULL, 0.0f, 0, NULL );

    // Cleanup and exit
    PopRenderTarget( 0 )->Release();

    pTexture->Format.DataFormat = OldFormat;
}


//--------------------------------------------------------------------------------------
// Name: DrawScreenSpaceQuad()
// Desc: Draw a viewport-aligned quad using screen-space coordinates
//--------------------------------------------------------------------------------------
VOID PostProcess::DrawScreenSpaceQuad( FLOAT w, FLOAT h, FLOAT fMaxU, FLOAT fMaxV )
{
    // Make sure that the required shaders and objects exist
    assert( m_pQuadVtxDecl );
    assert( m_pScreenSpaceVS );

    // Define vertices for the screen-space rect
    XMFLOAT4 v[3];
    v[0] = XMFLOAT4( 0, 0, 0.0f, 0.0f );
    v[1] = XMFLOAT4( w, 0, fMaxU, 0.0f );
    v[2] = XMFLOAT4( 0, h, 0.0f, fMaxV );

    // Set states for drawing the quad
    PushRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    PushRenderState( D3DRS_ZENABLE, FALSE );
    PushRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    PushRenderState( D3DRS_HALFPIXELOFFSET, TRUE );

    // Draw the quad
    g_pd3dDevice->SetVertexDeclaration( m_pQuadVtxDecl );
    g_pd3dDevice->SetVertexShader( m_pScreenSpaceVS );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_RECTLIST, 1, v, sizeof( v[0] ) );

    PopRenderStates();
}


//--------------------------------------------------------------------------------------
// Name: DrawFullScreenQuad()
// Desc: Draw a viewport-aligned quad covering the entire render target
//--------------------------------------------------------------------------------------
VOID PostProcess::DrawFullScreenQuad()
{
    // Make sure that the required shaders and objects exist
    assert( m_pQuadVtxDecl );
    assert( m_pQuadVB );
    assert( m_pScreenSpaceVS );

    // Set states for drawing the rect
    PushRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    PushRenderState( D3DRS_ZENABLE, FALSE );
    PushRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    PushRenderState( D3DRS_HALFPIXELOFFSET, TRUE );

    // Draw the rect
    g_pd3dDevice->SetVertexDeclaration( m_pQuadVtxDecl );
    g_pd3dDevice->SetVertexShader( m_pScreenSpaceVS );
    g_pd3dDevice->SetStreamSource( 0, m_pQuadVB, 0, sizeof( XMFLOAT4 ) );
    g_pd3dDevice->DrawPrimitive( D3DPT_RECTLIST, 0, g_dwNumQuadsInGrid );

    PopRenderStates();
}


//--------------------------------------------------------------------------------------
// Name: BrightPassFilterTexture()
// Desc: Run the bright-pass filter on m_pScaledSceneTexture and place the result
//       in m_pBrightPassTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::BrightPassFilterTexture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                           LPDIRECT3DTEXTURE9 pAdaptedLuminanceTexture,
                                           FLOAT fMiddleGrayKeyValue,
                                           LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pBrightPassFilterPS );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    // Get the offsets to be used within the GaussBlur5x5 pixel shader
    XMVECTOR avSampleOffsets[MAX_SAMPLES];
    XMVECTOR avSampleWeights[MAX_SAMPLES];
    GetSampleOffsets_GaussBlur5x5( SrcDesc.Width, SrcDesc.Height, avSampleOffsets, avSampleWeights );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleWeights, ( FLOAT* )avSampleWeights, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_fMiddleGray, &fMiddleGrayKeyValue, 1 );
    g_pd3dDevice->SetPixelShader( m_pBrightPassFilterPS );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetTexture( 1, pAdaptedLuminanceTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_POINT );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: SampleLuminance()
// Desc: Measure the average log luminance in the scene.
//--------------------------------------------------------------------------------------
VOID PostProcess::SampleLuminance( LPDIRECT3DTEXTURE9 pSrcTexture, BOOL bInitial,
                                   LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pSampleLumInitialPS );
    assert( m_pSampleLumFinalPS );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    // Sample initial luminance
    if( bInitial )
    {
        // Initialize the sample offsets for the initial luminance pass.
        XMVECTOR avSampleOffsets[MAX_SAMPLES];
        GetSampleOffsets_DownScale3x3( SrcDesc.Width, SrcDesc.Height, avSampleOffsets );
        g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
        g_pd3dDevice->SetPixelShader( m_pSampleLumInitialPS );

        g_pd3dDevice->SetTexture( 0, pSrcTexture );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    }
    else // ( bIntial == FALSE )
    {
        // Perform the final pass of the average luminance calculation. This pass
        // performs an exp() operation to return a single texel cooresponding to the
        // average luminance of the scene in m_pToneMapTexture.

        XMVECTOR avSampleOffsets[MAX_SAMPLES];
        GetSampleOffsets_DownScale4x4( SrcDesc.Width, SrcDesc.Height, avSampleOffsets );
        g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
        g_pd3dDevice->SetPixelShader( m_pSampleLumFinalPS );

        g_pd3dDevice->SetTexture( 0, pSrcTexture );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    }

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: AdaptLuminance()
// Desc: Adapt the luminance over time and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::AdaptLuminance( LPDIRECT3DTEXTURE9 pAdaptedLuminanceTexture,
                                  LPDIRECT3DTEXTURE9 pToneMapTexture,
                                  FLOAT fElapsedTime,
                                  LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pCalculateAdaptedLumPS );
    assert( pAdaptedLuminanceTexture && pToneMapTexture && pDstTexture );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    // This simulates the light adaptation that occurs when moving from a dark area to
    // a bright area, or vice versa. The m_pTexAdaptedLuminance texture stores a single
    // texel cooresponding to the user's adapted level.
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_fElapsedTime, &fElapsedTime, 1 );
    g_pd3dDevice->SetPixelShader( m_pCalculateAdaptedLumPS );

    g_pd3dDevice->SetTexture( 0, pAdaptedLuminanceTexture );
    g_pd3dDevice->SetTexture( 1, pToneMapTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: GaussBlur5x5Texture()
// Desc: Perform a 5x5 gaussian blur on pSrcTexture and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::GaussBlur5x5Texture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                       LPDIRECT3DTEXTURE9 pDstTexture,
                                       DWORD dwEdramOffset )
{
    // Make sure that the required shaders and objects exist
    assert( m_pGaussBlur5x5PS );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    D3DSURFACE_PARAMETERS surfaceParams =
    {
        0
    };
    surfaceParams.Base = dwEdramOffset;
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture, &surfaceParams ) );

    XMVECTOR avSampleOffsets[MAX_SAMPLES];
    XMVECTOR avSampleWeights[MAX_SAMPLES];
    GetSampleOffsets_GaussBlur5x5( SrcDesc.Width, SrcDesc.Height, avSampleOffsets, avSampleWeights );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleWeights, ( FLOAT* )avSampleWeights, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShader( m_pGaussBlur5x5PS );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: Downsample4x4Texture()
// Desc: Scale down pSrcTexture by 1/4 x 1/4 and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::Downsample4x4Texture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                        LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pDownScale4x4PS );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    // Get the sample offsets used within the pixel shader
    XMVECTOR avSampleOffsets[MAX_SAMPLES];
    GetSampleOffsets_DownScale4x4( SrcDesc.Width, SrcDesc.Height, avSampleOffsets );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShader( m_pDownScale4x4PS );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Draw a fullscreen quad
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: CopyTexture()
// Desc: Copy the src texture to the dst texture. The scale can (and should) be changed.
//--------------------------------------------------------------------------------------
VOID PostProcess::CopyTexture( LPDIRECT3DTEXTURE9 pSrcTexture,
                               LPDIRECT3DTEXTURE9 pDstTexture,
                               LPDIRECT3DPIXELSHADER9 pPixelShader,
                               DWORD dwEdramOffset )
{
    if( NULL == pPixelShader )
        pPixelShader = m_pCopyTexturePS;

    // Make sure that the required shaders and objects exist
    assert( pPixelShader );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    D3DSURFACE_PARAMETERS surfaceParams =
    {
        0
    };
    surfaceParams.Base = dwEdramOffset;
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture, &surfaceParams ) );

    // Scale and copy the src texture
    g_pd3dDevice->SetPixelShader( pPixelShader );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    XGTEXTURE_DESC DstDesc;
    XGGetTextureDesc( pDstTexture, 0, &DstDesc );
    DWORD ColorExpBias = 0;

    if( DstDesc.Format == D3DFMT_G16R16_SIGNED_INTEGER )            ColorExpBias = ( DWORD )
            D3DRESOLVE_EXPONENTBIAS( 10 );
    else if( DstDesc.Format == D3DFMT_A16B16G16R16_SIGNED_INTEGER ) ColorExpBias = ( DWORD )
            D3DRESOLVE_EXPONENTBIAS( 10 );

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0 | ColorExpBias, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: BuildMipMaps()
// Desc: Generate mip maps from the base texture
//--------------------------------------------------------------------------------------
VOID PostProcess::BuildMipMaps( LPDIRECT3DTEXTURE9 pTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pCopyTexturePS );
    assert( pTexture );

    DWORD dwNumMipLevels = pTexture->GetLevelCount();

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pTexture ) );

    // Scale and copy the src texture
    g_pd3dDevice->SetPixelShader( m_pCopyTexturePS );

    g_pd3dDevice->SetTexture( 0, pTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    D3DVIEWPORT9 vp;
    g_pd3dDevice->GetViewport( &vp );

    for( DWORD i = 1; i < dwNumMipLevels; i++ )
    {
        XGTEXTURE_DESC Desc;
        XGGetTextureDesc( pTexture, i, &Desc );
        vp.Width = Desc.Width;
        vp.Height = Desc.Height;
        g_pd3dDevice->SetViewport( &vp );
        g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINMIPLEVEL, i - 1 );

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad();

        DWORD ColorExpBias = 0;

        if( Desc.Format == D3DFMT_G16R16_SIGNED_INTEGER )            ColorExpBias = (DWORD) D3DRESOLVE_EXPONENTBIAS(10);
        else if( Desc.Format == D3DFMT_A16B16G16R16_SIGNED_INTEGER ) ColorExpBias = (DWORD) D3DRESOLVE_EXPONENTBIAS(10);

        g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0 | ColorExpBias, NULL, pTexture, NULL,
                               i, 0, NULL, 1.0f, 0L, NULL );
    }

    // Cleanup and return
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINMIPLEVEL, 13 );
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: Downsample2x2Texture()
// Desc: Scale down pSrcTexture by 1/2 x 1/2 and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::Downsample2x2Texture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                        LPDIRECT3DTEXTURE9 pDstTexture,
                                        DWORD dwEdramOffset )
{
    // Make sure that the required shaders and objects exist
    assert( m_pDownScale2x2PS );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    D3DSURFACE_PARAMETERS surfaceParams = { 0 };
    surfaceParams.Base = dwEdramOffset;
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture, &surfaceParams ) );

    XMVECTOR avSampleOffsets[MAX_SAMPLES];
    GetSampleOffsets_DownScale2x2( SrcDesc.Width, SrcDesc.Height, avSampleOffsets );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );

    // Create an exact 1/2 x 1/2 copy of the source texture
    g_pd3dDevice->SetPixelShader( m_pDownScale2x2PS );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // TODO: This should use border addressing with a black border!
    //m_pStarSourceTexture->Format.ClampX      = GPUCLAMP_CLAMP_TO_BORDER;
    //m_pStarSourceTexture->Format.ClampY      = GPUCLAMP_CLAMP_TO_BORDER;
    //m_pStarSourceTexture->Format.BorderColor = GPUBORDERCOLOR_ABGR_BLACK;
    //g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER );
    //g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: BloomTexture()
// Desc: Bloom the pSrcTexture and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::BloomTexture( LPDIRECT3DTEXTURE9 pSrcTexture,
                                BOOL bBloomAcrossWidth,
                                LPDIRECT3DTEXTURE9 pDstTexture,
                                FLOAT fSize, FLOAT fBrightness )
{
    // Make sure that the required shaders and objects exist
    assert( m_pBloomPS );
    assert( pSrcTexture && pDstTexture );

    XGTEXTURE_DESC SrcDesc;
    XGGetTextureDesc( pSrcTexture, 0, &SrcDesc );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    XMVECTOR avSampleOffsets[MAX_SAMPLES];
    XMVECTOR avSampleWeights[MAX_SAMPLES];

    if( bBloomAcrossWidth )
        GetSampleOffsets_Bloom( SrcDesc.Width, SrcDesc.Height, 0.0f * XM_PIDIV2, avSampleOffsets, avSampleWeights,
                                fSize, fBrightness );
    else
        GetSampleOffsets_Bloom( SrcDesc.Width, SrcDesc.Height, 1.0f * XM_PIDIV2, avSampleOffsets, avSampleWeights,
                                fSize, fBrightness );

    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleWeights, ( FLOAT* )avSampleWeights, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShader( m_pBloomPS );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: RenderStarLine()
// Desc: Merge the ppSrcTextures and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::RenderStarLine( LPDIRECT3DTEXTURE9 pSrcTexture, DWORD dwNumSamples,
                                  FLOAT fAttenuation, FLOAT fAttnPowScale,
                                  XMVECTOR* colors, DWORD pass,
                                  FLOAT fStepU, FLOAT fStepV,
                                  LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pStarPS );
    assert( pSrcTexture && pDstTexture );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    XMVECTOR avSampleOffsets[MAX_SAMPLES];
    XMVECTOR avSampleWeights[MAX_SAMPLES];

    // Sampling configration for each stage
    for( DWORD i = 0; i < dwNumSamples; i++ )
    {
        FLOAT lum = powf( fAttenuation, fAttnPowScale * i );

        avSampleWeights[i] = colors[i] * lum * ( pass + 1.0f ) * 0.5f;

        // Offset of sampling coordinate
        avSampleOffsets[i].x = fStepU * i;
        avSampleOffsets[i].y = fStepV * i;
        if( fabs( avSampleOffsets[i].x ) >= 0.9f || fabs( avSampleOffsets[i].y ) >= 0.9f )
        {
            avSampleOffsets[i].x = 0.0f;
            avSampleOffsets[i].y = 0.0f;
            avSampleWeights[i] *= 0.0f;
        }
    }

    g_pd3dDevice->SetPixelShader( m_pStarPS );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleOffsets, ( FLOAT* )avSampleOffsets, MAX_SAMPLES );
    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleWeights, ( FLOAT* )avSampleWeights, MAX_SAMPLES );

    g_pd3dDevice->SetTexture( 0, pSrcTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: MergeTextures()
// Desc: Merge the ppSrcTextures and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::MergeTextures( LPDIRECT3DTEXTURE9* ppSrcTextures,
                                 DWORD dwNumSrcTextures,
                                 LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pMergeTexturesPS[dwNumSrcTextures] );
    assert( ppSrcTextures && pDstTexture );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    XMVECTOR avSampleWeights[MAX_SAMPLES];
    const XMVECTOR vWhite = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );
    for( DWORD i = 0; i < dwNumSrcTextures; i++ )
    {
        g_pd3dDevice->SetTexture( i, ppSrcTextures[i] );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

        avSampleWeights[i] = vWhite * 1.0f / ( FLOAT )dwNumSrcTextures;
    }

    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleWeights, ( FLOAT* )avSampleWeights, dwNumSrcTextures );
    g_pd3dDevice->SetPixelShader( m_pMergeTexturesPS[dwNumSrcTextures] );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: AddTextures()
// Desc: Add the ppSrcTextures and place the result in pDstTexture
//--------------------------------------------------------------------------------------
VOID PostProcess::AddTextures( LPDIRECT3DTEXTURE9* ppSrcTextures,
                               DWORD dwNumSrcTextures,
                               LPDIRECT3DTEXTURE9 pDstTexture )
{
    // Make sure that the required shaders and objects exist
    assert( m_pMergeTexturesPS[dwNumSrcTextures] );
    assert( ppSrcTextures && pDstTexture );

    // Create and set a render target
    PushRenderTarget( 0L, CreateRenderTarget( pDstTexture ) );

    XMVECTOR avSampleWeights[MAX_SAMPLES];
    const XMVECTOR vWhite = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );
    for( DWORD i = 0; i < dwNumSrcTextures; i++ )
    {
        g_pd3dDevice->SetTexture( i, ppSrcTextures[i] );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        g_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );

        avSampleWeights[i] = vWhite;
    }

    g_pd3dDevice->SetPixelShaderConstantF( PSCONST_avSampleWeights, ( FLOAT* )avSampleWeights, dwNumSrcTextures );
    g_pd3dDevice->SetPixelShader( m_pMergeTexturesPS[dwNumSrcTextures] );

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad();

    g_pd3dDevice->Resolve( D3DRESOLVE_RENDERTARGET0, NULL, pDstTexture, NULL,
                           0, 0, NULL, 1.0f, 0L, NULL );

    // Cleanup and return
    PopRenderTarget( 0L )->Release();
    g_pd3dDevice->SetPixelShader( NULL );
}


//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initialize the effects library
//--------------------------------------------------------------------------------------
HRESULT PostProcess::Initialize()
{
    // Create a vertex buffer for screen-space effects
    g_pd3dDevice->CreateVertexBuffer( 3 * g_dwNumQuadsInGrid * sizeof( XMFLOAT4 ),
                                      0L, 0L, D3DPOOL_DEFAULT, &m_pQuadVB, NULL );
    XMFLOAT4* v;
    m_pQuadVB->Lock( 0, 0, ( VOID** )&v, 0 );

    FLOAT fGridDimX = 2.0f / ( FLOAT )g_dwQuadGridSizeX;
    FLOAT fGridDimY = 2.0f / ( FLOAT )g_dwQuadGridSizeY;
    FLOAT fGridDimU = 1.0f / ( FLOAT )g_dwQuadGridSizeX;
    FLOAT fGridDimV = 1.0f / ( FLOAT )g_dwQuadGridSizeY;
    FLOAT T = +1.0f;
    FLOAT V0 = 0.0f;
    for( DWORD y = 0; y < g_dwQuadGridSizeY; y++ )
    {
        FLOAT L = -1.0f;
        FLOAT U0 = 0.0f;
        for( DWORD x = 0; x < g_dwQuadGridSizeX; x++ )
        {
            FLOAT R = L + fGridDimX;
            FLOAT B = T - fGridDimY;
            FLOAT U1 = U0 + fGridDimU;
            FLOAT V1 = V0 + fGridDimV;

            *v++ = XMFLOAT4( L, T, U0, V0 ); // x, y, tu, tv
            *v++ = XMFLOAT4( R, T, U1, V0 ); // x, y, tu, tv
            *v++ = XMFLOAT4( L, B, U0, V1 ); // x, y, tu, tv
            L += fGridDimX;
            U0 += fGridDimU;
        }
        T -= fGridDimY;
        V0 += fGridDimV;
    }
    m_pQuadVB->Unlock();

    // Create common vertex declaration used by all the screen-space effects
    static const D3DVERTEXELEMENT9 decl[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };

    g_pd3dDevice->CreateVertexDeclaration( decl, &m_pQuadVtxDecl );

    // Create shaders. Note that it's okay if these do not load, as long as the app does
    // not later try to use them. An assert will fire otherwise. This is so that the app
    // only needs to supply shaders that it actually intends to use.
    ATG::LoadVertexShader( "game:\\Media\\Shaders\\ScreenSpaceShader.xvu", &m_pScreenSpaceVS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\CopyTexture.xpu", &m_pCopyTexturePS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\Bloom.xpu", &m_pBloomPS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\Star.xpu", &m_pStarPS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\SampleLumInitial.xpu", &m_pSampleLumInitialPS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\SampleLumFinal.xpu", &m_pSampleLumFinalPS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\CalculateAdaptedLum.xpu", &m_pCalculateAdaptedLumPS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\DownScale4x4.xpu", &m_pDownScale4x4PS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\DownScale2x2.xpu", &m_pDownScale2x2PS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\GaussBlur5x5.xpu", &m_pGaussBlur5x5PS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\BrightPassFilter.xpu", &m_pBrightPassFilterPS );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_1.xpu", &m_pMergeTexturesPS[1] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_2.xpu", &m_pMergeTexturesPS[2] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_3.xpu", &m_pMergeTexturesPS[3] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_4.xpu", &m_pMergeTexturesPS[4] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_5.xpu", &m_pMergeTexturesPS[5] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_6.xpu", &m_pMergeTexturesPS[6] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_7.xpu", &m_pMergeTexturesPS[7] );
    ATG::LoadPixelShader( "game:\\Media\\Shaders\\MergeTextures_8.xpu", &m_pMergeTexturesPS[8] );

    return S_OK;
}

}
;
