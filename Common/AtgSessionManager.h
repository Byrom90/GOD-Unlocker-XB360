//-----------------------------------------------------------------------------
// AtgSessionManager.h
//
// Class for managing all session-related activity for a single
// XSession instance
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------
const DWORD MODIFY_FLAGS_ALLOWED = XSESSION_CREATE_USES_ARBITRATION |
                                   XSESSION_CREATE_INVITES_DISABLED |
                                   XSESSION_CREATE_JOIN_VIA_PRESENCE_DISABLED |
                                   XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;

//------------------------------------------------------------------------
// Name: enum SessionState
// Desc: Session state enumeration       
//------------------------------------------------------------------------
enum SessionState
{
    SessionStateNone,
    SessionStateCreate,                 // XSessionCreate task created
    SessionStateCreating,               // XSessionCreate has been called
    SessionStateCreated,                // XSessionCreate has returned
    SessionStateIdle,                   // Session waiting for prompts
    SessionStateWaitingForRegistration, // Session waiting to register with arbitration
    SessionStateRegister,               // XSessionArbitrationRegister task created
    SessionStateRegistering,            // XSessionArbitrationRegister has been called
    SessionStateRegistered,             // XSessionArbitrationRegiste has returned
    SessionStateStart,                  // XSessionStart task created
    SessionStateStarting,               // XSessionStart has been called
    SessionStateInGame,                 // XSessionStart has returned
    SessionStateMigrateHost,            // XSessionMigrateHost task created
    SessionStateMigratingHost,          // XSessionMigrateHost has been called
    SessionStateMigratedHost,           // XSessionMigrateHost has returned
    SessionStateEnd,                    // XSessionEnd task created
    SessionStateEnding,                 // XSessionEnd has been called
    SessionStateEnded,                  // XSessionEnd has returned
    SessionStateDelete,                 // XSessionDelete task created
    SessionStateDeleting,               // XSessionDelete has been called, session is invalid
    SessionStateDeleted,                // XSessionDelete has returned, session is invalid
    SessionStateCount,                  // Count of SessionState enumeration values
};

static const char* g_astrSessionState[SessionStateCount] =
{
    "SessionStateNone",
    "SessionStateCreate",                 // XSessionCreate task created
    "SessionStateCreating",               // XSessionCreate has been called
    "SessionStateCreated",                // XSessionCreate has returned
    "SessionStateIdle",                   // Session waiting for prompts
    "SessionStateWaitingForRegistration", // Session waiting to register with arbitration
    "SessionStateRegister",               // XSessionArbitrationRegister task created
    "SessionStateRegistering",            // XSessionArbitrationRegister has been called
    "SessionStateRegistered",             // XSessionArbitrationRegiste has returned
    "SessionStateStart",                  // XSessionStart task created
    "SessionStateStarting",               // XSessionStart has been called
    "SessionStateInGame",                 // XSessionStart has returned
    "SessionStateMigrateHost",            // XSessionMigrateHost task created
    "SessionStateMigratingHost",          // XSessionMigrateHost has been called
    "SessionStateMigratedHost",           // XSessionMigrateHost has returned
    "SessionStateEnd",                    // XSessionEnd task created
    "SessionStateEnding",                 // XSessionEnd has been called
    "SessionStateEnded",                  // XSessionEnd has returned
    "SessionStateDelete",                 // XSessionDelete task created
    "SessionStateDeleting",               // XSessionDelete has been called, session is invalid
    "SessionStateDeleted"                 // XSessionDelete has returned, session is invalid
};


//------------------------------------------------------------------------
// Name: enum SessionState
// Desc: Slot types for the session       
//------------------------------------------------------------------------
typedef enum _SLOTS
{
    SLOTS_TOTALPUBLIC,
    SLOTS_TOTALPRIVATE,
    SLOTS_FILLEDPUBLIC,
    SLOTS_FILLEDPRIVATE,
    SLOTS_ZOMBIEPUBLIC,
    SLOTS_ZOMBIEPRIVATE,
    SLOTS_MAX
} SLOTS;

