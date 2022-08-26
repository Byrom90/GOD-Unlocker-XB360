
//--------------------------------------------------------------------------------------
// RESTFUL Objects for Xbox LIVE endpoint wrapper layer
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "stdafx.h"

#include "AtgREST.h"


namespace ATG
{
namespace REST
{

//--------------------------------------------------------------------------------------
// INCLUDES
//--------------------------------------------------------------------------------------
#include "AtgREST.h"


//--------------------------------------------------------------------------------------
// GLOBALS
//--------------------------------------------------------------------------------------
CHAR BASEURL[] = "https://services.part.xboxlive.com";   // PartnerNet
//CHAR BASEURL[] = "https://services.cert.xboxlive.com"; // CertNet
//CHAR BASEURL[] = "https://services.xboxlive.com";      //Production


//--------------------------------------------------------------------------------------
// AtgREST::Initialize
//
//--------------------------------------------------------------------------------------
HRESULT AtgREST::Initialize(CHAR *titleId, CHAR *groupId, DWORD dwUserIndex) 
{
    HRESULT hr = m_Auth.Startup(NULL, FALSE, &m_hWorkerThread, TRUE, dwUserIndex);
    if (hr != S_OK)
    {
        ATG::FatalError("Failed to Initialize: %x\n", hr);
    }

    if (titleId != NULL)
    {
        strcpy_s(m_titleId, titleId); 
    }

    if (groupId != NULL)
    {
        strcpy_s(m_groupId, groupId);
    }

    return hr;
}

//--------------------------------------------------------------------------------------
// AtgREST::Shutdown
//
//--------------------------------------------------------------------------------------
HRESULT AtgREST::Shutdown()
{
    m_Auth.Shutdown(); 
    return S_OK;
}


//--------------------------------------------------------------------------------------
// JSON PropertyTag descriptions for the RESTFUL-Objects
// tags are used to describe the JSON objects to deserialize
//--------------------------------------------------------------------------------------

PropertyTag NoObject::tags[] = {
    L"",                    DT_NONE,   0, NO_STRSIZE,
};
DWORD NoObject::tagsize = 0;


PropertyTag UsersMe::tags[] = {
    L"xuid",                DT_INT64,   offsetof(UsersMe, xuid),     NO_STRSIZE,
    L"gamerTag",            DT_STRING,  offsetof(UsersMe, gamerTag), GAMERTAG_STRSIZE,
};
DWORD UsersMe::tagsize = ARRAY_SIZE(UsersMe::tags);


PropertyTag Players::tags[] = {
    L"xuid",                DT_INT64,   offsetof(Players, xuid),                   NO_STRSIZE,
    L"gamertag",            DT_STRING,  offsetof(Players, gamertag),               GAMERTAG_STRSIZE,
    L"seatIndex",           DT_INT,     offsetof(Players, seatIndex),              NO_STRSIZE,
    L"customData",          DT_STRING,  offsetof(Players, customData),             CUSTOMDATA_STRSIZE,           
    L"isCurrentlyInSession",DT_INT,     offsetof(Players, isCurrentlyInSession),   NO_STRSIZE,
};
DWORD Players::tagsize = ARRAY_SIZE(Players::tags);


PropertyTag Sessions::tags[] = {
    L"sessionId",           DT_STRING,  offsetof(Sessions, sessionId),       DEFAULT_STRSIZE,
    L"titleId",             DT_INT,     offsetof(Sessions, titleId),         NO_STRSIZE,
    L"variant",             DT_INT,     offsetof(Sessions, variant),         NO_STRSIZE,
    L"displayName",         DT_STRING,  offsetof(Sessions, displayName),     DEFAULT_STRSIZE,
    L"customData",          DT_STRING,  offsetof(Sessions, customData),      CUSTOMDATA_STRSIZE,
    L"maxPlayers",          DT_INT,     offsetof(Sessions, maxPlayers),      NO_STRSIZE,
    L"seatsAvailable",      DT_INT,     offsetof(Sessions, seatsAvailable),  NO_STRSIZE,
    L"isClosed",            DT_INT,     offsetof(Sessions, isClosed),        NO_STRSIZE,
    L"hasEnded",            DT_INT,     offsetof(Sessions, hasEnded),        NO_STRSIZE,
    L"roster",              DT_ARRAY,   offsetof(Sessions, roster),          NO_STRSIZE, // collection of player objects
};
DWORD Sessions::tagsize = ARRAY_SIZE(Sessions::tags);


PropertyTag Request::tags[] = {
    L"RequestId",           DT_STRING,  offsetof(Request, RequestId),  DEFAULT_STRSIZE,
};
DWORD Request::tagsize = ARRAY_SIZE(Request::tags);


PropertyTag RequestStatus::tags[] = {
    L"SessionId",           DT_STRING,  offsetof(RequestStatus, SessionId), DEFAULT_STRSIZE,
    L"Status",              DT_STRING,  offsetof(RequestStatus, Status),    DEFAULT_STRSIZE,
};
DWORD RequestStatus::tagsize = ARRAY_SIZE(RequestStatus::tags);


PropertyTag GameMessages::tags[] = {
    L"queueIndex",          DT_INT,     offsetof(GameMessages, queueIndex),     NO_STRSIZE,
    L"sequenceNumber",      DT_INT,     offsetof(GameMessages, sequenceNumber), NO_STRSIZE,
    L"senderXuid",          DT_INT64,   offsetof(GameMessages, senderXuid),     NO_STRSIZE,
    L"data",                DT_STRING,  offsetof(GameMessages, data),           BIG_STRSIZE,
};
DWORD GameMessages::tagsize = ARRAY_SIZE(GameMessages::tags);


PropertyTag MessageArray::tags[] = {
    L"messages",            DT_ARRAY,   offsetof(MessageArray, messages), NO_STRSIZE, // collection of player objects
};
DWORD MessageArray::tagsize = ARRAY_SIZE(MessageArray::tags);


//--------------------------------------------------------------------------------------
// AtgREST::MakeWebRequest
//
// shared function used in each wrapper based on the custom JSON type we are working
// with
//--------------------------------------------------------------------------------------
template<typename Type> Type* AtgREST::MakeWebRequest(CHAR *customHeader, CHAR *url, CHAR *verb, CHAR *data, Type *p, HttpResponse &response)
{
    ATG::HTTP::AuthManager::AuthEndpoint *endpoint;
    endpoint = m_Auth.CreateEndpoint(url, TRUE);

    DWORD len = 0;
    if (data != NULL)
    {
        len = strlen(data);
    }

    // open up a request to the endpoint 
    HRESULT hr = endpoint->MakeSyncRequest(verb, customHeader, data, len);
    if (hr != S_OK)
    {
        ATG::FatalError("Failed to OpenRequest: %x\n", hr);
    }

    p = NULL;

    response.returnCode = endpoint->GetHTTPStatusCode();

    if (response.returnCode == HTTP_STATUS_OK) 
    {
        p = new Type();
        AtgJsonReader::Parse(endpoint->GetReadBuffer(), p, p->tags, p->tagsize);
    }
    
    endpoint->CloseRequest();

    m_Auth.RemoveEndpoint(endpoint);

    return p;
}


//--------------------------------------------------------------------------------------
// AtgREST::GetMe
//
//--------------------------------------------------------------------------------------
UsersMe * AtgREST::GetMe(HttpResponse &response)
{
    CHAR URL[] = "/users/me/id";
    CHAR FULLURL[URL_STRSIZE]; 

    sprintf_s(FULLURL, URL_STRSIZE, "%s%s", BASEURL, URL);

    UsersMe *me = NULL;
    me = MakeWebRequest<UsersMe>(NULL, FULLURL, "GET", NULL, me, response);
    return me;
}


//--------------------------------------------------------------------------------------
// AtgREST::CreateAsyncSession
//
// ...
//--------------------------------------------------------------------------------------
Sessions * AtgREST::CreateAsyncSession(INT variant, INT maxPlayers, CHAR *displayName, HttpResponse &response)
{
    CHAR URL[] = "/system/multiplayer/sessions";
    CHAR FULLURL[URL_STRSIZE];
    CHAR buf[MAXTOKENSIZE]; 

    sprintf_s(FULLURL, URL_STRSIZE, "%s%s", BASEURL, URL);

    sprintf_s(buf, MAXTOKENSIZE, "{\"titleId\":%s,\"groupId\":\"%s\",\"variant\":%d,\"displayName\":\"%s\",\"maxPlayers\":%d,\"visibility\":\"Everyone\"}", m_titleId, m_groupId, variant, displayName, maxPlayers);

    Sessions *s = NULL;
    s = MakeWebRequest<Sessions>(NULL, FULLURL, "POST", buf, s, response);
    return s;
}


//--------------------------------------------------------------------------------------
// AtgREST::GetAsyncSession
// ...
//--------------------------------------------------------------------------------------
Sessions * AtgREST::GetAsyncSession(CHAR *sessionid, HttpResponse &response)
{
    CHAR URL[] = "/system/multiplayer/sessions/";
    CHAR FULLURL[URL_STRSIZE];

    sprintf_s(FULLURL, URL_STRSIZE, "%s%s%s", BASEURL, URL, sessionid);

    Sessions *s = NULL;
    s = MakeWebRequest<Sessions>(NULL, FULLURL, "GET", NULL, s, response);
    return s;
}


//--------------------------------------------------------------------------------------
// AtgREST::JoinAsyncSession
//
//--------------------------------------------------------------------------------------
Players * AtgREST::JoinAsyncSession(CHAR *sessionid, CHAR *xuid, CHAR *gamertag, HttpResponse &response)
{
    CHAR URL[] = "/system/multiplayer/sessions/";
    CHAR FULLURL[256];
    CHAR buf[MAXTOKENSIZE]; 

    sprintf_s(FULLURL, URL_STRSIZE, "%s%s%s/players", BASEURL, URL, sessionid);

    sprintf_s(buf, MAXTOKENSIZE, "{\"xuid\":\"%s\",\"gamertag\":\"%s\"}", xuid, gamertag);

    Players *player = NULL;
    player = MakeWebRequest<Players>(NULL, FULLURL, "POST", buf, player, response);
    return player;
}


//--------------------------------------------------------------------------------------
// AtgREST::SendAsyncMPMessage
//
//--------------------------------------------------------------------------------------
VOID AtgREST::SendAsyncMPMessage(CHAR *sessionId, CHAR *xuid, CHAR *data, DWORD queueNum, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];
    CHAR buf[MAXTOKENSIZE*2]; 

