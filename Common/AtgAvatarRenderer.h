//--------------------------------------------------------------------------------------
// AvatarRenderer.h
//
// A class for rendering an Xbox avatar.  This class is similar to the IXAvatarRenderer
// interface provided in the XDK
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <xtl.h>
#include <xboxmath.h>
#include <algorithm>
#include <xavatar.h>
#include "AtgUtil.h"

namespace ATG
{
	#define GPU_MEMORY_BUFFER_ALIGN 4096        // 4k alignment for textures
	#define SAMPLE_ALLOCATOR_ID 0

	static const DWORD JOINT_BUFFER_COUNT = 2;  // double buffered to allow skinning using vfetch

	class AvatarRenderer
	{
	public:
		AvatarRenderer( D3DDevice* pDevice, XAVATAR_METADATA& metadata ) :  m_pAssets(NULL),
											   m_pGpuBuffer(NULL)
		{
			m_pd3dDevice = pDevice;
			CreateAvatar( metadata );
		}
		~AvatarRenderer()
		{
			if( m_pAssets )
			{
				XMemFree( m_pAssets, MAKE_XALLOC_ATTRIBUTES( 0, 
															 TRUE, 
															 TRUE, 
															 FALSE, 
															 SAMPLE_ALLOCATOR_ID, 
															 XALLOC_ALIGNMENT_4, 
															 XALLOC_MEMPROTECT_READWRITE, 
															 FALSE, XALLOC_MEMTYPE_HEAP ) );

				m_pAssets = NULL;
			}

			if( m_pGpuBuffer )
			{
				XPhysicalFree( m_pGpuBuffer );
				m_pGpuBuffer = NULL;
			}

			if( m_pJointBuffer )
			{
				XPhysicalFree( m_pJointBuffer );
				m_pJointBuffer = NULL;
			}

			// Remove appropriate references to each declaration and shader.
			for ( INT i = 0; i < ARRAYSIZE( m_D3dShaders ); ++i )
			{
				// Remove the reference to this vertex declarations, shaders and pixel shaders if we have them 
				SAFE_RELEASE( m_D3dShaders[ i ].m_VertexDeclaration );
				SAFE_RELEASE( m_D3dShaders[ i ].m_VertexShader );
				SAFE_RELEASE( m_D3dShaders[ i ].m_PixelShader );
			}
		}
		VOID Render( XMMATRIX matWorld, XMMATRIX matView, XMMATRIX matProj );
		VOID Update( )
		{
			RebuildJoints();
		}
		VOID SetJoints( XAVATAR_SKELETON_POSE_JOINT joints[] )
		{
			for( DWORD i = 0; i < XAVATAR_MAX_SKELETON_JOINTS; i++ )
			{
				m_AvatarJointPose[i] = joints[i];
			}
		}

		XAVATAR_SKELETON* GetSkeleton(){ return m_pAssets->pSkeleton;}

	private:
		HRESULT         CreateAvatar( XAVATAR_METADATA& metaData );
		HRESULT         RebuildJoints( );

		WORD GetShaderTextureIndex( const XAVATAR_SHADER_PARAM& param, XAVATAR_SHADER shader ) const;
		VOID SetShaderTexturesAndConstants( DWORD                           dwModelIndex,
											const XAVATAR_SHADER_INSTANCE*  shaderInstance,
											const XAVATAR_TEXTURE*          modelTextures,
											const DWORD*                    animatedTextureLayers ) ;

		BYTE GetPixelConstantRegister(  const XAVATAR_SHADER_PARAM* param ) const;
		BYTE GetVertexConstantRegister( const XAVATAR_SHADER_PARAM* param,
										XAVATAR_SHADER              shader ) const;


		HRESULT PreRender(          const XMMATRIX*     modelTransform,
									const XMMATRIX*     viewTransform,
									const XMMATRIX*     projectionTransform );
		HRESULT RenderOpaque(       DWORD dwModelIndex, D3DVertexBuffer* pJointBuffer );
		HRESULT RenderTransparent(  DWORD dwModelIndex, D3DVertexBuffer* pJointBuffer );
		HRESULT PostRender( );

		HRESULT ClearShaderTexturesAndConstants( );

	private:
		// Raw avatar asset buffers.  These hold the data retrieved by XAvatarGetAssets()
		XAVATAR_ASSETS*             m_pAssets;            // asset buffer
		BYTE*                       m_pGpuBuffer;         // GPU buffer
		BYTE*                       m_pJointBuffer;

		// Shaders and vertex decls
		struct D3dShaders_t
		{
			D3DVertexDeclaration*   m_VertexDeclaration;
			D3DVertexShader*        m_VertexShader;
			D3DPixelShader*         m_PixelShader;
		};
		D3dShaders_t                m_D3dShaders[ XAVATAR_SHADER_COUNT ];

		// Lookup tables for converting shader parameters to GPU registers
		BYTE                        m_ShaderTextureIndexMap    [ XAVATAR_SHADER_PARAM_USAGE_COUNT ][ XAVATAR_SHADER_COUNT ];
		BYTE                        m_VertexConstantRegisterMap[ XAVATAR_SHADER_PARAM_USAGE_COUNT ][ XAVATAR_SHADER_COUNT ];
		BYTE                        m_PixelConstantRegisterMap [ XAVATAR_SHADER_PARAM_USAGE_COUNT ];

		// Vertex buffers, index buffers, and texture headers for each of the components
		D3DVertexBuffer             m_VertexBuffers[ XAVATAR_COMPONENT_COUNT ];
		D3DIndexBuffer              m_IndexBuffers [ XAVATAR_COMPONENT_COUNT ];
		D3DTexture                  m_Textures     [ XAVATAR_COMPONENT_COUNT ][ XAVATAR_MAX_TEXTURES_PER_MODEL ][ XAVATAR_MAX_LAYERS_PER_TEXTURE ];

		// Animation data - updated every frame from the animation assets
		XAVATAR_SKELETON_POSE_JOINT m_AvatarJointPose[ XAVATAR_MAX_SKELETON_JOINTS    ]; // animated pos/rot/scale for the cur frame
	    
		// Lights
		XMVECTOR                    m_vLightColor[3];
		XMVECTOR                    m_vLightDirection[3];

		// Texture animation data
		DWORD                       m_dwTextureLayers[ XAVATAR_ANIMATED_TEXTURE_COUNT ]; // animated texture IDs for the cur frame

		D3DVertexBuffer             m_JointBufferVBs[ JOINT_BUFFER_COUNT ];  // Joints ready to be consumed by GPU
		DWORD                       m_dwJointBufferIndex;                    // current joint buffer in use

		D3DDevice*                  m_pd3dDevice;
	    
	};

}; // namespace ATG