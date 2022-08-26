//-----------------------------------------------------------------------------
// Resource.h
//
// Describes the database that manages native static resources
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_RESOURCEDATABASE_H
#define ATG_RESOURCEDATABASE_H

#include <vector>
#include <fxl.h>
#include "AtgNamedTypedObject.h"

namespace ATG
{

//-----------------------------------------------------------------------------
// Name: Resource
// Desc: Base class for static resources
//-----------------------------------------------------------------------------    
class Resource : public NamedTypedObject
{
    DEFINE_TYPE_INFO();
public:
    Resource()
        : bFromPackedResource( FALSE )
    {
    }
    BOOL        bFromPackedResource;
};


//-----------------------------------------------------------------------------
// A vertex stream consists of a single vertex buffer and its divider
//-----------------------------------------------------------------------------
struct VertexStream
{
    DWORD                      Divider;
    DWORD                      Stride;
    LPDIRECT3DVERTEXBUFFER9    pVertexBuffer;
};


//-----------------------------------------------------------------------------
// Name: VertexData
// Desc: VertexData has multiple streams.  The number of vertices in each
//        stream times its divider must equal the number of vertices specified 
//        in the VertexData structure
//-----------------------------------------------------------------------------
class VertexData : public Resource
{
    DEFINE_TYPE_INFO();
public:    
    VertexData();
    ~VertexData();

    static const DWORD MaxStreamCount = 4;

    // adds streams
    VOID                    AddVertexStream( LPDIRECT3DVERTEXBUFFER9 pBuffer, DWORD Stride, DWORD VertexCount, DWORD Divider = 1 );
    
    // accessors
    LPDIRECT3DVERTEXDECLARATION9    GetVertexDecl() CONST { return m_pVertexDecl; }
    VOID                            SetVertexDecl( LPDIRECT3DVERTEXDECLARATION9 pVertexDecl );
    VOID                            AppendVertexDecl( LPDIRECT3DVERTEXDECLARATION9 pVertexDecl, DWORD dwStreamIndex );

    DWORD                           GetNumVertexStreams() CONST { return m_VertexStreams.size(); }        
    CONST VertexStream*             GetVertexStream( DWORD Index ) CONST { return &m_VertexStreams[ Index ]; }    
    DWORD                           GetNumVertices() CONST { return m_NumVertices; }
    const DWORD*                    GetStreamStrides() CONST { return &m_dwStreamStrides[0]; };
    
private:
    DWORD                           m_NumVertices;
    // one decl to describe all of the streams in this structre
    LPDIRECT3DVERTEXDECLARATION9    m_pVertexDecl;
    std::vector< VertexStream > m_VertexStreams;
    DWORD                           m_dwStreamStrides[MaxStreamCount];
};


//-----------------------------------------------------------------------------
// Name: IndexData
//-----------------------------------------------------------------------------
class IndexData : public Resource
{
    DEFINE_TYPE_INFO();
public:    
    IndexData();
    ~IndexData();

    VOID                    SetIndexBuffer( LPDIRECT3DINDEXBUFFER9 pIndexBuffer );
    LPDIRECT3DINDEXBUFFER9  GetIndexBuffer() CONST { return m_pIndexBuffer; }
    
    DWORD                   GetNumIndices() CONST { return m_NumIndices; }
    VOID                    SetNumIndices( DWORD NumIndices ) { m_NumIndices = NumIndices; }
private:
    DWORD                   m_NumIndices;
    LPDIRECT3DINDEXBUFFER9  m_pIndexBuffer;
};


//-----------------------------------------------------------------------------
// Name: SubsetDesc
// Desc: describes the subets in a mesh, if it has any
//-----------------------------------------------------------------------------
class SubsetDesc : public Resource
{
    DEFINE_TYPE_INFO();
public:
    D3DPRIMITIVETYPE         GetPrimitiveType() CONST { return m_PrimitiveType; }
    VOID                     SetPrimitiveType( D3DPRIMITIVETYPE pt ) { m_PrimitiveType = pt; }

    DWORD                    GetStartIndex() CONST { return m_StartIndex; }
    VOID                     SetStartIndex( DWORD si ) { m_StartIndex = si; }
    
    DWORD                    GetNumPrimitives()  CONST { return m_NumPrimitives; }
    VOID                     SetNumPrimitives( DWORD np ) { m_NumPrimitives = np; }

private:
    D3DPRIMITIVETYPE         m_PrimitiveType;
    DWORD                    m_StartIndex;
    DWORD                    m_NumPrimitives;
};


//-----------------------------------------------------------------------------
// Name: Texture
// Desc: encapsulates a texture
//-----------------------------------------------------------------------------
class Texture : public Resource
{
    DEFINE_TYPE_INFO();   
public:
    Texture() { m_pD3DTexture = NULL; }
    ~Texture() { if( m_pD3DTexture != NULL ) m_pD3DTexture->Release(); }