    sprintf_s(FULLURL, URL_STRSIZE, "%s/system/multiplayer/sessions/%s/queues/%d/messages", BASEURL, sessionId, queueNum);

    sprintf_s(buf, (MAXTOKENSIZE*2), "%s%s%s%s%s", "{\"senderXuid\":", xuid, ",\"data\":\"", data, "\"}");

    // NOTE: need to create the return type object for this
    NoObject *empty = NULL;
    empty = MakeWebRequest<NoObject>(NULL, FULLURL, "POST", buf, empty, response);
    delete empty;
    return;
}


//--------------------------------------------------------------------------------------
// AtgREST::GetAsyncMPMessage
//
//--------------------------------------------------------------------------------------
MessageArray * AtgREST::GetAsyncMPMessage(CHAR *sessionId, DWORD startAfterSeqNum, DWORD queueNum, DWORD timeout, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];

    if (startAfterSeqNum == 0)
    {
        sprintf_s(FULLURL, URL_STRSIZE, "%s/system/multiplayer/sessions/%s/queues/%d/messages", BASEURL, sessionId, queueNum);
    }
    else
    {
        sprintf_s(FULLURL, URL_STRSIZE, "%s/system/multiplayer/sessions/%s/queues/%d/messages?startAfterSeqNum=%d&timeout=%d", BASEURL, sessionId, queueNum, startAfterSeqNum, timeout);
    }