//------------------------------------------------------------------------
// Name: enum SessionCreationType
// Desc: Enum to track the reason for creation of this SessionManager instance
//------------------------------------------------------------------------
enum SessionCreationReason
{
    SessionCreationReasonNone,
    SessionCreationReasonAmbiguous,
    SessionCreationReasonHosting,
    SessionCreationReasonJoinFromSearch
};

//------------------------------------------------------------------------
// Name: struct SessionManagerInitParams
// Desc: Initialization parameters       
//------------------------------------------------------------------------
struct SessionManagerInitParams
{
    SessionCreationReason           m_SessionCreationReason;                
    DWORD                           m_dwSessionFlags;
    BOOL                            m_bIsHost;
    UINT                            m_dwMaxPublicSlots;
    UINT                            m_dwMaxPrivateSlots;
};

//--------------------------------------------------------------------------------------
// Name: class SessionManager
// Desc: Class for managing all XSession-related calls for a single LIVE session. 
//--------------------------------------------------------------------------------------
class SessionManager
{
public:

    SessionManager( VOID );
    virtual ~SessionManager( VOID );

    BOOL Initialize( SessionManagerInitParams initParams );
    BOOL IsInitialized( VOID );

    SessionCreationReason GetSessionCreationReason( VOID ) const;

    DWORD GetSessionOwner( VOID ) const;
    XUID GetSessionOwnerXuid( VOID ) const;
    VOID SetSessionOwner( const DWORD dwOwnerController );
    BOOL IsSessionOwner( const DWORD dwController ) const;
    BOOL IsSessionOwner( const XUID xuidOwner ) const;
    BOOL IsPlayerInSession( const XUID xuidPlayer, const DWORD dwUserIndex = (DWORD)-1 ) const;

    BOOL IsSessionHost( VOID ) const;
    VOID MakeSessionHost( VOID );

    ULONGLONG GetSessionNonce( VOID ) const;
    VOID SetSessionNonce( const ULONGLONG qwNonce );

    VOID SetHostInAddr( const IN_ADDR& inaddr );
    IN_ADDR GetHostInAddr( VOID ) const;

    XNKID GetSessionID( VOID ) const;
    __int64 GetSessionIDAsInt( VOID ) const;
    XNKID GetMigratedSessionID( VOID ) const;

    BOOL HasSessionFlags( const DWORD dwFlags ) const;
    DWORD GetSessionFlags( VOID ) const;

    VOID GetMaxSlotCounts( DWORD& dwMaxPublicSlots, DWORD& dwMaxPrivateSlots ) const;
    VOID GetFilledSlotCounts( DWORD& dwFilledPublicSlots, DWORD& dwFilledPrivateSlots ) const;

    VOID SetMaxSlotCounts( const DWORD dwMaxPublicSlots, const DWORD dwMaxPrivateSlots );
    VOID SetFilledSlotCounts( const DWORD dwFilledPublicSlots, const DWORD dwFilledPrivateSlots );

    const XSESSION_INFO& GetSessionInfo( VOID ) const;
    VOID SetSessionInfo( const XSESSION_INFO& session_info );
    VOID SetNewSessionInfo( const XSESSION_INFO& session_info, const BOOL bIsNewSessionHost );

    VOID SetSessionFlags( const DWORD dwFlags, const BOOL fClearExisting = FALSE );
    VOID FlipSessionFlags( const DWORD dwFlags );
    VOID ClearSessionFlags( const DWORD dwFlags );

    WCHAR* GetSessionError( VOID ) const;
    VOID SetSessionError( WCHAR* error );

    SessionState GetSessionState( VOID ) const;
    const char* GetSessionStateString( VOID ) const;
    VOID SwitchToState( const SessionState newState );
    VOID SwitchToPreHostMigrationState();

    const PXSESSION_REGISTRATION_RESULTS GetRegistrationResults() const;

    VOID StartQoSListener( BYTE* data, const UINT dataLen, const DWORD bitsPerSec );

    BOOL IsProcessingOverlappedOperation( VOID ) const;
    DWORD GetOverlappedExtendedError( VOID ) const;

