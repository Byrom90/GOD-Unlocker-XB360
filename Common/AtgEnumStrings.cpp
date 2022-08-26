//-----------------------------------------------------------------------------
// enumstrings.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgEnumStrings.h"
#include "AtgLight.h"
#include "AtgCamera.h"
#include "AtgBound.h"

namespace ATG
{

//-----------------------------------------------------------------------------
EnumStringMap D3DXREGISTER_SET_StringMap[] =
{
    { D3DXRS_BOOL, L"BOOL" },
    { D3DXRS_INT4, L"INT4" },
    { D3DXRS_FLOAT4, L"FLOAT4" },
    { D3DXRS_SAMPLER, L"SAMPLER" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DXPARAMETER_CLASS_StringMap[] = 
{
    { D3DXPC_SCALAR, L"SCALAR" },
    { D3DXPC_VECTOR, L"VECTOR" },
    { D3DXPC_MATRIX_ROWS, L"MATRIX_ROWS" },
    { D3DXPC_MATRIX_COLUMNS, L"MATRIX_COLUMNS" },
    { D3DXPC_OBJECT, L"OBJECT" },
    { D3DXPC_STRUCT, L"STRUCT" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DXPARAMETER_TYPE_StringMap[] =
{
    { D3DXPT_VOID, L"VOID" },
    { D3DXPT_BOOL, L"BOOL" },
    { D3DXPT_INT, L"INT" },
    { D3DXPT_FLOAT, L"FLOAT" },
    { D3DXPT_STRING, L"STRING" },
    { D3DXPT_TEXTURE, L"TEXTURE" },
    { D3DXPT_TEXTURE1D, L"TEXTURE1D" },
    { D3DXPT_TEXTURE2D, L"TEXTURE2D" },
    { D3DXPT_TEXTURE3D, L"TEXTURE3D" },
    { D3DXPT_TEXTURECUBE, L"TEXTURECUBE" },
    { D3DXPT_SAMPLER, L"SAMPLER" },
    { D3DXPT_SAMPLER1D, L"SAMPLER1D" },
    { D3DXPT_SAMPLER2D, L"SAMPLER2D" },
    { D3DXPT_SAMPLER3D, L"SAMPLER3D" },
    { D3DXPT_SAMPLERCUBE, L"SAMPLERCUBE" },
    { D3DXPT_PIXELSHADER, L"PIXELSHADER" },
    { D3DXPT_VERTEXSHADER, L"VERTEXSHADER" },
    { D3DXPT_PIXELFRAGMENT, L"PIXELFRAGMENT" },
    { D3DXPT_VERTEXFRAGMENT, L"VERTEXFRAGMENT" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DFORMAT_StringMap[] = 
{
    { D3DFMT_DXT1, L"DXT1" },
    { D3DFMT_LIN_DXT1, L"LIN_DXT1" },
    { D3DFMT_DXT2, L"DXT2" },
    { D3DFMT_LIN_DXT2, L"LIN_DXT2" },
    { D3DFMT_DXT3, L"DXT3" },
    { D3DFMT_LIN_DXT3, L"LIN_DXT3" },
    { D3DFMT_DXT4, L"DXT4" },
    { D3DFMT_LIN_DXT4, L"LIN_DXT4" },
    { D3DFMT_DXT5, L"DXT5" },
    { D3DFMT_LIN_DXT5, L"LIN_DXT5" },
    { D3DFMT_DXN, L"DXN" },
    { D3DFMT_LIN_DXN, L"LIN_DXN" },
    { D3DFMT_A8, L"A8" },
    { D3DFMT_LIN_A8, L"LIN_A8" },
    { D3DFMT_L8, L"L8" },
    { D3DFMT_LIN_L8, L"LIN_L8" },
    { D3DFMT_R5G6B5, L"R5G6B5" },
    { D3DFMT_LIN_R5G6B5, L"LIN_R5G6B5" },
    { D3DFMT_R6G5B5, L"R6G5B5" },
    { D3DFMT_LIN_R6G5B5, L"LIN_R6G5B5" },
    { D3DFMT_L6V5U5, L"L6V5U5" },
    { D3DFMT_LIN_L6V5U5, L"LIN_L6V5U5" },
    { D3DFMT_X1R5G5B5, L"X1R5G5B5" },
    { D3DFMT_LIN_X1R5G5B5, L"LIN_X1R5G5B5" },
    { D3DFMT_A1R5G5B5, L"A1R5G5B5" },
    { D3DFMT_LIN_A1R5G5B5, L"LIN_A1R5G5B5" },
    { D3DFMT_A4R4G4B4, L"A4R4G4B4" },
    { D3DFMT_LIN_A4R4G4B4, L"LIN_A4R4G4B4" },
    { D3DFMT_X4R4G4B4, L"X4R4G4B4" },
    { D3DFMT_LIN_X4R4G4B4, L"LIN_X4R4G4B4" },
    { D3DFMT_Q4W4V4U4, L"Q4W4V4U4" },
    { D3DFMT_LIN_Q4W4V4U4, L"LIN_Q4W4V4U4" },
    { D3DFMT_A8L8, L"A8L8" },
    { D3DFMT_LIN_A8L8, L"LIN_A8L8" },
    { D3DFMT_G8R8, L"G8R8" },
    { D3DFMT_LIN_G8R8, L"LIN_G8R8" },
    { D3DFMT_V8U8, L"V8U8" },
    { D3DFMT_LIN_V8U8, L"LIN_V8U8" },
    { D3DFMT_D16, L"D16" },
    { D3DFMT_LIN_D16, L"LIN_D16" },
    { D3DFMT_L16, L"L16" },
    { D3DFMT_LIN_L16, L"LIN_L16" },
    { D3DFMT_R16F, L"R16F" },
    { D3DFMT_LIN_R16F, L"LIN_R16F" },
    { D3DFMT_R16F_EXPAND, L"R16F_EXPAND" },
    { D3DFMT_LIN_R16F_EXPAND, L"LIN_R16F_EXPAND" },
    { D3DFMT_UYVY, L"UYVY" },
    { D3DFMT_LIN_UYVY, L"LIN_UYVY" },
    { D3DFMT_G8R8_G8B8, L"G8R8_G8B8" },
    { D3DFMT_LIN_G8R8_G8B8, L"LIN_G8R8_G8B8" },
    { D3DFMT_R8G8_B8G8, L"R8G8_B8G8" },
    { D3DFMT_LIN_R8G8_B8G8, L"LIN_R8G8_B8G8" },
    { D3DFMT_YUY2, L"YUY2" },
    { D3DFMT_LIN_YUY2, L"LIN_YUY2" },
    { D3DFMT_A8R8G8B8, L"A8R8G8B8" },
    { D3DFMT_LIN_A8R8G8B8, L"LIN_A8R8G8B8" },
    { D3DFMT_X8R8G8B8, L"X8R8G8B8" },
    { D3DFMT_LIN_X8R8G8B8, L"LIN_X8R8G8B8" },
    { D3DFMT_A8B8G8R8, L"A8B8G8R8" },
    { D3DFMT_LIN_A8B8G8R8, L"LIN_A8B8G8R8" },
    { D3DFMT_X8B8G8R8, L"X8B8G8R8" },
    { D3DFMT_LIN_X8B8G8R8, L"LIN_X8B8G8R8" },
    { D3DFMT_X8L8V8U8, L"X8L8V8U8" },
    { D3DFMT_LIN_X8L8V8U8, L"LIN_X8L8V8U8" },
    { D3DFMT_Q8W8V8U8, L"Q8W8V8U8" },
    { D3DFMT_LIN_Q8W8V8U8, L"LIN_Q8W8V8U8" },
    { D3DFMT_A2R10G10B10, L"A2R10G10B10" },
    { D3DFMT_LIN_A2R10G10B10, L"LIN_A2R10G10B10" },
    { D3DFMT_X2R10G10B10, L"X2R10G10B10" },
    { D3DFMT_LIN_X2R10G10B10, L"LIN_X2R10G10B10" },
    { D3DFMT_A2B10G10R10, L"A2B10G10R10" },
    { D3DFMT_LIN_A2B10G10R10, L"LIN_A2B10G10R10" },
    { D3DFMT_A2W10V10U10, L"A2W10V10U10" },
    { D3DFMT_LIN_A2W10V10U10, L"LIN_A2W10V10U10" },
    { D3DFMT_A16L16, L"A16L16" },
    { D3DFMT_LIN_A16L16, L"LIN_A16L16" },
    { D3DFMT_G16R16, L"G16R16" },
    { D3DFMT_LIN_G16R16, L"LIN_G16R16" },
    { D3DFMT_V16U16, L"V16U16" },
    { D3DFMT_LIN_V16U16, L"LIN_V16U16" },
    { D3DFMT_R10G11B11, L"R10G11B11" },
    { D3DFMT_LIN_R10G11B11, L"LIN_R10G11B11" },
    { D3DFMT_R11G11B10, L"R11G11B10" },
    { D3DFMT_LIN_R11G11B10, L"LIN_R11G11B10" },
    { D3DFMT_W10V11U11, L"W10V11U11" },
    { D3DFMT_LIN_W10V11U11, L"LIN_W10V11U11" },
    { D3DFMT_W11V11U10, L"W11V11U10" },
    { D3DFMT_LIN_W11V11U10, L"LIN_W11V11U10" },
    { D3DFMT_G16R16F, L"G16R16F" },
    { D3DFMT_LIN_G16R16F, L"LIN_G16R16F" },
    { D3DFMT_G16R16F_EXPAND, L"G16R16F_EXPAND" },
    { D3DFMT_LIN_G16R16F_EXPAND, L"LIN_G16R16F_EXPAND" },
    { D3DFMT_L32, L"L32" },
    { D3DFMT_LIN_L32, L"LIN_L32" },
    { D3DFMT_R32F, L"R32F" },
    { D3DFMT_LIN_R32F, L"LIN_R32F" },
    { D3DFMT_A16B16G16R16, L"A16B16G16R16" },
    { D3DFMT_LIN_A16B16G16R16, L"LIN_A16B16G16R16" },
    { D3DFMT_Q16W16V16U16, L"Q16W16V16U16" },
    { D3DFMT_LIN_Q16W16V16U16, L"LIN_Q16W16V16U16" },
    { D3DFMT_A16B16G16R16F, L"A16B16G16R16F" },
    { D3DFMT_LIN_A16B16G16R16F, L"LIN_A16B16G16R16F" },
    { D3DFMT_A16B16G16R16F_EXPAND, L"A16B16G16R16F_EXPAND" },
    { D3DFMT_LIN_A16B16G16R16F_EXPAND, L"LIN_A16B16G16R16F_EXPAND" },
    { D3DFMT_A32L32, L"A32L32" },
    { D3DFMT_LIN_A32L32, L"LIN_A32L32" },
    { D3DFMT_G32R32, L"G32R32" },
    { D3DFMT_LIN_G32R32, L"LIN_G32R32" },
    { D3DFMT_V32U32, L"V32U32" },
    { D3DFMT_LIN_V32U32, L"LIN_V32U32" },
    { D3DFMT_G32R32F, L"G32R32F" },
    { D3DFMT_LIN_G32R32F, L"LIN_G32R32F" },
    { D3DFMT_A32B32G32R32, L"A32B32G32R32" },
    { D3DFMT_LIN_A32B32G32R32, L"LIN_A32B32G32R32" },
    { D3DFMT_Q32W32V32U32, L"Q32W32V32U32" },
    { D3DFMT_LIN_Q32W32V32U32, L"LIN_Q32W32V32U32" },
    { D3DFMT_A32B32G32R32F, L"A32B32G32R32F" },
    { D3DFMT_LIN_A32B32G32R32F, L"LIN_A32B32G32R32F" },
    { D3DFMT_A2B10G10R10F_EDRAM, L"A2B10G10R10F_EDRAM" },
    { D3DFMT_G16R16_EDRAM, L"G16R16_EDRAM" },
    { D3DFMT_A16B16G16R16_EDRAM, L"A16B16G16R16_EDRAM" },
    { D3DFMT_G16R16, L"G16R16" },
    { D3DFMT_A16B16G16R16, L"A16B16G16R16" },
    { D3DFMT_LE_X8R8G8B8, L"LE_X8R8G8B8" },
    { D3DFMT_LE_A8R8G8B8, L"LE_A8R8G8B8" },
    { D3DFMT_LE_X2R10G10B10, L"LE_X2R10G10B10" },
    { D3DFMT_LE_A2R10G10B10, L"LE_A2R10G10B10" },
    { D3DFMT_INDEX16, L"INDEX16" },
    { D3DFMT_INDEX32, L"INDEX32" },
    { D3DFMT_VERTEXDATA, L"VERTEXDATA" },
    { DWORD(D3DFMT_UNKNOWN), L"UNKNOWN" },
    { D3DFMT_DXT3A, L"DXT3A" },
    { D3DFMT_LIN_DXT3A, L"LIN_DXT3A" },
    { D3DFMT_DXT3A_1111, L"DXT3A_1111" },
    { D3DFMT_LIN_DXT3A_1111, L"LIN_DXT3A_1111" },
    { D3DFMT_DXT5A, L"DXT5A" },
    { D3DFMT_LIN_DXT5A, L"LIN_DXT5A" },
    { D3DFMT_CTX1, L"CTX1" },
    { D3DFMT_LIN_CTX1, L"LIN_CTX1" },
    { D3DFMT_D24S8, L"D24S8" },
    { D3DFMT_LIN_D24S8, L"LIN_D24S8" },
    { D3DFMT_D24X8, L"D24X8" },
    { D3DFMT_LIN_D24X8, L"LIN_D24X8" },
    { D3DFMT_D24FS8, L"D24FS8" },
    { D3DFMT_LIN_D24FS8, L"LIN_D24FS8" },
    { D3DFMT_D32, L"D32" },
    { D3DFMT_LIN_D32, L"LIN_D32" },
    { 0, NULL }
};


//-----------------------------------------------------------------------------
EnumStringMap D3DX_FILTER_StringMap[] = 
{
    { D3DX_DEFAULT, L"DEFAULT" },
    { D3DX_FILTER_NONE, L"FILTER_NONE" },
    { D3DX_FILTER_POINT, L"FILTER_POINT" },
    { D3DX_FILTER_LINEAR, L"FILTER_LINEAR" },
    { D3DX_FILTER_TRIANGLE, L"FILTER_TRIANGLE" },
    { D3DX_FILTER_BOX, L"FILTER_BOX" },
    { D3DX_FILTER_MIRROR_U, L"FILTER_MIRROR_U" },
    { D3DX_FILTER_MIRROR_V, L"FILTER_MIRROR_V" },
    { D3DX_FILTER_MIRROR_W, L"FILTER_MIRROR_W" },
    { D3DX_FILTER_MIRROR, L"FILTER_MIRROR" },
    { D3DX_FILTER_DITHER, L"FILTER_DITHER" },
    { D3DX_FILTER_SRGB_IN, L"FILTER_SRGB_IN" },
    { D3DX_FILTER_SRGB_OUT, L"FILTER_SRGB_OUT" },
    { D3DX_FILTER_SRGB, L"FILTER_SRGB" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DDECLTYPE_StringMap[] = 
{
    { D3DDECLTYPE_FLOAT1, L"FLOAT1" },
    { D3DDECLTYPE_FLOAT2, L"FLOAT2" },
    { D3DDECLTYPE_FLOAT3, L"FLOAT3" },
    { D3DDECLTYPE_FLOAT4, L"FLOAT4" },
    { D3DDECLTYPE_INT1, L"INT1" },
    { D3DDECLTYPE_INT2, L"INT2" },
    { D3DDECLTYPE_INT4, L"INT4" },
    { D3DDECLTYPE_UINT1, L"UINT1" },
    { D3DDECLTYPE_UINT2, L"UINT2" },
    { D3DDECLTYPE_UINT4, L"UINT4" },
    { D3DDECLTYPE_INT1N, L"INT1N" },
    { D3DDECLTYPE_INT2N, L"INT2N" },
    { D3DDECLTYPE_INT4N, L"INT4N" },
    { D3DDECLTYPE_UINT1N, L"UINT1N" },
    { D3DDECLTYPE_UINT2N, L"UINT2N" },
    { D3DDECLTYPE_UINT4N, L"UINT4N" },
    { D3DDECLTYPE_D3DCOLOR, L"D3DCOLOR" },
    { D3DDECLTYPE_UBYTE4, L"UBYTE4" },
    { D3DDECLTYPE_BYTE4, L"BYTE4" },
    { D3DDECLTYPE_UBYTE4N, L"UBYTE4N" },
    { D3DDECLTYPE_BYTE4N, L"BYTE4N" },
    { D3DDECLTYPE_SHORT2, L"SHORT2" },
    { D3DDECLTYPE_SHORT4, L"SHORT4" },
    { D3DDECLTYPE_USHORT2, L"USHORT2" },
    { D3DDECLTYPE_USHORT4, L"USHORT4" },
    { D3DDECLTYPE_SHORT2N, L"SHORT2N" },
    { D3DDECLTYPE_SHORT4N, L"SHORT4N" },
    { D3DDECLTYPE_USHORT2N, L"USHORT2N" },
    { D3DDECLTYPE_USHORT4N, L"USHORT4N" },
    { D3DDECLTYPE_UDEC3, L"UDEC3" },
    { D3DDECLTYPE_DEC3, L"DEC3" },
    { D3DDECLTYPE_UDEC3N, L"UDEC3N" },
    { D3DDECLTYPE_DEC3N, L"DEC3N" },
    { D3DDECLTYPE_UDHEN3, L"UDHEN3" },
    { D3DDECLTYPE_DHEN3, L"DHEN3" },
    { D3DDECLTYPE_UDHEN3N, L"UDHEN3N" },
    { D3DDECLTYPE_DHEN3N, L"DHEN3N" },
    { D3DDECLTYPE_UHEND3, L"UHEND3" },
    { D3DDECLTYPE_HEND3, L"HEND3" },
    { D3DDECLTYPE_UHEND3N, L"UHEND3N" },
    { D3DDECLTYPE_HEND3N, L"HEND3N" },
    { D3DDECLTYPE_FLOAT16_2, L"FLOAT16_2" },
    { D3DDECLTYPE_FLOAT16_4, L"FLOAT16_4" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DDECLMETHOD_StringMap[] = 
{
    { D3DDECLMETHOD_DEFAULT, L"DEFAULT" },
    { D3DDECLMETHOD_PARTIALU, L"PARTIALU" },
    { D3DDECLMETHOD_PARTIALV, L"PARTIALV" },
    { D3DDECLMETHOD_CROSSUV, L"CROSSUV" },
    { D3DDECLMETHOD_UV, L"UV" },
    { D3DDECLMETHOD_LOOKUP, L"LOOKUP" },
    { D3DDECLMETHOD_LOOKUPPRESAMPLED, L"LOOKUPPRESAMPLED" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DDECLUSAGE_StringMap[] = 
{
    { D3DDECLUSAGE_POSITION, L"POSITION" },
    { D3DDECLUSAGE_BLENDWEIGHT, L"BLENDWEIGHT" },
    { D3DDECLUSAGE_BLENDINDICES, L"BLENDINDICES" },
    { D3DDECLUSAGE_NORMAL, L"NORMAL" },
    { D3DDECLUSAGE_PSIZE, L"PSIZE" },
    { D3DDECLUSAGE_TEXCOORD, L"TEXCOORD" },
    { D3DDECLUSAGE_TANGENT, L"TANGENT" },
    { D3DDECLUSAGE_BINORMAL, L"BINORMAL" },
    { D3DDECLUSAGE_TESSFACTOR, L"TESSFACTOR" },
    { D3DDECLUSAGE_COLOR, L"COLOR" },
    { D3DDECLUSAGE_FOG, L"FOG" },
    { D3DDECLUSAGE_DEPTH, L"DEPTH" },
    { D3DDECLUSAGE_SAMPLE, L"SAMPLE" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DFILLMODE_StringMap[] =
{
    { D3DFILL_POINT, L"POINT" },
    { D3DFILL_WIREFRAME, L"WIREFRAME" },
    { D3DFILL_SOLID, L"SOLID" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DBLEND_StringMap[] =
{
    { D3DBLEND_ZERO, L"ZERO" },
    { D3DBLEND_ONE, L"ONE" },
    { D3DBLEND_SRCCOLOR, L"SRCCOLOR" },
    { D3DBLEND_INVSRCCOLOR, L"INVSRCCOLOR" },
    { D3DBLEND_SRCALPHA, L"SRCALPHA" },
    { D3DBLEND_INVSRCALPHA, L"INVSRCALPHA" },
    { D3DBLEND_DESTCOLOR, L"DESTCOLOR" },
    { D3DBLEND_INVDESTCOLOR, L"INVDESTCOLOR" },
    { D3DBLEND_DESTALPHA, L"DESTALPHA" },
    { D3DBLEND_INVDESTALPHA, L"INVDESTALPHA" },
    { D3DBLEND_BLENDFACTOR, L"BLENDFACTOR" },
    { D3DBLEND_INVBLENDFACTOR, L"INVBLENDFACTOR" },
    { D3DBLEND_CONSTANTALPHA, L"CONSTANTALPHA" },
    { D3DBLEND_INVCONSTANTALPHA, L"INVCONSTANTALPHA" },
    { D3DBLEND_SRCALPHASAT, L"SRCALPHASAT" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DBLENDOP_StringMap[] =
{
    { D3DBLENDOP_ADD, L"ADD" },
    { D3DBLENDOP_SUBTRACT, L"SUBTRACT" },
    { D3DBLENDOP_MIN, L"MIN" },
    { D3DBLENDOP_MAX, L"MAX" },
    { D3DBLENDOP_REVSUBTRACT, L"REVSUBTRACT" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTEXTUREADDRESS_StringMap[] =
{
    { D3DTADDRESS_WRAP, L"WRAP" },
    { D3DTADDRESS_MIRROR, L"MIRROR" },
    { D3DTADDRESS_CLAMP, L"CLAMP" },
    { D3DTADDRESS_BORDER, L"BORDER" },
    { D3DTADDRESS_MIRRORONCE, L"MIRRORONCE" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DCULL_StringMap[] =
{
    { D3DCULL_NONE, L"NONE" },
    { D3DCULL_CW, L"CW" },
    { D3DCULL_CCW, L"CCW" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DCMPFUNC_StringMap[] =
{
    { D3DCMP_NEVER, L"NEVER" },
    { D3DCMP_LESS, L"LESS" },
    { D3DCMP_EQUAL, L"EQUAL" },
    { D3DCMP_LESSEQUAL, L"LESSEQUAL" },
    { D3DCMP_GREATER, L"GREATER" },
    { D3DCMP_NOTEQUAL, L"NOTEQUAL" },
    { D3DCMP_GREATEREQUAL, L"GREATEREQUAL" },
    { D3DCMP_ALWAYS, L"ALWAYS" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DSTENCILOP_StringMap[] =
{
    { D3DSTENCILOP_KEEP, L"KEEP" },
    { D3DSTENCILOP_ZERO, L"ZERO" },
    { D3DSTENCILOP_REPLACE, L"REPLACE" },
    { D3DSTENCILOP_INCRSAT, L"INCRSAT" },
    { D3DSTENCILOP_DECRSAT, L"DECRSAT" },
    { D3DSTENCILOP_INVERT, L"INVERT" },
    { D3DSTENCILOP_INCR, L"INCR" },
    { D3DSTENCILOP_DECR, L"DECR" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DFOGMODE_StringMap[] =
{
    { D3DFOG_NONE, L"NONE" },
    { D3DFOG_EXP, L"EXP" },
    { D3DFOG_EXP2, L"EXP2" },
    { D3DFOG_LINEAR, L"LINEAR" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DZBUFFERTYPE_StringMap[] =
{
    { D3DZB_FALSE, L"FALSE" },
    { D3DZB_TRUE, L"TRUE" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DPRIMITIVETYPE_StringMap[] =
{
    { D3DPT_POINTLIST , L"POINTLIST " },
    { D3DPT_LINELIST, L"LINELIST" },
    { D3DPT_LINESTRIP, L"LINESTRIP" },
    { D3DPT_TRIANGLELIST, L"TRIANGLELIST" },
    { D3DPT_TRIANGLESTRIP, L"TRIANGLESTRIP" },
    { D3DPT_TRIANGLEFAN, L"TRIANGLEFAN" },
    { 0, NULL },
}; 

//-----------------------------------------------------------------------------
EnumStringMap D3DTRANSFORMSTATETYPE_StringMap[] =
{
    { D3DTS_VIEW, L"VIEW" },
    { D3DTS_PROJECTION, L"PROJECTION" },
    { D3DTS_TEXTURE0, L"TEXTURE0" },
    { D3DTS_TEXTURE1, L"TEXTURE1" },
    { D3DTS_TEXTURE2, L"TEXTURE2" },
    { D3DTS_TEXTURE3, L"TEXTURE3" },
    { D3DTS_TEXTURE4, L"TEXTURE4" },
    { D3DTS_TEXTURE5, L"TEXTURE5" },
    { D3DTS_TEXTURE6, L"TEXTURE6" },
    { D3DTS_TEXTURE7, L"TEXTURE7" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DWRAP_StringMap[] =
{
    { D3DWRAP_U, L"U" },
    { D3DWRAP_V, L"V" },
    { D3DWRAP_W, L"W" },
    { D3DWRAPCOORD_0, L"0" },
    { D3DWRAPCOORD_1, L"1" },
    { D3DWRAPCOORD_2, L"2" },
    { D3DWRAPCOORD_3, L"3" },
     { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DMATERIALCOLORSOURCE_StringMap[] =
{
    { D3DMCS_MATERIAL, L"MATERIAL" },
    { D3DMCS_COLOR1, L"COLOR1" },
    { D3DMCS_COLOR2, L"COLOR2" },
     { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DDEBUGMONITORTOKENS_StringMap[] =
{
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DVERTEXBLENDFLAGS_StringMap[] =
{
    { D3DVBF_DISABLE, L"DISABLE" },
    { D3DVBF_1WEIGHTS, L"1WEIGHTS" },
    { D3DVBF_2WEIGHTS, L"2WEIGHTS" },
    { D3DVBF_3WEIGHTS , L"3WEIGHTS " },
    { D3DVBF_TWEENING, L"TWEENING" },
    { D3DVBF_0WEIGHTS, L"0WEIGHTS" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DPATCHEDGESTYLE_StringMap[] =
{
    { D3DPATCHEDGE_DISCRETE, L"DISCRETE" },
    { D3DPATCHEDGE_CONTINUOUS, L"CONTINUOUS" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DDEGREETYPE_StringMap[] =
{
    { D3DDEGREE_LINEAR, L"LINEAR" },
    { D3DDEGREE_QUADRATIC, L"QUADRATIC" },
    { D3DDEGREE_CUBIC, L"CUBIC" },
    { D3DDEGREE_QUINTIC, L"QUINTIC" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTEXTUREFILTERTYPE_StringMap[] =
{
    { D3DTEXF_NONE, L"NONE" },
    { D3DTEXF_POINT, L"POINT" },
    { D3DTEXF_LINEAR, L"LINEAR" },
    { D3DTEXF_ANISOTROPIC, L"ANISOTROPIC" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTRILINEARTHRESHOLD_StringMap[] =
{
    { D3DTRILINEAR_IMMEDIATE, L"IMMEDIATE" },
    { D3DTRILINEAR_ONESIXTH, L"ONESIXTH" },
    { D3DTRILINEAR_ONEFOURTH, L"ONEFOURTH" },
    { D3DTRILINEAR_THREEEIGHTHS, L"THREEEIGHTHS" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTEXTUREOP_StringMap[] =
{
    { D3DTOP_DISABLE, L"DISABLE" },
    { D3DTOP_SELECTARG1, L"SELECTARG1" },
    { D3DTOP_SELECTARG2, L"SELECTARG2" },
    { D3DTOP_MODULATE, L"MODULATE" },
    { D3DTOP_MODULATE2X, L"MODULATE2X" },
    { D3DTOP_MODULATE4X, L"MODULATE4X" },
    { D3DTOP_ADD, L"ADD" },
    { D3DTOP_ADDSIGNED, L"ADDSIGNED" },
    { D3DTOP_ADDSIGNED2X, L"ADDSIGNED2X" },
    { D3DTOP_SUBTRACT, L"SUBTRACT" },
    { D3DTOP_ADDSMOOTH, L"ADDSMOOTH" },
    { D3DTOP_BLENDDIFFUSEALPHA, L"BLENDDIFFUSEALPHA" },
    { D3DTOP_BLENDTEXTUREALPHA, L"BLENDTEXTUREALPHA" },
    { D3DTOP_BLENDFACTORALPHA, L"BLENDFACTORALPHA" },
    { D3DTOP_BLENDTEXTUREALPHAPM, L"BLENDTEXTUREALPHAPM" },
    { D3DTOP_BLENDCURRENTALPHA, L"BLENDCURRENTALPHA" },
    { D3DTOP_PREMODULATE, L"PREMODULATE" },
    { D3DTOP_MODULATEALPHA_ADDCOLOR, L"ADDCOLOR" },
    { D3DTOP_MODULATECOLOR_ADDALPHA, L"ADDALPHA" },
    { D3DTOP_MODULATEINVALPHA_ADDCOLOR, L"ADDCOLOR" },
    { D3DTOP_MODULATEINVCOLOR_ADDALPHA, L"ADDALPHA" },
    { D3DTOP_BUMPENVMAP, L"BUMPENVMAP" },
    { D3DTOP_BUMPENVMAPLUMINANCE, L"BUMPENVMAPLUMINANCE" },
    { D3DTOP_DOTPRODUCT3, L"DOTPRODUCT3" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTA_StringMap[] =
{
    { D3DTA_SELECTMASK, L"SELECTMASK" },
    { D3DTA_DIFFUSE, L"DIFFUSE" },
    { D3DTA_CURRENT, L"CURRENT" },
    { D3DTA_TEXTURE, L"TEXTURE" },
    { D3DTA_TFACTOR, L"TFACTOR" },
    { D3DTA_SPECULAR, L"SPECULAR" },
    { D3DTA_TEMP, L"TEMP" },
    { D3DTA_CONSTANT, L"CONSTANT" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTA_FLAGS_StringMap[] =
{
    { D3DTA_COMPLEMENT, L"COMPLEMENT" },
    { D3DTA_ALPHAREPLICATE, L"ALPHAREPLICATE" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTEXTURETRANSFORMFLAGS_StringMap[] =
{
    { D3DTTFF_DISABLE, L"DISABLE" },
    { D3DTTFF_COUNT1, L"COUNT1" },
    { D3DTTFF_COUNT2, L"COUNT2" },
    { D3DTTFF_COUNT3, L"COUNT3" },
    { D3DTTFF_COUNT4, L"COUNT4" },
    { D3DTTFF_PROJECTED, L"PROJECTED" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DTSS_TCI_FLAGS_StringMap[] =
{
    { D3DTSS_TCI_CAMERASPACENORMAL, L"CAMERASPACENORMAL" },
    { D3DTSS_TCI_CAMERASPACEPOSITION, L"CAMERASPACEPOSITION" },
    { D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR, L"CAMERASPACEREFLECTIONVECTOR" },
    { D3DTSS_TCI_SPHEREMAP, L"SPHEREMAP" },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
EnumStringMap D3DCLEAR_StringMap[] =
{
    { D3DCLEAR_STENCIL, L"STENCIL" },
    { D3DCLEAR_TARGET, L"TARGET" },
    { D3DCLEAR_ZBUFFER, L"ZBUFFER" },
    { 0, NULL },
};

EnumStringMap D3DMULTISAMPLE_TYPE_StringMap[] = 
{
    { D3DMULTISAMPLE_NONE, L"NONE" },
    { D3DMULTISAMPLE_2_SAMPLES, L"2_SAMPLES" },
    { D3DMULTISAMPLE_4_SAMPLES, L"4_SAMPLES" },
    { 0, NULL },
};



//-----------------------------------------------------------------------------
StateStringMap D3DRENDERSTATETYPE_StateStringMap[] = 
{
    { D3DRS_VIEWPORTENABLE, L"VIEWPORTENABLE", STATE_TYPE_BOOL }, 
    { D3DRS_ZENABLE, L"ZENABLE", STATE_TYPE_ENUM, D3DZBUFFERTYPE_StringMap }, 
    { D3DRS_FILLMODE, L"FILLMODE", STATE_TYPE_ENUM, D3DFILLMODE_StringMap },
    { D3DRS_ZWRITEENABLE, L"ZWRITEENABLE", STATE_TYPE_BOOL },
    { D3DRS_ALPHATESTENABLE, L"ALPHATESTENABLE", STATE_TYPE_BOOL },
    { D3DRS_SRCBLEND, L"SRCBLEND", STATE_TYPE_ENUM, D3DBLEND_StringMap },
    { D3DRS_DESTBLEND, L"DESTBLEND", STATE_TYPE_ENUM, D3DBLEND_StringMap },
    { D3DRS_CULLMODE, L"CULLMODE", STATE_TYPE_ENUM, D3DCULL_StringMap },
    { D3DRS_ZFUNC, L"ZFUNC" , STATE_TYPE_ENUM, D3DCMPFUNC_StringMap },
    { D3DRS_ALPHAREF, L"ALPHAREF", STATE_TYPE_COLOR },
    { D3DRS_ALPHAFUNC, L"ALPHAFUNC", STATE_TYPE_ENUM, D3DCMPFUNC_StringMap },
//    { D3DRS_DITHERENABLE, L"DITHERENABLE", STATE_TYPE_BOOL },
    { D3DRS_ALPHABLENDENABLE, L"ALPHABLENDENABLE", STATE_TYPE_BOOL },
    { D3DRS_STENCILENABLE , L"STENCILENABLE", STATE_TYPE_BOOL },
    { D3DRS_STENCILFAIL , L"STENCILFAIL", STATE_TYPE_ENUM, D3DSTENCILOP_StringMap },
    { D3DRS_STENCILZFAIL , L"STENCILZFAIL", STATE_TYPE_ENUM, D3DSTENCILOP_StringMap },
    { D3DRS_STENCILPASS , L"STENCILPASS", STATE_TYPE_ENUM, D3DSTENCILOP_StringMap },
    { D3DRS_STENCILFUNC , L"STENCILFUNC", STATE_TYPE_ENUM, D3DCMPFUNC_StringMap },
    { D3DRS_STENCILREF , L"STENCILREF", STATE_TYPE_MASK },
    { D3DRS_STENCILMASK , L"STENCILMASK", STATE_TYPE_MASK },
    { D3DRS_STENCILWRITEMASK , L"STENCILWRITEMASK", STATE_TYPE_MASK },
    { D3DRS_WRAP0 , L"WRAP0", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP1 , L"WRAP1", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP2 , L"WRAP2", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP3 , L"WRAP3", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP4 , L"WRAP4", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP5 , L"WRAP5", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP6 , L"WRAP6", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP7 , L"WRAP7", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_CLIPPLANEENABLE , L"CLIPPLANEENABLE", STATE_TYPE_BOOL },
    { D3DRS_MULTISAMPLEANTIALIAS , L"MULTISAMPLEANTIALIAS", STATE_TYPE_BOOL },
    { D3DRS_MULTISAMPLEMASK , L"MULTISAMPLEMASK", STATE_TYPE_MASK },
//    { D3DRS_PATCHEDGESTYLE , L"PATCHEDGESTYLE", STATE_TYPE_ENUM,  D3DPATCHEDGESTYLE_StringMap },
    { D3DRS_COLORWRITEENABLE , L"COLORWRITEENABLE", STATE_TYPE_COLOR },
    { D3DRS_BLENDOP , L"BLENDOP", STATE_TYPE_ENUM, D3DBLENDOP_StringMap },
//    { D3DRS_POSITIONDEGREE , L"POSITIONDEGREE", STATE_TYPE_ENUM, D3DDEGREETYPE_StringMap },
//    { D3DRS_NORMALDEGREE , L"NORMALDEGREE", STATE_TYPE_ENUM, D3DDEGREETYPE_StringMap },
    { D3DRS_SCISSORTESTENABLE , L"SCISSORTESTENABLE", STATE_TYPE_BOOL },
    { D3DRS_SLOPESCALEDEPTHBIAS , L"SLOPESCALEDEPTHBIAS", STATE_TYPE_FLOAT },
//    { D3DRS_ANTIALIASEDLINEENABLE , L"ANTIALIASEDLINEENABLE", STATE_TYPE_BOOL },
    { D3DRS_MINTESSELLATIONLEVEL , L"MINTESSELLATIONLEVEL" , STATE_TYPE_FLOAT },
    { D3DRS_MAXTESSELLATIONLEVEL , L"MAXTESSELLATIONLEVEL", STATE_TYPE_FLOAT },
//    { D3DRS_ADAPTIVETESS_X , L"ADAPTIVETESS_X", STATE_TYPE_FLOAT },
//    { D3DRS_ADAPTIVETESS_Y , L"ADAPTIVETESS_Y", STATE_TYPE_FLOAT },
//    { D3DRS_ADAPTIVETESS_Z , L"ADAPTIVETESS_Z", STATE_TYPE_FLOAT },
//    { D3DRS_ADAPTIVETESS_W , L"ADAPTIVETESS_W", STATE_TYPE_FLOAT },
//    { D3DRS_ENABLEADAPTIVETESSELLATION , L"ENABLEADAPTIVETESSELLATION" , STATE_TYPE_BOOL },
    { D3DRS_TWOSIDEDSTENCILMODE , L"TWOSIDEDSTENCILMODE", STATE_TYPE_BOOL },
    { D3DRS_CCW_STENCILFAIL , L"CCW_STENCILFAIL", STATE_TYPE_ENUM, D3DSTENCILOP_StringMap },
    { D3DRS_CCW_STENCILZFAIL , L"CCW_STENCILZFAIL", STATE_TYPE_ENUM, D3DSTENCILOP_StringMap },
    { D3DRS_CCW_STENCILPASS , L"CCW_STENCILPASS", STATE_TYPE_ENUM, D3DSTENCILOP_StringMap },
    { D3DRS_CCW_STENCILFUNC , L"CCW_STENCILFUNC", STATE_TYPE_ENUM, D3DCMPFUNC_StringMap },
    { D3DRS_COLORWRITEENABLE1 , L"COLORWRITEENABLE1", STATE_TYPE_COLOR },
    { D3DRS_COLORWRITEENABLE2 , L"COLORWRITEENABLE2", STATE_TYPE_COLOR },
    { D3DRS_COLORWRITEENABLE3 , L"COLORWRITEENABLE3", STATE_TYPE_COLOR },
    { D3DRS_BLENDFACTOR , L"BLENDFACTOR", STATE_TYPE_COLOR },
//    { D3DRS_SRGBWRITEENABLE , L"SRGBWRITEENABLE", STATE_TYPE_BOOL },
    { D3DRS_DEPTHBIAS , L"DEPTHBIAS", STATE_TYPE_FLOAT },
    { D3DRS_WRAP8 , L"WRAP8", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP9 , L"WRAP9", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP10 , L"WRAP10", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP11 , L"WRAP11", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP12 , L"WRAP12", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP13 , L"WRAP13", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP14 , L"WRAP14", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_WRAP15 , L"WRAP15", STATE_TYPE_ENUM, D3DWRAP_StringMap },
    { D3DRS_SEPARATEALPHABLENDENABLE , L"SEPARATEALPHABLENDENABLE", STATE_TYPE_BOOL },
    { D3DRS_SRCBLENDALPHA , L"SRCBLENDALPHA", STATE_TYPE_ENUM, D3DBLEND_StringMap },
    { D3DRS_DESTBLENDALPHA , L"DESTBLENDALPHA", STATE_TYPE_ENUM, D3DBLEND_StringMap },
    { D3DRS_BLENDOPALPHA , L"BLENDOPALPHA", STATE_TYPE_ENUM, D3DBLEND_StringMap },
    { 0, NULL },
};


//-----------------------------------------------------------------------------
StateStringMap D3DSAMPLERSTATETYPE_StateStringMap[] = 
{
    { D3DSAMP_ADDRESSU, L"ADDRESSU", STATE_TYPE_ENUM, D3DTEXTUREADDRESS_StringMap },
    { D3DSAMP_ADDRESSV, L"ADDRESSV", STATE_TYPE_ENUM, D3DTEXTUREADDRESS_StringMap },
    { D3DSAMP_ADDRESSW, L"ADDRESSW", STATE_TYPE_ENUM, D3DTEXTUREADDRESS_StringMap },
    { D3DSAMP_BORDERCOLOR, L"BORDERCOLOR", STATE_TYPE_COLOR },
    { D3DSAMP_MAGFILTER, L"MAGFILTER", STATE_TYPE_ENUM, D3DTEXTUREFILTERTYPE_StringMap },
    { D3DSAMP_MINFILTER, L"MINFILTER", STATE_TYPE_ENUM, D3DTEXTUREFILTERTYPE_StringMap },
    { D3DSAMP_MIPFILTER, L"MIPFILTER", STATE_TYPE_ENUM, D3DTEXTUREFILTERTYPE_StringMap },
    { D3DSAMP_MIPMAPLODBIAS, L"MIPMAPLODBIAS", STATE_TYPE_FLOAT },
    { D3DSAMP_MAXMIPLEVEL, L"MAXMIPLEVEL", STATE_TYPE_UINT },
    { D3DSAMP_MAXANISOTROPY, L"MAXANISOTROPY", STATE_TYPE_UINT },
    { D3DSAMP_MAGFILTERZ, L"MAGFILTERZ", STATE_TYPE_ENUM, D3DTEXTUREFILTERTYPE_StringMap },
    { D3DSAMP_MINFILTERZ, L"MINFILTERZ", STATE_TYPE_ENUM, D3DTEXTUREFILTERTYPE_StringMap },
    { D3DSAMP_SEPARATEZFILTERENABLE, L"SEPARATEZFILTERENABLE", STATE_TYPE_BOOL },
    { D3DSAMP_MINMIPLEVEL, L"MINMIPLEVEL", STATE_TYPE_UINT },
    { D3DSAMP_TRILINEARTHRESHOLD, L"TRILINEARTHRESHOLD", STATE_TYPE_ENUM, D3DTRILINEARTHRESHOLD_StringMap },
    { D3DSAMP_ANISOTROPYBIAS, L"D3DSAMP_ANISOTROPYBIAS", STATE_TYPE_FLOAT },
    { D3DSAMP_HGRADIENTEXPBIAS, L"HGRADIENTEXPBIAS", STATE_TYPE_INT },
    { D3DSAMP_VGRADIENTEXPBIAS, L"VGRADIENTEXPBIAS", STATE_TYPE_INT },
    { D3DSAMP_WHITEBORDERCOLORW, L"WHITEBORDERCOLORW", STATE_TYPE_BOOL },
    { D3DSAMP_POINTBORDERENABLE, L"POINTBORDERENABLE", STATE_TYPE_BOOL },
    { 0, NULL },
};

//-----------------------------------------------------------------------------
StateStringMap D3DTEXTURESTAGESTATETYPE_StateStringMap[] = 
{
    { D3DTSS_COLOROP, L"COLOROP", STATE_TYPE_ENUM, D3DTEXTUREOP_StringMap },
    { D3DTSS_COLORARG1, L"COLORARG1", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap, D3DTA_FLAGS_StringMap },
    { D3DTSS_COLORARG2, L"COLORARG2", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap , D3DTA_FLAGS_StringMap},
    { D3DTSS_ALPHAOP, L"ALPHAOP", STATE_TYPE_ENUM, D3DTEXTUREOP_StringMap },
    { D3DTSS_ALPHAARG1, L"ALPHAARG1", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap, D3DTA_FLAGS_StringMap },
    { D3DTSS_ALPHAARG2, L"ALPHAARG2", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap, D3DTA_FLAGS_StringMap },
    { D3DTSS_BUMPENVMAT00, L"BUMPENVMAT00", STATE_TYPE_FLOAT },
    { D3DTSS_BUMPENVMAT01, L"BUMPENVMAT01", STATE_TYPE_FLOAT },
    { D3DTSS_BUMPENVMAT10, L"BUMPENVMAT10", STATE_TYPE_FLOAT },
    { D3DTSS_BUMPENVMAT11, L"BUMPENVMAT11", STATE_TYPE_FLOAT },
    { D3DTSS_TEXCOORDINDEX, L"TEXCOORDINDEX", STATE_TYPE_UINT_WITH_FLAGS, NULL, D3DTSS_TCI_FLAGS_StringMap },
    { D3DTSS_BUMPENVLSCALE, L"BUMPENVLSCALE", STATE_TYPE_FLOAT },
    { D3DTSS_BUMPENVLOFFSET, L"BUMPENVLOFFSET", STATE_TYPE_FLOAT },
    { D3DTSS_TEXTURETRANSFORMFLAGS, L"TEXTURETRANSFORMFLAGS", STATE_TYPE_ENUM, D3DTEXTURETRANSFORMFLAGS_StringMap },
    { D3DTSS_COLORARG0, L"COLORARG0", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap, D3DTA_FLAGS_StringMap },
    { D3DTSS_ALPHAARG0, L"ALPHAARG0", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap, D3DTA_FLAGS_StringMap },
    { D3DTSS_RESULTARG, L"RESULTARG", STATE_TYPE_ENUM_WITH_FLAGS, D3DTA_StringMap, D3DTA_FLAGS_StringMap },
    { D3DTSS_CONSTANT, L"CONSTANT", STATE_TYPE_COLOR },
    { 0, NULL },
};


//-----------------------------------------------------------------------------
BOOL GetValueFromString( CONST WCHAR* szValue,
                         CONST EnumStringMap* pEnumStringMap,
                         DWORD& Value )
{
    while( pEnumStringMap->szName != NULL )
    {
        if( _wcsicmp( pEnumStringMap->szName, szValue ) == 0 )
        {
            Value = pEnumStringMap->Value;
            return TRUE;
        }
        pEnumStringMap++;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL GetStringFromValue( CONST DWORD Value,
                         CONST EnumStringMap* pEnumStringMap,
                         CONST WCHAR*& strString )
{
    while( pEnumStringMap->szName != NULL )
    {
        if( pEnumStringMap->Value == Value )
        {
            strString = pEnumStringMap->szName;
            return TRUE;
        }
        pEnumStringMap++;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
BOOL GetValueFromStringPartial( CONST WCHAR* szValue,
                                CONST EnumStringMap* pEnumStringMap,
                                DWORD& Value )
{
    while( pEnumStringMap->szName != NULL )
    {
        if( wcsstr( szValue, pEnumStringMap->szName ) != NULL )
        {
            Value = pEnumStringMap->Value;
            return TRUE;
        }
        pEnumStringMap++;
    }
    return FALSE;
}

//-----------------------------------------------------------
// Utility function

CONST WCHAR* SkipPast( CONST WCHAR* szString, CONST WCHAR* szSkipChars )
{
    if( szString == NULL )
        return NULL;

    for( ;; )
    {
        if( szString[0] == NULL )
            break;

        CONST WCHAR* ptr;
        for( ptr = szSkipChars; ptr[0] != NULL; ptr++ )
        {
            if( szString[0] == ptr[0] )
            {
                szString++;
                break;
            }
        }
        if( ptr[0] == NULL )
            break;
    }
    return szString;
}

//-----------------------------------------------------------------------------
BOOL GetStateAndValueFromStrings( CONST WCHAR* szState, CONST WCHAR* szValue, 
                                  StateStringMap* pStateStringMap,
                                  DWORD &State, DWORD &Value )
{
    
    while( pStateStringMap->szName != NULL )
    {        
        if( _wcsnicmp( pStateStringMap->szName, szState, wcslen(pStateStringMap->szName) ) == 0 )
        {
            State = pStateStringMap->Value;
            switch( pStateStringMap->Type )
            {
                case STATE_TYPE_ENUM:
                {
                    EnumStringMap* pEnumStringMap = pStateStringMap->EnumValues;
                    while( pEnumStringMap->szName != NULL )
                    {
                        if( _wcsnicmp( pEnumStringMap->szName, szValue, wcslen( pEnumStringMap->szName)  ) == 0 )
                        {
                            Value = pEnumStringMap->Value;
                            return TRUE;
                        }
                        pEnumStringMap++;
                    }
                    return FALSE;
                }

                case STATE_TYPE_ENUM_WITH_FLAGS:
                {
                    assert( pStateStringMap->FlagValues );

                    EnumStringMap* pEnumStringMap = pStateStringMap->EnumValues;
                    Value = 0;
                    while( pEnumStringMap->szName != NULL )
                    {
                        if( _wcsnicmp( pEnumStringMap->szName, szValue, wcslen( pEnumStringMap->szName)  ) == 0 )
                        {
                            Value = pEnumStringMap->Value;
                            break;
                        }
                        pEnumStringMap++;
                    }

                    while( *szValue != NULL )
                    {
                        szValue = SkipPast( szValue, L"| " );

                        if( *szValue != NULL )
                        {
                            EnumStringMap* pStringMap = pStateStringMap->FlagValues;
                            while( pStringMap->szName != NULL )
                            {
                                assert( pEnumStringMap->szName );
                                if( _wcsnicmp( pStringMap->szName, szValue, wcslen( pEnumStringMap->szName ) ) == 0 )
                                {
                                    Value |= pStringMap->Value;
                                    break;
                                }
                                pStringMap++;
                            }

                            // not found
                            if( pStringMap->szName == NULL )
                                return FALSE;
                         }
                    }
                    
                    return TRUE;
                    break;
                    
                }
                case STATE_TYPE_UINT_WITH_FLAGS:
                {
                    assert( !"Tested" ); 

                    assert( pStateStringMap->FlagValues );
                    Value = 0;
                    
                    DWORD NumRead = swscanf_s( szValue, L"%x", &Value );
                    if( NumRead != 1 )
                        return FALSE;

                    while( *szValue != NULL )
                    {
                        szValue = SkipPast( szValue, L"| " );

                        if( *szValue != NULL )
                        {
                            EnumStringMap* pEnumStringMap = pStateStringMap->FlagValues;
                            while( pEnumStringMap->szName != NULL )
                            {
                                if( _wcsnicmp( pEnumStringMap->szName, szValue, wcslen( pEnumStringMap->szName ) ) == 0 )
                                {
                                    Value |= pEnumStringMap->Value;
                                    break;
                                }
                                pEnumStringMap++;
                            }
                            // not found
                            if( pEnumStringMap->szName == NULL )
                                return FALSE;
                         }
                    }

                    return TRUE;
                }
                case STATE_TYPE_FLOAT:
                {
                    DWORD NumRead = swscanf_s( szValue, L"%f", &Value );
                     if( NumRead != 1 )
                        return FALSE;
                     return TRUE;
                }
                case STATE_TYPE_COLOR:
                case STATE_TYPE_MASK:
                {
                    DWORD NumRead = swscanf_s( szValue, L"%x", &Value );
                     if( NumRead != 1 )
                        return FALSE;
                     return TRUE;
                }
                case STATE_TYPE_UINT:
                {
                    DWORD NumRead = swscanf_s( szValue, L"%u", &Value );
                    if( NumRead != 1 )
                        return FALSE;
                    return TRUE;
                }
                case STATE_TYPE_INT:
                {
                    DWORD NumRead = swscanf_s( szValue, L"%d", &Value );
                    if( NumRead != 1 )
                        return FALSE;
                    return TRUE;
                }
                case STATE_TYPE_BOOL:
                {
                    if( _wcsicmp( szValue, L"TRUE" ) )
                        Value = TRUE;
                    else if( _wcsicmp( szValue, L"FALSE" ) )
                        Value = FALSE;
                    else
                        return FALSE;
                    return TRUE;

                }
                default:
                {
                    return FALSE;
                }
                        
            }
        }
        pStateStringMap++;
    }
    
    return FALSE;
}

} // namespace ATG
