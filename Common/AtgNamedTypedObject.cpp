//-----------------------------------------------------------------------------
// AtgNamedTypedObject.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgNamedTypedObject.h"

namespace ATG
{

CONST WCHAR*             StringID::s_EmptyString = L"";
CONST StringID           NamedTypedObject::TypeID( L"NamedTypedObject" );

//-----------------------------------------------------------------------------
// Name: StringID::GetStringTable
// Desc: returns static string table data- used to ensure initialization is done
//-----------------------------------------------------------------------------
std::list<CONST WCHAR *>* StringID::GetStringTable() 
{
    static std::list<CONST WCHAR *> s_StringLists[ StringID_HASHSIZE ];  
    return s_StringLists;
}

//-----------------------------------------------------------------------------
// Name: StringID::operator==
// Desc: compare a string with a WCHAR 
//-----------------------------------------------------------------------------
BOOL StringID::operator== ( CONST WCHAR* strRHS ) CONST
{
    if( strRHS == NULL )
    {
        if( m_strString == s_EmptyString )
            return TRUE;
        return FALSE;
    }

    if( m_strString == strRHS )
        return TRUE;

    return ( wcscmp( m_strString, strRHS ) == 0 );
}

//-----------------------------------------------------------------------------
// Name: StringID::AddString 
// Desc: Add a string to the string table
//-----------------------------------------------------------------------------
CONST WCHAR* StringID::AddString( CONST WCHAR* strString )
{
    if( strString == NULL )
        return NULL;
    if( strString[0] == NULL )
        return s_EmptyString;

    int uBucketIndex = HashString( strString ) % StringID_HASHSIZE;
    std::list<CONST WCHAR*>& CurrentList = GetStringTable()[ uBucketIndex ];

    std::list<CONST WCHAR*>::iterator iter = CurrentList.begin();
    std::list<CONST WCHAR*>::iterator end = CurrentList.end();

    while( iter != end )
    {
        CONST WCHAR* strTest = *iter;
        if( wcscmp( strTest, strString ) == 0 )
            return strTest;
        ++iter;
    }
    
    // $OPTIMIZE: use a fixed size allocator here
    DWORD bufferLength = wcslen( strString ) + 1;
    WCHAR* strCopy = new WCHAR[ bufferLength ];
    wcscpy_s( strCopy, bufferLength, strString );
    CurrentList.push_back( strCopy );
    return strCopy;
}


//-----------------------------------------------------------------------------
// Name: StringID::HashString
// Desc: Create a hash value from a string
//-----------------------------------------------------------------------------
DWORD StringID::HashString( CONST WCHAR* strString )
{
    DWORD HashVal = 0;        
    CONST WCHAR *pChar;

    for ( pChar = strString; *pChar; pChar++ )
    {
        HashVal += *pChar * 193951;
        HashVal *= 399283;
    }
    return HashVal;
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::iterator::iterator()
//-----------------------------------------------------------------------------
NameIndexedCollection::iterator::iterator()
{
    m_pCollection = NULL;    
    m_iCurBucket = 0;
}

//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::iterator::operator*()
// Desc: iterator dereference
//-----------------------------------------------------------------------------
NamedTypedObject* NameIndexedCollection::iterator::operator*()
{
    return *m_iter;
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::iterator::operator=()
// Desc: iterator assignment operator
//-----------------------------------------------------------------------------
NameIndexedCollection::iterator& 
NameIndexedCollection::iterator::operator=( CONST NameIndexedCollection::iterator& iRHS)
{
    m_pCollection = iRHS.m_pCollection;
    m_iter = iRHS.m_iter;
    m_iCurBucket = iRHS.m_iCurBucket;
    return (*this);
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::iterator::operator==()
// Desc: iterator comparison operator
//-----------------------------------------------------------------------------
BOOL NameIndexedCollection::iterator::operator==( CONST NameIndexedCollection::iterator& iRHS)
{
    if ( ( m_pCollection == iRHS.m_pCollection ) &&
         ( m_iCurBucket == iRHS.m_iCurBucket ) && 
         ( m_iter == iRHS.m_iter ) )   
        return TRUE;
    return FALSE;
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::iterator::operator!=()
// Desc: iterator comparison operator
//-----------------------------------------------------------------------------
BOOL NameIndexedCollection::iterator::operator!=( CONST NameIndexedCollection::iterator& iRHS)
{
    if ( ( m_pCollection == iRHS.m_pCollection ) &&
         ( m_iCurBucket == iRHS.m_iCurBucket ) && 
         ( m_iter == iRHS.m_iter ) )    
        return FALSE;
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::iterator::operator++()
// Desc: iterator increment
//-----------------------------------------------------------------------------
NameIndexedCollection::iterator& 
NameIndexedCollection::iterator::operator++( int )
{    
    assert( m_pCollection );

    // increment the iterator
    m_iter++;    

    // if we are at the end of a bucket, move to the next bucket
    while ( ( m_iter == m_pCollection->s_Lists[ m_iCurBucket ].end() ) &&
            ( m_iCurBucket < DEFAULT_COLLECTION_HASHSIZE - 1 ) )
    {        
        m_iCurBucket++;        
        m_iter = m_pCollection->s_Lists[ m_iCurBucket ].begin();
    }
        
    return (*this);
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::Add()
//-----------------------------------------------------------------------------
VOID NameIndexedCollection::Add( NamedTypedObject *pObjectToAdd )
{
    assert( pObjectToAdd );
    
    int iBucket = StringID::HashString( pObjectToAdd->GetName() ) % DEFAULT_COLLECTION_HASHSIZE;
    
    // $OPTIMIZE: use a fixed size allocator here
    s_Lists[ iBucket ].push_back( pObjectToAdd );            
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::Remove()
//-----------------------------------------------------------------------------
VOID NameIndexedCollection::Remove( NamedTypedObject *pObjectToRemove )
{
    assert( pObjectToRemove );
    
    int iBucket = StringID::HashString( pObjectToRemove->GetName() ) % DEFAULT_COLLECTION_HASHSIZE;
    s_Lists[ iBucket ].remove( pObjectToRemove );            
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::Find()
//-----------------------------------------------------------------------------
NamedTypedObject* NameIndexedCollection::Find( CONST WCHAR* strName )
{
    std::list<NamedTypedObject *>::iterator i;
    
    int iBucket = StringID::HashString( strName ) % DEFAULT_COLLECTION_HASHSIZE;    

    for ( i = s_Lists[ iBucket ].begin(); i != s_Lists[ iBucket ].end(); ++i )
    {
        if ( (*i)->GetName() == strName )
            break;
    }
    
    if ( i == s_Lists[ iBucket ].end() )
        return NULL;
    
    return (*i);
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::FindTyped()
//-----------------------------------------------------------------------------
NamedTypedObject* NameIndexedCollection::FindTyped( CONST WCHAR* strName, const StringID TypeID )
{
    std::list<NamedTypedObject *>::iterator i;

    int iBucket = StringID::HashString( strName ) % DEFAULT_COLLECTION_HASHSIZE;    

    for ( i = s_Lists[ iBucket ].begin(); i != s_Lists[ iBucket ].end(); ++i )
    {
        NamedTypedObject* pNTO = (*i);
        if ( pNTO->IsDerivedFrom( TypeID ) && pNTO->GetName() == strName )
            break;
    }

    if ( i == s_Lists[ iBucket ].end() )
        return NULL;

    return (*i);
}


DWORD NameIndexedCollection::Size()
{
    DWORD dwSize = 0;
    for( DWORD i = 0; i < DEFAULT_COLLECTION_HASHSIZE; i++ )
    {
        dwSize += s_Lists[i].size();
    }
    return dwSize;
}


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection::begin()
//-----------------------------------------------------------------------------
NameIndexedCollection::iterator NameIndexedCollection::begin()
{
    NameIndexedCollection::iterator rtn;

    rtn.m_pCollection = this;
    rtn.m_iCurBucket = 0;
    rtn.m_iter = s_Lists[ rtn.m_iCurBucket ].begin();
    
    // if we are at the end of a bucket, move to the next bucket
    while ( ( rtn.m_iter == s_Lists[ rtn.m_iCurBucket ].end() ) &&
            ( rtn.m_iCurBucket != DEFAULT_COLLECTION_HASHSIZE - 1 ) )
    {        
        rtn.m_iCurBucket++;
        rtn.m_iter = s_Lists[ rtn.m_iCurBucket ].begin();
    }
        
    return rtn;
}


//----------------------------------------------------------------------------
// Name: NameIndexedCollection::end()
//-----------------------------------------------------------------------------
NameIndexedCollection::iterator NameIndexedCollection::end()
{
    NameIndexedCollection::iterator rtn;

    rtn.m_pCollection = this;
    rtn.m_iCurBucket = DEFAULT_COLLECTION_HASHSIZE - 1;
    rtn.m_iter = s_Lists[ DEFAULT_COLLECTION_HASHSIZE - 1].end();    
    
    return rtn;
}

} // namespace ATG
