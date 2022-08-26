//-----------------------------------------------------------------------------
// AtgAnimation.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <vector>
#include <algorithm>
#include "AtgAnimation.h"
#include "AtgScene.h"

namespace ATG
{
    const StringID AnimationTransformTrack::TypeID( L"AnimationTransformTrack" );
    const StringID Animation::TypeID( L"Animation" );

#define KEY_SEARCH_DISTANCE     4

    //----------------------------------------------------------------------------------
    // Name: AnimationKeyArray constructor
    // Desc: Initializes an AnimationKeyArray class to null values, and verifies the
    //       dimension is valid.
    //----------------------------------------------------------------------------------
    AnimationKeyArray::AnimationKeyArray( DWORD dwValueDimension )
        : m_dwValueDimension( dwValueDimension ),
          m_pFloatData( NULL ),
          m_dwSize( 0 ),
          m_dwCapacity( 0 )
    {
        assert( m_dwValueDimension > 0 );
    }


    //----------------------------------------------------------------------------------
    // Name: AnimationKeyArray destructor
    // Desc: Cleans up the memory allocations.
    //----------------------------------------------------------------------------------
    AnimationKeyArray::~AnimationKeyArray()
    {
        delete[] m_pFloatData;
        m_pFloatData = NULL;
        m_dwCapacity = 0;
        m_dwSize = 0;
        m_dwValueDimension = 0;
    }


    //----------------------------------------------------------------------------------
    // Name: Resize()
    // Desc: Increases the size of the key storage buffer.  Ideally, your initialization
    //       code should call this method once with the total number of keys, so no
    //       further resizes will be necessary.
    //----------------------------------------------------------------------------------
    VOID AnimationKeyArray::Resize( DWORD dwNewCapacity )
    {
        assert( dwNewCapacity > m_dwCapacity );
        assert( m_dwValueDimension > 0 );

        // Allocate and clear new buffer.
        DWORD dwFloatCount = ( m_dwValueDimension + 1 ) * dwNewCapacity;
        FLOAT* pNewBuffer = new FLOAT[ dwFloatCount ];
        ZeroMemory( pNewBuffer, dwFloatCount * sizeof( FLOAT ) );

        // Copy existing contents to new buffer.
        if( m_pFloatData != NULL )
        {
            DWORD dwExistingSize = m_dwSize * ( m_dwValueDimension + 1 ) * sizeof( FLOAT );
            XMemCpy( pNewBuffer, m_pFloatData, dwExistingSize );
            delete[] m_pFloatData;
            m_pFloatData = NULL;
        }

        // Update buffer pointer and capacity.
        m_pFloatData = pNewBuffer;
        m_dwCapacity = dwNewCapacity;
    }


    //----------------------------------------------------------------------------------
    // Name: PointToBuffer()
    // Desc: Initializes the key array with a pointer to a pre-allocated key array.
    //       This is the most preferred method of initialization if you already have the
    //       key data in memory, such as from a resource loader.
    //----------------------------------------------------------------------------------
    VOID AnimationKeyArray::PointToBuffer( FLOAT* pBuffer, DWORD dwBufferSizeBytes, DWORD dwKeyCount, DWORD dwValueDimension )
    {
        // Sanity checks.
        assert( pBuffer != NULL );
        assert( dwBufferSizeBytes == ( dwKeyCount * ( dwValueDimension + 1 ) * sizeof( FLOAT ) ) );

        // Destroy any existing buffer.
        if( m_pFloatData != NULL )
            delete[] m_pFloatData;

        // Initialize with new buffer.
        m_pFloatData = pBuffer;
        m_dwCapacity = dwKeyCount;
        m_dwSize = dwKeyCount;
        m_dwValueDimension = dwValueDimension;
    }


    //----------------------------------------------------------------------------------
    // Name: AddKey()
    // Desc: Increases the size member by one.  If there is not enough capacity to
    //       accomodate the new key, the capacity is increased.
    //       The index of the new key is returned.
    //       This is not an optimal way to fill this structure, but it is convenient
    //       for serialized loading.
    //----------------------------------------------------------------------------------
    DWORD AnimationKeyArray::AddKey()
    {
        assert( m_dwSize <= m_dwCapacity );
        if( m_dwSize == m_dwCapacity )
        {
            DWORD dwNewSize = m_dwCapacity + ( m_dwCapacity >> 1 );
            dwNewSize = max( 4, dwNewSize );
            Resize( dwNewSize );
        }
        DWORD dwResult = m_dwSize;
        ++m_dwSize;
        return dwResult;
    }


    //----------------------------------------------------------------------------------
    // Name: SetKeyValue()
    // Desc: Changes a key's value vector to a new value.
    //----------------------------------------------------------------------------------
    VOID AnimationKeyArray::SetKeyValue( DWORD dwKeyIndex, XMVECTOR vValue )
    {
        FLOAT* pValueData = GetKeyValue( dwKeyIndex );
        switch( m_dwValueDimension )
        {
        case 3:
            XMStoreFloat3( (XMFLOAT3*)pValueData, vValue );
            return;
        case 4:
            XMStoreFloat4( (XMFLOAT4*)pValueData, vValue );
            return;
        case 2:
            XMStoreFloat2( (XMFLOAT2*)pValueData, vValue );
            return;
        }
    }

