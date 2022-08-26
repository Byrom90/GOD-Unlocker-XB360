//-----------------------------------------------------------------------------
// AtgSceneFileParser.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
//
// This XML parser is somewhat unique, in that it bundles up the results from each SAX
// callback and distributes a smaller set of callbacks to the actual loader methods
// implemented in ProcessMeshData, ProcessFrameData, etc.
//
// For example, consider this sample XML document:
//
// <A>foobar</A>
//
// A straight SAX callback class would receive 3 callbacks:
// ElementBegin( "A" )
// ElementContent( "foobar" )
// ElementEnd( "A" )
//
// The problem is that the content is disassociated from the tag that it belongs to,
// especially since the ATG SAX parser may deliver multiple ElementContent() callbacks
// if the element content is particularly large.
//
// The ElementBegin(), ElementContent(), and ElementEnd() handlers implemented in
// SceneFileParser cache and accumulate the results into m_CurrentElementDesc, and then
// only call two handlers after that - HandleElementData() and HandleElementEnd().
// The data callback includes the name of the begin tag as well as any content between
// the begin and end tags.
//
// So, the same XML document is presented to the loader code like this:
// HandleElementData() strElementName = "A" strElementBody = "foobar" bEndElement = FALSE
// HandleElementEnd() strElementName = "A" strElementBody = "" bEndElement = TRUE
//
// Not only does this reduce complexity, it fits the XATG file format better because
// most of the data is in a rough key-value format, like <Size>3.0</Size>.
//
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgSceneFileParser.h"
#include "AtgEnumStrings.h"
#include "AtgSceneAll.h"
#include "AtgResource.h"
#include "AtgUtil.h"

