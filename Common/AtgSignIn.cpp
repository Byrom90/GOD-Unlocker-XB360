//--------------------------------------------------------------------------------------
// AtgSignIn.cpp
//
// Handler for automatic signin
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgSignIn.h"
#include "AtgUtil.h"

namespace ATG
{

//--------------------------------------------------------------------------------------
// Static members
//--------------------------------------------------------------------------------------

DWORD  SignIn::m_dwMinUsers                   = 0;     
DWORD  SignIn::m_dwMaxUsers                   = 4;     
BOOL   SignIn::m_bRequireOnlineUsers          = FALSE; 
DWORD  SignIn::m_dwSignInPanes                = 4;     
HANDLE SignIn::m_hNotification                = NULL;  
DWORD  SignIn::m_dwSignedInUserMask           = 0;     
DWORD  SignIn::m_dwNumSignedInUsers           = 0;     
DWORD  SignIn::m_dwOnlineUserMask             = 0;     
DWORD  SignIn::m_dwFirstSignedInUser          = (DWORD)-1;
BOOL   SignIn::m_bSystemUIShowing             = FALSE; 
BOOL   SignIn::m_bNeedToShowSignInUI          = FALSE; 
BOOL   SignIn::m_bMessageBoxShowing           = FALSE;
BOOL   SignIn::m_bSigninUIWasShown            = FALSE;
XOVERLAPPED SignIn::m_Overlapped              = {0};
LPCWSTR SignIn::m_pwstrButtons[2]             = { L"Exit", L"Sign In" };
MESSAGEBOX_RESULT SignIn::m_MessageBoxResult  = {0};

//--------------------------------------------------------------------------------------
// Name: Initialize()
// Desc: Sets up variables and creates notification listener
//--------------------------------------------------------------------------------------
VOID SignIn::Initialize( DWORD dwMinUsers,
                         DWORD dwMaxUsers,
                         BOOL bRequireOnlineUsers,
                         DWORD dwSignInPanes )
{
    // Sanity check inputs
    assert( dwMaxUsers <= 4 && dwMinUsers <= dwMaxUsers );
    assert( dwSignInPanes <= 4 && dwSignInPanes != 3 );

    // Assign variables
    m_dwMinUsers = dwMinUsers;
    m_dwMaxUsers = dwMaxUsers;
    m_bRequireOnlineUsers = bRequireOnlineUsers;
    m_dwSignInPanes = dwSignInPanes;

    // Register our notification listener
    m_hNotification = XNotifyCreateListener( XNOTIFY_SYSTEM | XNOTIFY_LIVE );
    if( m_hNotification == NULL || m_hNotification == INVALID_HANDLE_VALUE )
    {
        ATG::FatalError( "Failed to create state notification listener.\n" );
    }

    // Query who is signed in
    QuerySigninStatus();
}


//--------------------------------------------------------------------------------------
// Name: QuerySigninStatus()
// Desc: Query signed in status of all users.
//--------------------------------------------------------------------------------------
VOID SignIn::QuerySigninStatus()
{
    m_dwSignedInUserMask = 0;
    m_dwOnlineUserMask = 0;

    // Count the signed-in users
    m_dwNumSignedInUsers = 0;
    m_dwFirstSignedInUser = ( DWORD )-1;

    for( UINT nUser = 0;
         nUser < XUSER_MAX_COUNT;
         nUser++ )
    {
        XUSER_SIGNIN_STATE State = XUserGetSigninState( nUser );

        if( State != eXUserSigninState_NotSignedIn )
        {
            // Check whether the user is online
            BOOL bUserOnline =
                State == eXUserSigninState_SignedInToLive;

            m_dwOnlineUserMask |= bUserOnline << nUser;

            // If we want Online users only, only count signed-in users
            if( !m_bRequireOnlineUsers || bUserOnline )
            {
                m_dwSignedInUserMask |= ( 1 << nUser );

                if( m_dwFirstSignedInUser == ( DWORD )-1 )
                {
                    m_dwFirstSignedInUser = nUser;
                }

                ++m_dwNumSignedInUsers;
            }
        }
    }

    // check to see if we need to invoke the signin UI
    m_bNeedToShowSignInUI = !AreUsersSignedIn();
}

//--------------------------------------------------------------------------------------
// Name: Update()
// Desc: Does required per-frame processing for signin
//--------------------------------------------------------------------------------------
DWORD SignIn::Update()
{
    assert( m_hNotification != NULL );  // ensure Initialize() was called

    DWORD dwRet = 0;

    // Check for system notifications
    DWORD dwNotificationID;
    ULONG_PTR ulParam;

    //
    // For XN_SYS_SIGNINCHANGED, handle the case where the system sends a spurious
    // notification. See  the FAQ on 
    // Xbox 360 Central: https://xds.xbox.com/xbox360/nav.aspx?Page=devsupport/sitefaq.htm#misc17
    // for a description of the workaround
    //
    static const DWORD dwTimeBeforeTrustingSignInChangedNotif = 1000;
    static DWORD dwQuestionableSigninChangeNotifReceived = GetTickCount();
    static UINT  cQuestionableSigninChangeNotifReceived = 0;

    if( XNotifyGetNext( m_hNotification, 0, &dwNotificationID, &ulParam ) )
    {
        switch( dwNotificationID )
        {
            case XN_SYS_SIGNINCHANGED:

                // Query who is signed in
                QuerySigninStatus();

                //
                // Might be a "spurious notification". See comments above.
                if( m_dwSignedInUserMask == 0 )
                {
                    ++cQuestionableSigninChangeNotifReceived;
                    if( cQuestionableSigninChangeNotifReceived == 1 )
                    {
                        // If a second signin changed notification arrives within dwTimeBeforeTrustingSignInChangedNotif,
                        // trust it instead of this one
                        dwQuestionableSigninChangeNotifReceived = GetTickCount();
                    }
                    else if( ( cQuestionableSigninChangeNotifReceived > 1 )  && 
                             ( GetTickCount() - dwQuestionableSigninChangeNotifReceived <= dwTimeBeforeTrustingSignInChangedNotif ) )
                    {
                        dwRet |= SIGNIN_USERS_CHANGED;
                    }
                }
                else
                {
                    dwRet |= SIGNIN_USERS_CHANGED;
                }

                // Reset dwQuestionableSigninChangeNotifReceived to 0 if the notification is valid
                if( dwRet & SIGNIN_USERS_CHANGED )
                {
                    cQuestionableSigninChangeNotifReceived = 0;
                }

                break;

            case XN_SYS_UI:
                dwRet |= SYSTEM_UI_CHANGED;
                m_bSystemUIShowing = static_cast<BOOL>( ulParam );

                // check to see if we need to invoke the signin UI
                m_bNeedToShowSignInUI = !AreUsersSignedIn();
                break;

            case XN_LIVE_CONNECTIONCHANGED:
                dwRet |= CONNECTION_CHANGED;
                break;

        } // switch( dwNotificationID )
    } // if( XNotifyGetNext() )

    // If there are not enough or too many profiles signed in, display an 
    // error message box prompting the user to either sign in again or exit the sample
    if( !m_bMessageBoxShowing && !m_bSystemUIShowing && m_bSigninUIWasShown && !AreUsersSignedIn() )
    {
        DWORD dwResult;

        ZeroMemory( &m_Overlapped, sizeof( XOVERLAPPED ) );

        WCHAR strMessage[512];
        swprintf_s( strMessage, L"Incorrect number of profiles signed in. You must sign in at least %d"
                    L" and at most %d profiles. Currently there are %d profiles signed in.",
                    m_dwMinUsers, m_dwMaxUsers, m_dwNumSignedInUsers );

        dwResult = XShowMessageBoxUI( XUSER_INDEX_ANY,
                                      L"Signin Error",   // Message box title
                                      strMessage,                 // Message
                                      ARRAYSIZE( m_pwstrButtons ),// Number of buttons
                                      m_pwstrButtons,             // Button captions
                                      0,                          // Button that gets focus
                                      XMB_ERRORICON,              // Icon to display
                                      &m_MessageBoxResult,        // Button pressed result
                                      &m_Overlapped );

        if( dwResult != ERROR_IO_PENDING )
            ATG::FatalError( "Failed to invoke message box UI, error %d\n", dwResult );

        m_bSystemUIShowing = TRUE;
        m_bMessageBoxShowing = TRUE;
    }

    // Wait until the message box is discarded, then either exit or show the signin UI again
    if( m_bMessageBoxShowing && XHasOverlappedIoCompleted( &m_Overlapped ) )
    {
        m_bMessageBoxShowing = FALSE;

        if( XGetOverlappedResult( &m_Overlapped, NULL, TRUE ) == ERROR_SUCCESS )
        {
            switch( m_MessageBoxResult.dwButtonPressed )
            {
                case 0:     // Reboot to the launcher
                    XLaunchNewImage( "", 0 );
                    break;

                case 1:     // Show the signin UI again
                    ShowSignInUI();
                    m_bSigninUIWasShown = FALSE;
                    break;
            }
        }
    }

    // Check to see if we need to invoke the signin UI
    if( !m_bMessageBoxShowing && m_bNeedToShowSignInUI && !m_bSystemUIShowing )
    {
        m_bNeedToShowSignInUI = FALSE;

        DWORD ret = XShowSigninUI(
            m_dwSignInPanes,
            m_bRequireOnlineUsers ? XSSUI_FLAGS_SHOWONLYONLINEENABLED : 0 );

        if( ret != ERROR_SUCCESS )
        {
            ATG::FatalError( "Failed to invoke signin UI, error %d\n", ret );
        }
        else
        {
            m_bSystemUIShowing = TRUE;
            m_bSigninUIWasShown = TRUE;
        }
    }

    return dwRet;
}

//--------------------------------------------------------------------------------------
// Name: CheckPrivilege()
// Desc: Test to see if a user has a required privilege
//--------------------------------------------------------------------------------------
BOOL SignIn::CheckPrivilege( DWORD dwController, XPRIVILEGE_TYPE priv )
{
    BOOL bResult;

    return
        ( XUserCheckPrivilege( dwController, priv, &bResult ) == ERROR_SUCCESS ) &&
        bResult;
}

} // namespace ATG