    virtual LPDIRECT3DBASETEXTURE9  GetD3DTexture() { return m_pD3DTexture; };
    virtual VOID                    SetD3DTexture( LPDIRECT3DBASETEXTURE9 pTex ) 
    {
        if( m_pD3DTexture != NULL )
            m_pD3DTexture->Release();
        m_pD3DTexture = pTex; 
        m_pD3DTexture->AddRef(); 
    }

protected:
    LPDIRECT3DBASETEXTURE9 m_pD3DTexture;
};


//-----------------------------------------------------------------------------
// Name: Texture2D
// Desc: encapsulates a 2D texture
//-----------------------------------------------------------------------------
class Texture2D : public Texture
{
    DEFINE_TYPE_INFO();
};


//-----------------------------------------------------------------------------
// Name: TextureVolume
// Desc: encapsulates a volume texture
//-----------------------------------------------------------------------------
class TextureVolume : public Texture
{
    DEFINE_TYPE_INFO();
};


//-----------------------------------------------------------------------------
// Name: TextureCube
// Desc: encapsulates a cube texture
//-----------------------------------------------------------------------------
class TextureCube : public Texture
{
    DEFINE_TYPE_INFO();    
};

class PackedResource;
//-----------------------------------------------------------------------------
// Name: ResourceDatabase
// Desc: Contains non-instance resources
//-----------------------------------------------------------------------------    
class ResourceDatabase
{
public:
    ResourceDatabase();
    ~ResourceDatabase();

    // Add generic resources (bundled file loaders will use these)
    VOID            AddResource( Resource *pResource ) { m_Resources.Add( pResource ); }
    Resource*       FindResource( CONST WCHAR* szName ) { return (Resource*) m_Resources.Find( szName ); }
    Resource*       FindResourceOfType( CONST WCHAR* szName, const StringID TypeID ) { return (Resource*) m_Resources.FindTyped( szName, TypeID ); }
    VOID            AddBundledResources( PackedResource* pBundledResourceTable );

    VOID*           PhysicalAlloc( DWORD dwSize, DWORD dwAlignment, DWORD dwFlags );
    VOID*           VirtualAlloc( DWORD dwSize, DWORD dwFlags );

    // Add Texture resources from a file
    Texture2D*      AddTexture2D( CONST CHAR* strFilename, 
                                    DWORD Width = 0, DWORD Height = 0,
                                    D3DFORMAT Format = D3DFMT_UNKNOWN, DWORD Filter = D3DX_DEFAULT,
                                    DWORD MipLevels = 0, DWORD MipFilter = D3DX_DEFAULT );

    TextureCube*    AddTextureCube( CONST CHAR* strFilename, 
                                    DWORD Size = 0,
                                    D3DFORMAT Format = D3DFMT_UNKNOWN, DWORD Filter = D3DX_DEFAULT,
                                    DWORD MipLevels = 0, DWORD MipFilter = D3DX_DEFAULT );

    TextureVolume*  AddTextureVolume( CONST CHAR* strFilename, 
                                    DWORD Width = 0, DWORD Height = 0, DWORD Depth = 0, 
                                    D3DFORMAT Format = D3DFMT_UNKNOWN, DWORD Filter = D3DX_DEFAULT,
                                    DWORD MipLevels = 0, DWORD MipFilter = D3DX_DEFAULT );    
    
    // Default resources
    VOID            CreateDefaultResources();

    Texture2D*      SetDefaultTexture2D( const WCHAR* strResourceName );
    TextureCube*    SetDefaultTextureCube( const WCHAR* strResourceName );

    Texture2D*      GetDefaultTexture2D() const { return m_pDefaultTexture2D; }
    TextureCube*    GetDefaultTextureCube() const { return m_pDefaultTextureCube; }

    Texture2D*      GetBlackTexture() const { return m_pBlackTexture2D; }
    Texture2D*      CreateBlackTexture();

    Texture2D*      GetBlueTexture() const { return m_pBlueTexture2D; }
    Texture2D*      CreateBlueTexture();

    Texture2D*      GetWhiteTexture() const { return m_pWhiteTexture2D; }
    Texture2D*      CreateWhiteTexture();

private:
    NameIndexedCollection           m_Resources;
    Texture2D*                      m_pDefaultTexture2D;
    Texture2D*                      m_pBlackTexture2D;
    Texture2D*                      m_pBlueTexture2D;
    Texture2D*                      m_pWhiteTexture2D;
    TextureCube*                    m_pDefaultTextureCube;
    std::list< PackedResource* >    m_BundledResources;
    std::list< VOID* >              m_PhysicalAllocations;
    std::list< VOID* >              m_VirtualAllocations;
};

} // namespace ATG

#endif // ATG_RESOURCE_H
