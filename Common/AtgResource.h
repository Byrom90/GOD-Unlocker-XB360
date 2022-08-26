//--------------------------------------------------------------------------------------
// AtgResource.h
//
// Loads resources from an XPR (Xbox Packed Resource) file.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGRESOURCE_H
#define ATGRESOURCE_H

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name tag for resources. An app may initialize this structure, and pass
// it to the resource's Create() function. From then on, the app may call
// GetResource() to retrieve a resource using an ascii name.
//--------------------------------------------------------------------------------------
struct RESOURCE
{
    DWORD dwType;
    DWORD dwOffset;
    DWORD dwSize;
    CHAR* strName;
};


// Resource types
enum
{
    RESOURCETYPE_USERDATA       = ( ( 'U' << 24 ) | ( 'S' << 16 ) | ( 'E' << 8 ) | ( 'R' ) ),
    RESOURCETYPE_TEXTURE        = ( ( 'T' << 24 ) | ( 'X' << 16 ) | ( '2' << 8 ) | ( 'D' ) ),
    RESOURCETYPE_CUBEMAP        = ( ( 'T' << 24 ) | ( 'X' << 16 ) | ( 'C' << 8 ) | ( 'M' ) ),
    RESOURCETYPE_VOLUMETEXTURE  = ( ( 'T' << 24 ) | ( 'X' << 16 ) | ( '3' << 8 ) | ( 'D' ) ),
    RESOURCETYPE_VERTEXBUFFER   = ( ( 'V' << 24 ) | ( 'B' << 16 ) | ( 'U' << 8 ) | ( 'F' ) ),
    RESOURCETYPE_INDEXBUFFER    = ( ( 'I' << 24 ) | ( 'B' << 16 ) | ( 'U' << 8 ) | ( 'F' ) ),
    RESOURCETYPE_EOF            = 0xffffffff
};


//--------------------------------------------------------------------------------------
// Name: PackedResource
//--------------------------------------------------------------------------------------
class PackedResource
{
protected:
    BYTE* m_pSysMemData;        // Alloc'ed memory for resource headers etc.
    DWORD m_dwSysMemDataSize;

    BYTE* m_pVidMemData;        // Alloc'ed memory for resource data, etc.
    DWORD m_dwVidMemDataSize;

    RESOURCE* m_pResourceTags;      // Tags to associate names with the resources
    DWORD m_dwNumResourceTags;  // Number of resource tags
    BOOL m_bInitialized;       // Resource is fully initialized

public:
    // Loads the resources out of the specified bundle
    HRESULT Create( const CHAR* strFilename );

    VOID    Destroy();

    BOOL    Initialized() const;

    // Retrieves the resource tags
    VOID    GetResourceTags( DWORD* pdwNumResourceTags, RESOURCE** ppResourceTags ) const;

    // Helper function to make sure a resource is registered
    D3DResource* RegisterResource( D3DResource* pResource ) const
    {
        return pResource;
    }

    // Functions to retrieve resources by their offset
    VOID* GetData( DWORD dwOffset ) const
    {
        return &m_pSysMemData[dwOffset];
    }

    D3DResource* GetResource( DWORD dwOffset ) const
    {
        return RegisterResource( ( D3DResource* )GetData( dwOffset ) );
    }

    D3DTexture* GetTexture( DWORD dwOffset ) const
    {
        return ( D3DTexture* )GetResource( dwOffset );
    }

    D3DArrayTexture* GetArrayTexture( DWORD dwOffset ) const
    {
        return ( D3DArrayTexture* )GetResource( dwOffset );
    }

    D3DCubeTexture* GetCubemap( DWORD dwOffset ) const
    {
        return ( D3DCubeTexture* )GetResource( dwOffset );
    }

    D3DVolumeTexture* GetVolumeTexture( DWORD dwOffset ) const
    {
        return ( D3DVolumeTexture* )GetResource( dwOffset );
    }

    D3DVertexBuffer* GetVertexBuffer( DWORD dwOffset ) const
    {
        return ( D3DVertexBuffer* )GetResource( dwOffset );
    }

    // Functions to retrieve resources by their name
    VOID* GetData( const CHAR* strName ) const;

    D3DResource* GetResource( const CHAR* strName ) const
    {
        return RegisterResource( ( D3DResource* )GetData( strName ) );
    }

    D3DTexture* GetTexture( const CHAR* strName ) const
    {
        return ( D3DTexture* )GetResource( strName );
    }

    D3DArrayTexture* GetArrayTexture( const CHAR* strName ) const
    {
        return ( D3DArrayTexture* )GetResource( strName );
    }

    D3DCubeTexture* GetCubemap( const CHAR* strName ) const
    {
        return ( D3DCubeTexture* )GetResource( strName );
    }

    D3DVolumeTexture* GetVolumeTexture( const CHAR* strName ) const
    {
        return ( D3DVolumeTexture* )GetResource( strName );
    }

    D3DVertexBuffer* GetVertexBuffer( const CHAR* strName ) const
    {
        return ( D3DVertexBuffer* )GetResource( strName );
    }

            PackedResource();
            ~PackedResource();
};

} // namespace ATG

#endif // ATGRESOURCE_H
