//--------------------------------------------------------------------------------------
// AtgNuiVisualization.cpp
//
// Visualization tools for Kinect
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgDebugDraw.h"
#include "AtgNuiVisualization.h"
#include "AtgUtil.h"


const FLOAT EPSILON = 5.96e-08f;

#define MIN( v1, v2) ( v1 ) < ( v2 ) ? ( v1 ) : ( v2 )
#define MAX( v1, v2) ( v1 ) > ( v2 ) ? ( v1 ) : ( v2 )
#define CLAMP( value, min, max ) MIN( max, MAX( value, min ) )

namespace ATG
{

BOOL ClipLineToRect( XMFLOAT2* origin, XMFLOAT2* end, FLOAT fLeft, FLOAT fTop, FLOAT fWidth, FLOAT fHeight )
{
    FLOAT fBottom = fTop + fHeight;
    FLOAT fRight  = fLeft + fWidth;

    if( origin->x > fLeft && origin->x < fRight &&
        origin->y > fTop && origin->y < fBottom &&
        end->x > fLeft && end->x < fRight       &&
        end->y > fTop && end->y < fBottom          )
    {
        return TRUE;
    }

    if( origin->x < fLeft && end->x < fLeft     ||
        origin->x > fRight && end->x > fRight      )
    {
        return FALSE;
    }

    if( origin->y < fTop && end->y < fTop       ||
        origin->y > fBottom && end->y > fBottom    )
    {
        return FALSE;
    }

    if( XMScalarNearEqual( origin->x, end->x, EPSILON ) )
    {
        origin->y = CLAMP( origin->y, fTop, fBottom );
        end->y = CLAMP( end->y, fTop, fBottom );
    }
    else if( XMScalarNearEqual( origin->y, end->y, EPSILON ) )
    {
        origin->x = CLAMP( origin->x, fLeft, fRight );
        end->x = CLAMP( end->x, fLeft, fRight );
    }
    else
    {
        FLOAT fSlope = ( end->y - origin->y ) / ( end->x - origin->x );
        FLOAT fYIntercept = ( origin->y - origin->x * fSlope );

        if( origin->x < fLeft )
        {
            origin->x = fLeft;
            origin->y = fSlope * origin->x + fYIntercept;
        }
        else if( origin->x > fRight )
        {
            origin->x = fRight;
            origin->y = fSlope * origin->x + fYIntercept;
        }

        if( end->x < fLeft )
        {
            end->x = fLeft;
            end->y = fSlope * end->x + fYIntercept;
        }
        else if( end->x > fRight )
        {
            end->x = fRight;
            end->y = fSlope * end->x + fYIntercept;
        }

        if( origin->y < fTop )
        {
            origin->y = fTop;
            origin->x = ( origin->y - fYIntercept ) / fSlope;
        }
        else if( origin->y > fBottom )
        {
            origin->y = fBottom;
            origin->x = ( origin->y - fYIntercept ) / fSlope;
        }

        if( end->y < fTop )
        {
            end->y = fTop;
            end->x = ( end->y - fYIntercept ) / fSlope;
        }
        else if( end->y > fBottom )
        {
            end->y = fBottom;
            end->x = ( end->y - fYIntercept ) / fSlope;
        }
    }

    if( origin->x > fLeft - 0.5f && origin->x < fRight + 0.5f &&
        origin->y > fTop - 0.5f && origin->y < fBottom + 0.5f &&
        end->x > fLeft - 0.5f && end->x < fRight + 0.5f       &&
        end->y > fTop - 0.5f && end->y < fBottom + 0.5f          )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// By default, use green when a bone is tracked and red when it is inferred.
#define BONE_CONFIDENCE_TRACKED_COLOR   0xff00ff00
#define BONE_CONFIDENCE_INFERRED_COLOR  0xffff0000


// Define the bones in the skeleton using joint indices
const NUI_VISUALIZATION_BONE_JOINTS g_SkeletonBoneList[] =
{
    // Head
    { NUI_SKELETON_POSITION_HEAD,            NUI_SKELETON_POSITION_SHOULDER_CENTER },  // Top of head to top of neck

    // Right arm
    { NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT },   // Neck bottom to right shoulder internal
    { NUI_SKELETON_POSITION_SHOULDER_RIGHT,  NUI_SKELETON_POSITION_ELBOW_RIGHT },      // Right shoulder internal to right elbow
    { NUI_SKELETON_POSITION_ELBOW_RIGHT,     NUI_SKELETON_POSITION_WRIST_RIGHT },      // Right elbow to right wrist
    { NUI_SKELETON_POSITION_WRIST_RIGHT,     NUI_SKELETON_POSITION_HAND_RIGHT },       // Right wrist to right hand

    // Left arm
    { NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT },    // Neck bottom to left shoulder internal
    { NUI_SKELETON_POSITION_SHOULDER_LEFT,   NUI_SKELETON_POSITION_ELBOW_LEFT },       // Left shoulder internal to left elbow
    { NUI_SKELETON_POSITION_ELBOW_LEFT,      NUI_SKELETON_POSITION_WRIST_LEFT },       // Left elbow to left wrist
    { NUI_SKELETON_POSITION_WRIST_LEFT,      NUI_SKELETON_POSITION_HAND_LEFT },        // Left wrist to left hand

    // Right leg and foot
    { NUI_SKELETON_POSITION_HIP_RIGHT,       NUI_SKELETON_POSITION_KNEE_RIGHT  },      // Right hip internal to right knee
    { NUI_SKELETON_POSITION_KNEE_RIGHT,      NUI_SKELETON_POSITION_ANKLE_RIGHT },      // Right knee to right ankle
    { NUI_SKELETON_POSITION_ANKLE_RIGHT,     NUI_SKELETON_POSITION_FOOT_RIGHT },       // Right ankle to right foot

    // Left leg and foot
    { NUI_SKELETON_POSITION_HIP_LEFT,        NUI_SKELETON_POSITION_KNEE_LEFT  },       // Left hip internal to left knee
    { NUI_SKELETON_POSITION_KNEE_LEFT,       NUI_SKELETON_POSITION_ANKLE_LEFT },       // Left knee to left ankle
    { NUI_SKELETON_POSITION_ANKLE_LEFT,      NUI_SKELETON_POSITION_FOOT_LEFT },        // Left ankle to left foot

    // Spine
    { NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SPINE },            // Neck bottom to spine
    { NUI_SKELETON_POSITION_SPINE,           NUI_SKELETON_POSITION_HIP_CENTER },       // Spine to hip center

    // Hips
    { NUI_SKELETON_POSITION_HIP_RIGHT,       NUI_SKELETON_POSITION_HIP_CENTER },       // Right hip to hip center
    { NUI_SKELETON_POSITION_HIP_CENTER,      NUI_SKELETON_POSITION_HIP_LEFT }          // Hip center to left hip
};

const UINT g_uSkeletonBoneCount = ARRAYSIZE( g_SkeletonBoneList );


//-------------------------------------------------------------------------------------
// Video and pixel shader definitions
//-------------------------------------------------------------------------------------
static const char*  g_strVideoShaderHLSL =
    " struct VS_OUT                                                              "
    " {                                                                          "
    "     float4 Position : POSITION;                                            "
    "     float2 TexCoord : TEXCOORD0;                                           "
    " };                                                                         "
    "                                                                            "
    " sampler VideoTexture : register(s0);                                       "
    "                                                                            "
    " VS_OUT VideoVertexShader( const float3 Position : POSITION,                "
    "                           const float2 TexCoord : TEXCOORD0 )              "
    " {                                                                          "
    "     VS_OUT Output;                                                         "
    "     Output.Position.x  = ( Position.x-0.5);                                "
    "     Output.Position.y  = ( Position.y-0.5);                                "
    "     Output.Position.z  = ( 0.0 );                                          "
    "     Output.Position.w  = ( 1.0 );                                          "
    "     Output.TexCoord = TexCoord;                                            "
    "     return Output;                                                         "
    " }                                                                          "
    "                                                                            "
    " float4 VideoPixelShader( VS_OUT Input ) : COLOR                            "
    " {                                                                          "
    "     return tex2D( VideoTexture, Input.TexCoord );                          "
    " }                                                                          ";

struct VideoFeedVertex
{
    FLOAT vPosition[ 3 ];
    FLOAT vTexCoords[ 2 ];
};

struct DepthPreviewVertex
{
    FLOAT x, y;
    FLOAT u, v;
};

//--------------------------------------------------------------------------------------
// Name: NUI_VISUALIZATION_SKELETON_RENDER_INFO::SplatTrackedColor
// Desc: Initializes a NUI_VISUALIZATION_SKELETON_RENDER_INFO structure.
//--------------------------------------------------------------------------------------
VOID NUI_VISUALIZATION_SKELETON_RENDER_INFO::Initialize( D3DCOLOR trackedColor, D3DCOLOR inferredColor, BOOL draw )
{ 
    for( UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; ++ i )
    {
        bDraw[ i ] = draw;
        TrackedColor[ i ] = trackedColor; 
        InferredColor[ i ] = inferredColor;
    }
}


//--------------------------------------------------------------------------------------
// Name: NUI_VISUALIZATION_SKELETON_RENDER_INFO::SplatTrackedColor
// Desc: Set the color used to draw a tracked skeleton joint to the same value for the 
//       all the joints of the specified skeleton.
//--------------------------------------------------------------------------------------
VOID NUI_VISUALIZATION_SKELETON_RENDER_INFO::SplatTrackedColor( D3DCOLOR trackedColor) 
{ 
    for( UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; ++ i )
    {
        TrackedColor[ i ] = trackedColor; 
    }
}


//--------------------------------------------------------------------------------------
// Name: NUI_VISUALIZATION_SKELETON_RENDER_INFO::SplatInferredColor
// Desc: Set the color used to draw a inferred skeleton joint to the same value for the 
//       all the joints of the specified skeleton.
//--------------------------------------------------------------------------------------
VOID NUI_VISUALIZATION_SKELETON_RENDER_INFO::SplatInferredColor( D3DCOLOR inferredColor )
{ 
    for( UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; ++ i ) 
    {
        InferredColor[ i ] = inferredColor;
    }
}


//--------------------------------------------------------------------------------------
// Name: NuiVisualization
// Desc: Constructs a NuiVisualization object.
//--------------------------------------------------------------------------------------
NuiVisualization::NuiVisualization()
:m_bIsInitialized( FALSE ),
 m_pSmoothDepthTexture( NULL ),
 m_pSmoothDepthTextureSwap( NULL ),
 m_pNuiDepthTexture( NULL ),
 m_pDepthPreviewPS( NULL ),
 m_pDepthPreviewSmoothingVS( NULL ),
 m_bColorIsNew( FALSE ),
 m_colorDisplaying( 0 ),
 m_depthDisplaying( 0 ),
 m_colorImageResolution( NUI_IMAGE_RESOLUTION_640x480 ),
 m_dwColorStreamWidth( 640 ),
 m_dwColorStreamHeight( 480 ),
 m_dwNestedBeginCount( 0 )
{
    for( UINT i = 0 ; i < 2; ++ i )
    {
        m_pColorTexture[ i ] = NULL;
        m_pDepthTexture[ i ] = NULL;
    }

    XMemSet( &m_SkeletonFrame, 0, sizeof( NUI_SKELETON_FRAME ) );
    XMemSet( &m_colorViewArea, 0, sizeof( m_colorViewArea) );

    for( UINT i = 0 ; i < NUI_SKELETON_COUNT; ++ i )
        m_SkeletonRenderInfo[ i ].Initialize( BONE_CONFIDENCE_TRACKED_COLOR, BONE_CONFIDENCE_INFERRED_COLOR );
}

//--------------------------------------------------------------------------------------
// Name: ~NuiVisualization
// Desc: Destroys the object and releases all the buffers previously allocated.
//--------------------------------------------------------------------------------------
NuiVisualization::~NuiVisualization()
{
}


//--------------------------------------------------------------------------------------
// Name: Initialize
// Desc: Allocates and initializes the required buffers and system resources.
//       Use dwComponentsToProcess to specify which component NuiVisualization will 
//       process. Use flags a combination of the NUI_INITIALIZE_FLAGS flags.
//       Currently NUI_INITIALIZE_FLAG_USES_COLOR, NUI_INITIALIZE_FLAG_USES_DEPTH, 
//       NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX
//       and NUI_INITIALIZE_FLAG_USES_SKELETON flags are supported,
//       if NUI_INITIALIZE_FLAG_USES_COLOR is specified for dwComponentsToProcess then the 
//       colorImageResolution must a value from NUI_IMAGE_RESOLUTION. Otherwise, the 
//       value is ignored.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::Initialize( ::D3DDevice* pd3dDevice, DWORD dwComponentsToProcess, 
                                      NUI_IMAGE_RESOLUTION colorImageResolution,
                                      BOOL bRenderDashStyleDepthPreview )
{
    // Pre-conditions
    assert( !m_bIsInitialized );

    m_pd3dDevice = pd3dDevice;
    m_dwComponentsToProcess = dwComponentsToProcess;

    // Set the dimension of each stream since we will be using them during visualization
    if( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_COLOR )
    {
        m_colorImageResolution = colorImageResolution;
        switch( colorImageResolution )
        {
            case NUI_IMAGE_RESOLUTION_320x240:
                m_dwColorStreamWidth  = 320;
                m_dwColorStreamHeight = 240;
                break;

            case NUI_IMAGE_RESOLUTION_640x480:
                m_dwColorStreamWidth  = 640;
                m_dwColorStreamHeight = 480;
                break;

            default:
                assert( false ); //Unrecognized RESOLUTION
                m_dwColorStreamWidth  = 640;
                m_dwColorStreamHeight = 480;
                break;
        }
    }

    // Setup a ramp lookup table to display depth values as colors
    InitializeDepthColorTable();

    // Compile vertex shader.
    ID3DXBuffer* pVertexShaderCode;
    ID3DXBuffer* pVertexErrorMsg;
    HRESULT hr = D3DXCompileShader( g_strVideoShaderHLSL,
                                   ( UINT )strlen( g_strVideoShaderHLSL ),
                                   NULL,
                                   NULL,
                                   "VideoVertexShader",
                                   "vs_2_0",
                                   0,
                                   &pVertexShaderCode,
                                   &pVertexErrorMsg,
                                   NULL );
    if( FAILED( hr ) )
    {
        return E_FAIL;
    }

    // Create vertex shader.
    pd3dDevice->CreateVertexShader( ( DWORD* )pVertexShaderCode->GetBufferPointer(),
                                    &m_pVideoVertexShader );

    // Compile pixel shader.
    ID3DXBuffer* pPixelShaderCode;
    ID3DXBuffer* pPixelErrorMsg;

    hr = D3DXCompileShader( g_strVideoShaderHLSL,
                            ( UINT )strlen( g_strVideoShaderHLSL ),
                            NULL,
                            NULL,
                            "VideoPixelShader",
                            "ps_2_0",
                            0,
                            &pPixelShaderCode,
                            &pPixelErrorMsg,
                            NULL );
    if( FAILED( hr ) )
    {
        return E_FAIL;
    }

    // Create pixel shader.
    pd3dDevice->CreatePixelShader( ( DWORD* )pPixelShaderCode->GetBufferPointer(),
                                   &m_pVideoPixelShaderRGB );

    // Define the vertex elements and
    // Create a vertex declaration from the element descriptions.
    static const D3DVERTEXELEMENT9 VertexElements[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    pd3dDevice->CreateVertexDeclaration( VertexElements, &m_pVideoVertexDecl );

    for( UINT i = 0; i < 2; ++ i)
    {
        if( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_COLOR )
        {
            // Initialize color stream video texture
            if( FAILED( InitializeVideoTextures( pd3dDevice, &m_pColorTexture[ i ], m_dwColorStreamWidth, m_dwColorStreamHeight ) ) )
            {
                return E_FAIL;
            }
        }

        if( m_dwComponentsToProcess & ( NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX ) )
        {
            // Initialize depth stream video texture
            if( FAILED( InitializeVideoTextures( pd3dDevice, &m_pDepthTexture[ i ], s_dwDepthStreamWidth, s_dwDepthStreamHeight ) ) )
            {
                return E_FAIL;
            }
        }
    }

    // Are we going to be rendering the dash-style depth-preview?
    if( bRenderDashStyleDepthPreview )
    {
        // Create the vertex and pixel shaders.
        if( FAILED( LoadVertexShader( "game:\\Media\\Shaders\\DepthPreviewSmoothingVS.xvu", &m_pDepthPreviewSmoothingVS ) ) )
        {
            return E_FAIL;
        }

        if( FAILED( LoadPixelShader( "game:\\Media\\Shaders\\DepthPreviewPS.xpu", &m_pDepthPreviewPS ) ) )
        {
            return E_FAIL;
        }

        // Create the depth preview textures.
        if( FAILED( pd3dDevice->CreateTexture( s_dwDashStyleDepthPreviewWidth,
                                               s_dwDashStyleDepthPreviewHeight,
                                               1, 0,
                                               D3DFMT_LIN_D16,
                                               0, 
                                               &m_pSmoothDepthTexture, NULL ) ) )
        {
            return E_FAIL;
        }

        if( FAILED( pd3dDevice->CreateTexture( s_dwDashStyleDepthPreviewWidth,
                                               s_dwDashStyleDepthPreviewHeight,
                                               1, 0,
                                               D3DFMT_LIN_D16,
                                               0, 
                                               &m_pSmoothDepthTextureSwap, NULL ) ) )
        {
            return E_FAIL;
        }
    }

    m_bIsInitialized = TRUE;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Shutdown
// Desc: Releases the resources allocated in the Initialize method.
//       Any call to Initialize method should be macthed with a call to this 
//       Shutdown method.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::Shutdown()
{
    if( m_pVideoVertexDecl != NULL )
    {
        m_pVideoVertexDecl->Release();
        m_pVideoVertexDecl = NULL;
    }
    
    if( m_pVideoPixelShaderRGB != NULL )
    {
        m_pVideoPixelShaderRGB->Release();
        m_pVideoPixelShaderRGB = NULL;
    }

    if( m_pVideoVertexShader != NULL )
    {
        m_pVideoVertexShader->Release();
        m_pVideoVertexShader = NULL;
    }

    SAFE_RELEASE( m_pSmoothDepthTexture );
    SAFE_RELEASE( m_pSmoothDepthTextureSwap );
    SAFE_RELEASE( m_pDepthPreviewPS );
    SAFE_RELEASE( m_pDepthPreviewSmoothingVS );
    SAFE_RELEASE( m_pNuiDepthTexture );

    m_pSmoothDepthTexture = NULL;
    m_pSmoothDepthTextureSwap = NULL;
    m_pDepthPreviewPS = NULL;
    m_pDepthPreviewSmoothingVS = NULL;
    m_pNuiDepthTexture = NULL;
    
    m_bIsInitialized = FALSE;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: SetColorTexture
// Desc: Sets the color stream buffer to be displayed next time DisplayColorStream is 
//       called.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::SetColorTexture( IDirect3DTexture9* pColorTexture, const NUI_IMAGE_VIEW_AREA* pViewArea /* = NULL */ )
{
    // Pre-conditions
    assert( m_bIsInitialized );
    assert( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_COLOR );

    PIXBeginNamedEvent( 0, "VisualizeStreams - Fill ColorMap" );
    if( pColorTexture )
    {
        // Clear texture
        UINT textureID = m_colorDisplaying == 0 ? 1 : 0;
        D3DLOCKED_RECT Locked;
        if( FAILED( m_pColorTexture[ textureID ]->LockRect( 0, &Locked, NULL, 0 ) ) )
        {
            PIXEndNamedEvent();
            return E_FAIL;
        }

        D3DLOCKED_RECT LockedSrc;
        if( FAILED( pColorTexture->LockRect( 0, &LockedSrc, NULL, D3DLOCK_READONLY ) ) )
        {
            m_pColorTexture[ textureID ]->UnlockRect( 0 );
            PIXEndNamedEvent();
            return E_FAIL;
        }
        
        // Fill in the colormap data
        XMemCpyStreaming( Locked.pBits, LockedSrc.pBits, Locked.Pitch * m_dwColorStreamHeight );

        pColorTexture->UnlockRect( 0 );		
        m_pColorTexture[ textureID ]->UnlockRect( 0 );

        m_bColorIsNew = TRUE;
    }
    PIXEndNamedEvent();
    
    if (pViewArea != NULL)
    {
        m_colorViewArea = *pViewArea;
    } else {
        XMemSet( &m_colorViewArea, 0, sizeof( m_colorViewArea) );
    }
    
    return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// Name: SetDepthTexture
// Desc: Sets the depth stream buffer to be displayed next time DisplayDepthStream is 
//       called.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::SetDepthTexture( IDirect3DTexture9* pDepthTexture, BOOL bColorize )
{
    // Pre-conditions
    assert( m_bIsInitialized );
    assert( m_dwComponentsToProcess & ( NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX ) );

    PIXBeginNamedEvent( 0, "VisualizeStreams - Fill DepthMap" );
    if( pDepthTexture )
    {
        // Make a copy of the depth texture, so we can leverage the real depth values.
        SAFE_RELEASE( m_pNuiDepthTexture );
        m_pNuiDepthTexture = pDepthTexture;
        m_pNuiDepthTexture->AddRef();

        UINT textureID = m_depthDisplaying == 0 ? 1 : 0;
        D3DLOCKED_RECT Locked;
        if( FAILED( m_pDepthTexture[ textureID ]->LockRect( 0, &Locked, NULL, 0 ) ) )
        {
            PIXEndNamedEvent();
            return E_FAIL;
        }

        D3DLOCKED_RECT LockedSrc;
        if( FAILED( pDepthTexture->LockRect( 0, &LockedSrc, NULL, D3DLOCK_READONLY ) ) )
        {
            pDepthTexture->UnlockRect( 0 );		
            PIXEndNamedEvent();
            return E_FAIL;
        }

        // Fill in the depthmap data
        DWORD* lpBits = ( DWORD* )Locked.pBits;
        USHORT* pDepthMapCur = ( USHORT* )LockedSrc.pBits;
        
        for( UINT y = 0; y < s_dwDepthStreamHeight; ++ y )
        {
            for( UINT x = 0; x < s_dwDepthStreamWidth; ++ x )
            {
                if( bColorize )
                {
                    // To colorize the depth values, normalize the depth value against a maximum depth
                    // value to be colorized and lookup into the color table
                    const static FLOAT fMaxDepth = 3500.0f;                           // use 3.5 meters as max depth to colorize
                    const static UINT uNormalize = (UINT) ceil( fMaxDepth / 511.0 );
                    UINT uIndex = min( (USHORT)( pDepthMapCur[ x ] >> 3 ) / uNormalize, 511 );
                
                    lpBits[x] = m_DepthColorTable[ uIndex ];
                }
                else
                {
                    const static FLOAT fMaxDepth = 3500.0f;
                    const static UINT uNormalize = (UINT) ceil( fMaxDepth / 254.0 );
                    UINT uValue = min( (USHORT)( pDepthMapCur[ x ] >> 3 ) / uNormalize, 254 );

                    lpBits[x] = D3DCOLOR_RGBA( uValue, uValue, uValue, 0xff ); 
                }
            }
            lpBits += Locked.Pitch / sizeof( DWORD );
            pDepthMapCur += LockedSrc.Pitch / sizeof( USHORT );
        }

        pDepthTexture->UnlockRect( 0 );		
        m_pDepthTexture[ textureID ]->UnlockRect( 0 );
        m_depthDisplaying = m_depthDisplaying == 0 ? 1 : 0;
    }
    PIXEndNamedEvent();

    return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// Name: SetSkeletons
// Desc: Sets the skeletons to be displayed next time DisplaySkeletons is called.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::SetSkeletons( const NUI_SKELETON_FRAME* pSkeletonFrame )
{
    // Pre-conditions
    assert( m_bIsInitialized );
    assert( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_SKELETON );

    XMemCpy( &m_SkeletonFrame, pSkeletonFrame, sizeof( NUI_SKELETON_FRAME ) );
    return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// Name: SetSkeletonRenderInfo
// Desc: Sets the rendering options for the specified skeleton.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::SetSkeletonRenderInfo( DWORD dwSkeletonIndex, const NUI_VISUALIZATION_SKELETON_RENDER_INFO* pSkeletonRenderInfo )
{ 
    assert( dwSkeletonIndex < NUI_SKELETON_COUNT ); 
    
    XMemCpy( &m_SkeletonRenderInfo[ dwSkeletonIndex ], pSkeletonRenderInfo, sizeof( NUI_VISUALIZATION_SKELETON_RENDER_INFO ) ); 

    return ERROR_SUCCESS; 
}


//--------------------------------------------------------------------------------------
// Name: GetSkeletonRenderInfo
// Desc: Retrieves the rendering options for the specifed skeleton.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::GetSkeletonRenderInfo( DWORD dwSkeletonIndex, NUI_VISUALIZATION_SKELETON_RENDER_INFO* pSkeletonRenderInfo ) 
{  
    assert( dwSkeletonIndex < NUI_SKELETON_COUNT );
    
    XMemCpy( pSkeletonRenderInfo, &m_SkeletonRenderInfo[ dwSkeletonIndex ], sizeof( NUI_VISUALIZATION_SKELETON_RENDER_INFO ) ); 
    
    return ERROR_SUCCESS; 
}


//--------------------------------------------------------------------------------------
// Name: BeginRender
// Desc: Set renderstates to draw as overlay
//--------------------------------------------------------------------------------------
VOID NuiVisualization::BeginRender()
{
    // Set state on the first call
    if( m_dwNestedBeginCount == 0 )
    {
        // Note, we are not saving the texture, vertex, or pixel shader,
        //       since it's not worth the performance. We're more interested
        //       in saving state that would cause hard to find problems.
        m_pd3dDevice->GetRenderState( D3DRS_ALPHABLENDENABLE,
                                      &m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHABLENDENABLE ] );
        m_pd3dDevice->GetRenderState( D3DRS_SRCBLEND, &m_dwSavedState[ SAVEDSTATE_D3DRS_SRCBLEND ] );
        m_pd3dDevice->GetRenderState( D3DRS_DESTBLEND, &m_dwSavedState[ SAVEDSTATE_D3DRS_DESTBLEND ] );
        m_pd3dDevice->GetRenderState( D3DRS_BLENDOP, &m_dwSavedState[ SAVEDSTATE_D3DRS_BLENDOP ] );
        m_pd3dDevice->GetRenderState( D3DRS_ALPHATESTENABLE, &m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHATESTENABLE ] );
        m_pd3dDevice->GetRenderState( D3DRS_ALPHAREF, &m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHAREF ] );
        m_pd3dDevice->GetRenderState( D3DRS_ALPHAFUNC, &m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHAFUNC ] );
        m_pd3dDevice->GetRenderState( D3DRS_FILLMODE, &m_dwSavedState[ SAVEDSTATE_D3DRS_FILLMODE ] );
        m_pd3dDevice->GetRenderState( D3DRS_CULLMODE, &m_dwSavedState[ SAVEDSTATE_D3DRS_CULLMODE ] );
        m_pd3dDevice->GetRenderState( D3DRS_ZENABLE, &m_dwSavedState[ SAVEDSTATE_D3DRS_ZENABLE ] );
        m_pd3dDevice->GetRenderState( D3DRS_STENCILENABLE, &m_dwSavedState[ SAVEDSTATE_D3DRS_STENCILENABLE ] );
        m_pd3dDevice->GetRenderState( D3DRS_VIEWPORTENABLE, &m_dwSavedState[ SAVEDSTATE_D3DRS_VIEWPORTENABLE ] );
        m_pd3dDevice->GetSamplerState( 0, D3DSAMP_MINFILTER, &m_dwSavedState[ SAVEDSTATE_D3DSAMP_MINFILTER ] );
        m_pd3dDevice->GetSamplerState( 0, D3DSAMP_MAGFILTER, &m_dwSavedState[ SAVEDSTATE_D3DSAMP_MAGFILTER ] );
        m_pd3dDevice->GetSamplerState( 0, D3DSAMP_ADDRESSU, &m_dwSavedState[ SAVEDSTATE_D3DSAMP_ADDRESSU ] );
        m_pd3dDevice->GetSamplerState( 0, D3DSAMP_ADDRESSV, &m_dwSavedState[ SAVEDSTATE_D3DSAMP_ADDRESSV ] );

        // Set up some render states
        m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
        m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
        m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
        m_pd3dDevice->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
        m_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
        m_pd3dDevice->SetRenderState( D3DRS_ALPHAREF, 0x08 );
        m_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
        m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
        m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
        m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
        m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, FALSE );
        m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    }

    // Keep track of the nested begin/end calls.
    ++ m_dwNestedBeginCount;
}


//--------------------------------------------------------------------------------------
// Name: EndRender
// Desc: Restore renderstates
//--------------------------------------------------------------------------------------
VOID NuiVisualization::EndRender()
{
    assert( m_dwNestedBeginCount > 0 );

    if( -- m_dwNestedBeginCount > 0 )
        return;

    m_pd3dDevice->SetTexture( 0, NULL );
    m_pd3dDevice->SetVertexDeclaration( NULL );
    m_pd3dDevice->SetVertexShader( NULL );
    m_pd3dDevice->SetPixelShader( NULL );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHABLENDENABLE ] );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, m_dwSavedState[ SAVEDSTATE_D3DRS_SRCBLEND ] );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, m_dwSavedState[ SAVEDSTATE_D3DRS_DESTBLEND ] );
    m_pd3dDevice->SetRenderState( D3DRS_BLENDOP, m_dwSavedState[ SAVEDSTATE_D3DRS_BLENDOP ] );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHATESTENABLE ] );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHAREF, m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHAREF ] );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHAFUNC, m_dwSavedState[ SAVEDSTATE_D3DRS_ALPHAFUNC ] );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, m_dwSavedState[ SAVEDSTATE_D3DRS_FILLMODE ] );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, m_dwSavedState[ SAVEDSTATE_D3DRS_CULLMODE ] );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_ZENABLE ] );
    m_pd3dDevice->SetRenderState( D3DRS_STENCILENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_STENCILENABLE ] );
    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_VIEWPORTENABLE ] );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, m_dwSavedState[ SAVEDSTATE_D3DSAMP_MINFILTER ] );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, m_dwSavedState[ SAVEDSTATE_D3DSAMP_MAGFILTER ] );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, m_dwSavedState[ SAVEDSTATE_D3DSAMP_ADDRESSU ] );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, m_dwSavedState[ SAVEDSTATE_D3DSAMP_ADDRESSV ] );
}


