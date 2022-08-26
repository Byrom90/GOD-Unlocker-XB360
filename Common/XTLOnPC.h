//--------------------------------------------------------------------------------------
// XTLOnPC.h
//
// This module contains functions that allow most of the samples framework to compile
// on Windows using the Win32 XDK libraries.
//
// Some of the XTL memory functions are implemented here since the XTL libraries are
// not implemented in the Win32 XDK libraries.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef XTLONPC_H
#define XTLONPC_H

#ifdef _PC

#include <sal.h>

#define XMemCpy memcpy
#define XMemSet memset

LPVOID
WINAPI
XPhysicalAlloc(
               __in SIZE_T                      dwSize,
               __in_opt ULONG_PTR               ulPhysicalAddress,
               __in_opt ULONG_PTR               ulAlignment,
               __in DWORD                       flProtect
               );

VOID
WINAPI
XPhysicalFree(
              __in  LPVOID                      lpAddress
              );

LPVOID
WINAPI
XMemAlloc(
          __in    SIZE_T                      dwSize,
          __in    DWORD                       dwAllocAttributes
          );

VOID
WINAPI
XMemFree(
         __in_opt  PVOID                      pAddress,
         __in      DWORD                      dwAllocAttributes
         );

#define XALLOC_MEMTYPE_HEAP                         0
#define XALLOC_MEMTYPE_PHYSICAL                     1

#define XALLOC_MEMPROTECT_WRITECOMBINE_LARGE_PAGES  0
#define XALLOC_MEMPROTECT_NOCACHE                   1
#define XALLOC_MEMPROTECT_READWRITE                 2
#define XALLOC_MEMPROTECT_WRITECOMBINE              3

#define XALLOC_ALIGNMENT_DEFAULT                    0x0
#define XALLOC_ALIGNMENT_4                          0x1
#define XALLOC_ALIGNMENT_8                          0x2
#define XALLOC_ALIGNMENT_16                         0x4

#define XALLOC_PHYSICAL_ALIGNMENT_DEFAULT           0x0 // Default is 4K alignment
#define XALLOC_PHYSICAL_ALIGNMENT_4                 0x2
#define XALLOC_PHYSICAL_ALIGNMENT_8                 0x3
#define XALLOC_PHYSICAL_ALIGNMENT_16                0x4
#define XALLOC_PHYSICAL_ALIGNMENT_32                0x5
#define XALLOC_PHYSICAL_ALIGNMENT_64                0x6
#define XALLOC_PHYSICAL_ALIGNMENT_128               0x7
#define XALLOC_PHYSICAL_ALIGNMENT_256               0x8
#define XALLOC_PHYSICAL_ALIGNMENT_512               0x9
#define XALLOC_PHYSICAL_ALIGNMENT_1K                0xA
#define XALLOC_PHYSICAL_ALIGNMENT_2K                0xB
#define XALLOC_PHYSICAL_ALIGNMENT_4K                0xC
#define XALLOC_PHYSICAL_ALIGNMENT_8K                0xD
#define XALLOC_PHYSICAL_ALIGNMENT_16K               0xE
#define XALLOC_PHYSICAL_ALIGNMENT_32K               0xF

typedef enum _XALLOC_ALLOCATOR_IDS
{
    eXALLOCAllocatorId_GameMin = 0,
    eXALLOCAllocatorId_GameMax = 127,
    eXALLOCAllocatorId_MsReservedMin = 128,
    eXALLOCAllocatorId_D3D = 128,
    eXALLOCAllocatorId_D3DX,
    eXALLOCAllocatorId_XAUDIO,
    eXALLOCAllocatorId_XAPI,
    eXALLOCAllocatorId_XACT,
    eXALLOCAllocatorId_XBOXKERNEL,
    eXALLOCAllocatorId_XBDM,
    eXALLOCAllocatorId_XGRAPHICS,
    eXALLOCAllocatorId_XONLINE,
    eXALLOCAllocatorId_XVOICE,
    eXALLOCAllocatorId_XHV,
    eXALLOCAllocatorId_USB,
    eXALLOCAllocatorId_XMV,
    eXALLOCAllocatorId_SHADERCOMPILER,
    eXALLOCAllocatorId_XUI,
    eXALLOCAllocatorId_XASYNC,
    eXALLOCAllocatorId_XCAM,
    eXALLOCAllocatorId_XVIS,
    eXALLOCAllocatorId_XIME,
    eXALLOCAllocatorId_XFILECACHE,
    eXALLOCAllocatorId_XRN,
    eXALLOCAllocatorId_MsReservedMax = 191,
    eXALLOCAllocatorId_MiddlewareReservedMin = 192,
    eXALLOCAllocatorId_MiddlewareReservedMax = 255,
} XALLOC_ALLOCATOR_IDS;

#if defined(_M_PPCBE)
#pragma bitfield_order(push)
#pragma bitfield_order(lsb_to_msb)
#endif

typedef struct _XALLOC_ATTRIBUTES {
    DWORD                               dwObjectType:13;
    DWORD                               dwHeapTracksAttributes:1;
    DWORD                               dwMustSucceed:1;
    DWORD                               dwFixedSize:1;
    DWORD                               dwAllocatorId:8;
    DWORD                               dwAlignment:4;
    DWORD                               dwMemoryProtect:2;
    DWORD                               dwZeroInitialize:1;
    DWORD                               dwMemoryType:1;
} XALLOC_ATTRIBUTES, *PXALLOC_ATTRIBUTES;

#if defined(_M_PPCBE)
#pragma bitfield_order(pop)
#endif

#define MAKE_XALLOC_ATTRIBUTES(ObjectType,\
    HeapTracksAttributes,\
    MustSucceed,\
    FixedSize,\
    AllocatorId,\
    Alignment,\
    MemoryProtect,\
    ZeroInitialize,\
    MemoryType)\
    ((DWORD)( ObjectType | \
    (HeapTracksAttributes << 13) | \
    (MustSucceed << 14) | \
    (FixedSize << 15) | \
    (AllocatorId << 16) | \
    (Alignment << 24) | \
    (MemoryProtect << 28) | \
    (ZeroInitialize << 30) | \
    (MemoryType << 31)))

#define XALLOC_IS_PHYSICAL(Attributes)  ((BOOL)(Attributes & 0x80000000)!=0)

typedef struct _XVIDEO_MODE {
    DWORD                               dwDisplayWidth;
    DWORD                               dwDisplayHeight;
    BOOL                                fIsInterlaced;
    BOOL                                fIsWideScreen;
    BOOL                                fIsHiDef;
    float                               RefreshRate;
    DWORD                               VideoStandard;
    DWORD                               Reserved[5];
} XVIDEO_MODE, *PXVIDEO_MODE;

#define XC_VIDEO_STANDARD_NTSC_M        1
#define XC_VIDEO_STANDARD_NTSC_J        2
#define XC_VIDEO_STANDARD_PAL_I         3

VOID
WINAPI
XGetVideoMode(
              __out PXVIDEO_MODE                pVideoMode
              );

#endif // ifdef _PC

#endif // XTLONPC_H