namespace ATG
{

#define MATCH_ELEMENT_NAME(x) (_wcsicmp(m_CurrentElementDesc.strElementName,x)==0)
#define MIN_SAFE_VERSION        1.6f

const BOOL g_bDebugXMLParser = FALSE;

extern D3DDevice* g_pd3dDevice;

Scene*              g_pCurrentScene = NULL;
Frame*              g_pRootFrame = NULL;
DWORD               g_dwLoaderFlags = 0;
CHAR                g_strMediaRootPath[MAX_PATH];
DWORD*              g_pLoadProgress = NULL;
BYTE*               g_pBinaryBlobData = NULL;

CRITICAL_SECTION*   g_pD3DCriticalSection = NULL;

CHAR                g_strParseError[256];

HRESULT SceneFileParser::PrepareForThreadedLoad( CRITICAL_SECTION* pCriticalSection )
{
    g_pD3DCriticalSection = pCriticalSection;
    return S_OK;
}

VOID SceneFileParser::AcquireDirect3D()
{
    if( g_pD3DCriticalSection != NULL )
    {
        EnterCriticalSection( g_pD3DCriticalSection );
        g_pd3dDevice->AcquireThreadOwnership();
    }
}

VOID SceneFileParser::ReleaseDirect3D()
{
    if( g_pD3DCriticalSection != NULL )
    {
        g_pd3dDevice->ReleaseThreadOwnership();
        LeaveCriticalSection( g_pD3DCriticalSection );
    }
}

HRESULT SceneFileParser::LoadXATGFile( const CHAR* strFilename, Scene* pScene, Frame* pRootFrame, DWORD dwFlags,
                                       DWORD* pLoadProgress )
{
    XMLParser parser;
    SceneFileParser XATGParser;

    g_strParseError[0] = '\0';

    parser.RegisterSAXCallbackInterface( &XATGParser );
    g_pCurrentScene = pScene;
    g_pBinaryBlobData = NULL;
    g_pRootFrame = ( pRootFrame != NULL ) ? pRootFrame : pScene;
    g_dwLoaderFlags = dwFlags;
    g_pLoadProgress = pLoadProgress;
    XATGParser.m_Context.pCurrentObject = pScene;
    XATGParser.m_Context.pCurrentParentFrame = NULL;
    XATGParser.m_Context.pCurrentParentObject = NULL;

    // extract media root path from scene filename
    strcpy_s( g_strMediaRootPath, strFilename );
    CHAR* pSceneDir = strstr( g_strMediaRootPath, "scenes\\" );
    if( pSceneDir != NULL )
    {
        *pSceneDir = '\0';
    }
    else
    {
        pSceneDir = strrchr( g_strMediaRootPath, '\\' );
        assert( pSceneDir != NULL );
        *pSceneDir = '\0';
    }

    ATG::BaseMaterial::SetMediaRootPath( g_strMediaRootPath );

    HRESULT hr = parser.ParseXMLFile( strFilename );

    if( SUCCEEDED( hr ) )
    {
        pScene->SetFileName( strFilename );
        pScene->SetMediaRootPath( g_strMediaRootPath );
        CollapseSceneFrames( g_pRootFrame );
    }

    g_pLoadProgress = NULL;
    return hr;
}

const CHAR* SceneFileParser::GetParseErrorMessage()
{
    return g_strParseError;
}

VOID SceneFileParser::SetParseProgress( DWORD dwProgress )
{
    if( g_pLoadProgress != NULL )
        *g_pLoadProgress = dwProgress;
}

VOID SceneFileParser::Error( HRESULT hError, const CHAR* strMessage )
{
    OutputDebugString( strMessage );
    OutputDebugString( "\n" );
    strcpy_s( g_strParseError, strMessage );
}

HRESULT SceneFileParser::EndDocument()
{
    if( strlen( g_strParseError ) > 0 )
        return E_FAIL;
    return S_OK;
}


inline BOOL ErrorHasOccurred()
{
    return ( g_strParseError[0] != '\0' );
}


HRESULT SceneFileParser::ElementBegin( const WCHAR* strName, UINT NameLen, const XMLAttribute* pAttributes,
                                       UINT NumAttributes )
{
    // Check if an error has been encountered in scene parsing.
    if( ErrorHasOccurred() )
        return E_FAIL;

    // Distribute an accumulated begin+content package if one exists.
    HandleElementData();

    // Start a new begin+content package.
    // Copy the begin tag name to the current element desc.
    wcsncpy_s( m_CurrentElementDesc.strElementName, strName, NameLen );
    // Clear out the accumulated element body.
    m_CurrentElementDesc.strElementBody[0] = L'\0';
    // Copy all attributes from the begin tag into the current element desc.
    CopyAttributes( pAttributes, NumAttributes );
    return S_OK;
}

HRESULT SceneFileParser::ElementContent( const WCHAR* strData, UINT DataLen, BOOL More )
{
    // Accumulate this element content into the current desc body content.
    wcsncat_s( m_CurrentElementDesc.strElementBody, strData, DataLen );
    return S_OK;
}

HRESULT SceneFileParser::ElementEnd( const WCHAR* strName, UINT NameLen )
{
    // Check if an error has been encountered in scene parsing.
    if( ErrorHasOccurred() )
        return E_FAIL;

    // Distribute an accumulated begin+content package if one exists.
    HandleElementData();

    // Copy the end tag name into the current element desc.
    wcsncpy_s( m_CurrentElementDesc.strElementName, strName, NameLen );
    // Clear out the element body.
    m_CurrentElementDesc.strElementBody[0] = L'\0';
    // Distribute the end tag.
    HandleElementEnd();
    // Clear out the element name.
    m_CurrentElementDesc.strElementName[0] = L'\0';
    return S_OK;
}

VOID SceneFileParser::CopyAttributes( const XMLAttribute* pAttributes, UINT uAttributeCount )
{
    m_CurrentElementDesc.Attributes.clear();
    for( UINT i = 0; i < uAttributeCount; i++ )
    {
        XMLElementAttribute Attribute;
        wcsncpy_s( Attribute.strName, pAttributes[i].strName, pAttributes[i].NameLen );
        wcsncpy_s( Attribute.strValue, pAttributes[i].strValue, pAttributes[i].ValueLen );
        m_CurrentElementDesc.Attributes.push_back( Attribute );
    }
}

BOOL SceneFileParser::FindAttribute( const WCHAR* strName, WCHAR* strDest, UINT uDestLength )
{
    const WCHAR* strValue = FindAttribute( strName );
    if( strValue != NULL )
    {
        wcscpy_s( strDest, uDestLength, strValue );
        return TRUE;
    }
    strDest[0] = L'\0';
    return FALSE;
}

const WCHAR* SceneFileParser::FindAttribute( const WCHAR* strName )
{
    for( UINT i = 0; i < m_CurrentElementDesc.Attributes.size(); i++ )
    {
        const XMLElementAttribute& Attribute = m_CurrentElementDesc.Attributes[i];
        if( _wcsicmp( Attribute.strName, strName ) == 0 )
        {
            return Attribute.strValue;
        }
    }
    return NULL;
}

BOOL SceneFileParser::SetObjectNameFromAttribute( NamedTypedObject* pNTO )
{
    const WCHAR* strName = FindAttribute( L"Name" );
    if( strName != NULL )
    {
        pNTO->SetName( strName );
        return TRUE;
    }
    return FALSE;
}


VOID ScrubFloatString( WCHAR* strFloatString )
{
    WCHAR* pChar = strFloatString;
    while( *pChar != L'\0' )
    {
        if( *pChar == L'{' || *pChar == L'}' || *pChar == L',' || *pChar == L'\t' )
            *pChar = L' ';
        pChar++;
    }
}


XMVECTOR ScanVector3( const WCHAR* strThreeFloats )
{
    WCHAR strTemp[100];
    wcscpy_s( strTemp, strThreeFloats );
    ScrubFloatString( strTemp );

    XMFLOAT4A vResult;
    memset( &vResult, 0, sizeof(vResult ));
    swscanf_s( strTemp, L"%f %f %f",
               &vResult.x, &vResult.y, &vResult.z );
     
    return XMLoadVector4A( &vResult );
}


XMVECTOR ScanVector4( const WCHAR* strFourFloats )
{
    WCHAR strTemp[100];
    wcscpy_s( strTemp, strFourFloats );
    ScrubFloatString( strTemp );
    
    XMFLOAT4A vResult;
    memset( &vResult, 0, sizeof(vResult ));

    swscanf_s( strTemp, L"%f %f %f %f",
               &vResult.x, &vResult.y, &vResult.z, &vResult.w );

    return XMLoadVector4A( &vResult );
}


XMVECTOR ScanColorARGBtoRGBA( const WCHAR* strColorARGB )
{
    XMVECTOR vColor = ScanVector4( strColorARGB );
    return XMVectorSwizzle( vColor, 1, 2, 3, 0 );
}


//--------------------------------------------------------------------------------------
// Name: HandleElementData()
// Desc: This method gets "first crack" at a begin tag + content combo.
//       It identifies certain high-level tags and sets the loader state appropriately.
//--------------------------------------------------------------------------------------
VOID SceneFileParser::HandleElementData()
{
    // If the tag name is blank, return.
    if( wcslen( m_CurrentElementDesc.strElementName ) == 0 )
        return;
    // We are processing a begin tag.
    m_CurrentElementDesc.bEndElement = FALSE;

    // Check for certain high-level tags and set the loader current object type.
    if( MATCH_ELEMENT_NAME( L"StaticMesh" ) || MATCH_ELEMENT_NAME( L"Mesh" ) )
    {
        m_Context.CurrentObjectType = XATG_MESH;
    }
    else if( MATCH_ELEMENT_NAME( L"Frame" ) )
    {
        m_Context.CurrentObjectType = XATG_FRAME;
    }
    else if( MATCH_ELEMENT_NAME( L"Model" ) )
    {
        m_Context.CurrentObjectType = XATG_MODEL;
    }
    else if( MATCH_ELEMENT_NAME( L"MaterialInstance" ) )
    {
        m_Context.CurrentObjectType = XATG_MATERIAL;
    }
    else if( MATCH_ELEMENT_NAME( L"AmbientLight" ) )
    {
        m_Context.CurrentObjectType = XATG_AMBIENTLIGHT;
    }
    else if( MATCH_ELEMENT_NAME( L"DirectionalLight" ) )
    {
        m_Context.CurrentObjectType = XATG_DIRECTIONALLIGHT;
    }
    else if( MATCH_ELEMENT_NAME( L"PointLight" ) )
    {
        m_Context.CurrentObjectType = XATG_POINTLIGHT;
    }
    else if( MATCH_ELEMENT_NAME( L"SpotLight" ) )
    {
        m_Context.CurrentObjectType = XATG_SPOTLIGHT;
    }
    else if( MATCH_ELEMENT_NAME( L"PerspectiveCamera" ) )
    {
        m_Context.CurrentObjectType = XATG_CAMERA;
    }
    else if( MATCH_ELEMENT_NAME( L"Animation" ) )
    {
        m_Context.CurrentObjectType = XATG_ANIMATION;
    }

    // Distribute the data to the appropriate loader function.
    DistributeElementToLoaders();
}


//--------------------------------------------------------------------------------------
// Name: HandleElementEnd()
// Desc: This method gets "first crack" at an end tag.
//       It labels the tag state as "end tag" and passes the tag onto the loader
//       methods.
//--------------------------------------------------------------------------------------
VOID SceneFileParser::HandleElementEnd()
{
    // We are processing an end tag.
    m_CurrentElementDesc.bEndElement = TRUE;

    // Distribute the end tag to the appropriate loader function.
    DistributeElementToLoaders();
}


//--------------------------------------------------------------------------------------
// Name: DistributeElementToLoaders()
// Desc: This method calls the correct loader method based on the type of the current
//       object that we're parsing.  In general, a high level tag sets the object type,
//       and then this method sends all child tags of that high level tag to the right
//       loader method.
//--------------------------------------------------------------------------------------
VOID SceneFileParser::DistributeElementToLoaders()
{
    switch( m_Context.CurrentObjectType )
    {
        case XATG_NONE:
            ProcessRootData();
            break;
        case XATG_FRAME:
            ProcessFrameData();
            break;
        case XATG_MESH:
        case XATG_VERTEXBUFFER:
        case XATG_INDEXBUFFER:
        case XATG_VERTEXDECLELEMENT:
        case XATG_INDEXBUFFERSUBSET:
        case XATG_SKINNEDMESHINFLUENCES:
            ProcessMeshData();
            break;
        case XATG_MODEL:
            ProcessModelData();
            break;
        case XATG_MATERIAL:
            ProcessMaterialData();
            break;
        case XATG_AMBIENTLIGHT:
        case XATG_DIRECTIONALLIGHT:
        case XATG_POINTLIGHT:
        case XATG_SPOTLIGHT:
            ProcessLightData();
            break;
        case XATG_CAMERA:
            ProcessCameraData();
            break;
        case XATG_ANIMATION:
            ProcessAnimationData();
            break;
        default:
            assert( FALSE );
            break;
    }
}

VOID SceneFileParser::ProcessRootData()
{
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"XFileATG" ) )
        {
            WCHAR strAttributeValue[256];
            if( FindAttribute( L"Version", strAttributeValue, ARRAYSIZE( strAttributeValue ) ) )
            {
                FLOAT fVersion = ( FLOAT )_wtof( strAttributeValue );
                if( fVersion < MIN_SAFE_VERSION )
                {
                    Error( E_FAIL, "File version is out of date." );
                }
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"PhysicalMemoryFile" ) )
        {
            CHAR strFileName[MAX_PATH];
            strcpy_s( strFileName, g_strMediaRootPath );
            CHAR strBlobFile[MAX_PATH];
            WideCharToMultiByte( CP_ACP, 0, m_CurrentElementDesc.strElementBody,
                                 wcslen( m_CurrentElementDesc.strElementBody ) + 1, strBlobFile, MAX_PATH, NULL,
                                 NULL );
            strcat_s( strFileName, strBlobFile );
            HANDLE hFile = CreateFile( strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                       FILE_FLAG_SEQUENTIAL_SCAN, NULL );
            if( hFile == INVALID_HANDLE_VALUE )
            {
                Error( E_FAIL, "Could not find physical memory blob file." );
                return;
            }
            DWORD dwFileSize = GetFileSize( hFile, NULL );
            assert( dwFileSize > 0 );
            g_pBinaryBlobData = ( BYTE* )g_pCurrentScene->GetResourceDatabase()->PhysicalAlloc( dwFileSize, 0,
                                                                                                PAGE_READWRITE |
                                                                                                PAGE_WRITECOMBINE );
            DWORD dwBytesRead = 0;
            if( !ReadFile( hFile, g_pBinaryBlobData, dwFileSize, &dwBytesRead, 0 ) )
            {
                CloseHandle( hFile );
                return;
            }
            CloseHandle( hFile );
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"BundledResources" ) )
        {
            CHAR strFileName[MAX_PATH];
            strcpy_s( strFileName, g_strMediaRootPath );
            CHAR strBundleFile[MAX_PATH];
            WideCharToMultiByte( CP_ACP, 0, m_CurrentElementDesc.strElementBody,
                                 wcslen( m_CurrentElementDesc.strElementBody ) + 1, strBundleFile, MAX_PATH, NULL,
                                 NULL );
            strcat_s( strFileName, strBundleFile );
            PackedResource* pPackedResource = new PackedResource();
            //AcquireDirect3D();
            pPackedResource->Create( strFileName );
            //ReleaseDirect3D();
            g_pCurrentScene->GetResourceDatabase()->AddBundledResources( pPackedResource );
            return;
        }
    }
    else
    {
    }
}


VOID SceneFileParser::ProcessFrameData()
{
    Frame* pFrame = ( Frame* )m_Context.pCurrentObject;
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"Frame" ) )
        {
            if( m_Context.pCurrentObject == NULL )
            {
                // If we are the first frame, our parent is the root frame
                m_Context.pCurrentObject = g_pRootFrame;
            }
            pFrame = new Frame;
            pFrame->SetParent( ( Frame* )m_Context.pCurrentObject );
            pFrame->SetLocalTransform( XMMatrixIdentity() );
            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pFrame;
            m_Context.CurrentObjectType = XATG_FRAME;
            SetObjectNameFromAttribute( pFrame );
            const WCHAR* strMatrix = FindAttribute( L"Matrix" );
            if( strMatrix != NULL )
            {
                WCHAR strTemp[512];
                wcscpy_s( strTemp, strMatrix );
                ScrubFloatString( strTemp );
                XMMATRIX Matrix;
                swscanf_s( strTemp,
                           L"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                           &Matrix._11, &Matrix._12, &Matrix._13, &Matrix._14,
                           &Matrix._21, &Matrix._22, &Matrix._23, &Matrix._24,
                           &Matrix._31, &Matrix._32, &Matrix._33, &Matrix._34,
                           &Matrix._41, &Matrix._42, &Matrix._43, &Matrix._44 );
                pFrame->SetLocalTransform( Matrix );
            }
            return;
        }
        else if( g_bDebugXMLParser )
        {
            WCHAR s[100];
            swprintf_s( s, L"Frame tag: %s\n", m_CurrentElementDesc.strElementName );
            OutputDebugStringW( s );
        }
    }
    else
    {
        if( MATCH_ELEMENT_NAME( L"Frame" ) )
        {
            g_pCurrentScene->AddObject( pFrame );

            m_Context.pCurrentObject = m_Context.pCurrentParentFrame;

            if( m_Context.pCurrentParentFrame != NULL )
                m_Context.pCurrentParentFrame = m_Context.pCurrentParentFrame->GetParent();

            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
    }
}

