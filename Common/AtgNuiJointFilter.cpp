//--------------------------------------------------------------------------------------
// AtgNuiJointFilter.cpp
//
// This file contains various filters for filtering Joints
//
// Microsoft Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "ATGNuiJointFilter.h"

namespace ATG
{
    //-------------------------------------------------------------------------------------
    // Name: Lerp()
    // Desc: Linear interpolation between two floatss
    //-------------------------------------------------------------------------------------
    inline FLOAT Lerp( FLOAT f1, FLOAT f2, FLOAT fBlend )
    {
        return f1 + (f2-f1) * fBlend;
    }
 
    
    //--------------------------------------------------------------------------------------
    // The Blend Update function adds a new frame and then creates an average by summing and dividing the new joints
    // This could be done without storing 10 frames of history if memory is a concern.
    //--------------------------------------------------------------------------------------
    VOID FilterBlendJoint::Update( const XMVECTOR* pJoints )
    {
        
        memcpy( &m_pSkeletonsData[ m_iCurrentFrame ], pJoints, sizeof( FilterSkeletonData ) );
        ++ m_iCurrentFrame;
        m_iCurrentFrame %= m_nFramesToAverage;

        memset( &m_FilteredSkeleton, 0, sizeof( FilterSkeletonData ) );
        for ( int index=0; index < m_nFramesToAverage; ++index )
        {
            for ( int iCount = 0; iCount < NUI_SKELETON_POSITION_COUNT; ++iCount )
            {
                m_FilteredSkeleton.m_Positions[iCount] += m_pSkeletonsData[index].m_Positions[iCount];
            }
        }
        for ( int iCount = 0; iCount < NUI_SKELETON_POSITION_COUNT; ++iCount )
        {
            m_FilteredSkeleton.m_Positions[iCount] /= (FLOAT)m_nFramesToAverage;
        }
        

    }
    //--------------------------------------------------------------------------------------
    // Return a pointer to the filtered joints
    //--------------------------------------------------------------------------------------

    XMVECTOR* FilterBlendJoint::GetFilteredJoints( )
    {
        return &m_FilteredSkeleton.m_Positions[0];
    }

    //--------------------------------------------------------------------------------------
    // The VelDamp function estimates velocity as part of the smoothing function
    // 
    //--------------------------------------------------------------------------------------
    VOID FilterVelDamp::Update( const XMVECTOR* pJoints )
    {
        if ( m_iPreviousFrame == -1 )
        {
            memcpy( &m_SkeletonData[0], pJoints, sizeof( FilterSkeletonData ) );
            m_iPreviousFrame = 1;

			// Redundancy to ensure both previous and current frames are checked
			if ( m_iCurrentFrame == -1 )
			{
				memcpy( &m_SkeletonData[1], pJoints, sizeof( FilterSkeletonData ) );
				m_iCurrentFrame = 0;
			}
        }
        else if ( m_iCurrentFrame == -1 )
        {
            memcpy( &m_SkeletonData[1], pJoints, sizeof( FilterSkeletonData ) );
            m_iCurrentFrame = 0;    
        }
        else 
        {
            ++m_iPreviousFrame;
            m_iPreviousFrame%=2;
            ++m_iCurrentFrame;
            m_iCurrentFrame%=2;
            memcpy( &m_SkeletonData[m_iCurrentFrame], pJoints, sizeof( FilterSkeletonData ) );
            
            static const FLOAT  fAlphaCoeff = 0.4f;
            static const FLOAT  fBetaCoeff = ( fAlphaCoeff * fAlphaCoeff ) / ( 2.0f - fAlphaCoeff );
            
            FilterSkeletonData* pPreviousSkeleton = &m_SkeletonData[m_iPreviousFrame];
            FilterSkeletonData* pCurrentSkeleton = &m_SkeletonData[m_iCurrentFrame];
            FilterVelocitySingleEstimate* pPreviousEstimate = &m_VelocityEstimate[m_iPreviousFrame];
            FilterVelocitySingleEstimate* pCurrentEstimate = &m_VelocityEstimate[m_iCurrentFrame];

            XMVECTOR            vPredicted;
            XMVECTOR            vError;

            for ( INT joint = 0; joint < NUI_SKELETON_POSITION_COUNT ; ++joint )
            {
                // Calculate vPredicted position using last frames corrected position and velocity.
                vPredicted = XMVectorAdd( pPreviousSkeleton->m_Positions[ joint ], 
                    pPreviousEstimate->m_vEstVel[ joint ] );

                // Calculate vError between vPredicted and measured position.
                vError = XMVectorSubtract( pCurrentSkeleton->m_Positions[ joint ], vPredicted );

                // Calculate corrected position.
                pCurrentSkeleton->m_Positions[ joint ] = XMVectorAdd( vPredicted, vError * fAlphaCoeff );

                // Calculate corrected velocity.
                pCurrentEstimate->m_vEstVel[ joint ]= XMVectorAdd( pPreviousEstimate->m_vEstVel[ joint ], vError * fBetaCoeff );
            }

        }
    }
    //--------------------------------------------------------------------------------------
    // Return the filtered joints.
    //--------------------------------------------------------------------------------------

