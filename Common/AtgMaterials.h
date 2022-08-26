//-----------------------------------------------------------------------------
// AtgMaterials.h
//
// Materials classes for the scene hierarchy.  These classes support FXLite
// materials as well as pixel/vertex shader materials.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_MATERIALS_H
#define ATG_MATERIALS_H

#include <d3d9.h>

#include <fxl.h>
#include <vector>
#include "AtgResourceDatabase.h"

namespace ATG
{
    class BaseMaterial;
    class MaterialInstance;
    class Scene;

    //----------------------------------------------------------------------------------
    // Name: MaterialParameter
    // Desc: Struct to hold material parameter information that has been loaded from a
    //       scene file.
    //----------------------------------------------------------------------------------
    struct MaterialParameter
    {
    public:
        MaterialParameter()
            : strName( NULL ),
              dwCount( 1 ),
              Type( RPT_Other ),
              pValue( NULL ),
              bValue( FALSE ),
              strValue( NULL )
        {
        }
        enum ParameterType
        {
            RPT_Other = 0,
            RPT_Bool,
            RPT_Int,
            RPT_Float,
            RPT_String,
            RPT_Texture2D,
            RPT_TextureCube,
            RPT_Texture3D
        };
        StringID            strName;
        DWORD               dwCount;
        ParameterType       Type;
        union 
        {
            VOID*           pValue;
            FLOAT*          pFloatValues;
        };
        union
        {
            BOOL            bValue;
            INT             iValue;
            FLOAT           fValue;
        };
        StringID            strValue;
        StringID            strHint;
    };
    typedef std::vector<MaterialParameter> MaterialParameterVector;

    typedef VOID* MaterialInstanceData;


