//--------------------------------------------------------------------------------------
// AvatarRenderer.cpp
//
// A class for rendering an XBox avatar.  This class is similar to the IXAtgAvatarRenderer
// interface provided in the XDK
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgAvatarRenderer.h"
#include <xgraphics.h>
#include "AtgUtil.h"

namespace ATG
{

inline DWORD RoundUpToAlign( DWORD value, DWORD align )
{
    assert( ( align & ( align - 1 ) ) == 0 );   // checks the align is a power of two
    return ( ( value + align - 1 ) & ~( align - 1 ) );
}


//--------------------------------------------------------------------------------------
// Name: CreateAvatar()
// Desc: Create the avatar.  Normally XAvatarGetAssets would be used to create the avatar.
//       In this case, since there is no Avatar library for Windows, the avatar is loaded
//       from file that was previously saved from an avatar on Xbox 360.  The data is
//       endian swapped, and pointers are fixed up to arrive at the final avatar.  
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::CreateAvatar( XAVATAR_METADATA& metadata )
{
    // Set all pose joints to identity and all texture layers to frame zero on construction. 
    XAVATAR_SKELETON_POSE_JOINT jointIdentity;
    jointIdentity.Position = XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f );
    jointIdentity.Rotation = XMQuaternionIdentity();
    jointIdentity.Scale    = XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f );
    for ( INT j = 0; j < XAVATAR_MAX_SKELETON_JOINTS; ++j )
    {
        m_AvatarJointPose[ j ] = jointIdentity;
    }
    ZeroMemory( m_dwTextureLayers, sizeof( m_dwTextureLayers ) );

    // Retrieve the required CPU and GPU buffer sizes. If we cannot get
    // these sizes then the initialization fails.
    DWORD cpuBufferSize, gpuBufferSize;
    RETURN_ON_FAIL( XAvatarGetAssetsResultSize( XAVATAR_COMPONENT_MASK_ALL, &cpuBufferSize, &gpuBufferSize ) );
    
    // Calculate the joint buffer size required per joint buffer, and then
    // add this to the overall GPU allocation size.
    const DWORD jointBufferSize   = XAVATAR_MAX_SKELETON_JOINTS * 12 * sizeof( FLOAT );
    const DWORD gpuAllocSize      = RoundUpToAlign( gpuBufferSize, GPU_MEMORY_BUFFER_ALIGN );
    
    // Attempt to allocate the CPU and GPU buffers with the appropriate 
    // alignments. We add a little extra to the GPU buffer for joints.
    m_pAssets = reinterpret_cast<XAVATAR_ASSETS*>( XMemAlloc( cpuBufferSize, MAKE_XALLOC_ATTRIBUTES( 0, TRUE, TRUE, FALSE, SAMPLE_ALLOCATOR_ID, XALLOC_ALIGNMENT_4, XALLOC_MEMPROTECT_READWRITE, FALSE, XALLOC_MEMTYPE_HEAP ) ) );
    m_pGpuBuffer = reinterpret_cast<BYTE*>( XPhysicalAlloc( gpuAllocSize, MAXULONG_PTR, GPU_MEMORY_BUFFER_ALIGN, PAGE_READWRITE | PAGE_WRITECOMBINE ) );
    m_pJointBuffer = reinterpret_cast<BYTE*>( XPhysicalAlloc( jointBufferSize * JOINT_BUFFER_COUNT, MAXULONG_PTR, 4, PAGE_READWRITE | PAGE_WRITECOMBINE ) );

    // If we failed to allocate memory then fail.
    if ( !m_pAssets || !m_pGpuBuffer )//|| !m_pJointBuffer )
    {
        return E_OUTOFMEMORY;
    }

    // Construct the joint vertex buffers in place in the GPU buffer.
    for ( INT jbi =  0; jbi < JOINT_BUFFER_COUNT; ++jbi )
    {
        XGSetVertexBufferHeader( jointBufferSize, 0, 0, 0, &m_JointBufferVBs[ jbi ] );
        XGOffsetResourceAddress( &m_JointBufferVBs[ jbi ], &m_pJointBuffer[ jbi * jointBufferSize ] );
    }

    m_dwJointBufferIndex = 0;

    // Get the avatar assets
    if ( ERROR_SUCCESS != XAvatarGetAssets( &metadata, XAVATAR_COMPONENT_MASK_ALL, 0, cpuBufferSize, m_pAssets, gpuAllocSize, m_pGpuBuffer, NULL ) )
    {
        return E_FAIL;
    }

    // Populate vertex buffers, index buffers, and texture headers for all component models.
    for ( DWORD cmi = 0; cmi < m_pAssets->ComponentCount; ++cmi )
    {
        // Reference this component model.
        const XAVATAR_MODEL* model = &m_pAssets->pComponentModels[ cmi ];

        // Skip models with no geometry
        if ( model->GlobalIndexBufferSize == 0 )
        {
            continue;
        }

        // Construct the vertex buffer in place, pointing at the global
        // vertex buffer for this model.
        XGSetVertexBufferHeader( model->GlobalVertexBufferSize, 0, 0, 0, &m_VertexBuffers[ cmi ] );
        XGOffsetResourceAddress( &m_VertexBuffers[ cmi ], model->pGlobalVertexBuffer );

        // Construct the index buffer in place, pointing at the global
        // index buffer for this model.
        XGSetIndexBufferHeader ( model->GlobalIndexBufferSize, 0, D3DFMT_INDEX16, 0, 0, &m_IndexBuffers[ cmi ] );
        XGOffsetResourceAddress( &m_IndexBuffers[ cmi ], model->pGlobalIndexBuffer );

        // Construct textures in place.
        for ( DWORD ti = 0; ti < model->TextureCount; ++ti )
        {
            // Reference this texture.
            const XAVATAR_TEXTURE* texture = &model->pTextures[ ti ];

            // Reference the address of the first layer of this texture.
            BYTE* layerData = texture->pBaseData;

            // Iterate over all layers of this texture, creating the D3D
            // textures that reference that layer data.
            for ( DWORD li = 0; li < texture->LayerCount; ++li )
            {
                // Construct the layer texture, pointing at the
                // layer data for this texture and layer.
                XGSetTextureHeader( texture->Width,
                                    texture->Height, 
                                    1, 0, texture->Format, 0, 
                                    ( UINT )( layerData + li * texture->BaseSize ), 
                                    0, 0, &m_Textures[ cmi ][ ti ][ li ] , 0, 0 );

            }
        }
    }

    // Zero the shader structure before we begin to load shaders.
    ZeroMemory( m_D3dShaders, sizeof( m_D3dShaders ) );

    // Define the vertex structure for the "head" shaders. These have six
    // UV sets, for a total of 52 bytes per vertex in stream 0.
    const D3DVERTEXELEMENT9 headDeclDesc[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0 }, // XAVATAR_VERTEX_POSITION
        { 0, 12, D3DDECLTYPE_HEND3N,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0 }, // XAVATAR_VERTEX_NORMAL
        { 0, 16, D3DDECLTYPE_UBYTE4N,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0 }, // XAVATAR_VERTEX_WEIGHTS
        { 0, 20, D3DDECLTYPE_UBYTE4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 }, // XAVATAR_VERTEX_BINDINGS
        { 0, 24, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0 }, // XAVATAR_VERTEX_COLOR
        { 0, 28, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0 }, // XAVATAR_VERTEX_UV
        { 0, 32, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     1 }, // XAVATAR_VERTEX_UV
        { 0, 36, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     2 }, // XAVATAR_VERTEX_UV
        { 0, 40, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     3 }, // XAVATAR_VERTEX_UV
        { 0, 44, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     4 }, // XAVATAR_VERTEX_UV
        { 0, 48, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     5 }, // XAVATAR_VERTEX_UV
        { 1,  0, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     6 }, // Joint buffer.
        { 1, 16, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     7 }, // Joint buffer.
        { 1, 32, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     8 }, // Joint buffer.
        D3DDECL_END()
    };

    // Define the vertex structure for the "body" shaders. These have three
    // UV sets, for a total of 40 bytes per vertex in stream 0.
    const D3DVERTEXELEMENT9 bodyDeclDesc[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0 }, // XAVATAR_VERTEX_POSITION
        { 0, 12, D3DDECLTYPE_HEND3N,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0 }, // XAVATAR_VERTEX_NORMAL
        { 0, 16, D3DDECLTYPE_UBYTE4N,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0 }, // XAVATAR_VERTEX_WEIGHTS
        { 0, 20, D3DDECLTYPE_UBYTE4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 }, // XAVATAR_VERTEX_BINDINGS
        { 0, 24, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0 }, // XAVATAR_VERTEX_COLOR
        { 0, 28, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0 }, // XAVATAR_VERTEX_UV
        { 0, 32, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     1 }, // XAVATAR_VERTEX_UV
        { 0, 36, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     2 }, // XAVATAR_VERTEX_UV
        { 1,  0, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     6 }, // Joint buffer.
        { 1, 16, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     7 }, // Joint buffer.
        { 1, 32, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     8 }, // Joint buffer.
        D3DDECL_END()
    };

    // Define the vertex structure for the "shiny body" shaders. These have two
    // UV sets, for a total of 36 bytes per vertex in stream 0.
    const D3DVERTEXELEMENT9 shinyDeclDesc[] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0 }, // XAVATAR_VERTEX_POSITION
        { 0, 12, D3DDECLTYPE_HEND3N,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0 }, // XAVATAR_VERTEX_NORMAL
        { 0, 16, D3DDECLTYPE_UBYTE4N,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0 }, // XAVATAR_VERTEX_WEIGHTS
        { 0, 20, D3DDECLTYPE_UBYTE4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0 }, // XAVATAR_VERTEX_BINDINGS
        { 0, 24, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0 }, // XAVATAR_VERTEX_COLOR
        { 0, 28, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0 }, // XAVATAR_VERTEX_UV
        { 0, 32, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     1 }, // XAVATAR_VERTEX_UV
        { 1,  0, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     6 }, // Joint buffer.
        { 1, 16, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     7 }, // Joint buffer.
        { 1, 32, D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     8 }, // Joint buffer.
        D3DDECL_END()
    };

    IDirect3DDevice9& device = *m_pd3dDevice;

    HRESULT hr;

    // Create the head and body vertex declarations. We will add appropriate
    // references for each shader later.
    D3DVertexDeclaration *headDecl, *bodyDecl, *shinyDecl;
    if ( !SUCCEEDED( hr = device.CreateVertexDeclaration( bodyDeclDesc,  &bodyDecl  ) ) ||
         !SUCCEEDED( hr = device.CreateVertexDeclaration( headDeclDesc,  &headDecl  ) ) ||
         !SUCCEEDED( hr = device.CreateVertexDeclaration( shinyDeclDesc, &shinyDecl ) )  )
    {
        return E_FAIL;
    }

    // Create all vertex shaders. We will add appropriate references for 
    // each shader later.
    D3DVertexShader *bodyVs, *headOpaqueVs,*bodyShinyVs;
    ATG::LoadVertexShader( "game:\\media\\shaders\\body_vs.xvu", &bodyVs );
    ATG::LoadVertexShader( "game:\\media\\shaders\\head_opaque_vs.xvu", &headOpaqueVs );
    ATG::LoadVertexShader( "game:\\media\\shaders\\body_shiny_vs.xvu", &bodyShinyVs );

    // Create all pixel shaders. We will add appropriate references for each
    // shader later.
    D3DPixelShader *bodyOpaquePs, *headOpaquePs, *bodyTranspPs;
    D3DPixelShader *bodyShinyOpaquePs, *bodyShinyTranspPs;
    ATG::LoadPixelShader( "game:\\media\\shaders\\body_opaque_ps.xpu", &bodyOpaquePs );
    ATG::LoadPixelShader( "game:\\media\\shaders\\head_opaque_ps.xpu", &headOpaquePs );
    ATG::LoadPixelShader( "game:\\media\\shaders\\body_transparent_ps.xpu", &bodyTranspPs );
    ATG::LoadPixelShader( "game:\\media\\shaders\\body_shiny_opaque_ps.xpu", &bodyShinyOpaquePs );
    ATG::LoadPixelShader( "game:\\media\\shaders\\body_shiny_transparent_ps.xpu", &bodyShinyTranspPs );

    // Set declaration, vertex and pixel shaders for each material.
    m_D3dShaders[ XAVATAR_SHADER_BODY_OPAQUE            ].m_VertexDeclaration  = bodyDecl;
    m_D3dShaders[ XAVATAR_SHADER_BODY_OPAQUE            ].m_VertexShader       = bodyVs;
    m_D3dShaders[ XAVATAR_SHADER_BODY_OPAQUE            ].m_PixelShader        = bodyOpaquePs;

    m_D3dShaders[ XAVATAR_SHADER_BODY_TRANSPARENT       ].m_VertexDeclaration  = bodyDecl;
    m_D3dShaders[ XAVATAR_SHADER_BODY_TRANSPARENT       ].m_VertexShader       = bodyVs;
    m_D3dShaders[ XAVATAR_SHADER_BODY_TRANSPARENT       ].m_PixelShader        = bodyTranspPs;

    m_D3dShaders[ XAVATAR_SHADER_BODY_SHINY_OPAQUE      ].m_VertexDeclaration  = shinyDecl;
    m_D3dShaders[ XAVATAR_SHADER_BODY_SHINY_OPAQUE      ].m_VertexShader       = bodyShinyVs;
    m_D3dShaders[ XAVATAR_SHADER_BODY_SHINY_OPAQUE      ].m_PixelShader        = bodyShinyOpaquePs;

    m_D3dShaders[ XAVATAR_SHADER_BODY_SHINY_TRANSPARENT ].m_VertexDeclaration  = shinyDecl;
    m_D3dShaders[ XAVATAR_SHADER_BODY_SHINY_TRANSPARENT ].m_VertexShader       = bodyShinyVs;
    m_D3dShaders[ XAVATAR_SHADER_BODY_SHINY_TRANSPARENT ].m_PixelShader        = bodyShinyTranspPs;

    m_D3dShaders[ XAVATAR_SHADER_HEAD_OPAQUE            ].m_VertexDeclaration  = headDecl;
    m_D3dShaders[ XAVATAR_SHADER_HEAD_OPAQUE            ].m_VertexShader       = headOpaqueVs;
    m_D3dShaders[ XAVATAR_SHADER_HEAD_OPAQUE            ].m_PixelShader        = headOpaquePs;
 
    // Add appropriate references to each declaration and shader.
    for ( INT s = 0; s < XAVATAR_SHADER_COUNT; ++s )
    {
        m_D3dShaders[ s ].m_VertexDeclaration ->AddRef();
        m_D3dShaders[ s ].m_VertexShader      ->AddRef();
        m_D3dShaders[ s ].m_PixelShader       ->AddRef();
    }

    // Set light color. 
    m_vLightColor    [ 0 ] = XMVectorSet( +0.200f, +0.100f, +0.050f, 1.0f );
    m_vLightColor    [ 1 ] = XMVectorSet( +0.100f, +0.100f, +0.200f, 1.0f );
    m_vLightColor    [ 2 ] = XMVectorSet( +0.400f, +0.400f, +0.400f, 1.0f );

    // Set light direction. 
    m_vLightDirection[ 0 ] = XMVectorSet( +0.342f, +0.000f, -0.940f, 1.0f );
    m_vLightDirection[ 1 ] = XMVectorSet( +1.000f, +0.000f, +0.000f, 1.0f );
    m_vLightDirection[ 2 ] = XMVectorSet( -0.500f, -0.612f, -0.612f, 1.0f );

    // Clear the shader texture index and constant register maps to invalid
    // values so we will know immediately if we get an invalid value.
    FillMemory( m_ShaderTextureIndexMap,     sizeof( m_ShaderTextureIndexMap     ), 0xff );
    FillMemory( m_PixelConstantRegisterMap,  sizeof( m_PixelConstantRegisterMap  ), 0xff );
    FillMemory( m_VertexConstantRegisterMap, sizeof( m_VertexConstantRegisterMap ), 0xff );

    // Set up mappings for texture usages to the appropriate shader index.
    // We only provide mappings for textures that have no UVs. Textures with
    // UVs will automatically be mapped to the corresponding texture index.
    m_ShaderTextureIndexMap[ XAVATAR_SHADER_PARAM_USAGE_TEXTURE_REFLECTION ][ XAVATAR_SHADER_BODY_SHINY_OPAQUE      ] = 2;
    m_ShaderTextureIndexMap[ XAVATAR_SHADER_PARAM_USAGE_TEXTURE_REFLECTION ][ XAVATAR_SHADER_BODY_SHINY_TRANSPARENT ] = 2;

    // Set up mappings for all pixel constants to the appropriate register.
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_CUSTOM_0        ] = 210;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_CUSTOM_1        ] = 211;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_CUSTOM_2        ] = 212;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_TRANSPARENCY          ] = 213;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_REFLECTIVITY          ] = 214;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_SKIN            ] = 215;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_HAIR            ] = 216;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_MOUTH           ] = 217;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_IRIS            ] = 218;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_EYEBROW         ] = 219;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_EYE_SHADOW      ] = 220;    
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_FACIAL_HAIR     ] = 221;    
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_SKIN_FEATURE_1  ] = 222;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_SKIN_FEATURE_2  ] = 223;
    m_PixelConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_PIXEL_CONSTANT_COLOR_RIMLIGHT        ] = 5;

    // We can now remove the local reference to each declaration and shader.
    SAFE_RELEASE( headDecl );
    SAFE_RELEASE( bodyDecl );
    SAFE_RELEASE( shinyDecl );
    SAFE_RELEASE( bodyVs );
    SAFE_RELEASE( headOpaqueVs );
    SAFE_RELEASE( bodyOpaquePs );
    SAFE_RELEASE( headOpaquePs );
    SAFE_RELEASE( bodyTranspPs );
    SAFE_RELEASE( bodyShinyOpaquePs );
    SAFE_RELEASE( bodyShinyTranspPs );
    SAFE_RELEASE( bodyShinyVs );
    
	RebuildJoints();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: RebuildJoints()
