#include "stdafx.h"
#include "AtgUtil.h"
#include "AtgSessionManager.h"
#define DebugSpew ATG::DebugSpew
#define FatalError ATG::FatalError
#define XNKIDToInt64 ATG::XNKIDToInt64

//------------------------------------------------------------------------
// Name: SessionManager()
// Desc: Public constructor
//------------------------------------------------------------------------
SessionManager::SessionManager() 
{
    Reset();
}

//------------------------------------------------------------------------
// Name: Reset()
// Desc: Resets this SessionManager instance to its creation state
//------------------------------------------------------------------------
VOID SessionManager::Reset()
{
    m_bIsInitialized            = FALSE;
    m_SessionCreationReason     = SessionCreationReasonNone;
    m_dwSessionFlags            = 0;
    m_dwOwnerController         = XUSER_MAX_COUNT + 1;
    m_xuidOwner                 = INVALID_XUID;
    m_SessionState              = SessionStateNone;
    m_migratedSessionState      = SessionStateNone;
    m_bIsMigratedSessionHost    = FALSE;
    m_hSession                  = INVALID_HANDLE_VALUE;
    m_qwSessionNonce            = 0;
    m_strSessionError           = NULL;
    m_bUsingQoS                 = FALSE;
    m_pRegistrationResults      = NULL;
    ZeroMemory( &m_SessionInfo, sizeof( XSESSION_INFO ) );
    ZeroMemory( &m_NewSessionInfo, sizeof( XSESSION_INFO ) );
    ZeroMemory( &m_migratedSessionID, sizeof( XNKID ) );    
}


//------------------------------------------------------------------------
// Name: ~SessionManager()
// Desc: Public destructor
//------------------------------------------------------------------------
SessionManager::~SessionManager()
{
    // Close our XSession handle to clean up resources held by it
    if( m_hSession != INVALID_HANDLE_VALUE && 
        m_hSession != NULL )
    {
        XCloseHandle( m_hSession );
    }
}


//------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes this SessionManager instance
//------------------------------------------------------------------------
BOOL SessionManager::Initialize( SessionManagerInitParams initParams )
{
    if( m_bIsInitialized )
    {
        DebugSpew( "SessionManager already initialized. Reinitializing..\n" );
    }

    m_SessionCreationReason         = initParams.m_SessionCreationReason;
    m_dwSessionFlags                = initParams.m_dwSessionFlags;
    m_bIsHost                       = initParams.m_bIsHost;
    m_Slots[SLOTS_TOTALPUBLIC]      = initParams.m_dwMaxPublicSlots;
    m_Slots[SLOTS_TOTALPRIVATE]     = initParams.m_dwMaxPrivateSlots;
    m_Slots[SLOTS_FILLEDPUBLIC]     = 0;
    m_Slots[SLOTS_FILLEDPRIVATE]    = 0;
    m_Slots[SLOTS_ZOMBIEPUBLIC]     = 0;
    m_Slots[SLOTS_ZOMBIEPRIVATE]    = 0;

    m_bIsInitialized = TRUE;
    return TRUE;
}

//------------------------------------------------------------------------
// Name: IsInitialized()
// Desc: Queries SessionManager initialization state
//------------------------------------------------------------------------
BOOL SessionManager::IsInitialized()
{
    return m_bIsInitialized;
}

//------------------------------------------------------------------------
// Name: IsSessionDeleted()
// Desc: Returns TRUE if the XSession for this instance
//       has a 0 sessionID; otherwise false
//------------------------------------------------------------------------
BOOL SessionManager::IsSessionDeleted( VOID ) const
{
    static const XNKID zero = {0};

    if( m_SessionState == SessionStateDeleted )
    {
        return TRUE;
    }

    BOOL bIsSessionDeleted = FALSE;

    if( m_hSession == INVALID_HANDLE_VALUE || m_hSession == NULL )
    {
        // If the session hasn't been created yet, return false
        if ( m_SessionState < SessionStateCreated )
        {
            bIsSessionDeleted = FALSE;
        }
        else
        {
            bIsSessionDeleted = TRUE;
        }

        return bIsSessionDeleted;
    }

    // Call XSessionGetDetails first time to get back the size 
    // of the results buffer we need
    DWORD cbResults = 0;

    DWORD ret = XSessionGetDetails( m_hSession,
                                    &cbResults,
                                    NULL,
                                    NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ) )
    {
        DebugSpew( "SessionManager::IsSessionDeleted - Failed on first call to XSessionGetDetails, hr=0x%08x\n", ret );
        return FALSE;
    }
    
    XSESSION_LOCAL_DETAILS* pSessionDetails = (XSESSION_LOCAL_DETAILS*)new BYTE[ cbResults ];
    if( pSessionDetails == NULL )
    {
        FatalError( "SessionManager::IsSessionDeleted - Failed to allocate buffer.\n" );
    }

    // Call second time to fill our results buffer
    ret = XSessionGetDetails( m_hSession,
                              &cbResults,
                              pSessionDetails,
                              NULL ); 

    if( ret != ERROR_SUCCESS )
    {
        DebugSpew( "SessionManager::IsSessionDeleted - XSessionGetDetails failed with error %d\n", ret );
        return FALSE;
    }

    // Iterate through the returned results
    const XNKID sessionID = pSessionDetails->sessionInfo.sessionID;

    if ( !memcmp( &sessionID, &zero, sizeof( sessionID ) ) )
    {
        bIsSessionDeleted = TRUE;
    }

    if ( pSessionDetails )
    {
        delete [] pSessionDetails;
    }

    return bIsSessionDeleted;
}

//------------------------------------------------------------------------
// Name: IsPlayerInSession()
// Desc: Uses XSessionGetDetails API to determine if a player is in the session
//------------------------------------------------------------------------
BOOL SessionManager::IsPlayerInSession( const XUID xuidPlayer, const DWORD dwUserIndex ) const
{
    if( m_hSession == INVALID_HANDLE_VALUE || m_hSession == NULL )
    {
        DebugSpew( "SessionManager::IsPlayerInSession - Session handle invalid. Perhaps session has been deleted?\n" );
        return FALSE;
    }

    // Call XSessionGetDetails first time to get back the size 
    // of the results buffer we need
    DWORD cbResults = 0;

    DWORD ret = XSessionGetDetails( m_hSession,
                                    &cbResults,
                                    NULL,
                                    NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ) )
    {
        DebugSpew( "SessionManager::IsPlayerInSession - Failed on first call to XSessionGetDetails, hr=0x%08x\n", ret );
        return FALSE;
    }

    // Increase our buffer size to include an estimate of the maximum number
    // of players allowed in the session
    const DWORD dwMaxMembers = m_Slots[SLOTS_TOTALPUBLIC] + m_Slots[SLOTS_TOTALPRIVATE];
    cbResults += dwMaxMembers * sizeof(XSESSION_MEMBER);
    
    XSESSION_LOCAL_DETAILS* pSessionDetails = (XSESSION_LOCAL_DETAILS*)new BYTE[ cbResults ];
    if( pSessionDetails == NULL )
    {
        FatalError( "SessionManager::IsPlayerInSession - Failed to allocate buffer.\n" );
    }

    // Call second time to fill our results buffer
    ret = XSessionGetDetails( m_hSession,
                              &cbResults,
                              pSessionDetails,
                              NULL ); 

    if( ret != ERROR_SUCCESS )
    {
        DebugSpew( "SessionManager::IsPlayerInSession - XSessionGetDetails failed with error %d\n", ret );
        return FALSE;
    }

    const BOOL bIsValidUserIndex = ( dwUserIndex < XUSER_MAX_COUNT );
    BOOL bIsInSession = FALSE;
    for ( DWORD i = 0; i < pSessionDetails->dwReturnedMemberCount; ++i )
    {
        if( ( bIsValidUserIndex && ( pSessionDetails->pSessionMembers[i].dwUserIndex == dwUserIndex ) ) ||
            ( xuidPlayer == pSessionDetails->pSessionMembers[i].xuidOnline ) )
        {
            bIsInSession = TRUE;
            break;
        }
    }

    if ( pSessionDetails )
    {
        delete [] pSessionDetails;
    }

    return bIsInSession;
}

