//--------------------------------------------------------------------------------------
// AtgNuiJointConverter.cpp
//
// Demonstrates a method of constraining NUI joints to plausible human biometrics.  This 
// method attaches a set of cone constraints to the joints, and prohibits movement of
// the child bone of that joint outside of the cone.  If the bone does move outside the 
// cone, it is adjusted to lie on the closest point of the cone.
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <stdafx.h>
#include <xboxmath.h>
#include "AtgNuiJointConverter.h"
#include "AtgSimpleShaders.h"
#include "AtgDebugDraw.h"

namespace ATG
{

const XMVECTOR vBaseDir =           {  1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vHipCenterDir =      {  0.0f,   1.0f,  0.0f, 0.0f};
const XMVECTOR vSpineDir =          {  0.0f,  .985f,-.174f, 0.0f};
const XMVECTOR vShoulderCenterDir = {  1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vNeckDir =           {  0.0f,  .707f, .707f, 0.0f};
const XMVECTOR vCollarLeftDir =     { -1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vShoulderLeftDir =   { -1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vElbowLeftDir =      { -1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vWristLeftDir =      { -1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vCollarRightDir =    {  1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vShoulderRightDir =  {  1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vElbowRightDir =     {  1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vWristRightDir =     {  1.0f,   0.0f,  0.0f, 0.0f};
const XMVECTOR vPelvisLeftDir =     {-.707f, -.707f,  0.0f, 0.0f};
const XMVECTOR vHipLeftDir =        {  0.0f,  -1.0f,  0.0f, 0.0f};
const XMVECTOR vKneeLeftDir =       {  0.0f,  -1.0f,  0.0f, 0.0f};
const XMVECTOR vAnkleLeftDir =      {  0.0f, -.707f,-.707f, 0.0f};
const XMVECTOR vPelvisRightDir =    { .707f, -.707f,  0.0f, 0.0f};
const XMVECTOR vHipRightDir =       {  0.0f,  -1.0f,  0.0f, 0.0f};
const XMVECTOR vKneeRightDir =      {  0.0f,  -1.0f,  0.0f, 0.0f};
const XMVECTOR vAnkleRightDir =     {  0.0f, -.707f,-.707f, 0.0f};

IntermediateJointDesc g_IntJointDesc[ INTERMEDIATE_JOINT_COUNT ] =
{ 
    {  vBaseDir,           (NUI_SKELETON_POSITION_INDEX)(-1),        NUI_SKELETON_POSITION_HIP_CENTER, (INTERMEDIATE_JOINT_INDEX)(-1),      },
    {  vHipCenterDir,      NUI_SKELETON_POSITION_HIP_CENTER,         NUI_SKELETON_POSITION_SPINE,            INT_BASE,                      },
    {  vSpineDir,          NUI_SKELETON_POSITION_SPINE,              NUI_SKELETON_POSITION_SHOULDER_CENTER,  INT_HIP_CENTER,                },
    {  vShoulderCenterDir, NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_SHOULDER_CENTER,  INT_SPINE,                     },
    {  vNeckDir,           NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_HEAD,             INT_SPINE,                     },
    {  vCollarLeftDir,     NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_SHOULDER_LEFT,    INT_SPINE,                     },
    {  vShoulderLeftDir,   NUI_SKELETON_POSITION_SHOULDER_LEFT,      NUI_SKELETON_POSITION_ELBOW_LEFT,       INT_COLLAR_LEFT,               },
    {  vElbowLeftDir,      NUI_SKELETON_POSITION_ELBOW_LEFT,         NUI_SKELETON_POSITION_WRIST_LEFT,       INT_SHOULDER_LEFT,             },
    {  vWristLeftDir,      NUI_SKELETON_POSITION_WRIST_LEFT,         NUI_SKELETON_POSITION_HAND_LEFT,        INT_ELBOW_LEFT,                },
    {  vCollarRightDir,    NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_SHOULDER_RIGHT,   INT_SPINE,                     },
    {  vShoulderRightDir,  NUI_SKELETON_POSITION_SHOULDER_RIGHT,     NUI_SKELETON_POSITION_ELBOW_RIGHT,      INT_COLLAR_RIGHT,              },
    {  vElbowRightDir,     NUI_SKELETON_POSITION_ELBOW_RIGHT,        NUI_SKELETON_POSITION_WRIST_RIGHT,      INT_SHOULDER_RIGHT,            },
    {  vWristRightDir,     NUI_SKELETON_POSITION_WRIST_RIGHT,        NUI_SKELETON_POSITION_HAND_RIGHT,       INT_ELBOW_RIGHT,               },
    {  vPelvisLeftDir,     NUI_SKELETON_POSITION_HIP_CENTER,         NUI_SKELETON_POSITION_HIP_LEFT,         INT_BASE,                      },
    {  vHipLeftDir,        NUI_SKELETON_POSITION_HIP_LEFT,           NUI_SKELETON_POSITION_KNEE_LEFT,        INT_PELVIS_LEFT,               },
    {  vKneeLeftDir,       NUI_SKELETON_POSITION_KNEE_LEFT,          NUI_SKELETON_POSITION_ANKLE_LEFT,       INT_HIP_LEFT,                  },
    {  vAnkleLeftDir,      NUI_SKELETON_POSITION_ANKLE_LEFT,         NUI_SKELETON_POSITION_FOOT_LEFT,        INT_KNEE_LEFT,                 },
    {  vPelvisRightDir,    NUI_SKELETON_POSITION_HIP_CENTER,         NUI_SKELETON_POSITION_HIP_RIGHT,        INT_BASE,                      },
    {  vHipRightDir,       NUI_SKELETON_POSITION_HIP_RIGHT,          NUI_SKELETON_POSITION_KNEE_RIGHT,       INT_PELVIS_RIGHT,              },
    {  vKneeRightDir,      NUI_SKELETON_POSITION_KNEE_RIGHT,         NUI_SKELETON_POSITION_ANKLE_RIGHT,      INT_HIP_RIGHT,                 },
    {  vAnkleRightDir,     NUI_SKELETON_POSITION_ANKLE_RIGHT,        NUI_SKELETON_POSITION_FOOT_RIGHT,       INT_KNEE_RIGHT,                },
};                                                                                                                  
                                                        

// Mapping from intermediate skeleton to avatar skeleton. The NUI joints are first converted to an intermediate skeleton which holds joint rotations.
// Then the joints are remapped to an avatar skeleton, and converted to the avatar transformation space.  This struct contains the information needed
// to map from the intermediate skeleton to the avatar skeleton.
typedef struct IntermediateToAvatarMap_s
{
    INTERMEDIATE_JOINT_INDEX    m_intJoint;             // Intermediate skeleton joint
    AvatarJointRef              m_avatarJoint;          // Avatar joint corresponding to the intermediate joint
    AvatarJointRef              m_avatarParentJoint;    // Avatar joint parent
} IntermediateToAvatarMap;

const IntermediateToAvatarMap  g_IntermediateToAvatarMap[] =
{
    { INT_BASE,             BASE,  INVALID, },
    { INT_HIP_CENTER,       BACKA, BASE,    },
    { INT_SPINE,            BACKB, BACKA,   },
    { INT_NECK,             NECK,  BACKB,   },
      
    { INT_SHOULDER_RIGHT,   LF_S,  BACKB,   },
    { INT_ELBOW_RIGHT,      LF_E,  LF_S,    },
    { INT_WRIST_RIGHT,      LF_W,  LF_E,    },
      
    { INT_SHOULDER_LEFT,    RT_S,  BACKB,   },
    { INT_ELBOW_LEFT,       RT_E,  RT_S,    },
    { INT_WRIST_LEFT,       RT_W,  RT_E,    },
      
    { INT_HIP_RIGHT,        LF_H,  BASE,    },
    { INT_KNEE_RIGHT,       LF_K,  LF_H,    },
      
    { INT_HIP_LEFT,         RT_H,  BASE,    },
    { INT_KNEE_LEFT,        RT_K,  RT_H,    },
};


//--------------------------------------------------------------------------------------
// Name: NuiJointConverter()
// Desc: Constructor
//--------------------------------------------------------------------------------------
NuiJointConverter::NuiJointConverter( )
{
    Initialize();
}


//--------------------------------------------------------------------------------------
// Name: AddJointConstraint()
// Desc: Adds a joint constraint to the system.  
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::AddJointConstraint(INTERMEDIATE_JOINT_INDEX joint, XMVECTOR vDir, FLOAT fAngle)
{
    m_jointConstraints[m_dwNumJointConstraints].m_intJoint = joint;
    m_jointConstraints[m_dwNumJointConstraints].m_vDir = vDir;
    m_jointConstraints[m_dwNumJointConstraints].m_fAngle = fAngle;

    // Initialize the constraint value to 0.  This tracks the proximity of the joint to the constraint limit
    m_jointConstraints[m_dwNumJointConstraints].m_fConstraint = 0.0f;

    m_dwNumJointConstraints++;
}


//--------------------------------------------------------------------------------------
// Name: AddDefaultConstraints()
// Desc: Adds a set of default joint constraints.  This is a good set of constraints for
//       plausible human biomechanics.
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::AddDefaultConstraints( )
{
    AddJointConstraint( INT_SHOULDER_RIGHT, XMVectorSet(  0.7f,  0.0f,   .7f, 1.0f ),  80.0f );
    AddJointConstraint( INT_SHOULDER_LEFT,  XMVectorSet( -0.7f,  0.0f,   .7f, 1.0f ),  80.0f );

    AddJointConstraint( INT_ELBOW_RIGHT,    XMVectorSet(  0.0f,  0.0f,  1.0f, 1.0f ),  90.0f );
    AddJointConstraint( INT_ELBOW_LEFT,     XMVectorSet(  0.0f,  0.0f,  1.0f, 1.0f ),  90.0f );

    AddJointConstraint( INT_WRIST_RIGHT,    XMVectorSet(  1.0f,  0.0f,  0.0f, 1.0f ),  40.0f );
    AddJointConstraint( INT_WRIST_LEFT,     XMVectorSet( -1.0f,  0.0f,  0.0f, 1.0f ),  40.0f );

    AddJointConstraint( INT_HIP_RIGHT,      XMVectorSet(  0.0f, -1.0f,  1.0f, 1.0f ),  70.0f );
    AddJointConstraint( INT_HIP_LEFT,       XMVectorSet(  0.0f, -1.0f,  1.0f, 1.0f ),  70.0f );

    AddJointConstraint( INT_KNEE_RIGHT,     XMVectorSet(  0.0f, -0.5f, -1.0f, 1.0f ),  60.0f );
    AddJointConstraint( INT_KNEE_LEFT,      XMVectorSet(  0.0f, -0.5f, -1.0f, 1.0f ),  60.0f );

    AddJointConstraint( INT_ANKLE_RIGHT,    XMVectorSet(  0.0f,  0.0f,  1.0f, 1.0f ),  60.0f );
    AddJointConstraint( INT_ANKLE_LEFT,     XMVectorSet(  0.0f,  0.0f,  1.0f, 1.0f ),  60.0f );

    AddJointConstraint( INT_SPINE,          XMVectorSet(  0.0f,  1.0f,  0.0f, 1.0f ),  50.0f );
    AddJointConstraint( INT_HIP_CENTER,     XMVectorSet(  0.0f,  1.0f,  0.3f, 1.0f ),  90.0f );

    AddJointConstraint( INT_NECK,           XMVectorSet(  0.0f,  1.0f,  0.3f, 1.0f ),  40.0f );
}


//--------------------------------------------------------------------------------------
// Name: Initalize()
// Desc: Initialize the skeleton and set to bind pose
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::Initialize( )
{
    m_dwNumJointConstraints = 0;

    // Set the scale factor to mirror Z by default.  This puts the skeleton facing the
    // viewer as if the viewer were looking at a mirror.
    m_vJointScaleFactor = XMVectorSet( 1, 1, -1, 0);  
    
    ZeroMemory( &m_NuiSkeleton, sizeof(NUI_SKELETON_DATA) );

    for( DWORD i = 0; i < ARRAYSIZE( m_qBindPose ); ++i )
    {
        m_qBindPose[i] = XMQuaternionIdentity();
    }

    for( DWORD i = 0; i < ARRAYSIZE(m_IntJoints); ++i )
    {
        m_IntJoints[i].doubleExponentialFilter.m_qFilteredLocal = XMQuaternionIdentity();
        m_IntJoints[i].doubleExponentialFilter.m_qTrend = XMQuaternionIdentity();
        m_IntJoints[i].doubleExponentialFilter.m_uStarted = 0;
        m_PreviousIntJoints[i].doubleExponentialFilter.m_qFilteredLocal = XMQuaternionIdentity();
        m_PreviousIntJoints[i].doubleExponentialFilter.m_qTrend = XMQuaternionIdentity();
        m_PreviousIntJoints[i].doubleExponentialFilter.m_uStarted = 0;
    }
}


//--------------------------------------------------------------------------------------
// Name: ConvertCameraToAvatarSkeleton()
// Desc: Entry point for skeleton conversion.  Updates the skeleton to the current frame
//       and converts to Avatar format
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::ConvertNuiToAvatarSkeleton( const NUI_SKELETON_DATA* pNuiSkeleton, XAVATAR_SKELETON_POSE_JOINT* pAvatarJoints )
{
    ConvertNuiJoints( pNuiSkeleton, FALSE );

    if( pAvatarJoints != NULL )
    {
        // Initialize rotations to zero
        XMVECTOR VIdentity = XMQuaternionIdentity();		// 0,0,0,1
        XMVECTOR VOne = XMVectorSplatOne();					// 1,1,1,1
        for ( INT j = 0; j < XAVATAR_MAX_SKELETON_JOINTS; ++j )
        {
            pAvatarJoints[ j ].Position = VIdentity;
            pAvatarJoints[ j ].Rotation = VIdentity;
            pAvatarJoints[ j ].Scale = VOne;
        }
        GetAvatarRotations( pAvatarJoints );
    }
}


//--------------------------------------------------------------------------------------
// Name: ConvertNuiJoints()
// Desc: Entry point for skeleton conversion, if avatar joint creation is not desired.
//       Performs optional joint orientation smoothing before applying constraints.
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::ConvertNuiJoints( const NUI_SKELETON_DATA* pNuiSkeleton, BOOL bFilterJointOrientations )
{
    SetSkeleton( pNuiSkeleton );
    Update( bFilterJointOrientations );
}


//--------------------------------------------------------------------------------------
// Name: SetSkeleton()
// Desc: Set the local copy of the skeleton data
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::SetSkeleton( const NUI_SKELETON_DATA* pSkeleton )
{
    XMemCpy( &m_NuiSkeleton, pSkeleton, sizeof(NUI_SKELETON_DATA ) );
}


//--------------------------------------------------------------------------------------
// Name: QuaternionAngle
// Desc: Returns the amount of rotation in the given quaternion, in radians.
//--------------------------------------------------------------------------------------
inline FLOAT QuaternionAngle( XMVECTOR qRotation )
{
    XMVECTOR vAxis;
    FLOAT fAngle;
    XMQuaternionToAxisAngle( &vAxis, &fAngle, XMQuaternionNormalize( qRotation ) );
    return fAngle;
}


//--------------------------------------------------------------------------------------
// Name: EnsureQuaternionNeighborhood
// Desc: Ensures that quaternions qA and qB are in the same 3D sphere in 4D space.
//--------------------------------------------------------------------------------------
inline VOID EnsureQuaternionNeighborhood( const XMVECTOR& qA, XMVECTOR& qB )
{
    if( XMVectorGetX( XMQuaternionDot( qA, qB ) ) < 0 )
    {
        // Negate the second quaternion, to place it in the opposite 3D sphere.
        qB = -qB;
    }
}


//--------------------------------------------------------------------------------------
// Name: RotationBetweenQuaternions
// Desc: Returns a quaternion that represents a rotation qR such that qA * qR = qB.
//--------------------------------------------------------------------------------------
inline XMVECTOR RotationBetweenQuaternions( XMVECTOR qA, XMVECTOR qB )
{
    EnsureQuaternionNeighborhood( qA, qB );
    return XMQuaternionMultiply( XMQuaternionInverse( qA ), qB );
}


//--------------------------------------------------------------------------------------
// Name: EnhancedQuaternionSlerp
// Desc: Performs a quaternion slerp, after placing both input quaternions in the same
//       3D sphere.
//--------------------------------------------------------------------------------------
inline XMVECTOR EnhancedQuaternionSlerp( XMVECTOR qA, XMVECTOR qB, FLOAT T )
{
    EnsureQuaternionNeighborhood( qA, qB );
    return XMQuaternionSlerp( qA, qB, T );
}


//--------------------------------------------------------------------------------------
// Name: DoubleExponentialJointOrientationFilter
// Desc: Implements a double exponential smoothing filter on the skeleton joint orientation
//       quaternions.
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::DoubleExponentialJointOrientationFilter()
{
    static FLOAT fSmoothing      = 0.75f;        // [0..1], lower values is closer to the raw data and more noisy
    static FLOAT fCorrection     = 0.75f;        // [0..1], higher values correct faster and feel more responsive
    static FLOAT fPrediction     = 0.75f;       // [0..n], how many frames into the future we want to predict
    static FLOAT fJitterRadius   = 0.10f;       // The deviation angle in radians that defines jitter
    static FLOAT fMaxDeviationRadius = 0.10f;   // The maximum angle in radians that filtered positions are allowed to deviate from raw data

    XMVECTOR qPrevRawOrientation;
    XMVECTOR qPrevFilteredOrientation;
    XMVECTOR qPrevTrend;
    XMVECTOR qRawOrientation;
    XMVECTOR qFilteredOrientation;
    XMVECTOR qPredictedOrientation;
    XMVECTOR qDiff;
    XMVECTOR qTrend;
    FLOAT fDiff;
    BOOL bOrientationIsValid;

    for (INT i = 0; i < ARRAYSIZE(m_IntJoints); i++)
    {
        qRawOrientation             = m_IntJoints[i].quatLocal;
        qPrevFilteredOrientation    = m_PreviousIntJoints[i].doubleExponentialFilter.m_qFilteredLocal;
        qPrevTrend                  = m_PreviousIntJoints[i].doubleExponentialFilter.m_qTrend;
        bOrientationIsValid         = TRUE;

        if (!bOrientationIsValid)
        {
            m_IntJoints[i].doubleExponentialFilter.m_uStarted = 0;
        }

        // Initial start values or reset values
        if (m_IntJoints[i].doubleExponentialFilter.m_uStarted == 0)
        {
            // Use raw position and zero trend for first value
            qFilteredOrientation = qRawOrientation;
            qTrend = XMQuaternionIdentity();
            m_IntJoints[i].doubleExponentialFilter.m_uStarted++;
        }
        else if (m_IntJoints[i].doubleExponentialFilter.m_uStarted == 1)
        {
            // Use average of two positions and calculate proper trend for end value
            qPrevRawOrientation = m_PreviousIntJoints[i].quatLocal;
            qFilteredOrientation = EnhancedQuaternionSlerp( qPrevRawOrientation, qRawOrientation, 0.5f );
            qDiff = RotationBetweenQuaternions( qFilteredOrientation, qPrevFilteredOrientation );
            qTrend = EnhancedQuaternionSlerp( qPrevTrend, qDiff, fCorrection );
            m_IntJoints[i].doubleExponentialFilter.m_uStarted++;
        }
        else
        {
            // First apply a jitter filter
            qDiff = RotationBetweenQuaternions( qRawOrientation, qPrevFilteredOrientation );
            fDiff = fabs( QuaternionAngle( qDiff ) );

            if (fDiff <= fJitterRadius)
            {
                qFilteredOrientation = EnhancedQuaternionSlerp( qPrevFilteredOrientation, qRawOrientation, fDiff / fJitterRadius );
            }
            else
            {
                qFilteredOrientation = qRawOrientation;
            }

            // Now the double exponential smoothing filter
            qFilteredOrientation = EnhancedQuaternionSlerp( qFilteredOrientation, XMQuaternionMultiply( qPrevFilteredOrientation, qPrevTrend ), fSmoothing );

            qDiff = RotationBetweenQuaternions( qFilteredOrientation, qPrevFilteredOrientation );
            qTrend = EnhancedQuaternionSlerp( qPrevTrend, qDiff, fCorrection );
        }      

        // Use the trend and predict into the future to reduce latency
        qPredictedOrientation = XMQuaternionMultiply( qFilteredOrientation, EnhancedQuaternionSlerp( XMQuaternionIdentity(), qTrend, fPrediction ) );

        // Check that we are not too far away from raw data
        qDiff = RotationBetweenQuaternions( qPredictedOrientation, qFilteredOrientation );
        fDiff = fabs( QuaternionAngle( qDiff ) );

        if (fDiff > fMaxDeviationRadius)
        {
            qPredictedOrientation = EnhancedQuaternionSlerp( qFilteredOrientation, qPredictedOrientation, fMaxDeviationRadius / fDiff );
        }

        qPredictedOrientation = XMQuaternionNormalize( qPredictedOrientation );
        qFilteredOrientation = XMQuaternionNormalize( qFilteredOrientation );

        // Store current values
        m_IntJoints[i].doubleExponentialFilter.m_qFilteredLocal = qFilteredOrientation;
        m_IntJoints[i].doubleExponentialFilter.m_qTrend = qTrend;

        // Apply predicted orientation to joint
        m_IntJoints[i].quatLocal = qPredictedOrientation;
        XMVECTOR qParentRot = XMQuaternionIdentity();
        if( g_IntJointDesc[i].parentIdx != -1 )
        {
            qParentRot = m_IntJoints[g_IntJointDesc[i].parentIdx].quatWorld;
        }
        m_IntJoints[i].quatWorld = XMQuaternionMultiply( qParentRot, m_IntJoints[i].quatLocal );
    }
}


//--------------------------------------------------------------------------------------
// Name: Update()
// Desc: Update the skeleton with the latest skeleton data.  
//
//       1. Apply filter(s)
//       2. Convert the positions to our coordinate system
//       3. Calculate joint rotations from the positional data
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::Update( BOOL bEnableJointOrientationFilter )
{
    // Perform collision detection
    CollideSkeleton( &m_NuiSkeleton );

    // Convert coordinates to our system
    ScaleJoints();

    // Calculate joint rotations from joint positions
    CalculateJointRotations();

    // Apply orientation smoothing filter
    if( bEnableJointOrientationFilter )
    {
        DoubleExponentialJointOrientationFilter();
    }

    // Calculate the constraints
    ApplyJointConstraints();

    // Save joint orientations for use next frame in the filter
    if( bEnableJointOrientationFilter )
    {
        XMemCpy( &m_PreviousIntJoints, &m_IntJoints, sizeof( m_IntJoints ) );
    }
}


//--------------------------------------------------------------------------------------
// Name: DistanceToLineSegment()
// Desc: find the distance from a point to a line.  Return the normal for offset use.
//
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::DistanceToLineSegment(XMVECTOR x0, XMVECTOR x1, XMVECTOR p, float* distance, XMVECTOR* normal) const
{
    // find the vector from x0 to x1
    XMVECTOR vLine = x1 - x0;
    XMVECTOR vLineLength = XMVector3Length( vLine );
    FLOAT fLineLength = XMVectorGetX( vLineLength );
    XMVECTOR vLineToPoint = p - x0;

    const FLOAT epsilon = 0.0001f;
    // if the line is too short skip
    if( fLineLength > epsilon )
    {
        FLOAT t = XMVectorGetX( XMVector3Dot(vLine, vLineToPoint) ) / fLineLength;
        
        // projection is longer than the line itself so find distance to end point of line
        if( t > fLineLength )
        {
            vLineToPoint = p - x1;
        }
        // find distance to line
        else if(t >= 0.0f) 
        {
            XMVECTOR vNormalPoint = vLine;
			
            // Perform the float->vector conversion once by combining t/fLineLength
            vNormalPoint *= (t/fLineLength);
            vNormalPoint += x0;
            vLineToPoint = p - vNormalPoint;
        }
    }
    
    // The distance is the size ofthe final computed line
    XMVECTOR vVecLength = XMVector3Length( vLineToPoint );
    *distance = XMVectorGetX(vVecLength);
    
    // The normal is the final line normalized
    *normal = vLineToPoint / (*distance);
}


//--------------------------------------------------------------------------------------
// Name: CollideSkeleton()
// Desc: Keep the skeleton's hands and wrists from puncturing its body.
//
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::CollideSkeleton( NUI_SKELETON_DATA* pSkeleton )
{
    
    const FLOAT ShoulderExtend = 0.5f;
    const FLOAT HipExtend = 6.0f;
    const FLOAT CollisionTolerance = 1.01f;

    if (pSkeleton->eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_SHOULDER_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED
        && pSkeleton->eSkeletonPositionTrackingState[NUI_SKELETON_POSITION_HIP_CENTER] != NUI_SKELETON_POSITION_NOT_TRACKED )
    {
        XMVECTOR vLShoulderDiff =  pSkeleton->SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT] - 
            pSkeleton->SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER];
        XMVECTOR vLShoulderLength = XMVector3Length( vLShoulderDiff );
        XMVECTOR vRShoulderDiff =  pSkeleton->SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT] - 
            pSkeleton->SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER];
        XMVECTOR vRShoulderLength = XMVector3Length( vRShoulderDiff );
        // The distance beween shoulders is averaged for the radius
        float cylinderRadius = ( XMVectorGetX(vLShoulderLength) + XMVectorGetX(vRShoulderLength) ) * 0.5f;
        
        // Calculate the shoulder center and the hip center.  Extend them up and down respectively.
        XMVECTOR vShoulderCenter = pSkeleton->SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER];
        XMVECTOR vHipCenter = pSkeleton->SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER];
        XMVECTOR vHipShoulder = vHipCenter - vShoulderCenter;
        XMVECTOR vHipShoulderNormalized = XMVector3Normalize( vHipShoulder );
        vShoulderCenter = vShoulderCenter - ( vHipShoulderNormalized * ( ShoulderExtend * cylinderRadius ) );
        vHipCenter = vHipCenter + ( vHipShoulderNormalized * ( HipExtend * cylinderRadius ) );
        
        // Increse radius to account for bulky avatars
        cylinderRadius *= 1.5f;
       
        // joints to collide
        static const INT collisionIndices[] = { NUI_SKELETON_POSITION_WRIST_LEFT, NUI_SKELETON_POSITION_HAND_LEFT, 
            NUI_SKELETON_POSITION_WRIST_RIGHT, NUI_SKELETON_POSITION_HAND_RIGHT };
        
        for(INT i=0; i<ARRAYSIZE(collisionIndices); i++)
        {
            XMVECTOR vCollisionJoint = pSkeleton->SkeletonPositions[collisionIndices[i]];
            float distance;
            XMVECTOR normal;
            DistanceToLineSegment( vShoulderCenter, vHipCenter, vCollisionJoint, &distance, &normal);
            // if distance is within the cylinder then push the joint out and away from the cylinder
            if( distance < cylinderRadius )
            {
                XMVECTOR vMoveJointAmount = normal;
                vMoveJointAmount *= ((cylinderRadius - distance) * CollisionTolerance);
                vCollisionJoint += normal * ((cylinderRadius - distance) * CollisionTolerance);
                pSkeleton->SkeletonPositions[collisionIndices[i]] = vCollisionJoint;
            }
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: ScaleJoints()
// Desc: Scale the joints.  A negative scale factor can be used to flip the coordinate system or mirror joints.
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::ScaleJoints( )
{
    for( INT nJoint = 0; nJoint < NUI_SKELETON_POSITION_COUNT; ++nJoint )
    {
        m_NuiSkeleton.SkeletonPositions[nJoint] = XMVectorMultiply( m_NuiSkeleton.SkeletonPositions[ nJoint ], m_vJointScaleFactor );
    }
}


//--------------------------------------------------------------------------------------
// Name: CalculateJointRotations()
// Desc: Calculate joint rotations from joint positions. This function assumes that the 
//       bind pose is in a particular configuration (each joint in the bind pose aligns
//       with an axis).  It would be more correct to sync the bind pose with the user.
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::CalculateJointRotations()
{
    // Initialize quaternions to identity
    for( INT i = 0; i < INTERMEDIATE_JOINT_COUNT; ++i)
    {
        m_IntJoints[i].quatWorld = XMQuaternionIdentity();
    }

    // Calculate hip direction and assign it to the base joint
    XMVECTOR hipDir = XMVectorSubtract(  m_NuiSkeleton.SkeletonPositions[ NUI_SKELETON_POSITION_HIP_RIGHT ], 
                                         m_NuiSkeleton.SkeletonPositions[ NUI_SKELETON_POSITION_HIP_LEFT ] ); 
    hipDir.y = 0;
    hipDir = XMVector3Normalize(hipDir);
    XMVECTOR hipRot = GetShortestRotationBetweenVecs( hipDir, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) );

    m_IntJoints[ INT_BASE ].quatLocal = hipRot;
    m_IntJoints[ INT_BASE ].quatWorld = hipRot;

    // Get the rotations from the orientation of the bone
    for( int i = 0; i < ARRAYSIZE( m_IntJoints ); ++i )
    {
        if( g_IntJointDesc[i].startNuiJoint == -1 )
        {
            continue;
        }

        XMVECTOR qParentRot;
        if( g_IntJointDesc[i].parentIdx == -1 )
        {
            qParentRot = XMQuaternionIdentity();
        }
        else
        {
            qParentRot = m_IntJoints[g_IntJointDesc[i].parentIdx].quatWorld;
        }

        XMMATRIX matParentRot = XMMatrixRotationQuaternion( qParentRot );

        if( m_NuiSkeleton.eSkeletonPositionTrackingState[ g_IntJointDesc[i].endNuiJoint] == NUI_SKELETON_POSITION_NOT_TRACKED ||
            m_NuiSkeleton.eSkeletonPositionTrackingState[ g_IntJointDesc[i].startNuiJoint] == NUI_SKELETON_POSITION_NOT_TRACKED )
        {
            m_IntJoints[ i ].eTrackingState = NUI_SKELETON_POSITION_NOT_TRACKED;
        }
        else if( m_NuiSkeleton.eSkeletonPositionTrackingState[ g_IntJointDesc[i].endNuiJoint] == NUI_SKELETON_POSITION_INFERRED ||
                 m_NuiSkeleton.eSkeletonPositionTrackingState[ g_IntJointDesc[i].startNuiJoint] == NUI_SKELETON_POSITION_INFERRED )
        {
            m_IntJoints[ i ].eTrackingState = NUI_SKELETON_POSITION_INFERRED;
        }
        else
        {
            m_IntJoints[ i ].eTrackingState = NUI_SKELETON_POSITION_TRACKED;
        }

        if( m_IntJoints[ i ].eTrackingState == NUI_SKELETON_POSITION_NOT_TRACKED )
        {
            m_IntJoints[ i ].quatLocal = XMQuaternionIdentity();
        }
        else
        {
            // Calculate the local rotation based on the vector from the joint to its child.
            XMVECTOR vDir = XMVectorSubtract( m_NuiSkeleton.SkeletonPositions[ g_IntJointDesc[i].endNuiJoint ],
                                              m_NuiSkeleton.SkeletonPositions[ g_IntJointDesc[i].startNuiJoint ] );
            vDir = XMVector3Normalize( vDir );

            // Transform the direction of the NUI bone into local space so that it can be compared to the local bind pose direction
            vDir = XMVector3Transform( vDir, XMMatrixRotationQuaternion(XMQuaternionInverse( XMQuaternionMultiply( m_qBindPose[i], qParentRot ) ) ) );

            // Find the rotation between the skeleton's direction and the desired bone direction.  This is the joint's local rotation.
            XMVECTOR qRot = GetShortestRotationBetweenVecs( vDir,  g_IntJointDesc[i].vBoneDir );
            m_IntJoints[ i ].quatLocal = XMQuaternionNormalize( qRot );

            // Rebuild the world transform.  World = Parent * BindPose * Local
            m_IntJoints[i].quatWorld = XMQuaternionMultiply(m_IntJoints[i].quatLocal, XMQuaternionMultiply( m_qBindPose[i], qParentRot ) );
        }

        // Convert local rotation to world space
        m_IntJoints[i].quatWorld = XMQuaternionMultiply(m_IntJoints[i].quatLocal, XMQuaternionMultiply( m_qBindPose[i], qParentRot ) );
    }
}


//--------------------------------------------------------------------------------------
// Name: ApplyJointConstraints()
// Desc: Determine joint constraints and constrain rotations.
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::ApplyJointConstraints()
{
    // Calculate constraint values.  0.0-1.0 means the bone is within the constraint cone.  Greater than 1.0 means 
    // it lies outside the constraint cone.
    for( DWORD i = 0; i < m_dwNumJointConstraints; i++)
    {
        // Get the constraint direction in world space
        XMVECTOR quatConstraintLocalToWorld;
        quatConstraintLocalToWorld = m_IntJoints[g_IntJointDesc[m_jointConstraints[i].m_intJoint].parentIdx].quatWorld;
        XMMATRIX matConstraintLocalToWorld = XMMatrixRotationQuaternion( quatConstraintLocalToWorld );

        XMVECTOR constraintDirWS;
        constraintDirWS = XMVector3Normalize( m_jointConstraints[i].m_vDir );
        constraintDirWS = XMVector4Transform( constraintDirWS, matConstraintLocalToWorld );
        
        // Get the bone direction in world space
        XMVECTOR boneJoints[2];
        boneJoints[0] = GetNuiSkeletonJointPos( g_IntJointDesc[m_jointConstraints[i].m_intJoint].startNuiJoint );
        boneJoints[1] = GetNuiSkeletonJointPos( g_IntJointDesc[m_jointConstraints[i].m_intJoint].endNuiJoint );

        XMVECTOR boneDirWS = boneJoints[1] - boneJoints[0];
        boneDirWS = XMVector3Normalize( boneDirWS );

        // Calculate the constraint value.  0.0 is in the center of the constraint cone, 1.0 and above are outside the cone.
        FLOAT cosConstraintAngle = cosf( XMConvertToRadians( m_jointConstraints[i].m_fAngle ) );
        FLOAT boneDot = XMVector3Dot(constraintDirWS, boneDirWS).x;

        m_jointConstraints[i].m_fConstraint = (1.0f - boneDot)/(1.0f - cosConstraintAngle);
    }

    // Calculate bone lengths and local space bone vectors
    for(int i = 0; i < ARRAYSIZE(m_IntJoints); i++)
    {
        // Get a vector from the start joint to the end joint.
        XMVECTOR vJointPos[2];
        vJointPos[0] = GetNuiSkeletonJointPos( g_IntJointDesc[i].startNuiJoint );
        vJointPos[1] = GetNuiSkeletonJointPos( g_IntJointDesc[i].endNuiJoint );
        XMVECTOR vBoneVectorWS = vJointPos[1] - vJointPos[0];

        // Calculate the length of the bone vector
        m_IntJoints[i].fBoneLength = XMVector3Length( vBoneVectorWS ).x;
        
        // Transform the bone vector to local space
        XMVECTOR quatLocalToWorld = m_IntJoints[g_IntJointDesc[i].parentIdx].quatWorld;
        XMMATRIX matWorldToLocal;
        matWorldToLocal = XMMatrixRotationQuaternion( XMQuaternionInverse( quatLocalToWorld ) );

        m_IntJoints[i].vBoneVectorLS = XMVector4Transform( vBoneVectorWS, matWorldToLocal );
    }

    for( UINT nJointConstraintIdx = 0; nJointConstraintIdx < m_dwNumJointConstraints; nJointConstraintIdx++ )
    {
        // If the joint has a parent, constrain the bone direction to be within the constraint cone
        INT nIntJointIdx = m_jointConstraints[nJointConstraintIdx].m_intJoint;
        NUI_SKELETON_POSITION_INDEX parentIdx = g_IntJointDesc[ nIntJointIdx ].startNuiJoint;
        if( parentIdx != (NUI_SKELETON_POSITION_INDEX)(-1) )
        {
            XMVECTOR vParentPos = XMVectorZero();
            vParentPos = GetNuiSkeletonJointPos( parentIdx );
        
            XMMATRIX matTransform;
            XMVECTOR quatParentLStoWS;
            quatParentLStoWS = m_IntJoints[ g_IntJointDesc[nIntJointIdx].parentIdx ].quatWorld;

            // If the bone lies outside the constraint
            if( nJointConstraintIdx != -1 && 
                m_jointConstraints[nJointConstraintIdx].m_fConstraint > 1.0f )
            {
                // Transform bone vector to world space
                XMVECTOR quatBoneLStoWS;
                quatBoneLStoWS = m_IntJoints[g_IntJointDesc[nIntJointIdx].parentIdx].quatWorld;
                XMMATRIX matBoneLStoWS = XMMatrixRotationQuaternion( quatBoneLStoWS );
                XMVECTOR vBoneVecWS = XMVector4Transform( m_IntJoints[nIntJointIdx].vBoneVectorLS, matBoneLStoWS );
                if( m_IntJoints[nIntJointIdx].eTrackingState == NUI_SKELETON_POSITION_NOT_TRACKED ||
                    XMVector3LengthSq( vBoneVecWS ).x  < .001f )
                {
                    m_IntJoints[ nIntJointIdx ].quatLocal = XMQuaternionIdentity();
                }
                else
                {
                    // Transform constraint axis to world space
                    XMVECTOR quatConstraintLStoWS;
                    quatConstraintLStoWS = m_IntJoints[g_IntJointDesc[nIntJointIdx].parentIdx].quatWorld;
                    XMMATRIX matConstraintLStoWS = XMMatrixRotationQuaternion( quatConstraintLStoWS );

                    XMVECTOR vConstraintAxisWS = XMVector4Transform( m_jointConstraints[ nJointConstraintIdx ].m_vDir, matConstraintLStoWS );

                    // Calculate the axis of rotation between the constraint axis and the bone direction
                    XMVECTOR vRotAxis = XMVector3Cross( vConstraintAxisWS, vBoneVecWS  );
                    vRotAxis = XMVector3Normalize(vRotAxis);
                    
                    // Rotate the constraint axis to the edge of the cone in the direction of the original bone.  
                    XMMATRIX matConstrainedRot = XMMatrixRotationAxis( vRotAxis, XMConvertToRadians( m_jointConstraints[nJointConstraintIdx].m_fAngle ) );
                    XMVECTOR vConstrainedVecWS = XMVector4Transform( vConstraintAxisWS, matConstrainedRot );
                    vConstrainedVecWS = XMVector3Normalize( vConstrainedVecWS );

                    // Create the joint local space and world space rotations from the constrained direction

                    // Transform the bind pose direction into the parent's world space
                    XMMATRIX matParentRot;
                    if( g_IntJointDesc[nIntJointIdx].parentIdx == -1 )
                    {
                        matParentRot = XMMatrixIdentity();
                    }
                    else
                    {
                        matParentRot = XMMatrixRotationQuaternion( m_IntJoints[g_IntJointDesc[nIntJointIdx].parentIdx].quatWorld );
                    }

                    XMVECTOR vBoneDir = XMVector3Transform( g_IntJointDesc[ nIntJointIdx ].vBoneDir, matParentRot );

                    m_IntJoints[ nIntJointIdx ].quatLocal = GetShortestRotationBetweenVecs( vConstrainedVecWS, vBoneDir );
                    m_IntJoints[ nIntJointIdx ].quatLocal = XMQuaternionNormalize( m_IntJoints[ nIntJointIdx ].quatLocal );
                }
            }
        }
    }

    m_vReconstructedPos[ NUI_SKELETON_POSITION_HIP_CENTER ] = GetNuiSkeletonJointPos( NUI_SKELETON_POSITION_HIP_CENTER );

    // Recalculate world space rotations
    for(int i = 0; i < ARRAYSIZE(g_IntJointDesc); i++)
    {
        if( g_IntJointDesc[i].parentIdx < 0 )
        {
            m_IntJoints[i].quatWorld = m_IntJoints[i].quatLocal;
        }
        else
        {
            // Calculate joint positions based on the constrained skeleton
            XMMATRIX matTransform = XMMatrixRotationQuaternion( m_IntJoints[ i ].quatWorld );

            XMVECTOR vOffset = XMVector3Normalize(g_IntJointDesc[i].vBoneDir) * m_IntJoints[i].fBoneLength;
            vOffset = XMVector4Transform( vOffset, matTransform);

            assert( g_IntJointDesc[i].endNuiJoint >= 0  && g_IntJointDesc[i].startNuiJoint >= 0 );
            m_vReconstructedPos[ g_IntJointDesc[i].endNuiJoint] = m_vReconstructedPos[ g_IntJointDesc[i].startNuiJoint ] + vOffset;
        }
    }
}


//--------------------------------------------------------------------------------------
// Name: DrawSkeleton()
// Desc: Debug rendering of skeleton
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::DrawSkeleton( D3DDevice* pd3dDevice, XMMATRIX matWVP ) const
{
    static FLOAT PointSize = 7.0f;
    pd3dDevice->SetRenderState( D3DRS_POINTSIZE, *((DWORD*)&PointSize));
    pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);

    static XMVECTOR verts[2];

    ATG::SimpleShaders::SetDeclPos();

    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE);
    pd3dDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE);
    pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE);

    PIXBeginNamedEvent( 0, "Joint Constraint Skeleton");
    
    for( DWORD i = 0; i < m_dwNumJointConstraints; i++)
    {
        XMVECTOR vert;
        XMVECTOR quat;
        vert = GetNuiSkeletonJointPos( g_IntJointDesc[m_jointConstraints[i].m_intJoint].startNuiJoint );
        quat = m_IntJoints[g_IntJointDesc[m_jointConstraints[i].m_intJoint].parentIdx].quatWorld;

        XMMATRIX constraintMat = XMMatrixRotationQuaternion(quat);
        XMVECTOR dir;
        dir = XMVector3Normalize( m_jointConstraints[i].m_vDir );
        dir = XMVector4Transform( dir, constraintMat );

        // Draw the cone of the constraint
        static FLOAT coneLength = .08f;
        FLOAT coneHeight = coneLength * cosf( XMConvertToRadians( m_jointConstraints[i].m_fAngle) );
        FLOAT coneRadius = coneLength * sinf( XMConvertToRadians( m_jointConstraints[i].m_fAngle) );
        
        dir = dir * coneHeight;

        XMFLOAT3 coneDir;
        XMFLOAT3 centerBase;
        XMStoreVector3( &coneDir, dir );
        XMStoreVector3( &centerBase, vert);

        ATG::DebugDraw::SetViewProjection( matWVP );

        static FLOAT LineWidth = 1.0f;
        pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, *((DWORD*)&LineWidth));

        ATG::DebugDraw::DrawConeWireframe( centerBase, coneDir, 0.0f, coneRadius, D3DCOLOR_ARGB(255,255,255,255));
    }

    // Draw bones
    static float fLineWidth = 1.0f;
    pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, *((DWORD*)&fLineWidth));
    for( DWORD j = 0; j < ARRAYSIZE( g_IntJointDesc ); j++)
    {
        if( g_IntJointDesc[j].startNuiJoint == -1 )
            continue;

        verts[0] = GetNuiSkeletonJointPos( g_IntJointDesc[j].startNuiJoint );
        verts[1] = GetNuiSkeletonJointPos( g_IntJointDesc[j].endNuiJoint );
        XMFLOAT3 verts2[2];
        XMStoreVector3(&verts2[0],verts[0]);
        XMStoreVector3(&verts2[1],verts[1]);

        static FLOAT nearZ = -2.0f;
        static FLOAT farZ= -1.7f;
        FLOAT depth = ( verts[0].z - nearZ ) / (farZ - nearZ);
        if( depth > 1.0f) depth = 1.0f;
        if( depth < 0.0f) depth = 0.0f;

        D3DCOLOR boneColor = D3DCOLOR_ARGB(255,(INT)(depth * 87),(INT)(depth * 154),168);

        // Draw bones as red if they are outside the constraint cone.
        for( DWORD i = 0; i < m_dwNumJointConstraints; i++)
        {
            if( m_jointConstraints[i].m_intJoint == (INTERMEDIATE_JOINT_INDEX)j &&
                m_jointConstraints[i].m_fConstraint > 1.0f )
            {
                boneColor = D3DCOLOR_ARGB(255,255,0,0);
            }
        }

        ATG::SimpleShaders::BeginShader_Transformed_ConstantColor( matWVP, boneColor );

        pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, ( const VOID* )verts2, sizeof( XMFLOAT3 ) );
        ATG::SimpleShaders::EndShader();

