//--------------------------------------------------------------------------------------
// AtgNuiJointConverter.h
//
// Demonstrates a method of constraining NUI joints to plausible human biometrics.  This 
// method attaches a set of cone constraints to the joints, and prohibits movement of
// the child bone of that joint outside of the cone.  If the bone does move outside the 
// cone, it is adjusted to lie on the closest point of the cone.
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef ATG_NUIJOINTCONVERTER_H
#define ATG_NUIJOINTCONVERTER_H

#include <assert.h>
#include <xavatar.h>
#include <xtl.h>
#include <xboxmath.h>
#include <NuiApi.h>

namespace ATG
{

    // Avatar joints in the order documented in the XDK docs.
    enum AvatarJointRef{
        BASE = 0,
        BACKA,
        LF_H,
        RT_H,
        SC_BASE,
        BACKB,
        LF_K,
        LF_SC_H,
        RT_K,
        RT_SC_H,
        SC_BACKA,
        LF_A,
        LF_C,
        LF_SC_K,
        NECK,
        RT_A,
        RT_C,
        RT_SC_K,
        SC_BACKB,
        HEAD,
        LF_S,
        LF_T,
        RT_S,
        RT_T,
        SC_NECK,
        LF_E,
        LF_SC_S,
        LF_SC_S_SK,
        RT_E,
        RT_SC_S,
        RT_SC_S_SK,
        LF_E_SKIN,
        LF_SC_E,
        LF_W,
        RT_E_SKIN,
        RT_SC_E,
        RT_W,
        LF_FINGA,
        LF_FINGB,
        LF_FINGC,
        LF_FINGD,
        LF_PROP,
        LF_SPECIAL,
        LF_THUMB,
        RT_FINGA,
        RT_FINGB,
        RT_FINGC,
        RT_FINGD,
        RT_PROP,
        RT_SPECIAL,
        RT_THUMB,
        LF_FINGA1,
        LF_FINGB1,
        LF_FINGC1,
        LF_FINGD1,
        LF_THUMB1,
        RT_FINGA1,
        RT_FINGB1,
        RT_FINGC1,
        RT_FINGD1,
        RT_THUMB1,
        LF_FINGA2,
        LF_FINGB2,
        LF_FINGC2,
        LF_FINGD2,
        LF_THUMB2,
        RT_FINGA2,
        RT_FINGB2,
        RT_FINGC2,
        RT_FINGD2,
        RT_THUMB2,
        INVALID = 99
    };

    static const INT g_nNumSkeletonFrames   = 10; // The number of frames of camera data to buffer


    //-------------------------------------------------------------------------------------
    // Name: GetShortestRotationBetweenVecs()
    // Desc: Find the shortest rotation between two vectors and return in quaternion form
    //-------------------------------------------------------------------------------------
    inline XMVECTOR GetShortestRotationBetweenVecs( XMVECTOR v1, XMVECTOR v2 )
    {
        XMVECTOR v1Norm = XMVector3Normalize(v1);
        XMVECTOR v2Norm = XMVector3Normalize(v2);
        FLOAT angle = XMScalarACos( XMVectorGetX(XMVector3Dot(v1Norm, v2Norm)) );
        XMVECTOR axis = XMVector3Cross( v2Norm, v1Norm );

        // Check to see if the angle is very small, in which case, the cross product becomes unstable,
        // so set the axis to a default.  It doesn't matter much what this axis is, as the rotation angle 
        // will be near zero anyway.
        if( angle < 0.001f)
            axis = XMVectorSet( 1.0f, 0.0f, 0.0f, 0.0f );

        if( XMVectorGetX(XMVector3Length(axis)) < .001f )
            return XMQuaternionIdentity();

        axis = XMVector3Normalize(axis);
        XMVECTOR qRot = XMQuaternionRotationAxis( axis, angle );

        assert( XMVector3Dot(XMVector3Transform( v2Norm, XMMatrixRotationQuaternion( qRot ) ), v1Norm).x > .99f  );

        return qRot;
    }

    enum INTERMEDIATE_JOINT_INDEX
    {
        INT_BASE = 0,
        INT_HIP_CENTER,
        INT_SPINE,
        INT_SHOULDER_CENTER,
        INT_NECK,
        INT_COLLAR_LEFT,
        INT_SHOULDER_LEFT,
        INT_ELBOW_LEFT,
        INT_WRIST_LEFT,
        INT_COLLAR_RIGHT,
        INT_SHOULDER_RIGHT,
        INT_ELBOW_RIGHT,
        INT_WRIST_RIGHT,
        INT_PELVIS_LEFT,
        INT_HIP_LEFT,
        INT_KNEE_LEFT,
        INT_ANKLE_LEFT,
        INT_PELVIS_RIGHT,
        INT_HIP_RIGHT,
        INT_KNEE_RIGHT,
        INT_ANKLE_RIGHT,
        INTERMEDIATE_JOINT_COUNT
    };

