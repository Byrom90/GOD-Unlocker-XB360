


//--------------------------------------------------------------------------------------
// Light Weight JSON object reader based on XJSON
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "stdafx.h"

#include "AtgJson.h"
#include "AtgUtil.h"

namespace ATG
{
namespace JSON
{

//#define DEBUGMODE
#ifdef DEBUGMODE
#define ENABLE_PANIC // use to enable a "panic" mode for parser
CHAR * JSON_TOKENS[] = {
    "Json_NotStarted",
    "Json_BeginArray",
    "Json_EndArray",
    "Json_BeginObject",
    "Json_EndObject",
    "Json_String",
    "Json_Number",
    "Json_True",
    "Json_False",
    "Json_Null",
    "Json_FieldName",
    "Json_NameSeparator",   // ':' ?
    "Json_ObjectSeparator", // '.' inside an object
    "Json_ValueSeparator"   // ',' inside an array
};
#endif


DWORD g_inObject = 0;      // object counter
DWORD g_inArray = 0;       // array counter
BOOL  g_isSpecial = FALSE; // used for custom objects that are not represented as arrays in JSON


//--------------------------------------------------------------------------------------
// AtgJsonReader::ParseJSON
//
// recursive parse function for JSON objects.  (objects can contain objects)
// generic json parser that de-serializes the data into the bp based on tag information
//
// while this routine works for the JSON tested with, it has not been stress tested 
// with fuzz data.  XJSON(d).lib has been fuzz tested extensively however.
//--------------------------------------------------------------------------------------
HRESULT AtgJsonReader::ParseJSON(HJSONPARSER &p, VOID *bp, PropertyTag *pt, DWORD size, VOID *parent)
{
    JSONTOKENTYPE token;
    DWORD toklen;
    DWORD parsed;
    WCHAR outbuf[MAXTOKENSIZE];
    HRESULT hr = S_OK;
    
    VOID *poke = NULL; // this is a pointer into the field we want to write data to

    PropertyTag *tag = NULL; // this is the property tag of the class we want to write data to
    
#ifdef ENABLE_PANIC
    DWORD start_time = GetTickCount();
    CONST DWORD panic_time = 2000; //NOTE: if we are taking a few seconds to parse the JSON, something went wrong so we should panic and bail out with error code
#endif


    // walk the stream of tokens looking for a matching fieldname.  
    // With a fieldname and matching tags for the object, we can poke the data into it.
    do
    {
        hr = XJSONReadToken(p, &token, &toklen, &parsed);
        if (hr == JSON_E_OUT_OF_BUFFER)
            break; // legitimately ran out of buffer, we are done

        hr = XJSONGetTokenValue(p, outbuf, MAXTOKENSIZE); 

#ifdef ENABLE_PANIC
        if (GetTickCount() - start_time > panic_time)
        {
            ATG::DebugSpew("[ATGJSON] panic out as we are taking too long");
            break;
        }
#endif

#ifdef DEBUGMODE
        ATG::DebugSpew("[ATGJSON] outbuf: %ws %s %d\n", outbuf, JSON_TOKENS[t], hr);
#endif

        // handle the token type
        switch(token)
        {
        
        case Json_FieldName:
#ifdef DEBUGMODE
            ATG::DebugSpew("[ATGJSON] property field = %ws\n", outbuf);
#endif
            // NOTE: linear walk over tags (results in NxN algorithm)
            for (DWORD i=0; i<size; i++)
            {
                if (wcscmp(pt[i].fieldname, outbuf) == 0)
                {
#ifdef DEBUGMODE
                    ATG::DebugSpew("[ATGJSON] matched\n");
#endif
                    poke = (VOID *) ((int) bp + pt[i].offset);
                    tag = &pt[i];
                    if (tag->dt == DT_ARRAY)
                    {
                        g_isSpecial = true;
                    }
                    else
                    {
                        g_isSpecial = false;
                    }
                    break;
                }
            }
#ifdef DEBUGMODE
            if (poke == NULL)
            {
                ATG::DebugSpew("[ATGJSON] not matched\n"); // NOTE: great point to realize what is missing
            }
#endif
            break;

        case Json_BeginArray:
            g_inArray++;
            g_isSpecial = false;
            
            if (tag == NULL) 
                break; // if we dont have a tag, we cant do anything

            if (tag->dt == DT_ARRAY) 
            {
                parent = poke;
                poke = NULL;
                tag = NULL;
            }
            break;
        
        case Json_EndArray:
            g_inArray--;
#ifdef DEBUGMODE
            ATG::DebugSpew("[ATGJSON] end array\n");
#endif
            break;
        
        case Json_BeginObject:
            g_inObject++;
            if (g_isSpecial) 
            {
                parent = poke;
            }
            if ((g_inArray>0 || g_isSpecial) && parent != NULL)
            {
#ifdef DEBUGMODE
                ATG::DebugSpew("[ATGJSON] new object in array\n");
#endif
                g_isSpecial = FALSE; 
                ICollectionBase *b = (ICollectionBase *) parent;
                ICollectionBase *newobject = (ICollectionBase *) b->GetNewObject();
                poke = NULL;
                tag = NULL;
                
                ParseJSON(p, newobject, (PropertyTag *) b->GetTags(), b->GetTagSize(), parent);
            }
            break;
        
        case Json_EndObject:

            g_inObject--;
            if (g_inArray>0)
                return 0; 
            break;
        
        case Json_String:
        case Json_Number:
            if (poke != NULL)
            {
                assert (tag != NULL);

                switch (tag->dt)
                {
                case DT_STRING:
                    {
                        // sprintf conversion here and change all DT_STRING to CHAR*
                        sprintf_s((CHAR *) poke, tag->stringSize, "%ws", outbuf); 
                        poke = NULL;
                        tag = NULL;
                    }
                    break;

                case DT_INT:
                    {
                        DWORD *d = (DWORD *) poke;
                        DWORD foo = (DWORD) _wtoi64(outbuf); 

                        *d = foo;

                        poke = NULL;
                        tag = NULL;
                    }
                    break;

                case DT_INT64: 
                    {
                        unsigned __int64 *d = (unsigned __int64 *) poke;
                        unsigned __int64 foo = _wtoi64(outbuf); 

                        *d = foo;

                        poke = NULL;
                        tag = NULL;
                    }
                    break;

                default:
                    ATG::DebugSpew("[ATGJSON] unknown DT_TYPE passed\n");
                } // end switch on tag->dt
            }
            break;
        
        case Json_True:
            if (poke != NULL)
            {
                if (tag->dt != DT_INT)
                {
                    ATG::DebugSpew("[ATGJSON] Json_True requires DT_INT\n");
                }
                else
                {
                    DWORD *d = (DWORD *) poke;
                    *d = 1;
                    poke = NULL;
                    tag = NULL;
                }
            }
            break;
        
        case Json_False:
            if (poke != NULL)
            {
                if (tag->dt != DT_INT)
                {
                    ATG::DebugSpew("[ATGJSON] Json_False requires DT_INT\n");
                }
                else
                {
                    DWORD *d = (DWORD *) poke;
                    *d = 0;
                    poke = NULL;
                    tag = NULL;
                }
            }
            break;
        
        case Json_Null:
            break;
        
        case Json_NameSeparator:
            break;
        
        case Json_ObjectSeparator:  // '.' inside an object
            break;
        
        case Json_ValueSeparator:   // ',' inside an array
            break;
        
        default:
            ATG::DebugSpew("[ATGJSON] error, unknown token.  Parsed=%d\n", parsed);
            return 0;

        }
    }
    while (1); 

    return hr;
}


//--------------------------------------------------------------------------------------
// Parse
//
//--------------------------------------------------------------------------------------
HRESULT AtgJsonReader::Parse(CONST BYTE *inbuf, VOID *bp, PropertyTag *pt, DWORD size) 
{ 
    HJSONPARSER p = XJSONInitialize();
    
    HRESULT hr = XJSONSetBuffer(p, (CONST CHAR *) inbuf, strlen((CONST CHAR *) inbuf), FALSE);

    if (hr != S_OK)
    {
        return hr;
    }

    g_inObject = 0; 
    g_inArray = 0;
    g_isSpecial = FALSE;

    hr = ParseJSON(p, bp, pt, size, NULL);

    XJSONDestroy(p);

    return hr;
}

} // end namespace JSON
} // end namespace ATG
