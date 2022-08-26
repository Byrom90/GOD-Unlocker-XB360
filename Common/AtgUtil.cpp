//--------------------------------------------------------------------------------------
// AtgUtil.cpp
//
// Helper functions and typing shortcuts for samples
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgUtil.h"

namespace ATG
{

// Global access to the main D3D device
extern D3DDevice* g_pd3dDevice;

// Static shaders used for helper functions
static D3DVertexDeclaration* g_pGradientVertexDecl = NULL;
static D3DVertexShader* g_pGradientVertexShader = NULL;
static D3DPixelShader* g_pGradientPixelShader = NULL;

// Structure used to name threads
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;     // must be 0x1000
    LPCSTR szName;    // pointer to name (in user address space)
    DWORD dwThreadID; // thread ID (-1 = caller thread)
    DWORD dwFlags;    // reserved for future use, must be zero
} THREADNAME_INFO;

// Linear to high-precision texture format mapping table. Maps GPU standard formats to equivalent high-precision sampling format
// (i.e. _AS_16_16_16_16 etc). Used to create good sRGB formats for textures.
// Any entry that has no mapping just maps to the same format value.
const DWORD g_MapLinearToSrgbGpuFormat[] = 
{
    GPUTEXTUREFORMAT_1_REVERSE,
    GPUTEXTUREFORMAT_1,
    GPUTEXTUREFORMAT_8,
    GPUTEXTUREFORMAT_1_5_5_5,
    GPUTEXTUREFORMAT_5_6_5,
    GPUTEXTUREFORMAT_6_5_5,
    GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16,
    GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16,
    GPUTEXTUREFORMAT_8_A,
    GPUTEXTUREFORMAT_8_B,
    GPUTEXTUREFORMAT_8_8,
    GPUTEXTUREFORMAT_Cr_Y1_Cb_Y0_REP,     
    GPUTEXTUREFORMAT_Y1_Cr_Y0_Cb_REP,      
    GPUTEXTUREFORMAT_16_16_EDRAM,          
    GPUTEXTUREFORMAT_8_8_8_8_A,
    GPUTEXTUREFORMAT_4_4_4_4,
    GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16,
    GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16,
    GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16,
    GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16,  
    GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16,
    GPUTEXTUREFORMAT_16_16_16_16_EDRAM,
    GPUTEXTUREFORMAT_24_8,
    GPUTEXTUREFORMAT_24_8_FLOAT,
    GPUTEXTUREFORMAT_16,
    GPUTEXTUREFORMAT_16_16,
    GPUTEXTUREFORMAT_16_16_16_16,
    GPUTEXTUREFORMAT_16_EXPAND,
    GPUTEXTUREFORMAT_16_16_EXPAND,
    GPUTEXTUREFORMAT_16_16_16_16_EXPAND,
    GPUTEXTUREFORMAT_16_FLOAT,
    GPUTEXTUREFORMAT_16_16_FLOAT,
    GPUTEXTUREFORMAT_16_16_16_16_FLOAT,
    GPUTEXTUREFORMAT_32,
    GPUTEXTUREFORMAT_32_32,
    GPUTEXTUREFORMAT_32_32_32_32,
    GPUTEXTUREFORMAT_32_FLOAT,
    GPUTEXTUREFORMAT_32_32_FLOAT,
    GPUTEXTUREFORMAT_32_32_32_32_FLOAT,
    GPUTEXTUREFORMAT_32_AS_8,
    GPUTEXTUREFORMAT_32_AS_8_8,
    GPUTEXTUREFORMAT_16_MPEG,
    GPUTEXTUREFORMAT_16_16_MPEG,
    GPUTEXTUREFORMAT_8_INTERLACED,
    GPUTEXTUREFORMAT_32_AS_8_INTERLACED,
    GPUTEXTUREFORMAT_32_AS_8_8_INTERLACED,
    GPUTEXTUREFORMAT_16_INTERLACED,
    GPUTEXTUREFORMAT_16_MPEG_INTERLACED,
    GPUTEXTUREFORMAT_16_16_MPEG_INTERLACED,
    GPUTEXTUREFORMAT_DXN,
    GPUTEXTUREFORMAT_8_8_8_8_AS_16_16_16_16,
    GPUTEXTUREFORMAT_DXT1_AS_16_16_16_16,
    GPUTEXTUREFORMAT_DXT2_3_AS_16_16_16_16,
    GPUTEXTUREFORMAT_DXT4_5_AS_16_16_16_16,
    GPUTEXTUREFORMAT_2_10_10_10_AS_16_16_16_16,
    GPUTEXTUREFORMAT_10_11_11_AS_16_16_16_16,
    GPUTEXTUREFORMAT_11_11_10_AS_16_16_16_16,
    GPUTEXTUREFORMAT_32_32_32_FLOAT,
    GPUTEXTUREFORMAT_DXT3A,
    GPUTEXTUREFORMAT_DXT5A,
    GPUTEXTUREFORMAT_CTX1,
    GPUTEXTUREFORMAT_DXT3A_AS_1_1_1_1,
    GPUTEXTUREFORMAT_8_8_8_8_GAMMA_EDRAM,
    GPUTEXTUREFORMAT_2_10_10_10_FLOAT_EDRAM,
};