//--------------------------------------------------------------------------------------
// Name: RenderColorStream
// Desc: Displays the most recent color stream buffer.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::RenderColorStream( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight )
{
    // Pre-conditions
    assert( m_bIsInitialized );
    assert( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_COLOR );

    // Render video texture
    PIXBeginNamedEvent( 0, "VisualizeStreams - Render ColorMap" );
    
    // Set up render states
    BeginRender();

    m_pd3dDevice->SetPixelShader( m_pVideoPixelShaderRGB );

    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    m_pd3dDevice->SetVertexShader( m_pVideoVertexShader );
    m_pd3dDevice->SetVertexDeclaration( m_pVideoVertexDecl );

    if( m_bColorIsNew )
    {
        m_colorDisplaying = m_colorDisplaying == 0 ? 1 : 0;
        m_bColorIsNew = FALSE;
    }
    m_pd3dDevice->SetTexture( 0, m_pColorTexture[ m_colorDisplaying ] );

    // Fill in the VB for depth stream texture
    VideoFeedVertex g_SnapshotVertices[] =
    {
        //  x             y               z   u  v
        {   fX,           fY,             0,  0, 0 },
        {   fX + fWidth,  fY,             0,  1, 0 },
        {   fX,           fY + fHeight,   0,  0, 1 },
    };

    VideoFeedVertex* pVertices;

    m_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( *g_SnapshotVertices ), &(VOID*&)pVertices );
    memcpy( pVertices, g_SnapshotVertices, sizeof( g_SnapshotVertices ) );
    m_pd3dDevice->EndVertices();

    m_pd3dDevice->SetStreamSource( 0, NULL, 0, 0 );

    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    PIXEndNamedEvent();

    EndRender();
    return ERROR_SUCCESS;
}