//------------------------------------------------------------------------
// Name: DebugDumpSessionDetails()
// Desc: Dumps session deletes
//------------------------------------------------------------------------
VOID SessionManager::DebugDumpSessionDetails() const
{
    #ifdef _DEBUG

    if( m_hSession == INVALID_HANDLE_VALUE || m_hSession == NULL )
    {
        DebugSpew( "SessionManager::DebugDumpSessionDetails - Session handle invalid. Perhaps session has been deleted?\n" );
        return;
    }

    // Call XSessionGetDetails first time to get back the size 
    // of the results buffer we need
    DWORD cbResults = 0;

    DWORD ret = XSessionGetDetails( m_hSession,
                                    &cbResults,
                                    NULL,
                                    NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ) )
    {
        DebugSpew( "SessionManager::DebugDumpSessionDetails - Failed on first call to XSessionGetDetails, hr=0x%08x\n", ret );
        return;
    }

    // Increase our buffer size to include an estimate of the maximum number
    // of players allowed in the session
    const DWORD dwMaxMembers = m_Slots[SLOTS_TOTALPUBLIC] + m_Slots[SLOTS_TOTALPRIVATE];
    cbResults += dwMaxMembers * sizeof(XSESSION_MEMBER);
    
    XSESSION_LOCAL_DETAILS* pSessionDetails = (XSESSION_LOCAL_DETAILS*)new BYTE[ cbResults ];
    if( pSessionDetails == NULL )
    {
        FatalError( "SessionManager::DebugDumpSessionDetails - Failed to allocate buffer.\n" );
    }

    // Call second time to fill our results buffer
    ret = XSessionGetDetails( m_hSession,
                              &cbResults,
                              pSessionDetails,
                              NULL ); 

    if( ret != ERROR_SUCCESS )
    {
        DebugSpew( "SessionManager::DebugDumpSessionDetails - XSessionGetDetails failed with error %d\n", ret );
        return;
    }

    // Iterate through the returned results
    DebugSpew( "***************** DebugDumpSessionDetails *****************\n" \
                    "instance: %p\n" \
                    "sessionID: %016I64X\n" \
                    "Matchmaking session?: %s\n" \
                    "m_hSession: %p\n" \
                    "bIsHost: %d\n" \
                    "sessionstate: %s\n" \
                    "qwSessionNonce: %I64u\n" \
                    "dwUserIndexHost: %d\n" \
                    "dwGameType: %d\n" \
                    "dwGameMode: %d\n" \
                    "dwFlags: 0x%x\n" \
                    "dwMaxPublicSlots: %d\n" \
                    "dwMaxPrivateSlots: %d\n" \
                    "dwAvailablePublicSlots: %d\n" \
                    "dwAvailablePrivateSlots: %d\n" \
                    "dwActualMemberCount: %d\n" \
                    "dwReturnedMemberCount: %d\n" \
                    "xnkidArbitration: %016I64X\n",
                    this,
                    GetSessionIDAsInt(),
                    ( HasSessionFlags( XSESSION_CREATE_USES_MATCHMAKING ) ) ? "Yes" : "No",
                    m_hSession,
                    m_bIsHost,
                    g_astrSessionState[m_SessionState],
                    m_qwSessionNonce,
                    pSessionDetails->dwUserIndexHost,
                    pSessionDetails->dwGameType,
                    pSessionDetails->dwGameMode,
                    pSessionDetails->dwFlags,
                    pSessionDetails->dwMaxPublicSlots,
                    pSessionDetails->dwMaxPrivateSlots,
                    pSessionDetails->dwAvailablePublicSlots,
                    pSessionDetails->dwAvailablePrivateSlots,
                    pSessionDetails->dwActualMemberCount,
                    pSessionDetails->dwReturnedMemberCount,
                    XNKIDToInt64( pSessionDetails->xnkidArbitration ) );

    for ( DWORD i = 0; i < pSessionDetails->dwReturnedMemberCount; ++i )
    {
        DebugSpew( "***************** XSESSION_MEMBER %d *****************\n" \
                   "Online XUID: 0x%016I64X\n" \
                   "dwUserIndex: %d\n" \
                   "dwFlags: 0x%x\n",
                   i,
                   pSessionDetails->pSessionMembers[i].xuidOnline,
                   pSessionDetails->pSessionMembers[i].dwUserIndex,
                   pSessionDetails->pSessionMembers[i].dwFlags );    
    }

    if ( pSessionDetails )
    {
        delete [] pSessionDetails;
    }

    #endif
}

//------------------------------------------------------------------------
// Name: DebugValidateExpectedSlotCounts()
// Desc: Makes sure slot counts are consistent with what is stored with
//       the XSession
//------------------------------------------------------------------------
BOOL SessionManager::DebugValidateExpectedSlotCounts( const BOOL bBreakOnDifferent ) const
{
    BOOL bSlotCountsDiffer = FALSE;
    assert (m_SessionState < SessionStateCount);

    #ifdef _DEBUG

    if( m_hSession == INVALID_HANDLE_VALUE || m_hSession == NULL )
    {
        DebugSpew( "SessionManager::DebugValidateExpectedSlotCounts - Session handle invalid. Perhaps session has been deleted?\n" );
        return !bSlotCountsDiffer;
    }

    // Call XSessionGetDetails first time to get back the size 
    // of the results buffer we need
    DWORD cbResults = 0;

    DWORD ret = XSessionGetDetails( m_hSession,
                                    &cbResults,
                                    NULL,
                                    NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbResults == 0 ) )
    {
        DebugSpew( "SessionManager::DebugValidateExpectedSlotCounts - Failed on first call to XSessionGetDetails, hr=0x%08x\n", ret );
        return FALSE;
    }
    
    XSESSION_LOCAL_DETAILS* pSessionDetails = (XSESSION_LOCAL_DETAILS*)new BYTE[ cbResults ];
    if( pSessionDetails == NULL )
    {
        FatalError( "SessionManager::DebugValidateExpectedSlotCounts - Failed to allocate buffer.\n" );
    }

    // Call second time to fill our results buffer
    ret = XSessionGetDetails( m_hSession,
                              &cbResults,
                              pSessionDetails,
                              NULL ); 

    if( ret != ERROR_SUCCESS )
    {
        DebugSpew( "SessionManager::DebugValidateExpectedSlotCounts - XSessionGetDetails failed with error %d\n", ret );
        return FALSE;
    }

    const XNKID sessionID = pSessionDetails->sessionInfo.sessionID;

    const DWORD dwMaxPublicSlots        = m_Slots[SLOTS_TOTALPUBLIC];
    const DWORD dwMaxPrivateSlots       = m_Slots[SLOTS_TOTALPRIVATE];
    const DWORD dwAvailablePublicSlots  = m_Slots[SLOTS_TOTALPUBLIC] - m_Slots[SLOTS_FILLEDPUBLIC];
    const DWORD dwAvailablePrivateSlots = m_Slots[SLOTS_TOTALPRIVATE] - m_Slots[SLOTS_FILLEDPRIVATE];

    bSlotCountsDiffer = ( pSessionDetails->dwMaxPublicSlots != dwMaxPublicSlots  ||
                          pSessionDetails->dwMaxPrivateSlots != dwMaxPrivateSlots );

    // XSessionGetDetails only returns dwAvailablePublicSlots and dwAvailablePrivateSlots data
    // for:
    // 1. matchmaking sessions,
    // 2. sessions that are joinable during gameplay,
    // 3. sessions that aren't deleted
    //
    // Therefore it only makes sense to compare against are expected slot counts if all of
    // these conditions are met
    const BOOL bIsMatchmakingSession     = HasSessionFlags( XSESSION_CREATE_USES_MATCHMAKING );

    const BOOL bJoinable                 = !HasSessionFlags( XSESSION_CREATE_USES_ARBITRATION ) &&
                                           !HasSessionFlags( XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED );

    if( bIsMatchmakingSession && 
        ( m_SessionState != SessionStateDeleted ) &&
        ( ( m_SessionState != SessionStateInGame ) || ( ( m_SessionState == SessionStateInGame ) && bJoinable ) ) )
    {
        bSlotCountsDiffer |= ( pSessionDetails->dwAvailablePublicSlots != dwAvailablePublicSlots ||
                               pSessionDetails->dwAvailablePrivateSlots != dwAvailablePrivateSlots );
    }

    // Any differences detected?
    if( !bSlotCountsDiffer )
    {
        goto Terminate;
    }

    // Spew expected/actual slot counts
    DebugSpew( "***************** DebugValidateExpectedSlotCounts *****************\n" \
                    "instance: %p\n" \
                    "sessionID: %016I64X\n" \
                    "m_hSession: %p\n" \
                    "bIsHost: %d\n" \
                    "sessionstate: %s\n" \
                    "dwMaxPublicSlots        expected: %d;    actual: %d\n" \
                    "dwMaxPrivateSlots       expected: %d;    actual: %d\n",
                    this,
                    XNKIDToInt64( sessionID ),
                    m_hSession,
                    m_bIsHost,
                    g_astrSessionState[m_SessionState],
                    dwMaxPublicSlots, pSessionDetails->dwMaxPublicSlots,
                    dwMaxPrivateSlots, pSessionDetails->dwMaxPrivateSlots );

    if( HasSessionFlags( XSESSION_CREATE_USES_MATCHMAKING ) )
    {
        DebugSpew( "dwAvailablePublicSlots  expected: %d;    actual: %d\n" \
                   "dwAvailablePrivateSlots expected: %d;    actual: %d\n",
                   dwAvailablePublicSlots, pSessionDetails->dwAvailablePublicSlots,
                   dwAvailablePrivateSlots, pSessionDetails->dwAvailablePrivateSlots );
    }

    if( bBreakOnDifferent & bSlotCountsDiffer )
    {
        DebugBreak();
    }

