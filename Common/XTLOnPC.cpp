//--------------------------------------------------------------------------------------
// XTLOnPC.cpp
//
// This module contains functions that allow most of the samples framework to compile
// on Windows using the Win32 XDK libraries.
//
// Some of the XTL memory functions are implemented here since the XTL libraries are
// not implemented in the Win32 XDK libraries.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include <stdio.h>

#ifdef _PC

LPVOID
WINAPI
XPhysicalAlloc(
               SIZE_T                      dwSize,
               ULONG_PTR                   ulPhysicalAddress,
               ULONG_PTR                   ulAlignment,
               DWORD                       flProtect
               )
{
    DWORD vaProtect = 0;

    if (flProtect & PAGE_READONLY)
        vaProtect = PAGE_READONLY;
    else if (flProtect & PAGE_READWRITE)
        vaProtect = PAGE_READWRITE;
    else
        return NULL;

    // Always 4K aligned
    return VirtualAlloc( NULL, dwSize, MEM_COMMIT, vaProtect );
}

VOID
WINAPI
XPhysicalFree(
              LPVOID                      lpAddress
              )
{
    VirtualFree( lpAddress, 0, MEM_RELEASE );
}

LPVOID
WINAPI
XMemAlloc(
          SIZE_T                      dwSize,
          DWORD                       dwAllocAttributes
          )
{
    const PXALLOC_ATTRIBUTES alloc = (PXALLOC_ATTRIBUTES)(&dwAllocAttributes);   

    void *ptr = NULL;

    if ( alloc->dwMemoryType ==  XALLOC_MEMTYPE_HEAP)
    {
        size_t align = 16;

        switch ( alloc->dwAlignment )
        {
        case XALLOC_ALIGNMENT_DEFAULT:
        case XALLOC_ALIGNMENT_16:
            break;
    
        case XALLOC_ALIGNMENT_4:
            align = 4;
            break;
        
        case XALLOC_ALIGNMENT_8:
            align = 8;
            break;
        }

        ptr = _aligned_malloc( dwSize, align );
    }
    else if ( alloc->dwMemoryType == XALLOC_MEMTYPE_PHYSICAL )
    {
        // Always 4K aligned, which works for everything except XALLOC_PHYSICAL_ALIGNMENT_8K, _16K, and _32K
        ptr = VirtualAlloc( NULL, dwSize, MEM_COMMIT, PAGE_READWRITE );
    }

    if (ptr && alloc->dwZeroInitialize )
    {
        memset( ptr, 0, dwSize );
    }

    return ptr;
}

VOID
WINAPI
XMemFree(
         PVOID                       pAddress,
         DWORD                       dwAllocAttributes
         )
{
    if ( !pAddress )
        return;

    const PXALLOC_ATTRIBUTES alloc = (PXALLOC_ATTRIBUTES)(&dwAllocAttributes);

    if ( alloc->dwMemoryType ==  XALLOC_MEMTYPE_HEAP)
    {
        _aligned_free( pAddress );
    }
    else if ( alloc->dwMemoryType == XALLOC_MEMTYPE_PHYSICAL )
    {
        VirtualFree( pAddress, 0, MEM_RELEASE );
    }
}

VOID
WINAPI
XGetVideoMode(
              PXVIDEO_MODE                pVideoMode
              )
{
    // This requires knowing the device, access to DXGI, and other video parameters. For Windows we just return zero data for simplicity
    ZeroMemory( pVideoMode, sizeof( XVIDEO_MODE ) );
}

#endif // ifdef _PC