HRESULT NuiVisualization::RenderCustomDepthStream( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, LPDIRECT3DTEXTURE9 pTexture )
{
    // Pre-conditions
    assert( m_bIsInitialized );
    assert( m_dwComponentsToProcess & ( NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX ) );

    PIXBeginNamedEvent( 0, "VisualizeStreams - Render DepthMap" );
    
    // Set up render states
    BeginRender();

    m_pd3dDevice->SetPixelShader( m_pVideoPixelShaderRGB );

    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    m_pd3dDevice->SetVertexShader( m_pVideoVertexShader );
    m_pd3dDevice->SetVertexDeclaration( m_pVideoVertexDecl );

    m_pd3dDevice->SetTexture( 0, pTexture );

    // Fill in the VB for depth stream texture
    VideoFeedVertex g_SnapshotVertices[] =
    {
        //  x             y               z   u  v
        {   fX,           fY,             0,  0, 0 },
        {   fX + fWidth,  fY,             0,  1, 0 },
        {   fX,           fY + fHeight,   0,  0, 1 },
    };

    VideoFeedVertex* pVertices;

    m_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( *g_SnapshotVertices ), &(VOID*&)pVertices );
    memcpy( pVertices, g_SnapshotVertices, sizeof( g_SnapshotVertices ) );
    m_pd3dDevice->EndVertices();

    m_pd3dDevice->SetStreamSource( 0, NULL, 0, 0 );

    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );
    PIXEndNamedEvent();

    EndRender();
    return ERROR_SUCCESS;
}