WCHAR* AdvanceToken( WCHAR* pCurrentToken )
{
    pCurrentToken = wcschr( pCurrentToken, L' ' );
    if( pCurrentToken == NULL )
        return NULL;
    while( *pCurrentToken == L' ' )
        pCurrentToken++;
    if( *pCurrentToken == '\0' )
        return NULL;
    return pCurrentToken;
}

VOID SceneFileParser::CrackVertex( BYTE* pDest, const D3DVERTEXELEMENT9* pVertexElements, DWORD dwStreamIndex )
{
    WCHAR strSrc[1024];
    wcscpy_s( strSrc, m_CurrentElementDesc.strElementBody );
    ScrubFloatString( strSrc );
    WCHAR* pChar = strSrc;
    if( *pChar == L' ' )
        pChar = AdvanceToken( pChar );
    const D3DVERTEXELEMENT9* pCurrentElement = pVertexElements;
    while( pCurrentElement->Stream != 0xFF && pChar != NULL )
    {
        if( pCurrentElement->Stream != dwStreamIndex )
        {
            ++pCurrentElement;
            continue;
        }
        switch( pCurrentElement->Type )
        {
            case D3DDECLTYPE_FLOAT4:
                *( FLOAT* )pDest = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 4;
                // fallthru
            case D3DDECLTYPE_FLOAT3:
                *( FLOAT* )pDest = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 4;
                // fallthru
            case D3DDECLTYPE_FLOAT2:
                *( FLOAT* )pDest = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 4;
                // fallthru
            case D3DDECLTYPE_FLOAT1:
                *( FLOAT* )pDest = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 4;
                break;
            case D3DDECLTYPE_FLOAT16_4:
                *( D3DXFLOAT16* )pDest = D3DXFLOAT16( ( FLOAT )_wtof( pChar ) );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                *( D3DXFLOAT16* )pDest = D3DXFLOAT16( ( FLOAT )_wtof( pChar ) );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                // fallthru
            case D3DDECLTYPE_FLOAT16_2:
                *( D3DXFLOAT16* )pDest = D3DXFLOAT16( ( FLOAT )_wtof( pChar ) );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                *( D3DXFLOAT16* )pDest = D3DXFLOAT16( ( FLOAT )_wtof( pChar ) );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                break;
            case D3DDECLTYPE_DEC3N:
            {
                FLOAT Vec[3];
                Vec[0] = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                Vec[1] = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                Vec[2] = ( FLOAT )_wtof( pChar );
                pChar = AdvanceToken( pChar );
                XMXDECN4 DecN4( Vec[0], Vec[1], Vec[2], 1 );
                *( DWORD* )pDest = DecN4.v;
                pDest += 4;
                break;
            }
            case D3DDECLTYPE_D3DCOLOR:
            case D3DDECLTYPE_UBYTE4:
            case D3DDECLTYPE_UBYTE4N:
                *( DWORD* )pDest = ( DWORD )_wtoi( pChar );
                pDest += 4;
                pChar = AdvanceToken( pChar );
                break;
            case D3DDECLTYPE_SHORT4:
            case D3DDECLTYPE_SHORT4N:
            case D3DDECLTYPE_USHORT4:
            case D3DDECLTYPE_USHORT4N:
                *( WORD* )pDest = ( WORD )_wtoi( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                *( WORD* )pDest = ( WORD )_wtoi( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                // fallthru
            case D3DDECLTYPE_SHORT2:
            case D3DDECLTYPE_SHORT2N:
            case D3DDECLTYPE_USHORT2:
            case D3DDECLTYPE_USHORT2N:
                *( WORD* )pDest = ( WORD )_wtoi( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                *( WORD* )pDest = ( WORD )_wtoi( pChar );
                pChar = AdvanceToken( pChar );
                pDest += 2;
                break;
            default:
                // unsupported vertex type
                Error( E_FAIL, "Unsupported vertex declaration element type." );
                break;
        }
        ++pCurrentElement;
    }
}

VOID SceneFileParser::ProcessMeshData()
{
    BaseMesh* pMesh = ( BaseMesh* )m_Context.pCurrentObject;
    typedef std::vector <D3DVERTEXELEMENT9> VertexElementArray;
    if( !m_CurrentElementDesc.bEndElement )
    {
        // start tag + body processing
        if( MATCH_ELEMENT_NAME( L"StaticMesh" ) || MATCH_ELEMENT_NAME( L"Mesh" ) )
        {
            const WCHAR* strSubD = FindAttribute( L"SubD" );
            const WCHAR* strSkinned = FindAttribute( L"Skinned" );
            BaseMesh* pBaseMesh = NULL;
            if( strSubD != NULL )
            {
                pBaseMesh = new CatmullClarkMesh;
            }
            else if( strSkinned != NULL )
            {
                pBaseMesh = new SkinnedMesh;
            }
            else
            {
                pBaseMesh = new StaticMesh;
            }
            m_Context.pCurrentObject = pBaseMesh;
            m_Context.dwUserDataIndex = 0;
            SetObjectNameFromAttribute( pBaseMesh );
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"InfluenceObjects" ) )
        {
            assert( pMesh->Type() == SkinnedMesh::TypeID );
            m_Context.CurrentObjectType = XATG_SKINNEDMESHINFLUENCES;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"VertexBuffer" ) )
        {
            if( pMesh->GetVertexData( 0 ) == NULL )
            {
                VertexData* pVertexData = new VertexData;
                pMesh->SetVertexData( 0, pVertexData );
                SetObjectNameFromAttribute( pVertexData );
            }
            m_Context.CurrentObjectType = XATG_VERTEXBUFFER;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"IndexBuffer" ) )
        {
            IndexData* pIndexData = new IndexData;
            pMesh->SetIndexData( 0, pIndexData );
            m_Context.CurrentObjectType = XATG_INDEXBUFFER;
            const WCHAR* strValue = FindAttribute( L"IndexSize" );
            D3DFORMAT IndexFormat = D3DFMT_INDEX16;
            if( strValue != NULL )
            {
                DWORD dwSize = ( DWORD )_wtoi( strValue );
                if( dwSize == 32 )
                    IndexFormat = D3DFMT_INDEX32;
            }
            strValue = FindAttribute( L"IndexCount" );
            if( strValue != NULL )
            {
                assert( IndexFormat == D3DFMT_INDEX16 || IndexFormat == D3DFMT_INDEX32 );
                DWORD dwIndexCount = ( DWORD )_wtoi( strValue );
                DWORD dwBufferSize = dwIndexCount * sizeof( WORD );
                if( IndexFormat == D3DFMT_INDEX32 )
                    dwBufferSize *= 2;
                D3DIndexBuffer* pIndexBuffer = new D3DIndexBuffer;
                XGSetIndexBufferHeader( dwBufferSize, 0, IndexFormat, D3DPOOL_MANAGED, 0, pIndexBuffer );
                VOID* pBuffer = g_pCurrentScene->GetResourceDatabase()->PhysicalAlloc( dwBufferSize, 32,
                                                                                       PAGE_READWRITE |
                                                                                       PAGE_WRITECOMBINE );
                XGOffsetResourceAddress( pIndexBuffer, pBuffer );

                pIndexData->SetIndexBuffer( pIndexBuffer );
                pIndexData->SetNumIndices( dwIndexCount );
                VOID* pIndices = NULL;
                pIndexBuffer->Lock( 0, 0, &pIndices, 0 );
                m_Context.pUserData = pIndices;
                pIndexBuffer->Release();
            }
            m_Context.dwUserDataIndex = IndexFormat;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"VertexDecls" ) )
        {
            m_Context.pUserData = new VertexElementArray;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"VertexDecl" ) )
        {
            m_Context.CurrentObjectType = XATG_VERTEXDECLELEMENT;
            VertexElementArray* pElements = ( VertexElementArray* )m_Context.pUserData;
            D3DVERTEXELEMENT9 element;
            ZeroMemory( &element, sizeof( D3DVERTEXELEMENT9 ) );
            const WCHAR* strValue = FindAttribute( L"Type" );
            DWORD dwValue = 0;
            if( strValue != NULL )
            {
                GetValueFromString( strValue, D3DDECLTYPE_StringMap, dwValue );
                element.Type = dwValue;
            }
            strValue = FindAttribute( L"Offset" );
            if( strValue != NULL )
            {
                element.Offset = ( WORD )_wtoi( strValue );
            }
            strValue = FindAttribute( L"Method" );
            if( strValue != NULL )
            {
                GetValueFromString( strValue, D3DDECLMETHOD_StringMap, dwValue );
                element.Method = ( BYTE )dwValue;
            }
            strValue = FindAttribute( L"Usage" );
            if( strValue != NULL )
            {
                GetValueFromString( strValue, D3DDECLUSAGE_StringMap, dwValue );
                element.Usage = ( BYTE )dwValue;
            }
            strValue = FindAttribute( L"UsageIndex" );
            if( strValue != NULL )
            {
                element.UsageIndex = ( BYTE )_wtoi( strValue );
            }
            pElements->push_back( element );
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"IBSubset" ) )
        {
            m_Context.CurrentObjectType = XATG_INDEXBUFFERSUBSET;
            SubsetDesc* pSubsetDesc = new SubsetDesc;
            pSubsetDesc->SetStartIndex( 0 );
            pSubsetDesc->SetNumPrimitives( 0 );
            pSubsetDesc->SetPrimitiveType( D3DPT_TRIANGLELIST );
            SetObjectNameFromAttribute( pSubsetDesc );
            const WCHAR* strValue = FindAttribute( L"StartIndex" );
            if( strValue != NULL )
            {
                pSubsetDesc->SetStartIndex( _wtoi( strValue ) );
            }
            strValue = FindAttribute( L"PrimitiveType" );
            if( strValue != NULL )
            {
                if( _wcsicmp( L"TriangleList", strValue ) == 0 )
                {
                    pSubsetDesc->SetPrimitiveType( D3DPT_TRIANGLELIST );
                }
                else if( _wcsicmp( L"TriangleStrip", strValue ) == 0 )
                {
                    pSubsetDesc->SetPrimitiveType( D3DPT_TRIANGLESTRIP );
                }
                else if( _wcsicmp( L"QuadList", strValue ) == 0 )
                {
                    pSubsetDesc->SetPrimitiveType( D3DPT_QUADLIST );
                }
                else if( _wcsicmp( L"QuadPatchList", strValue ) == 0 )
                {
                    pSubsetDesc->SetPrimitiveType( (D3DPRIMITIVETYPE)-1 );
                }
            }
            strValue = FindAttribute( L"IndexCount" );
            if( strValue != NULL )
            {
                INT iIndexCount = _wtoi( strValue );
                switch( (INT)pSubsetDesc->GetPrimitiveType() )
                {
                    case D3DPT_TRIANGLELIST:
                        pSubsetDesc->SetNumPrimitives( iIndexCount / 3 );
                        break;
                    case D3DPT_QUADLIST:
                        pSubsetDesc->SetNumPrimitives( iIndexCount / 4 );
                        break;
                    case D3DPT_TRIANGLESTRIP:
                        pSubsetDesc->SetNumPrimitives( iIndexCount - 2 );
                        break;
                    case -1:
                        pSubsetDesc->SetNumPrimitives( iIndexCount );
                        break;
                    default:
                        pSubsetDesc->SetNumPrimitives( iIndexCount / 3 );
                        break;
                }
            }
            pMesh->AddSubsetDesc( pSubsetDesc );
            return;
        }
        else if( m_Context.CurrentObjectType == XATG_VERTEXBUFFER )
        {
            VertexData* pVertexData = pMesh->GetVertexData( 0 );
            if( pVertexData->GetVertexDecl() == NULL )
                return;
            BYTE* pVerts = ( BYTE* )m_Context.pUserData;
            DWORD dwStreamIndex = m_Context.dwUserDataIndex;
            D3DVERTEXELEMENT9 DeclElements[MAXD3DDECLLENGTH + 1];
            DWORD dwElementCount = MAXD3DDECLLENGTH + 1;
            D3DVertexDeclaration* pDecl = pVertexData->GetVertexDecl();
            pDecl->GetDeclaration( DeclElements, ( UINT* )&dwElementCount );
            DWORD dwVertexSize = D3DXGetDeclVertexSize( DeclElements, dwStreamIndex );
            if( MATCH_ELEMENT_NAME( L"Vertices" ) )
            {
                const WCHAR* strValue = FindAttribute( L"Count" );
                assert( strValue != NULL );
                DWORD dwVertexCount = ( DWORD )_wtoi( strValue );
                DWORD dwVBSize = dwVertexCount * dwVertexSize;
                D3DVertexBuffer* pVB = new D3DVertexBuffer;
                XGSetVertexBufferHeader( dwVBSize, 0, D3DPOOL_MANAGED, 0, pVB );
                VOID* pBuffer = g_pCurrentScene->GetResourceDatabase()->PhysicalAlloc( dwVBSize, 32,
                                                                                       PAGE_READWRITE |
                                                                                       PAGE_WRITECOMBINE );
                XGOffsetResourceAddress( pVB, pBuffer );
                pVertexData->AddVertexStream( pVB, dwVertexSize, dwVertexCount, 1 );
                assert( pVertexData->GetNumVertexStreams() == ( dwStreamIndex + 1 ) );
                pVB->Lock( 0, 0, ( VOID** )&pVerts, 0 );
                m_Context.pUserData = pVerts;
                pVB->Release();
                return;
            }
            else if( MATCH_ELEMENT_NAME( L"E" ) )
            {
                CrackVertex( pVerts, DeclElements, dwStreamIndex );
                pVerts += dwVertexSize;
                m_Context.pUserData = pVerts;
                return;
            }
            else if( MATCH_ELEMENT_NAME( L"PhysicalBinaryData" ) )
            {
                WCHAR strValue[50];
                FindAttribute( L"Offset", strValue, ARRAYSIZE( strValue ) );
                DWORD dwOffset = _wtoi( strValue );
                FindAttribute( L"Size", strValue, ARRAYSIZE( strValue ) );
                DWORD dwSize = _wtoi( strValue );
                FindAttribute( L"Count", strValue, ARRAYSIZE( strValue ) );
                DWORD dwCount = _wtoi( strValue );
                assert( ( dwSize / dwCount ) == dwVertexSize );
                D3DVertexBuffer* pVB = new D3DVertexBuffer;
                XGSetVertexBufferHeader( dwSize, 0, D3DPOOL_MANAGED, 0, pVB );
                VOID* pBuffer = ( VOID* )( g_pBinaryBlobData + dwOffset );
                XGOffsetResourceAddress( pVB, pBuffer );
                pVertexData->AddVertexStream( pVB, dwVertexSize, dwCount, 1 );
                assert( pVertexData->GetNumVertexStreams() == ( dwStreamIndex + 1 ) );
                m_Context.pUserData = NULL;
                pVB->Release();
                return;
            }
        }
        else if( m_Context.CurrentObjectType == XATG_INDEXBUFFER )
        {
            IndexData* pIndexData = pMesh->GetIndexData( 0 );
            WORD* pIndices = ( WORD* )m_Context.pUserData;
            if( MATCH_ELEMENT_NAME( L"E" ) )
            {
                if( m_Context.dwUserDataIndex == D3DFMT_INDEX32 )
                {
                    DWORD* pIndices32 = ( DWORD* )pIndices;
                    *pIndices32 = ( DWORD )_wtoi( m_CurrentElementDesc.strElementBody );
                    pIndices32++;
                    m_Context.pUserData = pIndices32;
                    return;
                }
                *pIndices = ( WORD )_wtoi( m_CurrentElementDesc.strElementBody );
                pIndices++;
                m_Context.pUserData = pIndices;
                return;
            }
            else if( MATCH_ELEMENT_NAME( L"PhysicalBinaryData" ) )
            {
                WCHAR strValue[50];
                FindAttribute( L"Offset", strValue, ARRAYSIZE( strValue ) );
                DWORD dwOffset = _wtoi( strValue );
                FindAttribute( L"Size", strValue, ARRAYSIZE( strValue ) );
                DWORD dwSize = _wtoi( strValue );
                FindAttribute( L"Count", strValue, ARRAYSIZE( strValue ) );
                DWORD dwCount = _wtoi( strValue );

                D3DFORMAT IndexFormat = ( D3DFORMAT )m_Context.dwUserDataIndex;
                assert( IndexFormat == D3DFMT_INDEX16 || IndexFormat == D3DFMT_INDEX32 );
                DWORD dwBufferSize = dwCount * sizeof( WORD );
                if( IndexFormat == D3DFMT_INDEX32 )
                    dwBufferSize *= 2;
                assert( dwSize == dwBufferSize );

                D3DIndexBuffer* pIB = new D3DIndexBuffer;
                XGSetIndexBufferHeader( dwSize, 0, IndexFormat, D3DPOOL_MANAGED, 0, pIB );
                VOID* pBuffer = ( VOID* )( g_pBinaryBlobData + dwOffset );
                XGOffsetResourceAddress( pIB, pBuffer );
                pIndexData->SetIndexBuffer( pIB );
                pIndexData->SetNumIndices( dwCount );
                m_Context.pUserData = NULL;
                pIB->Release();
                return;
            }
        }
        else if( m_Context.CurrentObjectType == XATG_SKINNEDMESHINFLUENCES )
        {
            if( MATCH_ELEMENT_NAME( L"E" ) )
            {
                assert( pMesh->Type() == SkinnedMesh::TypeID );
                SkinnedMesh* pSkinMesh = ( SkinnedMesh* )pMesh;
                pSkinMesh->AddInfluence( m_CurrentElementDesc.strElementBody );
                return;
            }
        }
        else if( g_bDebugXMLParser )
        {
            WCHAR s[100];
            swprintf_s( s, L"Mesh tag: %s\n", m_CurrentElementDesc.strElementName );
            OutputDebugStringW( s );
        }
    }
    else
    {
        // end tag processing
        if( MATCH_ELEMENT_NAME( L"VertexBuffer" ) )
        {
            DWORD dwStreamIndex = m_Context.dwUserDataIndex;
            if( pMesh->GetVertexData( 0 )->GetVertexStream( dwStreamIndex ) != NULL && m_Context.pUserData != NULL )
            {
                m_Context.pUserData = NULL;
                pMesh->GetVertexData( 0 )->GetVertexStream( dwStreamIndex )->pVertexBuffer->Unlock();
            }
            // next stream
            m_Context.dwUserDataIndex++;
            m_Context.CurrentObjectType = XATG_MESH;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"IndexBuffer" ) )
        {
            if( pMesh->GetIndexData( 0 )->GetIndexBuffer() != NULL && m_Context.pUserData != NULL )
            {
                m_Context.pUserData = NULL;
                pMesh->GetIndexData( 0 )->GetIndexBuffer()->Unlock();
            }
            m_Context.CurrentObjectType = XATG_MESH;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"VertexDecl" ) )
        {
            m_Context.CurrentObjectType = XATG_VERTEXBUFFER;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"IBSubset" ) )
        {
            m_Context.CurrentObjectType = XATG_MESH;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"InfluenceObjects" ) )
        {
            m_Context.CurrentObjectType = XATG_MESH;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"VertexDecls" ) )
        {
            VertexElementArray* pElements = ( VertexElementArray* )m_Context.pUserData;
            D3DVERTEXELEMENT9 EndElement = D3DDECL_END();
            pElements->push_back( EndElement );
            D3DVertexDeclaration* pVertexDecl = NULL;
            D3DVERTEXELEMENT9* pElementList = &( pElements->front() );
            // Create or find an existing vertex decl using the vertex decl pool.
            CreatePooledVertexDeclaration( pElementList, &pVertexDecl );
            DWORD dwStreamIndex = m_Context.dwUserDataIndex;
            pMesh->GetVertexData( 0 )->AppendVertexDecl( pVertexDecl, dwStreamIndex );
            delete pElements;
            m_Context.pUserData = NULL;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"StaticMesh" ) || MATCH_ELEMENT_NAME( L"Mesh" ) )
        {
            g_pCurrentScene->AddObject( pMesh );
            m_Context.pCurrentObject = NULL;
            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
    }
}

