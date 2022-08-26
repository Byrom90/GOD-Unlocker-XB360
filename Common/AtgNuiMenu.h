//--------------------------------------------------------------------------------------
// AtgNuiMenu.h
//
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#ifndef ATG_NUI_MENU_H
#define ATG_NUI_MENU_H


#include <NuiApi.h>
#include <nuihandles.h>
#include "AtgFont.h"


//--------------------------------------------------------------------------------------
// Name: NUI_MENU_ITEM macros
// Desc: Series of helper macros that ensure proper initialization of NUI_MENU_ITEM 
//       structures.
//--------------------------------------------------------------------------------------
#define ATG_NUI_MENU_ITEM_BUTTON( X, Y, Width, Height, dwItemID, szText )    { dwItemID, ATG::NUI_MENU_ITEM_BUTTON,       TRUE, FALSE, { X, Y, X + Width, Y + Height }, szText, 0 }
#define ATG_NUI_MENU_ITEM_CHECK_BOX( X, Y, Width, Height, dwItemID, szText ) { dwItemID, ATG::NUI_MENU_ITEM_CHECK_BOX,    TRUE, FALSE, { X, Y, X + Width, Y + Height }, szText, 0 }
#define ATG_NUI_MENU_ITEM_FRAME( X, Y, Width, Height, dwItemID )             { dwItemID, ATG::NUI_MENU_ITEM_FRAME,        TRUE, FALSE, { X, Y, X + Width, Y + Height }, NULL,   0 }
#define ATG_NUI_MENU_ITEM_PANEL( X, Y, Width, Height, dwItemID )             { dwItemID, ATG::NUI_MENU_ITEM_PANEL,        TRUE, FALSE, { X, Y, X + Width, Y + Height }, NULL,   0 }
#define ATG_NUI_MENU_ITEM_PROGRESS_BAR( X, Y, Width, Height, dwItemID )      { dwItemID, ATG::NUI_MENU_ITEM_PROGRESS_BAR, TRUE, FALSE, { X, Y, X + Width, Y + Height }, NULL,   0 }
#define ATG_NUI_MENU_ITEM_SPINNER( X, Y, Width, Height, dwItemID )           { dwItemID, ATG::NUI_MENU_ITEM_SPINNER,      TRUE, FALSE, { X, Y, X + Width, Y + Height }, NULL,   0 }
#define ATG_NUI_MENU_ITEM_TEXT( X, Y, Width, Height, dwItemID, szText )      { dwItemID, ATG::NUI_MENU_ITEM_TEXT,         TRUE, FALSE, { X, Y, X + Width, Y + Height }, szText, ATGFONT_CENTER_X }
#define ATG_NUI_MENU_ITEM_TEXT_LEFT( X, Y, Width, Height, dwItemID, szText ) { dwItemID, ATG::NUI_MENU_ITEM_TEXT,         TRUE, FALSE, { X, Y, X + Width, Y + Height }, szText, ATGFONT_LEFT }


namespace ATG
{

// Cursor sizes
const DWORD NUI_MENU_DEFAULT_CROSSHAIR_SIZE         = 40;     // in pixels
const DWORD NUI_MENU_DEFAULT_CROSSHAIR_THICKNESS    =  3;     // in pixels

// Time in milliseconds the cursor must hover over a button in order to select it.
// 1.2 seconds seem a reasonable time, being neither to quick or too slow...
const DWORD NUI_MENU_DEFAULT_HOVER_TIME = 750;  

// Default spinner speed
const FLOAT NUI_MENU_DEFAULT_SPINNER_SPEED = 0.06f;

// Default color used for drawing a menu
const D3DCOLOR NUI_MENU_DEFAULT_CURSOR_COLOR      = 0xffffffff;
const D3DCOLOR NUI_MENU_DEFAULT_PANEL_COLOR       = 0xff000000;
const D3DCOLOR NUI_MENU_DEFAULT_SELECTABLE_COLOR  = 0xff006600;
const D3DCOLOR NUI_MENU_DEFAULT_SELECTED_COLOR    = 0xff00ff00;
const D3DCOLOR NUI_MENU_DEFAULT_STATIC_COLOR      = 0xffffffff;


// Pre-defined menu item values.
const DWORD NUI_MENU_ITEM_ID_NONE       = 0xffffffff;
const DWORD NUI_MENU_ITEM_ID_UNDEFINED  = 0xfffffffe;


//--------------------------------------------------------------------------------------
// Name: enum HOVERED_ITEM_STATE
// Desc: Specifies the state of an item being hovered by the cursor. 
//--------------------------------------------------------------------------------------
enum HOVERED_ITEM_STATE
{
    HOVERED_ITEM_STATE_NOT_SELECTED = 0, // The cursor maybe hovering the item but the time threshold hasn't been met.
    HOVERED_ITEM_STATE_JUST_SELECTED,    // The selection time threshold was just pass during the present menu update. 
    HOVERED_ITEM_STATE_SELECTED,         // The selection is still selected. No new notification will be sent.
};


//--------------------------------------------------------------------------------------
// Name: struct NUI_MENU_SETTINGS 
// Desc: Defines menu wide configurable settings
//--------------------------------------------------------------------------------------
struct NUI_MENU_SETTINGS
{
    DWORD dwCursorSizeInPixels;      // Width and height of the cursor in pixels.
    DWORD dwCursorThicknessInPixels; // Thickness of the lines defining the cursor, in pixels.
    DWORD dwHoverTime;               // Time, in millisecond, that the cursor must hover a item before it is selected.

