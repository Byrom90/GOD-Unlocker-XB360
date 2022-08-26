//--------------------------------------------------------------------------------------
// AtgHelp.h
//
// Support class for rendering a help image of a gamepad with labelled callouts to the
// gamepad controls.
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGHELP_H
#define ATGHELP_H

#include "AtgFont.h"
#include "AtgResource.h"

namespace ATG
{


//--------------------------------------------------------------------------------------
// Name: struct ATGHELP_CALLOUT
// Desc: Structure for callout information, used to label controls when rendering an
//       image of an Xbox gamepad. An app will define an array of of these, one for
//       each game pad control used.
//--------------------------------------------------------------------------------------
struct HELP_CALLOUT
{
    WORD wControl;    // An index to identify a control, as enum'ed below
    WORD wPlacement;  // An offset to pick from one of the possible placements
    WCHAR* strText;     // Text to draw when rendering this callout
};


//--------------------------------------------------------------------------------------
// Name: class Help
// Desc: Class for rendering a help image of a gamepad with labelled callouts.
//--------------------------------------------------------------------------------------
class Help
{
    PackedResource m_xprResource;
    D3DTexture* m_pGamepadTexture;
    D3DTexture* m_pLineTexture;

public:
            Help()
            {
            }
            ~Help()
            {
            }

    // Functions to create and destroy the internal objects
    HRESULT Create( const CHAR* pResource );
    HRESULT Create( const PackedResource* pResource );

    // Renders the help screen (using a caller-supplied font)
    VOID    Render( Font* pFont, const HELP_CALLOUT* pTags, DWORD dwNumCallouts );
};


//--------------------------------------------------------------------------------------
// Constants used to identify callout positions
//--------------------------------------------------------------------------------------
enum
{
    HELP_LEFT_STICK,
    HELP_RIGHT_STICK,
    HELP_DPAD,
    HELP_BACK_BUTTON,
    HELP_START_BUTTON,
    HELP_X_BUTTON,
    HELP_Y_BUTTON,
    HELP_A_BUTTON,
    HELP_B_BUTTON,
    HELP_LEFT_SHOULDER,
    HELP_RIGHT_SHOULDER,
    HELP_BOTTOM_LEFT,
    HELP_BOTTOM_CENTER,
    HELP_BOTTOM_RIGHT,
    HELP_LEFT_TRIGGER,
    HELP_RIGHT_TRIGGER,

    // Temporary mappings until the samples are ported over to the new names
    HELP_WHITE_BUTTON   = HELP_LEFT_SHOULDER,
    HELP_BLACK_BUTTON   = HELP_RIGHT_SHOULDER,
    HELP_LEFT_BUTTON    = HELP_BOTTOM_LEFT,
    HELP_RIGHT_BUTTON   = HELP_BOTTOM_RIGHT,
    HELP_LEFTSTICK      = HELP_LEFT_STICK,
    HELP_RIGHTSTICK     = HELP_RIGHT_STICK,
    HELP_MISC_CALLOUT   = HELP_BOTTOM_LEFT,
    HELP_MISC_CALLOUT_2 = HELP_BOTTOM_CENTER,
    HELP_MISC_CALLOUT_3 = HELP_BOTTOM_RIGHT,
};


//--------------------------------------------------------------------------------------
// Placement options for each callout, used as an offset into the above list
//--------------------------------------------------------------------------------------
enum
{
    HELP_PLACEMENT_1    = 0,   // Callout has one line of text
    HELP_PLACEMENT_2    = 1    // Callout has two lines of text
};

} // namespace ATG

#endif // ATGHELP_H
