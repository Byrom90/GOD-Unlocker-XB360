//--------------------------------------------------------------------------------------
// AtgDevice.h
//
// Over-loaded device to trap and optimize certain calls to D3D
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGDEVICE_H
#define ATGDEVICE_H

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: CreatePooledVertexDeclaration()
// Desc: Function to coalesce vertex declarations into a shared pool of vertex declarations
//--------------------------------------------------------------------------------------
HRESULT WINAPI CreatePooledVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements,
                                              D3DVertexDeclaration** ppVertexDeclaration );
}

#ifdef _XBOX

namespace ATG
{

//--------------------------------------------------------------------------------------
// struct ATG::D3DDevice
// Over-loaded device to trap and optimize certain calls to D3D
//--------------------------------------------------------------------------------------
struct D3DDevice : public ::D3DDevice
{
    D3DVOID WINAPI SetVertexShader( D3DVertexShader *pShader );
    D3DVOID WINAPI SetPixelShader( D3DPixelShader* pShader );
    D3DVOID WINAPI SetVertexDeclaration( D3DVertexDeclaration *pDecl);
    HRESULT WINAPI CreateVertexDeclaration( const D3DVERTEXELEMENT9* pVertexElements,
                                            D3DVertexDeclaration **ppVertexDeclaration );
};


} // namespace ATG

#endif // _XBOX

#endif // ATGDEVICE_H