HRESULT NuiVisualization::RenderDashStyleDepthPreview( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight )
{
    // Early out if nothing to display.
    if( NULL == m_pNuiDepthTexture )
        return S_OK;

    PIXBeginNamedEvent( 0, "VisualizeStreams - Render Dash-Style Preview" );

    // Set state we need...(and cache current values for restore later).
    m_pd3dDevice->GetRenderState( D3DRS_VIEWPORTENABLE, &m_dwSavedState[ SAVEDSTATE_D3DRS_VIEWPORTENABLE ] );
    m_pd3dDevice->GetRenderState( D3DRS_ZENABLE, &m_dwSavedState[ SAVEDSTATE_D3DRS_ZENABLE ] );

    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );

    // Smooth the depth texture
    D3DTexture * pSrcTexture = m_pNuiDepthTexture;
    D3DTexture * pDstTexture = m_pSmoothDepthTexture;

    // Set the pixel shader here, although we don't use it until later.
    m_pd3dDevice->SetPixelShader( m_pDepthPreviewPS );

    // Do 3 passes to smooth the depth-map.
    DWORD dwIteration = 3;
    do
    {
        // Basically just memexport the averaged depth values, ping-ponging between textures.
        m_pd3dDevice->SetTexture( D3DVERTEXTEXTURESAMPLER0, pSrcTexture );
        m_pd3dDevice->SetSamplerFilterStates( D3DVERTEXTEXTURESAMPLER0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT, 1 );
        GPU_MEMEXPORT_STREAM_CONSTANT MemExportStreamConstant;
        GPU_SET_MEMEXPORT_STREAM_CONSTANT( &MemExportStreamConstant,
                                            ( USHORT* )( pDstTexture->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT ),
                                            s_dwDashStyleDepthPreviewPitch * s_dwDashStyleDepthPreviewHeight,
                                            SURFACESWAP_LOW_RED,
                                            GPUSURFACENUMBER_UINTEGER,
                                            GPUCOLORFORMAT_16,
                                            GPUENDIAN128_8IN16 );

        m_pd3dDevice->SetVertexShaderConstantF( 0, ( FLOAT* ) &MemExportStreamConstant, 1 );
        m_pd3dDevice->SetVertexShader( m_pDepthPreviewSmoothingVS );
        m_pd3dDevice->BeginExport( 0, pDstTexture, D3DBEGINEXPORT_VERTEXSHADER );
        m_pd3dDevice->DrawPrimitive( D3DPT_POINTLIST, 0, s_dwDashStyleDepthPreviewWidth * s_dwDashStyleDepthPreviewHeight );
        m_pd3dDevice->EndExport( 0, pDstTexture, 0 );

        // Swap (ping-pong) our textures until the final pass.
        if( --dwIteration )
        {
            pSrcTexture = pDstTexture;
            pDstTexture = ( pDstTexture == m_pSmoothDepthTexture ) ? m_pSmoothDepthTextureSwap : m_pSmoothDepthTexture;
        }
    } 
    while( dwIteration > 0 );

    // Unset the vertex shader texture.
    m_pd3dDevice->SetTexture( D3DVERTEXTEXTURESAMPLER0, NULL );

    // Now set up to actually render the smoothed depth map, with lights, etc.
    m_pd3dDevice->SetTexture( 0, pDstTexture );
    m_pd3dDevice->SetVertexDeclaration( m_pVideoVertexDecl );
    m_pd3dDevice->SetVertexShader( m_pVideoVertexShader );
    m_pd3dDevice->SetSamplerFilterStates( 0, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT, 1 );

    enum PLAYER_DATA_PARTS
    {
        PLAYER_HEAD = 0,
        PLAYER_NECK,
        PLAYER_SHOULDER,
        PLAYER_LEFT_HAND,
        PLAYER_RIGHT_HAND,
        PLAYER_PLANE,
    };

    // Initialize our tracking vectors.
    // Use tracked hands as light sources.
    XMVECTOR PlayerData[ NUI_SKELETON_COUNT ];
    XMVECTOR TrackedLeftHandPos[ NUI_SKELETON_COUNT ];
    XMVECTOR TrackedRightHandPos[ NUI_SKELETON_COUNT ];
    XMemSet( TrackedLeftHandPos, 0, sizeof( TrackedLeftHandPos ) );
    XMemSet( TrackedRightHandPos, 0, sizeof( TrackedRightHandPos ) );

    // Get all tracked hand positions
    INT nFirstTrackedPlayer = -1;
    for( UINT i = 0; i < NUI_SKELETON_COUNT; ++i )
    {
        if( m_SkeletonFrame.SkeletonData[ i ].eTrackingState == NUI_SKELETON_TRACKED )
        {
            if( nFirstTrackedPlayer == -1 )
                nFirstTrackedPlayer = i;

            if( m_SkeletonFrame.SkeletonData[ i ].eSkeletonPositionTrackingState[ NUI_SKELETON_POSITION_HAND_LEFT ] != NUI_SKELETON_POSITION_NOT_TRACKED )
            {
                TrackedLeftHandPos[ i ] = m_SkeletonFrame.SkeletonData[ i ].SkeletonPositions[ NUI_SKELETON_POSITION_HAND_LEFT ];
            }
            if( m_SkeletonFrame.SkeletonData[ i ].eSkeletonPositionTrackingState[ NUI_SKELETON_POSITION_HAND_RIGHT ] != NUI_SKELETON_POSITION_NOT_TRACKED )
            {
                TrackedRightHandPos[ i ] = m_SkeletonFrame.SkeletonData[ i ].SkeletonPositions[ NUI_SKELETON_POSITION_HAND_RIGHT ];
            }
        }
    }

    // If we find a tracked player, grab their hand positions.
    if( nFirstTrackedPlayer != -1 )
    {
        PlayerData[ PLAYER_HEAD ] = m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].SkeletonPositions[ NUI_SKELETON_POSITION_HEAD ];
        PlayerData[ PLAYER_NECK ] = m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].SkeletonPositions[ NUI_SKELETON_POSITION_SHOULDER_CENTER ];
        PlayerData[ PLAYER_SHOULDER ] = m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].SkeletonPositions[ NUI_SKELETON_POSITION_SHOULDER_RIGHT ];
        if( m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].eSkeletonPositionTrackingState[ NUI_SKELETON_POSITION_HAND_LEFT ] != NUI_SKELETON_POSITION_NOT_TRACKED )
        {
            PlayerData[ PLAYER_LEFT_HAND ] = m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].SkeletonPositions[ NUI_SKELETON_POSITION_HAND_LEFT ];
        }
        if( m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].eSkeletonPositionTrackingState[ NUI_SKELETON_POSITION_HAND_RIGHT ] != NUI_SKELETON_POSITION_NOT_TRACKED )
        {
            PlayerData[ PLAYER_RIGHT_HAND ] = m_SkeletonFrame.SkeletonData[ nFirstTrackedPlayer ].SkeletonPositions[ NUI_SKELETON_POSITION_HAND_RIGHT ];
        }

        XMVECTOR v1 = PlayerData[ PLAYER_HEAD ] - PlayerData[ PLAYER_NECK ];
        XMVECTOR v2 = PlayerData[ PLAYER_SHOULDER ] - PlayerData[ PLAYER_NECK ];
        if( XMVectorGetX( XMVector3Length( v1 ) ) == 0 || XMVectorGetX( XMVector3Length( v2 ) ) == 0 )
        {
            // If we don't get valid data to be able to calculate our player plane, we should probably just flag this player as not tracked.
            nFirstTrackedPlayer = -1;
        }
        else
        {
            XMVECTOR Normal = XMVector3Normalize( XMVector3Cross( v2, v1 ) );
            FLOAT d = XMVectorGetX( XMVector3Dot( Normal, PlayerData[ PLAYER_HEAD ] ) );
            PlayerData[ PLAYER_PLANE ] = XMVectorSetW( Normal, -d );
        }
    }

    // Set shader constants.
    m_pd3dDevice->SetPixelShaderConstantF( 1, ( FLOAT* ) PlayerData, 6 );

    BOOL Engaged = ( nFirstTrackedPlayer != -1 ) ? FALSE : TRUE;
    BOOL EngagedConst[ 4 ] = { Engaged, Engaged, Engaged, Engaged };
    m_pd3dDevice->SetPixelShaderConstantB( 0, EngagedConst, 1 );

    m_pd3dDevice->SetPixelShaderConstantF( 8, ( FLOAT* ) &TrackedLeftHandPos, 6 );
    m_pd3dDevice->SetPixelShaderConstantF( 14, ( FLOAT* ) &TrackedRightHandPos, 6 );
    XMVECTOR OpacityConst = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );
    m_pd3dDevice->SetPixelShaderConstantF( 20, ( FLOAT* )&OpacityConst, 1 );

    // Fill in the VB for depth stream texture
    VideoFeedVertex SnapshotVertices[] =
    {
        //  x             y               z   u  v
        {   fX,           fY,             0,  0, 0 },
        {   fX + fWidth,  fY,             0,  1, 0 },
        {   fX,           fY + fHeight,   0,  0, 1 },
    };

    VideoFeedVertex* pVertices;

    m_pd3dDevice->BeginVertices( D3DPT_RECTLIST, 3, sizeof( *SnapshotVertices ), &(VOID*&)pVertices );
    memcpy( pVertices, SnapshotVertices, sizeof( SnapshotVertices ) );
    m_pd3dDevice->EndVertices();

    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

    // Restore render states.
    m_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_VIEWPORTENABLE ] );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, m_dwSavedState[ SAVEDSTATE_D3DRS_ZENABLE ] );

    PIXEndNamedEvent();

    return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// Name: RenderDepthStream
