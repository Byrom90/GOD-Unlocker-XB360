//-----------------------------------------------------------------------------
// AtgFrame.h
//
// Describes a frame in the scene- a frame is a named typed object
// with a set of bounds and part of a transform heirarchy
//
// $OPTIMIZE: Pool Allocate
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_FRAME_H
#define ATG_FRAME_H

#include "AtgNamedTypedObject.h"
#include "AtgBound.h"

namespace ATG
{

//-----------------------------------------------------------------------------
// Name: Frame
// Desc: An named typed object with a frame of reference and a bound
//       Note: X is Right, Y is Up, and Z is Forward
//-----------------------------------------------------------------------------
class Frame : public NamedTypedObject
{
    DEFINE_TYPE_INFO();

public:
    Frame();
    Frame( const StringID& Name, CXMMATRIX matLocalTransform );
    ~Frame();

    // frame hierarchy
    VOID                        AddChild( Frame* pChild );
    VOID                        SetParent( Frame* pParent );  // Set to NULL to unparent
    
    Frame*                      GetFirstChild() CONST { return m_pFirstChild; }
    Frame*                      GetNextSibling() CONST { return m_pNextSibling; }
    Frame*                      GetParent() CONST { return m_pParent; }
    BOOL                        IsAncestor( Frame* pOtherFrame );
    BOOL                        IsChild( Frame* pOtherFrame );

    // cloned object support
    VOID                        DisconnectFromParent();    
        
    // local position
    XMVECTOR                    GetLocalPosition() CONST { return m_LocalTransform.r[3]; }
    VOID                        SetLocalPosition( CONST XMVECTOR& NewPosition ) { SetCachedWorldTransformDirty(); m_LocalTransform.r[3] = NewPosition; }
   
    // world position
    XMVECTOR                    GetWorldPosition();
    VOID                        SetWorldPosition( CONST XMVECTOR& NewPosition );

    // local transform
    const XMMATRIX&             GetLocalTransform() CONST { return m_LocalTransform; }
    VOID                        SetLocalTransform( CONST XMMATRIX& LocalTransform ) { SetCachedWorldTransformDirty(); m_LocalTransform = LocalTransform; }

    // world transform
    const XMMATRIX&             GetWorldTransform();
    VOID                        SetWorldTransform( CONST XMMATRIX& WorldTransform );

    // basis vectors
    XMVECTOR                    GetLocalRight() CONST { return m_LocalTransform.r[0]; }
    XMVECTOR                    GetLocalUp() CONST { return m_LocalTransform.r[1]; }
    XMVECTOR                    GetLocalDirection() CONST { return m_LocalTransform.r[2]; }      

    XMVECTOR                    GetWorldRight();
    XMVECTOR                    GetWorldUp();
    XMVECTOR                    GetWorldDirection();

    // bound    
    VOID                        SetLocalBound( const Bound& bound ) { m_LocalBound = bound; SetCachedWorldTransformDirty(); }
    CONST Bound&                GetLocalBound() CONST { return m_LocalBound; }
    CONST Bound&                GetWorldBound() { UpdateCachedWorldTransformIfNeeded(); return m_CachedWorldBound; }

private:
    VOID                        UpdateCachedWorldTransformIfNeeded();
    VOID                        SetCachedWorldTransformDirty();
    // Set the first element of m_CachedWorldTransform to a magic value to indicate that the cached
    // transform is invalid.
    VOID                        SetCachedWorldTransformDirtyLocal() { *(DWORD*)&m_CachedWorldTransform = 0xFFFFFFFF; }
    // An integer compare is used to check for the cached transforms validity because this avoids
    // the cost of doing a floating-point compare. Also, an integer compare allows the 0xFFFFFFFF
    // value to be used, which would evaluate to an uncomparable NAN as a float.
    // An integer comparison is faster than a floating-point comparison on this case because the value
    // to be compared is in memory, rather than in the floating-point registers.
    BOOL                        IsCachedWorldTransformDirty() const { return *(const DWORD*)&m_CachedWorldTransform == 0xFFFFFFFF; }

    XMMATRIX                    m_LocalTransform;
    XMMATRIX                    m_CachedWorldTransform;    

    Bound                       m_LocalBound;
    Bound                       m_CachedWorldBound;
    
    Frame*                      m_pParent;
    Frame*                      m_pNextSibling;
    Frame*                      m_pFirstChild;
};

} // namespace ATG

#endif // ATG_FRAME_H
