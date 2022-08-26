//--------------------------------------------------------------------------------------
// AtgNuiMenu.cpp
//
// Microsoft Advanced Technology Group
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "AtgNuiMenu.h"

#include <assert.h>
#include <nuihandles.h>
#include "AtgDebugDraw.h"


namespace ATG
{

const FLOAT SPINNER_RANGE = 2.0f;  // Spinner upper limit value. It is twice the lenght of 
                                   // the spinner object to account for the back and forth motion.


//--------------------------------------------------------------------------------------
// Name: GetSkeletonIndexFromTrackingID()
// Desc: Given a valid trackingID retrieves the corresponding SkeletonIndex. If the 
//       trackingID is not found, returns NUI_IDENTITY_MAX_ENROLLMENT_COUNT.
//--------------------------------------------------------------------------------------
DWORD GetSkeletonIndexFromTrackingID( const NUI_SKELETON_FRAME* pSkeletonFrame, DWORD dwTrackingID )
{
    assert( dwTrackingID != NUI_SKELETON_INVALID_TRACKING_ID );

    for( DWORD dwSkeletonIndex = 0; dwSkeletonIndex < NUI_SKELETON_COUNT; ++ dwSkeletonIndex )
    {
        if( pSkeletonFrame->SkeletonData[ dwSkeletonIndex ].dwTrackingID == dwTrackingID )
        {
            return dwSkeletonIndex;
        }
    }
    
    return NUI_SKELETON_COUNT;
}


//--------------------------------------------------------------------------------------
// Name: IsMenuItemSelectable()
// Desc: Returns TRUE if the specified menu item is of a selectable type and FALSE 
//       otherwise. 
//--------------------------------------------------------------------------------------
BOOL IsMenuItemSelectable( const NUI_MENU_ITEM* pMenuItem )
{
    if( pMenuItem->eItemType == NUI_MENU_ITEM_BUTTON || pMenuItem->eItemType == NUI_MENU_ITEM_CHECK_BOX )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//--------------------------------------------------------------------------------------
// Name: NuiMenu::NuiMenu()
// Desc: Constructs an instance of NuiMenu.
//--------------------------------------------------------------------------------------
NuiMenu::NuiMenu()
:m_bIsDisabled( FALSE ),
 m_dwItemCount( 0 ),
 m_dwColumnCount( 0 ),
 m_dwRowCount( 0 ),
 m_dwHoveredItemID( NUI_MENU_ITEM_ID_NONE ), 
 m_eHoveredItemState( HOVERED_ITEM_STATE_NOT_SELECTED ),
 m_fRightHandCursor( 0.0f, 0.0f ),
 m_pd3dDevice( NULL ),
 m_dwBackBufferWidth( 0 ),
 m_dwBackBufferHeight( 0 ),
 m_pFont( NULL )
{
    ZeroMemory( &m_settings, sizeof( m_settings ) );

    m_settings.dwCursorSizeInPixels      = NUI_MENU_DEFAULT_CROSSHAIR_SIZE;
    m_settings.dwCursorThicknessInPixels = NUI_MENU_DEFAULT_CROSSHAIR_THICKNESS;
    m_settings.dwHoverTime               = NUI_MENU_DEFAULT_HOVER_TIME;

    m_settings.cursorColor               = NUI_MENU_DEFAULT_CURSOR_COLOR;
    m_settings.panelColor                = NUI_MENU_DEFAULT_PANEL_COLOR;
    m_settings.selectableColor           = NUI_MENU_DEFAULT_SELECTABLE_COLOR;
    m_settings.selectedColor             = NUI_MENU_DEFAULT_SELECTED_COLOR;
    m_settings.staticColor               = NUI_MENU_DEFAULT_STATIC_COLOR;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::~NuiMenu()
// Desc: Release any acquired resources. 
//--------------------------------------------------------------------------------------
NuiMenu::~NuiMenu()
{
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::Initialize()
// Desc: Provides this instance of NuiMenu with system resources it needs. Must be 
//       called only once after the menu object has been created.
//
// NOTE: The data pointed to by pd3dDevice and pFont must persist for the life
//       duration of thisobject.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::Initialize( ::D3DDevice* pd3dDevice, DWORD dwBackBufferWidth, DWORD dwBackBufferHeight, ATG::Font* pFont )
{
    assert( pd3dDevice != NULL );
    assert( pFont != NULL );
    
    if( m_pd3dDevice != NULL )
    {
        return E_FAIL; // Object is already initialized.
    }
    
    m_pd3dDevice = pd3dDevice;

    m_dwBackBufferWidth  = dwBackBufferWidth;
    m_dwBackBufferHeight = dwBackBufferHeight;

    m_pFont = pFont;

    // No need to check the return value because NuiHandlesArmsInit always returns S_OK, unless the parameter is NULL.
    NuiHandlesArmsInit( &m_HandlesArms );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::SetMenuLayout()
// Desc: Sets the menu layout to use. 
//--------------------------------------------------------------------------------------
VOID NuiMenu::SetMenuLayout( NUI_MENU_ITEM aMenuItem[], DWORD dwItemCount, DWORD dwColumnCount, DWORD dwRowCount )
{
    m_pMenuItemList     = aMenuItem;
    m_dwItemCount       = dwItemCount;
    m_dwColumnCount     = dwColumnCount;
    m_dwRowCount        = dwRowCount;

    ClearActiveItem();
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::Update()
// Desc: Updates the internal menu states based on the player's right hand movements.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::Update( const NUI_SKELETON_FRAME* pUnfilteredSkeletonFrame, 
                         DWORD dwControllingTrackingID, 
                         const NUI_IMAGE_FRAME* pDepthFrame320x240, 
                         const NUI_IMAGE_FRAME* pDepthFrame80x60 )
{
    // Nothing to update if the whole menu is disabled
    if( m_bIsDisabled )
    {
        return S_OK;
    }

    // Retrieve the skeleton corresponding to the controlling tracking ID
    DWORD dwSkeletonIndex = GetSkeletonIndexFromTrackingID( pUnfilteredSkeletonFrame, dwControllingTrackingID );
    if( dwSkeletonIndex >= NUI_SKELETON_COUNT )
    {
        return E_FAIL;
    }

    // Keep our internal object time in sync with the skeleton frame time
    m_liCurrentTime = pUnfilteredSkeletonFrame->liTimeStamp;

    // Pass the new frame data to NuiHandles and retrieve the right hand position.
    // NuiHandlesArmsUpdate() always return S_OK unless one of the parameters is incorrectly specified.
    NuiHandlesArmsUpdate( &m_HandlesArms, dwSkeletonIndex, pUnfilteredSkeletonFrame, pDepthFrame320x240, NULL, pDepthFrame80x60, NULL );
    m_fRightHandCursor = GetXYForVector( NuiHandlesArmGetScreenSpaceLocation( &m_HandlesArms, NUI_HANDLES_ARMS_HANDEDNESS_RIGHT_ARM ) );

    // Clear the hovered item info if we have stopped hovereing said item since last tick    
    if( m_dwHoveredItemID != NUI_MENU_ITEM_ID_NONE && ! IsCursorOverItem( GetMenuItemFromItemID( m_dwHoveredItemID ) ) )
    {
        m_dwHoveredItemID = NUI_MENU_ITEM_ID_NONE;
    }

    // Find which selectable item the cursor is hovering, if any
    if( m_dwHoveredItemID == NUI_MENU_ITEM_ID_NONE )
    {
        for( DWORD i = 0; i < m_dwItemCount; ++ i )
        {
            if( IsMenuItemSelectable( &m_pMenuItemList[ i ] ) && IsCursorOverItem( &m_pMenuItemList[ i ] ) )
            {
                m_liHoverBeginTime = m_liCurrentTime;
                m_dwHoveredItemID  = m_pMenuItemList[ i ].dwItemID;
                break;
            }
        }
    }

    // Ensure the item being hovered was not disabled or hidden between two ticks
    if( m_dwHoveredItemID != NUI_MENU_ITEM_ID_NONE && 
        ( ! GetMenuItemFromItemID( m_dwHoveredItemID )->bEnabled || 
          GetMenuItemFromItemID( m_dwHoveredItemID )->bHidden ) )
    {
        m_dwHoveredItemID = NUI_MENU_ITEM_ID_NONE;
    }
 
    // Update the selection state for the hovered item
    if( m_dwHoveredItemID == NUI_MENU_ITEM_ID_NONE )
    {
        m_eHoveredItemState = HOVERED_ITEM_STATE_NOT_SELECTED;
    }
    else
    {
        // If the item has been hovered for the required amount of time, update it to the correct state.
        if( m_liCurrentTime.QuadPart - m_liHoverBeginTime.QuadPart >= m_settings.dwHoverTime )
        {
            if( m_eHoveredItemState == HOVERED_ITEM_STATE_NOT_SELECTED )
            {
                m_eHoveredItemState = HOVERED_ITEM_STATE_JUST_SELECTED;
            }
            else if( m_eHoveredItemState == HOVERED_ITEM_STATE_JUST_SELECTED )
            {
                m_eHoveredItemState = HOVERED_ITEM_STATE_SELECTED;
            }
        }
        else
        {
            m_eHoveredItemState = HOVERED_ITEM_STATE_NOT_SELECTED;
        }
    }

    // Update all spinners
    for( DWORD i = 0; i < m_dwItemCount; ++ i )
    {
        if( m_pMenuItemList[ i ].eItemType == NUI_MENU_ITEM_SPINNER && 
            m_pMenuItemList[ i ].bEnabled && ! m_pMenuItemList[ i ].bHidden )
        {
            // SPINNER_RANGE is twice the distance of an actual on screen spinner so that 
            // when rendered on screen moves back and forth 
            m_pMenuItemList[ i ].fValue += NUI_MENU_DEFAULT_SPINNER_SPEED;
            if( m_pMenuItemList[ i ].fValue >= SPINNER_RANGE )
            {
                m_pMenuItemList[ i ].fValue -= SPINNER_RANGE;
            }
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::Render()
// Desc: Renders the menu items and the cursor.
//--------------------------------------------------------------------------------------
VOID NuiMenu::Render() const
{ 
    assert( m_pd3dDevice != NULL );

    // Nothing to render.
    if( m_pMenuItemList == NULL )
    {
        return;
    }

    // Render each item in listed order
    for( DWORD dwItemIndex = 0; dwItemIndex < m_dwItemCount; ++ dwItemIndex )
    {
        // Only render items that aren't hidden
        if( ! m_pMenuItemList[ dwItemIndex ].bHidden )
        {
            switch( m_pMenuItemList[ dwItemIndex ].eItemType )
            {
                // Render a button. Buttons are shown in different colors depending on whether they are disabled, 
                // selectable or selected
                case NUI_MENU_ITEM_BUTTON:
                {
                    D3DCOLOR color = m_settings.staticColor;
                    if( m_pMenuItemList[ dwItemIndex ].dwItemID == m_dwHoveredItemID )
                    {
                        color = m_settings.selectedColor;
                    }
                    else if( m_pMenuItemList[ dwItemIndex ].bEnabled && ! m_bIsDisabled )
                    {
                        color = m_settings.selectableColor;
                    }
                    
                    DrawButton( &m_pMenuItemList[ dwItemIndex ].Location, m_pMenuItemList[ dwItemIndex ].szText, color );
                    break;
                }

                // Render a check box. Check boxes are shown in different colors depending on whether they are disabled, 
                // selectable or selected
                case NUI_MENU_ITEM_CHECK_BOX:
                {
                    D3DCOLOR color = m_settings.staticColor;
                    if( m_pMenuItemList[ dwItemIndex ].dwItemID == m_dwHoveredItemID )
                    {
                        color = m_settings.selectedColor;
                    }
                    else if( m_pMenuItemList[ dwItemIndex ].bEnabled && ! m_bIsDisabled )
                    {
                        color = m_settings.selectableColor;
                    }

    
                    DrawCheckBox( &m_pMenuItemList[ dwItemIndex ].Location, m_pMenuItemList[ dwItemIndex ].szText, color, m_pMenuItemList[ dwItemIndex ].fValue > 0.0f );
                    break;
                }

                case NUI_MENU_ITEM_FRAME:
                    DrawFrame( &m_pMenuItemList[ dwItemIndex ].Location, m_settings.staticColor );
                    break;

                case NUI_MENU_ITEM_PANEL:
                    DrawPanel( &m_pMenuItemList[ dwItemIndex ].Location, m_settings.panelColor );
                    break;

                case NUI_MENU_ITEM_PROGRESS_BAR:
                    DrawProgressBar( &m_pMenuItemList[ dwItemIndex ].Location, m_pMenuItemList[ dwItemIndex ].fValue, m_settings.staticColor );
                    break;

                case NUI_MENU_ITEM_SPINNER:
                    DrawSpinner( &m_pMenuItemList[ dwItemIndex ].Location, m_pMenuItemList[ dwItemIndex ].fValue, m_settings.staticColor );
                    break;

                case NUI_MENU_ITEM_TEXT:
                    DrawText( &m_pMenuItemList[ dwItemIndex ].Location, m_pMenuItemList[ dwItemIndex ].szText, m_settings.staticColor, m_pMenuItemList[ dwItemIndex ].dwValue );
                    break;

                default:
                    assert( false );
                    break;
            }
        }
    }

    // Add an alignment grid on top of the menu, if requested.
    if( m_settings.bDrawGuide )
    {
        DrawAlignmentGrid();
    }

    // Draw the cursor last, unless the whole menu is disabled
    if( ! m_bIsDisabled )
    {
        DrawCursor();
    }

} 


//--------------------------------------------------------------------------------------
// Name: NuiMenu::GetActiveItem()
// Desc: Returns the active button or check box. Return NUI_MENU_ITEM_ID_NONE if no 
//       button or check box is currently selected.
//--------------------------------------------------------------------------------------
DWORD NuiMenu::GetActiveItem() const
{
    // Ensure the selected state is returned only once
    if( m_eHoveredItemState == HOVERED_ITEM_STATE_JUST_SELECTED )
    { 
        assert( m_dwHoveredItemID != NUI_MENU_ITEM_ID_NONE );
        assert( GetMenuItemFromItemID( m_dwHoveredItemID )->bEnabled );

        return m_dwHoveredItemID;
    } 
        
    return NUI_MENU_ITEM_ID_NONE; 
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::ClearActiveItem()
// Desc: Clears the active selection and progress made so far. The selection process will 
//       restart at the next call to Update() 
//--------------------------------------------------------------------------------------
VOID NuiMenu::ClearActiveItem()
{
    m_dwHoveredItemID   = NUI_MENU_ITEM_ID_NONE;
    m_eHoveredItemState = HOVERED_ITEM_STATE_NOT_SELECTED;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::GetMenuSettings()
// Desc: Obtains the current menu settings.
//--------------------------------------------------------------------------------------
VOID NuiMenu::GetMenuSettings( NUI_MENU_SETTINGS* pNuiMenuSettings ) const
{
    assert( pNuiMenuSettings != NULL );

    XMemCpy( pNuiMenuSettings, &m_settings, sizeof( NUI_MENU_SETTINGS ) );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::SetMenuSettings()
// Desc: CHnages the settings for this object.
//--------------------------------------------------------------------------------------
VOID NuiMenu::SetMenuSettings( const NUI_MENU_SETTINGS* pNuiMenuSettings )
{
    assert( pNuiMenuSettings != NULL );

    XMemCpy( &m_settings, pNuiMenuSettings, sizeof( NUI_MENU_SETTINGS ) );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::EnableItem()
// Desc: Enables the specified item. Returns E_FAIL if the item doesn't exist.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::EnableItem( DWORD dwItemID )
{
    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    pItem->bEnabled = TRUE;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DisableItem()
// Desc: Disables the specified item. Disabled items are shown but cannot be selected.
//       Returns E_FAIL if the item doesn't exist.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::DisableItem( DWORD dwItemID )
{
    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    pItem->bEnabled = FALSE;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::ShowItem()
// Desc: The specified item will be made visible. Returns E_FAIL if the item doesn't 
//       exist.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::ShowItem( DWORD dwItemID )
{
    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    pItem->bHidden = FALSE;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::HideItem()
// Desc: The specified item will be hidden. Hidden items are also inactive.
//       Returns E_FAIL if the item doesn't exist.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::HideItem( DWORD dwItemID )
{
    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    pItem->bHidden = TRUE;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::SetItemText()
// Desc: Sets the text of a specific item.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::SetItemText( DWORD dwItemID, LPCWSTR wszText )
{
    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    pItem->szText = wszText;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::SetItemSelectedState()
// Desc: Sets the selectable state of a selectable item.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::SetItemSelectedState( DWORD dwItemID, BOOL bSelected )
{
    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    if( ! IsMenuItemSelectable( pItem ) )
    {
        return E_FAIL;
    }

    pItem->bValue = bSelected;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::SetItemProgress()
// Desc: Sets the progress for a progress bar item.
//--------------------------------------------------------------------------------------
HRESULT NuiMenu::SetItemProgress( DWORD dwItemID, FLOAT fProgress )
{
    assert( fProgress >= 0.0f && fProgress <= 1.0f );

    NUI_MENU_ITEM* pItem = GetMenuItemFromItemID( dwItemID );
    if( pItem == NULL )
    {
        return E_FAIL;
    }

    if( pItem->eItemType != NUI_MENU_ITEM_PROGRESS_BAR )
    {
        return E_FAIL;
    }

    pItem->fValue = fProgress;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawAlignmentGrid()
// Desc: Draws a grid to help align menu items on screen
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawAlignmentGrid() const
{
    assert( m_dwColumnCount > 0 );
    assert( m_dwRowCount > 0 );


    D3DCOLOR GridColor     = 0xffff0000;
    FLOAT    GridThickness = 1.0f;

    for( DWORD dwX = 0; dwX < m_dwColumnCount; ++ dwX )
    {
        ATG::DebugDraw::DrawScreenSpaceLine( ConvertPointToScreenSpace( dwX, 0 ), 
                                             ConvertPointToScreenSpace( dwX, m_dwRowCount ),
                                             GridColor, GridThickness );
    }

    for( DWORD dwY = 0; dwY < m_dwRowCount; ++ dwY )
    {
        ATG::DebugDraw::DrawScreenSpaceLine( ConvertPointToScreenSpace( 0,               dwY ), 
                                             ConvertPointToScreenSpace( m_dwColumnCount, dwY ),
                                             GridColor, GridThickness );
    }

    ATG::DebugDraw::DrawScreenSpaceLine( XMFLOAT2( m_dwBackBufferWidth - 1.0f, 0.0f ), 
                                         XMFLOAT2( m_dwBackBufferWidth - 1.0f, m_dwBackBufferHeight - 1.0f ),
                                         GridColor, GridThickness );

    ATG::DebugDraw::DrawScreenSpaceLine( XMFLOAT2( 0.0f,                       m_dwBackBufferHeight - 1.0f ), 
                                         XMFLOAT2( m_dwBackBufferWidth - 1.0f, m_dwBackBufferHeight - 1.0f ),
                                         GridColor, GridThickness );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawButton()
// Desc: Draws a button of the specified color on screen. The string specified by 
//       wszText is centered inside the button.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawButton( const D3DRECT* pLocation, LPCWSTR wszText, D3DCOLOR color ) const
{
    DrawFrame( pLocation, color );
    DrawText( pLocation, wszText, color, ATGFONT_CENTER_X );
}

   
//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawCheckBox()
// Desc: Draws a check box of the specified color on screen. The string specified by 
//       wszText will be displayed to the right of the check box. bChecked indicates 
//       whether the box should be ffilled or empty.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawCheckBox( const D3DRECT* pLocation, LPCWSTR wszText, D3DCOLOR color, BOOL bChecked ) const
{
    D3DRECT CheckBoxLocationInPixel;
    ConvertRectToScreenSpace( pLocation, &CheckBoxLocationInPixel );
    DWORD dwHeight = CheckBoxLocationInPixel.y2 - CheckBoxLocationInPixel.y1;

    D3DRECT TextLocationInPixel = CheckBoxLocationInPixel;

    DWORD dwCheckBoxSize = ( DWORD )( dwHeight * 0.5f );
    CheckBoxLocationInPixel.y1 += ( LONG )( ( dwHeight - dwCheckBoxSize ) * 0.5f );
    CheckBoxLocationInPixel.y2 = CheckBoxLocationInPixel.y1 + dwCheckBoxSize;
    CheckBoxLocationInPixel.x2 = CheckBoxLocationInPixel.x1 + dwCheckBoxSize;

    ATG::DebugDraw::DrawScreenSpaceRect( CheckBoxLocationInPixel, bChecked ? 0.0f : ( FLOAT )m_settings.dwCursorThicknessInPixels, color );

    TextLocationInPixel.x1 += ( LONG )( dwCheckBoxSize * 1.5f );

    m_pFont->Begin();

    D3DRECT rc;
    m_pFont->GetWindow( rc );
    m_pFont->SetWindow( TextLocationInPixel );

    FLOAT fXScaleFactor = m_pFont->m_fXScaleFactor;
    FLOAT fYScaleFactor = m_pFont->m_fYScaleFactor;
    FLOAT fScale = dwHeight * 0.5f / m_pFont->GetFontHeight();
    m_pFont->SetScaleFactors( fScale, fScale );

    m_pFont->DrawText( 0.0f, dwHeight * 0.5f, color, wszText, ATGFONT_LEFT | ATGFONT_CENTER_Y );            

    m_pFont->SetScaleFactors( fXScaleFactor, fYScaleFactor );
    m_pFont->SetWindow( rc );
    m_pFont->End();
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawCursor()
// Desc: Draw a crosshair cursor. The cursor will have a small progress bar displayed 
//       underneath, if it is hovering a selectable item.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawCursor() const
{
    FLOAT fCrosshairHalfSize = m_settings.dwCursorSizeInPixels / 2.0f;

    // Draw progress bar if the cursor is hovering an item
    if( m_dwHoveredItemID != NUI_MENU_ITEM_ID_NONE )
    {
        INT iHoverTime = INT( m_liCurrentTime.QuadPart - m_liHoverBeginTime.QuadPart );
        FLOAT fRatio = iHoverTime / ( FLOAT )m_settings.dwHoverTime;    
        if( fRatio <= 1.0f )
        {
            ATG::DebugDraw::DrawScreenSpaceRect( XMFLOAT2( m_fRightHandCursor.x - fCrosshairHalfSize, m_fRightHandCursor.y + fCrosshairHalfSize + m_settings.dwCursorSizeInPixels / 10 ), 
                                                 XMFLOAT2( ( FLOAT )m_settings.dwCursorSizeInPixels, m_settings.dwCursorSizeInPixels * 0.2f ), 1, m_settings.cursorColor );
        
            ATG::DebugDraw::DrawScreenSpaceRect( XMFLOAT2( m_fRightHandCursor.x - fCrosshairHalfSize, m_fRightHandCursor.y + fCrosshairHalfSize + m_settings.dwCursorSizeInPixels / 10 ), 
                                                 XMFLOAT2( ( FLOAT )m_settings.dwCursorSizeInPixels * fRatio, m_settings.dwCursorSizeInPixels * 0.2f ), 0, m_settings.cursorColor );
        }
    }

    // Draw the cursor.
    ATG::DebugDraw::DrawScreenSpaceLine( XMFLOAT2( m_fRightHandCursor.x - fCrosshairHalfSize, m_fRightHandCursor.y ), 
                                         XMFLOAT2( m_fRightHandCursor.x + fCrosshairHalfSize, m_fRightHandCursor.y ), 
                                         m_settings.cursorColor, ( FLOAT )m_settings.dwCursorThicknessInPixels );

    ATG::DebugDraw::DrawScreenSpaceLine( XMFLOAT2( m_fRightHandCursor.x, m_fRightHandCursor.y - fCrosshairHalfSize ), 
                                         XMFLOAT2( m_fRightHandCursor.x, m_fRightHandCursor.y + fCrosshairHalfSize ), 
                                         m_settings.cursorColor, ( FLOAT )m_settings.dwCursorThicknessInPixels );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawFrame()
// Desc: Draws a frame of the specified color on screen.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawFrame( const D3DRECT* pLocation, D3DCOLOR color ) const
{
    D3DRECT LocationInPixel;
    ConvertRectToScreenSpace( pLocation, &LocationInPixel );

    ATG::DebugDraw::DrawScreenSpaceRect( LocationInPixel, ( FLOAT )m_settings.dwCursorThicknessInPixels, color );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawPanel()
// Desc: Draws a filled box of the specified color on screen.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawPanel( const D3DRECT* pLocation, D3DCOLOR color ) const
{
    D3DRECT LocationInPixel;
    ConvertRectToScreenSpace( pLocation, &LocationInPixel );

    ATG::DebugDraw::DrawScreenSpaceRect( LocationInPixel, 0, color );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawProgressBar()
// Desc: Draws a progress bar of the specified color on screen. fProgress must be between 
//       0.0f and 1.0f
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawProgressBar( const D3DRECT* pLocation, FLOAT fProgress, D3DCOLOR color ) const
{
    assert( fProgress >= 0.0f && fProgress <= 1.0f );

    D3DRECT LocationInPixel;
    ConvertRectToScreenSpace( pLocation, &LocationInPixel );

    ATG::DebugDraw::DrawScreenSpaceRect( LocationInPixel, ( FLOAT )m_settings.dwCursorThicknessInPixels, color );

    DWORD dwRange =  LocationInPixel.x2 - LocationInPixel.x1;

    D3DRECT BarLocationInPixel;
    BarLocationInPixel.x1 = LocationInPixel.x1;
    BarLocationInPixel.y1 = LocationInPixel.y1;
    BarLocationInPixel.x2 = LocationInPixel.x1 + ( LONG )( dwRange * fProgress );
    BarLocationInPixel.y2 = LocationInPixel.y2;
    ATG::DebugDraw::DrawScreenSpaceRect( BarLocationInPixel, 0, color );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawSpinner()
// Desc: Draws a spinner item on screen.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawSpinner( const D3DRECT* pLocation, FLOAT fProgress, D3DCOLOR color ) const
{
    assert( fProgress >= 0.0f && fProgress <= SPINNER_RANGE );

    D3DRECT LocationInPixel;
    ConvertRectToScreenSpace( pLocation, &LocationInPixel );

    ATG::DebugDraw::DrawScreenSpaceRect( LocationInPixel, ( FLOAT )m_settings.dwCursorThicknessInPixels, color );

    DWORD dwBoxSize = LocationInPixel.y2 - LocationInPixel.y1;
    DWORD dwRange =  LocationInPixel.x2 - LocationInPixel.x1 - dwBoxSize;

    if( fProgress > 1.0f )
    {
        fProgress = fabs( fProgress - SPINNER_RANGE );
    }

    D3DRECT SpinnerLocationInPixel;
    SpinnerLocationInPixel.x1 = ( LONG )( LocationInPixel.x1 + dwRange * fProgress );
    SpinnerLocationInPixel.y1 = LocationInPixel.y1;
    SpinnerLocationInPixel.x2 = SpinnerLocationInPixel.x1 + dwBoxSize;
    SpinnerLocationInPixel.y2 = LocationInPixel.y2;
    ATG::DebugDraw::DrawScreenSpaceRect( SpinnerLocationInPixel, 0, color );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::DrawText()
// Desc: Draws a text item on screen. The text can be left aligned or centered 
//       horizontally.
//--------------------------------------------------------------------------------------
VOID NuiMenu::DrawText( const D3DRECT* pLocation, LPCWSTR wszText, D3DCOLOR color, DWORD dwHorizontalAlignment  ) const
{
    D3DRECT LocationInPixel;
    ConvertRectToScreenSpace( pLocation, &LocationInPixel );

    DWORD dwWidth  = LocationInPixel.x2 - LocationInPixel.x1;
    DWORD dwHeight = LocationInPixel.y2 - LocationInPixel.y1;

    m_pFont->Begin();

    D3DRECT rc;
    m_pFont->GetWindow( rc );
    m_pFont->SetWindow( LocationInPixel );

    FLOAT fXScaleFactor = m_pFont->m_fXScaleFactor;
    FLOAT fYScaleFactor = m_pFont->m_fYScaleFactor;
    FLOAT fScale = dwHeight * 0.5f / m_pFont->GetFontHeight();
    m_pFont->SetScaleFactors( fScale, fScale );

   
    if( dwHorizontalAlignment & ATGFONT_CENTER_X )
    {
        m_pFont->DrawText( dwWidth * 0.5f, dwHeight * 0.5f, color, wszText, ATGFONT_CENTER_X | ATGFONT_CENTER_Y );            
    }
    else
    {
        m_pFont->DrawText( 0.0f, dwHeight * 0.5f, color, wszText, ATGFONT_LEFT | ATGFONT_CENTER_Y );            
    }

    m_pFont->SetScaleFactors( fXScaleFactor, fYScaleFactor );
    m_pFont->SetWindow( rc );
    m_pFont->End();    
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::GetMenuItemFromItemID()
// Desc: Given a valid item ID, returns the corresponding item for the item list. It 
//       returns NULL if the item doesn't exist.
//--------------------------------------------------------------------------------------
const NUI_MENU_ITEM* NuiMenu::GetMenuItemFromItemID( DWORD dwItemID ) const
{
    assert( dwItemID != NUI_MENU_ITEM_ID_NONE );
    assert( dwItemID != NUI_MENU_ITEM_ID_UNDEFINED );

    for( DWORD dwItemIndex = 0; dwItemIndex < m_dwItemCount; ++ dwItemIndex )
    {
        if( m_pMenuItemList[ dwItemIndex ].dwItemID == dwItemID )
        {
            return &m_pMenuItemList[ dwItemIndex ];
        }
    }

    assert( false );
    return NULL;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::GetMenuItemFromItemID()
// Desc: constless version of GetMenuItemFromItemID()
//--------------------------------------------------------------------------------------
NUI_MENU_ITEM* NuiMenu::GetMenuItemFromItemID( DWORD dwItemID )
{
    return const_cast< NUI_MENU_ITEM* >( const_cast< const NuiMenu* >( this )->GetMenuItemFromItemID( dwItemID ) );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::IsCursorOverItem()
// Desc: Returns TRUE if the cursor is on top of the specified item.
//--------------------------------------------------------------------------------------
BOOL NuiMenu::IsCursorOverItem( const NUI_MENU_ITEM* pMenuItem ) const
{
    D3DRECT LocationInScreenSpace;
    ConvertRectToScreenSpace( &pMenuItem->Location, &LocationInScreenSpace );

    if ( m_fRightHandCursor.x < LocationInScreenSpace.x1 || m_fRightHandCursor.x > LocationInScreenSpace.x2 ||
         m_fRightHandCursor.y < LocationInScreenSpace.y1 || m_fRightHandCursor.y > LocationInScreenSpace.y2    )
        {
            return FALSE;
        }

    return TRUE;
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::ConvertRectToScreenSpace()
// Desc: Converts a rectangle from menu units to the actual on screen position.
//--------------------------------------------------------------------------------------
VOID NuiMenu::ConvertRectToScreenSpace( const D3DRECT* pLocation, D3DRECT* pLocationInScreenSpace ) const
{
    assert( pLocation != NULL );
    assert( pLocationInScreenSpace != NULL );

    assert( m_dwColumnCount > 0 );
    assert( m_dwRowCount > 0 );

    pLocationInScreenSpace->x1 = ( DWORD )( pLocation->x1 / ( FLOAT )m_dwColumnCount  * m_dwBackBufferWidth  );
    pLocationInScreenSpace->y1 = ( DWORD )( pLocation->y1 / ( FLOAT )m_dwRowCount     * m_dwBackBufferHeight );
    pLocationInScreenSpace->x2 = ( DWORD )( pLocation->x2 / ( FLOAT )m_dwColumnCount  * m_dwBackBufferWidth  );
    pLocationInScreenSpace->y2 = ( DWORD )( pLocation->y2 / ( FLOAT )m_dwRowCount     * m_dwBackBufferHeight );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::ConvertPointToScreenSpace()
// Desc: Converts a point from menu units to the actual on screen position.
//--------------------------------------------------------------------------------------
XMFLOAT2 NuiMenu::ConvertPointToScreenSpace( DWORD dwX, DWORD dwY ) const
{
    assert( m_dwColumnCount > 0 );
    assert( m_dwRowCount > 0 );

    return XMFLOAT2( dwX / ( FLOAT )m_dwColumnCount  * m_dwBackBufferWidth, dwY / ( FLOAT )m_dwRowCount * m_dwBackBufferHeight );
}


//--------------------------------------------------------------------------------------
// Name: NuiMenu::GetXYForVector()
// Desc: Map normalized space coordinates to screen space.
//--------------------------------------------------------------------------------------
XMFLOAT2 NuiMenu::GetXYForVector( FXMVECTOR vNormalizedSpace ) const
{
    assert( m_dwBackBufferWidth  > 0 );
    assert( m_dwBackBufferHeight > 0 );

    XMFLOAT2 rt;
    rt.x = XMVectorGetX( vNormalizedSpace ) * m_dwBackBufferWidth + m_dwBackBufferWidth;           
    rt.y = m_dwBackBufferHeight - ( XMVectorGetY( vNormalizedSpace ) * m_dwBackBufferHeight / 2 + m_dwBackBufferHeight / 2 );

    return rt;
}


}; // namespace ATG