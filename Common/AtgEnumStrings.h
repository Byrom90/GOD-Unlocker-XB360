//-----------------------------------------------------------------------------
// AtgEnumStrings.h
//
// converts ATG and D3D enums to and frome strings
// 
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#ifndef ATG_ENUMSTRINGS_H
#define ATG_ENUMSTRINGS_H

namespace ATG
{

//-----------------------------------------------------------------------------
// holds a single entry in the enum table
//-----------------------------------------------------------------------------
#ifndef ENUMSTRINGMAP_DEFINED
struct EnumStringMap
{
    DWORD Value;
    CONST WCHAR* szName;
};
#define ENUMSTRINGMAP_DEFINED
#endif
//-----------------------------------------------------------------------------
// what kind of d3d state does this value represent
//-----------------------------------------------------------------------------
enum StateType
{
    STATE_TYPE_ENUM,
    STATE_TYPE_ENUM_WITH_FLAGS,
    STATE_TYPE_FLOAT,
    STATE_TYPE_COLOR,
    STATE_TYPE_MASK,
    STATE_TYPE_BOOL,
    STATE_TYPE_UINT,
    STATE_TYPE_INT,
    STATE_TYPE_UINT_WITH_FLAGS,
    STATE_TYPE_DWORD_COMPILE    = 0x7fffffff,
};

//-----------------------------------------------------------------------------
// contains a single entry in the state table
//-----------------------------------------------------------------------------
struct StateStringMap
{
    DWORD                    Value;
    CONST WCHAR*             szName;
    StateType                Type;
    EnumStringMap* EnumValues;
    EnumStringMap* FlagValues;
};

//-----------------------------------------------------------------------------
// gets a DWORD value from a state
//-----------------------------------------------------------------------------
BOOL GetValueFromString( CONST WCHAR* szValue,
                         CONST EnumStringMap* pEnumStringMap,
                         DWORD& Value );

//-----------------------------------------------------------------------------
// gets a string from state
//-----------------------------------------------------------------------------
BOOL GetStringFromValue( CONST DWORD Value,
                         CONST EnumStringMap* pEnumStringMap,
                         CONST WCHAR*& strString );

//-----------------------------------------------------------------------------
// gets a DWORD value from a state
// first partial substring match from the table wins
//-----------------------------------------------------------------------------
BOOL GetValueFromStringPartial( CONST WCHAR* szValue,
                                CONST EnumStringMap* pEnumStringMap,
                                DWORD& Value );

//-----------------------------------------------------------------------------
// given a string representation of the state and its value, returns the 
// DWORD state and value.
// The value string may be bools, floats, color, enum with mask, etc.
//-----------------------------------------------------------------------------
BOOL GetStateAndValueFromStrings( CONST WCHAR* szState, CONST WCHAR* szValue, 
                                  StateStringMap* pStateStringMap,
                                  DWORD &State, DWORD &Value );

//-----------------------------------------------------------------------------
// enum and state tables
//-----------------------------------------------------------------------------
extern EnumStringMap D3DX_FILTER_StringMap[]; 
extern EnumStringMap D3DXREGISTER_SET_StringMap[];
extern EnumStringMap D3DXPARAMETER_CLASS_StringMap[];
extern EnumStringMap D3DXPARAMETER_TYPE_StringMap[];
extern EnumStringMap D3DFORMAT_StringMap[];
extern EnumStringMap D3DDECLTYPE_StringMap[];
extern EnumStringMap D3DDECLMETHOD_StringMap[];
extern EnumStringMap D3DDECLUSAGE_StringMap[];
extern EnumStringMap D3DFILLMODE_StringMap[];
extern EnumStringMap D3DBLEND_StringMap[];
extern EnumStringMap D3DBLENDOP_StringMap[];
extern EnumStringMap D3DTEXTUREADDRESS_StringMap[];
extern EnumStringMap D3DCULL_StringMap[];
extern EnumStringMap D3DCMPFUNC_StringMap[];
extern EnumStringMap D3DSTENCILOP_StringMap[];
extern EnumStringMap D3DFOGMODE_StringMap[];
extern EnumStringMap D3DZBUFFERTYPE_StringMap[];
extern EnumStringMap D3DPRIMITIVETYPE_StringMap[];
extern EnumStringMap D3DTRANSFORMSTATETYPE_StringMap[];
extern EnumStringMap D3DWRAP_StringMap[];
extern EnumStringMap D3DMATERIALCOLORSOURCE_StringMap[];
extern EnumStringMap D3DDEBUGMONITORTOKENS_StringMap[];
extern EnumStringMap D3DVERTEXBLENDFLAGS_StringMap[];
extern EnumStringMap D3DPATCHEDGESTYLE_StringMap[];
extern EnumStringMap D3DDEGREETYPE_StringMap[];
extern EnumStringMap D3DTEXTUREFILTERTYPE_StringMap[];
extern EnumStringMap D3DTEXTUREOP_StringMap[];
extern EnumStringMap D3DTA_StringMap[];
extern EnumStringMap D3DTA_FLAGS_StringMap[];
extern EnumStringMap D3DTEXTURETRANSFORMFLAGS_StringMap[];
extern EnumStringMap D3DTSS_TCI_FLAGS_StringMap[];
extern EnumStringMap D3DCLEAR_StringMap[];
extern StateStringMap D3DRENDERSTATETYPE_StateStringMap[];
extern StateStringMap D3DSAMPLERSTATETYPE_StateStringMap[];
extern StateStringMap D3DTEXTURESTAGESTATETYPE_StateStringMap[];
extern EnumStringMap D3DTRILINEARTHRESHOLD_StringMap[];
extern EnumStringMap D3DMULTISAMPLE_TYPE_StringMap[];

} // namespace ATG

#endif