        // Draw reconstructed bones
        ATG::SimpleShaders::BeginShader_Transformed_ConstantColor( matWVP, D3DCOLOR_ARGB(255,255,255,255));
        XMStoreVector3(&verts2[0],m_vReconstructedPos[g_IntJointDesc[j].startNuiJoint]);
        XMStoreVector3(&verts2[1],m_vReconstructedPos[g_IntJointDesc[j].endNuiJoint]);

        pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, ( const VOID* )verts2, sizeof( XMFLOAT3 ) );

        ATG::SimpleShaders::EndShader();
    }

    // Draw joints and X, Y, and Z axes
    for(DWORD j = 0; j < INTERMEDIATE_JOINT_COUNT; ++j)
    {
        XMVECTOR quat;
        quat = m_IntJoints[j].quatWorld;
        verts[0] = GetNuiSkeletonJointPos( g_IntJointDesc[j].endNuiJoint );
    
        ATG::SimpleShaders::BeginShader_Transformed_ConstantColor( matWVP, D3DCOLOR_ARGB(255,139,29,22));
        pd3dDevice->DrawPrimitiveUP( D3DPT_POINTLIST, 1, verts, sizeof(XMVECTOR) );
        ATG::SimpleShaders::EndShader();

        XMVECTOR vXAxis, vYAxis, vZAxis;
        static FLOAT fAxisSize = .08f;
        vXAxis = XMVectorSet( fAxisSize, 0.0f, 0.0f, 1.0f );
        vYAxis = XMVectorSet( 0.0f, fAxisSize, 0.0f, 1.0f );
        vZAxis = XMVectorSet( 0.0f, 0.0f, fAxisSize, 1.0f );

        XMMATRIX rotation = XMMatrixRotationQuaternion( quat );
        vXAxis = XMVector4Transform(vXAxis, rotation);
        vYAxis = XMVector4Transform(vYAxis, rotation);
        vZAxis = XMVector4Transform(vZAxis, rotation);
    
        fLineWidth = 1.0f;
        pd3dDevice->SetRenderState( D3DRS_LINEWIDTH, *((DWORD*)&fLineWidth));

        verts[1] = verts[0] + vXAxis;
        ATG::SimpleShaders::BeginShader_Transformed_ConstantColor( matWVP, D3DCOLOR_ARGB(255,255,0,0));
        pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, verts, sizeof(XMVECTOR) );
        ATG::SimpleShaders::EndShader();

        verts[1] = verts[0] + vYAxis;
        ATG::SimpleShaders::BeginShader_Transformed_ConstantColor( matWVP, D3DCOLOR_ARGB(255,0,255,0));
        pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, verts, sizeof(XMVECTOR) );
        ATG::SimpleShaders::EndShader();

        verts[1] = verts[0] + vZAxis;
        ATG::SimpleShaders::BeginShader_Transformed_ConstantColor( matWVP, D3DCOLOR_ARGB(255,0,0,255));
        pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, verts, sizeof(XMVECTOR) );
        ATG::SimpleShaders::EndShader();

    }

    PIXEndNamedEvent();
}


