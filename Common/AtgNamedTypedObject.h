//-----------------------------------------------------------------------------
// ATGNamedTypedObject.h
//
// Base class for all scene objects that are named and typed
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_NamedTypedObject_H
#define ATG_NamedTypedObject_H

// C4127: conditional expression is constant
// this shows up when using STL without exception handling
#pragma warning(disable:4127)

#include <list>

namespace ATG
{

//-----------------------------------------------------------------------------
// Name: DEFINE_TYPE_INFO
// Desc: Creates a type based on the NamedTypedObject class.  Use this in any
//       classes derived from NamedTypedObject to get the IsDerived() functionality.
//-----------------------------------------------------------------------------
#ifndef DEFINE_TYPE_INFO
#define DEFINE_TYPE_INFO() \
    public: \
        virtual BOOL IsDerivedFrom( ATG::StringID _TypeID ) CONST  { if( _TypeID == TypeID ) return TRUE;  return __super::IsDerivedFrom( _TypeID ); } \
        virtual const ATG::StringID Type() CONST { return TypeID; } \
        static const ATG::StringID TypeID; 
#endif


//-----------------------------------------------------------------------------
// Name: StringID
// Desc: Memory management for strings- strings will be inserted into a hash
//       table uniquely, and can be referenced by pointer.  If you want to 
//       insert a string case-insensitively, use SetCaseInsensitive
//-----------------------------------------------------------------------------    

// This is the number of lists in the string hashtable - should be prime
const int StringID_HASHSIZE = 61;

class StringID
{
public:
    // Constructors
    StringID()                          { m_strString = s_EmptyString; }    
    StringID( CONST WCHAR* strString )  { m_strString = AddString( strString ); }
    StringID( CONST StringID& other )   { m_strString = other.m_strString; }

    // Assignment
    StringID& operator= ( CONST StringID& RHS ) { m_strString = RHS.m_strString; return *this; }    
    StringID& operator= ( CONST WCHAR* strRHS ) { m_strString = AddString( strRHS ); return *this; }

    // Comparison
    BOOL operator== ( CONST StringID& RHS ) CONST { return m_strString == RHS.m_strString; }    
    BOOL operator== ( CONST WCHAR* strRHS ) CONST;
    BOOL IsEmptyString() const { return m_strString == s_EmptyString; }

    // Casting
    operator CONST WCHAR* () CONST { return m_strString; }
    CONST WCHAR* GetSafeString() CONST { return ( m_strString ? m_strString : L"null" ); }
    
    // Hash lookup function
    static DWORD        HashString( CONST WCHAR* strString );    
protected:
    static CONST WCHAR* AddString( CONST WCHAR* strString );
    static std::list<CONST WCHAR *>* GetStringTable();

protected:
    CONST WCHAR*                    m_strString;               
    static CONST WCHAR*             s_EmptyString;
};

//-----------------------------------------------------------------------------
// Name: EnumStringMap
// Desc: Maps values to strings
//-----------------------------------------------------------------------------    
#ifndef ENUMSTRINGMAP_DEFINED
struct EnumStringMap
{
    DWORD        Value;
    CONST WCHAR* szName;
};
#define ENUMSTRINGMAP_DEFINED
#endif

//-----------------------------------------------------------------------------
// Name: NamedTypedObject
// Desc: The base class that all name-referenced objects inheirit from.
//       They have a StringID name and a static class StringID type.
//-----------------------------------------------------------------------------    

//! class name="NamedTypedObject" Desc="Base class for exposed objects"
//!    property name="Name" Desc="Name of the Object" Get=GetName Set=SetName

class NamedTypedObject
{    
// Because NamedTypedObject has no base class we explicitly define the type info here- most classes
// will us the DEFINE_TYPE_INFO macro, with an explicit callout in the .cpp file.
// TypeID is public so you can use NamedTypedObject::TypeID for comparisons without an instance.

public:
    virtual BOOL                IsDerivedFrom( StringID _TypeID ) CONST { return ( _TypeID == TypeID ); }    
    virtual const StringID      Type() CONST { return TypeID; }
    static const StringID       TypeID;

public:        
    CONST StringID&             GetName() CONST { return m_Name; };
    VOID                        SetName( CONST StringID& Name ) { m_Name = Name; }
    
private:
    StringID                    m_Name;
};


//-----------------------------------------------------------------------------
// Name: NameIndexedCollection
// Desc: A hash table, referenceable by name of NamedTypedObject objects
//-----------------------------------------------------------------------------    

// This is the number of lists in the collection hashtable - should be prime
// $TODO: Allow the size to be settable on a collection basis

const int DEFAULT_COLLECTION_HASHSIZE = 61;

class NameIndexedCollection
{
public:
    // iterator for the collection - it is made so you can 
    // delete the object at the current position and STILL do a ++
    // safely afterwards
    class iterator
    {
    friend class NameIndexedCollection;
    public:
        iterator();      

        NamedTypedObject*   operator*();
        iterator&               operator++( int ); // only postfix defined 
        iterator&               operator=( CONST iterator& iRHS );
        BOOL                    operator==( CONST iterator& iRHS );
        BOOL                    operator!=( CONST iterator& iRHS );
    private:   
        NameIndexedCollection*                  m_pCollection;
        std::list<NamedTypedObject*>::iterator  m_iter;        
        int                                         m_iCurBucket;
    };
    friend class NameIndexedCollection::iterator;
        
    VOID    Add( NamedTypedObject *pObjectToAdd );        // Add a NamedTypedObject to the collection        
    VOID    Remove( NamedTypedObject *pObjectToRemove );  // Remove a NamedTypedObject from the collection        
    NamedTypedObject* Find( CONST WCHAR* strName );       // Find a NamedTypedObject in the collection
    NamedTypedObject* FindTyped( CONST WCHAR* strName, const StringID TypeID );       // Find a NamedTypedObject of a certain type in the collection
    DWORD   Size();
       
    iterator        begin();
    iterator        end();
private:
    std::list<NamedTypedObject *> s_Lists[ DEFAULT_COLLECTION_HASHSIZE ];     
};

} // namespace ATG

#endif // ATG_NamedTypedObject_H