    XMVECTOR* FilterVelDamp::GetFilteredJoints( )
    {
        return &m_SkeletonData[m_iCurrentFrame].m_Positions[0];
    }

    //--------------------------------------------------------------------------------------
    // if joint is 0 it is not valid.
    //--------------------------------------------------------------------------------------
    inline BOOL JointPositionIsValid(XMVECTOR vJointPosition)
    {
        return (vJointPosition.x != 0.0f ||
                vJointPosition.y != 0.0f ||
                vJointPosition.z != 0.0f);
    }



    // Description of connected joints
    typedef struct Bone
    {
        NUI_SKELETON_POSITION_INDEX startJoint;
        NUI_SKELETON_POSITION_INDEX endJoint;
    } Bone;


    // Define the bones in the skeleton using joint indices
    const Bone g_Bones[] =
    {
        // Spine
        {  NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_SPINE  },                // Spine to hip center
        {  NUI_SKELETON_POSITION_SPINE, NUI_SKELETON_POSITION_SHOULDER_CENTER },            // Neck bottom to spine

        // Head
        { NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_HEAD  },             // Top of head to top of neck

        // Right arm
        { NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_RIGHT },    // Neck bottom to right shoulder internal
        { NUI_SKELETON_POSITION_SHOULDER_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT },        // Right shoulder internal to right elbow
        { NUI_SKELETON_POSITION_ELBOW_RIGHT, NUI_SKELETON_POSITION_WRIST_RIGHT },           // Right elbow to right wrist
        { NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT },            // Right wrist to right hand

        // Left arm
        { NUI_SKELETON_POSITION_SHOULDER_CENTER, NUI_SKELETON_POSITION_SHOULDER_LEFT },     // Neck bottom to left shoulder internal
        { NUI_SKELETON_POSITION_SHOULDER_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT },          // Left shoulder internal to left elbow
        { NUI_SKELETON_POSITION_ELBOW_LEFT, NUI_SKELETON_POSITION_WRIST_LEFT },             // Left elbow to left wrist
        { NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT },              // Left wrist to left hand

        // Right leg and foot
        { NUI_SKELETON_POSITION_HIP_RIGHT, NUI_SKELETON_POSITION_KNEE_RIGHT },              // Right hip internal to right knee
        { NUI_SKELETON_POSITION_KNEE_RIGHT, NUI_SKELETON_POSITION_ANKLE_RIGHT },            // Right knee to right ankle

        { NUI_SKELETON_POSITION_ANKLE_RIGHT, NUI_SKELETON_POSITION_FOOT_RIGHT },            // Left ankle to left foot 

        // Left leg and foot
        { NUI_SKELETON_POSITION_HIP_LEFT, NUI_SKELETON_POSITION_KNEE_LEFT },                // Left hip internal to left knee
        { NUI_SKELETON_POSITION_KNEE_LEFT, NUI_SKELETON_POSITION_ANKLE_LEFT },              // Left knee to left ankle

        { NUI_SKELETON_POSITION_ANKLE_LEFT, NUI_SKELETON_POSITION_FOOT_LEFT },              // Left ankle to left foot 

        // Hips
        { NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_RIGHT },              // Right hip to hip center
        { NUI_SKELETON_POSITION_HIP_CENTER, NUI_SKELETON_POSITION_HIP_LEFT }                // Hip center to left hip

    };