    D3DCOLOR cursorColor;     // Color used to draw the curor
    D3DCOLOR panelColor;      // Color used to draw opaque panel items
    D3DCOLOR selectableColor; // Color used to display selectable buttons
    D3DCOLOR selectedColor;   // Color used to display buttons when the cursor is on top
    D3DCOLOR staticColor;     // Color used for non-selectable displaying menu items

    BOOL bDrawGuide; // If TRUE, a helper alignment grid will be drawn on top of the menu items.
};


//--------------------------------------------------------------------------------------
// Name: enum NUI_MENU_ITEM_TYPE 
// Desc: Supported menu item types
//--------------------------------------------------------------------------------------
enum NUI_MENU_ITEM_TYPE 
{ 
    NUI_MENU_ITEM_BUTTON = 0,   // A button is a selectable item
    NUI_MENU_ITEM_CHECK_BOX,    // Check boxes are selectable square with text on the right.
    NUI_MENU_ITEM_FRAME,        // A frame is a rectangle outline and is a static item
    NUI_MENU_ITEM_PANEL,        // A Panel is a filled rectangle, static item
    NUI_MENU_ITEM_PROGRESS_BAR, // Static item that graphically indicates some measure of progress 
    NUI_MENU_ITEM_SPINNER,      // A squre moving right to left to right indicating the system is 
                                // perforing an operation
    NUI_MENU_ITEM_TEXT ,        // A static text item that can either be centered or left aligned

    NUI_MENU_ITEM_COUNT         // Maximum number of items supported by NuiMenu
};


//--------------------------------------------------------------------------------------
// Name: struct NUI_MENU_ITEM
// Desc: Holds all data pertaining to a specific item.
//--------------------------------------------------------------------------------------
struct NUI_MENU_ITEM
{
    DWORD               dwItemID;  // ID to uniquely identify this item.
    NUI_MENU_ITEM_TYPE  eItemType; // Type of item.
    BOOL                bEnabled;  // Item is enabled if this is set to TRUE
    BOOL                bHidden;   // Item is hidden if this is set to TRUE
    D3DRECT             Location;  // On-screen corrdinates 
    LPCWSTR             szText;    // Point to the item text.
    
    union
    {
        // Extra data specific to different item types.
        // The dwValue must be the first item of the union, for initialization macros to work correctly.
        DWORD dwValue;
        BOOL  bValue;
        FLOAT fValue;
    };
};


//--------------------------------------------------------------------------------------
// Name: class NuiMenu
// Desc: Handles selecting menu items, rendering, etc.
//--------------------------------------------------------------------------------------
class NuiMenu
{
public:
    NuiMenu();
	~NuiMenu();

    HRESULT Initialize( ::D3DDevice* pd3dDevice, DWORD dwBackBufferWidth, DWORD dwBackBufferHeight, ATG::Font* pFont );

    VOID SetMenuLayout( NUI_MENU_ITEM aMenuItem[], DWORD dwItemCount, DWORD dwColumnCount, DWORD dwRowCount );

    HRESULT Update( const NUI_SKELETON_FRAME* pUnfilteredSkeletonFrame, 
                    DWORD dwControllingTrackingID, 
                    const NUI_IMAGE_FRAME* pDepthFrame320x240, 
                    const NUI_IMAGE_FRAME* pDepthFrame80x60 );

    VOID Render() const;

