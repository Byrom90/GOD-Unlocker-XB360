//--------------------------------------------------------------------------------------
// AtgNuiReletiveCoordinates.h
//
// This class maintains a body reletive coordainte system for NUI Samples
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifndef _ATG_NUI_BODY_RELETIVE_COORDINATES_
#define _ATG_NUI_BODY_RELETIVE_COORDINATES_

#include <nuiapi.h>
#include <xnamath.h>
#include "ATGNuiJointFilter.h"
#include "AtgUtil.h"

#pragma warning( disable:4324 )

namespace ATG
{

static const FLOAT BODY_RELATIVE_SPINE_UPDATE_RATE = 0.95f;
static const FLOAT BODY_RELATIVE_BODY_SIZE_UPDATE_RATE = 0.98f;

class SpineRelativeCameraSpaceCoordinateSystem
{
public:

    SpineRelativeCameraSpaceCoordinateSystem( );

    VOID SetUpdateRates( FLOAT fSpineUpdateRate = BODY_RELATIVE_SPINE_UPDATE_RATE, FLOAT fBodySizeUpdateRate = BODY_RELATIVE_BODY_SIZE_UPDATE_RATE );

    VOID Reset( );

    VOID Update( const NUI_SKELETON_FRAME* pRawSkeletonFrame, INT iSkeletonIndex, XMVECTOR vLeft, XMVECTOR vRight );

    XMVECTOR GetRightHandReletive( );
    XMVECTOR GetLeftHandReletive( );    
    XMVECTOR GetAverageSpine( ) { return m_vAverageSpine; };
    XMMATRIX GetRotateToNormalToGravityMatrix() { return m_matRotateToNormalToGravity; };
    XMVECTOR GetEstiamtedPivotOffsetLeft() { return m_vEstiamtedPivotOffsetLeft; };
    XMVECTOR GetEstiamtedPivotOffsetRight() { return m_vEstiamtedPivotOffsetRight; };
    FLOAT GetAverageSpineHeadLength() { return m_fAverageSpineHeadLength; };

private:

    DWORD       m_dwLastTrackingID;
    
    XMVECTOR    m_vAverageSpine;
    FLOAT       m_fAverageSpineHeadLength;

    FLOAT       m_fBodySizeUpdateRate;
    FLOAT       m_fSpineUpdateRate;
    
    XMVECTOR    m_vAverageNormalToGravity;
    XMVECTOR    m_vRightHandRelative;       
    XMVECTOR    m_vLeftHandRelative;
    XMMATRIX    m_matRotateToNormalToGravity;
    XMVECTOR    m_vEstiamtedPivotOffsetLeft;
    XMVECTOR    m_vEstiamtedPivotOffsetRight;

   
};

} // namespace ATG

#endif  // _ATG_NUI_BODY_RELETIVE_COORDINATES_