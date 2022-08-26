//--------------------------------------------------------------------------------------
// AtgMediaLocator.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#include "stdafx.h"
#include <xuiresource.h.>
#include "AtgMediaLocator.h"


namespace ATG
{


const WCHAR XZP_SEPARATOR  = L'#';
const WCHAR MEDIA_FOLDER[] = { L"media/" };

//--------------------------------------------------------------------------------------
// Name: LocateMediaFolder()
// Desc: Retrieve the path to the media folder that sits inside the xzp archive 
//       specified in szPackage. If the function succeeds, it returns TRUE and 
//       szMediaPath contains the relative path to the media folder 
//       (eg: "..\..\media\"). If the function fails, it returns FALSE and szMediaPath
//       is unaffected.
//--------------------------------------------------------------------------------------
BOOL LocateMediaFolder( LPWSTR szMediaPath, DWORD dwMediaPathSize, LPCWSTR szPackage )
{
    // Open the archive file
    HXUIPACKAGE hPackage;
    HRESULT hr = XuiResourceOpenPackage( const_cast< LPWSTR >( szPackage ), &hPackage, FALSE );
    if( hr != S_OK )
        return FALSE;

    UINT entryCount = XuiResourceGetPackageEntryCount( hPackage );
    WCHAR szLocator[ LOCATOR_SIZE ];
    UINT LocatorSize = ARRAYSIZE( szLocator );
    for( UINT i = 0; i < entryCount; ++ i )
    {
        hr = XuiResourceGetPackageEntryInfo( hPackage, i, szLocator, &LocatorSize, NULL, NULL, NULL );

        LPWSTR szPathStart = wcschr( szLocator, XZP_SEPARATOR ) + 1;
        assert( szPathStart != NULL );

        LPWSTR szMediaFolder = wcsstr( szPathStart, MEDIA_FOLDER );
        if( szMediaFolder != NULL )
        {
            wcsncpy_s( szMediaPath, 
                       dwMediaPathSize, 
                       szPathStart, 
                       szMediaFolder + wcslen( MEDIA_FOLDER ) - szPathStart );
            XuiResourceReleasePackage( hPackage );
            return TRUE;
            break;
        }
    }

    XuiResourceReleasePackage( hPackage );
    return FALSE;
}


//--------------------------------------------------------------------------------------
// Name: ComposeResourceLocator()
// Desc: Combines the strings from szPackage, szBaseDirectory, szPath and szFile into a 
//       valid resource locator. If successful, the function returns TRUE and szLocator 
//       points to the newly composed resource locator. If it fails, the function 
//       returns FALSE and the content of szLocator is undetermined.
//
// Note: szPackage is required and cannot be NULL, the other input parameters can be set 
//       to NULL if desired.
//--------------------------------------------------------------------------------------
BOOL ComposeResourceLocator( LPWSTR szLocator, DWORD dwLocatorSize, 
                             LPCWSTR szPackage, LPCWSTR szBaseDirectory, LPCWSTR szPath, LPCWSTR szFile )
{
    assert( szPackage != NULL );

    if( wcscpy_s( szLocator, dwLocatorSize, szPackage ) != 0 )
        return FALSE;

    if( wcscat_s( szLocator, dwLocatorSize, L"#" ) != 0 )
        return FALSE;

    if( szBaseDirectory != NULL )
    {
        if( wcscat_s( szLocator, dwLocatorSize, szBaseDirectory ) != 0 )
            return FALSE;
    }

    if( szPath != NULL )
    {
        if( wcscat_s( szLocator, dwLocatorSize, szPath ) != 0 )
            return FALSE;
    }

    if( szFile != NULL )
    {
        if( wcscat_s( szLocator, dwLocatorSize, szFile ) != 0 )
            return FALSE;
    }

    return TRUE;
}
    

//--------------------------------------------------------------------------------------
// Name: MediaLocator::MediaLocator()
// Desc: Constructs a MediaLocator object with default values. The caller should call 
//       SetPackage() before using the class.
//--------------------------------------------------------------------------------------
MediaLocator::MediaLocator()
{ 
    m_szPackage[ 0 ] = L'\0'; 
    m_szMediaPath[ 0 ] = L'\0'; 
}


//--------------------------------------------------------------------------------------
// Name: MediaLocator::MediaLocator()
// Desc: Constructs a MediaLocator object for use with the specified xzp archive.
//--------------------------------------------------------------------------------------
MediaLocator::MediaLocator( LPCWSTR szPackage ) 
{ 
    wcscpy_s( m_szPackage, szPackage ); 
    m_szMediaPath[ 0 ] = L'\0'; 
}


//--------------------------------------------------------------------------------------
// Name: MediaLocator::SetPackage()
// Desc: Replace the current working xzp archive with a new one.
//--------------------------------------------------------------------------------------
VOID MediaLocator::SetPackage( LPCWSTR szPackage )
{ 
    wcscpy_s( m_szPackage, szPackage ); 
    m_szMediaPath[ 0 ] = L'\0'; 
}

//--------------------------------------------------------------------------------------
// Name: MediaLocator::GetMediaPath()
// Desc: Retrieves the path to the media folder that sits inside the xzp archive.
//--------------------------------------------------------------------------------------
LPCWSTR MediaLocator::GetMediaPath() const
{ 
    if( m_szMediaPath[ 0 ] == L'\0' && m_szPackage[ 0 ] != L'\0' )
    { 
        LocateMediaFolder( m_szMediaPath, ARRAYSIZE( m_szMediaPath ), m_szPackage ); 
    } 
    
    return m_szMediaPath; 
}


//--------------------------------------------------------------------------------------
// Name: MediaLocator::ComposeResourceLocator()
// Desc: If succesful, returns TRUE and szLocator contains the resource locator for the
//       file identified by szPath and szFile that sits inside the media folder of the 
//       xzp archive. It returns FALSE upon failure.
//--------------------------------------------------------------------------------------
BOOL MediaLocator::ComposeResourceLocator( LPWSTR szLocator, DWORD dwLocatorSize, 
                                           LPCWSTR szPath, LPCWSTR szFile         ) const
{ 
   if( m_szPackage[ 0 ] == L'\0' )
       return FALSE;

   return ATG::ComposeResourceLocator( szLocator, dwLocatorSize, m_szPackage, GetMediaPath(), szPath, szFile ); 
}


} // namespace ATG