// Desc: Rebuilds the joints from the current pose. 
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::RebuildJoints()
{
    // Intermediate storage for the joint transforms and 12-FLOAT packed
    // transposes, which is how we present the data to the vertex shader.    
    static XMMATRIX  matJointTransforms[ XAVATAR_MAX_SKELETON_JOINTS ];
    static XMFLOAT4A transposes[ XAVATAR_MAX_SKELETON_JOINTS * 3 ];
    
    // First, build joint transforms in world-space.
    for ( DWORD j = 0; j < m_pAssets->pSkeleton->Count; ++j )
    {
        // Reference the relevant components for building this joint.
        const XAVATAR_SKELETON_POSE_JOINT&     offset    = m_AvatarJointPose[ j ];
        const XAVATAR_SKELETON_JOINT&          skelJoint = m_pAssets->pSkeleton->pJoints[ j ];
        const XAVATAR_SKELETON_POSE_JOINT&     bindPose  = skelJoint.BindPose.Local;
        const XAVATAR_SKELETON_HIERARCHY_JOINT hierarchy = skelJoint.Hierarchy;

        // Make sure that the parent has already been processed
        assert( j == 0 || hierarchy.Parent < j );

        // Generate the bindpose transform for this joint.
        XMMATRIX bindPoseT;
        bindPoseT = XMMatrixRotationQuaternion( bindPose.Rotation );
        bindPoseT = XMMatrixMultiply( bindPoseT, XMMatrixScalingFromVector( bindPose.Scale ) );
        bindPoseT.r[ 3 ] = bindPose.Position;

        // Generate the offset transform for this joint.
        XMMATRIX offsetT;
        offsetT = XMMatrixRotationQuaternion( offset.Rotation );
        offsetT = XMMatrixMultiply( offsetT, XMMatrixScalingFromVector( offset.Scale ) );
        offsetT.r[ 3 ] = offset.Position;

        // Retrieve the parent transform, or the basis for joint zero.
        XMMATRIX parentT = ( j ? matJointTransforms[ hierarchy.Parent ] : XMMatrixIdentity() );

        // Generate the joint transform by applying the bindpose to the 
        // offset, and then transforming by the parent transform.
        matJointTransforms[ j ] = XMMatrixMultiply( XMMatrixMultiply( offsetT, bindPoseT ), parentT );
    }

    // Now pre-multiply these joint transforms by the inverse bind-pose 
    // transform for that joint. This gives us a final transform that 
    // converts model-space vertices into skinned vertices in one multiply.
    // We take the transposes of these transforms and pack them into 12
    // floats for presentation to the vertex shader.
    for ( DWORD j = 0; j < m_pAssets->pSkeleton->Count; ++j )
    {
        // Reference the bindpose data for this joint.
        const XAVATAR_SKELETON_BINDPOSE_JOINT& bindPose = m_pAssets->pSkeleton->pJoints[ j ].BindPose;

        // Generate the bindpose transform for this joint, and preapply
        // it to the joint transform to generate the final transform.
        XMVECTOR determinant;
        XMMATRIX invBindPoseT;
        invBindPoseT = XMMatrixRotationQuaternion( bindPose.Rotation );
        invBindPoseT.r[ 3 ] = bindPose.Position;
        invBindPoseT = XMMatrixInverse( &determinant, invBindPoseT );
        XMMATRIX transform = XMMatrixMultiply ( invBindPoseT, matJointTransforms[ j ] );

        // Take the transpose and pack it into 12 floats, which is the
        // required format for our vertex shader.
        XMMATRIX transpose = XMMatrixTranspose( transform );
        XMStoreFloat4A( &transposes[ j * 3 + 0 ], transpose.r[ 0 ] );
        XMStoreFloat4A( &transposes[ j * 3 + 1 ], transpose.r[ 1 ] );
        XMStoreFloat4A( &transposes[ j * 3 + 2 ], transpose.r[ 2 ] );
    }

    // We now loop until we get an exclusive lock on the current joint
    // buffer. We only increment the buffer while we hold the lock.
    VOID*            pCurrJointData;
    D3DVertexBuffer* pCurrJointBuffer;
    for ( ; ; )
    {
        // Select the next joint buffer, into which we copy the data.
        DWORD dwCurrBufferIndex = m_dwJointBufferIndex;
        DWORD dwNextBufferIndex = ( dwCurrBufferIndex + 1 ) % JOINT_BUFFER_COUNT;
        pCurrJointBuffer       = &m_JointBufferVBs[ dwNextBufferIndex ];

        // Warn on joint buffer contention
        #ifdef _DEBUG
        {
            // If the current joint buffer is still busy, give
            // a warning about contention for the buffer.
            if( pCurrJointBuffer->IsBusy( ) )
            {
                // Regulates how often warnings like this appear.
                static const FLOAT JOINT_BUFFER_SET_WARN_INTERVAL_S = 10.0f;

                // Ensure we use a constant tick count throughout this
                // block.
                DWORD tickCount = GetTickCount();

                // Start out "last warning" time as double what we need
                // for a warning to spew
                static DWORD tickCountAtLastWarning = tickCount - (DWORD)(JOINT_BUFFER_SET_WARN_INTERVAL_S * 1000.0f * 2.0f);

                // Short-hand for the time that has passed (in seconds) 
                // since last warning was issued.
                FLOAT timeSinceLastWarning = (FLOAT)(tickCount - tickCountAtLastWarning) / 1000.0f;

                // If we haven't spewed out warning for a while, do so.
                if( timeSinceLastWarning > JOINT_BUFFER_SET_WARN_INTERVAL_S )
                {
                    ATG::DebugSpew( "WARNING: Avatar joint buffer is still in use by the device during update. Increase the joint buffer count." );
                    tickCountAtLastWarning = tickCount;
                }            
            }
        }
        #endif // _DEBUG

        // Lock the buffer ready to copy in the transposed transforms.
        if ( !SUCCEEDED( pCurrJointBuffer->Lock( 0, 0, &pCurrJointData, 0 ) ) )
        {
            return E_FAIL;
        }

        if ( m_dwJointBufferIndex == dwCurrBufferIndex )
        {
            m_dwJointBufferIndex = dwNextBufferIndex;
            break;
        }
    }

    // Copy the 12-FLOAT packed, transposed joint transforms into the
    // joint buffer, ready to be presented to the vertex shader.
    XMemCpyStreaming_WriteCombined( pCurrJointData, transposes, sizeof( transposes ) );

    // Unlock the joint buffer.
    if ( !SUCCEEDED( pCurrJointBuffer->Unlock() ) )
    {
        return E_FAIL;
    }
    
    // Return success.
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the avatar.
//--------------------------------------------------------------------------------------
VOID AvatarRenderer::Render( XMMATRIX matWorld, XMMATRIX matView, XMMATRIX matProj )
{

    // Set up device state and shader constants
    PreRender( &matWorld, &matView, &matProj );

    // Iterate over all component models and render opaque materials.
    for ( DWORD cmi = 0; cmi < m_pAssets->ComponentCount; ++cmi )
    {
        RenderOpaque( cmi,
                      &m_JointBufferVBs[ m_dwJointBufferIndex ] );
    }

    // Iterate over all component models and render transparent materials.
    for ( DWORD cmi = 0; cmi < m_pAssets->ComponentCount; ++cmi )
    {
        RenderTransparent( cmi,
                           &m_JointBufferVBs[ m_dwJointBufferIndex ] );
    }

    // End the render pass
    PostRender( );
}


//--------------------------------------------------------------------------------------
// Name: PreRender()
// Desc: Begin the rendering pass. This should be invoked before the
//       RenderOpaque() and RenderTransparent() functions, and
//       matched with a call to PostRender() once the pass is complete.
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::PreRender(  const XMMATRIX*     modelTransform,
                                    const XMMATRIX*     viewTransform,
                                    const XMMATRIX*     projectionTransform )
{
     XMMATRIX modelViewTransform = *modelTransform * *viewTransform;

    m_pd3dDevice->SetVertexShaderConstantF( 0, (FLOAT*)&modelViewTransform, 4 );
    m_pd3dDevice->SetVertexShaderConstantF( 4, (FLOAT*)projectionTransform, 4 );

    // set lights
    FLOAT ambientColor [ 4 ] = { 0.55f, 0.55f, 0.550f, 1.0f };

    XMVECTOR lightDirection[ 3 ];
    lightDirection[ 0 ] = XMVector3Normalize( m_vLightDirection[ 0 ] );
    lightDirection[ 1 ] = XMVector3Normalize( m_vLightDirection[ 1 ] );
    lightDirection[ 2 ] = XMVector3Normalize( XMVector3TransformNormal( XMVectorScale( m_vLightDirection[ 2 ], -1.0f ), *viewTransform ) );

    m_pd3dDevice->SetPixelShaderConstantF(   0, &m_vLightColor [ 2 ].x, 1 );
    m_pd3dDevice->SetPixelShaderConstantF(   1, &lightDirection[ 2 ].x, 1 );
    m_pd3dDevice->SetPixelShaderConstantF(   2, &m_vLightColor [ 0 ].x, 1 );
    m_pd3dDevice->SetPixelShaderConstantF(   3, &lightDirection[ 0 ].x, 1 );
    m_pd3dDevice->SetPixelShaderConstantF( 100, &m_vLightColor [ 1 ].x, 1 );
    m_pd3dDevice->SetPixelShaderConstantF( 101, &lightDirection[ 1 ].x, 1 );
    m_pd3dDevice->SetPixelShaderConstantF(   4, ambientColor,           1 );

    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );

    m_pd3dDevice->SetSamplerState_Inline( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 3, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 3, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 3, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 4, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 4, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 4, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 5, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 5, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 5, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 6, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 6, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 6, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 7, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 7, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 7, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 8, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 8, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState_Inline( 8, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: RenderOpaque()
// Desc: Render an opaque model. Make sure to call the PreRender()
//       function to set up the render state appropriately for the pass.
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::RenderOpaque( DWORD dwModelIndex, D3DVertexBuffer* pJointBuffer )
{
    XAVATAR_MODEL*      pModel = &m_pAssets->pComponentModels[ dwModelIndex ];
    D3DVertexBuffer*    pVertexBuffer = &m_VertexBuffers[ dwModelIndex ];
    D3DIndexBuffer*     pIndexBuffer = &m_IndexBuffers[ dwModelIndex ];

    // If the model is empty then we have nothing to do here.
    if ( !pModel->GlobalIndexBufferSize )
    {
        return S_OK;
    }

    // Defines the shaders we consider when performing the opaque pass.
    static const XAVATAR_SHADER OPAQUE_SHADERS[] = 
    { 
        XAVATAR_SHADER_BODY_SHINY_OPAQUE,
        XAVATAR_SHADER_BODY_OPAQUE, 
        XAVATAR_SHADER_HEAD_OPAQUE,
    };

    // Set up the render state for the opaque render pass.
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ALPHATESTENABLE,  TRUE                );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ALPHAFUNC,        D3DCMP_GREATEREQUAL );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ALPHAREF,         0x80                );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ZENABLE,          TRUE                );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ZWRITEENABLE,     TRUE                );

    D3DBLENDSTATE blendState;
    blendState.SrcBlend       = D3DBLEND_ONE;
    blendState.BlendOp        = D3DBLENDOP_ADD;
    blendState.DestBlend      = D3DBLEND_ZERO;
    blendState.SrcBlendAlpha  = D3DBLEND_ONE;
    blendState.BlendOpAlpha   = D3DBLENDOP_ADD;
    blendState.DestBlendAlpha = D3DBLEND_ZERO;
    m_pd3dDevice->SetBlendState( 0, blendState );

    m_pd3dDevice->SetIndices( pIndexBuffer );

    m_pd3dDevice->SetStreamSource( 1, pJointBuffer, 0, 12 * sizeof( FLOAT ) );

    // Render all appropriate batches of this model as normal opaque.
    for ( INT osi = 0; osi < ARRAYSIZE( OPAQUE_SHADERS ); ++osi )
    {
        // The current shader we are looking for.
        const XAVATAR_SHADER currentShader = OPAQUE_SHADERS[ osi ];

        // Set the vertex declaration and shader for the current shader.
        m_pd3dDevice->SetVertexDeclaration( m_D3dShaders[ currentShader ].m_VertexDeclaration  );
        m_pd3dDevice->SetVertexShader     ( m_D3dShaders[ currentShader ].m_VertexShader       );
        m_pd3dDevice->SetPixelShader      ( m_D3dShaders[ currentShader ].m_PixelShader        );

        // Render all batches of this model that use this shader.
        for ( DWORD bi = 0; bi < pModel->BatchCount; ++bi )
        {
            // Reference the current batch.
            const XAVATAR_TRIANGLE_BATCH& batch = pModel->pBatches[ bi ];

            // If this batch uses the current shader then we render it.
            if ( batch.ShaderInstance.Shader == currentShader )
            {
                SetShaderTexturesAndConstants( dwModelIndex, &batch.ShaderInstance, pModel->pTextures, m_dwTextureLayers );
                m_pd3dDevice->SetStreamSource( 0, pVertexBuffer, batch.pVertices - pModel->pGlobalVertexBuffer, batch.VertexStride );
                m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 0, ( WORD* )batch.pIndices - ( WORD* )pModel->pGlobalIndexBuffer, batch.TriangleCount );
                ClearShaderTexturesAndConstants( );
            }
        }
    }

    // Reset the device. Clear vertex declaration and shaders set above.
    m_pd3dDevice->SetVertexDeclaration( 0 );
    m_pd3dDevice->SetVertexShader     ( 0 );
    m_pd3dDevice->SetPixelShader      ( 0 );

    // Reset the device. Clear vertex buffer stream sources set above.
    m_pd3dDevice->SetStreamSource( 0, 0, 0, 0 );
    m_pd3dDevice->SetStreamSource( 1, 0, 0, 0 );

    // Reset the device: Clear the index buffer source set above.
    m_pd3dDevice->SetIndices( 0 );

    // Return success.
    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: RenderTransparent()
