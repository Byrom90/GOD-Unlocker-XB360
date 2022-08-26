//--------------------------------------------------------------------------------------
// AtgMesh.cpp
//
// Support code for loading sample geometry. See the header for details.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgMesh.h"
#include "AtgResource.h"
#include "AtgUtil.h"

namespace ATG
{

extern D3DDevice* g_pd3dDevice;


//--------------------------------------------------------------------------------------
// Name: Mesh2()
// Desc: Constructor
//--------------------------------------------------------------------------------------
Mesh2::Mesh2()
{
    m_pAllocatedSysMem = NULL;
    m_pAllocatedVidMem = NULL;
    m_pMeshData = NULL;
}


//--------------------------------------------------------------------------------------
// Name: ~Mesh2()
// Desc: Destructor
//--------------------------------------------------------------------------------------
Mesh2::~Mesh2()
{
    delete[] m_pAllocatedSysMem;
    if( m_pAllocatedVidMem )
        XPhysicalFree( m_pAllocatedVidMem );
}


//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Creates a mesh from an XBG file
//--------------------------------------------------------------------------------------
HRESULT Mesh2::Create( const CHAR* strFilename, const PackedResource* pResource )
{
    // Open the file
    DWORD dwNumBytesRead;
    HANDLE hFile = CreateFile( strFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
    {
        ATG_PrintError( "File not found!\n" );
        return E_FAIL;
    }

    // Read the magic number
    XBG_HEADER xbgHeader;
    if( !ReadFile( hFile, &xbgHeader, sizeof( XBG_HEADER ), &dwNumBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to read file header!\n" );
        return E_FAIL;
    }

    if( xbgHeader.dwMagic != XBG7_FILE_ID )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Invalid XBG file type!\n" );
        return E_FAIL;
    }

    // Read in system memory objects
    DWORD dwSysMemSize = xbgHeader.dwSysMemSize;
    m_pAllocatedSysMem = ( VOID* )new BYTE[dwSysMemSize];

    if( !ReadFile( hFile, m_pAllocatedSysMem, dwSysMemSize, &dwNumBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to read system memory objects!\n" );
        return E_FAIL;
    }

    // Read in video memory objects
    DWORD dwVidMemSize = xbgHeader.dwVidMemSize;
    m_pAllocatedVidMem = ( BYTE* )XPhysicalAllocEx( dwVidMemSize, 0, MAXULONG_PTR,
                                                    D3DVERTEXBUFFER_ALIGNMENT,
                                                    PAGE_READWRITE | PAGE_WRITECOMBINE );

    if( !ReadFile( hFile, m_pAllocatedVidMem, dwVidMemSize, &dwNumBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to read video memory objects!\n" );
        return E_FAIL;
    }

    // Done with the file
    CloseHandle( hFile );

    // Now we need to patch the mesh data. Any pointers read from the file were
    // stored as file offsets. So, we simply need to add a base address to patch
    // things up.
    MESH_FRAME* pFrames = ( MESH_FRAME* )m_pAllocatedSysMem;

    DWORD dwPatchOffset = ( DWORD )m_pAllocatedSysMem - sizeof( XBG_HEADER );

    // Note: for a simple mesh class, we only consider the first frame
    m_pMeshData = ( MESH_DATA* )( ( DWORD )pFrames[0].m_pMeshData + dwPatchOffset );

    // Patch up the GPU addresses
    XGOffsetResourceAddress( &m_pMeshData->m_VB, m_pAllocatedVidMem );
    XGOffsetResourceAddress( &m_pMeshData->m_IB, m_pAllocatedVidMem );

    // Create a vertex declaration for the mesh
    g_pd3dDevice->CreateVertexDeclaration( m_pMeshData->m_VertexElements, &m_pMeshData->m_pVertexDecl );

    // Finally, create any textures used by the meshes' subsets. In this 
    // implementation, we are pulling textures out of the passed in resource.
    if( pResource )
    {
        for( DWORD i = 0; i < m_pMeshData->m_dwNumSubsets; i++ )
        {
            m_pMeshData->m_pSubsets[i].pTexture = pResource->GetTexture( m_pMeshData->m_pSubsets[i].strTexture );
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the mesh geometry
//-------------------------------------------------------------------------------------
VOID MESH_DATA::Render( DWORD dwFlags )
{
    if( m_dwNumVertices == 0 )
        return;

    // Set the vertex stream
    g_pd3dDevice->SetStreamSource( 0, &m_VB, 0, m_dwVertexSize );
    g_pd3dDevice->SetIndices( &m_IB );

    // Set the vertex declaration
    if( 0 == ( dwFlags & MESH_NOVERTEXDECL ) )
        g_pd3dDevice->SetVertexDeclaration( m_pVertexDecl );

    // Render the subsets
    for( DWORD i = 0; i < m_dwNumSubsets; i++ )
    {
        BOOL bRender = FALSE;

        // Render the opaque subsets, unless the user asked us not to
        if( 0 == ( dwFlags & MESH_ALPHAONLY ) )
        {
            if( 0 == ( dwFlags & MESH_NOMATERIALS ) )
            {
                if( m_pSubsets[i].mtrl.Diffuse.a >= 1.0f )
                    bRender = TRUE;
            }
            else
                bRender = TRUE;
        }

        // Render the transparent subsets, unless the user asked us not to
        if( 0 == ( dwFlags & MESH_OPAQUEONLY ) )
        {
            if( 0 == ( dwFlags & MESH_NOMATERIALS ) )
            {
                if( m_pSubsets[i].mtrl.Diffuse.a < 1.0f )
                    bRender = TRUE;
            }
        }

        if( bRender )
        {
            // Set the texture, unless the user asked us not to
            if( 0 == ( dwFlags & MESH_NOTEXTURES ) && m_pSubsets[i].pTexture )
                g_pd3dDevice->SetTexture( 0, m_pSubsets[i].pTexture );

            // Draw the mesh subset
            g_pd3dDevice->DrawIndexedPrimitive( m_dwPrimType, 0, 0, m_dwNumVertices,
                                                m_pSubsets[i].dwIndexStart,
                                                m_pSubsets[i].dwPrimitiveCount );
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: Mesh()
// Desc: Constructor
//--------------------------------------------------------------------------------------
Mesh::Mesh()
{
    m_pAllocatedSysMem = NULL;
    m_pAllocatedVidMem = NULL;
    m_pFrames = NULL;
    m_dwNumFrames = 0;
}


//--------------------------------------------------------------------------------------
// Name: ~Mesh()
// Desc: Destructor
//--------------------------------------------------------------------------------------
Mesh::~Mesh()
{
    delete[] m_pAllocatedSysMem;
    if( m_pAllocatedVidMem )
        XPhysicalFree( m_pAllocatedVidMem );
}


//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Creates a mesh from an XBG file
//--------------------------------------------------------------------------------------
HRESULT Mesh::Create( const CHAR* strFilename, const PackedResource* pResource )
{
    // Open the file
    DWORD dwNumBytesRead;
    HANDLE hFile = CreateFile( strFilename, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
    {
        ATG_PrintError( "File not found!\n" );
        return E_FAIL;
    }

    // Read the magic number
    XBG_HEADER xbgHeader;
    if( !ReadFile( hFile, &xbgHeader, sizeof( XBG_HEADER ), &dwNumBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to read file header!\n" );
        return E_FAIL;
    }

    if( xbgHeader.dwMagic != XBG7_FILE_ID )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Invalid XBG file type!\n" );
        return E_FAIL;
    }

    // Read in system memory objects
    DWORD dwSysMemSize = xbgHeader.dwSysMemSize;
    m_pAllocatedSysMem = ( VOID* )new BYTE[dwSysMemSize];
    if( !m_pAllocatedSysMem )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to allocate system memory!\n" );
        return E_FAIL;
    }

    if( !ReadFile( hFile, m_pAllocatedSysMem, dwSysMemSize, &dwNumBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to read system memory objects!\n" );
        return E_FAIL;
    }

    // Read in video memory objects
    DWORD dwVidMemSize = xbgHeader.dwVidMemSize;
    m_pAllocatedVidMem = ( BYTE* )XPhysicalAllocEx( dwVidMemSize, 0, MAXULONG_PTR,
                                                    D3DVERTEXBUFFER_ALIGNMENT,
                                                    PAGE_READWRITE | PAGE_WRITECOMBINE );

    if( !m_pAllocatedVidMem )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to allocate video memory!\n" );
        return E_FAIL;
    }

    if( !ReadFile( hFile, m_pAllocatedVidMem, dwVidMemSize, &dwNumBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        ATG_PrintError( "Unable to read video memory objects!\n" );
        return E_FAIL;
    }

    // Done with the file
    CloseHandle( hFile );

    // Now we need to patch the mesh data. Any pointers read from the file were
    // stored as file offsets. So, we simply need to add a base address to patch
    // things up.
    DWORD dwPatchOffset = ( DWORD )m_pAllocatedSysMem - sizeof( XBG_HEADER );

    m_pFrames = ( MESH_FRAME* )m_pAllocatedSysMem;
    m_dwNumFrames = xbgHeader.dwNumFrames;

    for( DWORD i = 0; i < m_dwNumFrames; i++ )
    {
        MESH_FRAME* pFrame = &m_pFrames[i];

        // Patch up the frame and meshdata pointers
        if( pFrame->m_pMeshData )
            pFrame->m_pMeshData = ( MESH_DATA* )( ( DWORD )pFrame->m_pMeshData + dwPatchOffset );
        if( pFrame->m_pChild )
            pFrame->m_pChild = ( MESH_FRAME* )( ( DWORD )pFrame->m_pChild + dwPatchOffset );
        if( pFrame->m_pNext )
            pFrame->m_pNext = ( MESH_FRAME* )( ( DWORD )pFrame->m_pNext + dwPatchOffset );

        if( pFrame->m_pMeshData && pFrame->m_pMeshData->m_dwNumSubsets )
        {
            // Patch up the GPU addresses
            XGOffsetResourceAddress( &pFrame->m_pMeshData->m_VB, m_pAllocatedVidMem );
            XGOffsetResourceAddress( &pFrame->m_pMeshData->m_IB, m_pAllocatedVidMem );

            // Create a vertex declaration for the mesh
            g_pd3dDevice->CreateVertexDeclaration( pFrame->m_pMeshData->m_VertexElements,
                                                   &pFrame->m_pMeshData->m_pVertexDecl );

            // Finally, create any textures used by the meshes' subsets. In this 
            // implementation, we are pulling textures out of the passed in resource.
            if( pResource )
            {
                for( DWORD j = 0; j < pFrame->m_pMeshData->m_dwNumSubsets; j++ )
                {
                    MESH_SUBSET* pSubset = &pFrame->m_pMeshData->m_pSubsets[j];
                    pSubset->pTexture = pResource->GetTexture( pSubset->strTexture );
                }
            }
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Write()
// Desc: Writes the mesh to an XBG file
//--------------------------------------------------------------------------------------
HRESULT Mesh::Write( const CHAR* strFilename )
{
    // Open the file
    DWORD dwNumBytesWritten;
    HANDLE hFile = CreateFile( strFilename, FILE_ALL_ACCESS, 0, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
    {
        ATG_PrintError( "File not found!\n" );
        return E_FAIL;
    }

    // Compute storage requirements
    DWORD g_dwFrameSpace = 0;
    DWORD g_dwMeshSpace = 0;
    DWORD g_dwVertexSpace = 0;
    DWORD g_dwIndexSpace = 0;

    for( DWORD i = 0; i < m_dwNumFrames; i++ )
    {
        MESH_FRAME* pFrame = &m_pFrames[i];
        MESH_DATA* pMesh = pFrame->m_pMeshData;

        DWORD dwIndexSize = pMesh->m_IB.Common & D3DINDEXBUFFER_INDEX32 ? sizeof( DWORD ) : sizeof( WORD );

        g_dwFrameSpace += sizeof( MESH_FRAME );
        g_dwMeshSpace += sizeof( MESH_DATA ) - sizeof( MESH_SUBSET );
        g_dwMeshSpace += sizeof( MESH_SUBSET ) * pMesh->m_dwNumSubsets;
        g_dwIndexSpace += ( ( dwIndexSize * pMesh->m_dwNumIndices + ( 4096 - 1 ) ) / 4096 ) * 4096;
        g_dwVertexSpace += ( ( pMesh->m_dwVertexSize * pMesh->m_dwNumVertices + ( 4096 - 1 ) ) / 4096 ) * 4096;
    }
    DWORD dwNumSysMemBytes = g_dwFrameSpace + g_dwMeshSpace;
    DWORD dwNumVidMemBytes = g_dwIndexSpace + g_dwVertexSpace;

    // Read the magic number
    XBG_HEADER xbgHeader;
    xbgHeader.dwMagic = XBG7_FILE_ID;
    xbgHeader.dwSysMemSize = dwNumSysMemBytes;
    xbgHeader.dwVidMemSize = dwNumVidMemBytes;
    xbgHeader.dwNumFrames = m_dwNumFrames;
    WriteFile( hFile, &xbgHeader, sizeof( XBG_HEADER ), &dwNumBytesWritten, NULL );

    DWORD g_dwFramesFileOffset = sizeof( XBG_HEADER );
    DWORD g_dwMeshesFileOffset = g_dwFramesFileOffset + g_dwFrameSpace;
    DWORD g_dwIndicesFileOffset = 0;
    DWORD g_dwVerticesFileOffset = g_dwIndexSpace;

    // Write the frames
    for( DWORD i = 0; i < m_dwNumFrames; i++ )
    {
        MESH_FRAME* pFrame = &m_pFrames[i];
        MESH_DATA* pMesh = pFrame->m_pMeshData;

        // Write out the frame
        MESH_FRAME frame = *pFrame;
        frame.Pad[0] = 0x11111111;

        if( pFrame->m_pChild )
        {
            DWORD dwOffset = ( DWORD )pFrame->m_pChild - ( DWORD )m_pFrames;
            frame.m_pChild = ( MESH_FRAME* )( g_dwFramesFileOffset + dwOffset );
        }
        if( pFrame->m_pNext )
        {
            DWORD dwOffset = ( DWORD )pFrame->m_pNext - ( DWORD )m_pFrames;
            frame.m_pNext = ( MESH_FRAME* )( g_dwFramesFileOffset + dwOffset );
        }

        frame.m_pMeshData = ( MESH_DATA* )g_dwMeshesFileOffset;
        g_dwMeshesFileOffset += ( sizeof( MESH_DATA ) - sizeof( MESH_SUBSET ) ) + sizeof( MESH_SUBSET ) *
            pMesh->m_dwNumSubsets;

        // Write out frame info
        WriteFile( hFile, &frame, sizeof( MESH_FRAME ), &dwNumBytesWritten, NULL );
    }

    // Write the meshes
    for( DWORD i = 0; i < m_dwNumFrames; i++ )
    {
        MESH_FRAME* pFrame = &m_pFrames[i];
        MESH_DATA* pMesh = pFrame->m_pMeshData;

        DWORD dwIndexSize = pMesh->m_IB.Common & D3DINDEXBUFFER_INDEX32 ? sizeof( DWORD ) : sizeof( WORD );
        D3DFORMAT d3dIndexFormat = pMesh->m_IB.Common & D3DINDEXBUFFER_INDEX32 ? D3DFMT_INDEX32 : D3DFMT_INDEX16;

        // Write out the meshdata
        MESH_DATA mesh = *pMesh;

        if( pMesh->m_dwNumVertices )
        {
            XGSetVertexBufferHeader( pMesh->m_dwNumVertices * pMesh->m_dwVertexSize,
                                     0, D3DPOOL_DEFAULT, g_dwVerticesFileOffset,
                                     &mesh.m_VB );
            g_dwVerticesFileOffset += ( ( pMesh->m_dwVertexSize * pMesh->m_dwNumVertices + ( 4096 - 1 ) ) / 4096 ) *
                4096;
        }

        if( mesh.m_dwNumIndices )
        {
            XGSetIndexBufferHeader( pMesh->m_dwNumIndices * dwIndexSize,
                                    0, d3dIndexFormat, D3DPOOL_DEFAULT, g_dwIndicesFileOffset,
                                    &mesh.m_IB );
            g_dwIndicesFileOffset += ( ( pMesh->m_dwNumIndices * dwIndexSize + ( 4096 - 1 ) ) / 4096 ) * 4096;
        }

        mesh.m_pVertexDecl = NULL;

        // Write out mesh info
        WriteFile( hFile, &mesh, sizeof( MESH_DATA ) - sizeof( MESH_SUBSET ), &dwNumBytesWritten, NULL );

        // Write out subset data
        for( DWORD j = 0; j < pMesh->m_dwNumSubsets; j++ )
        {
            MESH_SUBSET subset = pMesh->m_pSubsets[j];
            subset.Pad[0] = 0x11111111;
            subset.pTexture = NULL;

            WriteFile( hFile, &subset, sizeof( MESH_SUBSET ), &dwNumBytesWritten, NULL );
        }
    }

    static BYTE Pad[4096] =
    {
        0
    };

    // Write indices
    for( DWORD i = 0; i < m_dwNumFrames; i++ )
    {
        MESH_FRAME* pFrame = &m_pFrames[i];
        MESH_DATA* pMesh = pFrame->m_pMeshData;

        if( pMesh->m_dwNumIndices )
        {
            VOID* pIndexData;
            DWORD dwIndexSize = pMesh->m_IB.Common & D3DINDEXBUFFER_INDEX32 ? sizeof( DWORD ) : sizeof( WORD );
            DWORD dwSize = pMesh->m_dwNumIndices * dwIndexSize;

            pMesh->m_IB.Lock( 0, 0, &pIndexData, 0 );
            WriteFile( hFile, pIndexData, dwSize, &dwNumBytesWritten, NULL );
            pMesh->m_IB.Unlock();

            // Pad to aligment
            DWORD dwPadSize = ( 4096 - ( dwSize % 4096 ) ) % 4096;
            if( dwPadSize )
                WriteFile( hFile, Pad, dwPadSize, &dwNumBytesWritten, NULL );
        }
    }

    // Write vertices
    for( DWORD i = 0; i < m_dwNumFrames; i++ )
    {
        MESH_FRAME* pFrame = &m_pFrames[i];
        MESH_DATA* pMesh = pFrame->m_pMeshData;

        if( pMesh->m_dwNumVertices )
        {
            VOID* pVertexData;
            DWORD dwSize = pMesh->m_dwNumVertices * pMesh->m_dwVertexSize;

            pMesh->m_VB.Lock( 0, 0, &pVertexData, 0 );
            WriteFile( hFile, pVertexData, dwSize, &dwNumBytesWritten, NULL );
            pMesh->m_VB.Unlock();

            // Pad to aligment
            DWORD dwPadSize = ( 4096 - ( dwSize % 4096 ) ) % 4096;
            if( dwPadSize )
                WriteFile( hFile, Pad, dwPadSize, &dwNumBytesWritten, NULL );
        }
    }

    // Done with the file
    CloseHandle( hFile );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the hierarchy of frames and meshes
//--------------------------------------------------------------------------------------
VOID Mesh::Render( DWORD dwFlags )
{
    if( m_pFrames )
        RenderFrame( m_pFrames, dwFlags );
}


//--------------------------------------------------------------------------------------
// Name: RenderFrame()
// Desc: Renders a frame (save state, apply matrix, render children, restore)
//--------------------------------------------------------------------------------------
VOID Mesh::RenderFrame( const MESH_FRAME* pFrame, DWORD dwFlags )
{
    // Apply the frame's local transform
    XMMATRIX matSavedWorld = m_matWorld;
    m_matWorld = XMMatrixMultiply( pFrame->m_matTransform, m_matWorld );

    // Render the mesh data
    if( pFrame->m_pMeshData->m_dwNumSubsets )
    {
        // Call the callback, so the app can tweak state before rendering the mesh
        DWORD dwFrame = pFrame - m_pFrames;
        RenderMeshCallback( dwFrame, pFrame, dwFlags );

        RenderMesh( pFrame->m_pMeshData, dwFlags );
    }

    // Render any child frames
    if( pFrame->m_pChild )
        RenderFrame( pFrame->m_pChild, dwFlags );

    // Restore the transformation matrix
    m_matWorld = matSavedWorld;

    // Render any sibling frames
    if( pFrame->m_pNext )
        RenderFrame( pFrame->m_pNext, dwFlags );
}


//--------------------------------------------------------------------------------------
// Name: RenderMesh()
// Desc: Renders the mesh geometry
//--------------------------------------------------------------------------------------
VOID Mesh::RenderMesh( MESH_DATA* pMesh, DWORD dwFlags )
{
    D3DVertexBuffer* pVB = &pMesh->m_VB;
    DWORD dwNumVertices = pMesh->m_dwNumVertices;
    D3DIndexBuffer* pIB = &pMesh->m_IB;
    DWORD dwVertexSize = pMesh->m_dwVertexSize;
    D3DPRIMITIVETYPE dwPrimType = pMesh->m_dwPrimType;
    DWORD dwNumSubsets = pMesh->m_dwNumSubsets;
    MESH_SUBSET* pSubsets = &pMesh->m_pSubsets[0];

    if( dwNumVertices == 0 )
        return;

    // Set the vertex stream
    g_pd3dDevice->SetStreamSource( 0, pVB, 0, dwVertexSize );
    g_pd3dDevice->SetIndices( pIB );

    // Set the vertex declaration
    if( 0 == ( dwFlags & MESH_NOVERTEXDECL ) )
    {
        g_pd3dDevice->SetVertexDeclaration( pMesh->m_pVertexDecl );
    }

    // Render the subsets
    for( DWORD i = 0; i < dwNumSubsets; i++ )
    {
        BOOL bRender = FALSE;

        // Render the opaque subsets, unless the user asked us not to
        if( 0 == ( dwFlags & MESH_ALPHAONLY ) )
        {
            if( 0 == ( dwFlags & MESH_NOMATERIALS ) )
            {
                if( pSubsets[i].mtrl.Diffuse.a >= 1.0f )
                    bRender = TRUE;
            }
            else
                bRender = TRUE;
        }

        // Render the transparent subsets, unless the user asked us not to
        if( 0 == ( dwFlags & MESH_OPAQUEONLY ) )
        {
            if( 0 == ( dwFlags & MESH_NOMATERIALS ) )
            {
                if( pSubsets[i].mtrl.Diffuse.a < 1.0f )
                    bRender = TRUE;
            }
        }

        if( bRender )
        {
            // Set the texture, unless the user asked us not to
            if( 0 == ( dwFlags & MESH_NOTEXTURES ) && pSubsets[i].pTexture )
                g_pd3dDevice->SetTexture( 0, pSubsets[i].pTexture );

            // Call the callback, so the app can tweak state before rendering
            // each subset
            BOOL bRenderSubset = RenderCallback( i, &pSubsets[i], dwFlags );

            // Draw the mesh subset
            if( bRenderSubset )
            {
                g_pd3dDevice->DrawIndexedPrimitive( dwPrimType, 0, 0, 0,
                                                    pSubsets[i].dwIndexStart,
                                                    pSubsets[i].dwPrimitiveCount );
            }
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: ComputeRadius()
// Desc: Finds the furthest point from zero on the mesh.
//--------------------------------------------------------------------------------------
FLOAT Mesh::ComputeRadius()
{
    return ComputeFrameRadius( m_pFrames, XMMatrixIdentity() );
}


//--------------------------------------------------------------------------------------
// Name: ComputeFrameRadius()
// Desc: Calls ComputeMeshRadius for each frame with the correct transform
//--------------------------------------------------------------------------------------
FLOAT Mesh::ComputeFrameRadius( const MESH_FRAME* pFrame, const XMMATRIX matParent )
{
    // Apply the frame's local transform
    XMMATRIX matWorld = XMMatrixMultiply( pFrame->m_matTransform, matParent );

    FLOAT fRadius = 0.0f;

    // Compute bounds for the mesh data
    if( pFrame->m_pMeshData->m_dwNumSubsets )
        fRadius = ComputeMeshRadius( pFrame->m_pMeshData, matWorld );

    // Compute bounds for any child frames
    if( pFrame->m_pChild )
    {
        FLOAT fChildRadius = ComputeFrameRadius( pFrame->m_pChild, matWorld );

        if( fChildRadius > fRadius )
            fRadius = fChildRadius;
    }

    // Compute bounds for any sibling frames
    if( pFrame->m_pNext )
    {
        FLOAT fSiblingRadius = ComputeFrameRadius( pFrame->m_pNext, matParent );

        if( fSiblingRadius > fRadius )
            fRadius = fSiblingRadius;
    }

    return fRadius;
}


//--------------------------------------------------------------------------------------
// Name: ComputeMeshRadius()
// Desc: Finds the furthest point from zero on the mesh
//--------------------------------------------------------------------------------------
FLOAT Mesh::ComputeMeshRadius( MESH_DATA* pMesh, const XMMATRIX mat )
{
    DWORD dwNumVertices = pMesh->m_dwNumVertices;
    DWORD dwVertexSize = pMesh->m_dwVertexSize;
    FLOAT fMaxDist2 = 0.0f;

    BYTE* pVertexData;
    pMesh->m_VB.Lock( 0, 0, ( VOID** )&pVertexData, D3DLOCK_READONLY );

    while( dwNumVertices-- )
    {
        XMFLOAT3* pVertex = ( XMFLOAT3* )pVertexData;
        XMVECTOR vPos = XMVectorSet( pVertex->x, pVertex->y, pVertex->z, 0 );
        vPos = XMVector3TransformCoord( vPos, mat );

        FLOAT fDist2 = vPos.x * vPos.x + vPos.y * vPos.y + vPos.z * vPos.z;

        if( fDist2 > fMaxDist2 )
            fMaxDist2 = fDist2;

        pVertexData += dwVertexSize;
    }

    pMesh->m_VB.Unlock();

    return sqrtf( fMaxDist2 );
}


//--------------------------------------------------------------------------------------
// Name: UnionBox()
// Desc: Take the union of two boxes
//--------------------------------------------------------------------------------------
static VOID UnionBox( XMVECTOR& vUnionMin, XMVECTOR& vUnionMax,
                      const XMVECTOR vMin, const XMVECTOR vMax )
{
    vUnionMin = XMVectorMinimize( vUnionMin, vMin );
    vUnionMax = XMVectorMaximize( vUnionMax, vMax );
}


//--------------------------------------------------------------------------------------
// Name: ComputeBoundingBox()
// Desc: Calculates the bounding box of the entire hierarchy.
//--------------------------------------------------------------------------------------
VOID Mesh::ComputeBoundingBox( XMVECTOR& vMin, XMVECTOR& vMax )
{
    ComputeFrameBoundingBox( m_pFrames, XMMatrixIdentity(), vMin, vMax );
}


//--------------------------------------------------------------------------------------
// Name: ComputeFrameBoundingBox()
// Desc: Calls ComputeMeshBoundingBox for each frame with the correct transform.
//--------------------------------------------------------------------------------------
VOID Mesh::ComputeFrameBoundingBox( const MESH_FRAME* pFrame, const XMMATRIX matParent,
                                    XMVECTOR& vMin, XMVECTOR& vMax )
{
    // Initialize bounds to be reset on the first UnionBox
    vMin.x = vMin.y = vMin.z = +FLT_MAX;
    vMax.x = vMax.y = vMax.z = -FLT_MAX;

    // Apply the frame's local transform
    XMMATRIX matWorld = XMMatrixMultiply( pFrame->m_matTransform, matParent );

    // Compute bounds for the mesh data
    if( pFrame->m_pMeshData->m_dwNumSubsets )
    {
        XMVECTOR vMeshMin, vMeshMax;
        ComputeMeshBoundingBox( pFrame->m_pMeshData, matWorld, vMeshMin, vMeshMax );
        UnionBox( vMin, vMax, vMeshMin, vMeshMax );
    }

    // Compute bounds for any child frames
    if( pFrame->m_pChild )
    {
        XMVECTOR vChildMin, vChildMax;
        ComputeFrameBoundingBox( pFrame->m_pChild, matWorld, vChildMin, vChildMax );
        UnionBox( vMin, vMax, vChildMin, vChildMax );
    }

    // Compute bounds for any sibling frames
    if( pFrame->m_pNext )
    {
        XMVECTOR vSiblingMin, vSiblingMax;
        ComputeFrameBoundingBox( pFrame->m_pNext, matParent, vSiblingMin, vSiblingMax );
        UnionBox( vMin, vMax, vSiblingMin, vSiblingMax );
    }
}


//--------------------------------------------------------------------------------------
// Name: ComputeMeshBoundingBox()
// Desc: Calculate the bounding box of the transformed mesh.
//--------------------------------------------------------------------------------------
VOID Mesh::ComputeMeshBoundingBox( MESH_DATA* pMesh, const XMMATRIX mat,
                                   XMVECTOR& vMin, XMVECTOR& vMax )
{
    // Initialize bounds to be reset on the first point
    vMin.x = vMin.y = vMin.z = +FLT_MAX;
    vMax.x = vMax.y = vMax.z = -FLT_MAX;

    DWORD dwNumVertices = pMesh->m_dwNumVertices;
    DWORD dwVertexSize = pMesh->m_dwVertexSize;

    BYTE* pVertexData;
    pMesh->m_VB.Lock( 0, 0, ( VOID** )&pVertexData, D3DLOCK_READONLY );

    while( dwNumVertices-- )
    {
        XMFLOAT3* pVertex = ( XMFLOAT3* )pVertexData;
        XMVECTOR vPos = XMVectorSet( pVertex->x, pVertex->y, pVertex->z, 0 );
        vPos = XMVector3TransformCoord( vPos, mat );

        UnionBox( vMin, vMax, vPos, vPos );  // Expand the bounding box to include the point
        pVertexData += dwVertexSize;
    }
    pMesh->m_VB.Unlock();
}

} // namespace ATG
