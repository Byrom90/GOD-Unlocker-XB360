//--------------------------------------------------------------------------------------
// ATGSpectralDisplay.cpp
//
// XNA Developer Connection
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "ATGSpectralDisplay.h"
#include "AtgUtil.h"
#include "XDSP.h"

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: CWaveDisplay::CWaveDisplay
// Desc: Constructor
//--------------------------------------------------------------------------------------
CWaveDisplay::CWaveDisplay()
: m_nSamples(0)
, m_hSampleCount( NULL )
, m_hAmplitudeSampler( NULL )
, m_pAmplitudeTexture( NULL )
, m_pEffect( NULL )
, m_pSampleBuffer( NULL )
{
}

//--------------------------------------------------------------------------------------
// Name: CWaveDisplay::Initialize
// Desc: Initilization for the waveform-rendering
//--------------------------------------------------------------------------------------
HRESULT CWaveDisplay::Initialize( ::IDirect3DDevice9* pDevice, UINT nSamples )
{
    HRESULT hr = S_OK;

    m_nSamples = nSamples;
	m_pSampleBuffer = new FLOAT[m_nSamples];
	InitializeCriticalSectionAndSpinCount(&m_cs, 1000);

    //
    // Load and compile the wave display effect
    //
    VOID* pCode;
    DWORD dwSize;
    if( FAILED( LoadFile( "game:\\Media\\Effects\\audio.fx", &pCode, &dwSize ) ) )
        FatalError( "Couldn't load file\n" );

    // Compile an effect
    LPD3DXBUFFER pEffectData;
    LPD3DXBUFFER pErrorList;

    DWORD CompileFlags = D3DXSHADER_FXLPARAMETERS_AS_VARIABLE_NAMES;

    if( FAILED( FXLCompileEffect( (CHAR*)pCode, dwSize,
        NULL, NULL,
        CompileFlags,
        &pEffectData, &pErrorList ) ) )
    {
        pErrorList->Release();
        FatalError( "Couldn't compile effect\n" );
    }

    // Create effect
    if( FAILED( FXLCreateEffect( pDevice, pEffectData->GetBufferPointer(),
        NULL, &m_pEffect ) ) )
        FatalError( "Couldn't compile audio.fx\n" );

    UnloadFile( pCode );
    pEffectData->Release();

    //
    // Create texture to hold wave sample values
    //
    if( FAILED( pDevice->CreateTexture( nSamples, 1, 1, 0, D3DFMT_LIN_R32F, 0, &m_pAmplitudeTexture, NULL )))
        FatalError( "Couldn't create texture for wave output" );

    // Initialize unity roots lookup table used by FFT functions
    XDSP::FFTInitializeUnityTable(m_UnityTable, nBins);

    // Zero-fill the wave texture
    D3DLOCKED_RECT rect;
    m_pAmplitudeTexture->LockRect( 0, &rect, NULL, 0 );
    memset( rect.pBits, 0, nSamples * sizeof(float) );
    m_pAmplitudeTexture->UnlockRect(0);

    m_hSampleCount = m_pEffect->GetParameterHandle( "nSamples" );
    if( m_hSampleCount )
        m_pEffect->SetFloat( m_hSampleCount, (float)nSamples );
    m_hAmplitudeSampler = m_pEffect->GetParameterHandle( "amplitude_sampler" );
    if( m_hAmplitudeSampler )
        m_pEffect->SetSampler( m_hAmplitudeSampler, m_pAmplitudeTexture );

    m_hMin = m_pEffect->GetParameterHandle( "rangeMin" );
    m_hMax = m_pEffect->GetParameterHandle( "rangeMax" );

    return hr;
}

//--------------------------------------------------------------------------------------
// Name: CWaveDisplay::~CWaveDisplay
// Desc: Destructor, free textures
//--------------------------------------------------------------------------------------
CWaveDisplay::~CWaveDisplay(void)
{
    if( m_pEffect )
        m_pEffect->Release();

    if( m_pAmplitudeTexture )
        m_pAmplitudeTexture->Release();

	delete[] m_pSampleBuffer;
}