    __declspec(align(16))
    typedef struct JointConstraint_s
    {
        XMVECTOR                    m_vDir;                 // Constraint cone direction
        INTERMEDIATE_JOINT_INDEX    m_intJoint;             // Intermediate joint
        FLOAT                       m_fAngle;               // Constraint cone angle
        FLOAT                       m_fConstraint;          // calculated dynamic value of constraint
    } JointConstraint;

    const DWORD MAX_JOINT_CONSTRAINTS = 100;

    __declspec(align(16))
    typedef struct IntermediateJoint
    {
        XMVECTOR quatLocal;
        XMVECTOR quatWorld;
        XMVECTOR vBoneVectorLS;                             // Local space bone vector
        FLOAT    fBoneLength;                               // Bone length
        NUI_SKELETON_POSITION_TRACKING_STATE eTrackingState;
        struct
        {
            XMVECTOR    m_qFilteredLocal;
            XMVECTOR    m_qTrend;
            UINT        m_uStarted;
        } doubleExponentialFilter;
    } IntermediateJoint;

    __declspec(align(16))
    typedef struct IntermediateJointDesc
    {
        XMVECTOR                        vBoneDir;               // Local space orientation of the joint
        NUI_SKELETON_POSITION_INDEX     startNuiJoint;          // Starting Nui joint for this bone
        NUI_SKELETON_POSITION_INDEX     endNuiJoint;            // End Nui joint for this bone
        INTERMEDIATE_JOINT_INDEX        parentIdx;              // Parent intermediate joint
    }IntermediateJointDesc;

    extern IntermediateJointDesc g_IntJointDesc[ INTERMEDIATE_JOINT_COUNT ];

    //--------------------------------------------------------------------------------------
    // Name: class NuiJointConverter
    // Desc: Converts a Camera skeleton to avatar joint rotations
    //--------------------------------------------------------------------------------------
    class NuiJointConverter
    {
    public:
        NuiJointConverter();
        VOID  ConvertNuiJoints( const NUI_SKELETON_DATA* pNuiSkeleton, BOOL bFilterJointOrientations );
        VOID  ConvertNuiToAvatarSkeleton( const NUI_SKELETON_DATA* pNuiSkeleton, XAVATAR_SKELETON_POSE_JOINT* pAvatarJoints );
        VOID  DrawSkeleton( ::D3DDevice* pd3dDevice, XMMATRIX matWVP ) const;
        VOID  AddJointConstraint( INTERMEDIATE_JOINT_INDEX joint, XMVECTOR vDir, FLOAT fAngle ); 
        VOID  AddDefaultConstraints( );
        VOID  GetNuiSkeletonData( NUI_SKELETON_DATA* pResult ) const;
        VOID  SetBindDir( INTERMEDIATE_JOINT_INDEX index, XMVECTOR vDir ) { g_IntJointDesc[index].vBoneDir = vDir; } 
        VOID  SetJointScaleFactor( XMVECTOR vScaleFactor ) { m_vJointScaleFactor = vScaleFactor; }
        VOID  SetBindPose( INTERMEDIATE_JOINT_INDEX index, XMVECTOR qRot ){ m_qBindPose[index] = qRot; }
        XMVECTOR GetLocalRot( INTERMEDIATE_JOINT_INDEX index ) { return m_IntJoints[index].quatLocal; }

    private:
        VOID  Initialize();
        VOID  Update( BOOL bEnableJointOrientationFilter );
        VOID  SetSkeleton( const NUI_SKELETON_DATA* pJoints );
        VOID  CollideSkeleton( NUI_SKELETON_DATA* pSkeleton );
        VOID  GetAvatarRotations( XAVATAR_SKELETON_POSE_JOINT* pAvatarJoints );
        VOID  ScaleJoints();
        VOID  CalculateJointRotations();
        VOID  DoubleExponentialJointOrientationFilter();
        VOID  ApplyJointConstraints();
        XMVECTOR GetNuiSkeletonJointPos( NUI_SKELETON_POSITION_INDEX jointIndex ) const;
        VOID  ModifyWrist( XAVATAR_SKELETON_POSE_JOINT* pAvatarJoints, AvatarJointRef wrist, AvatarJointRef elbow,
                           AvatarJointRef shoulder, FLOAT fShoulderAngleScale );

        VOID DistanceToLineSegment( XMVECTOR x0, XMVECTOR x1, XMVECTOR p, float* distance, XMVECTOR* normal ) const;

    private:
        NUI_SKELETON_DATA		m_NuiSkeleton;
        DWORD                   m_dwNumJointConstraints;
        JointConstraint		    m_jointConstraints[ MAX_JOINT_CONSTRAINTS ];
        IntermediateJoint       m_IntJoints[ INTERMEDIATE_JOINT_COUNT ];
        IntermediateJoint       m_PreviousIntJoints[ INTERMEDIATE_JOINT_COUNT ];
        XMVECTOR                m_qBindPose[ INTERMEDIATE_JOINT_COUNT ];
        XMVECTOR                m_vReconstructedPos[NUI_SKELETON_POSITION_COUNT];
        XMVECTOR                m_vJointScaleFactor;
    };
};

#endif // #ifdef ATG_NUIJOINTCONVERTER_H