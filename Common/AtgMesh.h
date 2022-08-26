//--------------------------------------------------------------------------------------
// AtgMesh.h
//
// Support code for loading geometry stored in .xbg files. These files are typically
// converted from .x geometry files using the MakeXBG tool. See that tool for more
// information.
//
// XBG files were designed to minimize overhead in the loading and rendering process on
// the Xbox. The data in a .xbg file is basically stored in one system memory chunk,
// and one video memory chunk. Therefore, loading a .xbg file is simply two fread()
// calls followed by some patch up (which turns file offsets into real pointers).
//
// Geometry files are loaded into arrays of the following structures. MESH_FRAME
// structures contain data to make a frame hierarchy (such as "next" and "child"
// pointers, plus a transformation matrix). The XMMESH_DATA structure contains data for
// rendering a mesh (such as the vertex buffer, num of indices, etc.). Finally, the
// MESH_SUBSET structure contains subset properties (materials and textures) and
// primitive ranges (start index, index count, etc.) for each subset of the data in the
// MESH_DATA structure.
//
// To use this class, simply instantiate the class, and call Create(). Thereafter, the
// mesh can be rendered with the Render() call. Some render flags are available (see
// below) to limit what gets rendered. For instance, an app might want to render opaque
// subsets only, or use a custom vertex shader. For truly custom control, override the
// Mesh class with a new RenderCallback() function, and put any custom pre-rendering
// code in the callback. The typical use for this is to pass data to a custom vertex
// shader.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGMESH_H
#define ATGMESH_H

namespace ATG
{


// Forware declaration
class PackedResource;


// Rendering flags. Default is no flags (0x00000000)
enum
{
    MESH_OPAQUEONLY     = 0x00000001, // Only render opaque subsets
    MESH_ALPHAONLY      = 0x00000002, // Only render alpha subsets

    MESH_NOMATERIALS    = 0x00000010, // Do not use mesh materials
    MESH_NOTEXTURES     = 0x00000020, // Do not use mesh textures
    MESH_NOFVF          = 0x00000040, // Do not use the FVF for the mesh
    MESH_NOVERTEXDECL   = 0x00000080, // Do not use the vertex decl for the mesh
};


// The magic number to identify .xbg files
const DWORD XBG7_FILE_ID = ( ( ( DWORD )'X' << 24 ) | ( ( ( DWORD )'B' << 16 ) ) | ( ( ( DWORD )'G' << 8 ) ) |
                             ( 7 << 0 ) );


//--------------------------------------------------------------------------------------
// Header for a .xbg file
//--------------------------------------------------------------------------------------
struct XBG_HEADER
{
    DWORD dwMagic;
    DWORD dwSysMemSize;
    DWORD dwVidMemSize;
    DWORD dwNumFrames;
};


//--------------------------------------------------------------------------------------
// Struct to hold data for rendering a mesh
//--------------------------------------------------------------------------------------
struct MESH_SUBSET
{
    D3DMATERIAL9 mtrl;
    LPDIRECT3DTEXTURE9 pTexture;
    CHAR    strTexture[64];
    DWORD dwVertexStart;
    DWORD dwVertexCount;
    DWORD dwIndexStart;
    DWORD dwIndexCount;
    DWORD dwPrimitiveCount;
    DWORD   Pad[1];
};


//--------------------------------------------------------------------------------------
// Struct to hold data for a mesh
//--------------------------------------------------------------------------------------
struct MESH_DATA
{
    D3DVertexBuffer m_VB;            // Mesh geometry
    DWORD m_dwNumVertices;
    D3DIndexBuffer m_IB;
    DWORD m_dwNumIndices;

    D3DVERTEXELEMENT9   m_VertexElements[16];
    D3DVertexDeclaration* m_pVertexDecl;
    DWORD m_dwVertexSize;
    D3DPRIMITIVETYPE m_dwPrimType;

    DWORD m_dwNumSubsets;  // Subset info, for rendering
    MESH_SUBSET         m_pSubsets[1];