// Desc: Displays the most recent depth stream buffer.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::RenderDepthStream( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight )
{
    return RenderCustomDepthStream( fX, fY, fWidth, fHeight, m_pDepthTexture[ m_depthDisplaying ] );
}


XMFLOAT2 g_ScreenSpaceJoints[ NUI_SKELETON_POSITION_COUNT ];


//--------------------------------------------------------------------------------------
// Name: RenderSkeletons
// Desc: Displays all tracked skeletons from the most recent skeleton frame buffer.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::RenderSkeletons( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip, BOOL bRegisterToColor )
{
    // Set up render states
    BeginRender();

    for( DWORD dwSkeletonIndex = 0; dwSkeletonIndex < NUI_SKELETON_COUNT; ++ dwSkeletonIndex )
    {
        if( m_SkeletonFrame.SkeletonData[ dwSkeletonIndex ].eTrackingState == NUI_SKELETON_TRACKED )
        {
            RenderSingleSkeleton( dwSkeletonIndex, fX, fY, fWidth, fHeight, bClip, bRegisterToColor );
        }
    }


    EndRender();
    return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------------
// Name: GetJointProjectedLocation
// Desc: Computes the projected position of a specific skeleton joint.
//--------------------------------------------------------------------------------------
XMFLOAT2 NuiVisualization::GetJointProjectedLocation( DWORD dwSkeletonIndex, NUI_SKELETON_POSITION_INDEX SkeletonPositionIndex, FLOAT fWidth, FLOAT fHeight, BOOL bRegisterToColor ) const
{
    XMFLOAT3 vJointLocation;
    XMStoreFloat3( &vJointLocation, m_SkeletonFrame.SkeletonData[ dwSkeletonIndex ].SkeletonPositions[ SkeletonPositionIndex ] );

    // Check for divide by zero
    if ( fabs( vJointLocation.z ) > FLT_EPSILON  )
    {
        // Note:  Without tilt correction, any projection will be off, as the skeleton positions 
        //        are camera-relative, with Up as ( 0, 1, 0), which the axis of the camera may
        //        not be aligned to. You can see this by turning on and off tilt correction
        //        in the sample.

        LONG plDepthX, plDepthY, plColorX, plColorY;
        USHORT usDepthValue;
        NuiTransformSkeletonToDepthImage( m_SkeletonFrame.SkeletonData[ dwSkeletonIndex ].SkeletonPositions[ SkeletonPositionIndex ],
                                          &plDepthX, &plDepthY, &usDepthValue );
        
        if ( bRegisterToColor )
        {
            HRESULT hr = NuiImageGetColorPixelCoordinatesFromDepthPixel( m_colorImageResolution,
                                                                         &m_colorViewArea,
                                                                         plDepthX,
                                                                         plDepthY,
                                                                         usDepthValue,
                                                                         &plColorX,
                                                                         &plColorY );

            if ( SUCCEEDED( hr ) )
            {
                return XMFLOAT2( plColorX * fWidth / m_dwColorStreamWidth, plColorY * fWidth / m_dwColorStreamWidth );       
            }
            else
            {
                // When a corresponding color value isn't avaible, use the raw depth value instead. 
                return XMFLOAT2( plDepthX * fWidth / s_dwDepthStreamWidth, plDepthY * fHeight / s_dwDepthStreamHeight );       
            }
        }
        else
        {
            return XMFLOAT2( plDepthX * fWidth / s_dwDepthStreamWidth, plDepthY * fHeight / s_dwDepthStreamHeight );       
        }
    }
    else
    {
        // A joint that is so close to the camera that its Z value is 0 can simply be drawn directly 
        // at the center of the 2D plane.
        return XMFLOAT2( fWidth * 0.5f , fHeight * 0.5f  );       
    }
}


//--------------------------------------------------------------------------------------
// Name: RenderSingleSkeleton
// Desc: Displays a skeleton based on the most skeleton frame buffer.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::RenderSingleSkeleton( DWORD dwSkeletonIndex, FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip, BOOL bRegisterToColor )
{
    // Pre-conditions
    assert( m_bIsInitialized );
    assert( dwSkeletonIndex < NUI_SKELETON_COUNT );
    assert( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_SKELETON );
    if( bRegisterToColor )
    {
        assert( m_dwComponentsToProcess & NUI_INITIALIZE_FLAG_USES_COLOR );
    }
    
    PIXBeginNamedEvent( 0, __FUNCTION__ );


    // There is nothing to draw if the player isn't tracked.
    if ( m_SkeletonFrame.SkeletonData[ dwSkeletonIndex ].eTrackingState != NUI_SKELETON_TRACKED )
    {
        PIXEndNamedEvent();
        return ERROR_SUCCESS;
    }
        
    XMFLOAT2 vScreenSpaceJoints[ NUI_SKELETON_POSITION_COUNT ];

    // Set up render states
    BeginRender();

    // Project the world space joints into screen space
    for ( UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; ++ i )
    {
        g_ScreenSpaceJoints[ i ] = GetJointProjectedLocation( dwSkeletonIndex, ( NUI_SKELETON_POSITION_INDEX )i, fWidth, fHeight, bRegisterToColor );
    }


    // Locate the beginning of the confidence array. We'll index into this array in the next loop.
    const NUI_SKELETON_POSITION_TRACKING_STATE *pSkeletonTrackingState = &m_SkeletonFrame.SkeletonData[ dwSkeletonIndex ].eSkeletonPositionTrackingState[ 0 ];
    const NUI_VISUALIZATION_SKELETON_RENDER_INFO *pSkeletonRenderInfo = &m_SkeletonRenderInfo[ dwSkeletonIndex ];

    // Draw each bone in the skeleton using the screen space joints
    for( UINT i = 0; i < g_uSkeletonBoneCount; i++ )
    {
        // Asign a color to each joint based on the confidence level. Don't draw a bone if one of its joints has no confidence.

        D3DCOLOR startColor;
        if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].StartJoint ] == NUI_SKELETON_POSITION_NOT_TRACKED || 
            !pSkeletonRenderInfo->bDraw[ g_SkeletonBoneList[ i ].StartJoint ] )
        {
            // A joint in the bone wasn't tracked during skeleton tracking...
            continue;
        }
        else if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].StartJoint ] == NUI_SKELETON_POSITION_TRACKED )
        {
            startColor = pSkeletonRenderInfo->TrackedColor[ g_SkeletonBoneList[ i ].StartJoint ];
        }
        else if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].StartJoint ] == NUI_SKELETON_POSITION_INFERRED )
        {
            startColor = pSkeletonRenderInfo->InferredColor[ g_SkeletonBoneList[ i ].StartJoint ];
        }
        else
        {
            assert( false );
            continue;
        }

        D3DCOLOR endColor;
        if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].EndJoint ] == NUI_SKELETON_POSITION_NOT_TRACKED || 
            !pSkeletonRenderInfo->bDraw[ g_SkeletonBoneList[ i ].EndJoint ] )
        {
            // A joint in the bone wasn't tracked during skeleton tracking...
            continue;
        }
        if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].EndJoint ] == NUI_SKELETON_POSITION_TRACKED )
        {
            endColor = pSkeletonRenderInfo->TrackedColor[ g_SkeletonBoneList[ i ].EndJoint ];
        }
        else if( pSkeletonTrackingState[ g_SkeletonBoneList[ i ].EndJoint ] == NUI_SKELETON_POSITION_INFERRED )
        {
            endColor = pSkeletonRenderInfo->InferredColor[ g_SkeletonBoneList[ i ].EndJoint ];
        }
        else
        {
            assert( false );
            continue;
        }

        // Vertically center the 1:1 aspect ratio projected skeleton over the destination area.
        // Draw the bone over the depth image
        XMFLOAT2 pntArray[ 2 ];
        pntArray[ 0 ].x = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].StartJoint ].x + fX;
        pntArray[ 0 ].y = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].StartJoint ].y + fY;
        pntArray[ 1 ].x = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].EndJoint ].x + fX;
        pntArray[ 1 ].y = g_ScreenSpaceJoints[ g_SkeletonBoneList[ i ].EndJoint ].y + fY;

        if( !bClip || ClipLineToRect( &pntArray[ 0 ], &pntArray[ 1 ], fX, fY, fWidth, fHeight ) )
        {
            ATG::DebugDraw::DrawScreenSpaceLine( pntArray[ 0 ], startColor, pntArray[ 1 ], endColor, 3 );
        }

    }

    PIXEndNamedEvent();

    EndRender();
    return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// Name: InitializeDepthColorTable()