    struct KeyTempStruct
    {
        FLOAT       fTime;
        DWORD       dwOriginalIndex;
    };


    //----------------------------------------------------------------------------------
    // Name: KeyTimeSortFunction
    // Desc: Helper function for the SortKeys() method, used with a STL sort.
    //----------------------------------------------------------------------------------
    BOOL KeyTimeSortFunction( const KeyTempStruct& A, const KeyTempStruct& B )
    {
        return A.fTime <= B.fTime;
    }


    //----------------------------------------------------------------------------------
    // Name: SortKeys()
    // Desc: Sorts the keys by time, in ascending order.  This should not be used
    //       unless the keys were loaded in a serial fashion, and sort order is not
    //       guaranteed.  If this class is initialized using PointToBuffer(), and the
    //       data is already sorted, you do not need to sort again.
    //----------------------------------------------------------------------------------
    VOID AnimationKeyArray::SortKeys()
    {
        if( m_dwSize == 0 )
            return;
        typedef std::vector<KeyTempStruct> KeyTempList;
        KeyTempList SortingList;
        SortingList.reserve( m_dwSize );
        for( DWORD i = 0; i < m_dwSize; i++ )
        {
            KeyTempStruct ts;
            ts.fTime = GetKeyTime( i );
            ts.dwOriginalIndex = i;
            SortingList.push_back( ts );
        }
        std::sort( SortingList.begin(), SortingList.end(), KeyTimeSortFunction );

        DWORD dwKeySizeBytes = ( m_dwValueDimension + 1 ) * sizeof( FLOAT );
        DWORD dwTotalSizeBytes = dwKeySizeBytes * m_dwSize;
        BYTE* pSortedKeys = new BYTE[ dwTotalSizeBytes ];
        BYTE* pCurrentSortedKey = pSortedKeys;
        for( DWORD i = 0; i < SortingList.size(); i++ )
        {
            KeyTempStruct& ts = SortingList[i];
            BYTE* pOriginalKey = (BYTE*)m_pFloatData + dwKeySizeBytes * ts.dwOriginalIndex;
            XMemCpy( pCurrentSortedKey, pOriginalKey, dwKeySizeBytes );
            pCurrentSortedKey += dwKeySizeBytes;
        }
        XMemCpy( m_pFloatData, pSortedKeys, dwTotalSizeBytes );
        delete[] pSortedKeys;
    }


    //----------------------------------------------------------------------------------
    // Name: FindKey()
    // Desc: Finds the key corresponding to the given time, using a starting hint and
    //       a preferred search direction.  This method is optimized to avoid float
    //       branches, by using a "time code" for each key and the search parameter.
    //       The time code is nothing more than a non-negative float treated as a DWORD.
    //       Non-negative floats treated as ints can be compared and sorted like ints.
    //----------------------------------------------------------------------------------
    DWORD AnimationKeyArray::FindKey( FLOAT fTime, DWORD dwKeyIndexHint, BOOL bPlayForward ) const
    {
        // Input checking.
        assert( fTime >= 0.0f );
        assert( m_dwSize > 0 );
        assert( dwKeyIndexHint < m_dwSize );

        // Timecode is an integer re-expression of the float time, used to compare times 
        // faster. Branching and comparing on float values is expensive on Xbox 360.
        DWORD dwTimeCode = *(DWORD*)&fTime;

        // Check if the time is less than the first key.
        if( dwTimeCode < GetKeyTimeCode( 0 ) )
            return 0;

        // Quick search a few keys, in a linear fashion with wraparound.
        INT iIncrement = -1 + ( (INT)( bPlayForward & 1 ) << 1 );
        for( INT i = 0; i < KEY_SEARCH_DISTANCE; i++ )
        {
            DWORD dwIndex = (DWORD)( (INT)( dwKeyIndexHint + m_dwSize ) + ( iIncrement * i ) ) % m_dwSize;
            DWORD dwTimeCodeTest = GetKeyTimeCode( dwIndex );
            // Check if we're looking at the last key.
            if( dwIndex == ( m_dwSize - 1 ) )
            {
                // If the time code is after the last key, the last key is the right key.
                if( dwTimeCode >= dwTimeCodeTest )
                    return dwIndex;
            }
            else
            {
                // If the time code comes before the key we're looking at, go to the next
                // key.
                if( dwTimeCode < dwTimeCodeTest )
                    continue;
                // If the time code comes before the next key, then we are on the right key.
                DWORD dwTimeCodeTestNext = GetKeyTimeCode( dwIndex + 1 );
                if( dwTimeCode < dwTimeCodeTestNext )
                    return dwIndex;
            }
        }

        // We didn't find the right key in the quick search.
        // Perform a binary search.
        DWORD dwStartIndex = 0;
        DWORD dwEndIndex = m_dwSize - 1;
        while( dwEndIndex > dwStartIndex )
        {
            // Get the middle index in the range, biased upwards.
            DWORD dwTestIndex = ( dwStartIndex + dwEndIndex + 1 ) >> 1;
            DWORD dwTestTimeCode = GetKeyTimeCode( dwTestIndex );
            if( dwTestTimeCode < dwTimeCode )
                dwStartIndex = dwTestIndex;
            else if( dwTestTimeCode > dwTimeCode )
                dwEndIndex = dwTestIndex - 1;
            else
                return dwTestIndex;
        }
        return dwStartIndex;
    }


