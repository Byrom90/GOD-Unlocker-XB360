//--------------------------------------------------------------------------------------
// AtgDevice.cpp
//
// Over-loaded device to trap and optimize certain calls to D3D
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include <vector>
#include "AtgDevice.h"
#include "AtgUtil.h"

namespace ATG
{

// Data structure to pair vertex elements with a previously-created vertex decl
struct VTX_DECL
{
    D3DVERTEXELEMENT9 pElements[MAXD3DDECLLENGTH];
    DWORD dwNumElements;
    D3DVertexDeclaration* pDeclaration;
};

//--------------------------------------------------------------------------------------
// Name: CreatePooledVertexDeclaration()
// Desc: Function to coalesce vertex declarations into a shared pool of vertex declarations
//--------------------------------------------------------------------------------------
HRESULT WINAPI CreatePooledVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements,
                                              D3DVertexDeclaration** ppVertexDeclaration )
{
    static std::vector <VTX_DECL> m_VertexDecls;

    // Count the number of vertex elements
    DWORD dwNumElements = 0;
    while( pVertexElements[dwNumElements].Stream < 16 )
        dwNumElements++;

    assert( dwNumElements <= MAXD3DDECLLENGTH );

    // Check for a previously-created vertex decl
    for( unsigned int i = 0; i < m_VertexDecls.size(); i++ )
    {
        if( ( m_VertexDecls[i].dwNumElements == dwNumElements ) &&
            ( !memcmp( m_VertexDecls[i].pElements, pVertexElements, sizeof( D3DVERTEXELEMENT9 ) * dwNumElements ) ) )
        {
            // If found, return it
            ( *ppVertexDeclaration ) = m_VertexDecls[i].pDeclaration;
            return S_OK;
        }
    }

    // No previously-created vertex decl was found, so create one
    ( *ppVertexDeclaration ) = D3DDevice_CreateVertexDeclaration( pVertexElements );

    // And save a record of it
    VTX_DECL d;
    XMemCpy( d.pElements, pVertexElements, sizeof( D3DVERTEXELEMENT9 ) * dwNumElements );
    d.dwNumElements = dwNumElements;
    d.pDeclaration = ( *ppVertexDeclaration );
    d.pDeclaration->AddRef();
    m_VertexDecls.push_back( d );

    return S_OK;
}

} // namespace ATG

#ifdef _XBOX

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: CreateVertexDeclaration()
// Desc: Overloaded function to coalesce vertex declarations
//--------------------------------------------------------------------------------------
HRESULT WINAPI D3DDevice::CreateVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements,
                                                  D3DVertexDeclaration** ppVertexDeclaration )
{
    return CreatePooledVertexDeclaration( pVertexElements, ppVertexDeclaration );
}


//--------------------------------------------------------------------------------------
// Name: SetVertexDeclaration()
// Desc: Overloaded function to avoid redundant calls to D3D
//--------------------------------------------------------------------------------------
D3DVOID WINAPI D3DDevice::SetVertexDeclaration( D3DVertexDeclaration* pDecl )
{
    static D3DVertexDeclaration* g_pDecl = NULL;
    // Synchronize with current D3D device state, to catch subsystems that do not go
    // through this codepath
    D3DVertexDeclaration* pCurrentDecl = D3DDevice_GetVertexDeclaration( this );
    g_pDecl = pCurrentDecl;
    if( pCurrentDecl != NULL )
        pCurrentDecl->Release();
    // Only update decl if it has changed
    if( g_pDecl != pDecl )
    {
        D3DDevice_SetVertexDeclaration( this, pDecl );
        g_pDecl = pDecl;
    }
    D3DVOIDRETURN;
}


//--------------------------------------------------------------------------------------
// Name: SetVertexShader()
// Desc: Overloaded function to avoid redundant calls to D3D
//--------------------------------------------------------------------------------------
D3DVOID WINAPI D3DDevice::SetVertexShader( D3DVertexShader* pShader )
{
    static D3DVertexShader* g_pShader = NULL;
    // Synchronize with current D3D device state, to catch subsystems that do not go
    // through this codepath
    D3DVertexShader* pCurrentVS = NULL;
    D3DDevice_GetVertexShader( this, &pCurrentVS );
    g_pShader = pCurrentVS;
    if( pCurrentVS != NULL )
        pCurrentVS->Release();
    // Only update shader if it has changed
    if( g_pShader != pShader )
    {
        D3DDevice_SetVertexShader( this, pShader );
        g_pShader = pShader;
    }
    D3DVOIDRETURN;
}


//--------------------------------------------------------------------------------------
// Name: SetPixelShader()
// Desc: Overloaded function to avoid redundant calls to D3D
//--------------------------------------------------------------------------------------
D3DVOID WINAPI D3DDevice::SetPixelShader( D3DPixelShader* pShader )
{
    static D3DPixelShader* g_pShader = NULL;
    // Synchronize with current D3D device state, to catch subsystems that do not go
    // through this codepath
    D3DPixelShader* pCurrentPS = NULL;
    D3DDevice_GetPixelShader( this, &pCurrentPS );
    g_pShader = pCurrentPS;
    if( pCurrentPS != NULL )
        pCurrentPS->Release();
    // Only update shader if it has changed
    if( g_pShader != pShader )
    {
        D3DDevice_SetPixelShader( this, pShader );
        g_pShader = pShader;
    }
    D3DVOIDRETURN;
}


} // namespace ATG

#endif // _XBOX