VOID SceneFileParser::ProcessModelData()
{
    Model* pModel = ( Model* )m_Context.pCurrentObject;
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"Model" ) )
        {
            pModel = new Model;
            pModel->GetLightGroups().push_back( L"default" );
            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pModel;
            pModel->SetParent( m_Context.pCurrentParentFrame );
            pModel->SetLocalTransform( XMMatrixIdentity() );
            DWORD dwModelFlags = MeshMapping::IsShadowCaster | MeshMapping::IsShadowReceiver;
            const WCHAR* strValue = FindAttribute( L"ShadowCaster" );
            if( strValue != NULL )
            {
                if( strValue[0] == L'0' || strValue[0] == L'F' || strValue[0] == L'f' )
                    dwModelFlags &= ~MeshMapping::IsShadowCaster;
            }
            strValue = FindAttribute( L"ShadowReceiver" );
            if( strValue != NULL )
            {
                if( strValue[0] == L'0' || strValue[0] == L'F' || strValue[0] == L'f' )
                    dwModelFlags &= ~MeshMapping::IsShadowReceiver;
            }
            strValue = FindAttribute( L"Mesh" );
            if( strValue != NULL )
            {
                BaseMesh* pMesh = NULL;
                NamedTypedObject* pObj = g_pCurrentScene->FindObject( strValue );
                if( pObj == NULL )
                {
                    Error( E_FAIL, "Model references a non-existent mesh." );
                    return;
                }

                assert( pObj->IsDerivedFrom( L"Mesh" ) );
                pMesh = ( BaseMesh* )pObj;

                if( pMesh != NULL )
                {
                    pModel->AddMesh( pMesh, dwModelFlags );
                }
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"SubsetMaterialMapping" ) )
        {
            const WCHAR* strValue = FindAttribute( L"SubsetName" );
            DWORD dwSubsetIndex = 0;
            if( strValue != NULL )
            {
                if( pModel->GetNumMeshMappings() == 0 )
                {
                    Error( E_FAIL, "Model has subset mappings but no mesh." );
                    return;
                }

                StringID SubsetID = strValue;
                UINT MeshIndex = pModel->GetNumMeshMappings() - 1;
                BaseMesh* pMesh = pModel->GetMeshMapping( MeshIndex ).pMesh;

                // can only map materials to renderable meshes
                if( !( pMesh->GetFlags() & BaseMesh::IsRenderable ) )
                {
                    Error( E_FAIL, "Model has a subset mapping to a non-renderable mesh." );
                    return;
                }

                for(; dwSubsetIndex < pMesh->GetNumSubsets(); dwSubsetIndex++ )
                {
                    if( pMesh->GetSubsetDesc( dwSubsetIndex )->GetName() == SubsetID )
                        break;
                }

                if( dwSubsetIndex == pMesh->GetNumSubsets() )
                {
                    Error( E_FAIL, "Subset mapping has a non-existent subset name." );
                    return;
                }
                m_Context.pUserData = ( VOID* )dwSubsetIndex;
            }
            strValue = FindAttribute( L"MaterialName" );
            if( strValue != NULL )
            {
                MaterialInstance* pMaterial = ( MaterialInstance* )g_pCurrentScene->FindObjectOfType( strValue,
                                                                                                      MaterialInstance
                                                                                                      ::TypeID );
                pModel->AddMaterial( 0, dwSubsetIndex, pMaterial );
                if( pMaterial->IsTransparent() )
                {
                    MeshMapping& mm = pModel->GetMeshMapping( 0 );
                    mm.dwFlags |= MeshMapping::IsTransparent;
                }
            }
        }
        else if( MATCH_ELEMENT_NAME( L"SphereBound" ) )
        {
            Sphere sph;
            sph.Center = XMFLOAT3( 0, 0, 0 );
            sph.Radius = 0;
            const WCHAR* strValue = FindAttribute( L"Center" );
            if( strValue != NULL )
            {
                XMVECTOR vCenter = ScanVector3( strValue );
                XMStoreFloat3( &sph.Center, vCenter );
            }
            strValue = FindAttribute( L"Radius" );
            if( strValue != NULL )
            {
                sph.Radius = ( FLOAT )_wtof( strValue );
            }
            Bound DefaultBound( sph );
            pModel->SetLocalBound( DefaultBound );
            m_Context.dwUserDataIndex = 0;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"AxisAlignedBoxBound" ) )
        {
            AxisAlignedBox aabb;
            aabb.Center = XMFLOAT3( 0, 0, 0 );
            aabb.Extents = XMFLOAT3( 0, 0, 0 );
            const WCHAR* strValue = FindAttribute( L"Center" );
            if( strValue != NULL )
            {
                XMVECTOR vCenter = ScanVector3( strValue );
                XMStoreFloat3( &aabb.Center, vCenter );
            }
            strValue = FindAttribute( L"Extents" );
            if( strValue != NULL )
            {
                XMVECTOR vExtents = ScanVector3( strValue );
                XMStoreFloat3( &aabb.Extents, vExtents );
            }
            Bound DefaultBound( aabb );
            pModel->SetLocalBound( DefaultBound );
            m_Context.dwUserDataIndex = 1;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"OrientedBoxBound" ) )
        {
            OrientedBox obb;
            obb.Center = XMFLOAT3( 0, 0, 0 );
            obb.Extents = XMFLOAT3( 0, 0, 0 );
            obb.Orientation = XMFLOAT4( 0, 0, 0, 1 );
            const WCHAR* strValue = FindAttribute( L"Center" );
            if( strValue != NULL )
            {
                XMVECTOR vCenter = ScanVector3( strValue );
                XMStoreFloat3( &obb.Center, vCenter );
            }
            strValue = FindAttribute( L"Extents" );
            if( strValue != NULL )
            {
                XMVECTOR vExtents = ScanVector3( strValue );
                XMStoreFloat3( &obb.Extents, vExtents );
            }
            strValue = FindAttribute( L"Orientation" );
            if( strValue != NULL )
            {
                XMVECTOR vOrientation = ScanVector4( strValue );
                XMStoreFloat4( &obb.Orientation, vOrientation );
            }
            Bound DefaultBound( obb );
            pModel->SetLocalBound( DefaultBound );
            m_Context.dwUserDataIndex = 2;
            return;
        }
        else if( g_bDebugXMLParser )
        {
            WCHAR s[100];
            swprintf_s( s, L"Model tag: %s\n", m_CurrentElementDesc.strElementName );
            OutputDebugStringW( s );
        }
    }
    else
    {
        if( MATCH_ELEMENT_NAME( L"Model" ) )
        {
            Model* pLocalModel = ( Model* )m_Context.pCurrentObject;

            g_pCurrentScene->AddObject( pLocalModel );

            m_Context.pCurrentObject = m_Context.pCurrentParentFrame;
            m_Context.pCurrentParentFrame = m_Context.pCurrentParentFrame->GetParent();
            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
    }
}

