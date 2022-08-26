//--------------------------------------------------------------------------------------
// AtgSkeletalAnimation.cpp
//
// Xbox Game Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgSkeletalAnimation.h"

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: Skeleton::Initialize()
// Desc: Initializes a skeleton struct from a hierarchy of frames in a scene.
//--------------------------------------------------------------------------------------
VOID Skeleton::Initialize( ATG::Frame* pSkeletonRootFrame )
{
    // Build skeleton data structure from the scene hierarchy.
    BuildFromFrameHierarchy( pSkeletonRootFrame, -1 );

    // Allocate bind poses for the skeleton.
    DWORD dwBoneCount = GetBoneCount();
    m_BindPoseLocal.Allocate( dwBoneCount );
    m_InverseBindPose.Allocate( dwBoneCount );

    // Build local bind pose and inverse bind pose.
    for( DWORD i = 0; i < dwBoneCount; ++i )
    {
        ATG::Frame* pFrame = m_FrameVector[i];
        XMMATRIX matWorld = pFrame->GetWorldTransform();
        XMVECTOR vDummy;
        XMMATRIX matInvWorld = XMMatrixInverse( &vDummy, matWorld );
        XMMATRIX matLocal = pFrame->GetLocalTransform();

        m_BindPoseLocal.StoreTransform( i, matLocal );
        m_InverseBindPose.StoreTransform( i, matInvWorld );
    }
}


//--------------------------------------------------------------------------------------
// Name: Skeleton::BuildFromFrameHierarchy
// Desc: Recursive function that walks a frame hierarchy and creates vectors of frame
//       pointers and parent indices.
//--------------------------------------------------------------------------------------
VOID Skeleton::BuildFromFrameHierarchy( ATG::Frame* pRootFrame, INT iParentIndex )
{
    assert( pRootFrame != NULL );
    m_FrameVector.push_back( pRootFrame );
    INT iCurrentIndex = ( INT )m_ParentIndex.size();
    m_ParentIndex.push_back( iParentIndex );

    ATG::Frame* pChild = pRootFrame->GetFirstChild();
    while( pChild != NULL )
    {
        BuildFromFrameHierarchy( pChild, iCurrentIndex );
        pChild = pChild->GetNextSibling();
    }
}


//--------------------------------------------------------------------------------------
// Name: Skeleton::FindBone()
// Desc: Searches for a frame of a given name and returns the index in the skeleton.
//--------------------------------------------------------------------------------------
INT Skeleton::FindBone( const ATG::StringID name )
{
    DWORD dwCount = GetBoneCount();
    for( DWORD i = 0; i < dwCount; ++i )
    {
        if( m_FrameVector[i]->GetName() == name )
            return ( INT )i;
    }
    return -1;
}


//--------------------------------------------------------------------------------------
// Name: SkeletonInstance::Initialize()
// Desc: Initializes a skeleton instance and allocates poses to match the bone count of
//       the skeleton.
//--------------------------------------------------------------------------------------
VOID SkeletonInstance::Initialize( Skeleton* pSkeleton, DWORD dwSkinnedMeshBindingCount )
{
    m_pActiveAnimation = NULL;
    m_pSkeleton = pSkeleton;
    m_LocalPose.Allocate( pSkeleton->GetBoneCount() );
    m_WorldPose.Allocate( pSkeleton->GetBoneCount() );

    m_qJointRotOffset = new XMVECTOR[pSkeleton->GetBoneCount() ];
    for( DWORD i = 0; i < m_pSkeleton->GetBoneCount(); i++)
    {
        m_qJointRotOffset[i] = XMQuaternionIdentity();
    }

    m_vJointPosOffset = new XMVECTOR[pSkeleton->GetBoneCount() ];
    for( DWORD i = 0; i < m_pSkeleton->GetBoneCount(); i++)
    {
        m_vJointPosOffset[i] = XMVectorZero();
    }

    if( m_pSkinnedMeshBindings != NULL )
    {
        if( dwSkinnedMeshBindingCount != m_dwSkinnedMeshBindingCount )
        {
            delete m_pSkinnedMeshBindings;
            m_pSkinnedMeshBindings = NULL;
        }
    }

    if( m_pSkinnedMeshBindings == NULL )
        m_pSkinnedMeshBindings = new SkinnedMeshBinding[ dwSkinnedMeshBindingCount ];

    m_dwSkinnedMeshBindingCount = dwSkinnedMeshBindingCount;
}


