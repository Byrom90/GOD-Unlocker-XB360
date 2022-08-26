//-----------------------------------------------------------------------------
// AtgFrame.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgFrame.h"
#include <float.h>

namespace ATG
{

CONST StringID Frame::TypeID( L"Frame" );


//-----------------------------------------------------------------------------
// Name: Frame::Frame()
//-----------------------------------------------------------------------------
Frame::Frame()
{
    m_pParent = NULL;
    m_pNextSibling = NULL;
    m_pFirstChild = NULL;

    SetCachedWorldTransformDirty();
    m_LocalTransform = XMMatrixIdentity();
}


//-----------------------------------------------------------------------------
// Name: Frame::Frame()
//-----------------------------------------------------------------------------
Frame::Frame( const StringID& Name, CXMMATRIX matLocalTransform )
{
    m_pParent = NULL;
    m_pNextSibling = NULL;
    m_pFirstChild = NULL;

    SetCachedWorldTransformDirty();
    m_LocalTransform = matLocalTransform;
    SetName( Name );
}


//-----------------------------------------------------------------------------
// Name: Frame::~Frame()
//-----------------------------------------------------------------------------
Frame::~Frame()
{
    SetParent( NULL );
}


//-----------------------------------------------------------------------------
// Name: Frame::AddChild()
//-----------------------------------------------------------------------------
VOID Frame::AddChild( Frame* pChild )
{
    assert( pChild );
    pChild->SetParent( this );
}


//-----------------------------------------------------------------------------
// Name: Frame::SetParent()
//-----------------------------------------------------------------------------
VOID Frame::SetParent( Frame* pParent )
{
    Frame* pSrch, *pLast;

    UpdateCachedWorldTransformIfNeeded();

    if( m_pParent )
    {
        pLast = NULL;
        for( pSrch = m_pParent->m_pFirstChild; pSrch; pSrch = pSrch->m_pNextSibling )
        {
            if( pSrch == this )
                break;
            pLast = pSrch;
        }

        // If we can't find this frame in the old parent's list, assert
        assert( pSrch );

        // Remove us from the list
        if( pLast )
        {
            pLast->m_pNextSibling = m_pNextSibling;
        }
        else // at the beginning of the list
        {
            m_pParent->m_pFirstChild = m_pNextSibling;
        }

        m_pNextSibling = NULL;
        m_pParent = NULL;
    }

    // Add us to the new parent's list               
    if( pParent )
    {
        m_pParent = pParent;
        m_pNextSibling = pParent->m_pFirstChild;
        pParent->m_pFirstChild = this;
    }

    // now we update our local transform to match the old world transform
    // (i.e. move under our new parent's frame with no detectable change)

    XMMATRIX worldMatrix = m_CachedWorldTransform;
    SetWorldTransform( worldMatrix );
}


//-----------------------------------------------------------------------------
// Name: Frame::IsAncestor()
// Desc: Returns TRUE if the specified frame is an ancestor of this frame
//       ancestor = parent, parent->parent, etc.
//       Also returns TRUE if specified frame is NULL.
//-----------------------------------------------------------------------------
BOOL Frame::IsAncestor( Frame* pOtherFrame )
{
    if( pOtherFrame == NULL )
        return TRUE;
    if( pOtherFrame == this )
        return FALSE;
    Frame* pFrame = GetParent();
    while( pFrame != NULL )
    {
        if( pFrame == pOtherFrame )
            return TRUE;
        pFrame = pFrame->GetParent();
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Frame::IsChild()
// Desc: Returns TRUE if the specified frame is a direct child of this frame
//-----------------------------------------------------------------------------
BOOL Frame::IsChild( Frame* pOtherFrame )
{
    if( pOtherFrame == NULL )
        return FALSE;
    Frame* pChild = GetFirstChild();
    while( pChild != NULL )
    {
        if( pChild == pOtherFrame )
            return TRUE;
        pChild = pChild->GetNextSibling();
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: Frame::GetWorldPosition()
//-----------------------------------------------------------------------------
XMVECTOR Frame::GetWorldPosition()
{
    UpdateCachedWorldTransformIfNeeded();
    return m_CachedWorldTransform.r[3];
}


//-----------------------------------------------------------------------------
// Name: Frame::SetWorldPosition()
//-----------------------------------------------------------------------------
VOID Frame::SetWorldPosition( CONST XMVECTOR &NewPosition )
 {
    UpdateCachedWorldTransformIfNeeded();
    XMMATRIX ModifiedWorldTransform = m_CachedWorldTransform;

    ModifiedWorldTransform.r[3] = NewPosition;

    SetWorldTransform( ModifiedWorldTransform );
}


//-----------------------------------------------------------------------------
// Name: Frame::GetWorldTransform()
//-----------------------------------------------------------------------------
const XMMATRIX& Frame::GetWorldTransform()
{
    UpdateCachedWorldTransformIfNeeded();
    return m_CachedWorldTransform;
}

//-----------------------------------------------------------------------------
// Name: Frame::GetWorldRight()
//-----------------------------------------------------------------------------
XMVECTOR Frame::GetWorldRight()
{
    UpdateCachedWorldTransformIfNeeded();
    return m_CachedWorldTransform.r[0];
}


//-----------------------------------------------------------------------------
// Name: Frame::GetWorldUp()
//-----------------------------------------------------------------------------
XMVECTOR Frame::GetWorldUp()
{
    UpdateCachedWorldTransformIfNeeded();
    return m_CachedWorldTransform.r[1];
}


//-----------------------------------------------------------------------------
// Name: Frame::GetWorldDirection()
//-----------------------------------------------------------------------------
XMVECTOR Frame::GetWorldDirection()
{
    UpdateCachedWorldTransformIfNeeded();
    return m_CachedWorldTransform.r[2];
}


//-----------------------------------------------------------------------------
// Name: Frame::SetWorldTransform()
//-----------------------------------------------------------------------------
VOID Frame::SetWorldTransform( CONST XMMATRIX& WorldTransform )
{
    XMMATRIX ParentInverse;
    XMVECTOR vDeterminant;

    if ( m_pParent )
    {    
        m_pParent->UpdateCachedWorldTransformIfNeeded();
        ParentInverse = XMMatrixInverse( &vDeterminant, m_pParent->m_CachedWorldTransform );
        
        // Parent's matrix should be invertible
        assert( XMVectorGetX( vDeterminant ) != 0.0f );
        
        m_LocalTransform = XMMatrixMultiply( WorldTransform, ParentInverse );        
    }
    else
    {
        m_LocalTransform = WorldTransform;    
    }

    SetCachedWorldTransformDirty();
}


//-----------------------------------------------------------------------------
// Name: Frame::UpdateCachedWorldTransformIfNeeded()
//-----------------------------------------------------------------------------
VOID Frame::UpdateCachedWorldTransformIfNeeded()
{
    if( IsCachedWorldTransformDirty() )
    {
        if( m_pParent )
        {
            m_pParent->UpdateCachedWorldTransformIfNeeded();
            m_CachedWorldTransform = XMMatrixMultiply( m_LocalTransform, m_pParent->m_CachedWorldTransform );
            m_CachedWorldBound = m_LocalBound * m_CachedWorldTransform;
        }
        else
        {
            m_CachedWorldTransform = m_LocalTransform;
            m_CachedWorldBound = m_LocalBound * m_CachedWorldTransform;
        }
    }
}


//-----------------------------------------------------------------------------
// Name: Frame::SetCachedWorldTransformDirty()
//-----------------------------------------------------------------------------
VOID Frame::SetCachedWorldTransformDirty()
{
    Frame* pChild;
    SetCachedWorldTransformDirtyLocal();

    for( pChild = m_pFirstChild; pChild; pChild = pChild->m_pNextSibling )
    {
        pChild->SetCachedWorldTransformDirty();
    }
}


//-----------------------------------------------------------------------------
// Name: Frame::DisconnectFromParent
// Desc: Removes parent link from the object, and converts world transform
//       to local transform.  This method is only to be used on clones of
//       scene objects, since the parent is not notified of the disconnection.
//-----------------------------------------------------------------------------
VOID Frame::DisconnectFromParent()
{
    // Debug check to make sure the parent really isn't the parent of this clone
#ifdef _DEBUG
    if( m_pParent != NULL )
    {
        assert( !m_pParent->IsChild( this ) );
    }
#endif

    UpdateCachedWorldTransformIfNeeded();
    m_pParent = NULL;
    SetWorldTransform( m_CachedWorldTransform );
}

} // namespace ATG