Terminate:
    if ( pSessionDetails )
    {
        delete [] pSessionDetails;
    }

    #endif

    return !bSlotCountsDiffer;
}

//------------------------------------------------------------------------
// Name: GetSessionCreationReason()
// Desc: Retrieves session creation reason
//------------------------------------------------------------------------
SessionCreationReason SessionManager::GetSessionCreationReason( VOID ) const
{
    return m_SessionCreationReason;        
}

//------------------------------------------------------------------------
// Name: GetSessionState()
// Desc: Retrieves current session state
//------------------------------------------------------------------------
SessionState SessionManager::GetSessionState( VOID ) const
{
    return m_SessionState;        
}

//------------------------------------------------------------------------
// Name: GetSessionStateString()
// Desc: Retrieves current session state
//------------------------------------------------------------------------
const char* SessionManager::GetSessionStateString( VOID ) const
{
    return g_astrSessionState[m_SessionState];
}

//------------------------------------------------------------------------
// Name: SetSessionOwner()
// Desc: Sets current session owner
//------------------------------------------------------------------------
VOID SessionManager::SetSessionOwner( const DWORD dwOwnerController )
{
    m_dwOwnerController = dwOwnerController;
    XUserGetXUID( m_dwOwnerController, &m_xuidOwner );
}

//------------------------------------------------------------------------
// Name: GetSessionOwner()
// Desc: Retrieves current session owner
//------------------------------------------------------------------------
DWORD SessionManager::GetSessionOwner( VOID ) const
{
    return m_dwOwnerController;
}

//------------------------------------------------------------------------
// Name: GetSessionOwner()
// Desc: Retrieves current session owner XUID
//------------------------------------------------------------------------
XUID SessionManager::GetSessionOwnerXuid( VOID ) const
{
    return m_xuidOwner;
}

//------------------------------------------------------------------------
// Name: IsSessionOwner()
// Desc: Retrieves current session owner
//------------------------------------------------------------------------
BOOL SessionManager::IsSessionOwner( const DWORD dwController ) const
{
    return ( dwController == m_dwOwnerController );
}

//------------------------------------------------------------------------
// Name: IsSessionOwner()
// Desc: Retrieves current session owner
//------------------------------------------------------------------------
BOOL SessionManager::IsSessionOwner( const XUID xuidOwner ) const
{
    return ( xuidOwner == m_xuidOwner );
}

//------------------------------------------------------------------------
// Name: IsSessionHost()
// Desc: True if this SessionManager instance is the session host,
//       else false
//------------------------------------------------------------------------
BOOL SessionManager::IsSessionHost( VOID ) const
{
    return m_bIsHost;
}

//------------------------------------------------------------------------
// Name: MakeSessionHost()
// Desc: Make this SessionManager instance the session host. 
//------------------------------------------------------------------------
VOID SessionManager::MakeSessionHost( VOID )
{
    m_bIsHost = TRUE;
}

//------------------------------------------------------------------------
// Name: GetSessionNonce()
// Desc: Function to retrieve current session nonce
//------------------------------------------------------------------------
ULONGLONG SessionManager::GetSessionNonce( VOID ) const
{
    return m_qwSessionNonce;
}

//------------------------------------------------------------------------
// Name: SetSessionNonce()
// Desc: Function to set the session nonce
//       Only non-hosts should ever call this method
//------------------------------------------------------------------------
VOID SessionManager::SetSessionNonce( const ULONGLONG qwNonce )
{    
    if( m_bIsHost )
    {
        FatalError( "Only non-hosts should call this function!\n" );
    }

    m_qwSessionNonce = qwNonce;

    DebugDumpSessionDetails();
}

//------------------------------------------------------------------------
// Name: SetHostInAddr()
// Desc: Sets the IN_ADDR of the session host
//------------------------------------------------------------------------
VOID SessionManager::SetHostInAddr( const IN_ADDR& inaddr )
{
    m_HostInAddr = inaddr;
}

//------------------------------------------------------------------------
// Name: GetHostInAddr()
// Desc: Returns the IN_ADDR of the session host
//------------------------------------------------------------------------
IN_ADDR SessionManager::GetHostInAddr( VOID ) const
{
    return m_HostInAddr;
}


//------------------------------------------------------------------------
// Name: GetSessionID()
// Desc: Function to retrieve current session ID
//------------------------------------------------------------------------
XNKID SessionManager::GetSessionID( VOID )  const
{
    return m_SessionInfo.sessionID;
}

//------------------------------------------------------------------------
// Name: GetSessionIDAsInt()
// Desc: Function to retrieve current session ID as an __int64
//------------------------------------------------------------------------
__int64 SessionManager::GetSessionIDAsInt( VOID )  const
{
    // XNKID's are big-endian. Convert to little-endian if needed.
    #ifdef LIVE_ON_WINDOWS
        XNKID sessionID = m_SessionInfo.sessionID;
        for( BYTE i = 0; i < 4; ++i )
        {
            BYTE temp = (BYTE)( *( (BYTE*)&sessionID + i ) ); //temp = sessionID[ i ]
            *( (BYTE*)&sessionID + i ) = (BYTE)( *( (BYTE*)&sessionID + 8 - i - 1 ) ); //sessionID[ i ] = sessionID[ 8 - i - 1 ]
            *( (BYTE*)&sessionID + 8 - i - 1 ) = temp; //sessionID[ 8 - i - 1 ] = temp                     
        }
        return *(const __int64*)&sessionID;
    #else        
        return *(const __int64*)&m_SessionInfo.sessionID;
    #endif
}

