//--------------------------------------------------------------------------------------
// AtgNuiVisualization.h
//
// Visualization tools for Kinect
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#pragma warning(push)
#pragma warning(disable : 4324)

#include <NuiApi.h>


namespace ATG
{

//------------------------------------------------------------------------
// Name: struct NUI_VISUALIZATION_BONE_JOINTS
// Desc: Holds the index to the two joints defining a bone. 
//------------------------------------------------------------------------
struct NUI_VISUALIZATION_BONE_JOINTS
{
    NUI_SKELETON_POSITION_INDEX StartJoint;
    NUI_SKELETON_POSITION_INDEX EndJoint;
};

// Global list of skeleton bones available to all sample.
extern const NUI_VISUALIZATION_BONE_JOINTS g_SkeletonBoneList[];
extern const UINT g_uSkeletonBoneCount;


//------------------------------------------------------------------------
// Name: struct NUI_VISUALIZATION_SKELETON_RENDER_INFO
// Desc: Holds the rendering options for a specific skeleton.
//------------------------------------------------------------------------
struct NUI_VISUALIZATION_SKELETON_RENDER_INFO
{
    BOOL     bDraw[ NUI_SKELETON_POSITION_COUNT ];
    D3DCOLOR TrackedColor[ NUI_SKELETON_POSITION_COUNT ];
    D3DCOLOR InferredColor[ NUI_SKELETON_POSITION_COUNT ];

    VOID Initialize( D3DCOLOR trackedColor, D3DCOLOR inferredColor, BOOL draw = TRUE );
    VOID SplatTrackedColor( D3DCOLOR trackedColor);
    VOID SplatInferredColor( D3DCOLOR inferredColor ); 
};


//------------------------------------------------------------------------
// Name: class NuiVisualization
// Desc: Visualization support for Kinect.
//------------------------------------------------------------------------
class NuiVisualization
{
public:
    NuiVisualization();
    ~NuiVisualization();


    HRESULT Initialize( ::D3DDevice* pd3dDevice, DWORD dwComponentsToProcess, 
    	                NUI_IMAGE_RESOLUTION colorImageResolution = NUI_IMAGE_RESOLUTION_640x480,
                        BOOL bRenderDashStyleDepthPreview = FALSE );
	HRESULT Shutdown();

    HRESULT SetColorTexture( IDirect3DTexture9* pColorTexture, const NUI_IMAGE_VIEW_AREA* pViewArea = NULL );
    HRESULT SetDepthTexture( IDirect3DTexture9* pDepthTexture, BOOL bColorize = TRUE );
    HRESULT SetSkeletons( const NUI_SKELETON_FRAME* pSkeletonFrame );

    HRESULT SetSkeletonRenderInfo( DWORD dwSkeletonIndex, const NUI_VISUALIZATION_SKELETON_RENDER_INFO* pSkeletonRenderInfo );
    HRESULT GetSkeletonRenderInfo( DWORD dwSkeletonIndex, NUI_VISUALIZATION_SKELETON_RENDER_INFO* pSkeletonRenderInfo );

    XMFLOAT2 GetJointProjectedLocation( DWORD dwSkeletonIndex, NUI_SKELETON_POSITION_INDEX SkeletonPositionIndex, FLOAT fWidth, FLOAT fHeight, BOOL bRegisterToColor = FALSE ) const;

    VOID BeginRender();
    VOID EndRender();

    HRESULT RenderColorStream( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight );
    HRESULT RenderDepthStream( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight );
    HRESULT RenderSkeletons( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip = FALSE, BOOL bRegisterToColor = FALSE );
    HRESULT RenderSingleSkeleton( DWORD dwSkeletonIndex, FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, BOOL bClip = FALSE, BOOL bRegisterToColor = FALSE );
    HRESULT RenderCustomDepthStream( FLOAT fX, FLOAT fY, FLOAT fWidth, FLOAT fHeight, LPDIRECT3DTEXTURE9 pTexture );
    HRESULT RenderDashStyleDepthPreview( FLOAT fX, float fY, float fWidth, FLOAT fHeight );


    inline D3DCOLOR* GetColorTable() 
    {
        return &m_DepthColorTable[0];
    }
    inline const IDirect3DTexture9* GetVis320x240Texture ()
    {
        return m_pDepthTexture[ m_depthDisplaying ];
    }

private:
    NuiVisualization( const NuiVisualization& rhs );
    NuiVisualization& operator =( const NuiVisualization& rhs );