    DWORD GetActiveItem() const;
    VOID ClearActiveItem();

    VOID GetMenuSettings( NUI_MENU_SETTINGS* pNuiMenuSettings ) const ;
    VOID SetMenuSettings( const NUI_MENU_SETTINGS* pNuiMenuSettings );

    VOID DisableMenu() { if( ! m_bIsDisabled ){ m_bIsDisabled = TRUE; ClearActiveItem(); } };
    VOID EnableMenu() { if( m_bIsDisabled ){ m_bIsDisabled = FALSE; NuiHandlesArmsInit( &m_HandlesArms ); } };

    HRESULT EnableItem( DWORD dwItemID );
    HRESULT DisableItem( DWORD dwItemID );

    HRESULT ShowItem( DWORD dwItemID );
    HRESULT HideItem( DWORD dwItemID );

    HRESULT SetItemText( DWORD dwItemID, LPCWSTR wszText );
    HRESULT SetItemSelectedState( DWORD dwItemID, BOOL bSelected );
    HRESULT SetItemProgress( DWORD dwItemID, FLOAT fProgress );


private:
    NuiMenu( const NuiMenu& rhs );
    NuiMenu& operator =( const NuiMenu& rhs );

    VOID DrawAlignmentGrid() const;
    VOID DrawButton( const D3DRECT* pLocation, LPCWSTR wszText, D3DCOLOR color ) const;
    VOID DrawCheckBox( const D3DRECT* pLocation, LPCWSTR wszText, D3DCOLOR color, BOOL bChecked ) const;
    VOID DrawCursor() const;
    VOID DrawFrame( const D3DRECT* pLocation, D3DCOLOR color ) const;
    VOID DrawPanel( const D3DRECT* pLocation, D3DCOLOR color ) const;
    VOID DrawProgressBar( const D3DRECT* pLocation, FLOAT fProgress, D3DCOLOR color ) const;
    VOID DrawSpinner( const D3DRECT* pLocation, FLOAT fProgress, D3DCOLOR color ) const;
    VOID DrawText( const D3DRECT* pLocation, LPCWSTR wszText, D3DCOLOR color, DWORD dwHorizontalAlignment ) const;

    const NUI_MENU_ITEM* GetMenuItemFromItemID( DWORD dwItemID ) const;
    NUI_MENU_ITEM* GetMenuItemFromItemID( DWORD dwItemID );

    BOOL IsCursorOverItem( const NUI_MENU_ITEM* pMenuItem ) const;
    
    VOID ConvertRectToScreenSpace( const D3DRECT* pLocation, D3DRECT* pLocationInScreenSpace ) const;
    XMFLOAT2 ConvertPointToScreenSpace( DWORD dwX, DWORD dwY ) const;
    XMFLOAT2 GetXYForVector( FXMVECTOR vNormalizedSpace ) const;

    // Menu wide configuration information
    BOOL               m_bIsDisabled; // The whole menu is disabled when set to TRUE. 
    NUI_MENU_SETTINGS  m_settings;    // Settings currently in use.

    // Menu information as defined by the calling application.
    NUI_MENU_ITEM*     m_pMenuItemList; // Pointer to a static list of menu item held by the calling application.
    DWORD              m_dwItemCount;   // Number of menu items in the preceding item list
    DWORD              m_dwColumnCount; // Number of horizontal menu units
    DWORD              m_dwRowCount;    // Number of vertical menu units 

    // Data related to the item curently being hovered, if it is a selectable item.
    DWORD              m_dwHoveredItemID;   // The item ID of the item being hovered or NUI_MENU_ITEM_ID_NONE
    LARGE_INTEGER      m_liHoverBeginTime;  // Time at which the cursor moved onto the item
    HOVERED_ITEM_STATE m_eHoveredItemState; // Selection state of the hovered item.

    LARGE_INTEGER      m_liCurrentTime; // Current time. Updated from the SkeletonFrame each time the Update function is called.

    // Cursor tracking info
    NUI_HANDLES_ARMS   m_HandlesArms;      // Nui handle data structure
    XMFLOAT2           m_fRightHandCursor; // Right hand ( and cursor ) location

    // System resources used by MenuManager.
    ::D3DDevice*       m_pd3dDevice;
    DWORD              m_dwBackBufferWidth;
    DWORD              m_dwBackBufferHeight;
    ATG::Font*         m_pFont;
};


}; // namespace ATG


#endif // ATG_NUI_MENU_H