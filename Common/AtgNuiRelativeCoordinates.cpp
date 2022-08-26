//--------------------------------------------------------------------------------------
// AtgNuiReletiveCoordinates.cpp
//
// This class maintains a body reletive coordainte system for NUI Samples
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgNuiRelativeCoordinates.h"
#include "AtgUtil.h"

namespace ATG
{
    //-------------------------------------------------------------------------------------
    // Name: Lerp()
    // Desc: Linear interpolation between two floats
    //-------------------------------------------------------------------------------------
    inline FLOAT Lerp( FLOAT f1, FLOAT f2, FLOAT fBlend )
    {
        return f1 + (f2-f1) * fBlend;
    }

 
    SpineRelativeCameraSpaceCoordinateSystem::SpineRelativeCameraSpaceCoordinateSystem( ) : 
        m_dwLastTrackingID( 0 ),
        m_fAverageSpineHeadLength( 0.0f ),
        m_vAverageNormalToGravity( XMVectorZero() )
    {    
        m_vAverageSpine = XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ); 
        m_vRightHandRelative = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f ); 
        m_vLeftHandRelative = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );

        // Call the updates function with default parameters
        SetUpdateRates(); 
    }

    //--------------------------------------------------------------------------------------
    // Name: SetUpdateRates()
    // Desc: Initializes the update rates used for filtering the spine coordinates and 
    //       body size related lengths used in this class. These are left and right hand length, 
    //       shoulder length, and spine to head length.  An update rate closer to 1.0f results 
    //       in more smoothing and adds some lagging when there is a fast body movement. 
    //--------------------------------------------------------------------------------------
    VOID SpineRelativeCameraSpaceCoordinateSystem::SetUpdateRates( FLOAT fSpineUpdateRate, FLOAT fBodySizeUpdateRate )
    {
        m_fBodySizeUpdateRate = max( 0.0f, min( 1.0f, fBodySizeUpdateRate ) );
        m_fSpineUpdateRate = max( 0.0f, min( 1.0f, fSpineUpdateRate ) );
    }

    //--------------------------------------------------------------------------------------
    // Name: Reset()
    // Desc: Resets the coordinate system. This method should be called if this class
    //       is to be used for a new player skeleton.
    //--------------------------------------------------------------------------------------
    VOID SpineRelativeCameraSpaceCoordinateSystem::Reset( )
    {
        m_vAverageNormalToGravity = XMVectorZero();
        m_dwLastTrackingID = 0;
        m_fAverageSpineHeadLength = 0.0f;
        m_vAverageSpine = XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ); 
        m_vRightHandRelative = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f ); 
        m_vLeftHandRelative = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
    }

    //--------------------------------------------------------------------------------------
    // Name: Update()
    // Desc: Adds a new frame of positions, updates the coordiante system, and calculates 
    //       left and right hand.
    //--------------------------------------------------------------------------------------
    VOID SpineRelativeCameraSpaceCoordinateSystem::Update( const NUI_SKELETON_FRAME* pRawSkeletonFrame, INT iSkeletonIndex, XMVECTOR vLeft, XMVECTOR vRight )
    {
        if ( pRawSkeletonFrame == NULL ) return;
        if ( iSkeletonIndex < 0 || iSkeletonIndex >= NUI_SKELETON_COUNT ) return;
        CONST XMVECTOR *pSkeletonPosition = &pRawSkeletonFrame->SkeletonData[iSkeletonIndex].SkeletonPositions[0];

        if ( m_dwLastTrackingID != pRawSkeletonFrame->SkeletonData[iSkeletonIndex].dwTrackingID )
        {
            m_vAverageNormalToGravity = pRawSkeletonFrame->vNormalToGravity;
        }
        else
        {
            m_vAverageNormalToGravity = m_fSpineUpdateRate * m_vAverageNormalToGravity +
                 pRawSkeletonFrame->vNormalToGravity * ( 1.0f - m_fSpineUpdateRate );
        }
        
#if 0
        CHAR out[255];
        sprintf_s( out, "x=%f,y=%f,z=%f,w=%f\n", pRawSkeletonFrame->vNormalToGravity.x, pRawSkeletonFrame->vNormalToGravity.y, pRawSkeletonFrame->vNormalToGravity.z, pRawSkeletonFrame->vNormalToGravity.w );
        OutputDebugString( out );
#endif 
        m_matRotateToNormalToGravity = NuiTransformMatrixLevel( m_vAverageNormalToGravity );
        XMVECTOR vSpineTilted = XMVector3Transform( pSkeletonPosition[NUI_SKELETON_POSITION_SPINE], m_matRotateToNormalToGravity );
        XMVECTOR vHeadTilted = XMVector3Transform( pSkeletonPosition[NUI_SKELETON_POSITION_HEAD], m_matRotateToNormalToGravity );
        m_vLeftHandRelative = XMVector3Transform( vLeft, m_matRotateToNormalToGravity );
        m_vRightHandRelative = XMVector3Transform( vRight, m_matRotateToNormalToGravity );


        FLOAT fSpineHeadLength = XMVectorGetY( vHeadTilted ) - XMVectorGetY( vSpineTilted );
        if ( m_dwLastTrackingID != pRawSkeletonFrame->SkeletonData[iSkeletonIndex].dwTrackingID )
        {
            m_dwLastTrackingID = pRawSkeletonFrame->SkeletonData[iSkeletonIndex].dwTrackingID;
            m_vAverageSpine = vSpineTilted;
            m_fAverageSpineHeadLength = fSpineHeadLength;
        }
        else
        {
            m_vAverageSpine = m_vAverageSpine * m_fSpineUpdateRate + 
                vSpineTilted * ( 1.0f - m_fSpineUpdateRate );
            m_fAverageSpineHeadLength = ATG::Lerp( fSpineHeadLength, m_fAverageSpineHeadLength, m_fBodySizeUpdateRate );
        }

        m_vEstiamtedPivotOffsetLeft = XMVectorSet( m_fAverageSpineHeadLength * 0.3f, m_fAverageSpineHeadLength * 0.1f, 0.0f, 0.0f );
        m_vEstiamtedPivotOffsetRight = XMVectorSet( -m_fAverageSpineHeadLength * 0.3f, m_fAverageSpineHeadLength * 0.1f, 0.0f, 0.0f );
        m_vRightHandRelative -= m_vAverageSpine;
        m_vRightHandRelative += m_vEstiamtedPivotOffsetRight;
        m_vLeftHandRelative -= m_vAverageSpine;
        m_vLeftHandRelative += m_vEstiamtedPivotOffsetLeft;
        static XMVECTOR vFlipZ = XMVectorSet( 1.0f, 1.0f, -1.0f, 1.0f );
        m_vRightHandRelative *= vFlipZ;
        m_vLeftHandRelative *= vFlipZ;

    }

    //--------------------------------------------------------------------------------------
    // Name: Update()
    // Desc: Returns the Right hand in the body reletive coordinate system.
    //--------------------------------------------------------------------------------------
    XMVECTOR SpineRelativeCameraSpaceCoordinateSystem::GetRightHandReletive()
    {
        return m_vRightHandRelative;
    }

    //--------------------------------------------------------------------------------------
    // Name: Update()
    // Desc: Returns the left hand in the body reletive coordinate system.
    //--------------------------------------------------------------------------------------
    XMVECTOR SpineRelativeCameraSpaceCoordinateSystem::GetLeftHandReletive() 
    {
        return m_vLeftHandRelative;    
    }

} // namespace ATG

