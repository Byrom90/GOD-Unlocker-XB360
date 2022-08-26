//-----------------------------------------------------------------------------
// mesh.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgSceneMesh.h"
#include "AtgResourceDatabase.h"

namespace ATG
{

extern D3DDevice* g_pd3dDevice;

CONST StringID BaseMesh::TypeID( L"Mesh" );
CONST StringID StaticMesh::TypeID( L"StaticMesh" );
CONST StringID SkinnedMesh::TypeID( L"SkinnedMesh" );
const StringID CatmullClarkMesh::TypeID( L"CatmullClarkMesh" );

//-----------------------------------------------------------------------------
// Name: StaticMesh::StaticMesh()
//-----------------------------------------------------------------------------
StaticMesh::StaticMesh()
{
    m_pVertexData = NULL;
    m_pIndexData = NULL;
    m_dwFlags = BaseMesh::IsRenderable;
}


//-----------------------------------------------------------------------------
// Name: StaticMesh::GetNumVertices()
//-----------------------------------------------------------------------------
UINT StaticMesh::GetNumVertices() CONST
{    
    assert( m_pVertexData );
    return m_pVertexData->GetNumVertices();
}


//-----------------------------------------------------------------------------
// Name: StaticMesh::GetNumPrimitives()
//-----------------------------------------------------------------------------
UINT StaticMesh::GetNumPrimitives() CONST
{
    UINT NumPrims = 0;
    for( UINT i = 0; i < m_Subsets.size(); i++ )
        NumPrims += m_Subsets[i]->GetNumPrimitives();

    return NumPrims;
}


//-----------------------------------------------------------------------------
// Name: StaticMesh::HasVertexElementType()
//-----------------------------------------------------------------------------
BOOL StaticMesh::HasVertexElementType( UINT Index, D3DDECLUSAGE Usage ) CONST
{    
    static D3DVERTEXELEMENT9 VertexElements[128];
    UINT NumElements = 128;

    assert( Index == 0 );

    // $ERRORREPORT
    BOOL bResult = FAILED( m_pVertexData->GetVertexDecl()->GetDeclaration( VertexElements, &NumElements ) );
    assert( !bResult );
    // get rid of warning in release build
    bResult = ( NumElements == 0 );

    UINT DeclSize = D3DXGetDeclLength( VertexElements );
    for( UINT i = 0; i < DeclSize; i++ )
    {
        if( VertexElements[i].Usage == Usage )
            return TRUE;
    }

    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: StaticMesh::RenderSubset()
//-----------------------------------------------------------------------------
UINT StaticMesh::RenderSubset( UINT SubsetIndex, ::D3DDevice* pd3dDevice, DWORD dwFlags ) CONST
{
    if( pd3dDevice == NULL )
        pd3dDevice = g_pd3dDevice;
    assert( pd3dDevice != NULL );

    SubsetDesc* pDesc = m_Subsets[ SubsetIndex ];

    assert( m_pVertexData );
    assert( m_pIndexData );
    assert( pDesc );
    BOOL bResult = FALSE;

    LPDIRECT3DVERTEXDECLARATION9 pVertexDecl = m_pVertexData->GetVertexDecl();

    // streams
    for( UINT i = 0; i < m_pVertexData->GetNumVertexStreams(); i++ )
    {
        UINT Stride = m_pVertexData->GetVertexStream( i )->Stride;

        if( dwFlags & BaseMesh::ZeroStreamStrides )
            Stride = 0;

        // $ERRORREPORT
        bResult = FAILED( pd3dDevice->SetStreamSource( i,
                                                       m_pVertexData->GetVertexStream( i )->pVertexBuffer,
                                                       0,
                                                       Stride ) );
        assert( !bResult );
    }

    // $ERRORREPORT
    if( !( dwFlags & BaseMesh::NoVertexDecl ) )
    {
        bResult = FAILED( pd3dDevice->SetVertexDeclaration( pVertexDecl ) );
        assert( !bResult );
    }

    // $ERRORREPORT
    bResult = FAILED( pd3dDevice->SetIndices( m_pIndexData->GetIndexBuffer() ) );
    assert( !bResult );

    // $ERRORREPORT
    bResult = FAILED( pd3dDevice->DrawIndexedPrimitive( pDesc->GetPrimitiveType(), 0, 0,
                                                        m_pVertexData->GetNumVertices(),
                                                        pDesc->GetStartIndex(),
                                                        pDesc->GetNumPrimitives() ) );
    assert( !bResult );

    return pDesc->GetNumPrimitives();
}

SkinnedMesh::SkinnedMesh()
{
    m_pVertexData = NULL;
    m_pIndexData = NULL;
    m_dwFlags = BaseMesh::IsRenderable | BaseMesh::IsSkinnable;
}

CatmullClarkMesh::CatmullClarkMesh()
{
    m_pVertexData = NULL;
    m_pIndexData = NULL;
    m_dwFlags = BaseMesh::IsRenderable | BaseMesh::IsSkinnable;
}

} // namespace ATG