    VOID InitializeDepthColorTable();
    HRESULT InitializeVideoTextures( D3DDevice* pd3dDevice, 
                                     IDirect3DTexture9** ppVideoTexture, 
                                     DWORD dwWidth, DWORD dwHeight );
    
    // List of states to save.  We use an enum so that the list can evolve over time
    // without worrying about re-ordering, inserting, or removing saved states.
    enum SAVEDSTATES
    {
        SAVEDSTATE_D3DRS_ALPHABLENDENABLE,
        SAVEDSTATE_D3DRS_SRCBLEND,
        SAVEDSTATE_D3DRS_DESTBLEND,
        SAVEDSTATE_D3DRS_BLENDOP,
        SAVEDSTATE_D3DRS_ALPHATESTENABLE,
        SAVEDSTATE_D3DRS_ALPHAREF,
        SAVEDSTATE_D3DRS_ALPHAFUNC,
        SAVEDSTATE_D3DRS_FILLMODE,
        SAVEDSTATE_D3DRS_CULLMODE,
        SAVEDSTATE_D3DRS_ZENABLE,
        SAVEDSTATE_D3DRS_STENCILENABLE,
        SAVEDSTATE_D3DRS_VIEWPORTENABLE,
        SAVEDSTATE_D3DSAMP_MINFILTER,
        SAVEDSTATE_D3DSAMP_MAGFILTER,
        SAVEDSTATE_D3DSAMP_ADDRESSU,
        SAVEDSTATE_D3DSAMP_ADDRESSV,

        SAVEDSTATE_MAX
    };

    IDirect3DVertexDeclaration9* m_pVideoVertexDecl;     // Vertex format declaration
    IDirect3DVertexShader9*      m_pVideoVertexShader;   // Vertex Shader
    IDirect3DPixelShader9*       m_pVideoPixelShaderRGB; // Pixel Shader for RGB image

    BOOL m_bIsInitialized;
    BOOL m_bColorIsNew;      
    UINT m_colorDisplaying;
    UINT m_depthDisplaying;
    IDirect3DTexture9* m_pColorTexture[ 2 ];
    IDirect3DTexture9* m_pDepthTexture[ 2 ];

    // Depth preview declarations, shaders and textures.
    IDirect3DVertexShader9*         m_pDepthPreviewVS;
    IDirect3DPixelShader9*          m_pDepthPreviewPS;
    IDirect3DVertexShader9*         m_pDepthPreviewSmoothingVS;
    IDirect3DTexture9*              m_pSmoothDepthTexture;
    IDirect3DTexture9*              m_pSmoothDepthTextureSwap;
    IDirect3DTexture9*              m_pNuiDepthTexture;

    // Caller supplied information.
    ::D3DDevice* m_pd3dDevice;
    DWORD m_dwComponentsToProcess;

    // Each color streams can have different dimensions but the depth stream is always 320 X 240.
    NUI_IMAGE_RESOLUTION m_colorImageResolution;
    DWORD m_dwColorStreamWidth;
    DWORD m_dwColorStreamHeight;
    NUI_IMAGE_VIEW_AREA m_colorViewArea;
    const static DWORD s_dwDepthStreamWidth = 320;
    const static DWORD s_dwDepthStreamHeight = 240;
    const static DWORD s_dwDashStyleDepthPreviewWidth = 128;
    const static DWORD s_dwDashStyleDepthPreviewPitch = 128;
    const static DWORD s_dwDashStyleDepthPreviewHeight = 96;

    // D3D State  preserving information
    DWORD m_dwNestedBeginCount;
    DWORD m_dwSavedState[ SAVEDSTATE_MAX ];

    // Holds the data for all skeletons for a given frame. 
    NUI_SKELETON_FRAME m_SkeletonFrame;
    NUI_VISUALIZATION_SKELETON_RENDER_INFO m_SkeletonRenderInfo[ NUI_SKELETON_COUNT ];

    // Color lookup table to display depth values as color. Depth values can be displayed
    // as color values in many different ways, we chose to use a ramp lookup table
    D3DCOLOR    m_DepthColorTable[ 512 ];
};

} // namespace ATG

#pragma warning(pop)