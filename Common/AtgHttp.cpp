//--------------------------------------------------------------------------------------
// ATGHttp
//
// Uses XHttp/XAuth to manage HTTP(s) connections and XSTS tokens 
// 
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "stdafx.h"

#include "AtgHttp.h"
#include "AtgUtil.h"

#define PROXY_TYPE XHTTP_ACCESS_TYPE_DEFAULT_PROXY

// shared token if requested during init
PRELYING_PARTY_TOKEN g_pSharedToken = NULL;

// the auth manager can cache a single token currently, alternatively each endpoint will get its own token
BOOL g_useSharedToken = FALSE;


namespace ATG
{
namespace HTTP
{

CHAR *STATES[] = {
    "Idle",
    "Init",
    "SendingRequest",
    "RequestSent",
    "ReceivingResponse",
    "ResponseHeaderAvailable",
    "ReceivingResponseHeader",
    "ReceivingResponseBody",
    "ResponseDataAvailable",
    "ResponseReceived",
    "Completed",
    "ErrorEncountered",
};

static CONST CHAR * STS_TOKEN_HEADER = "Authorization: XBL2.0 x="; // xboxlive specific header used for RESTful services
static CONST CHAR * STS_TOKEN_HEADER_END = "\r\n";

// NOTE: defaults added to header unless alternative is specifed in the calls
static CONST CHAR * CONTENT_TYPE_HEADER = "Content-Type: application/json\r\n";
static CONST CHAR * CONTRACT_VERSION_HEADER = "x-xbl-contract-version: 1\r\n";


//--------------------------------------------------------------------------------------
// static EngineThread
//
// static routine that simply take a this pointer parameter then calls into the real 
// object's RealEngineThread routine
//--------------------------------------------------------------------------------------
DWORD AuthManager::EngineThread (LPVOID lpThreadParameter)
{
    AuthManager *manager = (AuthManager *) lpThreadParameter;

    if (manager == NULL)
    {
        ATG::FatalError("[XAUTHMANAGER] null manager pointer passed\n");
    }

    return manager->RealEngineThread();
}


//--------------------------------------------------------------------------------------
// RealEngineThread
//
// worker thread for auth manager.  This thread ensures we consistently pump the dowork
// routines.
//--------------------------------------------------------------------------------------
DWORD AuthManager::RealEngineThread()
{
    while (!m_fShutdown)
    {
        Sleep(33); 

        HRESULT hr = DoWork();
        if (hr != S_OK)
        {
            ATG::DebugSpew("[XAUTHMANAGER] DoWork failure: %x\n", hr);
        }
    }
    
    return 0;
}


//--------------------------------------------------------------------------------------
// AuthManager::Startup
//
// initialize the XHTTP and XAuth libraries.
//--------------------------------------------------------------------------------------
HRESULT AuthManager::Startup(CHAR * pszCustomUserAgent, BOOL fBypassSecurity, HANDLE *workerThread, BOOL fSharedToken, DWORD dwUserIndex)
{
    hr = S_OK;
    XAUTH_SETTINGS settings;

    // specify if we are going to use a common token for all endpoints or if each gets its own 
    // NOTE: currently only caching a single token and also only dealing with single player scenarios (one user token)
    g_useSharedToken = fSharedToken;
    m_dwUserIndex = dwUserIndex;

    if (!XHttpStartup(0, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    memset(&settings, 0, sizeof(settings));
    settings.SizeOfStruct = sizeof(settings);
    settings.Flags = 0;
    if (fBypassSecurity)
    {
        settings.Flags |= XAUTH_FLAG_BYPASS_SECURITY;
    }

    hr = XAuthStartup(&settings);
    if (FAILED(hr))
    {
        return hr;
    }

    if ( pszCustomUserAgent != NULL )
    {
        m_szUserAgent = pszCustomUserAgent;
    }

    // open up an XHTTP session
    m_hSession = XHttpOpen(m_szUserAgent,
                           PROXY_TYPE,
                           NULL,
                           NULL,
                           XHTTP_FLAG_ASYNC);
    if (m_hSession == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    DWORD dwThreadId = 0;
    m_hEngineThread = CreateThread(NULL, 0, EngineThread, this, 0, &dwThreadId);
    *workerThread = m_hEngineThread;

    return hr;
}


//--------------------------------------------------------------------------------------
// AuthManager::Shutdown
//
// call this routine to shutdown the auth manager
//--------------------------------------------------------------------------------------
VOID AuthManager::Shutdown()
{
    // shutdown the main engine pump thread
    if (m_hEngineThread != NULL)
    {
        m_fShutdown = TRUE;
        WaitForSingleObject(m_hEngineThread,INFINITE);
        CloseHandle(m_hEngineThread);
    }
    
    if (m_hSession != NULL)
    {
        XHttpCloseHandle(m_hSession);
        m_hSession = NULL;
    }

    for (DWORD index=0; index < m_vectorEndpoints.size(); index++)
    {
        m_vectorEndpoints[index]->DestroyEndpoint();
        delete m_vectorEndpoints[index]; 
    }

    XAuthShutdown();
    XHttpShutdown();
}


//--------------------------------------------------------------------------------------
// AuthManager::CreateEndpoint
//
//--------------------------------------------------------------------------------------
AuthManager::AuthEndpoint * AuthManager::CreateEndpoint(CONST CHAR * pszUrl, BOOL requireToken)
{
    AuthEndpoint * endpoint = NULL;
    hr = S_OK;

    if (pszUrl == NULL)
    {
        ATG::DebugSpew("[XAUTHMANAGER] CreateEndpoint failure of NULL URL\n");
        return NULL;
    }

    endpoint = AuthEndpoint::CreateEndpoint(); 
    hr = endpoint->InitializeEndpoint(pszUrl, requireToken, m_hSession, m_dwUserIndex);

    if ( hr == S_OK )
    {
        m_vectorEndpoints.push_back(endpoint);
    }
    else
    {
        endpoint->DestroyEndpoint();
        endpoint = NULL;
    }

    return endpoint;
}


//--------------------------------------------------------------------------------------
// AuthManager::RemoveEndpoint
//
// routine removes endpoint from the endpoint vector and destroys it
//--------------------------------------------------------------------------------------
VOID AuthManager::RemoveEndpoint(AuthEndpoint *endpoint)
{
    endpoint->CloseRequest();
        
    // remove endpoint from the worker array
    for (DWORD i=0; i<m_vectorEndpoints.size(); i++) 
    {
        if (m_vectorEndpoints[i] == endpoint)
        {
            m_vectorEndpoints.erase(m_vectorEndpoints.begin() + i);
            endpoint->DestroyEndpoint();

            break;
        }
    }

    delete endpoint;
}


//--------------------------------------------------------------------------------------
// AuthManager::DoWork
//
// This routine ensures that all the related handles are closed for the endpoint
//--------------------------------------------------------------------------------------
HRESULT AuthManager::DoWork()
{
   HRESULT hr = S_OK;

    for (DWORD index=0; index < m_vectorEndpoints.size(); index++)
    {
        if (m_vectorEndpoints[index] != NULL)
        {
            hr = m_vectorEndpoints[index]->DoWork();
            if (hr != S_OK)
            {
                return hr;
            }
        }
    }

    return hr;
}


// AuthEndpoint ROUTINES ///////////////////////////////////////////////////////////////////////////////////////////
AuthManager::AuthEndpoint * AuthManager::AuthEndpoint::CreateEndpoint()
{
    return new AuthEndpoint();
}

BOOL AuthManager::AuthEndpoint::RequestCompleted() CONST
{
    return (m_state == Completed || m_state == ErrorEncountered);
}

VOID AuthManager::AuthEndpoint::MoveStateTo(CONST XAUTHENDPOINT_STATES state)
{
#ifdef DEBUGMODE
    ATG::DebugSpew("[XAUTHMANAGER] state change to: %s\n", STATES[state]); 
#endif
    m_state = state;
}

VOID AuthManager::AuthEndpoint::SetBytesRead(DWORD dwBytesRead)
{
    m_cbBytesRead = dwBytesRead;
}

AuthManager::AuthEndpoint::XAUTHENDPOINT_STATES AuthManager::AuthEndpoint::GetCurrentState() CONST
{
    return m_state;
}

CONST BYTE *AuthManager::AuthEndpoint::GetReadBuffer() CONST
{
    return m_readBuffer;
}

CONST DWORD AuthManager::AuthEndpoint::GetHTTPStatusCode() CONST
{
    return m_dwHttpStatusCode;
}

CONST CHAR *AuthManager::AuthEndpoint::GetETagHeader() CONST     
{
    if(strlen(m_szETag) == 0)
    {
        return NULL;
    }

    return m_szETag;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::InitializeEndpoint
//
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::InitializeEndpoint(CONST CHAR * pszUrl, BOOL requireToken, HINTERNET session, DWORD userIndex)
{
    hr = S_OK;

    if (pszUrl == NULL)
    {
        return E_INVALIDARG;
    }

    // if the URL will need an STS token, we need to query for it and store it until we make the actual request
    if (requireToken && g_pSharedToken == NULL)
    {
        if (m_pToken == NULL)
        {
            hr = XAuthGetToken(
                userIndex, 
                pszUrl,
                strlen(pszUrl),
                &m_pToken,
                NULL);

            if (FAILED(hr))
            {
                ATG::DebugSpew("[XAUTHMANAGER] failed to get a token %x\n", hr); 
                return hr;
            }

            if (g_useSharedToken == TRUE)
            {
                g_pSharedToken = m_pToken;
            }
        }
    }
    else if (requireToken)
    {
        m_pToken = g_pSharedToken;
    }

    // allocate the required endpoint memory here... read and header buffers
    m_readBuffer = (BYTE *) malloc(RECEIVE_BUFFER_SIZE);
    m_szETag = (CHAR *) malloc(MAX_HEADER_LENGTH);
    m_szHeader = (CHAR *) malloc(MAX_HEADER_LENGTH);
    // end allocations.

    m_hSession = session; 

    // initialize request data
    m_pszUrl = pszUrl;
    m_pszVerb = NULL;
    m_pszRequestData = NULL;
    m_requestDataSize = 0;

    memset(m_szHostName, 0, MAX_HOSTNAME_LENGTH);
    memset(m_readBuffer, 0, RECEIVE_BUFFER_SIZE);
    memset(m_szETag, 0, MAX_HEADER_LENGTH);
    memset(m_szHeader, 0, MAX_HEADER_LENGTH);
    memset(&m_urlComp, 0, sizeof(m_urlComp));

    m_urlComp.dwStructSize = sizeof(m_urlComp);
    m_urlComp.lpszHostName = m_szHostName;
    m_urlComp.dwHostNameLength = MAX_HOSTNAME_LENGTH;
    m_urlComp.lpszUrlPath = NULL;
    m_urlComp.dwUrlPathLength = (DWORD) -1;
    
    // crack the URL into url components
    if (!XHttpCrackUrl(m_pszUrl,
                      strlen(m_pszUrl),
                      (ICU_ESCAPE | ICU_DECODE),
                      &m_urlComp))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    if (m_urlComp.nScheme == INTERNET_SCHEME_HTTPS)
    {
        m_dwFlags |= XHTTP_FLAG_SECURE;
    }


    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::DestroyEndpoint
//
//--------------------------------------------------------------------------------------
VOID AuthManager::AuthEndpoint::DestroyEndpoint()
{
    if (m_readBuffer != NULL)
    {
        free(m_readBuffer);
    }

    if (m_szETag != NULL)
    {
        free(m_szETag);
    }

    if (m_szHeader != NULL)
    {
        free(m_szHeader);
    }

    if (m_pToken != NULL && g_useSharedToken == FALSE)
    {
        XAuthFreeToken(m_pToken);
    }
}


//--------------------------------------------------------------------------------------
// 
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::MakeSyncRequest(CHAR *pszVerb, CHAR *pszContentHeader, CHAR *pszData, DWORD cbSize)
{
    HRESULT hr = OpenRequest(pszVerb, pszContentHeader, pszData, cbSize);

    while (!RequestCompleted())
    {
        // Here we are polling the endpoint waiting for the request to be completed
        Sleep(33);
    }

#ifdef DEBUGMODE
    ATG::DebugSpew("[XAUTHMANAGER] HTTP response %d\n",GetHTTPStatusCode());
    ATG::DebugSpew("[XAUTHMANAGER] %s\n", GetReadBuffer());
#endif

    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::OpenRequest
//
// this routine establishes an endpoint request for a URL
//
// If there is custom data to add to the GET/PUT/POST/DELETE/... request you can 
// pass it in here as well.  
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::OpenRequest(CHAR *pszVerb, CHAR *pszContentHeader, CHAR *pszData, DWORD cbSize)
{
    hr = S_OK;

    if (pszVerb == NULL)
    {
        return E_INVALIDARG;
    }

    if (m_state != Idle)
    {
        DebugSpew("[XAUTHMANAGER] failed to open request\n");
        return E_UNEXPECTED; 
    }

    m_pszVerb = pszVerb;
    m_pszRequestData = pszData;
    m_requestDataSize = cbSize;

    // connect to the hostname/port for the session object
    m_hConnect = XHttpConnect(m_hSession,
                              m_urlComp.lpszHostName,
                              m_urlComp.nPort,
                              m_dwFlags);
    if (m_hConnect == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // open the actual request and specify the action verb and url path
    // however the final request will not be submitted until XhttpSendRequest occurs
    m_hRequest = XHttpOpenRequest(m_hConnect,
                                  m_pszVerb, 
                                  m_urlComp.lpszUrlPath,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0);
    if (m_hRequest == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // specify the callback routine to be used by XHTTP
    XHttpSetStatusCallback(m_hRequest,
                           StatusCallback,
                           XHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
                           NULL);

    // set the custom header if specified
    m_szContentTypeHeader = pszContentHeader;

    // put the auth manager state machine in the "Init" state
    MoveStateTo(Init);
    
    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::CloseRequest
//
// This routine ensures that all the related handles are closed for the endpoint
//--------------------------------------------------------------------------------------
VOID AuthManager::AuthEndpoint::CloseRequest()
{
    if (m_hRequest != NULL)
    {
        XHttpCloseHandle(m_hRequest);
        m_hRequest = NULL;
    }

    if (m_hConnect != NULL)
    {
        XHttpCloseHandle(m_hConnect);
        m_hConnect = NULL;
    }

    m_state = Idle;
    m_readBuffer[0] = 0;
    m_szETag[0] = 0;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::DoWork
//
// routine houses the core processing state machine for the endpoint request
// this routine also is used to do the DoWork pumping for XHTTP
//
// we need the statemachine below due to the asyncronous nature of XHTTP calls
// and coordinating with the XHTTP callback.
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::DoWork()
{
    HRESULT hr = S_OK;
    
    switch(m_state)
    {
        case Idle:
        {
            // not expected to do anything
        }
        break;
        
        case Init:
        {
            CHAR * pszHeader = NULL;
            DWORD cbHeader = 0;
            CHAR * pszCur = m_szHeader;

            m_szHeader[0] = 0; 

            if (m_pToken != NULL) // use a token
            {
                strcat_s(pszCur, MAX_HEADER_LENGTH, STS_TOKEN_HEADER);
                strcat_s(pszCur, MAX_HEADER_LENGTH, (CONST CHAR *)m_pToken->pToken);
                strcat_s(pszCur, MAX_HEADER_LENGTH, STS_TOKEN_HEADER_END);

                if ( m_szContentTypeHeader != NULL )
                {
                    strcat_s(pszCur, MAX_HEADER_LENGTH, m_szContentTypeHeader);
                }
                else
                {
                    strcat_s(pszCur, MAX_HEADER_LENGTH, CONTENT_TYPE_HEADER);
                    strcat_s(pszCur, MAX_HEADER_LENGTH, CONTRACT_VERSION_HEADER); 
                }

                cbHeader = strlen(pszCur); 
                pszHeader = m_szHeader;
            }
            else 
            {
                if ( m_szContentTypeHeader != NULL )
                {
                    strcat_s(pszCur, MAX_HEADER_LENGTH, m_szContentTypeHeader);
                }
                else
                {
                    strcat_s(pszCur, MAX_HEADER_LENGTH, CONTENT_TYPE_HEADER);
                    strcat_s(pszCur, MAX_HEADER_LENGTH, CONTRACT_VERSION_HEADER); 
                }

                cbHeader = strlen(pszCur); 
                pszHeader = m_szHeader;
            }

            // here we pass the header and the initial custom request data
            // NOTE: totalsize will always be custom data size due to keeping the class simple and keeping the request as a single call
            if (XHttpSendRequest(m_hRequest,
                pszHeader,
                cbHeader,
                m_pszRequestData, 
                m_requestDataSize, 
                m_requestDataSize, 
                (DWORD_PTR)this))
            {
                MoveStateTo(SendingRequest);
            }
            else
            {
                DebugSpew("[XAUTHMANAGER] Cannot send request!\n");
                MoveStateTo(ErrorEncountered);
            }
        } 
        break;

        case SendingRequest:
        {
            // statemachine will be updated in the callback
        }
        break;

        case RequestSent:
        {
            if (XHttpReceiveResponse(m_hRequest, NULL))
            {
                MoveStateTo(ReceivingResponseHeader);
            }
            else
            {
                DebugSpew("[XAUTHMANAGER] Cannot receive response!\n");
                MoveStateTo(ErrorEncountered);
            }
        }
        break;

        case ReceivingResponse:
        {
            // statemachine will be updated in the callback
        }
        break;

        case ReceivingResponseHeader:
        {
            // statemachine will be updated in the callback, expected move: ReponseHeaderAvailable
        }
        break;
                                         
        case ResponseHeaderAvailable:
        {
            hr = GetResponseETagHeader(m_hRequest, m_szETag);
            if(hr != HRESULT_FROM_WIN32(ERROR_XHTTP_HEADER_NOT_FOUND) && FAILED(hr))
            {
                 DebugSpew("[XAUTHMANAGER] GetResponseETagHEader failed! hr=0x%x!\n", hr);
                 MoveStateTo(ErrorEncountered);
            }

            hr = GetResponseStatusCode(m_hRequest, &m_dwHttpStatusCode);
            if (FAILED(hr))
            {
                DebugSpew("[XAUTHMANAGER] GetResponseStatusCode failed! hr=0x%x!\n", hr);
                MoveStateTo(ErrorEncountered);
            }
            else if (m_dwHttpStatusCode >= HTTP_STATUS_BAD_REQUEST)
            {
                DebugSpew("[XAUTHMANAGER] Server returned error status code %d!\n", m_dwHttpStatusCode);
                MoveStateTo(ErrorEncountered);
            }
            else // NOTE: if we get a 403/404 we do still have data that maybe helpful
            {
                hr = GetResponseContentLength(m_hRequest, &m_dwContentLength);
                if (FAILED(hr))
                {
                    DebugSpew("[XAUTHMANAGER] GetResponseContentLength failed hr=0x%x!\n", hr);
                    MoveStateTo(ErrorEncountered);
                }
                else if (m_dwContentLength > 0)
                {
#ifdef DEBUGMODE
                    DebugSpew("[XAUTHMANAGER] GetResponseContentLength size =%d!\n", m_dwContentLength);
#endif
                    m_dwContentLengthRemaining = m_dwContentLength;
                    MoveStateTo(ReceivingResponseBody);
                }
                else
                {
                    MoveStateTo(ResponseReceived);
                }
            }
        }
        break;

        case ReceivingResponseBody:
        {
            MoveStateTo(ReceivingResponse);
            m_cbBytesToRead = (m_dwContentLengthRemaining < RECEIVE_BUFFER_SIZE) ? m_dwContentLengthRemaining : RECEIVE_BUFFER_SIZE;
#ifdef DEBUGMODE
            DebugSpew("[XAUTHMANAGER] bytes to read %d\n", m_cbBytesToRead);
#endif
            if (m_cbBytesToRead == 0)
            {
                MoveStateTo(ResponseReceived);
            }
            else if (!XHttpReadData(m_hRequest,
                                    m_readBuffer,
                                    m_cbBytesToRead,
                                    NULL))
            {
                DebugSpew("[XAUTHMANAGER] failed to read repsonse body!\n");
                MoveStateTo(ErrorEncountered);
            }
        }
        break;

        case ResponseDataAvailable:
        {
            m_dwContentLengthRemaining -= m_cbBytesRead;
            DebugSpew("buffer: %s\n", m_readBuffer);
            MoveStateTo(ReceivingResponseBody);
        }
        break;

        case ResponseReceived:
        {
            MoveStateTo(Completed);
        }
        break;

        case ErrorEncountered: __fallthrough
        case Completed:
        {
            // final state
            return HRESULT_FROM_WIN32(m_hAsyncResult.dwError);
        }
        break;

        default:
        {
            FatalError("[XAUTHMANAGER] invalid state\n");
        }
        break;
    }

    assert(m_hSession != NULL);

    if (!XHttpDoWork(m_hSession, 0))
    {
        MoveStateTo(ErrorEncountered);
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::StatusCallback
//
// routine is the sole callback for XHTTP and is called during DoWork pumps.
// this routine is used to change the core state of the processing state machine
// based on the XHTTP_CALLBACK_STATUS recieved
//--------------------------------------------------------------------------------------
VOID CALLBACK AuthManager::AuthEndpoint::StatusCallback(HINTERNET hInternet,
                                                        DWORD_PTR dwpContext,
                                                        DWORD dwInternetStatus,
                                                        LPVOID lpvStatusInformation,
                                                        DWORD dwStatusInformationSize)
{
    AuthEndpoint * pxas = (AuthEndpoint *) dwpContext; 
    assert(pxas != NULL);
    
    switch (dwInternetStatus)
    {
        case XHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            pxas->MoveStateTo(RequestSent);
        }
        break;

        case XHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            assert(pxas->GetCurrentState() == ReceivingResponseHeader);
            pxas->MoveStateTo(ResponseHeaderAvailable);
        }
        break;

        case XHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            pxas->SetBytesRead(dwStatusInformationSize);
#ifdef DEBUGMODE
            ATG::DebugSpew("[XAUTHMANAGER] read complete with sizeof: %d\n", dwStatusInformationSize); 
#endif
            pxas->MoveStateTo(ResponseDataAvailable);
        }
        break;

        case XHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            memcpy_s( &pxas->m_hAsyncResult, sizeof(XHTTP_ASYNC_RESULT), (XHTTP_ASYNC_RESULT*) lpvStatusInformation, sizeof(XHTTP_ASYNC_RESULT) );

            DebugSpew("[XAUTHMANAGER] encountered request error %x!\n", pxas->m_hAsyncResult.dwError);
            pxas->MoveStateTo(ErrorEncountered);
        }
        break;

        case XHTTP_CALLBACK_STATUS_WRITE_COMPLETE: __fallthrough 
        case XHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE: 
        {
            // do nothing
            DebugSpew("[XAUTHMANAGER] Unsupported state!\n");
            pxas->MoveStateTo(ErrorEncountered);
        }
        break;
        
        case XHTTP_CALLBACK_STATUS_REDIRECT: __fallthrough
        case XHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            // do nothing
        }
        break;
        
        default:
        {
            FatalError("[XAUTHMANAGER] invalid state\n");
        }
    }
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::GetResponseStatusCode
//
// call AuthManager::GetResponseHeader to retrieve the HTTP status code 
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::GetResponseStatusCode(
    HINTERNET hRequest,
    DWORD * pdwStatusCode
)
{
    hr = S_OK;
    DWORD dwBufferLength = sizeof(DWORD);
    
    assert(pdwStatusCode != NULL);
    hr = GetResponseHeader(hRequest,
                           (XHTTP_QUERY_STATUS_CODE | XHTTP_QUERY_FLAG_NUMBER),
                           XHTTP_HEADER_NAME_BY_INDEX,
                           pdwStatusCode,
                           &dwBufferLength);
    
    return hr;
}

//--------------------------------------------------------------------------------------
// AuthEndpoint::GetResponseStatusCode
//
// call AuthManager::GetResponseHeader to retrieve the HTTP ETag header 
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::GetResponseETagHeader(
    HINTERNET hRequest,
    CHAR * szETag
)
{
    hr = S_OK;
    DWORD dwBufferLength = MAX_HEADER_LENGTH;

    assert(szETag != NULL);
    hr = GetResponseHeader(hRequest,
                           XHTTP_QUERY_CUSTOM,
                           "ETag",
                           szETag,
                           &dwBufferLength);
    
    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::GetLastErrorCode
//
// call AuthManager::GetLastErrorCode to retrieve and clear the last error status code
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::GetLastErrorCode()
{
    hr = m_hAsyncResult.dwError;

    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::GetResponseContentLength
//
// call AuthManager::GetResponseHeader to retrieve the content length of the response
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::GetResponseContentLength(
    HINTERNET hRequest,
    DWORD * pdwContentLength
)
{
    hr = S_OK;
    DWORD dwBufferLength = sizeof(DWORD);
    assert(pdwContentLength != NULL);
    hr = GetResponseHeader(hRequest,
                           (XHTTP_QUERY_CONTENT_LENGTH | XHTTP_QUERY_FLAG_NUMBER),
                           XHTTP_HEADER_NAME_BY_INDEX,
                           pdwContentLength,
                           &dwBufferLength);

    return hr;
}


//--------------------------------------------------------------------------------------
// AuthEndpoint::GetResponseHeader
//
// this routine is mainly used for GetResponseStatusCode and GetResponseContentLength
// routine uses XHttpQueryHeaders
//--------------------------------------------------------------------------------------
HRESULT AuthManager::AuthEndpoint::GetResponseHeader(
    HINTERNET hRequest,
    DWORD dwInfoLevel,
    CONST CHAR * pszHeader,
    VOID * pvBuffer,
    DWORD * pdwBufferLength
    )
{
    hr = S_OK;
    
    assert(pdwBufferLength != NULL);

    if (!XHttpQueryHeaders(hRequest,
                           dwInfoLevel,
                           pszHeader,
                           pvBuffer,
                           pdwBufferLength,
                           XHTTP_NO_HEADER_INDEX)) 
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

} // namespace HTTP
} // namespace ATG