//--------------------------------------------------------------------------------------
// Name: DebugSpewV()
// Desc: Internal helper function
//--------------------------------------------------------------------------------------
static VOID DebugSpewV( const CHAR* strFormat, const va_list pArgList )
{
    CHAR str[2048];
    // Use the secure CRT to avoid buffer overruns. Specify a count of
    // _TRUNCATE so that too long strings will be silently truncated
    // rather than triggering an error.
    _vsnprintf_s( str, _TRUNCATE, strFormat, pArgList );
    OutputDebugStringA( str );
}


//--------------------------------------------------------------------------------------
// Name: DebugSpew()
// Desc: Prints formatted debug spew
//--------------------------------------------------------------------------------------
#ifdef  _Printf_format_string_  // VC++ 2008 and later support this annotation
VOID CDECL DebugSpew( _In_z_ _Printf_format_string_ const CHAR* strFormat, ... )
#else
VOID CDECL DebugSpew( const CHAR* strFormat, ... )
#endif
{
    va_list pArgList;
    va_start( pArgList, strFormat );
    DebugSpewV( strFormat, pArgList );
    va_end( pArgList );
}


//--------------------------------------------------------------------------------------
// Name: FatalError()
// Desc: Prints formatted debug spew and breaks into the debugger. Exits the application.
//--------------------------------------------------------------------------------------
#ifdef  _Printf_format_string_  // VC++ 2008 and later support this annotation
VOID CDECL FatalError( _In_z_ _Printf_format_string_ const CHAR* strFormat, ... )
#else
VOID CDECL FatalError( const CHAR* strFormat, ... )
#endif
{
    va_list pArgList;
    va_start( pArgList, strFormat );
    DebugSpewV( strFormat, pArgList );
    va_end( pArgList );

    DebugBreak();

    exit(0);
}