VOID SceneFileParser::ProcessMaterialData()
{
    MaterialInstance* pMaterialInstance = ( MaterialInstance* )m_Context.pCurrentObject;
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"MaterialInstance" ) )
        {
            pMaterialInstance = new MaterialInstance();
            m_Context.pCurrentObject = pMaterialInstance;
            m_Context.dwCurrentParameterIndex = 0;
            if( SetObjectNameFromAttribute( pMaterialInstance ) )
                g_pCurrentScene->AddObject( pMaterialInstance );
            const WCHAR* strTransparent = FindAttribute( L"Transparent" );
            if( strTransparent != NULL )
            {
                pMaterialInstance->SetTransparent( TRUE );
            }
            const WCHAR* strBaseMaterialName = FindAttribute( L"MaterialName" );
            if( strBaseMaterialName != NULL )
            {
                BaseMaterial* pBaseMaterial = ( BaseMaterial* )g_pCurrentScene->GetResourceDatabase
                    ()->FindResourceOfType( strBaseMaterialName, BaseMaterial::TypeID );
                if( pBaseMaterial == NULL )
                {
                    pBaseMaterial = new BaseMaterial();
                    pBaseMaterial->SetName( strBaseMaterialName );
                    g_pCurrentScene->GetResourceDatabase()->AddResource( pBaseMaterial );
                    m_Context.pUserData = pBaseMaterial;
                }
                else
                {
                    m_Context.pUserData = NULL;
                }
                pMaterialInstance->SetBaseMaterial( pBaseMaterial );
                pMaterialInstance->SetBaseMaterialName( pBaseMaterial->GetName() );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"ParamString" ) )
        {
            assert( pMaterialInstance != NULL );
            const WCHAR* strParamName = FindAttribute( L"Name" );
            if( strParamName == NULL )
                return;
            MaterialParameter mp;
            mp.strName = strParamName;
            mp.Type = MaterialParameter::RPT_String;
            const WCHAR* strParamHint = FindAttribute( L"Hint" );
            if( strParamHint != NULL )
            {
                mp.strHint = strParamHint;
            }
            const WCHAR* strParamType = FindAttribute( L"Type" );
            if( strParamType != NULL )
            {
                // further decode parameter based on type
                if( _wcsicmp( strParamType, L"texture2d" ) == 0 )
                {
                    mp.Type = MaterialParameter::RPT_Texture2D;
                }
                else if( _wcsicmp( strParamType, L"texture3d" ) == 0 )
                {
                    mp.Type = MaterialParameter::RPT_Texture3D;
                }
                else if( _wcsicmp( strParamType, L"texturecube" ) == 0 )
                {
                    mp.Type = MaterialParameter::RPT_TextureCube;
                }
            }
            const WCHAR* strInstanceParam = FindAttribute( L"InstanceParam" );
            if( strInstanceParam == NULL && pMaterialInstance->GetBaseMaterial() != NULL )
            {
                if( m_Context.pUserData != NULL )
                    m_Context.dwCurrentParameterIndex = pMaterialInstance->GetBaseMaterial()->AddRawParameter( mp ) | 0x8000;
                else
                    m_Context.dwCurrentParameterIndex = 0x8000;
            }
            else
            {
                m_Context.dwCurrentParameterIndex = pMaterialInstance->AddRawParameter( mp );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"ParamFloat" ) )
        {
            assert( pMaterialInstance != NULL );
            const WCHAR* strParamName = FindAttribute( L"Name" );
            if( strParamName == NULL )
                return;
            MaterialParameter mp;
            mp.strName = strParamName;
            mp.Type = MaterialParameter::RPT_Float;

            DWORD dwCount = 1;
            const WCHAR* strCount = FindAttribute( L"Count" );
            if( strCount != NULL )
                dwCount = _wtoi( strCount );
            if( dwCount > 16 )
                dwCount = 16;
            if( dwCount > 1 )
            {
                mp.pFloatValues = new FLOAT[ dwCount ];
                ZeroMemory( mp.pFloatValues, dwCount * sizeof( FLOAT ) );
            }
            mp.dwCount = dwCount;
            m_Context.dwUserDataIndex = 0;

            const WCHAR* strParamHint = FindAttribute( L"Hint" );
            if( strParamHint != NULL )
            {
                mp.strHint = strParamHint;
            }
            const WCHAR* strInstanceParam = FindAttribute( L"InstanceParam" );
            if( strInstanceParam == NULL && pMaterialInstance->GetBaseMaterial() != NULL )
            {
                if( m_Context.pUserData != NULL )
                    m_Context.dwCurrentParameterIndex = pMaterialInstance->GetBaseMaterial()->AddRawParameter( mp ) | 0x8000;
                else
                    m_Context.dwCurrentParameterIndex = 0x8000;
            }
            else
            {
                m_Context.dwCurrentParameterIndex = pMaterialInstance->AddRawParameter( mp );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"ParamBool" ) )
        {
            assert( pMaterialInstance != NULL );
            const WCHAR* strParamName = FindAttribute( L"Name" );
            if( strParamName == NULL )
                return;
            MaterialParameter mp;
            mp.strName = strParamName;
            mp.Type = MaterialParameter::RPT_Bool;

            const WCHAR* strParamHint = FindAttribute( L"Hint" );
            if( strParamHint != NULL )
            {
                mp.strHint = strParamHint;
            }
            const WCHAR* strInstanceParam = FindAttribute( L"InstanceParam" );
            if( strInstanceParam == NULL && pMaterialInstance->GetBaseMaterial() != NULL )
            {
                if( m_Context.pUserData != NULL )
                    m_Context.dwCurrentParameterIndex = pMaterialInstance->GetBaseMaterial()->AddRawParameter( mp ) | 0x8000;
                else
                    m_Context.dwCurrentParameterIndex = 0x8000;
            }
            else
            {
                m_Context.dwCurrentParameterIndex = pMaterialInstance->AddRawParameter( mp );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"ParamInt" ) )
        {
            assert( pMaterialInstance != NULL );
            const WCHAR* strParamName = FindAttribute( L"Name" );
            if( strParamName == NULL )
                return;
            MaterialParameter mp;
            mp.strName = strParamName;
            mp.Type = MaterialParameter::RPT_Int;

            const WCHAR* strParamHint = FindAttribute( L"Hint" );
            if( strParamHint != NULL )
            {
                mp.strHint = strParamHint;
            }
            const WCHAR* strInstanceParam = FindAttribute( L"InstanceParam" );
            if( strInstanceParam == NULL && pMaterialInstance->GetBaseMaterial() != NULL )
            {
                if( m_Context.pUserData != NULL )
                    m_Context.dwCurrentParameterIndex = pMaterialInstance->GetBaseMaterial()->AddRawParameter( mp ) | 0x8000;
                else
                    m_Context.dwCurrentParameterIndex = 0x8000;
            }
            else
            {
                m_Context.dwCurrentParameterIndex = pMaterialInstance->AddRawParameter( mp );
            }
        }
        else if( MATCH_ELEMENT_NAME( L"Value" ) )
        {
            assert( pMaterialInstance != NULL );
            BOOL bBaseParam = m_Context.dwCurrentParameterIndex & 0x8000;
            DWORD dwParamIndex = m_Context.dwCurrentParameterIndex & 0x7FFF;
            MaterialParameter* pMP = NULL;
            if( bBaseParam && m_Context.pUserData == NULL )
                return;
            if( bBaseParam )
                pMP = &pMaterialInstance->GetBaseMaterial()->GetRawParameter( dwParamIndex );
            else
                pMP = &pMaterialInstance->GetRawParameter( dwParamIndex );

            switch( pMP->Type )
            {
                case MaterialParameter::RPT_String:
                case MaterialParameter::RPT_Texture2D:
                case MaterialParameter::RPT_Texture3D:
                case MaterialParameter::RPT_TextureCube:
                    pMP->strValue = m_CurrentElementDesc.strElementBody;
                    break;
                case MaterialParameter::RPT_Bool:
                {
                    WCHAR c = m_CurrentElementDesc.strElementBody[0];
                    pMP->bValue = ( c == L'1' || c == L'T' || c == L't' );
                    break;
                }
                case MaterialParameter::RPT_Int:
                    pMP->iValue = _wtoi( m_CurrentElementDesc.strElementBody );
                    break;
                case MaterialParameter::RPT_Float:
                {
                    FLOAT fValue = ( FLOAT )_wtof( m_CurrentElementDesc.strElementBody );
                    if( pMP->dwCount == 1 )
                    {
                        pMP->fValue = fValue;
                    }
                    else
                    {
                        assert( m_Context.dwUserDataIndex < pMP->dwCount );
                        pMP->pFloatValues[ m_Context.dwUserDataIndex ] = fValue;
                        m_Context.dwUserDataIndex++;
                    }
                    break;
                }
            }
            return;
        }
    }
    else
    {
        if( MATCH_ELEMENT_NAME( L"MaterialInstance" ) )
        {
            // Bind textures to material instance parameters and base material parameters
            if( ( g_dwLoaderFlags & XATGLOADER_DONOTBINDTEXTURES ) == 0 )
            {
                pMaterialInstance->BindTextures( g_strMediaRootPath, g_pCurrentScene );
            }
            // Initialize material instance from base material, if one exists
            if( ( g_dwLoaderFlags & XATGLOADER_DONOTINITIALIZEMATERIALS ) == 0 )
            {
                pMaterialInstance->Initialize();
            }
            m_Context.pCurrentObject = NULL;
            m_Context.dwCurrentParameterIndex = 0;
            m_Context.dwUserDataIndex = 0;
            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
    }
}