// Desc: Initialize the lookup table to display depth values in color
//--------------------------------------------------------------------------------------
VOID NuiVisualization::InitializeDepthColorTable()
{
    // Build depth map visualization color table. Rainbow linear gradient mirrored at
    // center point with gradual dimming starting at center point to start of table.
    
    const INT iHalfTableSize    = ARRAYSIZE( m_DepthColorTable ) / 2;
    FLOAT fGutter               = 0.2f;   
    INT iTableIndex             = iHalfTableSize;
    FLOAT fStep                 = ( 1.0f - ( fGutter * 2.0f ) ) / iHalfTableSize;

    for( FLOAT t = fGutter; t < ( 1.0f - fGutter ); t += fStep )
    {
        FLOAT fColor[ 3 ]       = { 0, 0, 0 };
        FLOAT fBand             = 0.7f;
        const FLOAT fCurveExp   = 2.0f;   
        const FLOAT fBandGap    = 1.0f - fBand;
         
        for( INT i = 0; i < 3; ++ i ) 
        {
            FLOAT s = ( t - fBandGap * 0.5f * i ) / fBand;
            if ( ( s >= 0 ) && ( s <= 1 ) )
            {
                fColor[ i ] = powf( sinf( s * XM_PI * 2.0f - XM_PI * 0.5f ) * 0.5f + 0.5f, fCurveExp );
            }
        }

        m_DepthColorTable[ iTableIndex++ ] = D3DCOLOR_RGBA( (BYTE)( fColor[ 0 ] * 255.0f ),
                                                            (BYTE)( fColor[ 1 ] * 255.0f ),
                                                            (BYTE)( fColor[ 2 ] * 255.0f ),
                                                            0xff );
    }
    
    for( INT i = 0; i < iHalfTableSize; ++ i )
    {
        COLORREF s = m_DepthColorTable[ ARRAYSIZE( m_DepthColorTable ) - 1 - i ];
        
        FLOAT fDim = ( FLOAT )i / (FLOAT)iHalfTableSize;
        
        m_DepthColorTable[ i ] = D3DCOLOR_RGBA( ( BYTE )( (FLOAT)D3DCOLOR_GETRED( s ) * ( 0.25f + ( fDim * 0.75f ) ) ),
                                                ( BYTE )( (FLOAT)D3DCOLOR_GETGREEN( s ) * ( 0.25f + ( fDim * 0.75f ) ) ),
                                                ( BYTE )( (FLOAT)D3DCOLOR_GETBLUE( s ) * ( 0.25f + ( fDim * 0.75f ) ) ),
                                                 0xff );
    }
}


