//-----------------------------------------------------------------------------
// AtgAnimation.h
//
// Animation data structures for keyframed animation.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_ANIMATION_H
#define ATG_ANIMATION_H

#include <assert.h>
#include "AtgResourceDatabase.h"

namespace ATG
{
    //----------------------------------------------------------------------------------
    // Name: class AnimationKeyArray
    // Desc: A data structure that holds an array of multidimensional animation keys.
    //       Each key consists of a non-negative float time and at least one float value.
    //       It is crucial that key times be non-negative, since this class is optimized
    //       to avoid float branches, since they cause a performance penalty on the
    //       Xbox 360 CPU.
    //       The class is also optimized to be initialized with a pre-sorted array of
    //       keys, using the PointToBuffer() method.
    //----------------------------------------------------------------------------------
    class AnimationKeyArray
    {
    public:
        // Constructor and destructor
        AnimationKeyArray( DWORD dwValueDimension );
        ~AnimationKeyArray();

        // Initialization
        VOID        Resize( DWORD dwNewCapacity );
        VOID        PointToBuffer( FLOAT* pBuffer, DWORD dwBufferSizeBytes, DWORD dwKeyCount, DWORD dwValueDimension );

        // Data access
        DWORD       GetKeyCount() const { return m_dwSize; }
        DWORD       GetCapacity() const { return m_dwCapacity; }
        FLOAT       GetKeyTime( DWORD dwKeyIndex ) const
        {
            assert( dwKeyIndex < m_dwSize );
            DWORD dwIndex = dwKeyIndex * ( m_dwValueDimension + 1 );
            return m_pFloatData[ dwIndex ];
        }
        FLOAT*      GetKeyValue( DWORD dwKeyIndex )
        {
            assert( dwKeyIndex < m_dwSize );
            DWORD dwIndex = dwKeyIndex * ( m_dwValueDimension + 1 ) + 1;
            return &m_pFloatData[ dwIndex ];
        }
        const FLOAT*    GetKeyValue( DWORD dwKeyIndex ) const
        {
            assert( dwKeyIndex < m_dwSize );
            DWORD dwIndex = dwKeyIndex * ( m_dwValueDimension + 1 ) + 1;
            return &m_pFloatData[ dwIndex ];
        }
        XMVECTOR    GetKeyVector( DWORD dwKeyIndex ) const
        {
            assert( m_dwValueDimension > 1 && m_dwValueDimension < 5 );
            const FLOAT* pFloatData = GetKeyValue( dwKeyIndex );
            switch( m_dwValueDimension )
            {
            case 3:
                return XMLoadFloat3( (const XMFLOAT3*)pFloatData );
            case 4:
                return XMLoadFloat4( (const XMFLOAT4*)pFloatData );
            case 2:
                return XMLoadFloat2( (const XMFLOAT2*)pFloatData );
            }
            return XMVectorZero();
        }
        FLOAT       GetKeyScalar( DWORD dwKeyIndex )
        {
            assert( m_dwValueDimension == 1 );
            FLOAT* pFloatData = GetKeyValue( dwKeyIndex );
            return *pFloatData;
        }

        // Data update functions
        // Note: Ideally, these should not be executed at runtime, since they are a
        //       slow way of updating this structure.
        DWORD       AddKey();
        VOID        SetKeyTime( DWORD dwKeyIndex, FLOAT fTime ) const
        {
            assert( dwKeyIndex < m_dwSize );
            assert( fTime >= 0.0f );
            DWORD dwIndex = dwKeyIndex * ( m_dwValueDimension + 1 );
            m_pFloatData[ dwIndex ] = fTime;
        }
        VOID        SetKeyValue( DWORD dwKeyIndex, XMVECTOR vValue );
        VOID        SortKeys();

        // Key search and sampling
        DWORD       FindKey( FLOAT fTime, DWORD dwKeyIndexHint, BOOL bPlayForward ) const;
        XMVECTOR    SampleVector( FLOAT fTime, DWORD* pKeyIndexHint, BOOL bPlayForward ) const;
        XMVECTOR    SampleVectorLooping( FLOAT fTime, FLOAT fDuration, DWORD* pKeyIndexHint, BOOL bPlayForward ) const;

    protected:
        DWORD       GetKeyTimeCode( DWORD dwKeyIndex ) const
        {
            assert( dwKeyIndex < m_dwSize );
            DWORD dwIndex = dwKeyIndex * ( m_dwValueDimension + 1 );
            return *(DWORD*)&m_pFloatData[ dwIndex ];            
        }
    protected:
        FLOAT*      m_pFloatData;
        DWORD       m_dwValueDimension;
        DWORD       m_dwCapacity;
        DWORD       m_dwSize;
    };

    
    //----------------------------------------------------------------------------------
    // Name: class AnimationTransformTrack
    // Desc: An animation track that contains a 3D position curve, a 4D quaternion
    //       orientation curve, and a 3D scaling curve, all stored as keyframed
    //       sequences.
    //----------------------------------------------------------------------------------
    class AnimationTransformTrack : public NamedTypedObject
    {
        DEFINE_TYPE_INFO();