//--------------------------------------------------------------------------------------
// Name: GetNuiSkeletonJointPos()
// Desc: Returns the joint data for a particular joint
//--------------------------------------------------------------------------------------
XMVECTOR NuiJointConverter::GetNuiSkeletonJointPos( NUI_SKELETON_POSITION_INDEX	jointName  ) const
{
    return m_NuiSkeleton.SkeletonPositions[ jointName ];
}


//--------------------------------------------------------------------------------------
// Name: GetNuiSkeleton()
// Desc: Returns the joint data for a particular joint
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::GetNuiSkeletonData( NUI_SKELETON_DATA*	pResult ) const
{
    assert( pResult != NULL );
    XMemCpy( pResult, &m_NuiSkeleton, sizeof( NUI_SKELETON_DATA ));
    for(UINT i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
    {
        pResult->SkeletonPositions[i] = m_vReconstructedPos[i];
        pResult->SkeletonPositions[i].z *= -1.0f;
    }
}
    
//--------------------------------------------------------------------------------------
// Name: GetAvatarRotations()
// Desc: Convert the skeleton rotations to the avatar skeleton
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::GetAvatarRotations( XAVATAR_SKELETON_POSE_JOINT* pAvatarJoints )
{
    // Clear out all avatar rotations to identity
    for(INT i = XAVATAR_MAX_SKELETON_JOINTS-1; i >= 1; --i)
    {
        pAvatarJoints[i].Rotation = XMQuaternionIdentity();
    }

    // Apply the retargeted skeleton to the avatar
    for ( INT i = 0; i < ARRAYSIZE( g_IntermediateToAvatarMap ); ++i )
    {
        pAvatarJoints[g_IntermediateToAvatarMap[i].m_avatarJoint].Rotation = m_IntJoints[ g_IntermediateToAvatarMap[i].m_intJoint ].quatWorld;
    }

    // Set the position for the base joint
    XMVECTOR vJointPos;
    if( m_NuiSkeleton.eSkeletonPositionTrackingState[ NUI_SKELETON_POSITION_HIP_CENTER ] == NUI_SKELETON_POSITION_NOT_TRACKED )
    {
        vJointPos = XMVectorSet(0.0f, 0.0f, -2.0f, 0.0f);
    }
    else
    {
        vJointPos = GetNuiSkeletonJointPos( NUI_SKELETON_POSITION_HIP_CENTER );
    }

    pAvatarJoints[0].Position = vJointPos;

    // Convert joint rotations from world space to local space.  We go backwards from the highest index so that the parent joints
    // have world space rotations when their children request it.
    for (  INT i = ARRAYSIZE(g_IntermediateToAvatarMap) - 1; i >= 1; --i )
    {
        XMVECTOR qInvParentRot = XMQuaternionInverse( pAvatarJoints[ g_IntermediateToAvatarMap[i].m_avatarParentJoint ].Rotation );
        pAvatarJoints[ g_IntermediateToAvatarMap[i].m_avatarJoint ].Rotation = XMQuaternionMultiply( pAvatarJoints[ g_IntermediateToAvatarMap[i].m_avatarJoint ].Rotation, qInvParentRot);
    }

    // Modify the rotation of the wrists about the axis of the wrist depending on the shoulder and elbow rotations
    // Since there is no way to get an actual wrist rotation, this makes it possible for the avatar to wave with
    // hands facing forward.
    
    // Left hand
    ModifyWrist( pAvatarJoints, LF_W, LF_E, LF_S, 1.0f );

    // Right Hand
    ModifyWrist( pAvatarJoints, RT_W, RT_E, RT_S, -1.0f);
}

//--------------------------------------------------------------------------------------
// Name: ModifyWrist()
// Desc: Modify the rotation of the wrist to show the hand forward when the arm is in a
//       waving position.  As the camera gives only joint positions, there is no way 
//       to determine wrist rotation, so we simply guess at it according to the arm
//       position
//--------------------------------------------------------------------------------------
VOID NuiJointConverter::ModifyWrist( XAVATAR_SKELETON_POSE_JOINT* pAvatarJoints, AvatarJointRef wrist, AvatarJointRef elbow, AvatarJointRef shoulder, FLOAT fShoulderAngleScale )
{
    XMVECTOR vElbowAxis;
    FLOAT fElbowAngle;
    XMQuaternionToAxisAngle( &vElbowAxis, &fElbowAngle, pAvatarJoints[elbow].Rotation );

    // Since the shoulder can rotate more freely, we have to do some work to find the rotation about Z
    XMVECTOR vShoulderAxis;
    FLOAT fShoulderAngle;
    XMMATRIX matRotation = XMMatrixRotationQuaternion( pAvatarJoints[shoulder].Rotation );
    XMVECTOR vRotated = XMVector3Transform( XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), matRotation );
    vRotated.z = 0.0f;      // zero out the z component so we get a pure z rotation
    XMVECTOR quatZRot = GetShortestRotationBetweenVecs( vRotated, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) );
    XMQuaternionToAxisAngle( &vShoulderAxis, &fShoulderAngle, quatZRot );
    if( vShoulderAxis.z < 0)
    {
        fShoulderAngle *= -1.0f;
    }

    // Now use the shoulder and elbow rotation information to create a rotation for the wrist
    FLOAT fShoulderLerpAmount = max( min( fShoulderAngleScale * fShoulderAngle/XM_PIDIV4, 1.0f ), 0.0f );
    FLOAT fLerpAmount = max( min( fElbowAngle/1.2f +  fShoulderLerpAmount, 1.0f ), 0.0f );
    FLOAT fWristRot = fLerpAmount * -XM_PIDIV2;
    XMVECTOR vAxisRot = XMQuaternionRotationAxis( XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), fWristRot );
    pAvatarJoints[wrist].Rotation = XMQuaternionMultiply( vAxisRot, pAvatarJoints[wrist].Rotation );
    pAvatarJoints[wrist].Rotation = XMQuaternionNormalize( pAvatarJoints[wrist].Rotation );

}

} // namespace ATG