//--------------------------------------------------------------------------------------
// Name: SkeletonInstance::BindSkinnedMesh
// Desc: Creates a skinned mesh binding.
//--------------------------------------------------------------------------------------
VOID SkeletonInstance::BindSkinnedMesh( DWORD dwIndex, ATG::SkinnedMesh* pSkinnedMesh )
{
    assert( dwIndex < m_dwSkinnedMeshBindingCount );
    SkinnedMeshBinding& Binding = m_pSkinnedMeshBindings[ dwIndex ];
    Binding.m_pSkinnedMesh = pSkinnedMesh;

    // Build skinning palette.  The transform buffer will be supplied at runtime.
    DWORD dwInfluenceCount = pSkinnedMesh->GetInfluenceCount();
    Binding.m_BoneMatrixPalette.m_dwPoseSize = dwInfluenceCount;
    Binding.m_BoneMatrixPalette.m_pTransforms = NULL;

    // Map influence bones in the skinned mesh to skeleton bones.
    Binding.m_pSkinInfluenceToSkeletonBone = new INT[ dwInfluenceCount ];
    for( DWORD i = 0; i < dwInfluenceCount; ++i )
    {
        const ATG::StringID strInfluenceName = Binding.m_pSkinnedMesh->GetInfluence( i );
        INT iBoneIndex = m_pSkeleton->FindBone( strInfluenceName );
        Binding.m_pSkinInfluenceToSkeletonBone[i] = iBoneIndex;
    }
}


//--------------------------------------------------------------------------------------
// Name: SkeletonInstance::CreateAnimationBinding()
//--------------------------------------------------------------------------------------
AnimationBinding* SkeletonInstance::CreateAnimationBinding( ATG::Animation* pAnimation )
{
    // Create animation binding and initialize it.
    if( m_pActiveAnimation == NULL )
        m_pActiveAnimation = new AnimationBinding;
    m_pActiveAnimation->Initialize( this, pAnimation );
    return m_pActiveAnimation;
}


//--------------------------------------------------------------------------------------
// Name: SkeletonInstance::UpdateAnimation
// Desc: Samples the active animation, creating new local and world pose transforms.
//--------------------------------------------------------------------------------------
VOID SkeletonInstance::UpdateAnimation( FLOAT fDeltaTime )
{
    // Update the animation clock.
    if( m_pActiveAnimation )
        m_pActiveAnimation->Tick( fDeltaTime );

    // Initialize the local pose with the bind pose local transforms.
    m_pSkeleton->m_BindPoseLocal.CopyToPose( &m_LocalPose );

    // Sample animation tracks into the local pose.
    if( m_pActiveAnimation )
        m_pActiveAnimation->SampleAnimationToLocalPose( &m_LocalPose );

    // Apply rotation and position offsets
    for( DWORD iBoneIndex = 0; iBoneIndex <  m_pSkeleton->GetBoneCount(); iBoneIndex++)
    {
        XMVECTOR qOrientation = XMLoadFloat4(&m_LocalPose.m_pPositionsOrientations[iBoneIndex*2+1]);
        qOrientation = XMQuaternionMultiply( m_qJointRotOffset[ iBoneIndex ], qOrientation );
        XMStoreFloat4( &m_LocalPose.m_pPositionsOrientations[iBoneIndex*2+1], qOrientation );

        XMVECTOR vPos = XMLoadFloat4(&m_LocalPose.m_pPositionsOrientations[iBoneIndex*2]);
        vPos += m_vJointPosOffset[iBoneIndex];
        XMStoreFloat4( &m_LocalPose.m_pPositionsOrientations[iBoneIndex*2], vPos );
    }
}