//--------------------------------------------------------------------------------------
// Name: GetAs16SRGBFormat()
// Desc: Get an sRGB format that's good for sampling.
//--------------------------------------------------------------------------------------
D3DFORMAT GetAs16SRGBFormat( D3DFORMAT fmtBase )
{
      return ( D3DFORMAT )(
        (fmtBase & ~(D3DFORMAT_TEXTUREFORMAT_MASK | D3DFORMAT_SIGNX_MASK | D3DFORMAT_SIGNY_MASK | D3DFORMAT_SIGNZ_MASK)) |
            (g_MapLinearToSrgbGpuFormat[ (fmtBase & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT] << D3DFORMAT_TEXTUREFORMAT_SHIFT) |
            (GPUSIGN_GAMMA << D3DFORMAT_SIGNX_SHIFT) |
            (GPUSIGN_GAMMA << D3DFORMAT_SIGNY_SHIFT) |
            (GPUSIGN_GAMMA << D3DFORMAT_SIGNZ_SHIFT)
      );
}

//--------------------------------------------------------------------------------------
// Name: GetAs16SRGBFormatGPU()
// Desc: Get a GPU sRGB format that's good for sampling.
//--------------------------------------------------------------------------------------
DWORD GetAs16SRGBFormatGPU( D3DFORMAT fmtBase )
{
    return g_MapLinearToSrgbGpuFormat[ (fmtBase & D3DFORMAT_TEXTUREFORMAT_MASK) >> D3DFORMAT_TEXTUREFORMAT_SHIFT ];
}

//--------------------------------------------------------------------------------------
// Name: ConvertTextureToAs16SRGBFormat()
// Desc: Get a GPU sRGB format that's good for sampling.
//--------------------------------------------------------------------------------------
void ConvertTextureToAs16SRGBFormat( D3DTexture *pTexture )
{
    // First thing, mark the texture SignX, SignY and SignZ as sRGB.
    pTexture->Format.SignX = GPUSIGN_GAMMA;
    pTexture->Format.SignY = GPUSIGN_GAMMA;
    pTexture->Format.SignZ = GPUSIGN_GAMMA;

    // Get the texture format...
    XGTEXTURE_DESC desc;
    XGGetTextureDesc( pTexture, 0, &desc );

    // ...and convert it to a "good" format (AS_16_16_16_16).
    pTexture->Format.DataFormat = GetAs16SRGBFormatGPU( desc.Format );
}


//--------------------------------------------------------------------------------------
// Name: GetVideoSettings()
// Desc: Returns the default display mode of 720P, and queries hardware for Widescreen
//--------------------------------------------------------------------------------------
VOID GetVideoSettings( UINT* pdwDisplayWidth, UINT* pdwDisplayHeight,
                       BOOL* pbWidescreen )
{
    // Query the hardware for video settings so we can determine if WideScreen is enabled.
    XVIDEO_MODE VideoMode;
    XGetVideoMode( &VideoMode );
    if( pbWidescreen )       ( *pbWidescreen ) = VideoMode.fIsWideScreen;
    
    // Resolution is always 720P. The hardware scaler will up or down-scale as needed.
    if( pdwDisplayWidth )    ( *pdwDisplayWidth ) = 1280;
    if( pdwDisplayHeight )   ( *pdwDisplayHeight ) = 720;
}


//--------------------------------------------------------------------------------------
// Name: GetTitleSafeArea()
// Desc: Returns the title safe area for the given display mode
//--------------------------------------------------------------------------------------
D3DRECT GetTitleSafeArea()
{
    D3DDISPLAYMODE mode;
    g_pd3dDevice->GetDisplayMode( 0, &mode );

    D3DRECT rcSafeArea;
    rcSafeArea.x1 = ( LONG )( mode.Width * 0.1f );
    rcSafeArea.y1 = ( LONG )( mode.Height * 0.1f );
    rcSafeArea.x2 = ( LONG )( mode.Width * 0.9f );
    rcSafeArea.y2 = ( LONG )( mode.Height * 0.9f );
    return rcSafeArea;
}


//--------------------------------------------------------------------------------------
// Name: CreateNormalizationCubeMap()
// Desc: Creates a cubemap and fills it with normalized UVW vectors
//--------------------------------------------------------------------------------------
HRESULT CreateNormalizationCubeMap( DWORD dwSize, D3DCubeTexture** ppCubeMap )
{
    // Create the cube map
    HRESULT hr = g_pd3dDevice->CreateCubeTexture( dwSize, 1, 0, D3DFMT_LIN_Q8W8V8U8,
                                                  D3DPOOL_DEFAULT, ppCubeMap, NULL );
    if( FAILED( hr ) )
    {
        ATG_PrintError( "CreateCubeTexture() failed.\n" );
        return hr;
    }

    // Allocate temp space for swizzling the cubemap surfaces
    DWORD* pSourceBits = new DWORD[ dwSize * dwSize ];

    // Fill all six sides of the cubemap
    for( DWORD i = 0; i < 6; i++ )
    {
        // Lock the i'th cubemap surface
        D3DSurface* pCubeMapFace;
        ( *ppCubeMap )->GetCubeMapSurface( ( D3DCUBEMAP_FACES )i, 0, &pCubeMapFace );

        // Write the RGBA-encoded normals to the surface pixels
        DWORD* pPixel = pSourceBits;
        FLOAT w, h;

        for( DWORD y = 0; y < dwSize; y++ )
        {
            h = ( FLOAT )y / ( FLOAT )( dwSize - 1 );  // 0 to 1
            h = ( h * 2.0f ) - 1.0f;           // -1 to 1

            for( DWORD x = 0; x < dwSize; x++ )
            {
                w = ( FLOAT )x / ( FLOAT )( dwSize - 1 );   // 0 to 1
                w = ( w * 2.0f ) - 1.0f;            // -1 to 1

                XMFLOAT3A n;
                memset(&n, 0, sizeof(n));

                // Calc the normal for this texel
                switch( i )
                {
                    case D3DCUBEMAP_FACE_POSITIVE_X:    // +x
                        n.x = +1.0;
                        n.y = -h;
                        n.z = -w;
                        break;

                    case D3DCUBEMAP_FACE_NEGATIVE_X:    // -x
                        n.x = -1.0;
                        n.y = -h;
                        n.z = +w;
                        break;

                    case D3DCUBEMAP_FACE_POSITIVE_Y:    // y
                        n.x = +w;
                        n.y = +1.0;
                        n.z = +h;
                        break;

                    case D3DCUBEMAP_FACE_NEGATIVE_Y:    // -y
                        n.x = +w;
                        n.y = -1.0;
                        n.z = -h;
                        break;

                    case D3DCUBEMAP_FACE_POSITIVE_Z:    // +z
                        n.x = +w;
                        n.y = -h;
                        n.z = +1.0;
                        break;

                    case D3DCUBEMAP_FACE_NEGATIVE_Z:    // -z
                        n.x = -w;
                        n.y = -h;
                        n.z = -1.0;
                        break;
                }

                // Store the normal as an signed QWVU vector
                XMVECTOR t = XMVector3Normalize( XMLoadFloat3A( &n ) );
                *pPixel++ = VectorToQWVU( t );
            }
        }

        // Swizzle the result into the cubemap face surface
        D3DLOCKED_RECT lock;
        pCubeMapFace->LockRect( &lock, 0, 0L );
        memcpy( lock.pBits, pSourceBits, dwSize * dwSize * sizeof( DWORD ) );
        pCubeMapFace->UnlockRect();

        // Release the cubemap face
        pCubeMapFace->Release();
    }

    delete [] pSourceBits;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: LoadFile()
// Desc: Helper function to load a file
//--------------------------------------------------------------------------------------
HRESULT LoadFile( const CHAR* strFileName, VOID** ppFileData, DWORD* pdwFileSize )
{
    assert( ppFileData );
    if( pdwFileSize )
        *pdwFileSize = 0L;

    // Open the file for reading
    HANDLE hFile = CreateFile( strFileName, GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
        return E_HANDLE;

    DWORD dwFileSize = GetFileSize( hFile, NULL );
    VOID* pFileData = malloc( dwFileSize );

    if( NULL == pFileData )
    {
        CloseHandle( hFile );
        return E_OUTOFMEMORY;
    }

    DWORD dwBytesRead;
    if( !ReadFile( hFile, pFileData, dwFileSize, &dwBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        free( pFileData );
        return E_FAIL;
    }

    // Finished reading file
    CloseHandle( hFile );

    if( dwBytesRead != dwFileSize )
    {
        free( pFileData );
        return E_FAIL;
    }

    if( pdwFileSize )
        *pdwFileSize = dwFileSize;
    *ppFileData = pFileData;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: UnloadFile()
// Desc: Matching unload
//--------------------------------------------------------------------------------------
VOID UnloadFile( VOID* pFileData )
{
    assert( pFileData != NULL );
    free( pFileData );
}


//--------------------------------------------------------------------------------------
// Name: LoadFilePhysicalMemory()
// Desc: Helper function to load a file into physicall memory
//--------------------------------------------------------------------------------------
HRESULT LoadFilePhysicalMemory( const CHAR* strFileName, VOID** ppFileData,
                                DWORD* pdwFileSize, DWORD dwAlignment )
{
    assert( ppFileData );
    if( pdwFileSize )
        *pdwFileSize = 0L;

    // Open the file for reading
    HANDLE hFile = CreateFile( strFileName, GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, NULL );

    if( INVALID_HANDLE_VALUE == hFile )
        return E_HANDLE;

    DWORD dwFileSize = GetFileSize( hFile, NULL );
    VOID* pFileData = XPhysicalAlloc( dwFileSize, MAXULONG_PTR, dwAlignment, PAGE_READWRITE );

    if( NULL == pFileData )
    {
        CloseHandle( hFile );
        return E_OUTOFMEMORY;
    }

    DWORD dwBytesRead;
    if( !ReadFile( hFile, pFileData, dwFileSize, &dwBytesRead, NULL ) )
    {
        CloseHandle( hFile );
        XPhysicalFree( pFileData );
        return E_FAIL;
    }

    // Finished reading file
    CloseHandle( hFile );

    if( dwBytesRead != dwFileSize )
    {
        XPhysicalFree( pFileData );
        return E_FAIL;
    }

    if( pdwFileSize )
        *pdwFileSize = dwFileSize;
    *ppFileData = pFileData;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: UnloadFilePhysicalMemory()
// Desc: Matching unload
//--------------------------------------------------------------------------------------
VOID UnloadFilePhysicalMemory( VOID* pFileData )
{
    assert( pFileData != NULL );
    XPhysicalFree( pFileData );
}


//--------------------------------------------------------------------------------------
// Name: SaveFile()
// Desc: Helper function to save a file
//--------------------------------------------------------------------------------------
HRESULT SaveFile( const CHAR* strFileName, VOID* pFileData, DWORD dwFileSize )
{
    // Open the file for reading
    HANDLE hFile = CreateFile( strFileName, GENERIC_WRITE, 0, NULL,
                               CREATE_ALWAYS, 0, NULL );
    if( INVALID_HANDLE_VALUE == hFile )
        return E_HANDLE;

    DWORD dwBytesWritten;
    WriteFile( hFile, pFileData, dwFileSize, &dwBytesWritten, NULL );

    // Finished reading file
    CloseHandle( hFile );

    if( dwBytesWritten != dwFileSize )
        return E_FAIL;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: SetVertexElement()
// Desc: Helper function for creating vertex declarations
//--------------------------------------------------------------------------------------
inline D3DVERTEXELEMENT9 SetVertexElement( WORD& Offset, DWORD Type,
                                           BYTE Usage, BYTE UsageIndex )
{
    D3DVERTEXELEMENT9 Element;
    Element.Stream = 0;
    Element.Offset = Offset;
    Element.Type = Type;
    Element.Method = D3DDECLMETHOD_DEFAULT;
    Element.Usage = Usage;
    Element.UsageIndex = UsageIndex;

    switch( Type )
    {
        case D3DDECLTYPE_FLOAT1:   Offset += 1*sizeof(FLOAT); break;
        case D3DDECLTYPE_FLOAT2:   Offset += 2*sizeof(FLOAT); break;
        case D3DDECLTYPE_FLOAT3:   Offset += 3*sizeof(FLOAT); break;
        case D3DDECLTYPE_FLOAT4:   Offset += 4*sizeof(FLOAT); break;
        case D3DDECLTYPE_D3DCOLOR: Offset += 1*sizeof(DWORD); break;
    }
    return Element;
}


//--------------------------------------------------------------------------------------
// Name: BuildVertexDeclFromFVF()
// Desc: Helper function to create vertex declarations
//--------------------------------------------------------------------------------------
VOID BuildVertexDeclFromFVF( DWORD dwFVF, D3DVERTEXELEMENT9* pDecl )
{
    WORD wOffset = 0;

    // Handle position
    switch( dwFVF & D3DFVF_POSITION_MASK )
    {
        case D3DFVF_XYZ:
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0 ); break;
        case D3DFVF_XYZW:
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_POSITION, 0 ); break;
        case D3DFVF_XYZB1:
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0 );
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_BLENDWEIGHT, 0 ); break;
        case D3DFVF_XYZB2:
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0 );
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_BLENDWEIGHT, 0 ); break;
        case D3DFVF_XYZB3:
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0 );
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_BLENDWEIGHT, 0 ); break;
        case D3DFVF_XYZB4:
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0 );
            *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0 ); break;
    }

    // Handle normal, diffuse, and specular
    if( dwFVF & D3DFVF_NORMAL )    *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0 );
    if( dwFVF & D3DFVF_DIFFUSE )   *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0 );
    if( dwFVF & D3DFVF_SPECULAR )  *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1 );

    // Handle texture coordinates
    DWORD dwNumTextures = ( dwFVF & D3DFVF_TEXCOUNT_MASK ) >> D3DFVF_TEXCOUNT_SHIFT;

    for( DWORD i = 0; i < dwNumTextures; i++ )
    {
        LONG lTexCoordSize = ( dwFVF & ( 0x00030000 << ( i * 2 ) ) );

        if( lTexCoordSize == D3DFVF_TEXCOORDSIZE1(i) ) *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT1, D3DDECLUSAGE_TEXCOORD, (BYTE)i );
        if( lTexCoordSize == D3DFVF_TEXCOORDSIZE2(i) ) *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, (BYTE)i );
        if( lTexCoordSize == D3DFVF_TEXCOORDSIZE3(i) ) *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TEXCOORD, (BYTE)i );
        if( lTexCoordSize == D3DFVF_TEXCOORDSIZE4(i) ) *pDecl++ = SetVertexElement( wOffset, D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_TEXCOORD, (BYTE)i );
    }

    // End the declarator
    pDecl->Stream = 0xff;
    pDecl->Offset = 0;
    pDecl->Type = ( DWORD )D3DDECLTYPE_UNUSED;
    pDecl->Method = 0;
    pDecl->Usage = 0;
    pDecl->UsageIndex = 0;
}



