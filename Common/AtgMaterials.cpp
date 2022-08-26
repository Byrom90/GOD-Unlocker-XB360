//-----------------------------------------------------------------------------
// AtgMaterials.cpp
//
// Materials classes for the scene hierarchy.  These classes support FXLite
// materials as well as pixel/vertex shader materials.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgMaterials.h"
#include "AtgSceneAll.h"
#include "AtgUtil.h"

namespace ATG
{
    const StringID BaseMaterial::TypeID( L"BaseMaterial" );
    const StringID MaterialInstance::TypeID( L"MaterialInstance" );
    const StringID MaterialImplementation::TypeID( L"MaterialImplementation" );
    const StringID FXLiteMaterialImplementation::TypeID( L"FXLiteMaterialImplementation" );
    const StringID ShaderMaterialImplementation::TypeID( L"ShaderMaterialImplementation" );

    CHAR g_strConvertBuffer[ 512 ];
    inline const CHAR* ConvertString( const WCHAR* strWide )
    {
        WideCharToMultiByte( CP_ACP, 0, strWide, wcslen( strWide ) + 1, g_strConvertBuffer, ARRAYSIZE( g_strConvertBuffer ), NULL, NULL );
        g_strConvertBuffer[ ARRAYSIZE( g_strConvertBuffer ) - 1 ] = '\0';
        return g_strConvertBuffer;
    }


    inline VOID CreateMediaPath( CHAR* strDest, UINT uDestLength, const WCHAR* strSrcPath, const CHAR* strMediaSubDirectory, const CHAR* strMediaRootPath )
    {
        const WCHAR* strFilename = wcsrchr( strSrcPath, L'/' );
        if( strFilename == NULL )
        {
            strFilename = wcsrchr( strSrcPath, L'\\' );
        }
        if( strFilename == NULL )
        {
            strFilename = strSrcPath;
        }
        else
        {
            strFilename++;
        }
        strcpy_s( strDest, uDestLength, strMediaRootPath );
        if( strMediaSubDirectory != NULL )
        {
            strcat_s( strDest, uDestLength, strMediaSubDirectory );
            strcat_s( strDest, uDestLength, "\\" );
        }
        UINT uLength = strlen( strDest );
        WideCharToMultiByte( CP_ACP, 0, strFilename, wcslen( strFilename ) + 1, strDest + uLength, uDestLength - uLength, NULL, NULL );
        // Guarantee that the buffer is null terminated.
        strDest[ uDestLength - 1 ] = 0;
    }


    inline VOID BindTextureHelper( MaterialParameter& mp, const CHAR* strMediaRootPath, Scene* pScene )
    {
        switch( mp.Type )
        {
        case MaterialParameter::RPT_Texture2D:
            {
                CHAR strTexturePath[MAX_PATH];
                Texture* pTexResource = NULL;
                CreateMediaPath( strTexturePath, MAX_PATH, mp.strValue, "textures", strMediaRootPath );
                pTexResource = pScene->GetResourceDatabase()->AddTexture2D( strTexturePath );
                if( pTexResource == NULL )
                    pTexResource = pScene->GetResourceDatabase()->GetDefaultTexture2D();
                mp.pValue = pTexResource;
                break;
            }
        case MaterialParameter::RPT_TextureCube:
            {
                CHAR strTexturePath[MAX_PATH];
                Texture* pTexResource = NULL;
                CreateMediaPath( strTexturePath, MAX_PATH, mp.strValue, "textures", strMediaRootPath );
                pTexResource = pScene->GetResourceDatabase()->AddTextureCube( strTexturePath );
                if( pTexResource == NULL )
                    pTexResource = pScene->GetResourceDatabase()->GetDefaultTextureCube();
                mp.pValue = pTexResource;
                break;
            }
        case MaterialParameter::RPT_Texture3D:
            {
                CHAR strTexturePath[MAX_PATH];
                Texture* pTexResource = NULL;
                CreateMediaPath( strTexturePath, MAX_PATH, mp.strValue, "textures", strMediaRootPath );
                pTexResource = pScene->GetResourceDatabase()->AddTextureVolume( strTexturePath );
                if( pTexResource == NULL )
                    pTexResource = pScene->GetResourceDatabase()->GetDefaultTexture2D();
                mp.pValue = pTexResource;
                break;
            }
        }
    }