    //----------------------------------------------------------------------------------
    // Name: MaterialImplementation
    // Desc: Interface class that describes an implementation of a material, such as an
    //       FXLite effect or a set of shaders.  It is also responsible for generating
    //       and destroying instance data specific to material instances.
    //----------------------------------------------------------------------------------
    class MaterialImplementation : public NamedTypedObject
    {
        DEFINE_TYPE_INFO();
    public:
        virtual DWORD GetPassCount() const = NULL;
        virtual VOID BeginPass( MaterialInstanceData Data, DWORD dwIndex ) = NULL;
        virtual VOID EndPass( MaterialInstanceData Data ) = NULL;
        virtual VOID BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant ) = NULL;
        virtual VOID EndMaterial( MaterialInstanceData Data ) = NULL;
        virtual MaterialInstanceData CreateInstanceData( MaterialInstance* pMaterialInstance ) = NULL;
        virtual VOID DestroyInstanceData( MaterialInstanceData Data ) = NULL;
        virtual VOID ChangeDevice( ::D3DDevice* pd3dDevice ) = NULL;
    };


    //----------------------------------------------------------------------------------
    // Name: ShaderIncludeHandler
    // Desc: Boilerplate code to process include files for the D3DX shader compiler and
    //       FXLite shader compiler.
    //----------------------------------------------------------------------------------
    class ShaderIncludeHandler : public ID3DXInclude
    {
    public:
        ShaderIncludeHandler( const CHAR* strBasePath );
        HRESULT __stdcall Open( D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData,
                      LPCVOID* ppData, UINT* pBytes, /* OUT */ LPSTR pFullPath, DWORD cbFullPath);
        HRESULT __stdcall Close( LPCVOID pData );
    protected:
        CHAR        m_strBasePath[ MAX_PATH ];
    };


    //----------------------------------------------------------------------------------
    // Name: FXLiteMaterialImplementation
    // Desc: A material implementation class that uses FXLite to set up shaders and
    //       renderstate.
    //----------------------------------------------------------------------------------
    class FXLiteMaterialImplementation : public MaterialImplementation
    {
        DEFINE_TYPE_INFO();
    public:
        FXLiteMaterialImplementation( BaseMaterial* pBaseMaterial );
        ~FXLiteMaterialImplementation();

        static BOOL IsFXLiteMaterial( BaseMaterial* pMaterial );

        virtual DWORD GetPassCount() const { return m_dwPassCount; }
        virtual VOID BeginPass( MaterialInstanceData Data, DWORD dwIndex );
        virtual VOID EndPass( MaterialInstanceData Data );
        virtual VOID BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant );
        virtual VOID EndMaterial( MaterialInstanceData Data );
        virtual MaterialInstanceData CreateInstanceData( MaterialInstance* pMaterialInstance );
        virtual VOID DestroyInstanceData( MaterialInstanceData Data );
        virtual VOID ChangeDevice( ::D3DDevice* pd3dDevice );

        static VOID SetParameterPool( FXLEffectPool* pEffectPool );
        static VOID SetNullSamplerTexture( D3DTexture* pTexture );

    protected:
        D3DXMACRO* CreateMacroList( BaseMaterial* pBaseMaterial );
        VOID CreateBinary( BYTE* pBuffer );
        VOID CreateAscii( const CHAR* strFileBuffer, const D3DXMACRO* pMacros, ShaderIncludeHandler* pShaderIncludeHandler );
        static VOID CopyParamDataToEffect( FXLEffect* pEffect, MaterialParameter* pParam );
        VOID SetNullSamplerParameters();

    public:
        DWORD           m_dwPassCount;
        FXLHANDLE       m_hTechnique;
        FXLEffect*      m_pEffect;

        struct FXLiteMaterialInstanceData
        {
            FXLEffect*  pEffect;
        };
    };


    //----------------------------------------------------------------------------------
    // Name: ShaderMaterialImplementation
    // Desc: A material implementation class that uses vertex and pixel shaders.
    //----------------------------------------------------------------------------------
    class ShaderMaterialImplementation : public MaterialImplementation
    {
        DEFINE_TYPE_INFO();
    public:
        enum MaterialFlags
        {
            DoNotSetShaders = 1,
        };
    public:
        ShaderMaterialImplementation( BaseMaterial* pBaseMaterial );
        ~ShaderMaterialImplementation();

        static BOOL IsShaderMaterial( BaseMaterial* pMaterial );

        virtual DWORD GetPassCount() const { return 1; }
        virtual VOID BeginPass( MaterialInstanceData Data, DWORD dwIndex );
        virtual VOID EndPass( MaterialInstanceData Data );
        virtual VOID BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant );
        virtual VOID EndMaterial( MaterialInstanceData Data );
        virtual MaterialInstanceData CreateInstanceData( MaterialInstance* pMaterialInstance );
        virtual VOID DestroyInstanceData( MaterialInstanceData Data );
        virtual VOID ChangeDevice( ::D3DDevice* pd3dDevice );
    protected:
        struct BoundTexture
        {
            DWORD       dwSamplerIndex;
            Texture*    pTextureResource;
        };
        typedef std::vector<BoundTexture>   BoundTextureVector;

        struct BoundFloatConstant
        {
            DWORD           dwConstantIndex;
            DWORD           dwConstantCount;
            const FLOAT*    pFloatData;
        };
        typedef std::vector<BoundFloatConstant>     BoundFloatConstantVector;

        struct BoundBoolConstant
        {
            DWORD           dwConstantIndex;
            DWORD           dwConstantCount;
            const BOOL*     pBoolData;
        };
        typedef std::vector<BoundBoolConstant>     BoundBoolConstantVector;

        struct ShaderMaterialInstanceData
        {
            BoundTextureVector          m_BoundTextures;
            BoundFloatConstantVector    m_BoundFloatConstants;
            BoundBoolConstantVector     m_BoundBoolConstants;
        };

        VOID BindParameter( ShaderMaterialInstanceData* pInstanceData, MaterialParameter* pMP );

    public:
        D3DVertexShader*            m_pVertexShader;
        D3DPixelShader*             m_pPixelShader;
        ID3DXConstantTable*         m_pVertexShaderConstantTable;
        ID3DXConstantTable*         m_pPixelShaderConstantTable;
        BoundTextureVector          m_BoundTextures;
        BoundFloatConstantVector    m_BoundFloatConstants;
        BoundBoolConstantVector     m_BoundBoolConstants;
    };


    //----------------------------------------------------------------------------------
    // Name: BaseMaterial
    // Desc: Class that represents a single material type in the resource database.
    //       BaseMaterial holds data that is common to all instances of a material type,
    //       such as shaders and renderstate.
    //       Most material data is held in material instances, which are used by models
    //       in the scene.
    //----------------------------------------------------------------------------------
    class BaseMaterial : public Resource
    {
        DEFINE_TYPE_INFO();
    public:
        BaseMaterial();
        ~BaseMaterial();

        static BaseMaterial* CreateFXLiteMaterial( StringID MaterialName, 
                                                   StringID FXLiteFilePath, 
                                                   StringID TechniqueName = NULL );

        static BaseMaterial* CreateShaderMaterial( StringID MaterialName,
                                                   StringID VertexShaderFileName,
                                                   StringID VertexShaderEntryPoint,
                                                   StringID PixelShaderFileName = NULL,
                                                   StringID PixelShaderEntryPoint = NULL );

        DWORD GetRawParameterCount() const { return (DWORD)m_RawParameters.size(); }
        DWORD AddRawParameter( const MaterialParameter& RawParam ) { m_RawParameters.push_back( RawParam ); return GetRawParameterCount() - 1; }
        MaterialParameter& GetRawParameter( DWORD dwIndex ) { return m_RawParameters[dwIndex]; }
        DWORD FindRawParameter( const StringID Name ) const;

        MaterialInstanceData InitializeInstance( MaterialInstance* pMaterialInstance );

        static VOID SetMediaRootPath( const CHAR* strMediaRootPath );
        static const CHAR* GetMediaRootPath() { return m_strMediaRootPath; }

        MaterialImplementation* GetMaterialImplementation() const { return m_pMaterialImplementation; }
        VOID InitializeImplementation();
        VOID ChangeDevice( ::D3DDevice* pd3dDevice );

        VOID BindTextures( const CHAR* strMediaRootPath, Scene* pScene );

        DWORD GetPassCount() const;
        VOID BeginPass( MaterialInstanceData Data, DWORD dwIndex );
        VOID EndPass( MaterialInstanceData Data );
        VOID BeginMaterial( MaterialInstanceData Data, ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant );
        VOID EndMaterial( MaterialInstanceData Data );

    protected:
        MaterialParameterVector     m_RawParameters;
        MaterialImplementation*     m_pMaterialImplementation;
        BOOL                        m_bTexturesBound;
        static CHAR                 m_strMediaRootPath[ MAX_PATH ];
    };


    //----------------------------------------------------------------------------------
    // Name: MaterialInstance
    // Desc: Class that represents an instance of a material.  For example, a material
    //       might be a particular vertex and pixel shader, where an instance would be
    //       the textures and renderstates used with those shaders for rendering a
    //       particular model.
    //----------------------------------------------------------------------------------
    class MaterialInstance : public NamedTypedObject
    {
        DEFINE_TYPE_INFO();
    public:
        MaterialInstance();
        ~MaterialInstance();

        DWORD GetRawParameterCount() const { return (DWORD)m_RawParameters.size(); }
        DWORD AddRawParameter( const MaterialParameter& RawParam ) { m_RawParameters.push_back( RawParam ); return GetRawParameterCount() - 1; }
        MaterialParameter& GetRawParameter( DWORD dwIndex ) { return m_RawParameters[dwIndex]; }
        DWORD FindRawParameter( const StringID Name ) const;

        VOID SetMaterialBindingIndex( DWORD dwIndex = 0 );
        DWORD GetMaterialBindingIndex() const { return m_dwCurrentBindingIndex; }
        VOID AllocateMaterialBindings( DWORD dwMaxCount );
        DWORD GetMaterialBindingCount() const { return (DWORD)m_MaterialBindings.size(); }

        VOID SetBaseMaterial( BaseMaterial* pBM );
        BaseMaterial* GetBaseMaterial() const { return m_MaterialBindings[m_dwCurrentBindingIndex].pMaterial; }

        VOID SetBaseMaterialName( StringID Name ) { m_OriginalMaterialName = Name; }
        StringID GetBaseMaterialName() const { return m_OriginalMaterialName; }

        VOID Initialize();
        VOID BindTextures( const CHAR* strMediaRootPath, Scene* pScene );

        DWORD GetPassCount() const;
        VOID BeginPass( DWORD dwIndex );
        VOID EndPass();
        VOID BeginMaterial( ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant = 0 );
        VOID EndMaterial();

        VOID BeginMaterialSinglePass( ::D3DDevice* pd3dDevice, DWORD dwMaterialVariant = 0 );
        VOID EndMaterialSinglePass();

        BOOL IsTransparent() const { return m_bTransparent; }
        VOID SetTransparent( BOOL bTransparent ) { m_bTransparent = bTransparent; }

    protected:
        MaterialInstanceData GetInstanceData() const { return m_MaterialBindings[m_dwCurrentBindingIndex].pMaterialInstanceData; }

        struct MaterialPair
        {
            BaseMaterial*           pMaterial;
            MaterialInstanceData    pMaterialInstanceData;
        };
        typedef std::vector<MaterialPair> MaterialPairVector;
        MaterialPairVector          m_MaterialBindings;
        DWORD                       m_dwCurrentBindingIndex;

        StringID                    m_OriginalMaterialName;
        MaterialParameterVector     m_RawParameters;
        BOOL                        m_bTexturesBound;
        BOOL                        m_bTransparent;
    };
}

#endif
