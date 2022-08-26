//--------------------------------------------------------------------------------------
// AtgMediaLocator.h
//
// Helper functions and class providing support for locating media files inside and xzp 
// archive.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


#pragma once
#ifndef ATGMEDIALOCATOR_H
#define ATGMEDIALOCATOR_H


namespace ATG
{


const DWORD LOCATOR_SIZE = 256; // Use this to allocate space to hold a ResourceLocator string 


BOOL LocateMediaFolder(  LPWSTR szMediaPath, DWORD dwMediaPathSize, LPWSTR szPackage );

BOOL ComposeResourceLocator( LPWSTR szLocator, DWORD dwLocatorSize, LPCWSTR szPackage, LPCWSTR szBaseDirectory, LPCWSTR szPath, LPCWSTR szFile );


//--------------------------------------------------------------------------------------
// Name: class MediaLocator
// Desc: Find paths to files in the media folder that sits inside an xzp archive.
//--------------------------------------------------------------------------------------
class MediaLocator
{
public:
    MediaLocator();
    MediaLocator( LPCWSTR szPackage );
    ~MediaLocator() {}

    VOID SetPackage( LPCWSTR szPackage );
    LPCWSTR GetMediaPath() const;

    BOOL ComposeResourceLocator( LPWSTR szLocator, DWORD dwpLocatorSize, LPCWSTR szPath, LPCWSTR szFile ) const;

private:
    MediaLocator( const MediaLocator& mediaLocator );
    MediaLocator& operator =( const MediaLocator& mediaLocator );

    WCHAR m_szPackage[ LOCATOR_SIZE ];
    mutable WCHAR m_szMediaPath[ LOCATOR_SIZE ];
};


} // namespace ATG


#endif // ATGMEDIARESOURCE_H