    //----------------------------------------------------------------------------------
    // Name: SampleVector()
    // Desc: Linearly interpolates between neighboring keys using a time parameter.
    //       The user of this method should store a DWORD key index hint, which will be
    //       updated each time a search is performed.  The hint reduces most searches
    //       to O(1), and when the hint fails, the performance of the search is
    //       O(log n).
    //       If the search time falls outside the key times, the result will be either
    //       the first key's value or the last key's value.
    //----------------------------------------------------------------------------------
    XMVECTOR AnimationKeyArray::SampleVector( FLOAT fTime, DWORD* pKeyIndexHint, BOOL bPlayForward ) const
    {
        // If we were given a hint, use it.
        DWORD dwKeyIndexHint = 0;
        if( pKeyIndexHint != NULL )
            dwKeyIndexHint = *pKeyIndexHint;

        // Find the key.
        DWORD dwKeyIndex = FindKey( fTime, dwKeyIndexHint, bPlayForward );

        // Update the hint so it can be used next time.
        if( pKeyIndexHint != NULL )
            *pKeyIndexHint = dwKeyIndex;

        // Check if the time is before the first key or after the last key.
        // In this case, we do not need to lerp.
        DWORD dwTimeCode = *(DWORD*)&fTime;
        if( ( dwKeyIndex == m_dwSize - 1 ) ||
            ( dwKeyIndex == 0 && dwTimeCode < GetKeyTimeCode( dwKeyIndex ) ) )
        {
            return GetKeyVector( dwKeyIndex );
        }

        // Extract the start and end times and values.
        FLOAT fStartTime = GetKeyTime( dwKeyIndex );
        XMVECTOR vStart = GetKeyVector( dwKeyIndex );
        FLOAT fEndTime = GetKeyTime( dwKeyIndex + 1 );
        XMVECTOR vEnd = GetKeyVector( dwKeyIndex + 1 );

        // Perform the lerp.
        FLOAT fParam = ( fTime - fStartTime ) / ( fEndTime - fStartTime );
        return XMVectorLerp( vStart, vEnd, fParam );
    }



    //----------------------------------------------------------------------------------
    // Name: SampleVectorLooping()
    // Desc: Linearly interpolates between neighboring keys using a time parameter.
    //       The user of this method should store a DWORD key index hint, which will be
    //       updated each time a search is performed.  The hint reduces most searches
    //       to O(1), and when the hint fails, the performance of the search is
    //       O(log n).
    //       If the search time falls outside the key times, the result will be either
    //       the first key's value or the last key's value.
    //----------------------------------------------------------------------------------
    XMVECTOR AnimationKeyArray::SampleVectorLooping( FLOAT fTime, FLOAT fDuration, DWORD* pKeyIndexHint, BOOL bPlayForward ) const
    {
        // If we were given a hint, use it.
        DWORD dwKeyIndexHint = 0;
        if( pKeyIndexHint != NULL )
            dwKeyIndexHint = *pKeyIndexHint;

        // Find the key.
        DWORD dwKeyIndex = FindKey( fTime, dwKeyIndexHint, bPlayForward );

        // Update the hint so it can be used next time.
        if( pKeyIndexHint != NULL )
            *pKeyIndexHint = dwKeyIndex;

        // Check if the time is before the first key or after the last key.
        // In this case, we do not need to lerp.
        DWORD dwTimeCode = *(DWORD*)&fTime;
        FLOAT fStartTime, fEndTime;
        XMVECTOR vStart, vEnd;

        if( dwKeyIndex == 0 && dwTimeCode < GetKeyTimeCode( dwKeyIndex ) )
        {
            fStartTime = GetKeyTime( m_dwSize - 1 ) - fDuration;
            vStart = GetKeyVector( m_dwSize - 1 );
            fEndTime = GetKeyTime( dwKeyIndex );
            vEnd = GetKeyVector( dwKeyIndex );
        }
        else if( dwKeyIndex == m_dwSize - 1 )
        {
            fStartTime = GetKeyTime( dwKeyIndex );
            vStart = GetKeyVector( dwKeyIndex );
            fEndTime = GetKeyTime( 0 ) + fDuration;
            vEnd = GetKeyVector( 0 );
        }
        else
        {
            // Extract the start and end times and values.
            fStartTime = GetKeyTime( dwKeyIndex );
            vStart = GetKeyVector( dwKeyIndex );
            fEndTime = GetKeyTime( dwKeyIndex + 1 );
            vEnd = GetKeyVector( dwKeyIndex + 1 );
        }

        // Perform the lerp.
        FLOAT fParam = ( fTime - fStartTime ) / ( fEndTime - fStartTime );
        return XMVectorLerp( vStart, vEnd, fParam );
    }

} // namespace ATG
