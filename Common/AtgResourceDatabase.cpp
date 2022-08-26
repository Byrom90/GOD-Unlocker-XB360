//-----------------------------------------------------------------------------
// meshdata.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgResourceDatabase.h"
#include "AtgResource.h"
#include "AtgUtil.h"

namespace ATG
{

extern D3DDevice* g_pd3dDevice;

// Define TypeIDs
CONST StringID Resource::TypeID( L"Resource" );
CONST StringID VertexData::TypeID( L"VertexData" );
CONST StringID IndexData::TypeID( L"IndexData" );
CONST StringID SubsetDesc::TypeID( L"SubsetDesc" );
CONST StringID Texture::TypeID( L"Texture" );
CONST StringID Texture2D::TypeID( L"Texture2D" );
CONST StringID TextureCube::TypeID( L"TextureCube" );
CONST StringID TextureVolume::TypeID( L"TextureVolume" );

ResourceDatabase::ResourceDatabase()
        : m_pDefaultTexture2D( NULL ),
          m_pDefaultTextureCube( NULL ),
          m_pBlackTexture2D( NULL ),
          m_pBlueTexture2D( NULL ),
          m_pWhiteTexture2D( NULL )
{
#ifdef PROFILE
    // Enable the PIX Texture Tracker feature
    PIXEnableTextureTracking(
        0,   // default to hardware thread 5
        0,   // default to 4MB of capture buffer
        NULL // default to no callback
            );
#endif
}

//-----------------------------------------------------------------------------
// Name: VertexData::VertexData()
//-----------------------------------------------------------------------------
VertexData::VertexData()
{
    m_pVertexDecl = NULL;
    m_NumVertices = 0;
    ZeroMemory( m_dwStreamStrides, sizeof( m_dwStreamStrides ) );
}


//-----------------------------------------------------------------------------
// Name: VertexData::~VertexData()
//-----------------------------------------------------------------------------
VertexData::~VertexData()
{
    // release vertex data members
    if( m_pVertexDecl )    
        m_pVertexDecl->Release();
    for( DWORD i = 0; i < m_VertexStreams.size(); i++ )
    {
        if( m_VertexStreams[i].pVertexBuffer )
            m_VertexStreams[i].pVertexBuffer->Release();
    }
}


//-----------------------------------------------------------------------------
// Name: VertexData::AddVertexStream()
//-----------------------------------------------------------------------------
VOID VertexData::AddVertexStream( LPDIRECT3DVERTEXBUFFER9 pBuffer, DWORD Stride, DWORD NumVerts, DWORD Divider )
{
    // vertex buffers are assumed to have the correct bound
    assert( pBuffer );

    DWORD dwStreamIndex = (DWORD)m_VertexStreams.size();
    assert( dwStreamIndex < MaxStreamCount );
    m_dwStreamStrides[dwStreamIndex] = Stride;

    VertexStream Stream;
    Stream.pVertexBuffer = pBuffer;
    Stream.Divider = Divider;
    Stream.Stride = Stride;
    m_VertexStreams.push_back( Stream );
    m_NumVertices += NumVerts;

    pBuffer->AddRef();
}


//-----------------------------------------------------------------------------
// Name: VertexData::SetVertexDecl()
//-----------------------------------------------------------------------------
VOID VertexData::SetVertexDecl( LPDIRECT3DVERTEXDECLARATION9 pVertexDecl )
{
    if( m_pVertexDecl )
    {
        m_pVertexDecl->Release();
    }

    m_pVertexDecl = pVertexDecl;
   
    if( m_pVertexDecl )
    {
        m_pVertexDecl->AddRef();    
    }
}


VOID VertexData::AppendVertexDecl( LPDIRECT3DVERTEXDECLARATION9 pVertexDecl, DWORD dwStreamIndex )
{
    if( m_pVertexDecl == NULL )
    {
        SetVertexDecl( pVertexDecl );
        return;
    }

    DWORD dwExistingCount = MAXD3DDECLLENGTH + 1;
    D3DVERTEXELEMENT9 ExistingElements[MAXD3DDECLLENGTH + 1];
    m_pVertexDecl->GetDeclaration( ExistingElements, (UINT*)&dwExistingCount );

    DWORD dwNewCount = MAXD3DDECLLENGTH + 1;
    D3DVERTEXELEMENT9 NewElements[MAXD3DDECLLENGTH + 1];
    pVertexDecl->GetDeclaration( NewElements, (UINT*)&dwNewCount );

    AppendVertexElements( ExistingElements, dwStreamIndex, NewElements, 0, 0 );

    D3DVertexDeclaration* pNewDecl = D3DDevice_CreateVertexDeclaration( ExistingElements );
    SetVertexDecl( pNewDecl );
}


//-----------------------------------------------------------------------------
// Name: IndexData::IndexData()
//-----------------------------------------------------------------------------
IndexData::IndexData()
{
    m_pIndexBuffer = NULL;
}


//-----------------------------------------------------------------------------
// Name: IndexData::~IndexData()
//-----------------------------------------------------------------------------
IndexData::~IndexData()
{
    if( m_pIndexBuffer )
        m_pIndexBuffer->Release(); // release index buffer    
}


//-----------------------------------------------------------------------------
// Name: IndexData::~IndexData()
//-----------------------------------------------------------------------------
VOID IndexData::SetIndexBuffer( D3DIndexBuffer* pIndexBuffer )
{
    if( m_pIndexBuffer )
        m_pIndexBuffer->Release();
    
    m_pIndexBuffer = pIndexBuffer;    

    if( m_pIndexBuffer )
        m_pIndexBuffer->AddRef();
}


ResourceDatabase::~ResourceDatabase()
{
    m_pDefaultTexture2D = NULL;
    m_pDefaultTextureCube = NULL;

    {
        std::list<VOID*>::iterator iter = m_PhysicalAllocations.begin();
        std::list<VOID*>::iterator end = m_PhysicalAllocations.end();
        while( iter != end )
        {
            XPhysicalFree( *iter );
            ++iter;
        }
        m_PhysicalAllocations.clear();

        iter = m_VirtualAllocations.begin();
        end = m_VirtualAllocations.end();
        while( iter != end )
        {
            delete[] *iter;
            ++iter;
        }
        m_VirtualAllocations.clear();
    }

    {
        std::list<PackedResource*>::iterator iter = m_BundledResources.begin();
        std::list<PackedResource*>::iterator end = m_BundledResources.end();
        while( iter != end )
        {
            delete *iter;
            ++iter;
        }
        m_BundledResources.clear();
    }

    {
        NameIndexedCollection::iterator iter = m_Resources.begin();
        NameIndexedCollection::iterator end = m_Resources.end();
        while( iter != end )
        {
            Resource* pResource = (Resource*)*iter;
            if( pResource->bFromPackedResource == FALSE )
            {
                delete pResource;
            }
            iter++;
        }
    }
}


//-----------------------------------------------------------------------------
// Name: ResourceDatabase::AddTexture2D
//-----------------------------------------------------------------------------
Texture2D* ResourceDatabase::AddTexture2D(  CONST CHAR* strFilename, 
                                            DWORD Width, DWORD Height,
                                            D3DFORMAT Format, DWORD Filter,
                                            DWORD MipLevels, DWORD MipFilter )
{
    WCHAR wszConvertedFilename[ _MAX_PATH ];
    Texture2D *pTexture = NULL;
    Resource *pResource = NULL;

    const CHAR* strFileOnly = strrchr( strFilename, '\\' );
    if( strFileOnly == NULL )
    {
        strFileOnly = strFilename;
    }
    else
    {
        strFileOnly++;
    }
    
    MultiByteToWideChar( CP_ACP, 0, strFileOnly, strlen( strFileOnly ) + 1, wszConvertedFilename, MAX_PATH );
    _wcslwr_s( wszConvertedFilename );

    pResource = FindResource( wszConvertedFilename );
    if ( pResource )
    {
        // $ERROR: report error of duplicate name
        assert( pResource->IsDerivedFrom( Texture2D::TypeID ) );
        return (Texture2D*)pResource;            
    }

    LPDIRECT3DTEXTURE9 pD3DTexture;

    Format = D3DFMT_DXT5;
    MipLevels = 0;

    HRESULT hr = D3DXCreateTextureFromFileEx( g_pd3dDevice, 
                                              strFilename, 
                                              Width, Height, MipLevels,
                                              0,
                                              Format,
                                              D3DPOOL_MANAGED,//D3DPOOL_DEFAULT,
                                              Filter, MipFilter, 0, NULL, NULL, 
                                              &pD3DTexture );
    if ( FAILED( hr ) )
    {
        ATG::DebugSpew( "Could not load 2D texture %s.\n", strFilename );
        return NULL;
    }
    
    pTexture = new Texture2D;
    pTexture->SetName( wszConvertedFilename );
    pTexture->SetD3DTexture( pD3DTexture );
    pD3DTexture->Release();

    AddResource( pTexture );
    return pTexture;
}


//-----------------------------------------------------------------------------
// Name: ResourceDatabase::AddTextureCube
//-----------------------------------------------------------------------------
TextureCube* ResourceDatabase::AddTextureCube(  CONST CHAR* strFilename, 
                                                DWORD Size, D3DFORMAT Format, DWORD Filter,
                                                DWORD MipLevels, DWORD MipFilter )
{
    WCHAR wszConvertedFilename[ _MAX_PATH ];
    TextureCube *pTexture = NULL;
    Resource *pResource = NULL;
    
    const CHAR* strFileOnly = strrchr( strFilename, '\\' );
    if( strFileOnly == NULL )
    {
        strFileOnly = strFilename;
    }
    else
    {
        strFileOnly++;
    }

    MultiByteToWideChar( CP_ACP, 0, strFileOnly, strlen( strFileOnly ) + 1, wszConvertedFilename, MAX_PATH );
    _wcslwr_s( wszConvertedFilename );

    pResource = FindResource( wszConvertedFilename );
    if ( pResource )
    {
        // $ERROR: report error of duplicate name
        assert( pResource->IsDerivedFrom( TextureCube::TypeID ) );
        return (TextureCube*)pResource;            
    }

    LPDIRECT3DCUBETEXTURE9 pD3DTexture;

    HRESULT hr = D3DXCreateCubeTextureFromFileEx( g_pd3dDevice, 
                                              strFilename, 
                                              Size, MipLevels,
                                              0,
                                              Format,
                                              D3DPOOL_MANAGED,//D3DPOOL_DEFAULT,
                                              Filter, MipFilter, 0, NULL, NULL, 
                                              &pD3DTexture );
    if ( FAILED( hr ) )
    {
        ATG::DebugSpew( "Could not load cubemap texture %s.\n", strFilename );
        return NULL;
    }
    
    pTexture = new TextureCube;
    pTexture->SetName( wszConvertedFilename );
    pTexture->SetD3DTexture( pD3DTexture );
    pD3DTexture->Release();
    
    AddResource( pTexture );
    return pTexture;
}


//-----------------------------------------------------------------------------
// Name: ResourceDatabase::AddTextureVolume
//-----------------------------------------------------------------------------
TextureVolume* ResourceDatabase::AddTextureVolume( CONST CHAR* strFilename, 
                                    DWORD Width, DWORD Height, DWORD Depth,
                                    D3DFORMAT Format, DWORD Filter, 
                                    DWORD MipLevels, DWORD MipFilter )
{
    WCHAR wszConvertedFilename[ _MAX_PATH ];
    TextureVolume *pTexture = NULL;
    Resource *pResource = NULL;
    
    const CHAR* strFileOnly = strrchr( strFilename, '\\' );
    if( strFileOnly == NULL )
    {
        strFileOnly = strFilename;
    }
    else
    {
        strFileOnly++;
    }

    MultiByteToWideChar( CP_ACP, 0, strFileOnly, strlen( strFileOnly ) + 1, wszConvertedFilename, MAX_PATH );
    _wcslwr_s( wszConvertedFilename );

    pResource = FindResource( wszConvertedFilename );
    if ( pResource )
    {
        // $ERROR: report error of duplicate name
        assert( pResource->IsDerivedFrom( TextureVolume::TypeID ) );
        return (TextureVolume*)pResource;            
    }

    LPDIRECT3DVOLUMETEXTURE9 pD3DTexture;

    HRESULT hr = D3DXCreateVolumeTextureFromFileEx( g_pd3dDevice, 
                                              strFilename, 
                                              Width, Height, Depth, MipLevels,
                                              0,
                                              Format,
                                              D3DPOOL_MANAGED,//D3DPOOL_DEFAULT,
                                              Filter, MipFilter, 0, NULL, NULL, 
                                              &pD3DTexture );
    if ( FAILED( hr ) )
    {
        ATG::DebugSpew( "Could not load volume texture %s.\n", strFilename );
        return NULL;
    }
    
    pTexture = new TextureVolume;
    pTexture->SetName( wszConvertedFilename );
    pTexture->SetD3DTexture( pD3DTexture );
    pD3DTexture->Release();

    AddResource( pTexture );
    return pTexture;
}


Texture2D* CreateColorTexture( D3DCOLOR Color )
{
    D3DTexture* pTexture = NULL;
    g_pd3dDevice->CreateTexture( 1, 1, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture, NULL );
        
#ifdef PROFILE
    // Report texture to texture tracker
    PIXReportNewTexture( pTexture );
    PIXSetTextureName( pTexture, "::CreateColorTexture" );
#endif

    assert( pTexture != NULL );
    D3DLOCKED_RECT LockRect;
    pTexture->LockRect( 0, &LockRect, NULL, 0 );
    *(DWORD*)LockRect.pBits = Color;
    pTexture->UnlockRect( 0 );

    Texture2D* pTexture2D = new Texture2D();
    pTexture2D->SetD3DTexture( pTexture );
    return pTexture2D;
}


VOID ResourceDatabase::CreateDefaultResources()
{
    if( m_pBlackTexture2D == NULL )
        CreateBlackTexture();
    if( m_pBlueTexture2D == NULL )
        CreateBlueTexture();
    if( m_pWhiteTexture2D == NULL )
        CreateWhiteTexture();

    Texture2D* pDefaultNormalMap = new Texture2D();
    pDefaultNormalMap->SetName( L"default-normalmap.dds" );
    pDefaultNormalMap->SetD3DTexture( m_pBlueTexture2D->GetD3DTexture() );
    AddResource( pDefaultNormalMap );

    Texture2D* pDefaultDiffuse = CreateColorTexture( 0xFF808080 );
    pDefaultDiffuse->SetName( L"default.dds" );
    AddResource( pDefaultDiffuse );

    Texture2D* pDefaultSpecMap = new Texture2D();
    pDefaultSpecMap->SetName( L"default-specularmap.dds" );
    pDefaultSpecMap->SetD3DTexture( m_pWhiteTexture2D->GetD3DTexture() );
    AddResource( pDefaultSpecMap );
}


Texture2D* ResourceDatabase::CreateBlackTexture()
{
    Texture2D* pTexture2D = CreateColorTexture( 0 );
    pTexture2D->SetName( L"black" );

    AddResource( pTexture2D );
    m_pBlackTexture2D = pTexture2D;
    return m_pBlackTexture2D;
}


Texture2D* ResourceDatabase::CreateBlueTexture()
{
    Texture2D* pTexture2D = CreateColorTexture( 0xFF8080FF );
    pTexture2D->SetName( L"blue" );

    AddResource( pTexture2D );
    m_pBlueTexture2D = pTexture2D;
    return m_pBlueTexture2D;
}


Texture2D* ResourceDatabase::CreateWhiteTexture()
{
    Texture2D* pTexture2D = CreateColorTexture( 0xFFFFFFFF );
    pTexture2D->SetName( L"white" );

    AddResource( pTexture2D );
    m_pWhiteTexture2D = pTexture2D;
    return m_pWhiteTexture2D;
}


VOID ResourceDatabase::AddBundledResources( PackedResource* pBundledResourceTable )
{
    assert( pBundledResourceTable != NULL );
    m_BundledResources.push_back( pBundledResourceTable );

    RESOURCE* pTags = NULL;
    DWORD dwResourceCount = 0;
    WCHAR strNameTemp[256];
    pBundledResourceTable->GetResourceTags( &dwResourceCount, &pTags );
    DWORD i = 0;
    while( i < dwResourceCount )
    {
        MultiByteToWideChar( CP_ACP, 0, pTags[i].strName, strlen( pTags[i].strName ) + 1, strNameTemp, 256 );
        _wcslwr_s( strNameTemp );
        switch( pTags[i].dwType )
        {
        case RESOURCETYPE_TEXTURE:
            {
                Texture2D* pTexture = new Texture2D();
                pTexture->SetName( strNameTemp );
                pTexture->bFromPackedResource = TRUE;
                DWORD dwOffset = pTags[ i ].dwOffset;
                D3DTexture* pD3DTexture = pBundledResourceTable->GetTexture( dwOffset );
                pTexture->SetD3DTexture( pD3DTexture );
                AddResource( pTexture );
#ifdef PROFILE
                // Report texture to texture tracker
                PIXReportNewTexture( pD3DTexture );
                CHAR strConverted[256];
                strConverted[0] = 0;
                wcstombs_s( 0, strConverted, strNameTemp, _countof(strConverted) );  
                PIXSetTextureName( pD3DTexture, TEXT(strConverted) );
#endif
                break;
            }
        case RESOURCETYPE_CUBEMAP:
            {
                TextureCube* pTexture = new TextureCube();
                pTexture->SetName( strNameTemp );
                pTexture->bFromPackedResource = TRUE;
                DWORD dwOffset = pTags[ i ].dwOffset;
                D3DCubeTexture* pD3DTexture = pBundledResourceTable->GetCubemap( dwOffset );
                pTexture->SetD3DTexture( pD3DTexture );
                AddResource( pTexture );
#ifdef PROFILE
                // Report texture to texture tracker
                PIXReportNewTexture( pD3DTexture );
                CHAR strConverted[256];
                strConverted[0] = 0;
                wcstombs_s( 0, strConverted, strNameTemp, _countof(strConverted) );  
                PIXSetTextureName( pD3DTexture, TEXT(strConverted) );
#endif
               break;
            }
        case RESOURCETYPE_VOLUMETEXTURE:
            {
                TextureVolume* pTexture = new TextureVolume();
                pTexture->SetName( strNameTemp );
                pTexture->bFromPackedResource = TRUE;
                DWORD dwOffset = pTags[ i ].dwOffset;
                D3DVolumeTexture* pD3DTexture = pBundledResourceTable->GetVolumeTexture( dwOffset );
                pTexture->SetD3DTexture( pD3DTexture );
                AddResource( pTexture );
#ifdef PROFILE
                // Report texture to texture tracker
                PIXReportNewTexture( pD3DTexture );
                CHAR strConverted[256];
                strConverted[0] = 0;
                wcstombs_s( 0, strConverted, strNameTemp, _countof(strConverted) );  
                PIXSetTextureName( pD3DTexture, TEXT(strConverted) );
#endif
              break;
            }
        case RESOURCETYPE_VERTEXBUFFER:
        case RESOURCETYPE_INDEXBUFFER:
        case RESOURCETYPE_USERDATA:
        default:
            {
                // not supported yet
                DebugSpew( "Unsupported resource type %d for resource \"%s\".\n", pTags[i].dwType, pTags[i].strName );
                break;
            }
        }
        ++i;
    }
}

VOID* ResourceDatabase::PhysicalAlloc( DWORD dwSize, DWORD dwAlignment, DWORD dwFlags )
{
    VOID* pBuf = XPhysicalAlloc( dwSize, MAXULONG_PTR, dwAlignment, dwFlags );
    m_PhysicalAllocations.push_back( pBuf );
    return pBuf;
}

VOID* ResourceDatabase::VirtualAlloc( DWORD dwSize, DWORD dwFlags )
{
    VOID* pBuf = new BYTE[ dwSize ];
    m_VirtualAllocations.push_back( pBuf );
    return pBuf;
}

Texture2D* ResourceDatabase::SetDefaultTexture2D( const WCHAR* strResourceName )
{
    m_pDefaultTexture2D = (Texture2D*)FindResource( strResourceName );
    return m_pDefaultTexture2D;
}

TextureCube* ResourceDatabase::SetDefaultTextureCube( const WCHAR* strResourceName )
{
    m_pDefaultTextureCube = (TextureCube*)FindResource( strResourceName );
    return m_pDefaultTextureCube;
}

}