    inline DWORD FindRawParameterHelper( const StringID Name, const MaterialParameterVector* pParams )
    {
        DWORD dwCount = (DWORD)pParams->size();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            const MaterialParameter& mp = (*pParams)[i];
            if( mp.strName == Name )
                return i;
        }
        return (DWORD)-1;
    }


    //----------------------------------------------------------------------------------
    // BaseMaterial
    //----------------------------------------------------------------------------------

    CHAR BaseMaterial::m_strMediaRootPath[ MAX_PATH ] = "game:\\media\\";

    BaseMaterial::BaseMaterial()
        : m_pMaterialImplementation( NULL ),
          m_bTexturesBound( FALSE )
    { }


    VOID BaseMaterial::SetMediaRootPath( const CHAR* strMediaRootPath )
    {
        strcpy_s( m_strMediaRootPath, strMediaRootPath );
    }


    BaseMaterial* BaseMaterial::CreateFXLiteMaterial( StringID MaterialName,
                                                      StringID FXLiteFilePath,
                                                      StringID TechniqueName )
    {
        BaseMaterial* pMaterial = new BaseMaterial();
        pMaterial->SetName( MaterialName );

        MaterialParameter mp;
        mp.Type = MaterialParameter::RPT_String;
        mp.strName = L"EffectFile";
        mp.strValue = FXLiteFilePath;
        pMaterial->AddRawParameter( mp );

        if( TechniqueName != NULL )
        {
            mp.strName = L"EffectTechnique";
            mp.strValue = TechniqueName;
            pMaterial->AddRawParameter( mp );
        }

        return pMaterial;
    }


    BaseMaterial* BaseMaterial::CreateShaderMaterial( StringID MaterialName,
                                                      StringID VertexShaderFileName,
                                                      StringID VertexShaderEntryPoint,
                                                      StringID PixelShaderFileName,
                                                      StringID PixelShaderEntryPoint )
    {
        BaseMaterial* pMaterial = new BaseMaterial();
        pMaterial->SetName( MaterialName );

        MaterialParameter mp;
        mp.Type = MaterialParameter::RPT_String;
        mp.strName = L"VertexShader";
        mp.strValue = VertexShaderFileName;
        pMaterial->AddRawParameter( mp );

        mp.strName = L"VertexShaderEntryPoint";
        mp.strValue = VertexShaderEntryPoint;
        pMaterial->AddRawParameter( mp );

        if( PixelShaderFileName != NULL )
        {
            assert( PixelShaderEntryPoint != NULL );
            mp.strName = L"PixelShader";
            mp.strValue = PixelShaderFileName;
            pMaterial->AddRawParameter( mp );

            mp.strName = L"PixelShaderEntryPoint";
            mp.strValue = PixelShaderEntryPoint;
            pMaterial->AddRawParameter( mp );
        }

        return pMaterial;
    }


    MaterialInstanceData BaseMaterial::InitializeInstance( MaterialInstance* pMaterialInstance )
    {
        if( m_pMaterialImplementation == NULL )
        {
            InitializeImplementation();
        }

        if( m_pMaterialImplementation != NULL )
            return m_pMaterialImplementation->CreateInstanceData( pMaterialInstance );

        return NULL;
    }


    VOID BaseMaterial::InitializeImplementation()
    {
        assert( m_pMaterialImplementation == NULL );

        // Figure out what type of material implementation we need to create
        if( FXLiteMaterialImplementation::IsFXLiteMaterial( this ) )
        {
            // Effect material
            m_pMaterialImplementation = new FXLiteMaterialImplementation( this );
            return;
        }

        if( ShaderMaterialImplementation::IsShaderMaterial( this ) )
        {
            // Shader material
            m_pMaterialImplementation = new ShaderMaterialImplementation( this );
        }
    }


    VOID BaseMaterial::ChangeDevice( ::D3DDevice* pd3dDevice )
    {
        if( m_pMaterialImplementation != NULL )
            m_pMaterialImplementation->ChangeDevice( pd3dDevice );
    }


    VOID BaseMaterial::BindTextures( const CHAR* strMediaRootPath, Scene* pScene )
    {
        if( m_bTexturesBound )
            return;

        DWORD dwCount = GetRawParameterCount();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            MaterialParameter& mp = GetRawParameter( i );
            BindTextureHelper( mp, strMediaRootPath, pScene );
        }

        m_bTexturesBound = TRUE;
    }


    DWORD BaseMaterial::FindRawParameter( const StringID Name ) const
    {
        return FindRawParameterHelper( Name, &m_RawParameters );
    }


    DWORD BaseMaterial::GetPassCount() const
    {
        assert( m_pMaterialImplementation != NULL );
        return m_pMaterialImplementation->GetPassCount();
    }


    VOID BaseMaterial::BeginPass( MaterialInstanceData Data, DWORD dwIndex )
    {
        assert( m_pMaterialImplementation != NULL );
        m_pMaterialImplementation->BeginPass( Data, dwIndex );
    }


    VOID BaseMaterial::EndPass( MaterialInstanceData Data )
    {
        assert( m_pMaterialImplementation != NULL );
        m_pMaterialImplementation->EndPass( Data );
    }


    VOID BaseMaterial::BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant )
    {
        assert( m_pMaterialImplementation != NULL );
        m_pMaterialImplementation->BeginMaterial( Data, pd3dDevice, dwMaterialVariant );
    }


    VOID BaseMaterial::EndMaterial( MaterialInstanceData Data )
    {
        assert( m_pMaterialImplementation != NULL );
        m_pMaterialImplementation->EndMaterial( Data );
    }


    //----------------------------------------------------------------------------------
    // MaterialInstance
    //----------------------------------------------------------------------------------

    MaterialInstance::MaterialInstance()
        : m_bTexturesBound( FALSE ),
          m_bTransparent( FALSE ),
          m_dwCurrentBindingIndex( 0 )
    {
        AllocateMaterialBindings( 1 );
    }


    VOID MaterialInstance::AllocateMaterialBindings( DWORD dwMaxCount )
    {
        MaterialPair mp = { 0 };
        m_MaterialBindings.resize( dwMaxCount, mp );
    }


    VOID MaterialInstance::SetMaterialBindingIndex( DWORD dwIndex )
    {
        assert( dwIndex >= 0 && dwIndex < GetMaterialBindingCount() );
        m_dwCurrentBindingIndex = dwIndex;
    }


    VOID MaterialInstance::SetBaseMaterial( BaseMaterial* pBM )
    {
        BaseMaterial* pCurrentMaterial = GetBaseMaterial();
        if( pCurrentMaterial == pBM )
            return;

        if( pCurrentMaterial != NULL && pCurrentMaterial->GetMaterialImplementation() != NULL )
            pCurrentMaterial->GetMaterialImplementation()->DestroyInstanceData( GetInstanceData() );

        MaterialPair mp = { pBM, NULL };
        m_MaterialBindings[m_dwCurrentBindingIndex] = mp;
    }


    VOID MaterialInstance::Initialize()
    {
        if( GetBaseMaterial() == NULL || GetInstanceData() != NULL )
            return;

        m_MaterialBindings[m_dwCurrentBindingIndex].pMaterialInstanceData = GetBaseMaterial()->InitializeInstance( this );
    }


    VOID MaterialInstance::BindTextures( const CHAR* strMediaRootPath, Scene* pScene )
    {
        if( m_bTexturesBound )
            return;

        DWORD dwCount = GetRawParameterCount();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            MaterialParameter& mp = GetRawParameter( i );
            BindTextureHelper( mp, strMediaRootPath, pScene );
        }

        m_bTexturesBound = TRUE;

        if( GetBaseMaterial() != NULL )
            GetBaseMaterial()->BindTextures( strMediaRootPath, pScene );
    }


    DWORD MaterialInstance::FindRawParameter( const StringID Name ) const
    {
        return FindRawParameterHelper( Name, &m_RawParameters );
    }


    DWORD MaterialInstance::GetPassCount() const
    {
        assert( GetBaseMaterial() != NULL );
        return GetBaseMaterial()->GetPassCount();
    }


    VOID MaterialInstance::BeginPass( DWORD dwIndex )
    {
        assert( GetBaseMaterial() != NULL );
        GetBaseMaterial()->BeginPass( GetInstanceData(), dwIndex );
    }


    VOID MaterialInstance::EndPass()
    {
        assert( GetBaseMaterial() != NULL );
        GetBaseMaterial()->EndPass( GetInstanceData() );
    }


    VOID MaterialInstance::BeginMaterial( ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant )
    {
        assert( GetBaseMaterial() != NULL );
        GetBaseMaterial()->BeginMaterial( GetInstanceData(), pd3dDevice, dwMaterialVariant );
    }


    VOID MaterialInstance::EndMaterial()
    {
        assert( GetBaseMaterial() != NULL );
        GetBaseMaterial()->EndMaterial( GetInstanceData() );
    }


    VOID MaterialInstance::BeginMaterialSinglePass( ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant )
    {
        assert( GetPassCount() == 1 );
        BeginMaterial( pd3dDevice, dwMaterialVariant );
        BeginPass( 0 );
    }


    VOID MaterialInstance::EndMaterialSinglePass()
    {
        EndPass();
        EndMaterial();
    }

    //-------------------------------------------------------------------------
    // Shader Include Handler
    //-------------------------------------------------------------------------

    ShaderIncludeHandler::ShaderIncludeHandler( const CHAR* strBasePath )
    {
        strcpy_s( m_strBasePath, strBasePath );
        CHAR* pLastSlash = strrchr( m_strBasePath, '\\' );
        if( pLastSlash != NULL )
            pLastSlash[1] = '\0';
    }

    HRESULT ShaderIncludeHandler::Open( D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData,
        LPCVOID* ppData, UINT* pBytes, /* OUT */ LPSTR pFullPath, DWORD cbFullPath )
    {
        CHAR strIncludeFileName[MAX_PATH];
        strcpy_s( strIncludeFileName, m_strBasePath );
        strcat_s( strIncludeFileName, pFileName );

        strcpy_s( pFullPath, cbFullPath, strIncludeFileName );

        HANDLE hFile = ::CreateFile( strIncludeFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            ATG::DebugSpew( "ATG::ShaderIncludeHandler error: Could not load shader include file \"%s\".\n", strIncludeFileName );
            return E_FAIL;
        }

        DWORD dwSize, dwWritten;
        dwSize = GetFileSize( hFile, NULL );

        CHAR* pFileBuffer = new CHAR[ dwSize + 1 ];

        if( !ReadFile(hFile, pFileBuffer, dwSize, &dwWritten, NULL ))
        {
            ATG::DebugSpew( "ATG::ShaderIncludeHandler error: Could not load shader include file \"%s\".\n", strIncludeFileName );
            CloseHandle( hFile );
            return E_FAIL;
        }

        CloseHandle( hFile );
        pFileBuffer[ dwSize ] = '\0';

        *ppData = (VOID*)pFileBuffer;
        *pBytes = dwSize;

        return S_OK;
    }


    HRESULT ShaderIncludeHandler::Close( LPCVOID pData )
    {
        delete[] pData;
        return S_OK;
    }

    //-------------------------------------------------------------------------
    // FXLite Material Implementation
    //-------------------------------------------------------------------------

    FXLEffectPool* g_pFXLiteEffectPool = NULL;
    VOID FXLiteMaterialImplementation::SetParameterPool( FXLEffectPool* pEffectPool )
    {
        g_pFXLiteEffectPool = pEffectPool;
    }

    D3DTexture* g_pNullSamplerTexture = NULL;
    VOID FXLiteMaterialImplementation::SetNullSamplerTexture( D3DTexture* pTexture )
    {
        g_pNullSamplerTexture = pTexture;
    }

    FXLiteMaterialImplementation::FXLiteMaterialImplementation( BaseMaterial* pBaseMaterial )
        : m_dwPassCount( 0 ),
          m_hTechnique( NULL ),
          m_pEffect( NULL )
    {
        assert( pBaseMaterial != NULL );
        DWORD dwParamIndex = pBaseMaterial->FindRawParameter( L"EffectFile" );
        if( dwParamIndex == (DWORD)-1 )
        {
            DebugSpew( "WARNING: FXLite material %S does not have an EffectFile parameter.\n", pBaseMaterial->GetName().GetSafeString() );
            return;
        }

        const WCHAR* strFileName = pBaseMaterial->GetRawParameter( dwParamIndex ).strValue;
        CHAR strFileNameAscii[ MAX_PATH ];
        CreateMediaPath( strFileNameAscii, ARRAYSIZE( strFileNameAscii ), strFileName, "Effects", BaseMaterial::GetMediaRootPath() );

        HANDLE hFile = ::CreateFile( strFileNameAscii, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
        if ( hFile == INVALID_HANDLE_VALUE )
        {
            return;
        }

        DWORD dwSize, dwRead;
        dwSize = GetFileSize( hFile, NULL );

        CHAR *pFileBuffer = new CHAR[ dwSize + 1 ];

        if( !ReadFile( hFile, pFileBuffer, dwSize, &dwRead, NULL ))
        {
            CloseHandle( hFile );
            delete[] pFileBuffer;
            return;
        }

        CloseHandle( hFile );
        pFileBuffer[ dwSize ] = '\0';

        if( (BYTE)pFileBuffer[0] > 0x7f )
        {
            // binary object file
            CreateBinary( (BYTE*)pFileBuffer );
        }
        else
        {
            // ASCII effect source code
            D3DXMACRO* pMacros = CreateMacroList( pBaseMaterial );
            ShaderIncludeHandler SIH( strFileNameAscii );
            CreateAscii( pFileBuffer, pMacros, &SIH );
            if( pMacros != NULL )
                delete[] pMacros;
        }

        delete[] pFileBuffer;

        DWORD dwParamCount = pBaseMaterial->GetRawParameterCount();
        for( DWORD i = 0; i < dwParamCount; ++i )
        {
            CopyParamDataToEffect( m_pEffect, &pBaseMaterial->GetRawParameter( i ) );
        }

        SetNullSamplerParameters();

        dwParamIndex = pBaseMaterial->FindRawParameter( L"EffectTechnique" );
        const WCHAR* strTechniqueName = NULL;
        if( dwParamIndex != (DWORD)-1 )
            strTechniqueName = pBaseMaterial->GetRawParameter( dwParamIndex ).strValue;
        if( strTechniqueName == NULL )
            m_hTechnique = m_pEffect->GetTechniqueHandleFromIndex( 0 );
        else
            m_hTechnique = m_pEffect->GetTechniqueHandle( ConvertString( strTechniqueName ) );

        FXLTECHNIQUE_DESC TechDesc;
        m_pEffect->GetTechniqueDesc( m_hTechnique, &TechDesc );
        m_dwPassCount = (DWORD)TechDesc.Passes;
    }

    BOOL FXLiteMaterialImplementation::IsFXLiteMaterial( BaseMaterial* pMaterial )
    {
        assert( pMaterial != NULL );
        DWORD dwIndex = pMaterial->FindRawParameter( L"EffectFile" );
        return ( dwIndex != (DWORD)-1 );
    }

    D3DXMACRO* FXLiteMaterialImplementation::CreateMacroList( BaseMaterial* pBaseMaterial )
    {
        return NULL;
    }

    VOID FXLiteMaterialImplementation::CreateBinary( BYTE* pBuffer )
    {
        if( m_pEffect != NULL )
            m_pEffect->Release();

        HRESULT hr = FXLCreateEffect( NULL, pBuffer, g_pFXLiteEffectPool, &m_pEffect );
        if( FAILED( hr ) )
        {
            DebugSpew( "FXLite effect creation error.\n" );
        }
    }

    VOID FXLiteMaterialImplementation::SetNullSamplerParameters()
    {
        assert( m_pEffect != NULL );
        FXLEFFECT_DESC EffectDesc;
        m_pEffect->GetEffectDesc( &EffectDesc );
        for( DWORD i = 0; i < EffectDesc.Parameters; ++i )
        {
            FXLHANDLE hParam = m_pEffect->GetParameterHandleFromIndex( i );
            FXLPARAMETER_DESC ParamDesc;
            m_pEffect->GetParameterDesc( hParam, &ParamDesc );
            if( ParamDesc.Class == FXLDCLASS_SAMPLER ||
                ParamDesc.Class == FXLDCLASS_SAMPLER1D ||
                ParamDesc.Class == FXLDCLASS_SAMPLER2D ||
                ParamDesc.Class == FXLDCLASS_SAMPLER3D ||
                ParamDesc.Class == FXLDCLASS_SAMPLERCUBE )
            {
                D3DBaseTexture* pTex = NULL;
                m_pEffect->GetSampler( hParam, &pTex );
                if( pTex == NULL )
                    m_pEffect->SetSampler( hParam, g_pNullSamplerTexture );
            }
        }
    }

    VOID FXLiteMaterialImplementation::CreateAscii( const CHAR* strFileBuffer, const D3DXMACRO* pMacros, ShaderIncludeHandler* pShaderIncludeHandler )
    {
        ID3DXBuffer *pBuffer = NULL, *pErrorBuffer = NULL;
        DWORD dwCompileFlags = 0;

        FXLCompileEffect( strFileBuffer, strlen( strFileBuffer ), pMacros, pShaderIncludeHandler, dwCompileFlags, &pBuffer, &pErrorBuffer );

        if( pErrorBuffer != NULL )
        {
            DebugSpew( "FXLite error: %s\n", (const CHAR*)pErrorBuffer->GetBufferPointer() );
            pErrorBuffer->Release();
        }

        if( pBuffer == NULL )
        {
            return;
        }

        CreateBinary( (BYTE*)pBuffer->GetBufferPointer() );
        pBuffer->Release();
    }

    MaterialInstanceData FXLiteMaterialImplementation::CreateInstanceData( MaterialInstance* pMaterialInstance )
    {
        assert( pMaterialInstance != NULL );

        if( m_pEffect == NULL )
            return NULL;

        /*
        if( m_pEffect->m_dwDevice == 0 )
        {
            ATG_PrintWarning( "Material instance being created before FXLiteMaterial is assigned a D3D device.\n" );
        }
        */

        FXLiteMaterialInstanceData* pInstanceData = new FXLiteMaterialInstanceData;
        m_pEffect->CreateInstance( &pInstanceData->pEffect );

        DWORD dwParamCount = pMaterialInstance->GetRawParameterCount();
        for( DWORD i = 0; i < dwParamCount; ++i )
        {
            CopyParamDataToEffect( pInstanceData->pEffect, &pMaterialInstance->GetRawParameter( i ) );
        }
        return (MaterialInstanceData)pInstanceData;
    }

    VOID FXLiteMaterialImplementation::DestroyInstanceData( MaterialInstanceData Data )
    {
        FXLiteMaterialInstanceData* pInstanceData = (FXLiteMaterialInstanceData*)Data;
        assert( pInstanceData != NULL );
        assert( pInstanceData->pEffect != NULL );
        pInstanceData->pEffect->Release();

        delete pInstanceData;
    }

    VOID FXLiteMaterialImplementation::ChangeDevice( ::D3DDevice* pd3dDevice )
    {
        if( m_pEffect != NULL )
            m_pEffect->ChangeDevice( pd3dDevice );
    }

    VOID FXLiteMaterialImplementation::CopyParamDataToEffect( FXLEffect* pEffect, MaterialParameter* pParam )
    {
        assert( pParam != NULL );
        assert( pEffect != NULL );

        if( pParam->Type == MaterialParameter::RPT_String )
            return;

        const CHAR* strParamName = ConvertString( pParam->strName );
        FXLHANDLE hParam = pEffect->GetParameterHandle( strParamName );
        if( hParam == NULL )
            return;

        FXLPARAMETER_DESC desc;
        pEffect->GetParameterDesc( hParam, &desc );
        switch( desc.Type )
        {
        case FXLDTYPE_INT:
            switch( desc.Class )
            {
            case FXLDCLASS_SCALAR:
                assert( pParam->Type == MaterialParameter::RPT_Int );
                pEffect->SetScalarI( hParam, &pParam->iValue );
                break;
            }
            break;
        case FXLDTYPE_BOOL:
            switch( desc.Class )
            {
            case FXLDCLASS_SCALAR:
                if( pParam->Type == MaterialParameter::RPT_Bool )
                {
                    pEffect->SetScalarB( hParam, &pParam->bValue );
                }
                else if( pParam->Type == MaterialParameter::RPT_Int )
                {
                    BOOL bValue = ( pParam->iValue != 0 );
                    pEffect->SetScalarB( hParam, &bValue );
                }
                break;
            }
            break;
        case FXLDTYPE_FLOAT:
            switch( desc.Class )
            {
            case FXLDCLASS_SCALAR:
                assert( pParam->dwCount == 1 );
                if( pParam->Type == MaterialParameter::RPT_Float )
                {
                    pEffect->SetScalarF( hParam, &pParam->fValue );
                }
                else if( pParam->Type == MaterialParameter::RPT_Int )
                {
                    FLOAT fValue = (FLOAT)pParam->iValue;
                    pEffect->SetScalarF( hParam, &fValue );
                }
                break;
            case FXLDCLASS_VECTOR:
                assert( pParam->Type == MaterialParameter::RPT_Float );
                assert( pParam->dwCount == desc.Rows * desc.Columns );
                pEffect->SetVectorF( hParam, pParam->pFloatValues );
                break;
            }
            break;
        case FXLDTYPE_SAMPLER:
            switch( desc.Class )
            {
            case FXLDCLASS_SAMPLER2D:
            case FXLDCLASS_SAMPLER:
            case FXLDCLASS_SAMPLERCUBE:
                {
                    assert( pParam->Type == MaterialParameter::RPT_Texture2D ||
                            pParam->Type == MaterialParameter::RPT_Texture3D ||
                            pParam->Type == MaterialParameter::RPT_TextureCube );
                    Texture* pTexResource = (Texture*)pParam->pValue;
                    if( pTexResource != NULL )
                        pEffect->SetSampler( hParam, pTexResource->GetD3DTexture() );
                    else
                        pEffect->SetSampler( hParam, g_pNullSamplerTexture );
                    break;
                }
            }
            break;
        }
    }

    VOID FXLiteMaterialImplementation::BeginPass( MaterialInstanceData Data, DWORD dwIndex )
    {
        FXLiteMaterialInstanceData* pInstanceData = (FXLiteMaterialInstanceData*)Data;
        pInstanceData->pEffect->BeginPassFromIndex( dwIndex );
        pInstanceData->pEffect->Commit();
    }

    VOID FXLiteMaterialImplementation::EndPass( MaterialInstanceData Data )
    {
        FXLiteMaterialInstanceData* pInstanceData = (FXLiteMaterialInstanceData*)Data;
        pInstanceData->pEffect->EndPass();
    }

    VOID FXLiteMaterialImplementation::BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant )
    {
        FXLiteMaterialInstanceData* pInstanceData = (FXLiteMaterialInstanceData*)Data;
        assert( pInstanceData != NULL );
        pInstanceData->pEffect->ChangeDevice( pd3dDevice );
        FXLHANDLE hTechnique = m_hTechnique;
        if( dwMaterialVariant != 0 )
            hTechnique = (FXLHANDLE)dwMaterialVariant;
        pInstanceData->pEffect->BeginTechnique( hTechnique, 0 );
    }

    VOID FXLiteMaterialImplementation::EndMaterial( MaterialInstanceData Data )
    {
        FXLiteMaterialInstanceData* pInstanceData = (FXLiteMaterialInstanceData*)Data;
        pInstanceData->pEffect->EndTechnique();
    }


    //-------------------------------------------------------------------------
    // Shader Material Implementation
    //-------------------------------------------------------------------------

    ShaderMaterialImplementation::ShaderMaterialImplementation( BaseMaterial* pBaseMaterial )
        : m_pVertexShader( NULL ),
          m_pPixelShader( NULL ),
          m_pVertexShaderConstantTable( NULL ),
          m_pPixelShaderConstantTable( NULL )
    {
        // find vertex shader parameter
        DWORD dwParamIndex = pBaseMaterial->FindRawParameter( L"VertexShader" );
        if( dwParamIndex == (DWORD)-1 )
        {
            DebugSpew( "WARNING: Shader material %S does not have an VertexShader parameter.\n", pBaseMaterial->GetName().GetSafeString() );
            return;
        }

        // load vertex shader
        const WCHAR* strFileName = pBaseMaterial->GetRawParameter( dwParamIndex ).strValue;
        CHAR strFileNameAscii[ MAX_PATH ];
        CreateMediaPath( strFileNameAscii, ARRAYSIZE( strFileNameAscii ), strFileName, "Shaders", BaseMaterial::GetMediaRootPath() );

        HRESULT hr = ATG::LoadVertexShader( strFileNameAscii, &m_pVertexShader, &m_pVertexShaderConstantTable );

        if( FAILED( hr ) || m_pVertexShader == NULL )
        {
            DebugSpew( "ERROR: Could not load vertex shader for shader material %S.\n", pBaseMaterial->GetName().GetSafeString() );
            return;
        }

        // find pixel shader parameter (it's optional)
        dwParamIndex = pBaseMaterial->FindRawParameter( L"PixelShader" );
        if( dwParamIndex != -1 )
        {
            // load pixel shader
            strFileName = pBaseMaterial->GetRawParameter( dwParamIndex ).strValue;
            CreateMediaPath( strFileNameAscii, ARRAYSIZE( strFileNameAscii ), strFileName, "Shaders", BaseMaterial::GetMediaRootPath() );

            hr = ATG::LoadPixelShader( strFileNameAscii, &m_pPixelShader, &m_pPixelShaderConstantTable );

            if( FAILED( hr ) || m_pPixelShader == NULL )
            {
                DebugSpew( "ERROR: Could not load pixel shader for shader material %S.\n", pBaseMaterial->GetName().GetSafeString() );
            }
        }

        // bind all of the base material parameters
        DWORD dwParamCount = pBaseMaterial->GetRawParameterCount();
        for( DWORD i = 0; i < dwParamCount; ++i )
        {
            MaterialParameter& mp = pBaseMaterial->GetRawParameter( i );
            BindParameter( NULL, &mp );
        }
    }

    VOID ShaderMaterialImplementation::BindParameter( ShaderMaterialInstanceData* pInstanceData, MaterialParameter* pMP )
    {
        if( pMP->Type == MaterialParameter::RPT_String )
            return;

        const CHAR* strNameAscii = ConvertString( pMP->strName.GetSafeString() );

        D3DXCONSTANT_DESC ConstantDesc = { 0 };
        D3DXHANDLE hConstant = NULL;

        // Search for the parameter in the pixel shader constant table
        if( m_pPixelShaderConstantTable != NULL )
        {
            hConstant = m_pPixelShaderConstantTable->GetConstantByName( NULL, strNameAscii );
            if( hConstant != NULL )
            {
                UINT uValue = 1;
                m_pPixelShaderConstantTable->GetConstantDesc( hConstant, &ConstantDesc, &uValue );
                switch( pMP->Type )
                {
                case MaterialParameter::RPT_Bool:
                    ConstantDesc.RegisterIndex += 128;
                    break;
                case MaterialParameter::RPT_Float:
                    ConstantDesc.RegisterIndex += 256;
                    break;
                }
            }
        }
        // Search for the parameter in the vertex shader constant table
        if( hConstant == NULL && m_pVertexShaderConstantTable != NULL )
        {
            hConstant = m_pVertexShaderConstantTable->GetConstantByName( NULL, strNameAscii );
            if( hConstant != NULL )
            {
                UINT uValue = 1;
                m_pVertexShaderConstantTable->GetConstantDesc( hConstant, &ConstantDesc, &uValue );
            }
        }

        const WCHAR* strHint = pMP->strHint;
        if( ( strHint == NULL || strHint[0] == L'\0' ) && hConstant == NULL )
            return;

        // Use the hint field in the parameter to decode a shader constant register
        if( hConstant == NULL && strHint != NULL )
        {
            switch( strHint[0] )
            {
            case 's':
                ConstantDesc.Type = D3DXPT_TEXTURE;
                ConstantDesc.RegisterIndex = _wtoi( strHint + 1 );
                ConstantDesc.RegisterCount = 1;
                break;
            case 'c':
                ConstantDesc.Type = D3DXPT_FLOAT;
                ConstantDesc.RegisterIndex = _wtoi( strHint + 1 );
                ConstantDesc.RegisterCount = 1;
                break;
            }
        }

        // If the parameter was specified as an int parameter, and the shader matches it
        // to a float parameter, convert the parameter to a float parameter
        if( pMP->Type == MaterialParameter::RPT_Int && ConstantDesc.Type == D3DXPT_FLOAT )
        {
            pMP->fValue = (FLOAT)pMP->iValue;
            pMP->Type = MaterialParameter::RPT_Float;
            pMP->dwCount = 1;
            pMP->pFloatValues = NULL;
        }

        switch( pMP->Type )
        {
        case MaterialParameter::RPT_Texture2D:
        case MaterialParameter::RPT_Texture3D:
        case MaterialParameter::RPT_TextureCube:
            {
                // texture sampler parameter
                assert( ConstantDesc.Type == D3DXPT_TEXTURE ||
                        ConstantDesc.Type == D3DXPT_TEXTURE1D ||
                        ConstantDesc.Type == D3DXPT_TEXTURE2D ||
                        ConstantDesc.Type == D3DXPT_TEXTURE3D ||
                        ConstantDesc.Type == D3DXPT_SAMPLER ||
                        ConstantDesc.Type == D3DXPT_SAMPLER1D ||
                        ConstantDesc.Type == D3DXPT_SAMPLER2D ||
                        ConstantDesc.Type == D3DXPT_SAMPLER3D );
                if( pMP->pValue == NULL )
                    break;
                BoundTexture bt;
                bt.pTextureResource = (Texture*)pMP->pValue;
                DWORD dwSamplerIndex = ConstantDesc.RegisterIndex;
                assert( dwSamplerIndex < 16 );
                bt.dwSamplerIndex = dwSamplerIndex;
                if( pInstanceData != NULL )
                    pInstanceData->m_BoundTextures.push_back( bt );
                else
                    m_BoundTextures.push_back( bt );
                break;
            }
        case MaterialParameter::RPT_Float:
            {
                // float parameter
                assert( ConstantDesc.Type == D3DXPT_FLOAT );
                BoundFloatConstant bfc;
                bfc.dwConstantIndex = ConstantDesc.RegisterIndex;
                bfc.dwConstantCount = ConstantDesc.RegisterCount;
                assert( bfc.dwConstantCount < 256 );
                assert( bfc.dwConstantIndex < 512 );
                if( pMP->dwCount == 1 )
                    bfc.pFloatData = &pMP->fValue;
                else
                    bfc.pFloatData = pMP->pFloatValues;
                if( pInstanceData != NULL )
                    pInstanceData->m_BoundFloatConstants.push_back( bfc );
                else
                    m_BoundFloatConstants.push_back( bfc );
                break;
            }
        case MaterialParameter::RPT_Bool:
            {
                // float parameter
                assert( ConstantDesc.Type == D3DXPT_BOOL );
                BoundBoolConstant bbc;
                bbc.dwConstantIndex = ConstantDesc.RegisterIndex;
                bbc.dwConstantCount = ConstantDesc.RegisterCount;
                assert( bbc.dwConstantCount == 1 );
                assert( bbc.dwConstantIndex < 256 );
                bbc.pBoolData = &pMP->bValue;
                if( pInstanceData != NULL )
                    pInstanceData->m_BoundBoolConstants.push_back( bbc );
                else
                    m_BoundBoolConstants.push_back( bbc );
                break;
            }
        }
    }

    BOOL ShaderMaterialImplementation::IsShaderMaterial( BaseMaterial* pMaterial )
    {
        assert( pMaterial != NULL );
        DWORD dwIndex = pMaterial->FindRawParameter( L"VertexShader" );
        return ( dwIndex != (DWORD)-1 );
    }

    MaterialInstanceData ShaderMaterialImplementation::CreateInstanceData( MaterialInstance* pMaterialInstance )
    {
        assert( pMaterialInstance != NULL );

        DWORD dwParamCount = pMaterialInstance->GetRawParameterCount();
        if( dwParamCount == 0 )
            return NULL;

        ShaderMaterialInstanceData* pInstanceData = new ShaderMaterialInstanceData;

        for( DWORD i = 0; i < dwParamCount; ++i )
        {
            MaterialParameter& mp = pMaterialInstance->GetRawParameter( i );
            BindParameter( pInstanceData, &mp );
        }

        return (MaterialInstanceData)pInstanceData;
    }

    VOID ShaderMaterialImplementation::DestroyInstanceData( MaterialInstanceData Data )
    {
        ShaderMaterialInstanceData* pInstanceData = (ShaderMaterialInstanceData*)Data;
        if( pInstanceData != NULL )
            delete pInstanceData;
    }

    VOID ShaderMaterialImplementation::ChangeDevice( ::D3DDevice* pd3dDevice )
    {
    }

    VOID ShaderMaterialImplementation::BeginPass( MaterialInstanceData Data, DWORD dwIndex )
    {
    }

    VOID ShaderMaterialImplementation::EndPass( MaterialInstanceData Data )
    {
    }

    VOID ShaderMaterialImplementation::BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant )
    {
        // set vertex and pixel shaders
        if( ( dwMaterialVariant & DoNotSetShaders ) == 0 )
        {
            D3DVertexShader* pCurrentVertexShader = NULL;
            pd3dDevice->GetVertexShader( &pCurrentVertexShader );
            if( pCurrentVertexShader != m_pVertexShader )
            {
                pd3dDevice->SetVertexShader( m_pVertexShader );
            }
            if( pCurrentVertexShader != NULL )
            {
                pCurrentVertexShader->Release();
            }

            D3DPixelShader* pCurrentPixelShader = NULL;
            pd3dDevice->GetPixelShader( &pCurrentPixelShader );
            if( pCurrentPixelShader != m_pPixelShader )
            {
                pd3dDevice->SetPixelShader( m_pPixelShader );
            }
            if( pCurrentPixelShader != NULL )
            {
                pCurrentPixelShader->Release();
            }
        }

        // set base material parameters
        DWORD dwCount = (DWORD)m_BoundTextures.size();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            const BoundTexture& bt = m_BoundTextures[i];
            D3DBaseTexture* pTexture = bt.pTextureResource->GetD3DTexture();
            pd3dDevice->SetTexture( bt.dwSamplerIndex, pTexture );
        }
        dwCount = (DWORD)m_BoundFloatConstants.size();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            const BoundFloatConstant& bfc = m_BoundFloatConstants[i];
            if( bfc.dwConstantIndex < 256 )
                pd3dDevice->SetVertexShaderConstantF( bfc.dwConstantIndex, bfc.pFloatData, bfc.dwConstantCount );
            else
                pd3dDevice->SetPixelShaderConstantF( bfc.dwConstantIndex - 256, bfc.pFloatData, bfc.dwConstantCount );
        }
        dwCount = (DWORD)m_BoundBoolConstants.size();
        for( DWORD i = 0; i < dwCount; ++i )
        {
            const BoundBoolConstant& bbc = m_BoundBoolConstants[i];
            if( bbc.dwConstantIndex < 128 )
                pd3dDevice->SetVertexShaderConstantB( bbc.dwConstantIndex, bbc.pBoolData, bbc.dwConstantCount );
            else
                pd3dDevice->SetPixelShaderConstantB( bbc.dwConstantIndex - 128, bbc.pBoolData, bbc.dwConstantCount );
        }

        // set material instance parameters
        ShaderMaterialInstanceData* pInstanceData = (ShaderMaterialInstanceData*)Data;
        if( pInstanceData != NULL )
        {
            dwCount = (DWORD)pInstanceData->m_BoundTextures.size();
            for( DWORD i = 0; i < dwCount; ++i )
            {
                const BoundTexture& bt = pInstanceData->m_BoundTextures[i];
                D3DBaseTexture* pTexture = bt.pTextureResource->GetD3DTexture();
                pd3dDevice->SetTexture( bt.dwSamplerIndex, pTexture );
            }
            dwCount = (DWORD)pInstanceData->m_BoundFloatConstants.size();
            for( DWORD i = 0; i < dwCount; ++i )
            {
                const BoundFloatConstant& bfc = pInstanceData->m_BoundFloatConstants[i];
                if( bfc.dwConstantIndex < 256 )
                    pd3dDevice->SetVertexShaderConstantF( bfc.dwConstantIndex, bfc.pFloatData, bfc.dwConstantCount );
                else
                    pd3dDevice->SetPixelShaderConstantF( bfc.dwConstantIndex - 256, bfc.pFloatData, bfc.dwConstantCount );
            }
            dwCount = (DWORD)pInstanceData->m_BoundBoolConstants.size();
            for( DWORD i = 0; i < dwCount; ++i )
            {
                const BoundBoolConstant& bbc = pInstanceData->m_BoundBoolConstants[i];
                if( bbc.dwConstantIndex < 128 )
                    pd3dDevice->SetVertexShaderConstantB( bbc.dwConstantIndex, bbc.pBoolData, bbc.dwConstantCount );
                else
                    pd3dDevice->SetPixelShaderConstantB( bbc.dwConstantIndex - 128, bbc.pBoolData, bbc.dwConstantCount );
            }
        }
    }

    VOID ShaderMaterialImplementation::EndMaterial( MaterialInstanceData Data )
    {
    }

}
