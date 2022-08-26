//--------------------------------------------------------------------------------------
// AtgSkeletalAnimation.h
//
// A set of structs and methods for performing skeletal animation.
//
// Xbox Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifndef ATG_SKELETAL_ANIMATION_H
#define ATG_SKELETAL_ANIMATION_H

#include <assert.h>
#include <xnamath.h>
#include <vector>
#include "AtgScene.h"
#include "AtgSceneMesh.h"
#include "AtgAnimation.h"


extern "C"
    void _WriteBarrier();
#pragma intrinsic(_WriteBarrier)

namespace ATG
{

static const DWORD g_dwMaxBoneCount = 80;

//--------------------------------------------------------------------------------------
// Name: struct PoseDecomp
// Desc: Stores an array of interleaved position vectors and orientation quaternions.
//--------------------------------------------------------------------------------------
struct
PoseDecomp
{
    DWORD m_dwPoseSize;
    XMFLOAT4A* m_pPositionsOrientations;

    PoseDecomp()
    {
        m_pPositionsOrientations = NULL;
    }

    VOID Allocate( DWORD dwBoneCount )
    {
        if( m_dwPoseSize != dwBoneCount && m_pPositionsOrientations != NULL )
        {
            delete[] m_pPositionsOrientations;
            m_pPositionsOrientations = NULL;
        }
        m_dwPoseSize = dwBoneCount;
        if( m_pPositionsOrientations == NULL )
        {
            m_pPositionsOrientations = new XMFLOAT4A[ dwBoneCount * 2 ];
        }
    }
    VOID CopyToPose( PoseDecomp* pDestPose )
    {
        assert( m_dwPoseSize == pDestPose->m_dwPoseSize );
        XMemCpy( pDestPose->m_pPositionsOrientations, m_pPositionsOrientations, m_dwPoseSize * 2 * sizeof
                 ( XMFLOAT4A ) );
    }
    VOID StorePositionOrientation( DWORD dwIndex, const XMVECTOR vPos, const XMVECTOR qOrientation )
    {
        dwIndex *= 2;
        XMStoreFloat4A( &m_pPositionsOrientations[ dwIndex ], vPos );
        XMStoreFloat4A( &m_pPositionsOrientations[ dwIndex + 1 ], qOrientation );
    }
    VOID StoreTransform( DWORD dwIndex, CXMMATRIX matTransform )
    {
        XMVECTOR vPos = matTransform.r[3];
        XMVECTOR qOrientation = XMQuaternionRotationMatrix( matTransform );
        StorePositionOrientation( dwIndex, vPos, qOrientation );
    }
    XMMATRIX LoadTransform( DWORD dwIndex ) const
    {
        static const XMVECTOR vSelectW = XMVectorSelectControl( 0, 0, 0, 1 );
        static const XMVECTOR vOne = XMVectorReplicate( 1.0f );

        dwIndex *= 2;
        XMVECTOR vPos = XMLoadFloat4A( &m_pPositionsOrientations[ dwIndex ] );
        XMVECTOR qOrientation = XMLoadFloat4A( &m_pPositionsOrientations[ dwIndex + 1 ] );
        // Compose a transform matrix from the rotation quaternion and the position vector.
        XMMATRIX matTransform = XMMatrixRotationQuaternion( qOrientation );
        matTransform.r[3] = XMVectorSelect( vPos, vOne, vSelectW );
        return matTransform;
    }
#if defined ( _PPC_ )
    VOID Prefetch() const
    {
        DWORD dwSize = m_dwPoseSize * 2 * sizeof( XMFLOAT4A );
        for( DWORD i = 0; i < dwSize; i += 128 )
            __dcbt( i, m_pPositionsOrientations );
    }
#else
    VOID Prefetch() const
    {
        // Do not prefetch data, if not running on Xbox
    }
#endif
    VOID BlendPoses( const PoseDecomp* __restrict pSrcPoseA, const PoseDecomp* __restrict pSrcPoseB, FLOAT fLerpFactor )
    {
        const XMVECTOR vLerpFactor = XMVectorReplicate( fLerpFactor );
        assert( m_dwPoseSize == pSrcPoseA->m_dwPoseSize && m_dwPoseSize == pSrcPoseB->m_dwPoseSize );
        pSrcPoseA->Prefetch();
        pSrcPoseB->Prefetch();
        DWORD dwArrayIndex = 0;
        for( DWORD i = 0; i < m_dwPoseSize; ++i )
        {
            XMVECTOR vPosA = XMLoadFloat4A( &pSrcPoseA->m_pPositionsOrientations[dwArrayIndex] );
            XMVECTOR vPosB = XMLoadFloat4A( &pSrcPoseB->m_pPositionsOrientations[dwArrayIndex] );
            XMVECTOR vBlendedPos = XMVectorLerpV( vPosA, vPosB, vLerpFactor );
            XMStoreFloat4A( &m_pPositionsOrientations[dwArrayIndex], vBlendedPos );
            XMVECTOR vOrientationA = XMLoadFloat4A( &pSrcPoseA->m_pPositionsOrientations[dwArrayIndex + 1] );
            XMVECTOR vOrientationB = XMLoadFloat4A( &pSrcPoseB->m_pPositionsOrientations[dwArrayIndex + 1] );
            XMVECTOR vBlendedOrientation = XMQuaternionSlerpV( vOrientationA, vOrientationB, vLerpFactor );
            XMStoreFloat4A( &m_pPositionsOrientations[dwArrayIndex + 1], vBlendedOrientation );
            dwArrayIndex += 2;
        }
    }
};


//--------------------------------------------------------------------------------------
// Name: struct Pose4x4
// Desc: Stores an array of aligned 4x4 transform matrices.
//--------------------------------------------------------------------------------------
struct Pose4x4
{
    DWORD m_dwPoseSize;
    XMFLOAT4X4A* m_pTransforms;