    MessageArray *m = NULL;
    m = MakeWebRequest<MessageArray>(NULL, FULLURL, "GET", NULL, m, response);
    return m;
}


//--------------------------------------------------------------------------------------
// AtgREST::DeleteAsyncMPMessage
//
//--------------------------------------------------------------------------------------
VOID AtgREST::DeleteAsyncMPMessage(HttpResponse &response)
{
// DELETE /system/mutiplayer/sessions/{sessionId}/queues/{queueIndex}/messages
}


//--------------------------------------------------------------------------------------
// AtgREST::UpdateSessionPlayer
//
//--------------------------------------------------------------------------------------
VOID AtgREST::UpdateSessionPlayer(HttpResponse &response)
{
// POST /system/multiplayer/sessions/{sessionid}/players/xuid({xuid})
}

//--------------------------------------------------------------------------------------
// AtgREST::UpdateSession
//
//--------------------------------------------------------------------------------------
VOID AtgREST::UpdateSession(HttpResponse &response)
{
// POST /system/multiplayer/sessions/{sessionid}
}

//--------------------------------------------------------------------------------------
// AtgREST::RemovePlayer
//
//--------------------------------------------------------------------------------------
VOID AtgREST::RemovePlayer(HttpResponse &response)
{
// DELETE /system/multiplayer/sessions/{sessionid}/players/xuid({xuid})

}