    const UINT g_numBones = ARRAYSIZE( g_Bones );

    //--------------------------------------------------------------------------------------
    // Name: CombiJointFilter()
    // Desc: A filter for the positional data.  This filter uses a combination of velocity 
    //       position history to filter the joint positions.
    //--------------------------------------------------------------------------------------
    VOID FilterCombination::Update( const XMVECTOR* pJointPositions )
    {
        // Process each joint
        for ( UINT nJoint = 0; nJoint < NUI_SKELETON_POSITION_COUNT; ++nJoint )
        {
            // Remember where the camera thinks this joint should be
            m_History[ nJoint ].m_vWantedPos = pJointPositions[ nJoint ];

            XMVECTOR vDelta;
            vDelta = XMVectorSubtract( m_History[ nJoint ].m_vWantedPos, m_History[ nJoint ].m_vLastWantedPos);
            {
                XMVECTOR vBlended;

                // Calculate the vBlended value - could optimize this by remembering the running total and
                // subtracting the oldest value and then adding the newest. Saves adding them all up on each frame.
                vBlended = XMVectorZero();
                for( UINT k = 0; k < m_nUseTaps; ++k)
                {
                    vBlended = XMVectorAdd( vBlended, m_History[ nJoint ].m_vPrevDeltas[k] );
                }
                vBlended = vBlended / ((FLOAT)m_nUseTaps);

                FLOAT fDeltaLength;
                FLOAT fBlendedLength;
                m_History[ nJoint ].m_fWantedLocalBlendRate = m_fDefaultApplyRate;
                m_History[ nJoint ].m_bActive[0] = FALSE;
                m_History[ nJoint ].m_bActive[1] = FALSE;
                m_History[ nJoint ].m_bActive[2] = FALSE;

                XMVECTOR vDeltaLength = XMVector3Length( vDelta );
                XMVECTOR vBlendedLength = XMVector3Length( vBlended );
                fDeltaLength = XMVectorGetX( vDeltaLength );
                fBlendedLength = XMVectorGetX( vBlendedLength );

                // Does the current velocity and history have a reasonable magnitude?
                if( fDeltaLength   >= m_fDeltaLengthThreshold &&
                    fBlendedLength >= m_fBlendedLengthThreshold )
                {
                    FLOAT fDotProd;
                    FLOAT fConfidence;
                    XMVECTOR vDeltaOne;
                    XMVECTOR vBlendedOne;

                    if( m_bDotProdNormalize )
                    {
                        vDeltaOne = XMVector3Normalize( vDelta );
                        vBlendedOne = XMVector3Normalize( vBlended );
                        XMVECTOR vDotProd = XMVector3Dot( vDeltaOne, vBlendedOne );
                        fDotProd = XMVectorGetX( vDotProd );
                    }
                    else
                    {
                        XMVECTOR vDotProd = XMVector3Dot( vDelta, vBlended );
                        fDotProd = XMVectorGetX( vDotProd );
                    }

                    // Is the current frame aligned to the recent history?
                    if( fDotProd >= m_fDotProdThreshold )
                    {
                        fConfidence = fDotProd;
                        m_History[ nJoint ].m_fWantedLocalBlendRate = min( fConfidence, 1.0f );
                        m_History[ nJoint ].m_bActive[0] = TRUE;
                    }
                }

                assert( m_History[ nJoint ].m_fWantedLocalBlendRate <= 1.0f );
            }

            // Push the previous deltas down the history
            for( INT j = m_nUseTaps-2; j >= 0; --j )
            {
                m_History[ nJoint ].m_vPrevDeltas[j+1] = m_History[ nJoint ].m_vPrevDeltas[j];
            }

            // Store the current history
            m_History[ nJoint ].m_vPrevDeltas[0] = vDelta;	

            // Remember where the camera thought this joint was on the this frame
            m_History[ nJoint ].m_vLastWantedPos = m_History[ nJoint ].m_vWantedPos;
        }

        // Secondary and tertiary blending
        for ( UINT pass = 0; pass < 2; ++pass )
        {
            for ( UINT bone = 0; bone < g_numBones; ++bone )
            {
                FLOAT fRate1;
                FLOAT fRate2;

                fRate1 = m_History[ g_Bones[bone].startJoint ].m_fWantedLocalBlendRate;
                fRate2 = m_History[ g_Bones[bone].endJoint ].m_fWantedLocalBlendRate;

                // Blend down? Start to end
                if( (fRate1 * m_fDownBlendRate) > fRate2)
                {
                    // Yes, apply
                    m_History[ g_Bones[bone].endJoint ].m_fWantedLocalBlendRate = ( fRate1 * m_fDownBlendRate );

                    // Flag
                    m_History[ g_Bones[bone].endJoint ].m_bActive[pass+1] = TRUE;
                }
                // Blend down? End to start
                if( ( fRate2 * m_fDownBlendRate ) > fRate1)
                {
                    // Yes, apply
                    m_History[ g_Bones[bone].startJoint ].m_fWantedLocalBlendRate = ( fRate2 * m_fDownBlendRate );

                    // Flag
                    m_History[ g_Bones[bone].startJoint ].m_bActive[pass+1] = TRUE;
                }
            }
        }

        // Apply
        for ( UINT joint = 0; joint < NUI_SKELETON_POSITION_COUNT; ++joint )
        {
            // Blend the blend rate
            m_History[ joint ].m_fActualLocalBlendRate = 
                    Lerp(m_History[ joint ].m_fActualLocalBlendRate,
                         m_History[ joint ].m_fWantedLocalBlendRate,
                         m_fBlendBlendRate);

            // Blend the actual position towards the wanted positon
            m_History[ joint ].m_vPos = 
                    XMVectorLerp(m_History[ joint ].m_vPos,
                                 m_History[ joint ].m_vWantedPos,
                                 m_History[ joint ].m_fActualLocalBlendRate);
            m_FilteredJoints[ joint ] = m_History[ joint ].m_vPos;
        }
    }


