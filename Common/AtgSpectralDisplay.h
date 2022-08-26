//--------------------------------------------------------------------------------------
// SpectralDisplay.h
//
// XNA Developer Connection
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#include <xtl.h>
#include <fxl.h>
#include "ATGAPOBase.h"
#include "AtgLockFreePipe.h"

namespace ATG
{

#ifndef MONITOR_APO_PIPE_LEN
#define MONITOR_APO_PIPE_LEN 14
#endif

typedef ATG::LockFreePipe<MONITOR_APO_PIPE_LEN> MonitorAPOPipe;

struct MonitorAPOParams
{
    MonitorAPOPipe *pipe;
};

class __declspec( uuid("{A4945B8A-EB14-4c96-8067-DF726B528091}"))
CMonitorAPO
: public CSampleXAPOBase<CMonitorAPO, MonitorAPOParams>
{
public:
    CMonitorAPO();
    ~CMonitorAPO();

    void DoProcess( const MonitorAPOParams&, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels, BOOL bEnabled );
};

class CWaveDisplay
{
public:
    CWaveDisplay();
    ~CWaveDisplay();

    const static DWORD NSAMPLESLOG2 = 8;
    const static DWORD nSamples = 1 << NSAMPLESLOG2;

    HRESULT Initialize( ::IDirect3DDevice9* pDevice, UINT nSamples );
    HRESULT Update( float* __restrict pSamples, const BOOL bMonitorTimeDomain = TRUE );
    HRESULT Render( ::IDirect3DDevice9* pDevice, const D3DRECT& pBounds );

    void SetRange( float min, float max );
private:
    // unity table, used with FFT
    const static int nBinsLog2 = 8;
    const static int nBins = 1 << nBinsLog2;
    __vector4 m_UnityTable[nBins];

	CRITICAL_SECTION m_cs;
    UINT m_nSamples;
	FLOAT* m_pSampleBuffer;

    // FX objects
    FXLEffect*  m_pEffect;

    // Handles for the effect
    FXLHANDLE   m_hAmplitudeSampler;
    FXLHANDLE   m_hSampleCount;
    FXLHANDLE   m_hMin;
    FXLHANDLE   m_hMax;

    IDirect3DTexture9*  m_pAmplitudeTexture;
	
};


} // NameSpace ATG