//--------------------------------------------------------------------------------------
// AtgREST::GetUserSessions
//
//--------------------------------------------------------------------------------------
VOID AtgREST::GetUserSessions(CHAR *xuid, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];

    sprintf_s(FULLURL, URL_STRSIZE, "%s/users/xuid(%s)/multiplayer/sessions?titleId=%s", BASEURL, xuid, m_titleId);

    NoObject *empty = NULL;
    empty = MakeWebRequest<NoObject>(NULL, FULLURL, "GET", NULL, empty, response);
    delete empty;
    return;
}


//--------------------------------------------------------------------------------------
// AtgREST::GetMessages
//
//--------------------------------------------------------------------------------------
VOID AtgREST::GetMessages(CHAR *xuid, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];

    sprintf_s(FULLURL, URL_STRSIZE, "%s/messages/users/xuid(%s)/inbox", BASEURL, xuid);

    NoObject *empty = NULL;
    empty = MakeWebRequest<NoObject>(NULL, FULLURL, "GET", NULL, empty, response);
    delete empty;
    return;
}


//--------------------------------------------------------------------------------------
// AtgREST::SendMessage
//
//--------------------------------------------------------------------------------------
VOID AtgREST::SendMessage(CHAR *sessionId, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];

    sprintf_s(FULLURL, URL_STRSIZE, "%s/system/messaging/outbox", BASEURL);

    // NOTE: not completed

    return;
}


//--------------------------------------------------------------------------------------
// AtgREST::DeleteMessage
//
//--------------------------------------------------------------------------------------
VOID AtgREST::DeleteMessage(CHAR *xuid, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];

    sprintf_s(FULLURL, URL_STRSIZE, "%susers/xuid(%s)/inbox/%s", BASEURL, xuid, "messageid");

    NoObject *empty = NULL;
    empty = MakeWebRequest<NoObject>(NULL, FULLURL, "DELETE", NULL, empty, response);
    delete empty;
    return;
}