    //--------------------------------------------------------------------------------------
    // Implementation of a Holt Double Exponential Smoothing filter. The double exponential
    // smooths the curve and predicts.  There is also noise jitter removal. And maximum
    // prediction bounds.  The paramaters are commented in the init function.
    //--------------------------------------------------------------------------------------
    VOID FilterDoubleExponential::Update( const NUI_SKELETON_DATA* pSkeletonData )
    {
        // Check for divide by zero. Use an epsilon of a 10th of a millimeter
        m_fJitterRadius = max(0.0001f, m_fJitterRadius);

        NUI_TRANSFORM_SMOOTH_PARAMETERS SmoothingParams;

        for (UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
        {
            SmoothingParams.fSmoothing      = m_fSmoothing;
            SmoothingParams.fCorrection     = m_fCorrection;
            SmoothingParams.fPrediction     = m_fPrediction;
            SmoothingParams.fJitterRadius   = m_fJitterRadius;
            SmoothingParams.fMaxDeviationRadius = m_fMaxDeviationRadius;

            // If inferred, we smooth a bit more by using a bigger jitter radius
            if (pSkeletonData->eSkeletonPositionTrackingState[i] == NUI_SKELETON_POSITION_INFERRED)
            {
                SmoothingParams.fJitterRadius       *= 2.0f;
                SmoothingParams.fMaxDeviationRadius *= 2.0f;
            }

            Update( pSkeletonData, i, SmoothingParams );
        }
    }

    VOID FilterDoubleExponential::Update( const NUI_SKELETON_DATA* pSkeletonData, UINT i, NUI_TRANSFORM_SMOOTH_PARAMETERS smoothingParams )
    {
        XMVECTOR vPrevRawPosition;
        XMVECTOR vPrevFilteredPosition;
        XMVECTOR vPrevTrend;
        XMVECTOR vRawPosition;
        XMVECTOR vFilteredPosition;
        XMVECTOR vPredictedPosition;
        XMVECTOR vDiff;
        XMVECTOR vTrend;
        XMVECTOR vLength;
        FLOAT fDiff;
        BOOL bJointIsValid;

        const XMVECTOR* __restrict pJointPositions = pSkeletonData->SkeletonPositions;

        vRawPosition            = pJointPositions[i];
        vPrevFilteredPosition   = m_History[i].m_vFilteredPosition;
        vPrevTrend              = m_History[i].m_vTrend;
        vPrevRawPosition        = m_History[i].m_vRawPosition;
        bJointIsValid           = JointPositionIsValid(vRawPosition);

        // If joint is invalid, reset the filter
        if (!bJointIsValid)
        {
            m_History[i].m_dwFrameCount = 0;
        }

        // Initial start values
        if (m_History[i].m_dwFrameCount == 0)
        {
            vFilteredPosition = vRawPosition;
            vTrend = XMVectorZero();
            m_History[i].m_dwFrameCount++;
        }
        else if (m_History[i].m_dwFrameCount == 1)
        {
            vFilteredPosition = XMVectorScale(XMVectorAdd(vRawPosition, vPrevRawPosition), 0.5f);
            vDiff = XMVectorSubtract(vFilteredPosition, vPrevFilteredPosition);
            vTrend = XMVectorAdd(XMVectorScale(vDiff, smoothingParams.fCorrection), XMVectorScale(vPrevTrend, 1.0f - smoothingParams.fCorrection));
            m_History[i].m_dwFrameCount++;
        }
        else
        {              
            // First apply jitter filter
            vDiff = XMVectorSubtract(vRawPosition, vPrevFilteredPosition);
            vLength = XMVector3Length(vDiff);
            fDiff = fabs(XMVectorGetX(vLength));

            if (fDiff <= smoothingParams.fJitterRadius)
            {
                vFilteredPosition = XMVectorAdd(XMVectorScale(vRawPosition, fDiff/smoothingParams.fJitterRadius),
                                                XMVectorScale(vPrevFilteredPosition, 1.0f - fDiff/smoothingParams.fJitterRadius));
            }
            else
            {
                vFilteredPosition = vRawPosition;
            }

            // Now the double exponential smoothing filter
            vFilteredPosition = XMVectorAdd(XMVectorScale(vFilteredPosition, 1.0f - smoothingParams.fSmoothing),
                                            XMVectorScale(XMVectorAdd(vPrevFilteredPosition, vPrevTrend), smoothingParams.fSmoothing));


            vDiff = XMVectorSubtract(vFilteredPosition, vPrevFilteredPosition);
            vTrend = XMVectorAdd(XMVectorScale(vDiff, smoothingParams.fCorrection), XMVectorScale(vPrevTrend, 1.0f - smoothingParams.fCorrection));
        }      

        // Predict into the future to reduce latency
        vPredictedPosition = XMVectorAdd(vFilteredPosition, XMVectorScale(vTrend, smoothingParams.fPrediction));

        // Check that we are not too far away from raw data
        vDiff = XMVectorSubtract(vPredictedPosition, vRawPosition);
        vLength = XMVector3Length(vDiff);
        fDiff = fabs(XMVectorGetX(vLength));

        if (fDiff > smoothingParams.fMaxDeviationRadius)
        {
            vPredictedPosition = XMVectorAdd(XMVectorScale(vPredictedPosition, smoothingParams.fMaxDeviationRadius/fDiff),
                                             XMVectorScale(vRawPosition, 1.0f - smoothingParams.fMaxDeviationRadius/fDiff));
        }

        // Save the data from this frame
        m_History[i].m_vRawPosition      = vRawPosition;
        m_History[i].m_vFilteredPosition = vFilteredPosition;
        m_History[i].m_vTrend            = vTrend;
        
        // Output the data
        m_FilteredJoints[i] = vPredictedPosition;
        m_FilteredJoints[i].w = 1.0f;
    }


    //--------------------------------------------------------------------------------------
    // The Taylor Series smooths and removes jitter based on a taylor series expansion
    //--------------------------------------------------------------------------------------
    VOID FilterTaylorSeries::Update( const XMVECTOR* pJointPositions )
    {
        const FLOAT fJitterRadius = 0.05f;
        const FLOAT fAlphaCoef  = 1.0f - m_fSmoothing;
        const FLOAT fBetaCoeff  = (fAlphaCoef * fAlphaCoef ) / ( 2 - fAlphaCoef );

        XMVECTOR vRawPos;
        // Velocity, accelaration and Jolt are 1st, 2nd and 3rd degree derivatives of position respectively. 
        XMVECTOR vCurFilteredPos, vEstVelocity, vEstAccelaration, vEstJolt;
        XMVECTOR vPrevFilteredPos, vPrevEstVelocity, vPrevEstAccelaration, vPrevEstJolt;
        XMVECTOR vDiff;
        FLOAT fDiff;

        XMVECTOR vPredicted, vError;
        XMVECTOR vConstants = { 0.0f, 1.0f, 0.5f, 0.1667f };

        for (INT i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
        {
            vRawPos             = pJointPositions[i];
            vPrevFilteredPos    = m_History[i].vPos;
            vPrevEstVelocity    = m_History[i].vEstVelocity;
            vPrevEstAccelaration = m_History[i].vEstAccelaration;
            vPrevEstJolt        = m_History[i].vEstJolt;

            if (!JointPositionIsValid(vPrevFilteredPos))
            {
                vCurFilteredPos = vRawPos;
                vEstVelocity      = XMVectorZero();
                vEstAccelaration     = XMVectorZero();
                vEstJolt     = XMVectorZero();
            }
            else if (!JointPositionIsValid(vRawPos))
            {
                vCurFilteredPos = vPrevFilteredPos;
                vEstVelocity = vPrevEstVelocity;
                vEstAccelaration = vPrevEstAccelaration;
                vEstJolt = vPrevEstJolt;
            }
            else
            {
                // If the current and previous frames have valid data, perform interpolation

                vDiff = XMVectorSubtract(vPrevFilteredPos, vRawPos);
                fDiff = fabs(XMVector3Length(vDiff).x);

                if (fDiff <= fJitterRadius)
                {
                    vCurFilteredPos = XMVectorAdd(XMVectorScale(vRawPos, fDiff/fJitterRadius),
                                                  XMVectorScale(vPrevFilteredPos, 1.0f - fDiff/fJitterRadius));
                }
                else
                {
                    vCurFilteredPos = vRawPos;
                }

                vPredicted  = XMVectorAdd(vPrevFilteredPos, vPrevEstVelocity);
                vPredicted  = XMVectorAdd(vPredicted, vPrevEstAccelaration * (vConstants.y * vConstants.y * vConstants.z));
                vPredicted  = XMVectorAdd(vPredicted, vPrevEstJolt * (vConstants.y * vConstants.y * vConstants.y * vConstants.w));
                vError      = XMVectorSubtract(vCurFilteredPos, vPredicted);

                vCurFilteredPos = XMVectorAdd(vPredicted, vError * fAlphaCoef);
                vEstVelocity = XMVectorAdd(vPrevEstVelocity, vError * fBetaCoeff);
                vEstAccelaration = XMVectorSubtract(vEstVelocity, vPrevEstVelocity);
                vEstJolt = XMVectorSubtract(vEstAccelaration, vPrevEstAccelaration);
            }

            // Update the state
            m_History[i].vPos = vCurFilteredPos;
            m_History[i].vEstVelocity = vEstVelocity;
            m_History[i].vEstAccelaration = vEstAccelaration;
            m_History[i].vEstJolt = vEstJolt;
          
            // Output the data
            m_FilteredJoints[i]     = vCurFilteredPos;
            m_FilteredJoints[i].w   = 1.0f;
        }
    }


    //--------------------------------------------------------------------------------------
    // Name: FilterAdaptiveDoubleExponential::Init()
    // Desc: Initializes the filter
    //--------------------------------------------------------------------------------------
    VOID FilterAdaptiveDoubleExponential::Init()
    {
        // Paramters for a high latency filter with smooth results, but laggy.
        m_HighLatencySmoothingParams.fSmoothing             = 0.6f;
        m_HighLatencySmoothingParams.fCorrection            = 0.2f;
        m_HighLatencySmoothingParams.fPrediction            = 0.1f;
        m_HighLatencySmoothingParams.fJitterRadius          = 0.1f;
        m_HighLatencySmoothingParams.fMaxDeviationRadius    = 0.1f;

        // Paramters for a low latency fitler with somewhat smooth results, but responsive
        m_LowLatencySmoothingParams.fSmoothing              = 0.5f;
        m_LowLatencySmoothingParams.fCorrection             = 0.5f;
        m_LowLatencySmoothingParams.fPrediction             = 0.5f;
        m_LowLatencySmoothingParams.fJitterRadius           = 0.1f;
        m_LowLatencySmoothingParams.fMaxDeviationRadius     = 0.1f;

        Reset();
        m_DoubleExponentialFilter.Init();
    }


    //--------------------------------------------------------------------------------------
    // Name: FilterAdaptiveDoubleExponential::UpdateSmoothingParameters()
    // Desc: Updates the smoothing parameters based on the smoothing filter's trend
    //--------------------------------------------------------------------------------------
    VOID FilterAdaptiveDoubleExponential::Update( const NUI_SKELETON_DATA* pSkeletonData, const FLOAT fDeltaTime  )
    {
        for (UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
        {
            XMVECTOR vPreviousPosition  = m_DoubleExponentialFilter.m_History[ i ].m_vRawPosition;
            XMVECTOR vCurrentPosition   = pSkeletonData->SkeletonPositions[ i ];
            XMVECTOR vVelocity          = ( vCurrentPosition - vPreviousPosition ) / fDeltaTime;
            FLOAT fVelocity             = fabsf( XMVectorGetX( XMVector3Length( vVelocity ) ) );

            UpdateSmoothingParameters( i, fVelocity, pSkeletonData->eSkeletonPositionTrackingState[i] );

            m_DoubleExponentialFilter.Update( pSkeletonData, i, m_SmoothingParams[ i ] );
        }

        // Copy filtered data to output data
        XMemCpy( m_FilteredJoints, m_DoubleExponentialFilter.GetFilteredJoints(), sizeof( m_FilteredJoints ) );

    }


    //--------------------------------------------------------------------------------------
    // Name: FilterAdaptiveDoubleExponential::UpdateSmoothingParameters()
    // Desc: Updates the smoothing parameters of a joint based on he current joint velocity
    //--------------------------------------------------------------------------------------
    VOID FilterAdaptiveDoubleExponential::UpdateSmoothingParameters( UINT i, FLOAT fVelocity, NUI_SKELETON_POSITION_TRACKING_STATE eTrackingState )
    {
        assert( i < NUI_SKELETON_POSITION_COUNT );

        NUI_TRANSFORM_SMOOTH_PARAMETERS LerpedParams;

        static const FLOAT fBigJitterThreshold      = 5.0f;     // meters per second
        static const FLOAT fLowLatencyThreshold     = 1.5f;     // meters per second
        static const FLOAT fHighLatencyThreshold    = 0.25f;    // meters per second

        BOOL bDetectBigJitter = FALSE;

        // Parameters for hands should should be the same as wrists, otherwise they look unnatural
        switch( i )
        {
        case NUI_SKELETON_POSITION_HAND_LEFT:
            XMemCpy( &m_SmoothingParams[ NUI_SKELETON_POSITION_HAND_LEFT ], &m_SmoothingParams[ NUI_SKELETON_POSITION_WRIST_LEFT ], sizeof( NUI_TRANSFORM_SMOOTH_PARAMETERS ) );
            break;

        case NUI_SKELETON_POSITION_HAND_RIGHT:
            XMemCpy( &m_SmoothingParams[ NUI_SKELETON_POSITION_HAND_RIGHT ], &m_SmoothingParams[ NUI_SKELETON_POSITION_WRIST_RIGHT ], sizeof( NUI_TRANSFORM_SMOOTH_PARAMETERS ) );
            break;

        case NUI_SKELETON_POSITION_FOOT_LEFT:
            XMemCpy( &m_SmoothingParams[ NUI_SKELETON_POSITION_FOOT_LEFT ], &m_SmoothingParams[ NUI_SKELETON_POSITION_ANKLE_LEFT ], sizeof( NUI_TRANSFORM_SMOOTH_PARAMETERS ) );
            break;

        case NUI_SKELETON_POSITION_FOOT_RIGHT:
            XMemCpy( &m_SmoothingParams[ NUI_SKELETON_POSITION_FOOT_RIGHT ], &m_SmoothingParams[ NUI_SKELETON_POSITION_ANKLE_RIGHT ], sizeof( NUI_TRANSFORM_SMOOTH_PARAMETERS ) );
            break;

        default:
            {
                // Lerp between high and low latency params
                FLOAT fLerp = max( 0.0f, min( 1.0f, ( fVelocity - fHighLatencyThreshold ) / ( fLowLatencyThreshold - fHighLatencyThreshold ) ) );

                // Detected a big jitter
                if ( fVelocity > fBigJitterThreshold ||
                     fLerp - m_fPreviousLerp[ i ] > 0.999f )
                {
                    bDetectBigJitter = TRUE;
                    fLerp = ( fLerp * 0.25f ) + ( m_fPreviousLerp[ i ] * 0.75f );
                }

                m_fPreviousLerp[ i ] = fLerp;

                LerpedParams.fSmoothing = m_HighLatencySmoothingParams.fSmoothing + fLerp * ( m_LowLatencySmoothingParams.fSmoothing - m_HighLatencySmoothingParams.fSmoothing );
                LerpedParams.fCorrection = m_HighLatencySmoothingParams.fCorrection + fLerp * ( m_LowLatencySmoothingParams.fCorrection - m_HighLatencySmoothingParams.fCorrection );
                LerpedParams.fPrediction = m_HighLatencySmoothingParams.fPrediction + fLerp * ( m_LowLatencySmoothingParams.fPrediction - m_HighLatencySmoothingParams.fPrediction );
                LerpedParams.fJitterRadius = m_HighLatencySmoothingParams.fJitterRadius + fLerp * ( m_LowLatencySmoothingParams.fJitterRadius - m_HighLatencySmoothingParams.fJitterRadius );
                LerpedParams.fMaxDeviationRadius = m_HighLatencySmoothingParams.fMaxDeviationRadius + fLerp * ( m_LowLatencySmoothingParams.fMaxDeviationRadius - m_HighLatencySmoothingParams.fMaxDeviationRadius );

                XMemCpy( &m_SmoothingParams[ i ], &LerpedParams, sizeof( NUI_TRANSFORM_SMOOTH_PARAMETERS ) );               
            }
        }
        
        if ( bDetectBigJitter )
        {
            m_SmoothingParams[ i ].fJitterRadius *= 2.0f;
            m_SmoothingParams[ i ].fMaxDeviationRadius *= 2.0f;
            m_SmoothingParams[ i ].fPrediction /= 2.0f;
        }
        else if ( eTrackingState == NUI_SKELETON_POSITION_INFERRED )
        {
            // if the joint is not tracked, but inferred, we make sure it gets smoothed more by doubling the jitter radius
            m_SmoothingParams[ i ].fJitterRadius *= 2.0f;
            m_SmoothingParams[ i ].fMaxDeviationRadius *= 2.0f;
        }      
    }

}