VOID SceneFileParser::ProcessLightData()
{
    Light* pLight = ( Light* )m_Context.pCurrentObject;
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"AmbientLight" ) )
        {
            pLight = new AmbientLight;
            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pLight;
            pLight->SetParent( m_Context.pCurrentParentFrame );
            pLight->SetLocalTransform( XMMatrixIdentity() );
            SetObjectNameFromAttribute( pLight );
            const WCHAR* strValue = FindAttribute( L"Color" );
            if( strValue != NULL )
            {
                XMVECTOR Color = ScanColorARGBtoRGBA( strValue );
                pLight->SetColor( Color );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"DirectionalLight" ) )
        {
            pLight = new DirectionalLight;
            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pLight;
            pLight->SetParent( m_Context.pCurrentParentFrame );
            pLight->SetLocalTransform( XMMatrixIdentity() );
            SetObjectNameFromAttribute( pLight );
            const WCHAR* strValue = FindAttribute( L"Color" );
            if( strValue != NULL )
            {
                XMVECTOR Color = ScanColorARGBtoRGBA( strValue );
                pLight->SetColor( Color );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"PointLight" ) )
        {
            PointLight* pPointLight = new PointLight;
            pLight = pPointLight;
            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pLight;
            pLight->SetParent( m_Context.pCurrentParentFrame );
            pLight->SetLocalTransform( XMMatrixIdentity() );
            SetObjectNameFromAttribute( pLight );
            const WCHAR* strValue = FindAttribute( L"Color" );
            if( strValue != NULL )
            {
                XMVECTOR Color = ScanColorARGBtoRGBA( strValue );
                pLight->SetColor( Color );
            }
            strValue = FindAttribute( L"Range" );
            if( strValue != NULL )
            {
                pPointLight->SetLocalRange( ( FLOAT )_wtof( strValue ) );
            }
            strValue = FindAttribute( L"Falloff" );
            if( strValue != NULL )
            {
                Light::FalloffType Falloff = Light::NoFalloff;
                GetValueFromString( strValue, Light::FalloffType_StringMap, ( DWORD& )Falloff );
                pPointLight->SetFalloff( Falloff );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"SpotLight" ) )
        {
            SpotLight* pSpotLight = new SpotLight;
            pLight = pSpotLight;
            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pLight;
            pLight->SetParent( m_Context.pCurrentParentFrame );
            pLight->SetLocalTransform( XMMatrixIdentity() );
            SetObjectNameFromAttribute( pLight );
            const WCHAR* strValue = FindAttribute( L"Color" );
            if( strValue != NULL )
            {
                XMVECTOR Color = ScanColorARGBtoRGBA( strValue );
                pLight->SetColor( Color );
            }
            strValue = FindAttribute( L"Range" );
            if( strValue != NULL )
            {
                pSpotLight->SetLocalRange( ( FLOAT )_wtof( strValue ) );
            }
            strValue = FindAttribute( L"Falloff" );
            if( strValue != NULL )
            {
                Light::FalloffType Falloff = Light::NoFalloff;
                GetValueFromString( strValue, Light::FalloffType_StringMap, ( DWORD& )Falloff );
                pSpotLight->SetFalloff( Falloff );
            }
            strValue = FindAttribute( L"SpotlightFalloff" );
            if( strValue != NULL )
            {
                Light::FalloffType Falloff = Light::NoFalloff;
                GetValueFromString( strValue, Light::FalloffType_StringMap, ( DWORD& )Falloff );
                pSpotLight->SetSpotFalloff( Falloff );
            }
            FLOAT fInnerAngle = XM_PIDIV2;
            FLOAT fOuterAngle = XM_PIDIV2;
            strValue = FindAttribute( L"InnerAngle" );
            if( strValue != NULL )
            {
                fInnerAngle = ( FLOAT )_wtof( strValue );
            }
            strValue = FindAttribute( L"OuterAngle" );
            if( strValue != NULL )
            {
                fOuterAngle = ( FLOAT )_wtof( strValue );
            }
            if( fInnerAngle < 0 )
                fInnerAngle = 0;
            if( fInnerAngle > XM_PI )
                fInnerAngle = XM_PI;
            if( fOuterAngle > XM_PI )
                fOuterAngle = XM_PI;
            if( fOuterAngle < fInnerAngle )
                fOuterAngle = fInnerAngle;
            pSpotLight->SetInnerAngle( fInnerAngle );
            pSpotLight->SetOuterAngle( fOuterAngle );
            return;
        }
        else if( g_bDebugXMLParser )
        {
            WCHAR s[100];
            swprintf_s( s, L"Light tag: %s\n", m_CurrentElementDesc.strElementName );
            OutputDebugStringW( s );
        }
    }
    else
    {
        if( MATCH_ELEMENT_NAME( L"AmbientLight" ) ||
            MATCH_ELEMENT_NAME( L"DirectionalLight" ) ||
            MATCH_ELEMENT_NAME( L"PointLight" ) ||
            MATCH_ELEMENT_NAME( L"SpotLight" ) )
        {
            pLight->AddLightGroup( L"default" );
            pLight->SetFlag( Light::IsShadowCaster );
            g_pCurrentScene->AddObject( pLight );

            m_Context.pCurrentObject = m_Context.pCurrentParentFrame;
            m_Context.pCurrentParentFrame = m_Context.pCurrentParentFrame->GetParent();
            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
    }
}
VOID SceneFileParser::ProcessCameraData()
{
    Camera* pCamera = ( Camera* )m_Context.pCurrentObject;
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"PerspectiveCamera" ) )
        {
            // Create a new camera and set it up with some default values
            pCamera = new Camera;
            pCamera->SetFocalLength( 10.0f );
            pCamera->SetClearColor( 0x00070720 );

            D3DVIEWPORT9 Viewport;
            Viewport.X = 0;
            Viewport.Y = 0;
            Viewport.Width = 640; // $TODO: Make this scale based on the actual screen size
            Viewport.Height = 480;
            Viewport.MinZ = 0.0f;
            Viewport.MaxZ = 1.0f;
            pCamera->SetViewport( Viewport );

            {
                Projection proj;
                proj.SetFovXAspect( 0.7f, ( FLOAT )Viewport.Width / ( FLOAT )Viewport.Height, 0.01f, 1000.0f );
                pCamera->SetProjection( proj );
            }

            m_Context.pCurrentParentFrame = ( Frame* )m_Context.pCurrentObject;
            m_Context.pCurrentObject = pCamera;
            pCamera->SetParent( m_Context.pCurrentParentFrame );
            pCamera->SetLocalTransform( XMMatrixIdentity() );

            SetObjectNameFromAttribute( pCamera );
            const WCHAR* strValue = FindAttribute( L"EyePoint" );
            if( strValue != NULL )
            {
                XMVECTOR LocalPosition = ScanVector3( strValue );
                LocalPosition = XMVectorSetW( LocalPosition, 1 );
                pCamera->SetLocalPosition( LocalPosition );
            }
            const WCHAR* strValueDir = FindAttribute( L"LookDirection" );
            const WCHAR* strValueUp = FindAttribute( L"UpDirection" );
            if( strValueDir != NULL && strValueUp != NULL )
            {
                XMVECTOR vLocalDirection = ScanVector3( strValueDir );
                vLocalDirection = XMVector3Normalize( vLocalDirection );
                XMVECTOR vLocalUpDirection = ScanVector3( strValueUp );
                vLocalUpDirection = XMVector3Normalize( vLocalUpDirection );
                XMVECTOR vLocalRightDirection = XMVector3Cross( vLocalUpDirection, vLocalDirection );
                vLocalUpDirection = XMVector3Cross( vLocalDirection, vLocalRightDirection );
                vLocalDirection = XMVector3Cross( vLocalRightDirection, vLocalUpDirection );
                XMMATRIX matLocalTransform = pCamera->GetLocalTransform();
                matLocalTransform.r[0] = XMVector3Normalize( vLocalRightDirection );
                matLocalTransform.r[1] = XMVector3Normalize( vLocalUpDirection );
                matLocalTransform.r[2] = XMVector3Normalize( vLocalDirection );
                pCamera->SetLocalTransform( matLocalTransform );
            }
            /*
            strValue = FindAttribute( L"LookDirection" );
            if( strValue != NULL )
            {
            XMVECTOR LocalDirection;
            swscanf_s( strValue, L"%f, %f, %f", 
            &LocalDirection.x, &LocalDirection.y, &LocalDirection.z );                
            LocalDirection.w = 0;
            XMMATRIX mTransform;
            mTransform = pCamera->GetLocalTransform();
            
            mTransform.r[2] = LocalDirection;
            mTransform.r[0] = XMVector3Cross( mTransform.r[1], mTransform.r[2] );               
            mTransform.r[0] = XMVector3Normalize( mTransform.r[0] );
            mTransform.r[1] = XMVector3Cross( mTransform.r[2], mTransform.r[0] );
            
            pCamera->SetLocalTransform( mTransform );
            }
            strValue = FindAttribute( L"UpDirection" );
            if( strValue != NULL )
            {
            XMVECTOR LocalUpDirection;
            swscanf_s( strValue, L"%f, %f, %f", 
            &LocalUpDirection.x, &LocalUpDirection.y, &LocalUpDirection.z );
            LocalUpDirection.w = 0;
            
            XMMATRIX mTransform;
            mTransform = pCamera->GetLocalTransform();
            mTransform.r[1] = LocalUpDirection;
            mTransform.r[0] = XMVector3Cross( mTransform.r[1], mTransform.r[2] );                
            mTransform.r[0] = XMVector3Normalize( mTransform.r[0] );
            mTransform.r[2] = XMVector3Cross( mTransform.r[0], mTransform.r[1] );
            
            pCamera->SetLocalTransform( mTransform );
            }
            */
            strValue = FindAttribute( L"FieldOfView" );
            if( strValue != NULL )
            {
                Projection proj = pCamera->GetProjection();
                FLOAT fFOV = ( FLOAT )_wtof( strValue );
                FLOAT fAspect = proj.GetFovX() / proj.GetFovY();
                proj.SetFovXAspect( fFOV, fAspect, proj.GetZNear(), proj.GetZFar() );
                pCamera->SetProjection( proj );
            }
            strValue = FindAttribute( L"Aspect" );
            if( strValue != NULL )
            {
                Projection proj = pCamera->GetProjection();
                FLOAT fAspect = ( FLOAT )_wtof( strValue );
                proj.SetFovXAspect( proj.GetFovX(), fAspect, proj.GetZNear(), proj.GetZFar() );
                pCamera->SetProjection( proj );
            }
            strValue = FindAttribute( L"ZNear" );
            if( strValue != NULL )
            {
                Projection proj = pCamera->GetProjection();
                FLOAT fZNear = ( FLOAT )_wtof( strValue );
                proj.SetFovXFovY( proj.GetFovX(), proj.GetFovY(), fZNear, proj.GetZFar() );
                pCamera->SetProjection( proj );
            }
            strValue = FindAttribute( L"ZFar" );
            if( strValue != NULL )
            {
                Projection proj = pCamera->GetProjection();
                FLOAT fZFar = ( FLOAT )_wtof( strValue );
                proj.SetFovXFovY( proj.GetFovX(), proj.GetFovY(), proj.GetZNear(), fZFar );
                pCamera->SetProjection( proj );
            }
            return;
        }
        else if( g_bDebugXMLParser )
        {
            WCHAR s[100];
            swprintf_s( s, L"Camera tag: %s\n", m_CurrentElementDesc.strElementName );
            OutputDebugStringW( s );
        }
    }
    else
    {
        if( MATCH_ELEMENT_NAME( L"PerspectiveCamera" ) )
        {
            g_pCurrentScene->AddObject( pCamera );
            m_Context.pCurrentObject = m_Context.pCurrentParentFrame;
            m_Context.pCurrentParentFrame = m_Context.pCurrentParentFrame->GetParent();
            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
    }
}


