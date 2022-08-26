//-----------------------------------------------------------------------------
// AtgSceneMesh.h
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_MESH_H
#define ATG_MESH_H

#include <vector>
#include "AtgNamedTypedObject.h"

namespace ATG
{

class VertexData;
class IndexData;
class SubsetDesc;

class BaseMesh : public NamedTypedObject
{
    DEFINE_TYPE_INFO();
public:
    enum RenderSubsetFlags
    {
        NoVertexDecl = 1,
        ZeroStreamStrides = 2,
    };

    enum MeshFlags
    {        
        NoFlags =                0x0,      
        IsSkinnable =            0x1,
        IsMorphable =            0x2,
        IsRenderable =           0x4,
        IsStencilShadowCasting = 0x8,
        IsShadowBufferCasting =  0x10,
        IsSortable =             0x20,        
        IsEnabled =              0x40,      
     
        ForceDWORD =           0x7fffffff
    };

    BaseMesh() : m_dwFlags( BaseMesh::NoFlags )        {}
    
    // IsSkinnable
    virtual VOID    Skin( XMMATRIX* pPallet )      {}
    
    // IsMorphable
    virtual VOID    Morph( FLOAT* pWeights )       {}
    
    // IsRenderable
    virtual UINT            GetNumSubsets() CONST { return 0; }
    virtual UINT            RenderSubset( UINT Index, ::D3DDevice* pd3dDevice = NULL, DWORD dwFlags = 0 ) CONST { return 0; }
    virtual SubsetDesc*     GetSubsetDesc( UINT Index ) CONST { return NULL; }
    virtual VOID            AddSubsetDesc( SubsetDesc *pSD ) {}
    virtual VOID            SetSubsetDesc( UINT Index ) {}
    virtual BOOL            HasVertexElementType( UINT Index, D3DDECLUSAGE Usage ) CONST { return FALSE; }
    
    // IsStencilShadowCasting
    virtual UINT            RenderStencilShadow( CONST XMVECTOR& LightPos, FLOAT ExtensionAmmount ) CONST { return 0; }
    
    // IsShadowBufferCasting
    virtual UINT            RenderShadowBuffer() CONST { return 0; }
    
    // IsSortable
    virtual VOID            Sort( CONST XMVECTOR& Plane ) { return; }

    // Number of primitives and vertices, for debug output
    virtual UINT            GetNumVertices() CONST { return 0; }
    virtual UINT            GetNumPrimitives() CONST { return 0; }

    // get vertex data from mesh
    virtual UINT            GetNumVertexDatas() CONST { return 0; }
    virtual VertexData*     GetVertexData( UINT Index ) CONST { return NULL; }
    virtual VOID            SetVertexData( UINT Index, VertexData* pData ) {}
    
    // Get index data from mesh
    virtual UINT            GetNumIndexDatas() CONST { return 0; }
    virtual IndexData*      GetIndexData( UINT Index ) CONST { return NULL; }
    virtual VOID            SetIndexData( UINT Index, IndexData* pData ) {}

    // Flags
    DWORD                   GetFlags()                      { return m_dwFlags; }
    VOID                    ClearFlag( MeshFlags dwFlag )   { m_dwFlags &= ~dwFlag; }    
    VOID                    SetFlag( MeshFlags dwFlag )     { m_dwFlags |= dwFlag; }

protected:    
    DWORD                   m_dwFlags;
};



//-----------------------------------------------------------------------------
// Represents a static, non-skinned mesh
//-----------------------------------------------------------------------------

class StaticMesh : public BaseMesh
{
    DEFINE_TYPE_INFO();
public:    
    StaticMesh();

    virtual UINT                    GetNumVertices() CONST;
    virtual UINT                    GetNumPrimitives() CONST;
    
    virtual UINT                    GetNumSubsets() CONST { return m_Subsets.size(); };  
    virtual SubsetDesc*         GetSubsetDesc( UINT Index ) CONST { return m_Subsets[ Index ]; }
    virtual VOID                    SetSubsetDesc( UINT Index, SubsetDesc* pDesc ) { m_Subsets[ Index ] = pDesc; }
    virtual VOID                    AddSubsetDesc( SubsetDesc *pSD ) { m_Subsets.push_back( pSD ); }
    virtual UINT                    RenderSubset( UINT Index, ::D3DDevice* pd3dDevice = NULL, DWORD dwFlags = 0 ) CONST;
    virtual BOOL                    HasVertexElementType( UINT Index, D3DDECLUSAGE Usage ) CONST;
    

    virtual UINT                    GetNumVertexDatas() CONST { return 1; };
    virtual VertexData*         GetVertexData( UINT Index ) CONST { return m_pVertexData; };
    virtual VOID                    SetVertexData( UINT Index, VertexData* pData ) { m_pVertexData = pData; }

    virtual UINT                    GetNumIndexDatas() CONST { return 1; };
    virtual IndexData*          GetIndexData( UINT Index ) CONST { return m_pIndexData; }
    virtual VOID                    SetIndexData( UINT Index, IndexData* pData ) { m_pIndexData = pData; }

protected:
    std::vector<SubsetDesc*>    m_Subsets;
    VertexData*                 m_pVertexData;
    IndexData*                  m_pIndexData;    
};

class SkinnedMesh : public StaticMesh
{
    DEFINE_TYPE_INFO();
public:
    SkinnedMesh();

    VOID AddInfluence( StringID strInfluenceName ) { m_InfluenceNames.push_back( strInfluenceName ); }
    DWORD GetInfluenceCount() const { return (DWORD)m_InfluenceNames.size(); }
    const StringID GetInfluence( DWORD dwIndex ) const { return m_InfluenceNames[dwIndex]; }

protected:
    std::vector<StringID>       m_InfluenceNames;
};

class CatmullClarkMesh : public SkinnedMesh
{
    DEFINE_TYPE_INFO();
public:
    CatmullClarkMesh();
};

} // namespace ATG

#endif // ATG_MESH_H