                Pose4x4()
                {
                    m_pTransforms = NULL;
                }
    VOID        Allocate( DWORD dwBoneCount )
    {
        if( m_dwPoseSize != dwBoneCount && m_pTransforms != NULL )
        {
            delete[] m_pTransforms;
            m_pTransforms = NULL;
        }

        m_dwPoseSize = dwBoneCount;
        if( m_pTransforms == NULL )
        {
            m_pTransforms = new XMFLOAT4X4A[ dwBoneCount ];
        }
    }
    VOID        CopyToPose( Pose4x4* pDestPose ) const
    {
        assert( m_dwPoseSize == pDestPose->m_dwPoseSize );
        XMemCpy( pDestPose->m_pTransforms, m_pTransforms, m_dwPoseSize * sizeof( XMFLOAT4X4A ) );
    }
    XMMATRIX    LoadTransform( DWORD dwIndex ) const
    {
        return XMLoadFloat4x4A( &m_pTransforms[ dwIndex ] );
    }
    VOID        StoreTransform( DWORD dwIndex, CXMMATRIX matTransform )
    {
        XMStoreFloat4x4A( &m_pTransforms[ dwIndex ], matTransform );
    }
};

//--------------------------------------------------------------------------------------
// Name: struct Pose3x4
// Desc: Stores an array of aligned 3x4 transposed transform matrices.
//       This is the type of transform matrix consumed by the GPU or CPU as a bone
//       palette matrix.  The perspective components of the matrix in the fourth
//       row are not needed for skinning, therefore they are omitted.
//--------------------------------------------------------------------------------------
struct Pose3x4
{
    DWORD m_dwPoseSize;
    union
    {
        XMFLOAT4A* m_pTransforms;
        XMHALF4* m_pTransformsHalf;
    };

                Pose3x4()
                {
                    m_pTransforms = NULL;
                }