//--------------------------------------------------------------------------------------
// Vertex and pixel shaders for gradient background rendering
//--------------------------------------------------------------------------------------
static const CHAR* g_strGradientShader =
    "struct VS_IN                                              \n"
    "{                                                         \n"
    "   float4   Position     : POSITION;                      \n"
    "   float4   Color        : COLOR0;                        \n"
    "};                                                        \n"
    "                                                          \n"
    "struct VS_OUT                                             \n"
    "{                                                         \n"
    "   float4 Position       : POSITION;                      \n"
    "   float4 Diffuse        : COLOR0;                        \n"
    "};                                                        \n"
    "                                                          \n"
    "VS_OUT GradientVertexShader( VS_IN In )                   \n"
    "{                                                         \n"
    "   VS_OUT Out;                                            \n"
    "   Out.Position = In.Position;                            \n"
    "   Out.Diffuse  = In.Color;                               \n"
    "   return Out;                                            \n"
    "}                                                         \n"
    "                                                          \n"
    "                                                          \n"
    "float4 GradientPixelShader( VS_OUT In ) : COLOR0          \n"
    "{                                                         \n"
    "   return In.Diffuse;                                     \n"
    "}                                                         \n";


//--------------------------------------------------------------------------------------
// Name: CreateGradientShaders()
// Desc: Creates the global gradient shaders
//--------------------------------------------------------------------------------------
HRESULT CreateGradientShaders()
{
    // Create vertex declaration
    if( NULL == g_pGradientVertexDecl )
    {
        static const D3DVERTEXELEMENT9 decl[] =
        {
            { 0,  0, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
            D3DDECL_END()
        };

        if( FAILED( g_pd3dDevice->CreateVertexDeclaration( decl, &g_pGradientVertexDecl ) ) )
            return E_FAIL;
    }

    // Create vertex shader
    if( NULL == g_pGradientVertexShader )
    {
        ID3DXBuffer* pShaderCode;
        if( FAILED( D3DXCompileShader( g_strGradientShader, strlen( g_strGradientShader ),
                                       NULL, NULL, "GradientVertexShader", "vs.2.0", 0,
                                       &pShaderCode, NULL, NULL ) ) )
            return E_FAIL;

        if( FAILED( g_pd3dDevice->CreateVertexShader( ( DWORD* )pShaderCode->GetBufferPointer(),
                                                      &g_pGradientVertexShader ) ) )
            return E_FAIL;

        pShaderCode->Release();
    }

    // Create pixel shader.
    if( NULL == g_pGradientPixelShader )
    {
        ID3DXBuffer* pShaderCode;
        if( FAILED( D3DXCompileShader( g_strGradientShader, strlen( g_strGradientShader ),
                                       NULL, NULL, "GradientPixelShader", "ps.2.0", 0,
                                       &pShaderCode, NULL, NULL ) ) )
            return E_FAIL;

        if( FAILED( g_pd3dDevice->CreatePixelShader( ( DWORD* )pShaderCode->GetBufferPointer(),
                                                     &g_pGradientPixelShader ) ) )
            return E_FAIL;

        pShaderCode->Release();
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: RenderBackground()
// Desc: Draws a gradient filled background
//--------------------------------------------------------------------------------------
VOID RenderBackground( DWORD dwTopColor, DWORD dwBottomColor )
{
    // Save state
    DWORD dwZFunc;
    g_pd3dDevice->GetRenderState( D3DRS_ZFUNC, &dwZFunc );

    // Set state
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );

    // Draw a background-filling quad
    struct VERTEX
    {
        FLOAT sx, sy, sz, rhw;
        DWORD Color;
    };

    VERTEX v[3] =
    {
        { -1.0f,  1.0f, 1.0f, 1.0f, dwTopColor },
        {  1.0f,  1.0f, 1.0f, 1.0f, dwTopColor },
        { -1.0f, -1.0f, 1.0f, 1.0f, dwBottomColor },
    };

    // The shaders will be created on the first call to CreateGradientShaders()
    // and then re-used for subsequent calls
    if( FAILED( CreateGradientShaders() ) )
        FatalError( "Couldn't create shaders for RenderBackground" );

    g_pd3dDevice->SetVertexDeclaration( g_pGradientVertexDecl );
    g_pd3dDevice->SetVertexShader( g_pGradientVertexShader );
    g_pd3dDevice->SetPixelShader( g_pGradientPixelShader );
    g_pd3dDevice->DrawPrimitiveUP( D3DPT_RECTLIST, 1, v, sizeof( VERTEX ) );

    // Restore state
    g_pd3dDevice->SetRenderState( D3DRS_ZFUNC, dwZFunc );
}


//--------------------------------------------------------------------------------------
// Name: LoadConstantTable()
// Desc: Creates a D3DX constant table object from the microcode constant table
//       contained within a microcode shader code buffer.
//       Returns NULL if no constant table is present in the shader.
//--------------------------------------------------------------------------------------
LPD3DXCONSTANTTABLE LoadConstantTable( VOID* pShaderCode )
{
    // Get the D3DXSHADER_CONSTANTTABLE structure. 
    const D3DXSHADER_CONSTANTTABLE* pDXShaderConstantTable;
    DWORD dwTableSize = 0;

    HRESULT hr = XGMicrocodeGetConstantTable( pShaderCode, &pDXShaderConstantTable, &dwTableSize );

    if( FAILED( hr ) )
    {
        return NULL;
    }

    if( dwTableSize > 0 )
    {
        // Get the LPD3DXCONSTANTTABLE object from the D3DXSHADER_CONSTANTTABLE structure. 
        D3DXSHADER_CONSTANTTABLE* pNativeConstantTable = ( D3DXSHADER_CONSTANTTABLE* )malloc( dwTableSize );
        assert( pNativeConstantTable != NULL );

        XGCopyUCodeToNativeConstantTable( pDXShaderConstantTable, pNativeConstantTable, dwTableSize );

        LPD3DXCONSTANTTABLE pConstantTableObject = NULL;

        hr = XGCreateConstantTable( pNativeConstantTable, dwTableSize, &pConstantTableObject );

        free( pNativeConstantTable );

        if( FAILED( hr ) )
        {
            return NULL;
        }

        return pConstantTableObject;
    }

    return NULL;
}


//--------------------------------------------------------------------------------------
// Name: LoadVertexShader()
// Desc: Loads pre-compiled vertex shader microcode from the specified file and
//       creates a vertex shader resource.
//--------------------------------------------------------------------------------------
HRESULT LoadVertexShader( const CHAR* strFileName, LPDIRECT3DVERTEXSHADER9* ppVS,
                          LPD3DXCONSTANTTABLE* ppConstantTable )
{
    HRESULT hr;
    VOID* pCode = NULL;
    ( *ppVS ) = NULL;

    if( FAILED( hr = LoadFile( strFileName, &pCode ) ) )
        return hr;
    if( FAILED( hr = g_pd3dDevice->CreateVertexShader( ( DWORD* )pCode, ppVS ) ) )
    {
        UnloadFile( pCode );
        return hr;
    }
    if( ppConstantTable != NULL )
    {
        *ppConstantTable = LoadConstantTable( pCode );
    }
    UnloadFile( pCode );

    return hr;
}


//--------------------------------------------------------------------------------------
// Name: LoadPixelShader()
// Desc: Loads pre-compiled pixel shader microcode from the specified file and
//       creates a pixel shader resource.
//--------------------------------------------------------------------------------------
HRESULT LoadPixelShader( const CHAR* strFileName, LPDIRECT3DPIXELSHADER9* ppPS, LPD3DXCONSTANTTABLE* ppConstantTable )
{
    HRESULT hr;
    VOID* pCode = NULL;
    ( *ppPS ) = NULL;

    if( FAILED( hr = LoadFile( strFileName, &pCode ) ) )
        return hr;
    if( FAILED( hr = g_pd3dDevice->CreatePixelShader( ( DWORD* )pCode, ppPS ) ) )
    {
        UnloadFile( pCode );
        return hr;
    }
    if( ppConstantTable != NULL )
    {
        *ppConstantTable = LoadConstantTable( pCode );
    }
    UnloadFile( pCode );

    return hr;
}


//--------------------------------------------------------------------------------------
// Name: AppendVertexElements()
// Desc: Helper function for building an array of vertex elements.
//--------------------------------------------------------------------------------------
VOID AppendVertexElements( D3DVERTEXELEMENT9* pDstElements, DWORD dwSrcStream,
                           D3DVERTEXELEMENT9* pSrcElements, DWORD dwSrcUsageIndex,
                           DWORD dwSrcOffset )
{
    // Find the end of the destination stream
    while( pDstElements->Stream != 0xff && pDstElements->Type != 0L )
        pDstElements++;

    // Add the source elements
    for(; ; )
    {
        pDstElements->Stream = ( WORD )dwSrcStream;
        pDstElements->Offset = pSrcElements->Offset + ( WORD )dwSrcOffset;
        pDstElements->Type = pSrcElements->Type;
        pDstElements->Method = pSrcElements->Method;
        pDstElements->Usage = pSrcElements->Usage;
        pDstElements->UsageIndex = ( BYTE )dwSrcUsageIndex;

        if( pSrcElements->Stream == 0xff )
        {
            pDstElements->Stream = 0xff;
            pDstElements->Offset = 0;
            pDstElements->Type = ( DWORD )D3DDECLTYPE_UNUSED;
            pDstElements->Method = 0;
            pDstElements->Usage = 0;
            pDstElements->UsageIndex = 0;
            break;
        }

        pSrcElements++;
        pDstElements++;
    }
}


//--------------------------------------------------------------------------------------
// Name: SetThreadName()
// Desc: Set the name of the given thread so that it will show up in the Threads Window
//       in Visual Studio and in PIX timing captures.
//--------------------------------------------------------------------------------------
VOID SetThreadName( DWORD dwThreadID, LPCSTR strThreadName )
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = strThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try
    {
        RaiseException( 0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info );
    }
    __except( GetExceptionCode()== 0x406D1388 ? 
                EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER ) 
    {
        __noop;
    }
}


} // namespace ATG
