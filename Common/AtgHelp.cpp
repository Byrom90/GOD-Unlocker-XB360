//--------------------------------------------------------------------------------------
// AtgHelp.cpp
//
// Support class for rendering a help image of a gamepad with labelled callouts to the
// gamepad controls.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgDevice.h"
#include "AtgFont.h"
#include "AtgHelp.h"
#include "AtgResource.h"
#include "AtgUtil.h"

namespace ATG
{

// Global access to the main D3D device
extern D3DDevice* g_pd3dDevice;


//--------------------------------------------------------------------------------------
// Constants for rendering callouts on the help screen. The order of these callouts is
// in agreement with the enum structure in AtgHelp.h
//--------------------------------------------------------------------------------------
struct HELP_CALLOUT_POS
{
    XMFLOAT2 Button;

    struct
    {
        XMFLOAT2 Line;
        XMFLOAT2 Text;
    }
        Placement[2];

    DWORD dwFontFlags;
};

static struct
{
    FLOAT fButtonX, fButtonY;
    FLOAT fLine1X,  fLine1Y, fText1X, fText1Y;
    FLOAT fLine2X,  fLine2Y, fText2X, fText2Y;
    DWORD dwFontFlags;
} g_vHelpCallouts[] =
{
    // Left thumbstick
    { 511.0f, 271.0f,
        428.0f, 285.0f,  426.0f, 275.0f,
        428.0f, 285.0f,  426.0f, 275.0f,
        ATGFONT_RIGHT },

    // Right thumbstick 
    { 701.0f, 351.0f,
        701.0f, 447.0f,  711.0f, 447.0f,
        701.0f, 447.0f,  711.0f, 447.0f,
        ATGFONT_RIGHT },

    // D-pad
    { 577.0f, 353.0f,
        521.0f, 381.0f,  521.0f, 381.0f,
        521.0f, 381.0f,  521.0f, 381.0f,
        ATGFONT_RIGHT },

    // Back button
    { 595.0f, 276.0f,
        595.0f, 122.0f,  605.0f,  97.0f,
        595.0f, 122.0f,  605.0f,  72.0f,
        ATGFONT_RIGHT },

    // Start button
    { 686.0f, 273.0f,
        686.0f, 122.0f,  676.0f,  97.0f,
        686.0f, 122.0f,  676.0f,  72.0f,
        ATGFONT_LEFT },

    // X button
    { 732.0f, 272.0f,
        759.0f, 447.0f,  759.0f, 447.0f,
        759.0f, 447.0f,  759.0f, 447.0f,
        ATGFONT_LEFT },

    // Y button
    { 763.0f, 235.0f,
        843.0f, 235.0f,  845.0f, 222.0f,
        843.0f, 235.0f,  845.0f, 222.0f,
        ATGFONT_LEFT },

    // A button
    { 765.0f, 308.0f,
        800.0f, 372.0f,  800.0f, 372.0f,
        800.0f, 372.0f,  800.0f, 372.0f,
        ATGFONT_LEFT },

    // B button
    { 796.0f, 271.0f,
        824.0f, 297.0f,  824.0f, 297.0f,
        824.0f, 297.0f,  824.0f, 297.0f,
        ATGFONT_LEFT },

    // Left shoulder button
    { 509.0f, 199.0f,
        462.0f, 117.0f,  460.0f, 107.0f,
        462.0f, 117.0f,  460.0f, 107.0f,
        ATGFONT_RIGHT },

    // Right shoulder button
    { 763.0f, 193.0f,
        815.0f, 117.0f,  817.0f, 107.0f,
        815.0f, 117.0f,  817.0f, 107.0f,
        ATGFONT_LEFT },

    // Misc callout: bottom left
    {   0.0f,   0.0f,
        0.0f,   0.0f,  128.0f, 618.0f,
        0.0f,   0.0f,  128.0f, 588.0f,
        ATGFONT_LEFT },

    // Misc callout: bottom center
    {   0.0f,   0.0f,
        0.0f,   0.0f,  640.0f, 618.0f,
        0.0f,   0.0f,  640.0f, 588.0f,
        ATGFONT_CENTER_X },

    // Misc callout: bottom right
    {   0.0f,   0.0f,
        0.0f,   0.0f, 1152.0f, 618.0f,
        0.0f,   0.0f, 1152.0f, 588.0f,
        ATGFONT_RIGHT },

    // Left Trigger
    { 465.0f, 230.0f,
        412.0f, 177.0f,  410.0f, 167.0f,
        412.0f, 177.0f,  410.0f, 167.0f,
        ATGFONT_RIGHT },

    // Right Trigger
    { 813.0f, 230.0f,
        865.0f, 177.0f,  867.0f, 167.0f,
        865.0f, 177.0f,  867.0f, 167.0f,
        ATGFONT_LEFT },
};

static HELP_CALLOUT_POS* g_pHelpCallouts = ( HELP_CALLOUT_POS* )&g_vHelpCallouts[0];


//--------------------------------------------------------------------------------------
// Vertex and pixel shaders for rendering the help screen
//--------------------------------------------------------------------------------------
static const CHAR* g_strHelpShader =
    " struct VS_IN                                          "
    " {                                                     "
    "     float2 Pos            : POSITION;                 "
    "     float2 Tex            : TEXCOORD0;                "
    " };                                                    "
    "                                                       "
    " struct VS_OUT                                         "
    " {                                                     "
    "     float4 Position       : POSITION;                 "
    "     float2 TexCoord0      : TEXCOORD0;                "
    " };                                                    "
    "                                                       "
    " VS_OUT HelpVertexShader( VS_IN In )                   "
    " {                                                     "
    "     VS_OUT Out;                                       "
    "     Out.Position.x  = In.Pos.x;                       "
    "     Out.Position.y  = In.Pos.y;                       "
    "     Out.Position.z  = 0.0;                            "
    "     Out.Position.w  = 1.0;                            "
    "     Out.TexCoord0.x = In.Tex.x;                       "
    "     Out.TexCoord0.y = In.Tex.y;                       "
    "     return Out;                                       "
    " }                                                     "
    "                                                       "
    "sampler Texture : register(s0);                        "
    "                                                       "
    "float4 HelpPixelShader( VS_OUT In ) : COLOR0           "
    "{                                                      "
    "    return tex2D( Texture, In.TexCoord0 );             "
    "}                                                      "
    "                                                       "
    "float4 DarkPixelShader( [unused] VS_OUT In ) : COLOR0  "
    "{                                                      "
    "    return float4( 0.0f, 0.0f, 0.0f, 0.65f);           "
    "}                                                      ";


static D3DVertexDeclaration* g_pHelpVertexDecl = NULL;
static D3DVertexShader* g_pHelpVertexShader = NULL;
static D3DPixelShader* g_pHelpPixelShader = NULL;
static D3DPixelShader* g_pDarkPixelShader = NULL;


//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the help class internal objects
//--------------------------------------------------------------------------------------
HRESULT Help::Create( const CHAR* strResource )
{
    // Create the gamepad resource
    if( FAILED( m_xprResource.Create( strResource ) ) )
        return E_FAIL;

    return Create( &m_xprResource );
}


//--------------------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the help class' internal objects
//--------------------------------------------------------------------------------------
HRESULT Help::Create( const PackedResource* pResource )
{
    // Get access to the gamepad texture
    m_pGamepadTexture = pResource->GetTexture( "GamepadTexture" );

    // Load the texture used for pointing help text at specific buttons
    m_pLineTexture = pResource->GetTexture( "HelpCalloutTexture" );

    // Create vertex declaration
    if( NULL == g_pHelpVertexDecl )
    {
        static const D3DVERTEXELEMENT9 decl[] =
        {
            { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
            { 0, 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END()
        };

        if( FAILED( g_pd3dDevice->CreateVertexDeclaration( decl, &g_pHelpVertexDecl ) ) )
            return E_FAIL;
    }

    // Create vertex shader
    ID3DXBuffer* pShaderCode;
    if( NULL == g_pHelpVertexShader )
    {
        if( FAILED( D3DXCompileShader( g_strHelpShader, strlen( g_strHelpShader ),
                                       NULL, NULL, "HelpVertexShader", "vs.2.0", 0,
                                       &pShaderCode, NULL, NULL ) ) )
            return E_FAIL;

        if( FAILED( g_pd3dDevice->CreateVertexShader( ( DWORD* )pShaderCode->GetBufferPointer(),
                                                      &g_pHelpVertexShader ) ) )
            return E_FAIL;

        pShaderCode->Release();
    }

    // Create pixel shader.
    if( NULL == g_pHelpPixelShader )
    {
        if( FAILED( D3DXCompileShader( g_strHelpShader, strlen( g_strHelpShader ),
                                       NULL, NULL, "HelpPixelShader", "ps.2.0", 0,
                                       &pShaderCode, NULL, NULL ) ) )
            return E_FAIL;

        if( FAILED( g_pd3dDevice->CreatePixelShader( ( DWORD* )pShaderCode->GetBufferPointer(),
                                                     &g_pHelpPixelShader ) ) )
            return E_FAIL;

        pShaderCode->Release();
    }

    if( NULL == g_pDarkPixelShader )
    {
        if( FAILED( D3DXCompileShader( g_strHelpShader, strlen( g_strHelpShader ),
                                       NULL, NULL, "DarkPixelShader", "ps.2.0", 0,
                                       &pShaderCode, NULL, NULL ) ) )
            return E_FAIL;

        if( FAILED( g_pd3dDevice->CreatePixelShader( ( DWORD* )pShaderCode->GetBufferPointer(),
                                                     &g_pDarkPixelShader ) ) )
            return E_FAIL;

        pShaderCode->Release();
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the gamepad help image and its labelled callouts
//--------------------------------------------------------------------------------------
VOID Help::Render( Font* pFont, const HELP_CALLOUT* pTags, DWORD dwNumCallouts )
{
#ifdef ALLOW_CALLOUT_EDITTING
    // Use the shoulder buttons, triggers, and dpad to edit callout positions
    XINPUT_STATE CurrInputState;
    static XINPUT_STATE LastInputState = {0};
    static DWORD dwCurrCallout = 0;

    if( XInputGetState( 0, &CurrInputState ) == ERROR_SUCCESS )
    {
        if( ( 0 != (CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER) ) &&
            ( 0 == (LastInputState.Gamepad.wButtons&XINPUT_GAMEPAD_LEFT_SHOULDER) ) )
            dwCurrCallout = (dwCurrCallout+dwNumCallouts-1) % dwNumCallouts;
        if( ( 0 != (CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER) ) &&
            ( 0 == (LastInputState.Gamepad.wButtons&XINPUT_GAMEPAD_RIGHT_SHOULDER) ) )
            dwCurrCallout = (dwCurrCallout+dwNumCallouts+1) % dwNumCallouts;

        HELP_CALLOUT_POS* pCallout = &g_pHelpCallouts[pTags[dwCurrCallout].wControl];
        XMFLOAT2* pPos1 = &pCallout->Button;
        XMFLOAT2* pPos2 = &pCallout->Placement[pTags[dwCurrCallout].wPlacement].Line;
        XMFLOAT2* pPos3 = &pCallout->Placement[pTags[dwCurrCallout].wPlacement].Text;

        if( CurrInputState.Gamepad.bLeftTrigger )
        {
            pPos1->x -= CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT  ? 1.0f : 0.0f;
            pPos1->x += CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT ? 1.0f : 0.0f;
            pPos1->y -= CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_UP    ? 1.0f : 0.0f;
            pPos1->y += CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN  ? 1.0f : 0.0f;
        }
        else if( CurrInputState.Gamepad.bRightTrigger )
        {
            pPos2->x -= CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT  ? 1.0f : 0.0f;
            pPos2->x += CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT ? 1.0f : 0.0f;
            pPos2->y -= CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_UP    ? 1.0f : 0.0f;
            pPos2->y += CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN  ? 1.0f : 0.0f;
            pPos3->x -= CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_LEFT  ? 1.0f : 0.0f;
            pPos3->x += CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_RIGHT ? 1.0f : 0.0f;
            pPos3->y -= CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_UP    ? 1.0f : 0.0f;
            pPos3->y += CurrInputState.Gamepad.wButtons&XINPUT_GAMEPAD_DPAD_DOWN  ? 1.0f : 0.0f;
        }

        memcpy( &LastInputState, &CurrInputState, sizeof(XINPUT_STATE) );
    }
#endif // ALLOW_CALLOUT_EDITTING

    // Calculate a scale factor based on the video mode
    D3DDISPLAYMODE ModeDesc;
    XGTEXTURE_DESC TextureDesc;
    g_pd3dDevice->GetDisplayMode( 0, &ModeDesc );
    XGGetTextureDesc( m_pGamepadTexture, 0, &TextureDesc );

    FLOAT fScale = ModeDesc.Width / 1280.0f;
    FLOAT h = fScale * 720 * 0.48f;
    FLOAT w = TextureDesc.Width * h / TextureDesc.Height;
    FLOAT x = ( ModeDesc.Width - w ) / 2;
    FLOAT y = ( ModeDesc.Height - h ) / 2;

    D3DRECT rcSaved = pFont->m_rcWindow;
    pFont->SetWindow( 0, 0, ModeDesc.Width, ModeDesc.Height );

    // Setup the gamepad vertices
    struct VERTEX
    {
        FLOAT sx, sy;
        FLOAT tu, tv;
    };

    // Set up state for rendering the gamepad
    g_pd3dDevice->SetVertexDeclaration( g_pHelpVertexDecl );
    g_pd3dDevice->SetVertexShader( g_pHelpVertexShader );
    g_pd3dDevice->SetTexture( 0, m_pGamepadTexture );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    g_pd3dDevice->SetRenderState( D3DRS_ZENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    g_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, FALSE );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    g_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    g_pd3dDevice->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );

    // Draw the scene-darkening quad
    {
        VERTEX GamepadVertices[4] =
        {
            {                0.0f,                 0.0f, 0.0f, 0.0f },
            { 1.0f * ModeDesc.Width,                 0.0f, 1.0f, 0.0f },
            { 1.0f * ModeDesc.Width, 1.0f * ModeDesc.Height, 1.0f, 1.0f },
            {                0.0f, 1.0f * ModeDesc.Height, 0.0f, 1.0f },
        };

        g_pd3dDevice->SetPixelShader( g_pDarkPixelShader );
        g_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, GamepadVertices, sizeof( GamepadVertices[0] ) );
    }

    // Draw the gamepad image
    {
        VERTEX GamepadVertices[4] =
        {
            { x,   y,   0.0f, 0.0f },
            { x + w, y,   1.0f, 0.0f },
            { x + w, y + h, 1.0f, 1.0f },
            { x,   y + h, 0.0f, 1.0f },
        };

        g_pd3dDevice->SetPixelShader( g_pHelpPixelShader );
        g_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, GamepadVertices, sizeof( GamepadVertices[0] ) );
    }

    // Set state to draw the lines
    g_pd3dDevice->SetTexture( 0, m_pLineTexture );
    g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

    for( DWORD i = 0; i < dwNumCallouts; i++ )
    {
        if( pTags[i].wControl >= ARRAYSIZE( g_vHelpCallouts ) )
            continue;

        HELP_CALLOUT_POS* pCallout = &g_pHelpCallouts[pTags[i].wControl];
        XMFLOAT2 line1 = pCallout->Button;
        XMFLOAT2 line2 = pCallout->Placement[pTags[i].wPlacement].Line;
        line1.x = fScale * ( line1.x - 640 ) + ModeDesc.Width / 2;
        line1.y = fScale * ( line1.y - 360 ) + ModeDesc.Height / 2;
        line2.x = fScale * ( line2.x - 640 ) + ModeDesc.Width / 2;
        line2.y = fScale * ( line2.y - 360 ) + ModeDesc.Height / 2;
        XMVECTOR vc = XMVector2Normalize( XMVectorSet( line2.y - line1.y, -line2.x + line1.x, 0, 0 ) );

#ifdef ALLOW_CALLOUT_EDITTING
        if( dwCurrCallout == i )
            vc *= 2;
#endif // ALLOW_CALLOUT_EDITTING

        // Initialize the callout line vertices
        VERTEX LineVertices[4] =
        {
            { ( line1.x - 2 * vc.x ), ( line1.y - 2 * vc.y ), 0.0f, 0.0f },
            { ( line1.x + 2 * vc.x ), ( line1.y + 2 * vc.y ), 1.0f, 0.0f },
            { ( line2.x + 2 * vc.x ), ( line2.y + 2 * vc.y ), 1.0f, 1.0f },
            { ( line2.x - 2 * vc.x ), ( line2.y - 2 * vc.y ), 0.0f, 1.0f },
        };

        g_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, LineVertices, sizeof( LineVertices[0] ) );
    }

    // Turn the viewport back on
    g_pd3dDevice->SetRenderState( D3DRS_VIEWPORTENABLE, TRUE );

    // Prepare font for rendering
    pFont->Begin();
    static FLOAT fFontXScale = 1.0f;
    static FLOAT fFontYScale = 1.0f;
    pFont->SetScaleFactors( fFontXScale * fScale, fFontYScale * fScale );

    // Render the callouts
    for( DWORD i = 0; i < dwNumCallouts; i++ )
    {
        if( pTags[i].wControl >= ARRAYSIZE( g_vHelpCallouts ) )
            continue;

        HELP_CALLOUT_POS* pCallout = &g_pHelpCallouts[pTags[i].wControl];
        XMFLOAT2 pos = pCallout->Placement[pTags[i].wPlacement].Text;
        pos.x = fScale * ( pos.x - 640 ) + ModeDesc.Width / 2;
        pos.y = fScale * ( pos.y - 360 ) + ModeDesc.Height / 2;

        // Draw the callout text
        pFont->DrawText( pos.x, pos.y, 0xffffffff,
                         pTags[i].strText, g_pHelpCallouts[pTags[i].wControl].dwFontFlags );
    }

    // Flush the text drawing
    pFont->SetScaleFactors( 1.0f, 1.0f );
    pFont->End();

    pFont->m_rcWindow = rcSaved;
}


} // namespace ATG