//------------------------------------------------------------------------
// Name: GetMigratedSessionID()
// Desc: Function to retrieve the session ID of the session while
//       it is being migrated. This method can only be called
//       when this session manager instance is actively migrating a session
//------------------------------------------------------------------------
XNKID SessionManager::GetMigratedSessionID( VOID )  const
{
    if( !( m_SessionState >= SessionStateMigrateHost &&
           m_SessionState <= SessionStateMigratedHost ) )
    {
        FatalError( "GetMigratedSessionID called in state %s!\n", GetSessionStateString() );
    }

    return m_migratedSessionID;
}


//------------------------------------------------------------------------
// Name: SetSessionState()
// Desc: Sets current session state
//------------------------------------------------------------------------
VOID SessionManager::SetSessionState( const SessionState newState )
{
    m_SessionState = newState;
}

//------------------------------------------------------------------------
// Name: CheckSessionState()
// Desc: Checks if session in the expected state
//------------------------------------------------------------------------
BOOL SessionManager::CheckSessionState( const SessionState expectedState )  const
{
    SessionState actual = GetSessionState();

    if( actual != expectedState )
    {
        FatalError( "SessionManager instance in invalid state. Expected: %d; Got: %d\n", 
                    expectedState, actual );
    }

    return TRUE;
}

//------------------------------------------------------------------------
// Name: GetRegistrationResults()
// Desc: Retrieves last results from registering this client for
//       arbitratrion
//------------------------------------------------------------------------
const PXSESSION_REGISTRATION_RESULTS SessionManager::GetRegistrationResults()  const
{
    return (const PXSESSION_REGISTRATION_RESULTS)m_pRegistrationResults;    
}

//------------------------------------------------------------------------
// Name: SwitchToState()
// Desc: Changes to a new session state and performs initialization 
//       for the new state
//------------------------------------------------------------------------
VOID SessionManager::SwitchToState( SessionState newState )
{
    // Ignore transitions to the same state
    if( m_SessionState == newState )
    {
        return;
    }

    // If we're in state SessionStateMigratedHost, force a switch to 
    // the state we were in prior to migration
    if( m_SessionState == SessionStateMigratedHost )
    {
        DebugSpew( "SessionManager %016I64X (instance %p): Detected to be in state SessionStateMigratedHost. " \
                   "Switching to correct pre-migration state\n",
                   XNKIDToInt64( m_SessionInfo.sessionID ),
                   this );
        
        SwitchToPreHostMigrationState();
    }

    assert( m_SessionState < _countof( g_astrSessionState ) );
    DebugSpew( "SessionManager %016I64X (instance %p): Switching from session state %s to %s\n", 
               XNKIDToInt64( m_SessionInfo.sessionID ), 
               this, 
               g_astrSessionState[m_SessionState], 
               g_astrSessionState[newState] );

    // Clean up from the previous state
    switch( m_SessionState )
    {
        case SessionStateDeleted:
            //Reset();

            // Clear out our slots and update our presence info
            m_Slots[ SLOTS_FILLEDPUBLIC  ] = 0;
            m_Slots[ SLOTS_FILLEDPRIVATE ] = 0;
            m_Slots[ SLOTS_ZOMBIEPUBLIC ]  = 0;
            m_Slots[ SLOTS_ZOMBIEPRIVATE ] = 0;

            if( m_pRegistrationResults )
            {
                delete[] m_pRegistrationResults;
                m_pRegistrationResults = NULL;
            }
            break;

        case SessionStateMigratedHost:
            m_bIsMigratedSessionHost = FALSE;
            break;

        case SessionStateRegistering:
        case SessionStateInGame:
        case SessionStateNone:
        case SessionStateCreating:
        case SessionStateWaitingForRegistration:
        case SessionStateStarting:
        case SessionStateEnding:
        case SessionStateDeleting:
        default:
            break;
    }

    // Initialize the next state
    switch( newState )
    {
        case SessionStateCreated:
            if( !IsSessionHost() )
            {
                // Re-entering this state is a bad thing, since
                // that would wipe out the session nonce
                assert( m_SessionState != SessionStateCreated );
                
                // XSessionCreate would have
                // returned a 0 session nonce,
                // so set our session nonce
                // to a random value that we'll
                // replace once we get the
                // true nonce from the session host
                int retVal = XNetRandom( (BYTE *)&m_qwSessionNonce, sizeof( m_qwSessionNonce ) );
                if ( retVal )
                {
                    FatalError( "XNetRandom failed: %d\n", retVal );
                }
                DebugDumpSessionDetails();
            }
            break;

        case SessionStateMigratedHost:
            // Copy m_NewSessionInfo into m_SessionInfo
            // and zero out m_NewSessionInfo
            m_SessionInfo = m_NewSessionInfo; 
            ZeroMemory( &m_NewSessionInfo, sizeof( XSESSION_INFO ) );
            break;

        case SessionStateEnded:
            // If this is an arbitrated session, we might have slots occupied by zombies. Free up those slots
            // and zero out our zombie slot counts
            if( m_dwSessionFlags & XSESSION_CREATE_USES_ARBITRATION )
            {
                m_Slots[ SLOTS_FILLEDPRIVATE ] -= m_Slots[ SLOTS_ZOMBIEPRIVATE ];
                m_Slots[ SLOTS_FILLEDPRIVATE ] = max( m_Slots[ SLOTS_FILLEDPRIVATE ], 0 );

                m_Slots[ SLOTS_FILLEDPUBLIC ] -= m_Slots[ SLOTS_ZOMBIEPUBLIC ];
                m_Slots[ SLOTS_FILLEDPUBLIC ] = max( m_Slots[ SLOTS_FILLEDPUBLIC ], 0 );


                m_Slots[ SLOTS_ZOMBIEPRIVATE ] = 0;
                m_Slots[ SLOTS_ZOMBIEPUBLIC ]  = 0;
            }
            break;

        case SessionStateDeleted:
            if( ( m_hSession != INVALID_HANDLE_VALUE ) && 
                ( m_hSession != NULL ) )
            {
                XCloseHandle( m_hSession );
                m_hSession = NULL;
            }
            break;

        case SessionStateNone:
        case SessionStateIdle:
        case SessionStateWaitingForRegistration:
        case SessionStateRegistered:
        case SessionStateInGame:
        default:
            break;
    }

    m_SessionState = newState;
}

//------------------------------------------------------------------------
// Name: NotifyOverlappedOperationCancelled()
// Desc: Lets this instance know that the current overlapped operation
//       was cancelled.
//------------------------------------------------------------------------
HRESULT SessionManager::NotifyOverlappedOperationCancelled( const XOVERLAPPED* const pXOverlapped )
{
    //
    // Only certain states can be rolled back to the previous state
    switch( m_SessionState )
    {
    case SessionStateCreating:
        m_SessionState = SessionStateNone;
        break;
    case SessionStateRegistering:
        m_SessionState = SessionStateIdle;
        break;
    case SessionStateStarting:
        m_SessionState = HasSessionFlags( XSESSION_CREATE_USES_ARBITRATION ) ? SessionStateRegistered : SessionStateIdle;
        break;
    case SessionStateEnding:
        m_SessionState = SessionStateInGame;
        break;
    case SessionStateDeleting:
        m_SessionState = SessionStateEnded;
        break;
    }    

    return S_OK;
}


