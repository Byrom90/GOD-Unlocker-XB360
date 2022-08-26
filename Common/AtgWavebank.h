//--------------------------------------------------------------------------------------
// CWavebank.h
//
// Class for handling wavebank
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <XAct3wb.h>
namespace ATG
{
//--------------------------------------------------------------------------------------
// Name: CWavebank
// Desc: Class to handle the Wavebank file
//--------------------------------------------------------------------------------------
class CWavebank
{
public:
            CWavebank();
            ~CWavebank();

    // Initialization
    // Open a wavebank. The function allocates buffer for a wavebank
    HRESULT Open( const CHAR* strFileName );
    // Close the wavebank
    VOID    Close();
    // Get # of wave entries in the wavebank
    DWORD   GetEntryCount( void ) const;
    // Get a wave format of the wave entry
    HRESULT GetEntryFormat( const DWORD dwEntry, WAVEFORMATEX* pFormat ) const;
    // Get a duration of the wave entry
    DWORD   GetEntryLengthInBytes( const DWORD dwEntry ) const;
    // Get offset of the wave entry in the bank file
    DWORD   GetEntryOffset( const DWORD dwEntry ) const;
    // Get samples of the wave entry
    HRESULT GetEntryData( const DWORD dwEntry, void** pData ) const;
private:
    DWORD m_dwFileSize;
    WAVEBANKHEADER m_wavebankHeader;
    WAVEBANKDATA m_wavebankData;
    WAVEBANKENTRY* m_pDataEntry;
    BYTE* m_pSeekData;
    HANDLE m_hFile;
};
}