//--------------------------------------------------------------------------------------
// AtgREST::CreateMatchmakingRequest
//
//--------------------------------------------------------------------------------------
Request * AtgREST::CreateMatchmakingRequest(CHAR *xuid, CHAR *sessionid, DWORD giveupDuration, DWORD seatsOccupied, DWORD minSeats, DWORD maxSeats, CHAR *custom, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];
    CHAR buf[MAXTOKENSIZE]; 

    sprintf_s(FULLURL, URL_STRSIZE, "%s/users/xuid(%s)/multiplayer/matchrequests", BASEURL, xuid);

    sprintf_s(buf, MAXTOKENSIZE, "{\"TitleId\":%s,\"GroupId\":\"%s\",\"GiveupDuration\":%d,\"PostbackUrl\":null,\"SeatsOccupied\":%d,\"MinSeats\":%d,\"MaxSeats\":%d,\"MatchCriteria\":{\"Restriction\":\"MatchAny\",\"Custom\":\"%s\"}}", m_titleId, m_groupId, giveupDuration, seatsOccupied, minSeats, maxSeats, custom);

    Request *req = NULL;
    req = MakeWebRequest<Request>(NULL, FULLURL, "POST", buf, req, response);
    return req;   
}


//--------------------------------------------------------------------------------------
// AtgREST::GetMatchmakingRequestStatus
//
//--------------------------------------------------------------------------------------
RequestStatus * AtgREST::GetMatchmakingRequestStatus(CHAR *xuid, CHAR *request, DWORD timeout, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];

    sprintf_s(FULLURL, URL_STRSIZE, "%s/users/xuid(%s)/multiplayer/matchrequests/%s?timeout=%d", BASEURL, xuid, request, timeout);

    RequestStatus *status = NULL;
    status = MakeWebRequest<RequestStatus>(NULL, FULLURL, "GET", NULL, status, response);
    return status;   
}


//--------------------------------------------------------------------------------------
// AtgREST::CancelMatchmakingRequestStatus
//
//--------------------------------------------------------------------------------------
VOID AtgREST::CancelMatchmakingRequestStatus(CHAR *xuid, CHAR *requestId, HttpResponse &response)
{
    CHAR FULLURL[URL_STRSIZE];
    sprintf_s(FULLURL, URL_STRSIZE, "%s/users/xuid(%s)/multiplayer/matchrequests/%s", BASEURL, xuid, requestId);

    NoObject *empty = NULL;
    empty = MakeWebRequest<NoObject>(NULL, FULLURL, "DELETE", NULL, empty, response);
    delete empty;
    return;   
}


//--------------------------------------------------------------------------------------
// Base64::EncodeChunk1
//
// encode 1 byte of raw data
//--------------------------------------------------------------------------------------
VOID Base64::EncodeChunk1(BYTE *rawData, BYTE *encodedData)
{
    BYTE val1 = rawData[0] >> 2; // first 6 bits
    BYTE val2 = ((rawData[0] & 0x03) << 4);

    encodedData[0] = Base64Table[val1];
    encodedData[1] = Base64Table[val2];
    encodedData[2] = '=';
    encodedData[3] = '=';
}


//--------------------------------------------------------------------------------------
// Base64::EncodeChunk2
//
// encode 2 bytes of raw data
//--------------------------------------------------------------------------------------

VOID Base64::EncodeChunk2(BYTE *rawData, BYTE *encodedData)
{
    BYTE val1 = rawData[0] >> 2; // first 6 bits
    BYTE val2 = ((rawData[0] & 0x03) << 4) | (rawData[1] >> 4); // last 2 bits of first byte + first 4 bits of second byte
    BYTE val3 = ((rawData[1] & 0x0F) << 2);

    encodedData[0] = Base64Table[val1];
    encodedData[1] = Base64Table[val2];
    encodedData[2] = Base64Table[val3];
    encodedData[3] = '=';
}