//--------------------------------------------------------------------------------------
// Name: SkeletonInstance::BuildWorldPose()
// Desc: Create the World pose from the Local pose
//--------------------------------------------------------------------------------------
VOID SkeletonInstance::BuildWorldPose()
{
    // The root bone's world pose is the local pose.
    m_WorldPose.StoreTransform( 0, m_LocalPose.LoadTransform( 0 ) );

    // Compute the world pose for each bone.
    const DWORD dwBoneCount = m_pSkeleton->GetBoneCount();
    for( DWORD i = 1; i < dwBoneCount; ++i )
    {
        // Determine the parent index for the local bone.
        INT iParentBoneIndex = m_pSkeleton->m_ParentIndex[i];
        assert( iParentBoneIndex >= 0 && iParentBoneIndex < ( INT )dwBoneCount );

        // Load the local transform and the parent's world transform.
        XMMATRIX matLocal = m_LocalPose.LoadTransform( i );
        XMMATRIX matParentWorld = m_WorldPose.LoadTransform( iParentBoneIndex );

        // Compute the world transform for this bone, and store it into the world pose.
        XMMATRIX matWorld = XMMatrixMultiply( matLocal, matParentWorld );
        m_WorldPose.StoreTransform( i, matWorld );
    }
}


//--------------------------------------------------------------------------------------
// Name: SkeletonInstance::CreateBonePalette
// Desc: For a given skinned mesh index, creates the bone palette from the world pose
//       and the skeleton's inverse bind pose.
//--------------------------------------------------------------------------------------
VOID SkeletonInstance::CreateBonePalette( DWORD dwSkinnedMeshIndex, VOID* pDestBonePalette, BOOL bDestPaletteHalf4 )
{
    assert( dwSkinnedMeshIndex < m_dwSkinnedMeshBindingCount );
    SkinnedMeshBinding& Binding = m_pSkinnedMeshBindings[ dwSkinnedMeshIndex ];

    // Update skin palette buffer pointer.
    // This allows the animation system to write its results directly into the memory
    // used by the GPU or the CPU for skinning.
    assert( pDestBonePalette != NULL );
    if( bDestPaletteHalf4 )
        Binding.m_BoneMatrixPalette.m_pTransformsHalf = ( XMHALF4* )pDestBonePalette;
    else
        Binding.m_BoneMatrixPalette.m_pTransforms = ( XMFLOAT4A* )pDestBonePalette;

    // Compute skinning transforms (also known as composite transforms).
    // We only need to compute the ones used by the skinned mesh.
    const DWORD dwPaletteCount = Binding.GetPaletteSize();
    for( DWORD i = 0; i < dwPaletteCount; ++i )
    {
        // Determine skin influence to bone index mapping.
        // Usually, not all bones in the skeleton influence the mesh.
        INT iSourceBoneIndex = Binding.m_pSkinInfluenceToSkeletonBone[i];
        assert( iSourceBoneIndex >= 0 && iSourceBoneIndex < ( INT )m_pSkeleton->GetBoneCount() );

        // Load the world transform for the bone, as well as the inverse bind pose transform.
        XMMATRIX matWorld = m_WorldPose.LoadTransform( iSourceBoneIndex );
        XMMATRIX matInvBindPose = m_pSkeleton->m_InverseBindPose.LoadTransform( iSourceBoneIndex );

        // Compute the composite transform, transpose it for consumption by the GPU,
        // and store it into the skin palette.  The transform is ready to be used for
        // mesh deformation.
        XMMATRIX matComposite = matInvBindPose * matWorld;
        matComposite = XMMatrixTranspose( matComposite );
        if( bDestPaletteHalf4 )
            Binding.m_BoneMatrixPalette.StoreTransformHalf( i, matComposite );
        else
            Binding.m_BoneMatrixPalette.StoreTransform( i, matComposite );
    }
}


