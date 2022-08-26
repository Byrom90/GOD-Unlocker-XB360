//-----------------------------------------------------------------------------
// AtgModel.h
//
// A model can has several meshes.  Each mesh may or may not have materials
// assigned to its subsets.  Note that the meshes may be shadow proxies,
// optimized shadows buffer meshes, etc.
//
// $OPTIMIZE: Pool Allocate
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_MODEL_H
#define ATG_MODEL_H

#include <vector>
#include "AtgFrame.h"
#include "AtgBound.h"

namespace ATG
{

class MaterialInstance;
class BaseMesh;

//-----------------------------------------------------------------------------
// Name: MeshMapping
// Desc: A model can contain several meshes.  Each mesh has materials assigned to it.
//-----------------------------------------------------------------------------
struct MeshMapping
{
    enum MeshMappingFlags
    {
        NoFlags             = 0,
        IsShadowCaster      = 0x1,
        IsShadowReceiver    = 0x2,
        IsTransparent       = 0x4,
        ForceDWORD          = 0x7fffffff
    };

    BaseMesh* pMesh;
    DWORD dwFlags;
    std::vector <MaterialInstance*> Materials;
};


//-----------------------------------------------------------------------------
// Name: Model
//-----------------------------------------------------------------------------
class Model : public Frame
{
    DEFINE_TYPE_INFO();    
public:  
    // Adds meshes and materials to the model.
    VOID    AddMesh( BaseMesh* pMesh, DWORD dwMeshMappingFlags );
    VOID    AddMaterial( UINT MeshIndex, UINT MaterialIndex, MaterialInstance* pMaterial );

    // Gets the union of all the mesh flags on this model (MeshFlags enum in the Mesh class )
    DWORD                       GetMeshFlagUnion() CONST;

    // Gets the union of all the mesh instance flags (MeshMappingFlags enum above)
    DWORD                       GetMeshMappingFlagUnion() CONST;
       
    // Get the mesh instances (mesh instance = mesh + assigned materials)
    UINT                        GetNumMeshMappings() CONST { return m_MeshMappings.size(); }         
    MeshMapping&                GetMeshMapping( UINT Index ) { return m_MeshMappings[Index]; }
    BaseMesh*                   GetMesh( CONST WCHAR* strName ) CONST;
    BOOL                        RemoveMesh( CONST BaseMesh* pMesh );
    BOOL    ContainsTransparentSubsets() const;
    BOOL    ContainsOpaqueSubsets() const;

    // Get the light groups this model is in.  In order to be lit by a light, the 
    // model and light must be in the same group and their bounds must be intersecting.
    std::vector <StringID>& GetLightGroups()
    {
        return m_LightGroups;
    }

private:
    std::vector <StringID> m_LightGroups;
    std::vector <MeshMapping> m_MeshMappings;
};

} // namespace ATG

#endif // ATG_MODEL_H