// Desc: Render a model in the transparent pass.  Make sure to call the PreRender()
//       function to set up the render state appropriately for the pass.
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::RenderTransparent( DWORD dwModelIndex, D3DVertexBuffer* pJointBuffer )
{
     XAVATAR_MODEL*      pModel = &m_pAssets->pComponentModels[ dwModelIndex ];
    D3DVertexBuffer*    pVertexBuffer = &m_VertexBuffers[ dwModelIndex ];
    D3DIndexBuffer*     pIndexBuffer = &m_IndexBuffers[ dwModelIndex ];

    // If the model is empty then we have nothing to do here.
    if ( !pModel->GlobalIndexBufferSize )
    {
        return S_OK;
    }

    // Defines the shaders we consider when performing the transparent pass.
    const XAVATAR_SHADER TRANSPARENT_SHADERS[] = 
    { 
        XAVATAR_SHADER_BODY_SHINY_TRANSPARENT,
        XAVATAR_SHADER_BODY_TRANSPARENT, 
    };

    // Set up the render state for the transparent render pass.
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ALPHATESTENABLE,          FALSE );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ZENABLE,                  TRUE  );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_ZWRITEENABLE,             FALSE );
    m_pd3dDevice->SetRenderState_Inline( D3DRS_HIGHPRECISIONBLENDENABLE, TRUE  );

    D3DBLENDSTATE blendState;
    blendState.SrcBlend       = D3DBLEND_SRCALPHA;
    blendState.BlendOp        = D3DBLENDOP_ADD;
    blendState.DestBlend      = D3DBLEND_INVSRCALPHA;
    blendState.SrcBlendAlpha  = D3DBLEND_ONE;
    blendState.BlendOpAlpha   = D3DBLENDOP_ADD;
    blendState.DestBlendAlpha = D3DBLEND_INVSRCALPHA;
    m_pd3dDevice->SetBlendState( 0, blendState );

    m_pd3dDevice->SetIndices( pIndexBuffer );

    // Set the joint buffer as a stream source on the device.
    m_pd3dDevice->SetStreamSource( 1, pJointBuffer, 0, 12 * sizeof( FLOAT ) );

    // Render all appropriate batches of this model as transparent.
    for ( INT tsi = 0; tsi < ARRAYSIZE( TRANSPARENT_SHADERS ); ++tsi )
    {
        // The current shader we are looking for.
        const XAVATAR_SHADER currentShader = TRANSPARENT_SHADERS[ tsi ];

        // Set the vertex declaration and shader for the current shader.
        m_pd3dDevice->SetVertexDeclaration( m_D3dShaders[ currentShader ].m_VertexDeclaration  );
        m_pd3dDevice->SetVertexShader     ( m_D3dShaders[ currentShader ].m_VertexShader       );
        m_pd3dDevice->SetPixelShader      ( m_D3dShaders[ currentShader ].m_PixelShader        );

        // Render all batches of this model that use this shader.
        for ( DWORD bi = 0; bi < pModel->BatchCount; ++bi )
        {
            // Reference the current batch.
            const XAVATAR_TRIANGLE_BATCH& batch = pModel->pBatches[ bi ];

            // If this batch uses the current shader then we render it.
            if ( batch.ShaderInstance.Shader == currentShader )
            {
                SetShaderTexturesAndConstants( dwModelIndex, &batch.ShaderInstance, pModel->pTextures, m_dwTextureLayers );
                m_pd3dDevice->SetStreamSource( 0, pVertexBuffer, batch.pVertices - pModel->pGlobalVertexBuffer, batch.VertexStride );
                m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 0, ( WORD* )batch.pIndices - ( WORD* )pModel->pGlobalIndexBuffer, batch.TriangleCount );
                ClearShaderTexturesAndConstants( );
            }
        }
    }

    // Reset the device. Clear vertex declaration and shaders set above.
    m_pd3dDevice->SetVertexDeclaration( 0 );
    m_pd3dDevice->SetVertexShader     ( 0 );
    m_pd3dDevice->SetPixelShader      ( 0 );

    // Reset the device. Clear vertex buffer stream sources set above.
    m_pd3dDevice->SetStreamSource( 0, 0, 0, 0 );
    m_pd3dDevice->SetStreamSource( 1, 0, 0, 0 );

    // Reset the device: Clear the index buffer source set above.
    m_pd3dDevice->SetIndices( 0 );

    // Return success.
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: PostRender()
// Desc: Complete the normal rendering pass.
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::PostRender( )
{
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: SetShaderTexturesAndConstants()
// Desc: Complete the normal rendering pass. Removes textures and shader settings
//       from the render state.
//--------------------------------------------------------------------------------------
VOID AvatarRenderer::SetShaderTexturesAndConstants(
    DWORD                           dwModelIndex,
    const XAVATAR_SHADER_INSTANCE*  shaderInstance,
    const XAVATAR_TEXTURE*          modelTextures,
    const DWORD*                    animatedTextureLayers )
{
    assert( modelTextures );
    assert( animatedTextureLayers );

    const XAVATAR_SHADER shader = shaderInstance->Shader;

    for ( INT pi = 0; pi < XAVATAR_SHADER_INSTANCE_MAX_PARAMS; ++pi )
    {
        const XAVATAR_SHADER_PARAM& param = shaderInstance->Params[ pi ];

        switch ( param.Type )
        {
            case ( XAVATAR_SHADER_PARAM_TYPE_NONE ):
            {
                assert( param.Usage == XAVATAR_SHADER_PARAM_USAGE_NONE );
                break;
            }

            case ( XAVATAR_SHADER_PARAM_TYPE_VERTEX_CONSTANT ):
            {
                const BYTE reg = GetVertexConstantRegister( &param, shader );
                m_pd3dDevice->SetVertexShaderConstantF( reg, param.Data.Constant.Value, 1 );
                break;
            }

            case ( XAVATAR_SHADER_PARAM_TYPE_PIXEL_CONSTANT ):
            {
                const BYTE reg = GetPixelConstantRegister( &param );
                m_pd3dDevice->SetPixelShaderConstantF( reg, param.Data.Constant.Value, 1 );
                break;
            }

            case ( XAVATAR_SHADER_PARAM_TYPE_TEXTURE ):
            {
                const XAVATAR_TEXTURE& texture = modelTextures[ param.Data.Texture.Index ];

                DWORD layerIndex   = 0; 
                switch ( param.Usage )
                {
                    case ( XAVATAR_SHADER_PARAM_USAGE_TEXTURE_EYEBROW_LEFT  ):
                    {
                        layerIndex = animatedTextureLayers[ XAVATAR_ANIMATED_TEXTURE_EYEBROW_LEFT ];
                        break;
                    }
                    case ( XAVATAR_SHADER_PARAM_USAGE_TEXTURE_EYEBROW_RIGHT ):
                    {
                        layerIndex = animatedTextureLayers[ XAVATAR_ANIMATED_TEXTURE_EYEBROW_RIGHT ];
                        break;
                    }
                    case ( XAVATAR_SHADER_PARAM_USAGE_TEXTURE_EYE_LEFT  ):
                    {
                        layerIndex = animatedTextureLayers[ XAVATAR_ANIMATED_TEXTURE_EYE_LEFT ];
                        break;
                    }
                    case ( XAVATAR_SHADER_PARAM_USAGE_TEXTURE_EYE_RIGHT ):
                    {
                        layerIndex = animatedTextureLayers[ XAVATAR_ANIMATED_TEXTURE_EYE_RIGHT ];
                        break;
                    }
                    case ( XAVATAR_SHADER_PARAM_USAGE_TEXTURE_MOUTH ):
                    {
                        layerIndex = animatedTextureLayers[ XAVATAR_ANIMATED_TEXTURE_MOUTH ];
                        break;
                    }
 
                }
            
                layerIndex = ( layerIndex < texture.LayerCount ? layerIndex : 0 );
                D3DTexture* textureHeader = &m_Textures[ dwModelIndex ][ param.Data.Texture.Index ][ layerIndex ];

                const WORD index = GetShaderTextureIndex( param, shader );
                m_pd3dDevice->SetTexture( index, textureHeader );
                m_pd3dDevice->SetSamplerState( index, D3DSAMP_ADDRESSU, ( ( param.Data.Texture.Flags & XAVATAR_TEXTURE_FLAGS_WRAP_U ) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP ) );
                m_pd3dDevice->SetSamplerState( index, D3DSAMP_ADDRESSV, ( ( param.Data.Texture.Flags & XAVATAR_TEXTURE_FLAGS_WRAP_V ) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP ) );
                break;
            }

            default:
            {
                // There are no other types
                assert( 0 );
            }
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: GetVertexConstantRegister()
// Desc: Retrieve the constant register for a given vertex shader and parameter usage. 
//--------------------------------------------------------------------------------------
inline BYTE AvatarRenderer::GetVertexConstantRegister(  const XAVATAR_SHADER_PARAM* param,
                                                XAVATAR_SHADER              shader ) const
{
    assert( shader < XAVATAR_SHADER_COUNT );
    assert( param->Type == XAVATAR_SHADER_PARAM_TYPE_VERTEX_CONSTANT );
    assert( param->Usage < XAVATAR_SHADER_PARAM_USAGE_COUNT );
    assert( m_VertexConstantRegisterMap[ param->Usage ][ shader ] != 0xffff );
    return  m_VertexConstantRegisterMap[ param->Usage ][ shader ];
}


//--------------------------------------------------------------------------------------
// Name: GetPixelConstantRegister()
// Desc: Retrieve the texture index or constant start register for a
//       given parameter usage. Note: Pixel constants are uniform 
//       across all shaders.
//--------------------------------------------------------------------------------------
inline BYTE AvatarRenderer::GetPixelConstantRegister( const XAVATAR_SHADER_PARAM* param ) const
{
    assert( param->Type == XAVATAR_SHADER_PARAM_TYPE_PIXEL_CONSTANT );
    assert( param->Usage < XAVATAR_SHADER_PARAM_USAGE_COUNT );
    assert( m_PixelConstantRegisterMap[ param->Usage ] != 0xffff );
    return  m_PixelConstantRegisterMap[ param->Usage ];
}

//--------------------------------------------------------------------------------------
// Name: GetShaderTextureIndex()
// Desc: Retrieve the texture index for a given shader and parameter usage. 
//--------------------------------------------------------------------------------------
inline WORD AvatarRenderer::GetShaderTextureIndex(  const XAVATAR_SHADER_PARAM& param,
                                            XAVATAR_SHADER              shader ) const
{
    assert( shader < XAVATAR_SHADER_COUNT );
    assert( param.Type == XAVATAR_SHADER_PARAM_TYPE_TEXTURE );
    assert( param.Usage < XAVATAR_SHADER_PARAM_USAGE_COUNT );

    if ( param.Data.Texture.UvIndex != XAVATAR_INVALID_UV_INDEX )
    {
        assert( m_ShaderTextureIndexMap[ param.Usage ][ shader ] == 0xff );
        return param.Data.Texture.UvIndex;
    }
    else
    {
        assert( m_ShaderTextureIndexMap[ param.Usage ][ shader ] != 0xff );
        return  m_ShaderTextureIndexMap[ param.Usage ][ shader ];
    }
}


//--------------------------------------------------------------------------------------
// Name: ClearShaderTexturesAndConstants()
// Desc: Remove textures and shader settings from the render state.
//--------------------------------------------------------------------------------------
HRESULT AvatarRenderer::ClearShaderTexturesAndConstants( )
{
    m_pd3dDevice->SetTexture( 0, 0 );
    m_pd3dDevice->SetTexture( 1, 0 );
    m_pd3dDevice->SetTexture( 2, 0 );
    m_pd3dDevice->SetTexture( 3, 0 );
    m_pd3dDevice->SetTexture( 4, 0 );
    m_pd3dDevice->SetTexture( 5, 0 );

    return S_OK;
}

} // namespace ATG


