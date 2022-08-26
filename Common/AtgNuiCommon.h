//--------------------------------------------------------------------------------------
// AtgNuiCommon.h
//
// Common defines and macros for NUI samples 
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef ATG_NUI_COMMON_H
#define ATG_NUI_COMMON_H

#include <NuiApi.h>
#include "AtgUtil.h"

namespace ATG
{
    // NuiSkeletonGetNextFrame and NuiImageStreamGetNextFrame require a maximum wait time to be passed into the function.  
    // If this is 0, then the app can easily become 1 frame behind the camera introducing lag into the system.
    // This value is a maximum time so the app will likely return before the 10 ms.
    // 10 to 15 ms has been a good amount of time to wait.
    // Alternatively an application could do 10 to 15 ms of work and pass in 0.
    #define NUI_CAMERA_TIMEOUT_DEFAULT 10

    // The event set via NuiSetFrameEndEvent is used to synchronize reads with NuiSkeletonGetNextFrame and NuiImageStreamGetNextFrame.
    // When waiting on this event, the timeout needs to be long enough that frame processing will complete and be synchronized.
    // 100 ms ensures proper synchronization with NUI when using NuiSetFrameEndEvent.
    // When using NuiSetFrameEndEvent, calls to NuiSkeletonGetNextFrame and NuiImageStreamGetNextFrame should use a timeout of 0.
    #define NUI_FRAME_END_TIMEOUT_DEFAULT 100

    VOID NuiPrintError( HRESULT hResult, const CHAR* szFunctionName );

    VOID ApplyTiltCorrectionInPlayerSpace( NUI_SKELETON_FRAME* pDstSkeleton, const NUI_SKELETON_FRAME* pSrcSkeleton );

    const WCHAR* GetIdentityQualityFlagPrompt( DWORD dwIdentityQualityFlags );
    const WCHAR* GetHeadOrientationQualityFlagPrompt( DWORD dwHeadOrientationQualityFlags );
};

#endif // ATG_NUI_COMMON_H