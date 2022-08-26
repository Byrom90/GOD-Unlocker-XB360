

//--------------------------------------------------------------------------------------
// RESTFUL Objects for Xbox LIVE endpoint wrapper layer
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef ROBLIVE_H
#define ROBLIVE_H


//--------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <malloc.h>
#include <stdio.h>
#include "AtgInput.h"
#include "AtgUtil.h"
#include "AtgConsole.h"
#include "AtgHttp.h"
#include "AtgJson.h"


namespace ATG
{
namespace REST
{

// ATGREST sits on top of the HTTP and JSON components
using namespace ATG::HTTP;
using namespace ATG::JSON;

#define NO_STRSIZE          0
#define GAMERTAG_STRSIZE    16
#define CUSTOMDATA_STRSIZE  1024
#define DEFAULT_STRSIZE     256
#define BIG_STRSIZE         3000
#define URL_STRSIZE         256

#define TITLEID_STRSIZE     12
#define GROUPID_STRSIZE     40

//----------------------------------------------------------------------------------------------
// JSON representation classes for AtgJsonReader (Based on the Xbox LIVE REST API definitions)
//----------------------------------------------------------------------------------------------
class AtgREST;

class NoObject
{
public:
    // no publics, used simply to make webrequest when we dont care about JSON
private: 
    friend AtgREST;
    static PropertyTag tags[];
    static DWORD tagsize;
};


class UsersMe
{
public:
    XUID xuid;
    CHAR gamerTag[GAMERTAG_STRSIZE]; // string sizes are put into tags

private:
    friend AtgREST;
    static PropertyTag tags[];
    static DWORD tagsize;
};

class Players
{
public:
    XUID xuid;
    CHAR gamertag[GAMERTAG_STRSIZE]; 
    INT  seatIndex;
    CHAR customData[CUSTOMDATA_STRSIZE]; 
    BOOL isCurrentlyInSession;

private:
    friend AtgREST;
    friend Collection<Players>;
    static PropertyTag tags[];
    static DWORD tagsize;
};

class Sessions
{
public:
    CHAR  sessionId[DEFAULT_STRSIZE]; 
    DWORD titleId;
    INT   variant;
    CHAR  displayName[DEFAULT_STRSIZE]; 
    //creationTime
    CHAR  customData[CUSTOMDATA_STRSIZE]; 
    INT   maxPlayers;
    INT   seatsAvailable;
    BOOL  isClosed;
    BOOL  hasEnded;
    Collection<Players> roster;

private: 
    friend AtgREST;
    static PropertyTag tags[];
    static DWORD tagsize;
};

class Request
{
public:
    CHAR RequestId[DEFAULT_STRSIZE]; 

private: 
    friend AtgREST;
    static PropertyTag tags[];
    static DWORD tagsize;
};


class RequestStatus
{
public:
    CHAR SessionId[DEFAULT_STRSIZE]; 
    CHAR Status[DEFAULT_STRSIZE]; 

private: 
    friend AtgREST;
    static PropertyTag tags[];
    static DWORD tagsize;
};

class GameMessages
{
public:
    INT queueIndex;
    INT sequenceNumber;
    XUID senderXuid;
    //DateTime timeStampe;
    CHAR data[BIG_STRSIZE]; 

private: 
    friend AtgREST;
    friend Collection<GameMessages>;
    static PropertyTag tags[];
    static DWORD tagsize;
};


class MessageArray
{
public:
    Collection<GameMessages> messages;

private: 
    friend AtgREST;
    static PropertyTag tags[];
    static DWORD tagsize;
};


//--------------------------------------------------------------------------------------
// class Base64
//
// simple base64 encoder/decoder.  Easy to re-use.
//--------------------------------------------------------------------------------------
class Base64
{
public:
    VOID ConstructTables();
    VOID EncodeData(BYTE *data, BYTE *outData, DWORD cbData);
    VOID DecodeData(BYTE *data, BYTE *outData, DWORD cbData);

private:
    VOID DecodeChunk(BYTE *encodedData, BYTE *outData);
    VOID EncodeChunk1(BYTE *rawData, BYTE *encodedData);
    VOID EncodeChunk2(BYTE *rawData, BYTE *encodedData);
    VOID EncodeChunk3(BYTE *rawData, BYTE *encodedData);

    BYTE Base64Table [64]; 
    BYTE Base64Decode[256]; 

};


//--------------------------------------------------------------------------------------
// class HttpResponse
//
// this class is used for each wrapper to return up the HTTP response information
// if the function fails, it will return NULL and then the client can use this class 
// to understand why the call failed
//--------------------------------------------------------------------------------------
class HttpResponse
{
public:
    DWORD returnCode;
};


//--------------------------------------------------------------------------------------
// class AtgREST
//
// RESTFUL Objects for Xbox LIVE endpoint wrapper layer
//--------------------------------------------------------------------------------------
class AtgREST 
{
public:
    AtgREST() {};

    HRESULT Initialize(CHAR *titleId, CHAR *groupId, DWORD dwUserIndex);
    HRESULT Shutdown();

    template<typename Type> Type* MakeWebRequest(CHAR *customHeader, CHAR *url, CHAR *verb, CHAR *data, Type *p, HttpResponse &response);

    Sessions* CreateAsyncSession(INT variant, INT maxPlayers, CHAR *displayName, HttpResponse &response);
    Players* JoinAsyncSession(CHAR *sessionid, CHAR *xuid, CHAR *gamertag, HttpResponse &response); 
    Sessions* GetAsyncSession(CHAR *sessionid, HttpResponse &response); 
    Request* CreateMatchmakingRequest(CHAR *xuid, CHAR *sessionid, DWORD giveupDuration, DWORD seatsOccupied, DWORD minSeats, DWORD maxSeats, CHAR *custom, HttpResponse &response);
    RequestStatus* GetMatchmakingRequestStatus(CHAR *xuid, CHAR *request, DWORD timeout, HttpResponse &response);
    VOID CancelMatchmakingRequestStatus(CHAR *xuid, CHAR *requestId, HttpResponse &response);
    UsersMe* GetMe(HttpResponse &response);
    VOID GetMessages(CHAR *xuid, HttpResponse &response);
    VOID SendMessage(CHAR *sessionId, HttpResponse &response);
    VOID DeleteMessage(CHAR *xuid, HttpResponse &response);
    VOID SendAsyncMPMessage(CHAR *sessionId, CHAR *xuid, CHAR *data, DWORD queueNum, HttpResponse &response);
    MessageArray* GetAsyncMPMessage(CHAR *sessionId, DWORD startAfterSeqNum, DWORD queueNum, DWORD timeout, HttpResponse &response);
    VOID DeleteAsyncMPMessage(HttpResponse &response);
    VOID UpdateSessionPlayer(HttpResponse &response);
    VOID UpdateSession(HttpResponse &response);
    VOID RemovePlayer(HttpResponse &response);
    VOID GetUserSessions(CHAR *xuid, HttpResponse &response);

private:
    AuthManager m_Auth;
    HANDLE      m_hWorkerThread;
    CHAR        m_titleId[TITLEID_STRSIZE]; // string representation of 32 bit number
    CHAR        m_groupId[GROUPID_STRSIZE]; // string representation of the title group id
};


} // end namespace REST
} // end namespace ATG

#endif