    VOID        Allocate( const DWORD dwBoneCount )
    {
        if( m_dwPoseSize != dwBoneCount && m_pTransforms != NULL )
        {
            delete[] m_pTransforms;
            m_pTransforms = NULL;
        }

        m_dwPoseSize = dwBoneCount;
        if( m_pTransforms == NULL )
        {
            m_pTransforms = new XMFLOAT4A[ dwBoneCount * 3 ];
        }
    }
    VOID        AllocateHalf( const DWORD dwBoneCount )
    {
        if( m_dwPoseSize != dwBoneCount && m_pTransformsHalf != NULL )
        {
            delete[] m_pTransformsHalf;
            m_pTransformsHalf = NULL;
        }

        m_dwPoseSize = dwBoneCount;
        if( m_pTransformsHalf == NULL )
        {
            m_pTransformsHalf = new XMHALF4[ dwBoneCount * 3 ];
        }
    }
    VOID        StoreTransform( const DWORD dwIndex, CXMMATRIX matTransform )
    {
        // Writing the 3x4 matrix to memory.
        // Note the usage of write barriers here; they instruct the compiler not to
        // reorder the writes, in case we're writing to write-combined memory.
        // If the writes happen out of order, the CPU's store-gathering hardware that
        // is used for write-combined memory will not be used effectively, and the 
        // entire write operation will be much slower.
        XMFLOAT4A* pDest = &m_pTransforms[ dwIndex * 3 ];
        XMStoreFloat4A( pDest++, matTransform.r[0] );
        _WriteBarrier();
        XMStoreFloat4A( pDest++, matTransform.r[1] );
        _WriteBarrier();
        XMStoreFloat4A( pDest, matTransform.r[2] );
    }
    VOID        StoreTransformHalf( const DWORD dwIndex, CXMMATRIX matTransform )
    {
        // Writing the 3x4 matrix to memory.
        // Note the usage of write barriers here; they instruct the compiler not to
        // reorder the writes, in case we're writing to write-combined memory.
        // If the writes happen out of order, the CPU's store-gathering hardware that
        // is used for write-combined memory will not be used effectively, and the 
        // entire write operation will be much slower.
        XMHALF4* pDest = &m_pTransformsHalf[ dwIndex * 3 ];
        XMStoreHalf4( pDest++, matTransform.r[0] );
        _WriteBarrier();
        XMStoreHalf4( pDest++, matTransform.r[1] );
        _WriteBarrier();
        XMStoreHalf4( pDest, matTransform.r[2] );
    }
    // Note: LoadTransform does not fill in the fourth row of the matrix.
    XMMATRIX    LoadTransform( const DWORD dwIndex ) const
    {
        XMMATRIX matTransform;
        XMFLOAT4A* pSrc = &m_pTransforms[ dwIndex * 3 ];
        matTransform.r[0] = XMLoadFloat4A( pSrc++ );
        matTransform.r[1] = XMLoadFloat4A( pSrc++ );
        matTransform.r[2] = XMLoadFloat4A( pSrc );
        return matTransform;
    }
    XMMATRIX    LoadTransformHalf( const DWORD dwIndex ) const
    {
        XMMATRIX matTransform;
        XMHALF4* pSrc = &m_pTransformsHalf[ dwIndex * 3 ];
        matTransform.r[0] = XMLoadHalf4( pSrc++ );
        matTransform.r[1] = XMLoadHalf4( pSrc++ );
        matTransform.r[2] = XMLoadHalf4( pSrc );
        return matTransform;
    }
};


//--------------------------------------------------------------------------------------
// Name: struct Skeleton
// Desc: Represents a hierarchy of bone transforms that point back to frames within
//       a scene hierarchy.
//       The bind pose is the "resting" pose of the skeleton, and the inverse bind
//       pose is computed and cached at load time to be used at runtime for skinning.
//--------------------------------------------------------------------------------------
struct Skeleton
{
    std::vector <ATG::Frame*> m_FrameVector;
    std::vector <INT> m_ParentIndex;
    PoseDecomp m_BindPoseLocal;
    Pose4x4 m_InverseBindPose;