    BOOL IsSessionDeleted() const;

    VOID DebugDumpSessionDetails( VOID ) const;

    BOOL DebugValidateExpectedSlotCounts( const BOOL bBreakOnDifferent = TRUE ) const;

    //
    // Following functions call XSession APIs synchronously or asynchronously
    //
    HRESULT CreateSession( XOVERLAPPED* pXOverlapped = NULL );
    HRESULT StartSession( XOVERLAPPED* pXOverlapped = NULL );
    HRESULT EndSession( XOVERLAPPED* pXOverlapped = NULL );    
    HRESULT DeleteSession( XOVERLAPPED* pXOverlapped = NULL );

    HRESULT AddLocalPlayers( const DWORD dwUserCount,
                             DWORD* pdwUserIndices,
                             const BOOL* pfPrivateSlots,
                             XOVERLAPPED* pXOverlapped = NULL );

    HRESULT AddRemotePlayers( const DWORD dwXuidCount,
                              XUID* pXuids,
                              const BOOL* pfPrivateSlots,
                              XOVERLAPPED* pXOverlapped = NULL );

    HRESULT RemoveLocalPlayers( const DWORD dwUserCount,
                                const DWORD* pdwUserIndices,
                                const BOOL* pfPrivateSlots,
                                XOVERLAPPED* pXOverlapped = NULL );

    HRESULT RemoveRemotePlayers( const DWORD dwXuidCount,
                                 const XUID* pXuids,
                                 const BOOL* pfPrivateSlots,
                                 XOVERLAPPED* pXOverlapped = NULL );

    HRESULT ModifySkill( const DWORD dwNumXuids, const XUID* pXuids, XOVERLAPPED* pXOverlapped = NULL );

    HRESULT ModifySessionFlags( const DWORD dwFlags, const BOOL bClearFlags = FALSE, XOVERLAPPED* pXOverlapped = NULL );

    HRESULT WriteStats( const XUID xuid, const DWORD dwNumViews, const XSESSION_VIEW_PROPERTIES *pViews, XOVERLAPPED* pXOverlapped = NULL );

    HRESULT MigrateSession( XOVERLAPPED* pXOverlapped = NULL );

    HRESULT RegisterArbitration( XOVERLAPPED* pXOverlapped = NULL );

    HRESULT NotifyOverlappedOperationCancelled( const XOVERLAPPED* const pXOverlapped );

protected:
    VOID Reset( VOID );
    VOID SetSessionState( const SessionState newState );
    BOOL CheckSessionState( const SessionState expectedState ) const;
    VOID StopQoSListener();
    
private:
    SessionManager& operator=( const SessionManager& ref ) {}

protected:
    SessionCreationReason   m_SessionCreationReason;    // Creation reason
    BOOL                    m_bIsInitialized;           // Initialized?
    BOOL                    m_bIsHost;                  // Is hosting
    BOOL                    m_bUsingQoS;                // Is the QoS listener enabled
    DWORD                   m_dwOwnerController;        // Which controller created the session
    XUID                    m_xuidOwner;                // XUID of gamer who created the session
    WCHAR*                  m_strSessionError;          // Error message for current session
    ULONGLONG               m_qwSessionNonce;           // Session nonce
    XSESSION_INFO           m_SessionInfo;              // Session ID, key, and host address 
    XSESSION_INFO           m_NewSessionInfo;           // New session ID, key, and host address 
    BOOL                    m_bIsMigratedSessionHost;   // Is hosting migrated session?
    XNKID                   m_migratedSessionID;        // Session ID of migrated session
    HANDLE                  m_hSession;                 // Session handle
    SessionState            m_SessionState;             // Current session state
    SessionState            m_migratedSessionState;     // Session state of migrated session
    DWORD                   m_dwSessionFlags;           // Flags for the current session  exit?
    UINT                    m_Slots[ SLOTS_MAX ];       // Filled/open slots for the session
    IN_ADDR                 m_HostInAddr;               // Host IP address
    
    PXSESSION_REGISTRATION_RESULTS m_pRegistrationResults;
};