//------------------------------------------------------------------------
// Name: SwitchToPreHostMigrationState()
// Desc: Switches the current session state back to what it was
//       prior to host migration
//------------------------------------------------------------------------
VOID SessionManager::SwitchToPreHostMigrationState()
{
    // We can only transition to this state from SessionStateMigratedHost
    assert( m_SessionState == SessionStateMigratedHost );

    // Zero out m_migratedSessionID
    ZeroMemory( &m_migratedSessionID, sizeof( XNKID ) );

    // Return session state to what it was before migration started
    // and set m_migratedSessionState to a blank state
    m_SessionState           = m_migratedSessionState;
    m_migratedSessionState   = SessionStateNone;
    m_bIsMigratedSessionHost = FALSE;

    DebugDumpSessionDetails();
}


//------------------------------------------------------------------------
// Name: HasSessionFlags()
// Desc: Checks if passed-in flags are set for the current session
//------------------------------------------------------------------------
BOOL SessionManager::HasSessionFlags( const DWORD dwFlags )  const
{
    BOOL bHasFlags = FALSE;
    
    // What flags in m_dwSessionFlags and dwFlags are different?
    DWORD dwDiffFlags = m_dwSessionFlags ^ dwFlags;

    // If none of dwDiffFlags are in dwFlags,
    // we have a match
    if( ( dwDiffFlags & dwFlags ) == 0 )
    {
        bHasFlags = TRUE;
    }

    return bHasFlags;
}

//------------------------------------------------------------------------
// Name: FlipSessionFlags()
// Desc: XORs the state of the current session's
//       flags with that of the passed-in flags
//------------------------------------------------------------------------
VOID SessionManager::FlipSessionFlags( const DWORD dwFlags )
{ 
    m_dwSessionFlags ^= dwFlags;

    DebugSpew( "FlipSessionFlags: New session flags: %d\n", m_dwSessionFlags );
}

//------------------------------------------------------------------------
// Name: ClearSessionFlags()
// Desc: Clear the passed-in flags for the
//       current session
//------------------------------------------------------------------------
VOID SessionManager::ClearSessionFlags( const DWORD dwFlags )
{ 
    m_dwSessionFlags &= ~dwFlags; 

    DebugSpew( "ClearSessionFlags: New session flags: %d\n", m_dwSessionFlags );
}

//------------------------------------------------------------------------
// Name: SetSessionInfo()
// Desc: Set session info data. This only be called outside of an
//       active session
//------------------------------------------------------------------------
VOID SessionManager::SetSessionInfo( const XSESSION_INFO& session_info )
{
    SessionState actual = GetSessionState();
    if( m_SessionState > SessionStateCreating && m_SessionState < SessionStateDelete )
    {
        FatalError( "SessionManager instance in invalid state: %s\n", g_astrSessionState[actual] );
    }

    m_SessionInfo = session_info;
}

//------------------------------------------------------------------------
// Name: SetNewSessionInfo()
// Desc: Set session info data for the new session we will migrate this
//       session to. This only be called inside of an active session
//------------------------------------------------------------------------
VOID SessionManager::SetNewSessionInfo( const XSESSION_INFO& session_info,
                                        const BOOL bIsNewSessionHost )
{
    SessionState actual = GetSessionState();
    if( m_SessionState < SessionStateCreated || m_SessionState >= SessionStateDelete )
    {
        FatalError( "SessionManager instance in invalid state: %d\n", actual );
    }

    m_NewSessionInfo         = session_info;
    m_bIsMigratedSessionHost = bIsNewSessionHost;

    // Cache current session ID for a possible host migration scenario..
    memcpy( &m_migratedSessionID, &m_SessionInfo.sessionID, sizeof( XNKID ) ); 
}

//------------------------------------------------------------------------
// Name: SetSessionFlags()
// Desc: Set session flags. Optionally first clears all currently set 
//       flags. This only be called outside of an active session
//------------------------------------------------------------------------
VOID SessionManager::SetSessionFlags( const DWORD dwFlags, const BOOL fClearExisting  )
{
    SessionState actual = GetSessionState();
    if( m_SessionState > SessionStateCreating && m_SessionState < SessionStateDelete )
    {
        FatalError( "SessionManager instance in invalid state: %s\n", g_astrSessionState[actual] );
    }

    if( fClearExisting )
    {
        m_dwSessionFlags = 0;
    }

    m_dwSessionFlags |= dwFlags;

    DebugSpew( "SetSessionFlags: New session flags: %d\n", m_dwSessionFlags );
}

//------------------------------------------------------------------------
// Name: GetSessionFlags()
// Desc: Returns current session flags
//------------------------------------------------------------------------
DWORD SessionManager::GetSessionFlags()  const
{
    return m_dwSessionFlags;
}

//------------------------------------------------------------------------
// Name: GetSessionInfo()
// Desc: Returns current XSESSION_INFO
//------------------------------------------------------------------------
const XSESSION_INFO& SessionManager::GetSessionInfo() const
{
    return m_SessionInfo;
}

//------------------------------------------------------------------------
// Name: GetMaxSlotCounts()
// Desc: Returns current maximum slot counts
//------------------------------------------------------------------------
VOID SessionManager::GetMaxSlotCounts( DWORD& dwMaxPublicSlots, DWORD& dwMaxPrivateSlots )  const
{
    dwMaxPublicSlots  = m_Slots[SLOTS_TOTALPUBLIC];
    dwMaxPrivateSlots = m_Slots[SLOTS_TOTALPRIVATE];

//    DebugValidateExpectedSlotCounts();
}

//------------------------------------------------------------------------
// Name: GetFilledSlotCounts()
// Desc: Returns current filled slot counts
//------------------------------------------------------------------------
VOID SessionManager::GetFilledSlotCounts( DWORD& dwFilledPublicSlots, DWORD& dwFilledPrivateSlots ) const
{
    dwFilledPublicSlots  = m_Slots[SLOTS_FILLEDPUBLIC];
    dwFilledPrivateSlots = m_Slots[SLOTS_FILLEDPRIVATE];

//    DebugValidateExpectedSlotCounts();
}

//------------------------------------------------------------------------
// Name: SetMaxSlotCounts()
// Desc: Sets current maximum slot counts
//------------------------------------------------------------------------
VOID SessionManager::SetMaxSlotCounts( const DWORD dwMaxPublicSlots, const DWORD dwMaxPrivateSlots )
{
    #ifdef _DEBUG
    DebugSpew( "SetMaxSlotCounts(%016I64X): Old: [%d, %d, %d, %d]\n",
               XNKIDToInt64( m_SessionInfo.sessionID ),
               m_Slots[SLOTS_TOTALPUBLIC],
               m_Slots[SLOTS_TOTALPRIVATE],
               m_Slots[SLOTS_FILLEDPUBLIC],
               m_Slots[SLOTS_FILLEDPRIVATE]);
    #endif

    m_Slots[SLOTS_TOTALPUBLIC]  = dwMaxPublicSlots;
    m_Slots[SLOTS_TOTALPRIVATE] = dwMaxPrivateSlots;

    #ifdef _DEBUG
    DebugSpew( "SetMaxSlotCounts(%016I64X): New: [%d, %d, %d, %d]\n",
               XNKIDToInt64( m_SessionInfo.sessionID ),
               m_Slots[SLOTS_TOTALPUBLIC],
               m_Slots[SLOTS_TOTALPRIVATE],
               m_Slots[SLOTS_FILLEDPUBLIC],
               m_Slots[SLOTS_FILLEDPRIVATE]);
    #endif
}