    VOID    Initialize( ATG::Frame* pSkeletonRootFrame );
    DWORD   GetBoneCount() const
    {
        assert( m_FrameVector.size() == m_ParentIndex.size() );
        return m_FrameVector.size();
    }
    INT     FindBone( const ATG::StringID name );

private:
    VOID    BuildFromFrameHierarchy( ATG::Frame* pRootFrame, INT iParentIndex );
};


//--------------------------------------------------------------------------------------
// Name: struct SkinnedMeshBinding
// Desc: Represents a mapping between the bones that influence a skinned mesh and the
//       bones in a skeleton instance.
//--------------------------------------------------------------------------------------
struct SkinnedMeshBinding
{
    ATG::SkinnedMesh* m_pSkinnedMesh;
    INT* m_pSkinInfluenceToSkeletonBone;
    Pose3x4 m_BoneMatrixPalette;

    DWORD GetPaletteSize() const
    {
        return m_BoneMatrixPalette.m_dwPoseSize;
    }
};


//--------------------------------------------------------------------------------------
// Name: struct SkeletonInstance
// Desc: Represents the animated state of a skeleton.
//       Manages bindings to an animation track set and a group of skinned meshes.
//--------------------------------------------------------------------------------------
struct AnimationBinding;
struct SkeletonInstance
{
    Skeleton* m_pSkeleton;
    PoseDecomp m_LocalPose;
    Pose4x4 m_WorldPose;
    AnimationBinding* m_pActiveAnimation;
    SkinnedMeshBinding* m_pSkinnedMeshBindings;
    
    XMVECTOR* m_qJointRotOffset;
    XMVECTOR* m_vJointPosOffset;

    DWORD m_dwSkinnedMeshBindingCount;
            SkeletonInstance()
            {
                m_pActiveAnimation = NULL;
                m_pSkinnedMeshBindings = NULL;
                m_pSkeleton = NULL;
                m_qJointRotOffset = NULL;
                m_vJointPosOffset = NULL;
            }
    VOID    Initialize( Skeleton* pSkeleton, DWORD dwSkinnedMeshBindingCount );
    VOID    BindSkinnedMesh( DWORD dwIndex, ATG::SkinnedMesh* pSkinnedMesh );
    AnimationBinding* CreateAnimationBinding( ATG::Animation* pAnimation );
    VOID    UpdateAnimation( FLOAT fDeltaTime );
    VOID    SetJointRotationOffset( DWORD jointIndex, XMVECTOR qRot ) {m_qJointRotOffset[jointIndex] = qRot;}
    VOID    SetJointPositionOffset( DWORD jointIndex, XMVECTOR vPos ) {m_vJointPosOffset[jointIndex] = vPos;}
    VOID    BuildWorldPose();
    VOID    CreateBonePalette( DWORD dwSkinnedMeshIndex, VOID* pDestBonePalette, BOOL bDestPaletteHalf4 );
};


//--------------------------------------------------------------------------------------
// Name: struct AnimationBinding
// Desc: Represents a mapping between tracks in an animation and bones in a skeleton
//       instance.  Also holds "last key" values that accelerate curve sampling at
//       runtime.
//--------------------------------------------------------------------------------------
struct AnimationBinding
{
    SkeletonInstance* m_pSkeletonInstance;
    ATG::Animation* m_pAnimation;
    INT* m_pAnimationTrackToSkeletonBone;
    DWORD* m_pPositionTrackLastKey;
    DWORD* m_pOrientationTrackLastKey;
    FLOAT m_fPlaybackTime;
    FLOAT m_fPlaybackSpeed;
    DWORD m_dwTrackCount;

            AnimationBinding()
            {
                m_pAnimationTrackToSkeletonBone = NULL;
                m_pPositionTrackLastKey = NULL;
                m_pOrientationTrackLastKey = NULL;
                m_dwTrackCount = 0;
            }
    VOID    Initialize( SkeletonInstance* pSkeletonInstance, ATG::Animation* pAnimation );
    VOID    Tick( FLOAT fDeltaTime )
    {
        m_fPlaybackTime += ( fDeltaTime * m_fPlaybackSpeed );
        m_fPlaybackTime = fmodf( m_fPlaybackTime, m_pAnimation->GetDuration() );
    }
    VOID    SampleAnimationToLocalPose( PoseDecomp* pLocalPose );
};

} //namespace ATG

#endif
