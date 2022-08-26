//-------------------------------------------------------------------------------------
//  AtgSceneFileParser.h
//
//  A SAX-based parser to read the ATG scene file format.
//
//  Xbox Advanced Technology Group
//  Copyright (C) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#ifndef ATG_SCENEFILEPARSER_H
#define ATG_SCENEFILEPARSER_H

#include <vector>
#include "AtgXmlParser.h"
#include "AtgSceneAll.h"

namespace ATG
{

enum XATGObjectLoaderType
{
    XATG_NONE = 0,
    XATG_MESH,
    XATG_SKINNEDMESHINFLUENCES,
    XATG_VERTEXBUFFER,
    XATG_INDEXBUFFER,
    XATG_VERTEXDECLELEMENT,
    XATG_INDEXBUFFERSUBSET,
    XATG_FRAME,
    XATG_MODEL,
    XATG_MATERIAL,
    XATG_AMBIENTLIGHT,
    XATG_DIRECTIONALLIGHT,
    XATG_POINTLIGHT,
    XATG_SPOTLIGHT,
    XATG_CAMERA,
    XATG_ANIMATION,
};

struct XMLElementAttribute
{
    WCHAR   strName[100];
    WCHAR   strValue[256];
};
typedef std::vector <XMLElementAttribute> XMLElementAttributeList;

class XMLElementDesc
{
public:
            XMLElementDesc()
            {
                strElementName[0] = L'\0';
                strElementBody[0] = L'\0';
                bEndElement = FALSE;
            }
    WCHAR   strElementName[128];
    WCHAR   strElementBody[1024];
    XMLElementAttributeList Attributes;
    BOOL bEndElement;
};

class Frame;
class ParameterDesc;

class XATGParserContext
{
public:
    XATGParserContext() : CurrentObjectType( XATG_NONE ),
                          pCurrentObject( NULL ),
                          pUserData( NULL ),
                          pCurrentParentFrame( NULL ),
                          pCurrentParentObject( NULL )
    {
    }
    XATGObjectLoaderType CurrentObjectType;
    VOID* pCurrentObject;
    VOID* pUserData;
    DWORD dwUserDataIndex;
    Frame* pCurrentParentFrame;
    VOID* pCurrentParentObject;
    DWORD dwCurrentParameterIndex;
};

class Scene;

enum XATGLoaderFlags
{
    XATGLOADER_DONOTINITIALIZEMATERIALS = 1,
    XATGLOADER_EFFECTSELECTORPARAMETERS = 2,
    XATGLOADER_DONOTBINDTEXTURES        = 4,
};

class SceneFileParser : public ISAXCallback
{
public:
    static HRESULT  PrepareForThreadedLoad( CRITICAL_SECTION* pCriticalSection );
    static HRESULT  LoadXATGFile( const CHAR* strFileName, Scene* pScene, Frame* pRootFrame, DWORD dwFlags = 0,
                                  DWORD* pLoadProgress = NULL );
    static const CHAR* GetParseErrorMessage();

    virtual HRESULT StartDocument()
    {
        return S_OK;
    }
    virtual HRESULT EndDocument();

    virtual HRESULT ElementBegin( const WCHAR* strName, UINT NameLen,
                                  const XMLAttribute* pAttributes, UINT NumAttributes );
    virtual HRESULT ElementContent( const WCHAR* strData, UINT DataLen, BOOL More );
    virtual HRESULT ElementEnd( const WCHAR* strName, UINT NameLen );

    virtual HRESULT CDATABegin()
    {
        return S_OK;
    }
    virtual HRESULT CDATAData( const WCHAR* strCDATA, UINT CDATALen, BOOL bMore )
    {
        return S_OK;
    }
    virtual HRESULT CDATAEnd()
    {
        return S_OK;
    }
    virtual VOID    SetParseProgress( DWORD dwProgress );

    virtual VOID    Error( HRESULT hError, const CHAR* strMessage );

protected:
    XATGParserContext m_Context;
    XMLElementDesc m_CurrentElementDesc;

    VOID            CopyAttributes( const XMLAttribute* pAttributes, UINT uAttributeCount );
    VOID            HandleElementData();
    VOID            HandleElementEnd();
    VOID            DistributeElementToLoaders();

    BOOL            FindAttribute( const WCHAR* strName, WCHAR* strDest, UINT uDestLength );
    const WCHAR* FindAttribute( const WCHAR* strName );
    BOOL            SetObjectNameFromAttribute( NamedTypedObject* pNTO );

    VOID            AcquireDirect3D();
    VOID            ReleaseDirect3D();

    VOID            ProcessRootData();
    VOID            ProcessMeshData();
    VOID            ProcessFrameData();
    VOID            ProcessModelData();
    VOID            ProcessMaterialData();
    VOID            ProcessLightData();
    VOID            ProcessCameraData();
    VOID            ProcessAnimationData();
    VOID            CrackVertex( BYTE* pDest, const D3DVERTEXELEMENT9* pVertexElements, DWORD dwStreamIndex );

    static VOID     CollapseSceneFrames( Frame* pFrame );
};

} // namespace ATG

#endif