VOID SceneFileParser::ProcessAnimationData()
{
    Animation* pAnimation = NULL;
    AnimationTransformTrack* pTrack = NULL;
    if( m_Context.pCurrentParentObject != NULL )
    {
        pTrack = ( AnimationTransformTrack* )m_Context.pCurrentObject;
        pAnimation = ( Animation* )m_Context.pCurrentParentObject;
    }
    else
    {
        pAnimation = ( Animation* )m_Context.pCurrentObject;
    }
    if( !m_CurrentElementDesc.bEndElement )
    {
        if( MATCH_ELEMENT_NAME( L"Animation" ) )
        {
            Animation* pNewAnimation = new Animation();
            m_Context.pCurrentObject = pNewAnimation;
            assert( m_Context.pCurrentParentObject == NULL );
            SetObjectNameFromAttribute( pNewAnimation );
            const WCHAR* strDuration = FindAttribute( L"Duration" );
            if( strDuration != NULL )
                pNewAnimation->SetDuration( ( FLOAT )_wtof( strDuration ) );
            g_pCurrentScene->AddObject( pNewAnimation );
            g_pCurrentScene->GetResourceDatabase()->AddResource( pNewAnimation );
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"AnimationTrack" ) )
        {
            assert( pAnimation != NULL );
            AnimationTransformTrack* pNewTrack = new AnimationTransformTrack();
            assert( m_Context.pCurrentParentObject == NULL );
            m_Context.pCurrentParentObject = pAnimation;
            m_Context.pCurrentObject = pNewTrack;
            SetObjectNameFromAttribute( pNewTrack );
            pAnimation->AddAnimationTrack( pNewTrack );
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"PositionKeys" ) )
        {
            m_Context.dwUserDataIndex = 0;
            const WCHAR* strValue = FindAttribute( L"Count" );
            if( strValue != NULL )
            {
                DWORD dwCount = ( DWORD )_wtoi( strValue );
                if( pTrack )
                    pTrack->GetPositionKeys().Resize( dwCount );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"OrientationKeys" ) )
        {
            m_Context.dwUserDataIndex = 1;
            const WCHAR* strValue = FindAttribute( L"Count" );
            if( strValue != NULL )
            {
                DWORD dwCount = ( DWORD )_wtoi( strValue );
                if( pTrack )
                    pTrack->GetOrientationKeys().Resize( dwCount );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"ScaleKeys" ) )
        {
            m_Context.dwUserDataIndex = 2;
            const WCHAR* strValue = FindAttribute( L"Count" );
            if( strValue != NULL )
            {
                DWORD dwCount = ( DWORD )_wtoi( strValue );
                if( pTrack )
                    pTrack->GetScaleKeys().Resize( dwCount );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"PositionKey" ) )
        {
            assert( pTrack != NULL );
            DWORD dwKeyIndex = pTrack->GetPositionKeys().AddKey();
            m_Context.dwCurrentParameterIndex = dwKeyIndex;
            m_Context.dwUserDataIndex = 0;
            const WCHAR* strValue = FindAttribute( L"Time" );
            if( strValue != NULL )
            {
                FLOAT fTime = ( FLOAT )_wtof( strValue );
                pTrack->GetPositionKeys().SetKeyTime( dwKeyIndex, fTime );
            }
            strValue = FindAttribute( L"Position" );
            if( strValue != NULL )
            {
                XMVECTOR vPosition = ScanVector3( strValue );
                pTrack->GetPositionKeys().SetKeyValue( dwKeyIndex, vPosition );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"OrientationKey" ) )
        {
            assert( pTrack != NULL );
            DWORD dwKeyIndex = pTrack->GetOrientationKeys().AddKey();
            m_Context.dwCurrentParameterIndex = dwKeyIndex;
            m_Context.dwUserDataIndex = 1;
            const WCHAR* strValue = FindAttribute( L"Time" );
            if( strValue != NULL )
            {
                FLOAT fTime = ( FLOAT )_wtof( strValue );
                pTrack->GetOrientationKeys().SetKeyTime( dwKeyIndex, fTime );
            }
            strValue = FindAttribute( L"Orientation" );
            if( strValue != NULL )
            {
                XMVECTOR vOrientation = ScanVector4( strValue );
                pTrack->GetOrientationKeys().SetKeyValue( dwKeyIndex, vOrientation );
            }
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"ScaleKey" ) )
        {
            assert( pTrack != NULL );
            DWORD dwKeyIndex = pTrack->GetScaleKeys().AddKey();
            m_Context.dwCurrentParameterIndex = dwKeyIndex;
            m_Context.dwUserDataIndex = 2;
            const WCHAR* strValue = FindAttribute( L"Time" );
            if( strValue != NULL )
            {
                FLOAT fTime = ( FLOAT )_wtof( strValue );
                pTrack->GetScaleKeys().SetKeyTime( dwKeyIndex, fTime );
            }
            strValue = FindAttribute( L"Scale" );
            if( strValue != NULL )
            {
                XMVECTOR vScale = ScanVector3( strValue );
                pTrack->GetScaleKeys().SetKeyValue( dwKeyIndex, vScale );
            }
            return;
        }
        else if( g_bDebugXMLParser )
        {
            WCHAR s[100];
            swprintf_s( s, L"Animation tag: %s\n", m_CurrentElementDesc.strElementName );
            OutputDebugStringW( s );
        }
    }
    else
    {
        if( MATCH_ELEMENT_NAME( L"Animation" ) )
        {
            m_Context.pCurrentObject = NULL;
            m_Context.CurrentObjectType = XATG_FRAME;
            return;
        }
        else if( MATCH_ELEMENT_NAME( L"AnimationTrack" ) )
        {
            assert( pTrack != NULL );
            pTrack->GetPositionKeys().SortKeys();
            pTrack->GetScaleKeys().SortKeys();
            pTrack->GetOrientationKeys().SortKeys();
            m_Context.dwCurrentParameterIndex = 0;
            m_Context.pCurrentObject = m_Context.pCurrentParentObject;
            m_Context.pCurrentParentObject = NULL;
            return;
        }
    }
}


