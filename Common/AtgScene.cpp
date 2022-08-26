//-----------------------------------------------------------------------------
// scene.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgMaterials.h"
#include "AtgModel.h"
#include "AtgScene.h"
#include "AtgSceneMesh.h"
#include "AtgResourceDatabase.h"

namespace ATG
{

CONST StringID Scene::TypeID( L"Scene" );

//-----------------------------------------------------------------------------
// Name: Scene::Scene
//-----------------------------------------------------------------------------
Scene::Scene()
{
    m_pResourceDatabase = NULL;
    FXLCreateEffectPool( &m_pGlobalParameterPool );
    m_pResourceDatabase = new ResourceDatabase();
}

Scene::~Scene()
{
    m_pGlobalParameterPool->Release();
    m_pGlobalParameterPool = NULL;
    delete m_pResourceDatabase;
    m_pResourceDatabase = NULL;
}

VOID Scene::Render( ::D3DDevice* pd3dDevice, BOOL bSetTextures )
{
    ATG::NameIndexedCollection::iterator i;

    for( i = GetInstanceList()->begin(); i != GetInstanceList()->end(); i++ )
    {
        // Select models from the object list.
        if( ( *i )->IsDerivedFrom( ATG::Model::TypeID ) )
        {
            ATG::Model* pModel = ( ATG::Model* )( *i );

            // Loop over mesh mappings.
            DWORD dwMeshMappingCount = pModel->GetNumMeshMappings();
            for( DWORD dwMapIndex = 0; dwMapIndex < dwMeshMappingCount; ++dwMapIndex )
            {
                ATG::MeshMapping& mm = pModel->GetMeshMapping( dwMapIndex );
                ATG::BaseMesh* pMesh = mm.pMesh;

                // Loop over mesh subsets.
                DWORD dwSubsetCount = pMesh->GetNumSubsets();
                for( DWORD dwSubsetIndex = 0; dwSubsetIndex < dwSubsetCount; ++dwSubsetIndex )
                {
                    if( bSetTextures )
                    {
                        ATG::MaterialInstance* pMaterial = mm.Materials[ dwSubsetIndex ];

                        for( DWORD j = 0; j < pMaterial->GetRawParameterCount(); ++j )
                        {
                            // Retrieve diffuse texture and normalmaps and set it
                            ATG::MaterialParameter& param = pMaterial->GetRawParameter( j );
                            if( param.pValue != NULL )
                            {
                                ATG::Texture2D* pTex2D = ( ATG::Texture2D* )param.pValue;

                                pd3dDevice->SetTexture( j, pTex2D->GetD3DTexture() );
                            }
                        }
                    }

                    // Render the mesh subset.
                    pMesh->RenderSubset( dwSubsetIndex, pd3dDevice );
                }
            }
        }
    }
}

} // namespace ATG
