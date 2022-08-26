//--------------------------------------------------------------------------------------
// CWavebank.cpp
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include <xtl.h>
#include "AtgUtil.h"
#include "AtgWavebank.h"
#include "xma2defs.h"

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: CWavebank::Ctor
// Desc: 
//--------------------------------------------------------------------------------------
CWavebank::CWavebank() :
    m_dwFileSize(0),
    m_pDataEntry(NULL),
    m_pSeekData(NULL),
    m_hFile(INVALID_HANDLE_VALUE)
{
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::Dtor
// Desc: 
//--------------------------------------------------------------------------------------
CWavebank::~CWavebank()
{
    Close();
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::Open
// Desc: Open the wave bank file and verify
//--------------------------------------------------------------------------------------
HRESULT CWavebank::Open( const CHAR* strFileName )
{
    assert( strFileName != NULL );

    HRESULT hr = S_OK;

    // Open the file
    m_hFile = CreateFile(
        strFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0L,
        NULL );
    if( m_hFile == INVALID_HANDLE_VALUE )
        ATG::FatalError( "Error opening Wavebank file\n" );

    //
    // Load header
    //
    DWORD cbBytesRead;
    if( !ReadFile( m_hFile, &m_wavebankHeader, sizeof( WAVEBANKHEADER ), &cbBytesRead, NULL ) )
        ATG::FatalError( "Couldn't read wavebank header\n" );

    // Verify header
    if( WAVEBANK_HEADER_SIGNATURE != m_wavebankHeader.dwSignature )
    {
        ATG::FatalError( "Invalid Wavebank format\n" );
    }
    if( WAVEBANK_HEADER_VERSION < m_wavebankHeader.dwHeaderVersion )
    {
        ATG::FatalError(
            "Wave bank version (%i) more recent than this tool supports (%i).\n",
            m_wavebankHeader.dwHeaderVersion, WAVEBANK_HEADER_VERSION );
    }

    //
    // Load WAVEBANKDATA
    //
    SetFilePointer( m_hFile, m_wavebankHeader.Segments[WAVEBANK_SEGIDX_BANKDATA].dwOffset, 0, SEEK_SET );
    if( !ReadFile( m_hFile, &m_wavebankData, sizeof( WAVEBANKDATA ), &cbBytesRead, NULL ) )
        ATG::FatalError( "Couldn't read wavebank data block\n" );


    //
    // Load entries
    //
    DWORD cbEntries = m_wavebankHeader.Segments[WAVEBANK_SEGIDX_ENTRYMETADATA].dwLength;
    m_pDataEntry = new WAVEBANKENTRY[ cbEntries / sizeof( WAVEBANKENTRY ) ];
    SetFilePointer( m_hFile, m_wavebankHeader.Segments[WAVEBANK_SEGIDX_ENTRYMETADATA].dwOffset, 0, SEEK_SET );
    if( !ReadFile( m_hFile, m_pDataEntry, cbEntries, &cbBytesRead, NULL ) )
        ATG::FatalError( "Couldn't read wavebank entry metadata\n" );

    //
    // Load seek tables (if any)
    //
    DWORD cbSeekTables = m_wavebankHeader.Segments[WAVEBANK_SEGIDX_SEEKTABLES].dwLength;
    if (cbSeekTables > 0 )
    {
        m_pSeekData = new BYTE[ cbSeekTables ];
        SetFilePointer( m_hFile, m_wavebankHeader.Segments[WAVEBANK_SEGIDX_SEEKTABLES].dwOffset, 0, SEEK_SET );
        if( !ReadFile( m_hFile, m_pSeekData, cbSeekTables, &cbBytesRead, NULL ) )
           ATG::FatalError( "Couldn't read wavebank seek tables\n" );
    } 

    return hr;
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::Close
// Desc: 
//--------------------------------------------------------------------------------------
VOID CWavebank::Close()
{
    delete [] m_pDataEntry;
    m_pDataEntry = NULL;

    delete [] m_pSeekData;
    m_pSeekData = NULL;

    CloseHandle( m_hFile );
    m_hFile = INVALID_HANDLE_VALUE;
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::GetEntries
// Desc: Retrieve a number of entries in the wavebank
//--------------------------------------------------------------------------------------
DWORD CWavebank::GetEntryCount( void ) const
{
    return m_wavebankData.dwEntryCount;
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::GetFormat
// Desc: Get a wave format
//--------------------------------------------------------------------------------------
HRESULT CWavebank::GetEntryFormat( const DWORD dwEntry, WAVEFORMATEX* pFormat ) const
{
    assert( m_pDataEntry != NULL );
    assert( pFormat != NULL );

    // Check entry #
    if( dwEntry >= m_wavebankData.dwEntryCount )
        return S_FALSE;

    const WAVEBANKMINIWAVEFORMAT& miniFmt =
        m_wavebankData.dwFlags & WAVEBANK_FLAGS_COMPACT
        ? m_wavebankData.CompactFormat
        : m_pDataEntry[ dwEntry ].Format;

    switch( miniFmt.wFormatTag )
    {
        case WAVEBANKMINIFORMAT_TAG_PCM:
            pFormat->wFormatTag = WAVE_FORMAT_PCM;
            pFormat->cbSize = 0;
            break;

        case WAVEBANKMINIFORMAT_TAG_XMA:
            pFormat->wFormatTag = WAVE_FORMAT_XMA2;
            pFormat->cbSize = sizeof(XMA2WAVEFORMATEX) - sizeof(WAVEFORMATEX);
            {
                XMA2WAVEFORMATEX* xma2Fmt = reinterpret_cast<XMA2WAVEFORMATEX*>(pFormat);

                WORD wBlockCount = 0;

                // See if we have a seek table and use it for the block count
                if ( m_pSeekData != NULL )
                {
                    WAVEBANKOFFSET offset = ((WAVEBANKOFFSET*)m_pSeekData)[ dwEntry ];
                    if (offset != 0xffffffff) /* XACTOFFSET_INVALID */
                    {
                        const DWORD* pEntrySeekTable = reinterpret_cast<const DWORD*>(m_pSeekData + sizeof(WAVEBANKOFFSET)*m_wavebankData.dwEntryCount + offset);
                        wBlockCount = (WORD)pEntrySeekTable[0];
                    }
                }

                miniFmt.XMA2FillFormatEx( xma2Fmt, wBlockCount, &m_pDataEntry[ dwEntry ] );
            }
            break;

        case WAVEBANKMINIFORMAT_TAG_WMA:
            pFormat->wFormatTag = (miniFmt.wBitsPerSample & 0x1) ? WAVE_FORMAT_WMAUDIO3 : WAVE_FORMAT_WMAUDIO2;
            pFormat->cbSize = 0;
            break;

        default:
            // WAVEBANKMINIFORMAT_TAG_ADPCM is only valid for Windows
            return E_FAIL;
    }

    pFormat->nChannels = miniFmt.nChannels;
    pFormat->wBitsPerSample = miniFmt.BitsPerSample();
    pFormat->nBlockAlign = (WORD) miniFmt.BlockAlign();
    pFormat->nSamplesPerSec = miniFmt.nSamplesPerSec;
    pFormat->nAvgBytesPerSec = miniFmt.nSamplesPerSec * miniFmt.wBlockAlign;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::GetDuration
// Desc: Get a duration of a wave entry
//--------------------------------------------------------------------------------------
DWORD CWavebank::GetEntryLengthInBytes( const DWORD dwEntry ) const
{
    assert( m_pDataEntry != NULL );

    DWORD result = 0;

    // Check entry #
    if( dwEntry < m_wavebankData.dwEntryCount )
    {
        // Fill out format data
        result = m_pDataEntry[ dwEntry ].PlayRegion.dwLength;
    }

    return result;
}


//--------------------------------------------------------------------------------------
// Name: CWavebank::GetEntryData
// Desc: Retrieve a pointer of sample data
//--------------------------------------------------------------------------------------
HRESULT CWavebank::GetEntryData( const DWORD dwEntry, void** pSample ) const
{
    assert( m_pDataEntry != NULL );
    assert( pSample );
    *pSample = NULL;

    HRESULT hr = S_OK;

    // Check entry #
    if( dwEntry >= m_wavebankData.dwEntryCount )
        hr = S_FALSE;

    *pSample = new BYTE[m_pDataEntry[dwEntry].PlayRegion.dwLength];

    DWORD cbRead;
    SetFilePointer( m_hFile, m_pDataEntry[dwEntry].PlayRegion.dwOffset, 0L, SEEK_SET );
    if( !ReadFile( m_hFile, ( *pSample ), m_pDataEntry[dwEntry].PlayRegion.dwLength, &cbRead, NULL ) )
        hr = E_FAIL;

    if( FAILED( hr ) )
    {
        delete *pSample;
        pSample = NULL;
    }
    return hr;
}

//--------------------------------------------------------------------------------------
// Name: CWavebank::GetEntryOffset
// Desc: Retrieve a pointer of sample data
//--------------------------------------------------------------------------------------
DWORD CWavebank::GetEntryOffset( const DWORD dwEntry ) const
{
    assert( m_pDataEntry != NULL );

    DWORD result = 0;

    // Check entry #
    if( dwEntry < m_wavebankData.dwEntryCount )
    {
        result =
            0//m_wavebankHeader.Segments[WAVEBANK_SEGIDX_ENTRYWAVEDATA].dwOffset
            + m_pDataEntry[ dwEntry ].PlayRegion.dwOffset;
    }
    return result;
}



} // namespace ATG
