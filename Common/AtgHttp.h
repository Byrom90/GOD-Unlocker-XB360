//--------------------------------------------------------------------------------------
// ATGHttp
//
// Uses XHttp/XAuth to manage HTTP(s) connections and XSTS tokens 
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#pragma once
#ifndef ATGHTTP_H
#define ATGHTTP_H

#include <xtl.h>
#include <xhttp.h>
#include <xauth.h>
#include <assert.h>
#include <stdio.h>
#include <vector>


namespace ATG
{
namespace HTTP
{

#define MAX_HOSTNAME_LENGTH 128           // maximum size for the hostname
#define MAX_HEADER_LENGTH   16384         // maximum token size expected
#define MAX_URL_LENGTH      1024          // each endpoint URL size max as specified by the NSAL
#define RECEIVE_BUFFER_SIZE 16384         // buffer size for reading HTTP data

#define ATGHTTP_DEFAULT_USER_AGENT "ATGHTTP_XBOX360"  // default user agent string


//--------------------------------------------------------------------------------------
// class AuthManager
//
// This class handles XHTTP and XAuth code usage for the samples.
// We want to deal with the endpoints at a higher level of abstraction.
// AuthManager manages XHTTP/XAuth endpoint requests and responses as well as token usage
//--------------------------------------------------------------------------------------
class AuthManager
{
public:
    class AuthEndpoint; // TODO: make private and manage internally

    AuthManager()
    {
        m_hSession = NULL;
        m_hEngineThread = NULL;
        m_fShutdown = FALSE;
        m_szUserAgent = ATGHTTP_DEFAULT_USER_AGENT;
    }

    ~AuthManager()
    {
    }

    HRESULT Startup(CHAR * pszCustomUserAgent, BOOL fBypassSecurity, HANDLE *workerThread, BOOL fSharedToken, DWORD dwUserIndex);
    VOID Shutdown();

    AuthEndpoint * CreateEndpoint(CONST CHAR * pszUrl, BOOL requireToken);
    VOID RemoveEndpoint(AuthEndpoint *endpoint);

    HRESULT DoWork();
    static DWORD __stdcall EngineThread(LPVOID lpThreadParameter);
    DWORD RealEngineThread();


private:
    // endpoints vector
    std::vector<AuthEndpoint *> m_vectorEndpoints; 

    // user agent string
    CONST CHAR *m_szUserAgent;

    // main XHTTP session handle
    HINTERNET m_hSession;

    // auth engine thread
    HANDLE m_hEngineThread;

    // application shutdown
    BOOL m_fShutdown;

    // reused HR
    HRESULT hr;

    // user index used to aquire the token (NOTE: currently only handling single player scenarios)
    DWORD m_dwUserIndex;
};



//--------------------------------------------------------------------------------------
// class AuthManager::AuthEndpoint
//
//--------------------------------------------------------------------------------------
class AuthManager::AuthEndpoint
{

public:

    // states for the authendpoint state machine
    enum XAUTHENDPOINT_STATES
    {
        Idle,
        Init,
        SendingRequest,
        RequestSent,
        ReceivingResponse,
        ResponseHeaderAvailable,
        ReceivingResponseHeader,
        ReceivingResponseBody,
        ResponseDataAvailable,
        ResponseReceived,
        Completed,
        ErrorEncountered,
    };


    AuthEndpoint()
    {
        m_state = Idle;

        m_pToken = NULL;
        m_readBuffer = NULL;
        m_szETag = NULL;
        m_szContentTypeHeader = NULL;
        m_szHeader = NULL;

        m_pszUrl = NULL;
        m_pszVerb = NULL;
        m_pszRequestData = NULL;
        m_requestDataSize = 0;

        m_cbBytesRead = 0;
        m_cbBytesToRead = 0;

        m_dwHttpStatusCode = 0;
        m_dwContentLength = 0;
        m_dwContentLengthRemaining = 0;
        
        m_hSession = NULL;
        m_hConnect = NULL;
        m_hRequest = NULL;

        m_hAsyncResult.dwError = S_OK;

        m_dwFlags = 0;
    }

    ~AuthEndpoint()
    {
    }

    static AuthEndpoint * CreateEndpoint();
    HRESULT InitializeEndpoint(CONST CHAR * pszUrl, BOOL requireToken, HINTERNET session, DWORD userIndex);
    VOID DestroyEndpoint();
    HRESULT MakeSyncRequest(CHAR *pszVerb, CHAR *pszContentHeader, CHAR *pszData, DWORD cbSize);
    HRESULT OpenRequest(CHAR *pszVerb, CHAR *pszContentHeader, CHAR *pszData, DWORD cbSize);
    VOID CloseRequest();
    BOOL RequestCompleted() CONST;
    HRESULT DoWork();
    CONST BYTE *GetReadBuffer() CONST;
    CONST CHAR *GetETagHeader() CONST;   
    CONST DWORD GetHTTPStatusCode() CONST;
    
    HRESULT GetLastErrorCode(); 

    HRESULT hr;
    XHTTP_ASYNC_RESULT m_hAsyncResult;

private:

    XAUTHENDPOINT_STATES GetCurrentState() CONST;
    VOID MoveStateTo(CONST XAUTHENDPOINT_STATES state);
    VOID SetBytesRead(DWORD dwBytesRead);

    static VOID CALLBACK StatusCallback(HINTERNET hInternet,
                                        DWORD_PTR dwpContext,
                                        DWORD dwInternetStatus,
                                        LPVOID lpvStatusInformation,
                                        DWORD dwStatusInformationSize);
    HRESULT GetResponseStatusCode(HINTERNET hRequest,
                                  DWORD * pdwStatusCode);
    HRESULT GetResponseETagHeader(HINTERNET hRequest,
                                  CHAR * szETag);
    HRESULT GetResponseContentLength(HINTERNET hRequest,
                                     DWORD * pdwContentLength);
    HRESULT GetResponseHeader(HINTERNET hRequest,
                              DWORD dwInfoLevel,
                              CONST CHAR * pszHeader,
                              VOID * pvBuffer,
                              DWORD * pdwBufferLength);
    
    
    CONST CHAR * m_pszUrl;      // endpoint URL
    
    CHAR m_szHostName[MAX_HOSTNAME_LENGTH];
    BYTE *m_readBuffer;         // read buffer used for HTTP reading
    CHAR *m_szETag;             // buffer for ETag
    CHAR *m_szContentTypeHeader;// custom header to replace default "Content-Type" header
    CHAR *m_szHeader;           // request header (this generally contains the STS token, and additional header info like "Content-Type")
    PRELYING_PARTY_TOKEN m_pToken;
    
    CHAR *m_pszVerb;            // GET, PUT, POST, DELETE
    CHAR *m_pszRequestData;     // custom data to go with your verb
    DWORD m_requestDataSize;    // size of custom data

    DWORD m_cbBytesRead;        // member variable used to track how many bytes last read
    URL_COMPONENTS m_urlComp;   // XHttpCrackUrl populates the url components
    DWORD m_dwFlags; 

    DWORD m_dwHttpStatusCode;
    DWORD m_dwContentLength;
    DWORD m_dwContentLengthRemaining;
    DWORD m_cbBytesToRead;

    XAUTHENDPOINT_STATES m_state; 

    // HINTERNET handles
    HINTERNET m_hSession;
    HINTERNET m_hConnect;
    HINTERNET m_hRequest;
};


} // namespace HTTP
} // namespace ATG

#endif // ATGAUTH_H