    VOID                Render( DWORD dwFlags = 0L );
};


//--------------------------------------------------------------------------------------
// Struct to provide a hierarchial layout of meshes.
//--------------------------------------------------------------------------------------
__declspec( align( 16 ) ) struct MESH_FRAME
{
    CHAR    m_strName[64];
    XMMATRIX m_matTransform;  // Make sure this is 16-byte aligned!
    MESH_DATA* m_pMeshData;
    MESH_FRAME* m_pChild;
    MESH_FRAME* m_pNext;
    DWORD   Pad[1];
};


//--------------------------------------------------------------------------------------
// Name: class Mesh
// Desc: Class for loading geometry files, and rendering the resulting hierarchy of
//       meshes and frames.
//--------------------------------------------------------------------------------------
class Mesh
{
    // Memory allocated during file loading. Ptrs are retained for cleanup.
    VOID* m_pAllocatedSysMem;
    VOID* m_pAllocatedVidMem;

public:
    // Hierarchy (frames and meshes) of loaded geometry
    MESH_FRAME* m_pFrames;
    DWORD m_dwNumFrames;

    // Matrix transform set
    XMMATRIX m_matWorld;
    XMMATRIX m_matView;
    XMMATRIX m_matProj;

                    Mesh();
    virtual         ~Mesh();

    // Creation function. Call this function to create the hierarchy of frames
    // and meshes from a geometry file.
    HRESULT         Create( const CHAR* strFilename, const PackedResource* pResource = NULL );

    // Function to write a mesh back to a file
    HRESULT         Write( const CHAR* strFilename );

    // Access functions
    MESH_FRAME* GetFrame( DWORD i )
    {
        return &m_pFrames[i];
    }
    MESH_DATA* GetMesh( DWORD i )
    {
        return m_pFrames[i].m_pMeshData;
    }

    // Overridable callback function (called before each mesh is rendered). 
    virtual VOID    RenderMeshCallback( DWORD dwFrame, const MESH_FRAME* pFrame, DWORD dwFlags )
    {
    }

    // Overridable callback function (called before each subset is rendered). 
    // This is useful for setting vertex shader constants, etc., before
    // rendering.
    virtual BOOL    RenderCallback( DWORD dwSubset, const MESH_SUBSET* pSubset, DWORD dwFlags )
    {
        return TRUE;
    }

    // Render function. Call this function to render the hierarchy of frames
    // and meshes.
    VOID            Render( DWORD dwFlags = 0x00000000 );

    // Function to find the radius of sphere centered at zero enclosing mesh.
    FLOAT           ComputeRadius();

    // Find the bounding box of all the subsets
    VOID            ComputeBoundingBox( XMVECTOR& vMin, XMVECTOR& vMax );

    // Internal rendering functions
    virtual VOID    RenderFrame( const MESH_FRAME* pMesh, DWORD dwFlags );
    virtual VOID    RenderMesh( MESH_DATA* pMesh, DWORD dwFlags );
    // Internal functions to find the radius of sphere centered at zero enclosing mesh.
    FLOAT           ComputeFrameRadius( const MESH_FRAME* pFrame, const XMMATRIX matParentMatrix );
    FLOAT           ComputeMeshRadius( MESH_DATA* pMesh, const XMMATRIX mat );

    // Internal functions to find the bounding box of the mesh.
    VOID            ComputeFrameBoundingBox( const MESH_FRAME* pFrame, const XMMATRIX matParentMatrix, XMVECTOR& vMin,
                                             XMVECTOR& vMax );
    VOID            ComputeMeshBoundingBox( MESH_DATA* pMesh, const XMMATRIX mat, XMVECTOR& vMin, XMVECTOR& vMax );
};


//--------------------------------------------------------------------------------------
// Name: class Mesh2
// Desc: Class for loading geometry files, and rendering the resulting hierarchy of
//       meshes and frames.
//--------------------------------------------------------------------------------------
class Mesh2
{
    // Memory allocated during file loading. Ptrs are retained for cleanup.
    VOID* m_pAllocatedSysMem;
    VOID* m_pAllocatedVidMem;

public:
    // Mesh data
    MESH_DATA* m_pMeshData;

public:
    // Constructor/destructor
            Mesh2();
    virtual ~Mesh2();

    // Creation function. Call this function to create the hierarchy of frames
    // and meshes from a geometry file.
    HRESULT Create( const CHAR* strFilename, const PackedResource* pResource = NULL );

    MESH_DATA* GetMesh() const
    {
        return m_pMeshData;
    }
};

} // namespace ATG

#endif // ATGMESH_H