//--------------------------------------------------------------------------------------
// Name: CWaveDisplay::Update
// Desc: Copies amplitude data into texture. Optionally generates frequencies.
//--------------------------------------------------------------------------------------
HRESULT CWaveDisplay::Update(float* __restrict pSamples, const BOOL bMonitorTimeDomain )
{
    // This path will generate 256 frequency values
    if( !bMonitorTimeDomain )
    {
        static __vector4 realSamples[nBins/4] = {0};
        static __vector4 imaginarySamples[nBins/4] = {0};
        static float scratch[nBins] = {0};

        memset( realSamples, 0, sizeof( realSamples ) );
        memset( imaginarySamples, 0, sizeof( imaginarySamples ) );
        memset( scratch, 0, sizeof( scratch ) );

        memcpy_s((void* __restrict)realSamples, sizeof(realSamples), (void* __restrict)pSamples, min(m_nSamples, nBins) * sizeof(FLOAT));

        //Run the result through an FFT to analyze the frequency response.
        XDSP::FFT(realSamples, imaginarySamples, m_UnityTable, nBins);
        //Convert to polar form
        XDSP::FFTPolar(realSamples, realSamples, imaginarySamples, nBins);

        // The FFT produces samples out of order; get them back into order
        // of increasing frequency
        XDSP::FFTUnswizzle((XDSP::XVECTOR*)scratch, realSamples, nBinsLog2);

        // convert to a more useful display format: throw away the
        // "negative" samples and change to logarithmic.
        //
        int nBands = 64;
        int bandWidth = nSamples / nBands;

		float exponent = log( (float)( nBins / 2 ) ) / log( 2.0f );
        exponent /= (float)nBands;
        for( int j = 0; j < nBands; ++j )
        {
            int low = (int)floor( pow( 2.0f, exponent * (float)j ) );
            int high = (int)ceil( pow( 2.0f, exponent * (float)(j+1) ) );
            float energy = 0;
            float binwidth = 0;
            for( int bin = low; ( bin < high ) && ( bin < (nBins/2) ); ++bin )
            {
                energy += scratch[bin];
                binwidth += 1.0f;
            }

            const float biasScale = 4.0f / binwidth;
            for( int x = 0; x < bandWidth; ++x )
            {
                ((float*)pSamples)[j*bandWidth + x] = energy * biasScale;
            }

        }
    }

	// Note: we double buffer the samples so that we can collect data and render
	//       it on separate threads. To make this work efficiently requires a
	//       bit more work than this, but at this point would require extensive
	//       changes. So for now, we do it inefficiently and move data around.
	EnterCriticalSection(&m_cs);
	memcpy((void* __restrict) m_pSampleBuffer, (void* __restrict )pSamples, m_nSamples * sizeof(FLOAT));
	LeaveCriticalSection(&m_cs);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: CWaveDisplay::Render
// Desc: Does actual drawing of amplitude texture.
//--------------------------------------------------------------------------------------
HRESULT CWaveDisplay::Render( ::IDirect3DDevice9* pDevice, const D3DRECT& pBounds )
{
    HRESULT hr = S_OK;

    D3DVIEWPORT9 vpOld;
    hr = pDevice->GetViewport( &vpOld );

	D3DLOCKED_RECT rect;

	hr = m_pAmplitudeTexture->LockRect( 0, &rect, NULL, 0 );
	if( SUCCEEDED( hr ) )
	{
		EnterCriticalSection(&m_cs);
		memcpy( rect.pBits, m_pSampleBuffer, m_nSamples * sizeof( float ) );
		LeaveCriticalSection(&m_cs);
		hr = m_pAmplitudeTexture->UnlockRect(0);
	}

    if( SUCCEEDED( hr ) )
    {
        D3DVIEWPORT9 vpNew = { 0, 0, 0, 0, 0.0f, 1.0f };
        vpNew.X = pBounds.x1;
        vpNew.Y = pBounds.y1;
        vpNew.Width = pBounds.x2 - pBounds.x1;
        vpNew.Height = pBounds.y2 - pBounds.y1;
        hr = pDevice->SetViewport( &vpNew );
    }

    m_pEffect->BeginTechniqueFromIndex( 0, 0 );
    m_pEffect->BeginPassFromIndex( 0 );

    // Set the wave data texture
    m_pEffect->SetSampler( m_hAmplitudeSampler, m_pAmplitudeTexture );
    m_pEffect->Commit();

    hr = FAILED(hr) ? hr : pDevice->SetVertexDeclaration( NULL );
    hr = FAILED(hr) ? hr : pDevice->SetStreamSource( 0, NULL, 0, 0 );
    hr = FAILED(hr) ? hr : pDevice->DrawTessellatedPrimitive( D3DTPT_QUADPATCH, 0, m_nSamples );

    // Unset the wave data texture so that we can put new data into it
    m_pEffect->SetSampler( m_hAmplitudeSampler, NULL );
    m_pEffect->Commit();

    m_pEffect->EndPass();
    m_pEffect->EndTechnique();

    hr = FAILED(hr) ? hr : pDevice->SetViewport( &vpOld );

    return hr;
}

//--------------------------------------------------------------------------------------
// Name: CWaveDisplay::SetRange
// Desc: Set min/max range of FXL
//--------------------------------------------------------------------------------------
void CWaveDisplay::SetRange( float min, float max )
{
    m_pEffect->SetFloat( m_hMin, min );
    m_pEffect->SetFloat( m_hMax, max );
}

//--------------------------------------------------------------------------------------
// Name: CMonitorAPO::CMonitorAPO
// Desc: Constructor
//--------------------------------------------------------------------------------------
CMonitorAPO::CMonitorAPO()
: CSampleXAPOBase<CMonitorAPO, MonitorAPOParams>()
{
}

//--------------------------------------------------------------------------------------
// Name: CMonitorAPO::~CMonitorAPO
// Desc: Destructor
//--------------------------------------------------------------------------------------
CMonitorAPO::~CMonitorAPO()
{
}


//--------------------------------------------------------------------------------------
// Name: CMonitorAPO::DoProcess
// Desc: Process by copying off a portion of the samples to another thread via a LF pipe
//--------------------------------------------------------------------------------------
void CMonitorAPO::DoProcess( const MonitorAPOParams& params, FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels, BOOL bIsEnabled )
{
    if (!bIsEnabled)
        return;

    if( cFrames )
    {
        MonitorAPOPipe* pipe = params.pipe;
        if( pipe )
            pipe->Write( pData, cFrames * cChannels * (WaveFormat().wBitsPerSample >> 3) );
    }
}

} // Namespace ATG