//------------------------------------------------------------------------
// Name: SetFilledSlotCounts()
// Desc: Sets current filled slot counts
//------------------------------------------------------------------------
VOID SessionManager::SetFilledSlotCounts( const DWORD dwFilledPublicSlots, DWORD dwFilledPrivateSlots )
{
    #ifdef _DEBUG
    DebugSpew( "SetFilledSlotCounts(%016I64X): Old: [%d, %d, %d, %d]\n",
               XNKIDToInt64( m_SessionInfo.sessionID ),
               m_Slots[SLOTS_TOTALPUBLIC],
               m_Slots[SLOTS_TOTALPRIVATE],
               m_Slots[SLOTS_FILLEDPUBLIC],
               m_Slots[SLOTS_FILLEDPRIVATE]);               
    #endif

    m_Slots[SLOTS_FILLEDPUBLIC]  = dwFilledPublicSlots;
    m_Slots[SLOTS_FILLEDPRIVATE] = dwFilledPrivateSlots;

    #ifdef _DEBUG
    DebugSpew( "SetFilledSlotCounts(%016I64X): New: [%d, %d, %d, %d]\n",
               XNKIDToInt64( m_SessionInfo.sessionID ),
               m_Slots[SLOTS_TOTALPUBLIC],
               m_Slots[SLOTS_TOTALPRIVATE],
               m_Slots[SLOTS_FILLEDPUBLIC],
               m_Slots[SLOTS_FILLEDPRIVATE]);
    #endif
}

//------------------------------------------------------------------------
// Name: GetSessionError()
// Desc: Returns session error string
//------------------------------------------------------------------------
WCHAR* SessionManager::GetSessionError()  const
{
    return m_strSessionError;
}

//------------------------------------------------------------------------
// Name: SetSessionError()
// Desc: Sets session error string
//------------------------------------------------------------------------
VOID SessionManager::SetSessionError( WCHAR* error )
{
    m_strSessionError = error;
}


//--------------------------------------------------------------------------------------
// Name: StartQoSListener()
// Desc: Turn on the Quality of Service Listener for the current session
//--------------------------------------------------------------------------------------
VOID SessionManager::StartQoSListener( BYTE* data, const UINT dataLen, const DWORD bitsPerSec )
{
    DWORD flags = XNET_QOS_LISTEN_SET_DATA;
    if( bitsPerSec )
    {
        flags |= XNET_QOS_LISTEN_SET_BITSPERSEC;
    }
    if( !m_bUsingQoS )
    {
        flags |= XNET_QOS_LISTEN_ENABLE;
    }
    DWORD dwRet;
    dwRet = XNetQosListen( &( m_SessionInfo.sessionID ), data, dataLen, bitsPerSec, flags );
    if( ERROR_SUCCESS != dwRet )
    {
        FatalError( "Failed to start QoS listener, error 0x%08x\n", dwRet );
    }
    m_bUsingQoS = TRUE;
}


//--------------------------------------------------------------------------------------
// Name: StopQoSListener()
// Desc: Turn off the Quality of Service Listener for the current session
//--------------------------------------------------------------------------------------
VOID SessionManager::StopQoSListener()
{
    if( m_bUsingQoS )
    {
        DWORD dwRet;
        dwRet = XNetQosListen( &( m_SessionInfo.sessionID ), NULL, 0, 0,
                               XNET_QOS_LISTEN_RELEASE );
        if( ERROR_SUCCESS != dwRet )
        {
            DebugSpew( "Warning: Failed to stop QoS listener, error 0x%08x\n", dwRet );
        }
        m_bUsingQoS = FALSE;
    }
}