VOID SceneFileParser::CollapseSceneFrames( Frame* pFrame )
{
    // If we only have one non-pure-frame child and we're a frame, we'll merge 
    Frame* pSrch = NULL, *pMergeFrame = NULL, *pNext = NULL;
    int iCount = 0;
    for( pSrch = pFrame->GetFirstChild(); pSrch; pSrch = pSrch->GetNextSibling() )
    {
        if( pSrch->Type() != Frame::TypeID )
        {
            pMergeFrame = pSrch;
            iCount++;
        }
    }

    if( iCount == 1 )
    {
        XMMATRIX mTransform;

        // this will automatically do the right thing to the local transform
        pMergeFrame->SetParent( pFrame->GetParent() );

        // reparent all other children of this frame
        for( pSrch = pFrame->GetFirstChild(); pSrch; pSrch = pNext )
        {
            pNext = pSrch->GetNextSibling();
            pSrch->SetParent( pMergeFrame );
        }
        pFrame->SetParent( NULL );
        g_pCurrentScene->RemoveObject( pFrame );

        // if we change the name, we need to remove and re-add to the current scene
        g_pCurrentScene->RemoveObject( pMergeFrame );
        pMergeFrame->SetName( pFrame->GetName() );
        g_pCurrentScene->AddObject( pMergeFrame );

        delete pFrame;
        pFrame = pMergeFrame;
    }

    for( pSrch = pFrame->GetFirstChild(); pSrch; pSrch = pNext )
    {
        pNext = pSrch->GetNextSibling();
        if( pSrch->Type() == Frame::TypeID )
            CollapseSceneFrames( pSrch );
    }
}

} // namespace ATG