//--------------------------------------------------------------------------------------
// Base64::EncodeChunk3
//
// convert 3 8bit normal bytes (24bits total) into 4 64bit base64
//--------------------------------------------------------------------------------------
VOID Base64::EncodeChunk3(BYTE *rawData, BYTE *encodedData)
{
    BYTE val1 = rawData[0] >> 2; // first 6 bits
    BYTE val2 = ((rawData[0] & 0x03) << 4) | (rawData[1] >> 4); // last 2 bits of first byte + first 4 bits of second byte
    BYTE val3 = ((rawData[1] & 0x0F) << 2) | (rawData[2] >> 6); // last 4 bits of second byte and first 2 bits of last byte
    BYTE val4 = (rawData[2] << 2); // last 6 bits
    val4 = val4 >> 2;

    encodedData[0] = Base64Table[val1];
    encodedData[1] = Base64Table[val2];
    encodedData[2] = Base64Table[val3];
    encodedData[3] = Base64Table[val4];
}

//--------------------------------------------------------------------------------------
// Base64::DecodeChunk
//
// base64 comes in 4 6-bit group bundles, process one bundle
//--------------------------------------------------------------------------------------
VOID Base64::DecodeChunk(BYTE *encodedData, BYTE *outData)
{
    DWORD out = 0;

    // get 4 6-bit groups
    DWORD a = Base64Decode[encodedData[0]];
    DWORD b = Base64Decode[encodedData[1]];
    DWORD c = Base64Decode[encodedData[2]];
    DWORD d = Base64Decode[encodedData[3]];
    
    // combine 4 6-bit entries into 8bit array (3 bytes or lower 24 bits)
    out = (a << 18) | (b << 12) | (c << 6) | d;

    // write out 24bits that are now grouped together correctly
    if (outData != NULL) 
    {
        BYTE *p = (BYTE *) &out;
        outData[0] = p[1];
        outData[1] = p[2];
        outData[2] = p[3];
    }
}

//--------------------------------------------------------------------------------------
// Base64::EncodeData
//
// Encode a string into Base64
//--------------------------------------------------------------------------------------
VOID Base64::EncodeData(BYTE *data, BYTE *outData, DWORD cbData)
{
    while ((int) cbData > 0)
    {
        if (cbData == 1)
        {
            EncodeChunk1(data, outData);
            cbData -= 1;
            data += 1;
            outData += 4;
        }
        else if (cbData == 2)
        {
            EncodeChunk2(data, outData);
            cbData -= 2;
            data += 2;
            outData += 4;
        }
        else
        {
            EncodeChunk3(data, outData);
            cbData -= 3;
            data += 3;
            outData += 4;
        }
    }
    *outData = 0;
}


//--------------------------------------------------------------------------------------
// Base64::DecodeData
//
// Decode a base64 string
//--------------------------------------------------------------------------------------
VOID Base64::DecodeData(BYTE *data, BYTE *outData, DWORD cbData)
{
    while ((int) cbData > 0)
    {
        DecodeChunk(data, outData);

        cbData -= 4;
        data += 4;
        outData += 3;
    }
    *outData = 0;
}

//--------------------------------------------------------------------------------------
// Base64::ConstructTables
//
// Function builds both Base64 encode and decode look-up tables (used for faster processing)
//--------------------------------------------------------------------------------------
VOID Base64::ConstructTables()
{
    for(BYTE i='A'; i<='Z'; i++)
    {
        BYTE val = i - 'A';
        Base64Decode[i] = val;
        Base64Table[val] = i;
    }

    for(BYTE i='a'; i<='z'; i++)
    {
        BYTE val = i - 'a' + 26;
        Base64Decode[i] = val;
        Base64Table[val] = i;
    }

    for (BYTE i='0'; i<='9'; i++)
    {
        BYTE val = i - '0' + 52;
        Base64Decode[i] = val;
        Base64Table[val] = i;
    }

    Base64Decode['+'] = 62;
    Base64Table[62] = '+';

    Base64Decode['/'] = 63;
    Base64Table[63] = '/';

    Base64Decode['='] = 0; //padding means no bytes existed so zero bits
}


} // end namespace REST
} // end namespace ATG