//------------------------------------------------------------------------
// Name: CreateSession()
// Desc: Creates a session
//------------------------------------------------------------------------
HRESULT SessionManager::CreateSession( XOVERLAPPED* pXOverlapped )
{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual != SessionStateNone && actual != SessionStateDeleted  )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    // Modify session flags if host
    if( m_bIsHost )
    {
        m_dwSessionFlags |= XSESSION_CREATE_HOST;
    }
    else
    {
        // Turn off host bit
        m_dwSessionFlags &= ~XSESSION_CREATE_HOST;
    }

    // If we're just in a Matchmaking session, we can only specify the 
    // XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED modifier
    if( !( m_dwSessionFlags & XSESSION_CREATE_USES_PRESENCE ) && 
         ( m_dwSessionFlags & XSESSION_CREATE_USES_MATCHMAKING ) && 
         ( m_dwSessionFlags & XSESSION_CREATE_MODIFIERS_MASK ) )
    {
        // turn off all modifiers
        m_dwSessionFlags &= ~XSESSION_CREATE_MODIFIERS_MASK;

        // turn on XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED
        m_dwSessionFlags |= XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED;
    }

    // Call XSessionCreate
    DWORD ret = XSessionCreate( m_dwSessionFlags,
                                m_dwOwnerController,
                                m_Slots[SLOTS_TOTALPRIVATE],
                                m_Slots[SLOTS_TOTALPUBLIC],
                                &m_qwSessionNonce,
                                &m_SessionInfo,
                                pXOverlapped,
                                &m_hSession );

    if( pXOverlapped && ( ret == ERROR_IO_PENDING ) )
    {
        SwitchToState( SessionStateCreating );
    }
    else if( !pXOverlapped )
    {
        SwitchToState( SessionStateCreated );
    }

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: StartSession()
// Desc: Starts the current session
//------------------------------------------------------------------------
HRESULT SessionManager::StartSession( XOVERLAPPED* pXOverlapped )
{
    DWORD ret = XSessionStart( m_hSession,
                               0,
                               pXOverlapped );

    if( pXOverlapped && ( ret == ERROR_IO_PENDING ) )
    {
        SwitchToState( SessionStateStarting );
    }
    else if( !pXOverlapped )
    {
        SwitchToState( SessionStateInGame );
    }

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: EndSession()
// Desc: Ends the current session
//------------------------------------------------------------------------
HRESULT SessionManager::EndSession( XOVERLAPPED* pXOverlapped )
{
    DWORD ret = XSessionEnd( m_hSession, pXOverlapped );

    if( pXOverlapped && ( ret == ERROR_IO_PENDING ) )
    {
        SwitchToState( SessionStateEnding );
    }
    else if( !pXOverlapped )
    {
        SwitchToState( SessionStateEnded );
    }

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: DeleteSession()
// Desc: Deletes the current session
//------------------------------------------------------------------------
HRESULT SessionManager::DeleteSession( XOVERLAPPED* pXOverlapped )
{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    SwitchToState( SessionStateDelete );

    // Stop QoS listener if it's running
    StopQoSListener();

    // Call XSessionDelete
    DWORD ret = XSessionDelete( m_hSession, pXOverlapped );

    if( pXOverlapped && ( ret == ERROR_IO_PENDING ) )
    {
        SwitchToState( SessionStateDeleting );
    }
    else if( !pXOverlapped )
    {
        SwitchToState( SessionStateDeleted );
    }

    return HRESULT_FROM_WIN32( ret );
}


//------------------------------------------------------------------------
// Name: MigrateSession()
// Desc: Migrates the current session
//------------------------------------------------------------------------
HRESULT SessionManager::MigrateSession( XOVERLAPPED* pXOverlapped )
{
    // Make sure our instance is in the right state
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateDelete  )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    m_migratedSessionState = m_SessionState;

    DWORD ret = XSessionMigrateHost( m_hSession,
                                     ( m_bIsMigratedSessionHost ) ? m_dwOwnerController : XUSER_INDEX_NONE,
                                     &m_NewSessionInfo,
                                     pXOverlapped );

    if( pXOverlapped && ( ret == ERROR_IO_PENDING ) )
    {
        SwitchToState( SessionStateMigratingHost );
    }
    else if( !pXOverlapped )
    {
        SwitchToState( SessionStateMigratedHost );
    }

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: ModifySessionFlags()
// Desc: Modifies session flags
//------------------------------------------------------------------------
HRESULT SessionManager::ModifySessionFlags( const DWORD dwFlags, 
                                                                              const BOOL bClearFlags,
                                                                              XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( m_SessionState <= SessionStateCreating || m_SessionState >= SessionStateDelete )
    {
        FatalError( "SessionManager instance in invalid state: %s\n", g_astrSessionState[actual] );
    }

    DWORD dwNewFlags = dwFlags;

    // If no flags in the modifiers mask are specified, do nothing
    if( ( dwNewFlags & XSESSION_CREATE_MODIFIERS_MASK ) == 0 )
    {
        return ERROR_SUCCESS;
    }

    // Flags cannot be modified for arbitrated sessions
    if( m_dwSessionFlags & XSESSION_CREATE_USES_ARBITRATION )
    {
        return ERROR_SUCCESS;
    }

    // Add host bit to dwFlags if host; otherwise remove
    if( m_bIsHost )
    {
        dwNewFlags |= XSESSION_CREATE_HOST;
    }
    else
    {
        // Turn off host bit
        dwNewFlags &= ~XSESSION_CREATE_HOST;
    }

    // Apply allowable session modifier flags to our session flags
    if( bClearFlags )
    {
        m_dwSessionFlags ^= dwNewFlags & MODIFY_FLAGS_ALLOWED;
    }
    else
    {
        m_dwSessionFlags |= dwNewFlags & MODIFY_FLAGS_ALLOWED;
    }

    // Re-insert host bit if host
    if( m_bIsHost )
    {
        m_dwSessionFlags |= XSESSION_CREATE_HOST;
    }

    DebugSpew( "ProcessModifySessionFlagsMessage: New session flags: %d\n", m_dwSessionFlags );

    DWORD ret = XSessionModify( m_hSession,
                                m_dwSessionFlags,
                                m_Slots[SLOTS_TOTALPUBLIC],
                                m_Slots[SLOTS_TOTALPRIVATE],
                                pXOverlapped );

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: ModifySkill()
// Desc: Modifies TrueSkill(TM)
//------------------------------------------------------------------------
HRESULT SessionManager::ModifySkill( const DWORD dwNumXuids, 
                                     const XUID* pXuids, 
                                     XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( m_SessionState <= SessionStateCreating || m_SessionState >= SessionStateDelete )
    {
        FatalError( "SessionManager instance in invalid state: %s\n", g_astrSessionState[actual] );
    }

    DWORD ret = XSessionModifySkill( m_hSession,
                                     dwNumXuids,
                                     pXuids,
                                     pXOverlapped );

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: RegisterArbitration()
// Desc: Registers the current session for arbitration
//------------------------------------------------------------------------
HRESULT SessionManager::RegisterArbitration( XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateStarting )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    // Call once to determine size of results buffer
    DWORD cbRegistrationResults = 0;

    DWORD ret = XSessionArbitrationRegister( m_hSession,
                                             0,
                                             m_qwSessionNonce,
                                             &cbRegistrationResults,
                                             NULL,
                                             NULL ); 

    if( ( ret != ERROR_INSUFFICIENT_BUFFER ) || ( cbRegistrationResults == 0 ) )
    {
        DebugSpew( "Failed on first call to XSessionArbitrationRegister, hr=0x%08x\n", ret );
        return ERROR_FUNCTION_FAILED;
    }

    m_pRegistrationResults = (PXSESSION_REGISTRATION_RESULTS)new BYTE[cbRegistrationResults];
    if( m_pRegistrationResults == NULL )
    {
        DebugSpew( "Failed to allocate buffer.\n" );
        return ERROR_FUNCTION_FAILED;
    }

    // Call second time to fill our results buffer
    ret = XSessionArbitrationRegister( m_hSession,
                                       0,
                                       m_qwSessionNonce,
                                       &cbRegistrationResults,
                                       m_pRegistrationResults,
                                       pXOverlapped ); 

    if( pXOverlapped && ( ret == ERROR_IO_PENDING ) )
    {
        SwitchToState( SessionStateRegistering );
    }
    else if( !pXOverlapped )
    {
        SwitchToState( SessionStateRegistered );
    }

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: AddLocalPlayers()
// Desc: Add local players to the session
//------------------------------------------------------------------------
HRESULT SessionManager::AddLocalPlayers( const DWORD dwUserCount,
                                         DWORD* pdwUserIndices,
                                         const BOOL* pfPrivateSlots,
                                         XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreating || actual >= SessionStateDelete )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    DWORD ret = ERROR_SUCCESS;

    // Do nothing if the session is already deleted
    if( m_SessionState == SessionStateDeleted )
    {
        return HRESULT_FROM_WIN32( ret );
    }

    for ( DWORD  i = 0; i < dwUserCount; ++i )
    {
        XUID xuid;
        XUserGetXUID( pdwUserIndices[i], &xuid );

        DebugSpew( "Processing local user: index: %d; xuid: 0x%016I64X; bInvited: %d\n", 
                   pdwUserIndices[ i ], xuid, pfPrivateSlots[ i ] );

        // If xuid is already in the session, don't re-add
        if( IsPlayerInSession( xuid ) )
        {
            pdwUserIndices[ i ] = XUSER_INDEX_NONE;
        }
    }

    ret = XSessionJoinLocal( m_hSession,
                             dwUserCount,
                             pdwUserIndices,
                             pfPrivateSlots,
                             pXOverlapped );


    // Update slot counts
    for ( DWORD i = 0; i < dwUserCount; ++i )
    {
        m_Slots[ SLOTS_FILLEDPRIVATE ] += ( pfPrivateSlots[ i ] ) ? 1 : 0;
        m_Slots[ SLOTS_FILLEDPUBLIC ] += ( pfPrivateSlots[ i ] ) ? 0 : 1;
    }

    if( m_Slots[ SLOTS_FILLEDPRIVATE ] > m_Slots[ SLOTS_TOTALPRIVATE ] )
    {
        DWORD diff = m_Slots[ SLOTS_FILLEDPRIVATE ] - m_Slots[ SLOTS_TOTALPRIVATE ];
        m_Slots[ SLOTS_FILLEDPUBLIC ] += diff;

        assert( m_Slots[ SLOTS_FILLEDPUBLIC ] <= m_Slots[ SLOTS_TOTALPUBLIC ] );
        if( m_Slots[ SLOTS_FILLEDPUBLIC ] > m_Slots[ SLOTS_TOTALPUBLIC ] )
        {
            FatalError( "Too many slots filled!\n" );
        }

        m_Slots[ SLOTS_FILLEDPRIVATE ] = m_Slots[ SLOTS_TOTALPRIVATE ];
    }

    DebugDumpSessionDetails();
    DebugValidateExpectedSlotCounts();

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: AddRemotePlayers()
// Desc: Adds remote players to the session
//------------------------------------------------------------------------
HRESULT SessionManager::AddRemotePlayers( const DWORD dwXuidCount,
                                          XUID* pXuids,
                                          const BOOL* pfPrivateSlots,
                                          XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreating || actual >= SessionStateDelete )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    for ( DWORD i=0; i<dwXuidCount; ++i )
    {
        DebugSpew( "AddRemotePlayers: 0x%016I64X; bInvited: %d\n", pXuids[ i ], pfPrivateSlots[ i ] );
    }

    DWORD ret = ERROR_SUCCESS;

    // Do nothing if the session is already deleted
    if( m_SessionState == SessionStateDeleted )
    {
        return HRESULT_FROM_WIN32( ret );
    }

    for ( DWORD  i = 0; i < dwXuidCount; ++i )
    {
        DebugSpew( "Processing remote user: 0x%016I64X; bInvited: %d\n", 
                   pXuids[ i ], pfPrivateSlots[ i ] );

        // If msg.m_pXuids[ i ] is already in the session, don't re-add
        if( IsPlayerInSession( pXuids[ i ] ) )
        {
            pXuids[ i ] = INVALID_XUID;

            DebugSpew( "Remote user: 0x%016I64X already in the session!\n", 
                       pXuids[ i ] );
        }
    }

    ret = XSessionJoinRemote( m_hSession,
                              dwXuidCount,
                              pXuids,
                              pfPrivateSlots,
                              pXOverlapped );

    // Update slot counts
    for ( DWORD i = 0; i < dwXuidCount; ++i )
    {
        m_Slots[ SLOTS_FILLEDPRIVATE ] += ( pfPrivateSlots[ i ] ) ? 1 : 0;
        m_Slots[ SLOTS_FILLEDPUBLIC ]  += ( pfPrivateSlots[ i ] ) ? 0 : 1;
    }

    if( m_Slots[ SLOTS_FILLEDPRIVATE ] > m_Slots[ SLOTS_TOTALPRIVATE ] )
    {
        DWORD diff = m_Slots[ SLOTS_FILLEDPRIVATE ] - m_Slots[ SLOTS_TOTALPRIVATE ];
        m_Slots[ SLOTS_FILLEDPUBLIC ] += diff;

        assert( m_Slots[ SLOTS_FILLEDPUBLIC ] <= m_Slots[ SLOTS_TOTALPUBLIC ] );
        if( m_Slots[ SLOTS_FILLEDPUBLIC ] > m_Slots[ SLOTS_TOTALPUBLIC ] )
        {
            FatalError( "Too many slots filled!\n" );
        }

        m_Slots[ SLOTS_FILLEDPRIVATE ] = m_Slots[ SLOTS_TOTALPRIVATE ];
    }

    DebugDumpSessionDetails();
    DebugValidateExpectedSlotCounts();

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: RemoveLocalPlayers()
// Desc: Add local players to the session
//------------------------------------------------------------------------
HRESULT SessionManager::RemoveLocalPlayers( const DWORD dwUserCount,
                                            const DWORD* pdwUserIndices,
                                            const BOOL* pfPrivateSlots,
                                            XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateDelete )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    DWORD ret = ERROR_SUCCESS;

    // Do nothing if the session is already deleted
    if( m_SessionState == SessionStateDeleted )
    {
        return HRESULT_FROM_WIN32( ret );
    }

    #ifdef _DEBUG
    for( DWORD i = 0; i < dwUserCount; ++i )
    {
        XUID xuid;
        XUserGetXUID( i, &xuid );
        DebugSpew( "%016I64X: ProcessRemoveLocalPlayerMessage: 0x%016I64X\n", 
                   GetSessionIDAsInt(), 
                   xuid );
    }
    #endif

    ret = XSessionLeaveLocal( m_hSession,
                              dwUserCount,
                              pdwUserIndices,
                              pXOverlapped );

    // If the session is arbitrated and gameplay is happening, then the player will still be
    // kept in the session so stats can be reported, so we don't want to modify our slot counts
    const BOOL bPlayersKeptForStatsReporting = ( ( m_dwSessionFlags & XSESSION_CREATE_USES_ARBITRATION ) && 
                                                 ( m_SessionState == SessionStateInGame ) );

    if( !bPlayersKeptForStatsReporting )
    {
        for ( DWORD i = 0; i < dwUserCount; ++i )
        {
            m_Slots[ SLOTS_FILLEDPRIVATE ] -= ( pfPrivateSlots[ i ] ) ? 1 : 0;
            m_Slots[ SLOTS_FILLEDPUBLIC ]  -= ( pfPrivateSlots[ i ] ) ? 0 : 1;

            m_Slots[ SLOTS_FILLEDPRIVATE ] = max( m_Slots[ SLOTS_FILLEDPRIVATE ], 0 );
            m_Slots[ SLOTS_FILLEDPUBLIC ]  = max( m_Slots[ SLOTS_FILLEDPUBLIC ], 0 );
        }
    }
    else
    {
        DebugSpew( "%016I64X: ProcessRemoveLocalPlayerMessage: Arbitrated session, so leaving players become zombies and not updating local slot counts\n", 
                   GetSessionIDAsInt() );

        for ( DWORD i = 0; i < dwUserCount; ++i )
        {
            m_Slots[ SLOTS_ZOMBIEPRIVATE ] += ( pfPrivateSlots[ i ] ) ? 1 : 0;
            m_Slots[ SLOTS_ZOMBIEPUBLIC ]  += ( pfPrivateSlots[ i ] ) ? 0 : 1;
        }
    }

    DebugDumpSessionDetails();
    DebugValidateExpectedSlotCounts();

    return HRESULT_FROM_WIN32( ret );
}

//------------------------------------------------------------------------
// Name: RemoveRemotePlayers()
// Desc: Remove remote players to the session
//------------------------------------------------------------------------
HRESULT SessionManager::RemoveRemotePlayers( const DWORD dwXuidCount,
                                             const XUID* pXuids,
                                             const BOOL* pfPrivateSlots,
                                             XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateCreated || actual >= SessionStateDelete )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    #ifdef _DEBUG
    for( DWORD i = 0; i < dwXuidCount; ++i )
    {
        DebugSpew( "%016I64X: ProcessRemoveRemotePlayerMessage: 0x%016I64X\n", 
                   GetSessionIDAsInt(), 
                   pXuids[i] );
    }
    #endif

    DWORD ret = XSessionLeaveRemote( m_hSession,
                                     dwXuidCount,
                                     pXuids,
                                     pXOverlapped );

    {
        // If the session is arbitrated and gameplay is happening, then the player will still be
        // kept in the session so stats can be reported, so we don't want to modify our slot counts
        const BOOL bPlayersKeptForStatsReporting = ( ( m_dwSessionFlags & XSESSION_CREATE_USES_ARBITRATION ) && 
                                                     ( m_SessionState == SessionStateInGame ) );

        if( !bPlayersKeptForStatsReporting )
        {
            for ( DWORD i = 0; i < dwXuidCount; ++i )
            {
                m_Slots[ SLOTS_FILLEDPRIVATE ] -= ( pfPrivateSlots[ i ] ) ? 1 : 0;
                m_Slots[ SLOTS_FILLEDPUBLIC ]  -= ( pfPrivateSlots[ i ] ) ? 0 : 1;

                m_Slots[ SLOTS_FILLEDPRIVATE ] = max( m_Slots[ SLOTS_FILLEDPRIVATE ], 0 );
                m_Slots[ SLOTS_FILLEDPUBLIC ]  = max( m_Slots[ SLOTS_FILLEDPUBLIC ], 0 );
            }
        }
        else
        {
            DebugSpew( "%016I64X: ProcessRemoveRemotePlayerMessage: Arbitrated session, so leaving players become zombies and not updating local slot counts\n", 
                       GetSessionIDAsInt() );

            for ( DWORD i = 0; i < dwXuidCount; ++i )
            {
                m_Slots[ SLOTS_ZOMBIEPRIVATE ] += ( pfPrivateSlots[ i ] ) ? 1 : 0;
                m_Slots[ SLOTS_ZOMBIEPUBLIC ]  += ( pfPrivateSlots[ i ] ) ? 0 : 1;
            }
        }
    }

    DebugDumpSessionDetails();
    DebugValidateExpectedSlotCounts();

    return HRESULT_FROM_WIN32( ret );
}


//------------------------------------------------------------------------
// Name: WriteStats()
// Desc: Write stats for a player in the session
//------------------------------------------------------------------------
HRESULT SessionManager::WriteStats( const XUID xuid,
                                    const DWORD dwNumViews,
                                    const XSESSION_VIEW_PROPERTIES *pViews,
                                    XOVERLAPPED* pXOverlapped )
{
    SessionState actual = GetSessionState();
    if( actual < SessionStateInGame || actual >= SessionStateEnding )
    {
        FatalError( "Wrong state: %s\n", g_astrSessionState[actual] );
    }

    DebugSpew( "%016I64X: ProcessWriteStatsMessage: 0x%016I64X\n", 
               GetSessionIDAsInt(), 
               xuid );

    DWORD ret = XSessionWriteStats( m_hSession,
                                    xuid,
                                    dwNumViews,
                                    pViews,
                                    pXOverlapped );

    return HRESULT_FROM_WIN32( ret );
}
