//-----------------------------------------------------------------------------
// AtgScene.h
//
// describes a scene which can own per-scene materials and animations
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_SCENE_H
#define ATG_SCENE_H

#include <list>
#include "AtgFrame.h"
#include <fxl.h>

namespace ATG
{

class ResourceDatabase;
class EffectGlobalParameterPool;

//-----------------------------------------------------------------------------
// Name: Scene
// Desc: A database containing meshes, materials, and model instances
//-----------------------------------------------------------------------------
class Scene : public Frame
{
    DEFINE_TYPE_INFO();
public:
    Scene();
    ~Scene();

    ResourceDatabase*       GetResourceDatabase() { return m_pResourceDatabase; }

    VOID                    AddObject( NamedTypedObject *pObject ) { m_InstanceDatabase.Add( pObject ); }
    NamedTypedObject*       FindObject( CONST WCHAR* szName ) { return m_InstanceDatabase.Find( szName ); }
    NamedTypedObject*       FindObjectOfType( CONST WCHAR* szName, const StringID TypeID ) { return m_InstanceDatabase.FindTyped( szName, TypeID ); }
    VOID                    RemoveObject( NamedTypedObject *pObject ) { m_InstanceDatabase.Remove( pObject ); }
    FXLEffectPool*          GetEffectParameterPool() { return m_pGlobalParameterPool; }
    NameIndexedCollection*  GetInstanceList() { return &m_InstanceDatabase; }

    VOID                    SetFileName( const CHAR* strFileName ) { strcpy_s( m_strFileName, strFileName ); }
    VOID                    SetMediaRootPath( const CHAR* strMediaRootPath ) { strcpy_s( m_strMediaRootPath,
                                                                                         strMediaRootPath ); }
    const CHAR*             GetFileName() const { return m_strFileName; }
    const CHAR*             GetMediaRootPath() const { return m_strMediaRootPath; }

    VOID                    Render( ::D3DDevice* pd3dDevice, BOOL bSetTextures = TRUE );

private:
    FXLEffectPool* m_pGlobalParameterPool;
    ResourceDatabase* m_pResourceDatabase;
    NameIndexedCollection m_InstanceDatabase;
    CHAR m_strFileName[MAX_PATH];
    CHAR m_strMediaRootPath[MAX_PATH];
};

} // namespace ATG

#endif // ATG_SCENE_H