//--------------------------------------------------------------------------------------
// Name: InitializeVideoTextures()
// Desc: Recreates the m_pVideoTextures array of D3D textures using the current
//       resolution and pixel format settings.
//--------------------------------------------------------------------------------------
HRESULT NuiVisualization::InitializeVideoTextures( D3DDevice* pd3dDevice, 
                                                   IDirect3DTexture9** ppVideoTexture, 
                                                   DWORD dwWidth, DWORD dwHeight )
{
    HRESULT hr;

    // Release the old texture
    if( *ppVideoTexture != NULL )
    {
        (*ppVideoTexture)->BlockUntilNotBusy();
        (*ppVideoTexture)->Release();
        *ppVideoTexture = NULL;
    }

       
    // Create the new texture using the current resolution and pixel format
    hr = pd3dDevice->CreateTexture( dwWidth, dwHeight, 1, 0,
        GetAs16SRGBFormat( D3DFMT_LIN_X8R8G8B8 ), D3DPOOL_MANAGED, ppVideoTexture, NULL );
    if( FAILED( hr ) )
    {
        return E_FAIL;
    }

    // Clear texture
    D3DLOCKED_RECT Locked;
    if( FAILED( (*ppVideoTexture)->LockRect( 0, &Locked, NULL, 0 ) ) )
        return E_FAIL;

    // Fill the texture with black
    DWORD* lpBits = ( DWORD* )Locked.pBits;
    for( UINT k = 0; k < Locked.Pitch * dwHeight / 4; ++ k ) 
        *lpBits++ = 0xff000000;

    (*ppVideoTexture)->UnlockRect( 0 );

    return S_OK;
}

} // namespace ATG