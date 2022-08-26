//--------------------------------------------------------------------------------------
// AtgSignIn.h
//
// Helper class to automatically handle signin
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGSIGNIN_H
#define ATGSIGNIN_H

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: class SignIn
// Desc: Class to manage maintaining signed-in users. This class is a singleton; all
//       members are static
//--------------------------------------------------------------------------------------
class SignIn
{
public:
    // Flags that can be returned from Update()
    static enum SIGNIN_UPDATE_FLAGS
    {
        SIGNIN_USERS_CHANGED    = 0x00000001,
        SYSTEM_UI_CHANGED       = 0x00000002,
        CONNECTION_CHANGED      = 0x00000004
    };

    // Check users that are signed in
    static DWORD    GetSignedInUserCount()
    {
        return m_dwNumSignedInUsers;
    }
    static DWORD    GetSignedInUserMask()
    {
        return m_dwSignedInUserMask;
    }
    static BOOL     IsUserSignedIn( DWORD dwController )
    {
        return ( m_dwSignedInUserMask & ( 1 << dwController ) ) != 0;
    }

    static BOOL     AreUsersSignedIn()
    {
        return ( m_dwNumSignedInUsers >= m_dwMinUsers ) &&
            ( m_dwNumSignedInUsers <= m_dwMaxUsers );
    }

    // Get the first signed-in user
    static DWORD    GetSignedInUser()
    {
        return m_dwFirstSignedInUser;
    }

    // Check users that are signed into live
    static DWORD    GetOnlineUserMask()
    {
        return m_dwOnlineUserMask;
    }
    static BOOL     IsUserOnline( DWORD dwController )
    {
        return ( m_dwOnlineUserMask & ( 1 << dwController ) ) != 0;
    }

    // Check the presence of system UI
    static BOOL     IsSystemUIShowing()
    {
        return m_bSystemUIShowing || m_bNeedToShowSignInUI;
    }

    // Function to reinvoke signin UI
    static VOID     ShowSignInUI()
    {
        m_bNeedToShowSignInUI = TRUE;
    }

    // Check privileges for a signed-in users
    static BOOL     CheckPrivilege( DWORD dwController, XPRIVILEGE_TYPE priv );

    // Methods to drive autologin
    static VOID Initialize( 
        DWORD dwMinUsers, 
        DWORD dwMaxUsers,
        BOOL  bRequireOnlineUsers,
        DWORD dwSignInPanes );

    static DWORD    Update();

private:

    // Private constructor to prevent instantiation
                    SignIn();

    // Parameters
    static DWORD m_dwMinUsers;             // minimum users to accept as signed in
    static DWORD m_dwMaxUsers;             // maximum users to accept as signed in
    static BOOL m_bRequireOnlineUsers;    // online profiles only
    static DWORD m_dwSignInPanes;          // number of panes to show in signin UI

    // Internal variables
    static HANDLE m_hNotification;                // listener to accept notifications
    static DWORD m_dwSignedInUserMask;           // bitfields for signed-in users
    static DWORD m_dwFirstSignedInUser;          // first signed-in user
    static DWORD m_dwNumSignedInUsers;           // number of signed-in users
    static DWORD m_dwOnlineUserMask;             // users who are online
    static BOOL m_bSystemUIShowing;             // system UI present
    static BOOL m_bNeedToShowSignInUI;          // invoking signin UI necessary
    static BOOL m_bMessageBoxShowing;           // is retry signin message box showing?
    static BOOL m_bSigninUIWasShown;            // was the signin ui shown at least once?
    static XOVERLAPPED m_Overlapped;              // message box overlapped struct
    static LPCWSTR  m_pwstrButtons[2];             // message box buttons
    static MESSAGEBOX_RESULT m_MessageBoxResult;  // message box button pressed

    static VOID     QuerySigninStatus();              // Query signed in users

};

} // namespace ATG

#endif // ATGSIGNIN_H
