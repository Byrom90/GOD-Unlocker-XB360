//-----------------------------------------------------------------------------
// model.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgSceneMesh.h"
#include "AtgMaterials.h"
#include "AtgModel.h"

namespace ATG
{

CONST StringID Model::TypeID( L"Model" );
    
//-----------------------------------------------------------------------------
// Name: Model::AddMesh()
//-----------------------------------------------------------------------------
VOID Model::AddMesh( BaseMesh* pMesh, DWORD dwMeshMappingFlags )
{
    assert( pMesh );
    MeshMapping mi;

    mi.pMesh = pMesh;
    mi.dwFlags = dwMeshMappingFlags;
    m_MeshMappings.push_back( mi );
}


//-----------------------------------------------------------------------------
// Name: Model::GetMesh()
//-----------------------------------------------------------------------------
BaseMesh* Model::GetMesh( CONST WCHAR* strName ) CONST
{
    for( UINT i = 0; i < m_MeshMappings.size(); i++ )
    {
        if( m_MeshMappings[i].pMesh->GetName() == strName )
            return m_MeshMappings[i].pMesh;
    }
    return NULL;
}


//-----------------------------------------------------------------------------
// Name: Model::RemoveMesh()
//-----------------------------------------------------------------------------
BOOL Model::RemoveMesh( CONST BaseMesh* pMesh )
{ 
    std::vector<MeshMapping>::iterator i;
    for( i = m_MeshMappings.begin(); i != m_MeshMappings.end(); i++ )
    {
        if( i->pMesh == pMesh )
        {
            m_MeshMappings.erase(i);
            return TRUE;
        }
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Model::AddMaterial()
//-----------------------------------------------------------------------------
VOID Model::AddMaterial( UINT MeshIndex, UINT MaterialIndex, MaterialInstance* pMaterial )
{
    // assert that the mesh index is valid
    assert( MeshIndex < m_MeshMappings.size() );

    // assign the material
    // $TODO: maybe do this a better way, if things are added out-of-order
    if( MaterialIndex == m_MeshMappings[MeshIndex].Materials.size() )
    {
        m_MeshMappings[MeshIndex].Materials.push_back( pMaterial );
    }
    else
    {
        m_MeshMappings[MeshIndex].Materials[MaterialIndex] = pMaterial;
    }
};

//-----------------------------------------------------------------------------
// Name: Model::GetMeshFlagUnion()
//-----------------------------------------------------------------------------
DWORD Model::GetMeshFlagUnion() CONST 
{
    DWORD dwFlagUnion = 0;
    for( UINT i = 0; i < m_MeshMappings.size(); i++ )
        dwFlagUnion |= m_MeshMappings[i].pMesh->GetFlags();
    return dwFlagUnion;
}


//-----------------------------------------------------------------------------
// Name: Model::GetMeshMappingFlagUnion()
//-----------------------------------------------------------------------------
DWORD Model::GetMeshMappingFlagUnion() CONST 
{
    DWORD dwFlagUnion = 0;
    for( UINT i = 0; i < m_MeshMappings.size(); i++ )
        dwFlagUnion |= m_MeshMappings[i].dwFlags;
    return dwFlagUnion;
}


BOOL Model::ContainsOpaqueSubsets() const
{
    DWORD dwMeshMappingCount = ( DWORD )m_MeshMappings.size();
    for( DWORD i = 0; i < dwMeshMappingCount; ++i )
    {
        const MeshMapping& mm = m_MeshMappings[i];
        DWORD dwMaterialCount = ( DWORD )mm.Materials.size();
        for( DWORD j = 0; j < dwMaterialCount; ++j )
        {
            MaterialInstance* pMaterial = mm.Materials[j];
            if( !pMaterial->IsTransparent() )
                return TRUE;
        }
    }
    return FALSE;
}


BOOL Model::ContainsTransparentSubsets() const
{
    DWORD dwMeshMappingCount = ( DWORD )m_MeshMappings.size();
    for( DWORD i = 0; i < dwMeshMappingCount; ++i )
    {
        const MeshMapping& mm = m_MeshMappings[i];
        DWORD dwMaterialCount = ( DWORD )mm.Materials.size();
        for( DWORD j = 0; j < dwMaterialCount; ++j )
        {
            MaterialInstance* pMaterial = mm.Materials[j];
            if( pMaterial->IsTransparent() )
                return TRUE;
        }
    }
    return FALSE;
}

} // namespace ATG