    public:
        AnimationTransformTrack()
            : m_PositionKeys( 3 ),
              m_OrientationKeys( 4 ),
              m_ScaleKeys( 3 )
        {
        }

        AnimationKeyArray& GetPositionKeys() { return m_PositionKeys; }
        AnimationKeyArray& GetOrientationKeys() { return m_OrientationKeys; }
        AnimationKeyArray& GetScaleKeys() { return m_ScaleKeys; }

        XMVECTOR SamplePosition( FLOAT fTime, DWORD* pLastKeyIndex = NULL, BOOL bPlayingForward = TRUE ) const
        {
            if( m_PositionKeys.GetKeyCount() == 0 )
                return XMVectorZero();
            return m_PositionKeys.SampleVector( fTime, pLastKeyIndex, bPlayingForward );
        }
        XMVECTOR SampleOrientation( FLOAT fTime, DWORD* pLastKeyIndex = NULL, BOOL bPlayingForward = TRUE ) const
        {
            if( m_OrientationKeys.GetKeyCount() == 0 )
                return XMQuaternionIdentity();
            return m_OrientationKeys.SampleVector( fTime, pLastKeyIndex, bPlayingForward );
        }
        XMVECTOR SampleScale( FLOAT fTime, DWORD* pLastKeyIndex = NULL, BOOL bPlayingForward = TRUE ) const
        {
            if( m_ScaleKeys.GetKeyCount() == 0 )
                return XMVectorSet( 1, 1, 1, 1 );
            return m_ScaleKeys.SampleVector( fTime, pLastKeyIndex, bPlayingForward );
        }

        XMVECTOR SamplePositionLooping( FLOAT fTime, FLOAT fDuration, DWORD* pLastKeyIndex = NULL, BOOL bPlayingForward = TRUE ) const
        {
            if( m_PositionKeys.GetKeyCount() == 0 )
                return XMVectorZero();
            return m_PositionKeys.SampleVectorLooping( fTime, fDuration, pLastKeyIndex, bPlayingForward );
        }
        XMVECTOR SampleOrientationLooping( FLOAT fTime, FLOAT fDuration, DWORD* pLastKeyIndex = NULL, BOOL bPlayingForward = TRUE ) const
        {
            if( m_OrientationKeys.GetKeyCount() == 0 )
                return XMQuaternionIdentity();
            return m_OrientationKeys.SampleVectorLooping( fTime, fDuration, pLastKeyIndex, bPlayingForward );
        }
        XMVECTOR SampleScaleLooping( FLOAT fTime, FLOAT fDuration, DWORD* pLastKeyIndex = NULL, BOOL bPlayingForward = TRUE ) const
        {
            if( m_ScaleKeys.GetKeyCount() == 0 )
                return XMVectorSet( 1, 1, 1, 1 );
            return m_ScaleKeys.SampleVectorLooping( fTime, fDuration, pLastKeyIndex, bPlayingForward );
        }

    protected:
        AnimationKeyArray   m_PositionKeys;
        AnimationKeyArray   m_OrientationKeys;
        AnimationKeyArray   m_ScaleKeys;
    };

    typedef std::vector<AnimationTransformTrack*> AnimationTransformTrackVector;


    //----------------------------------------------------------------------------------
    // Name: class Animation
    // Desc: A graphics resource that contains a list of animation tracks, as well as
    //       a duration for the animation.
    //----------------------------------------------------------------------------------
    class Animation : public Resource
    {
        DEFINE_TYPE_INFO();

    public:
        Animation()
            : m_fDuration( 0.0f )
        {
        }
        ~Animation();

        VOID SetDuration( FLOAT fDuration ) { m_fDuration = fDuration; }
        FLOAT GetDuration() const { return m_fDuration; }

        VOID AddAnimationTrack( AnimationTransformTrack* pTrack ) { m_Tracks.push_back( pTrack ); }
        DWORD GetAnimationTrackCount() const { return (DWORD)m_Tracks.size(); }
        AnimationTransformTrack* GetAnimationTrack( DWORD dwIndex ) { return m_Tracks[dwIndex]; }

    protected:
        AnimationTransformTrackVector   m_Tracks;
        FLOAT                           m_fDuration;
    };

} // namespace ATG

#endif // ATG_ANIMATION_H