//--------------------------------------------------------------------------------------
// Name: AnimationBinding::Initialize()
// Desc: Creates a binding between animation tracks and skeleton bone indices.
//--------------------------------------------------------------------------------------
VOID AnimationBinding::Initialize( SkeletonInstance* pSkeletonInstance, ATG::Animation* pAnimation )
{
    m_pSkeletonInstance = pSkeletonInstance;
    m_pAnimation = pAnimation;
    m_fPlaybackTime = 0;
    m_fPlaybackSpeed = 1.0f;
    DWORD dwTrackCount = m_pAnimation->GetAnimationTrackCount();

    if( m_dwTrackCount != 0 && m_dwTrackCount != dwTrackCount )
    {
        //delete old memory
        delete[] m_pAnimationTrackToSkeletonBone;
        m_pAnimationTrackToSkeletonBone = NULL;
        delete[] m_pPositionTrackLastKey;
        m_pPositionTrackLastKey = NULL;
        delete[] m_pOrientationTrackLastKey;
        m_pOrientationTrackLastKey = NULL;
    }

    // Map animation tracks to bone indices.

    if( m_pAnimationTrackToSkeletonBone == NULL )
        m_pAnimationTrackToSkeletonBone = new INT[ dwTrackCount ];

    Skeleton* pSkeleton = m_pSkeletonInstance->m_pSkeleton;
    for( DWORD i = 0; i < dwTrackCount; ++i )
    {
        ATG::AnimationTransformTrack* pTrack = m_pAnimation->GetAnimationTrack( i );
        INT iBoneIndex = pSkeleton->FindBone( pTrack->GetName() );
        m_pAnimationTrackToSkeletonBone[i] = iBoneIndex;
    }

    // Create buffers to hold the last sampled key.  These make animation curve sampling
    // much faster.
    if( m_pPositionTrackLastKey == NULL )
        m_pPositionTrackLastKey = new DWORD[ dwTrackCount ];
    ZeroMemory( m_pPositionTrackLastKey, dwTrackCount * sizeof( DWORD ) );

    if( m_pOrientationTrackLastKey == NULL )
        m_pOrientationTrackLastKey = new DWORD[ dwTrackCount ];
    ZeroMemory( m_pOrientationTrackLastKey, dwTrackCount * sizeof( DWORD ) );

    m_dwTrackCount = dwTrackCount;

}


//--------------------------------------------------------------------------------------
// Name: AnimationBinding::SampleAnimationToLocalPose()
// Desc: Samples each transform track to local transform components (position and
//       orientation).
//--------------------------------------------------------------------------------------
VOID AnimationBinding::SampleAnimationToLocalPose( PoseDecomp* pLocalPose )
{
    const DWORD dwTrackCount = m_pAnimation->GetAnimationTrackCount();
    const FLOAT fDuration = m_pAnimation->GetDuration();
    for( DWORD i = 0; i < dwTrackCount; ++i )
    {
        // Determine track index to bone index mapping.
        INT iBoneIndex = m_pAnimationTrackToSkeletonBone[i];
        if( iBoneIndex == -1 )
            continue;
        assert( iBoneIndex < ( INT )pLocalPose->m_dwPoseSize );

        // Sample the animation track.
        // We keep track of the last key index used to accelerate the key search.
        const ATG::AnimationTransformTrack* pTrack = m_pAnimation->GetAnimationTrack( i );
        XMVECTOR vPosition = pTrack->SamplePositionLooping( m_fPlaybackTime, fDuration, &m_pPositionTrackLastKey[i] );
        XMVECTOR qOrientation = pTrack->SampleOrientationLooping( m_fPlaybackTime,
                                                                  fDuration, &m_pOrientationTrackLastKey[i] );
        qOrientation = XMQuaternionNormalize( qOrientation );

        // Store the local transform into the local pose.
        pLocalPose->StorePositionOrientation( iBoneIndex, vPosition, qOrientation );
    }
}

} // namespace ATG