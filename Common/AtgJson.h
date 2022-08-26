


//--------------------------------------------------------------------------------------
// Light Weight JSON reader reader based on XJSON
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef ATGJSON_H
#define ATGJSON_H


//--------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <assert.h>
#include <XJSONParser.h>
#include <vector>



namespace ATG
{
namespace JSON
{

//--------------------------------------------------------------------------------------
// DEFINES
//--------------------------------------------------------------------------------------
#define MAXTOKENSIZE 3000 


//--------------------------------------------------------------------------------------
// STRUCTS
//--------------------------------------------------------------------------------------
enum DATATYPE
{
    DT_NONE = 0,
    DT_STRING,
    DT_INT,     
    DT_INT64,
    DT_ARRAY
};


//--------------------------------------------------------------------------------------
// struct PropertyTag
//
// this is used to define a RESTful object for reading
//--------------------------------------------------------------------------------------
struct PropertyTag
{
    WCHAR *fieldname; // e.g. "titleId"
    DATATYPE dt;
    DWORD offset;
    DWORD stringSize;
};


//--------------------------------------------------------------------------------------
// class ICollectionBase
//
// pure virtual base class
// generic interface for all nested user defined objects to be deserialized
// awesome as the JSON serializer can access the nested objects through this interface
// and not care about the underlying user type
//--------------------------------------------------------------------------------------
class ICollectionBase
{
public:
    virtual VOID *GetNewObject() = 0; 
    virtual VOID *GetTags() = 0;
    virtual int GetTagSize() = 0;
};


//--------------------------------------------------------------------------------------
// class Collection : public ICollectionBase
//
// with this awesome abstraction with a pure virtual interface base class we can remove
// the need to known what Type we are actually deserializing in the parser as it is
// implicit in the template declaration of the user type
//--------------------------------------------------------------------------------------
template <typename Type> class Collection : public ICollectionBase
{
public:
    Collection() {}
    
    ~Collection()
    {
        object_array.clear();
    }
    
    VOID *GetNewObject() 
    {
        Type *t = new Type();
        object_array.push_back(t);
        return t;
    }

    Type *operator[](int i)
    {
        return object_array[i];
    }

    VOID *GetTags()
    {
        return Type::tags;
    }

    int GetTagSize()
    {
        return Type::tagsize;
    }

    int GetArraySize()
    {
        return object_array.size();
    }

private:
    std::vector<Type *> object_array;
};


//--------------------------------------------------------------------------------------
// class AtgJsonReader
//
//--------------------------------------------------------------------------------------
class AtgJsonReader
{
public:
    static HRESULT Parse(CONST BYTE *inbuf, VOID *bp, PropertyTag *pt, DWORD size);

private:
    static HRESULT ParseJSON(HJSONPARSER &p, VOID *bp, PropertyTag *pt, DWORD size, VOID *parent);
};

} // end namespace JSON
} // end namespace ATG

#endif
