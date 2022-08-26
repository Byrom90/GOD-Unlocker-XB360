
// ********************************************************************************************************* //
// --------------------------------------------------------------------------------------------------------- //
//
// Copyright (c) Microsoft Corporation
//
// --------------------------------------------------------------------------------------------------------- //
// ********************************************************************************************************* //

#include "stdafx.h"
#include "AtgNuiHandRefinement.h"
#include <math.h>

// ********************************************************************************************************* //
//
//  Namespace:
//

namespace ATG
{

//--------------------------------------------------------------------------------------
// Name: struct SpiralSearchLocation
// Desc: An array of these makes up the spiraling outward search pattern that is used
//  to build a mask around the hand
//--------------------------------------------------------------------------------------
struct SpiralSearchLocation
{
    INT2Vector m_iSearchSpot;
    INT m_iSearchDirection;
    INT m_iOffsetIn80x60;
    INT m_iOffsetIn12x12;
};

// The 50 or so entries in this mask are precalculated to result in exact offsets 
// for the spiral search.  So the offset calculation is now a lookup and an addition.
SpiralSearchLocation g_UnrolledNeighborSearchSpiral[] = 
{
    // ring 1 
    INT2Vector(-1, 0), 4, 0, 0,
    INT2Vector(-1, -1), 5, 0, 0, 
    INT2Vector(0, -1), 6, 0, 0, 
    INT2Vector(1, -1), 7, 0, 0, 
    INT2Vector(1, 0), 0, 0, 0, 
    INT2Vector(1, 1), 1, 0, 0, 
    INT2Vector(0, 1), 2, 0, 0,
    INT2Vector(-1, 1), 3, 0, 0,

    //ring 2
    INT2Vector(-2, 0), 4, 0, 0,
    INT2Vector(-2, -1), 4, 0, 0, 
    INT2Vector(-2, -2), 5, 0, 0,
    INT2Vector(-1, -2), 6, 0, 0,
    INT2Vector(0, -2), 6, 0, 0, 
    INT2Vector(1, -2), 6, 0, 0, 
    INT2Vector(2, -2), 7, 0, 0, 
    INT2Vector(2, -1), 0, 0, 0, 
    INT2Vector(2, 0), 0, 0, 0, 
    INT2Vector(2, 1), 0, 0, 0, 
    INT2Vector(2, 2), 1, 0, 0, 
    INT2Vector(1, 2), 2, 0, 0, 
    INT2Vector(0, 2), 2, 0, 0,
    INT2Vector(-1, 2), 2, 0, 0,
    INT2Vector(-2, 2), 3, 0, 0,
    INT2Vector(-2, 1), 4, 0, 0,

    //ring 3
    INT2Vector(-3, 0), 4, 0, 0,
    INT2Vector(-3, -1), 4, 0, 0,
    INT2Vector(-3, -2), 4, 0, 0,
    INT2Vector(-3, -3), 5, 0, 0,
    INT2Vector(-2, -3), 6, 0, 0,
    INT2Vector(-1, -3), 6, 0, 0,
    INT2Vector(0, -3), 6, 0, 0,
    INT2Vector(1, -3), 6, 0, 0,
    INT2Vector(2, -3), 6, 0, 0,
    INT2Vector(3, -3), 7, 0, 0,
    INT2Vector(3, -2), 0, 0, 0,
    INT2Vector(3, -1), 0, 0, 0,
    INT2Vector(3, 0), 0, 0, 0,
    INT2Vector(3, 1), 0, 0, 0,
    INT2Vector(3, 2), 0, 0, 0,
    INT2Vector(3, 3), 1, 0, 0,
    INT2Vector(2, 3), 2, 0, 0,
    INT2Vector(1, 3), 2, 0, 0,
    INT2Vector(0, 3), 2, 0, 0,
    INT2Vector(-1, 3), 2, 0, 0,
    INT2Vector(-2, 3), 2, 0, 0,
    INT2Vector(-3, 3), 3, 0, 0,
    INT2Vector(-3, 2), 4, 0, 0,
    INT2Vector(-3, 1), 4, 0, 0,

    //ring 4
    INT2Vector(-4, 0), 4, 0, 0,
    INT2Vector(-4, -1), 4, 0, 0,
    INT2Vector(-4, -2), 4, 0, 0,
    INT2Vector(-4, -3), 4, 0, 0,
    INT2Vector(-4, -4), 5, 0, 0,
    INT2Vector(-3, -4), 6, 0, 0,
    INT2Vector(-2, -4), 6, 0, 0,
    INT2Vector(-1, -4), 6, 0, 0,
    INT2Vector(0, -4), 6, 0, 0,
    INT2Vector(1, -4), 6, 0, 0,
    INT2Vector(2, -4), 6, 0, 0,
    INT2Vector(3, -4), 6, 0, 0,
    INT2Vector(4, -4), 7, 0, 0,
    INT2Vector(4, -3), 0, 0, 0,
    INT2Vector(4, -2), 0, 0, 0,
    INT2Vector(4, -1), 0, 0, 0,
    INT2Vector(4, 0), 0, 0, 0,
    INT2Vector(4, 1), 0, 0, 0,
    INT2Vector(4, 2), 0, 0, 0,
    INT2Vector(4, 3), 0, 0, 0,
    INT2Vector(4, 4), 1, 0, 0,
    INT2Vector(3, 4), 2, 0, 0,
    INT2Vector(2, 4), 2, 0, 0,
    INT2Vector(1, 4), 2, 0, 0,
    INT2Vector(0, 4), 2, 0, 0,
    INT2Vector(-1, 4), 2, 0, 0,
    INT2Vector(-2, 4), 2, 0, 0,
    INT2Vector(-3, 4), 2, 0, 0,
    INT2Vector(-4, 4), 3, 0, 0,
    INT2Vector(-4, 3), 4, 0, 0,
    INT2Vector(-4, 2), 4, 0, 0,
    INT2Vector(-4, 1), 4, 0, 0,

    //ring5
    INT2Vector(-5, 0), 4, 0, 0,
    INT2Vector(-5, -1), 4, 0, 0,
    INT2Vector(-5, -2), 4, 0, 0,
    INT2Vector(-5, -3), 4, 0, 0,
    INT2Vector(-5, -4), 4, 0, 0,
    INT2Vector(-5, -5), 5, 0, 0,
    INT2Vector(-4, -5), 6, 0, 0,
    INT2Vector(-3, -5), 6, 0, 0,
    INT2Vector(-2, -5), 6, 0, 0,
    INT2Vector(-1, -5), 6, 0, 0,
    INT2Vector(0, -5), 6, 0, 0,
    INT2Vector(1, -5), 6, 0, 0,
    INT2Vector(2, -5), 6, 0, 0,
    INT2Vector(3, -5), 6, 0, 0,
    INT2Vector(4, -5), 6, 0, 0,
    INT2Vector(5, -5), 7, 0, 0,
    INT2Vector(5, -4), 0, 0, 0,
    INT2Vector(5, -3), 0, 0, 0,
    INT2Vector(5, -2), 0, 0, 0,
    INT2Vector(5, -1), 0, 0, 0,
    INT2Vector(5, 0), 0, 0, 0,
    INT2Vector(5, 1), 0, 0, 0,
    INT2Vector(5, 2), 0, 0, 0,
    INT2Vector(5, 3), 0, 0, 0,
    INT2Vector(5, 4), 0, 0, 0,
    INT2Vector(5, 5), 1, 0, 0,
    INT2Vector(4, 5), 2, 0, 0,
    INT2Vector(3, 5), 2, 0, 0,
    INT2Vector(2, 5), 2, 0, 0,
    INT2Vector(1, 5), 2, 0, 0,
    INT2Vector(0, 5), 2, 0, 0,
    INT2Vector(-1, 5), 2, 0, 0,
    INT2Vector(-2, 5), 2, 0, 0,
    INT2Vector(-3, 5), 2, 0, 0,
    INT2Vector(-4, 5), 2, 0, 0,
    INT2Vector(-5, 5), 3, 0, 0,
    INT2Vector(-5, 4), 4, 0, 0,
    INT2Vector(-5, 3), 4, 0, 0,
    INT2Vector(-5, 2), 4, 0, 0,
    INT2Vector(-5, 1), 4, 0, 0

};

// The search diretions used to walk along the arm.  The vector between the elbow and 
// the hand can be wrong. It is mostly correct.  These directions are used to search the 8
// cardinal directions and diagonals that correspond to a give elbow hand direction.

enum SearchDirections
{
    SEARCHDIRECTION_FORWARD = 0,
    SEARCHDIRECTION_DIAGONAL_LEFT,
    SEARCHDIRECTION_DIAGONAL_RIGHT,
    SEARCHDIRECTION_ORTHOGONAL_LEFT,
    SEARCHDIRECTION_ORTHOGONAL_RIGHT,
    SEARCHDIRECTION_COUNT
};

// When walking to the end of the hand. we're looking for small changes as we're trying to stay on the arm.
// If the change is greater than 4 centimeters or 3 centimeters (depending on fall back code)
// we don't want to follow that direction.
static CONST INT g_iWalktoEndofHandThesholds[] = { 40, 30 };

// We start at the beginning of this list of thresholds and continue 
// building masks until we build a valid threshold.
static CONST INT g_iBuildMaskThresholds[] = {130, 80, 50, 30};

// This bias is added to the difference between the neighbor and the current pixel to
// bias towards the directions that are more aligned to the elbow hand vector.
static CONST INT g_iBiasFor5SearchDirections[] = {0, 40, 40, 80, 80};

// We test the hand against the depth map. If the hand is much closer to the camera than the map
// then we know that we have a compound fracture.
static CONST INT g_iDistanceOffDepthMaptoWarrantLargeSearch = 100;

// This smaller threshold tells us the bone is only poking out of the arm a little.
static CONST INT g_iDistanceOffDepthMaptoWarrantSmallSearch = 20;

// Number of iterations to walk along the arm in the 80x60 map.
static CONST INT g_nWalkToEdgeIterations = 12; 

// These two thresholds are used to build a 3 division histogram for segmenting the hand away from the body
static CONST INT g_iCenterFor320x240CloseThreshold = 50;
static CONST INT g_iCenterFor320x240FarThreshold = 150;

// - in y is up
// The search directions for the 8 surrounding pixels
static CONST INT2Vector g_SpiralSearchOffsets[] =
{
    INT2Vector(-1, 0),
    INT2Vector(-1, -1),
    INT2Vector(0, -1),
    INT2Vector(1, -1),
    INT2Vector(1, 0),
    INT2Vector(1, 1),
    INT2Vector(0, 1),
    INT2Vector(-1, 1)
};

// Do it branchless on PPC
// These are branchless instructions to avoid branch mispredictions
// Determine the difference between them.
// diff = (a<b) ? (a-b) : 0;
// a-(a-b) = b; // Flip if (a<b)
// b+(a-b) = a; // Flip if (a>b)
#define GETMIN(dst,a,b) diff = a-b; mask = diff>>31; diff = diff&mask; dst = b+diff    
#define GETMAX(dst,a,b) diff = a-b; mask = diff>>31; diff = diff&mask; dst = a-diff  

// This abs function is much faster but assumes I'm only comparing shorts.
__forceinline INT INTABSFAST( INT iIn )
{
    INT i = (iIn ^ (iIn >> 31)) - (iIn >> 31);
    return i;
}
static CONST INT g_iMaxImageDepth = NUI_IMAGE_DEPTH_MAXIMUM >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

//--------------------------------------------------------------------------------------
// Simplifies passing the depth maps.
//--------------------------------------------------------------------------------------
struct DepthMaps
{
    USHORT* m_pData320x240;
    INT m_iShortPitch320x240;
    USHORT* m_pData80x60Min;
    INT m_iShortPitch80x60Min;
};

// These offsets represent 4 groups of xy coordinates
// This makes indexing memory a lookup and an addition.
// InitUnrolledNeighborSearchSpiral fills in these values because they depend upon pitch.
INT g_SpiralSearchOffsets80x60[] = {0,0,0,0,0,0,0,0};
INT g_SpiralSearchOffsets12x12[] = {0,0,0,0,0,0,0,0};

//--------------------------------------------------------------------------------------
// Name: InitUnrolledNeighborSearchSpiral()
// Desc: Fills in the offsets for the search structure. Now an offset can be calculated 
// for any location in the search spiral with a simple addition.
//--------------------------------------------------------------------------------------
VOID InitUnrolledNeighborSearchSpiral( INT iShortPitch80x60Min )
{
    // This is not thread safe
    if ( g_UnrolledNeighborSearchSpiral[0].m_iOffsetIn12x12 != 0 ) return;
        
    INT iSearchIterations = ARRAYSIZE( g_UnrolledNeighborSearchSpiral );
    g_UnrolledNeighborSearchSpiral[0].m_iOffsetIn80x60 = g_UnrolledNeighborSearchSpiral[0].m_iSearchSpot.iX + 
        (g_UnrolledNeighborSearchSpiral[0].m_iSearchSpot.iY ) * iShortPitch80x60Min;
    g_UnrolledNeighborSearchSpiral[0].m_iOffsetIn12x12 = g_UnrolledNeighborSearchSpiral[0].m_iSearchSpot.iX + 
        (g_UnrolledNeighborSearchSpiral[0].m_iSearchSpot.iY ) * g_iNuiRefinementKernelSize80x60;

    for ( INT iIndex = 1; iIndex < iSearchIterations; ++iIndex )
    {
        SpiralSearchLocation* pPrevious = &g_UnrolledNeighborSearchSpiral[iIndex-1];
        SpiralSearchLocation* pCurrent = &g_UnrolledNeighborSearchSpiral[iIndex];
        pCurrent->m_iOffsetIn80x60 = 
            pCurrent->m_iSearchSpot.iX - pPrevious->m_iSearchSpot.iX + 
            ( pCurrent->m_iSearchSpot.iY -  pPrevious->m_iSearchSpot.iY ) * iShortPitch80x60Min;
        pCurrent->m_iOffsetIn12x12 = 
            pCurrent->m_iSearchSpot.iX - pPrevious->m_iSearchSpot.iX + 
            ( pCurrent->m_iSearchSpot.iY -  pPrevious->m_iSearchSpot.iY ) * g_iNuiRefinementKernelSize80x60;
    }
    g_SpiralSearchOffsets80x60[0] = -1;
    g_SpiralSearchOffsets80x60[1] = -1 - iShortPitch80x60Min;
    g_SpiralSearchOffsets80x60[2] = -iShortPitch80x60Min;
    g_SpiralSearchOffsets80x60[3] = 1 - iShortPitch80x60Min;
    g_SpiralSearchOffsets80x60[4] = 1;
    g_SpiralSearchOffsets80x60[5] = 1 + iShortPitch80x60Min;
    g_SpiralSearchOffsets80x60[6] = iShortPitch80x60Min;
    g_SpiralSearchOffsets80x60[7] = -1 + iShortPitch80x60Min;
    g_SpiralSearchOffsets12x12[0] = -1;
    g_SpiralSearchOffsets12x12[1] = -1 - g_iNuiRefinementKernelSize80x60;
    g_SpiralSearchOffsets12x12[2] = -g_iNuiRefinementKernelSize80x60;
    g_SpiralSearchOffsets12x12[3] = 1 - g_iNuiRefinementKernelSize80x60;
    g_SpiralSearchOffsets12x12[4] = 1;
    g_SpiralSearchOffsets12x12[5] = 1 + g_iNuiRefinementKernelSize80x60;
    g_SpiralSearchOffsets12x12[6] = g_iNuiRefinementKernelSize80x60;
    g_SpiralSearchOffsets12x12[7] = -1 + g_iNuiRefinementKernelSize80x60;
    
}

//--------------------------------------------------------------------------------------
// Name: Clamp80x60()
// Desc: Clamps values to be valid lookup valus in a 80x60 map
//--------------------------------------------------------------------------------------
__forceinline void Clamp80x60( INT* pX, INT* pY )
{
    INT diff, mask;
    INT iMin;
    GETMIN( iMin, 79, *pX );
    GETMAX( *pX, iMin, 0 ); 
    GETMIN( iMin, 59, *pY );
    GETMAX( *pY, iMin, 0 ); 
}

//--------------------------------------------------------------------------------------
// Name: Clamp80x60()
// Desc: Clamps values to be valid lookup valus in a 320x240 map
//--------------------------------------------------------------------------------------
__forceinline void Clamp320x240( INT* pX, INT* pY )
{
    INT diff, mask;
    INT iMin;
    GETMIN( iMin, 319, *pX );
    GETMAX( *pX, iMin, 0 ); 
    GETMIN( iMin, 239, *pY );
    GETMAX( *pY, iMin, 0 ); 
}

//-----------------------------------------------------------------------------
// Name: TransformWorldToScreen320x240
// Desc:Transforms from world coordinates to camera coordinates.
//
// - w: The world coordinates (in milimeters)
// - screenY: The x coordinate in camera space
// - screenY: The y coordinate in camera space
//-----------------------------------------------------------------------------
__forceinline void TransformWorldToScreen320x240(
                                   _In_ XMVECTOR w, 
                                   _In_ FLOAT FOV_X_DEGREES, 
                                   _Out_ INT* pScreenX, 
                                   _Out_ INT* pScreenY)
{
    static FLOAT TAN_FOV_PER_PIXEL_INV = ( (320.0f)/2.0f ) / tan( XMConvertToRadians( NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV * 0.5f ) );
    *pScreenX = (INT)( ( w.x * TAN_FOV_PER_PIXEL_INV ) /w.z + (320.0f)/2.0f );
    *pScreenY = (INT)( (240.0f)/2.0f - ( w.y * TAN_FOV_PER_PIXEL_INV ) /w.z );
}

//-----------------------------------------------------------------------------
// Name: TransformWorldToScreen80x60
// Desc:Transforms from world coordinates to camera coordinates.
//
// - w: The world coordinates (in milimeters)
// - screenY: The x coordinate in camera space
// - screenY: The y coordinate in camera space
//-----------------------------------------------------------------------------
__forceinline void TransformWorldToScreen80x60(
                                   _In_ XMVECTOR w, 
                                   _Out_ INT3Vector* pCoord )
{
    static FLOAT TAN_FOV_PER_PIXEL_INV = ( (80.0f)/2.0f ) / 
        tan( XMConvertToRadians( NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV * 0.5f ) );
    pCoord->iX = (INT)( ( w.x * TAN_FOV_PER_PIXEL_INV ) /w.z + (80.0f)/2.0f );
    pCoord->iY = (INT)( 60.0f / 2.0f - ( w.y * TAN_FOV_PER_PIXEL_INV ) /w.z );
    pCoord->iZ = (INT)( XMVectorGetZ( w ) * 1000.0f);
}

//-----------------------------------------------------------------------------
// Name: TransformScreenToWorld320x240
// Desc: Transforms from camera to world coordinates. The world coordinates are 
// represented in milimeters.
//
// - screenX: The x coordinate
// - screenY: The y coordinate
// - screenZ: The depth
// - pWX:     The x position of the point in world space
// - pWY:     The y position of the point in world space
// - pWZ:     The z position of the point in world space
//-----------------------------------------------------------------------------
__forceinline void TransformScreenToWorld320x240(
                                   _In_ FLOAT screenX, 
                                   _In_ FLOAT screenY, 
                                   _In_ FLOAT screenZ, 
                                   _Out_ XMVECTOR* pW)
{
    static const FLOAT TAN_FOV_PER_PIXEL = 
        tan( XMConvertToRadians( NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV * 0.5f ) ) / ( ((FLOAT)320)/2.0f);

    pW->x = ( ( screenX - ( 320.0f ) / 2.0f ) * screenZ * TAN_FOV_PER_PIXEL );
    pW->y = ( ( ( 240.0f ) / 2.0f - screenY ) * screenZ * TAN_FOV_PER_PIXEL );
    pW->z = (FLOAT)screenZ;
    pW->w = 1.0f;
}

//-----------------------------------------------------------------------------
// Name: TransformScreenToWorld80x60
// Desc: Transforms from camera to world coordinates. The world coordinates are 
// represented in milimeters.
//
// - screenX: The x coordinate
// - screenY: The y coordinate
// - screenZ: The depth
// - pWX:     The x position of the point in world space
// - pWY:     The y position of the point in world space
// - pWZ:     The z position of the point in world space
//-----------------------------------------------------------------------------
__forceinline void TransformScreenToWorld80x60(
                                   _In_ FLOAT screenX, 
                                   _In_ FLOAT screenY, 
                                   _In_ FLOAT screenZ, 
                                   _Out_ XMVECTOR* pW )
{
    static const FLOAT TAN_FOV_PER_PIXEL = 
        tan( XMConvertToRadians( NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV * 0.5f ) ) / ( ((FLOAT)80)/2.0f);

    pW->x = ( ( screenX - ( 80.0f ) / 2.0f ) * screenZ * TAN_FOV_PER_PIXEL );
    pW->y = ( ( (60.0f)/2.0f - (FLOAT)screenY ) * screenZ * TAN_FOV_PER_PIXEL );
    pW->z = screenZ;
    pW->w = 1.0f;
}

//--------------------------------------------------------------------------------------
// Name: FindAverageForHand320x240
// Desc: This function uses the location in the 80x60 map to calculate the average from 
// the 320x240 depth map.
//--------------------------------------------------------------------------------------
INT FindAverageForHand320x240 ( DepthMaps* __restrict pDMS,
                               RefinementData* __restrict pRefinementData,
                               HandSpecificData* __restrict pHandSpecificData,
                               XMVECTOR* __restrict pRefinedHand,
                               OptionalRefinementData* __restrict pOptionalRefinementData,
                               OptionalHandSpecificData* __restrict pEHSP
                              )
{
    static CONST INT iBeginOffsetfor12x12ThresholdSearch = -3;
    static CONST INT iEbdOffsetfor12x12ThresholdSearch = 9;

    INT iShortPitch320x240 = pDMS->m_iShortPitch320x240;
    USHORT* pDepth320x240 = pDMS->m_pData320x240;
    INT iShortPitch80x60 = pDMS->m_iShortPitch80x60Min;
    USHORT* pDepth80x60 = pDMS->m_pData80x60Min;

    INT3Vector iRefinedPosition80x60 = pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;

    INT3Vector iSearchLocation320x240 = pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;
    iSearchLocation320x240.iX *= 4;
    iSearchLocation320x240.iY *= 4;
    iSearchLocation320x240.iZ = pDepth80x60[pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX +
        pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY * iShortPitch80x60] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
    memset( &pHandSpecificData->m_VisualizeHandFramesData, 2, sizeof (VisualizeHandFramesData) );
#endif

    INT2Range XRange, YRange;
    XRange.iMin = 0;
    XRange.iMax = g_iNuiRefinementKernelSize80x60;
    YRange.iMin = 0;
    YRange.iMax = g_iNuiRefinementKernelSize80x60;
    const INT iNumSearchPixels = g_iNuiRefinementKernelCenter80x60;

    if ( iRefinedPosition80x60.iX < iNumSearchPixels )
    {
        XRange.iMin = iNumSearchPixels - iRefinedPosition80x60.iX;
    }
    if ( iRefinedPosition80x60.iX > ( 80 + iNumSearchPixels - XRange.iMax ) )
    {
        XRange.iMax = 80 + iNumSearchPixels - iRefinedPosition80x60.iX;
    }
    if ( iRefinedPosition80x60.iY < iNumSearchPixels )
    {
        YRange.iMin = iNumSearchPixels - iRefinedPosition80x60.iY;
    }
    if ( iRefinedPosition80x60.iY > ( 60 + iNumSearchPixels - YRange.iMax ) )
    {
        YRange.iMax = 60 + iNumSearchPixels - iRefinedPosition80x60.iY;
    }

    // Build a 3 level histrogram of the 12 x 12  grid in the 320x240 around the hand
    // This will help position the ellipsoid
    INT2Range iCenterIn320X ( iSearchLocation320x240.iX + iBeginOffsetfor12x12ThresholdSearch,
        iSearchLocation320x240.iX + iEbdOffsetfor12x12ThresholdSearch );
    INT2Range iCenterIn320Y ( iSearchLocation320x240.iY + iBeginOffsetfor12x12ThresholdSearch, 
        iSearchLocation320x240.iY + iEbdOffsetfor12x12ThresholdSearch );

    Clamp320x240( &iCenterIn320X.iMin, &iCenterIn320Y.iMin );
    Clamp320x240( &iCenterIn320X.iMax, &iCenterIn320Y.iMax );

    INT3Vector iCloseThresholdPosition( 0,0,0 );
    INT3Vector iFarThresholdPosition( 0,0,0 );

    INT iLessThanCloseThreshold = 0;
    INT iLessThanFarThreshold = 0;

    PIXBeginNamedEvent( 0, "Find Pivot" );

    for ( INT iY320x240 = iCenterIn320Y.iMin; iY320x240 < iCenterIn320Y.iMax; ++iY320x240 )
    {
        for ( INT iX320x240 = iCenterIn320X.iMin; iX320x240 < iCenterIn320X.iMax; ++iX320x240 )
        {
            USHORT uValue = pDepth320x240[iY320x240 * iShortPitch320x240 + iX320x240];
            USHORT uDepth = uValue >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
            INT iDiff = INTABSFAST ((INT)uDepth - iSearchLocation320x240.iZ );
            if ( iDiff < g_iCenterFor320x240CloseThreshold )
            {
                ++ iLessThanCloseThreshold;
                iCloseThresholdPosition.iX += iX320x240;
                iCloseThresholdPosition.iY += iY320x240;
                iCloseThresholdPosition.iZ += (INT)uDepth;
            }
            if ( iDiff < g_iCenterFor320x240FarThreshold )
            {
                ++ iLessThanFarThreshold;
                iFarThresholdPosition.iX += iX320x240;
                iFarThresholdPosition.iY += iY320x240;
                iFarThresholdPosition.iZ += (INT)uDepth;
            }
        }
    }

    // Determine which of hte thresholds use of the first two segments in our 3 segment histogram.
    if ( iLessThanCloseThreshold != 0 && iLessThanCloseThreshold > g_iNuiRefinementHalfKernelSize320x240 )
    {
        iCloseThresholdPosition.iX /= iLessThanCloseThreshold;
        iCloseThresholdPosition.iY /= iLessThanCloseThreshold;
        iCloseThresholdPosition.iZ /= iLessThanCloseThreshold;
        iSearchLocation320x240 = iCloseThresholdPosition;
        pEHSP->m_SearchSphereCenter320x240 = iSearchLocation320x240;
    }
    else if ( iLessThanFarThreshold != 0 )
    {
        iFarThresholdPosition.iX /= iLessThanFarThreshold;
        iFarThresholdPosition.iY /= iLessThanFarThreshold;
        iFarThresholdPosition.iZ /= iLessThanFarThreshold;
        iSearchLocation320x240 = iFarThresholdPosition;
        pEHSP->m_SearchSphereCenter320x240 = iSearchLocation320x240;
    }
    PIXEndNamedEvent();

// It is important to maintain the slow unoptimized version because it is much easier to understand
#define USE_FAST_HARD_TO_READ_VMX_CODE_TO_COMPUTE_AVERAGE 1
#ifdef USE_FAST_HARD_TO_READ_VMX_CODE_TO_COMPUTE_AVERAGE
    INT iBytePitch320x240 = iShortPitch320x240 * 2;

    SHORT iOffsetCloseToBody = pHandSpecificData->m_iOffsetForHandCloseToBody;

    __declspec(align(16)) SHORT OffsetCloseToBody[] = 
    { 
        iOffsetCloseToBody, iOffsetCloseToBody, iOffsetCloseToBody, iOffsetCloseToBody,
        iOffsetCloseToBody, iOffsetCloseToBody, iOffsetCloseToBody, iOffsetCloseToBody
    };

    __vector4 vmxOffsetCloseToBody = __lvlx( OffsetCloseToBody, 0 );

    static __vector4 vSingleBitPerHalf = __vspltish( 1 );
    static __vector4 v8ChannelShiftRight3 = __vspltish( 3 );
    static __vector4 v8ChannelShiftRight4 = __vspltish( 4 );

    __declspec(align(16)) SHORT v8WayDepth[8];
    v8WayDepth[0] = (SHORT)iSearchLocation320x240.iZ;
    v8WayDepth[1] = v8WayDepth[0];
    v8WayDepth[2] = v8WayDepth[0];
    v8WayDepth[3] = v8WayDepth[0];
    v8WayDepth[4] = v8WayDepth[0];
    v8WayDepth[5] = v8WayDepth[0];
    v8WayDepth[6] = v8WayDepth[0];
    v8WayDepth[7] = v8WayDepth[0];
    __vector4 vmxDepth8 = __lvx( v8WayDepth, 0 );

    __declspec(align(16)) static const BYTE Merge2Rows[] = 
    { 
        0, 1, 2, 3,
        4, 5, 6, 7,
        16, 17, 18, 19,
        20, 21, 22, 23 
    };
    static const __vector4 vmxMergeTwoRows = __lvlx( Merge2Rows, 0 );

    __declspec(align(16)) static const SHORT Sub80x60Offsets[] = 
    { 
        0, 1, 2, 3,
        0, 1, 2, 3
    };
    static const __vector4 vmxSub80x60Offsets = __lvlx( Sub80x60Offsets, 0 );

    __declspec(align(16)) static const BYTE PermuteForIndividualXLocations[] = 
    { 
        0, 1, 2, 3,
        4, 5, 6, 7,
        0, 1, 2, 3,
        4, 5, 6, 7
    };
    static const __vector4 vmxPermuteForIndividualXLocations = __lvlx( PermuteForIndividualXLocations, 0 );

    __declspec(align(16)) static const BYTE PermuteForIndividualYLocationsFirst8[] = 
    { 
        8, 9, 8, 9,
        8, 9, 8, 9,
        10, 11, 10, 11,
        10, 11, 10, 11
    };
    static const __vector4 vmxPermuteForIndividualYLocationsFirst8 = __lvlx( PermuteForIndividualYLocationsFirst8, 0 );

    __declspec(align(16)) static const BYTE PermuteForIndividualYLocationsSecond8[] = 
    { 
        12, 13, 12, 13,
        12, 13, 12, 13,
        14, 15, 14, 15,
        14, 15, 14, 15
    };
    static const __vector4 vmxPermuteForIndividualYLocationsSecond8 = __lvlx( PermuteForIndividualYLocationsSecond8, 0 );

    __declspec(align(16)) CONST SHORT CopiesOfSearchLocationXANDY[] = 
    { 
        -(CONST SHORT)iSearchLocation320x240.iX, -(CONST SHORT)iSearchLocation320x240.iX, 
        -(CONST SHORT)iSearchLocation320x240.iX, -(CONST SHORT)iSearchLocation320x240.iX,
        -(CONST SHORT)iSearchLocation320x240.iY, -(CONST SHORT)iSearchLocation320x240.iY,
        -(CONST SHORT)iSearchLocation320x240.iY, -(CONST SHORT)iSearchLocation320x240.iY
    };
    const __vector4 vmxCopiesOfSearchLocationXANDY = __lvlx( CopiesOfSearchLocationXANDY, 0 );
    
    __declspec(align(16)) CONST SHORT MaxSquaredRadius[] = 
    { 
        (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold, (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold, 
        (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold, (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold,
        (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold, (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold,
        (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold, (CONST SHORT)pEHSP->m_iSquaredSizeOfHandTheshold
    };
    CONST __vector4 vmxMaxSquaredRadius = __lvlx( MaxSquaredRadius, 0 );

    __declspec(align(16)) static CONST BYTE CreateXSquaredPerm[] = 
    { 
        0, 1, 2, 3,
        4, 5, 6, 7,
        0, 1, 2, 3,
        4, 5, 6, 7
    };
    static const __vector4 vmxCreateXSquaredPerm = __lvlx( CreateXSquaredPerm, 0 );

    __declspec(align(16)) static const BYTE CreateYSquaredPermFirst8[] = 
    { 
        8, 9, 8, 9,
        8, 9, 8, 9,
        10, 11, 10, 11,
        10, 11, 10, 11
    };
    static const __vector4 vmxCreateYSquaredPermFirst8 = __lvlx( CreateYSquaredPermFirst8, 0 );

    __declspec(align(16)) static const BYTE ReplicateIteratorFor4x4Population[] = 
    { 
        0, 1, 0, 1,
        0, 1, 0, 1,
        2, 3, 2, 3,
        2, 3, 2, 3
    };
    static const __vector4 vmxReplicateIteratorFor4x4Population = __lvlx( ReplicateIteratorFor4x4Population, 0 );
    
    __declspec(align(16)) static const BYTE CreateYSquaredPermSecond8[] = 
    { 
        12, 13, 12, 13,
        12, 13, 12, 13,
        14, 15, 14, 15,
        14, 15, 14, 15
    };
    static const __vector4 vmxCreateYSquaredPermSecond8 = __lvlx( CreateYSquaredPermSecond8, 0 );

    PIXBeginNamedEvent( 0xffffffff, "Expensive Average" );
        
    __vector4 vmxFinal4WayDepth = __vzero();
    __vector4 vmxFinal4WayX = __vzero();
    __vector4 vmxFinal4WayY = __vzero();
    __vector4 vmxFinalConnt = __vzero();

    static CONST __declspec(align(16)) SHORT IncrementXOuterLoop[] = 
    {
       4,0,0,0,0,0,0,0
    };
    static CONST __vector4 vmxIncrementXOuterLoop = __lvlx( IncrementXOuterLoop, 0 );

    __declspec(align(16))SHORT IncrementYOuterLoop[] = 
    {
       (SHORT)(-4*(XRange.iMax - XRange.iMin )),4,0,0,0,0,0,0
    };
    __vector4 vmxIncrementYOuterLoop = __lvlx( IncrementYOuterLoop, 0 );

    __declspec(align(16)) SHORT InitialLoopValues[] = 
    {
       (SHORT)((XRange.iMin - iNumSearchPixels + iRefinedPosition80x60.iX) * 4),
       (SHORT)((YRange.iMin - iNumSearchPixels + iRefinedPosition80x60.iY) * 4),
       0,0,0,0,0,0
    };
    __vector4 vmxInitialLoopValues = __lvlx( InitialLoopValues, 0 );

    __vector4 vmxXYIterator = vmxInitialLoopValues;
    
    INT iOffset = ((YRange.iMin - iNumSearchPixels + iRefinedPosition80x60.iY) * 4) * iBytePitch320x240 
        + ((XRange.iMin - iNumSearchPixels + iRefinedPosition80x60.iX) * 4) * 2;

    // 8 shorts per row
    // 2 rows per voxel
    __declspec(align(16)) USHORT TemporaryMemoryFor320x240Copy[ 8 * 2 * g_iNuiRefinementKernelSize80x60 * g_iNuiRefinementKernelSize80x60 ];
    XMemSet( TemporaryMemoryFor320x240Copy, 0, sizeof(TemporaryMemoryFor320x240Copy) );
    INT iCurrentCopyLocationByte = 0;
    iCurrentCopyLocationByte = ( 32 * g_iNuiRefinementKernelSize80x60);   
    INT iBeginXOffset = XRange.iMin * 32;
    INT iEndXOffset = (g_iNuiRefinementKernelSize80x60 - XRange.iMax) * 32;

    for ( INT iY12x12 = YRange.iMin; iY12x12 < YRange.iMax; ++iY12x12 )
    {
        iCurrentCopyLocationByte += iBeginXOffset;
        for ( INT iX12x12 = XRange.iMin; iX12x12 < XRange.iMax; ++iX12x12 )
        {       
            if ( pEHSP->m_bValid80x60Voxels[iY12x12][iX12x12] )
            {
                __vector4 vmxRow1;
                __vector4 vmxRow2;
                __vector4 vmxRow3;
                __vector4 vmxRow4;
                INT iTouchOffset = iOffset;
                __dcbt( iTouchOffset, pDepth320x240 );
                iTouchOffset += iBytePitch320x240;
                __dcbt( iTouchOffset, pDepth320x240 );
                iTouchOffset += iBytePitch320x240;
                __dcbt( iTouchOffset, pDepth320x240 );
                iTouchOffset += iBytePitch320x240;
                __dcbt( iTouchOffset, pDepth320x240 );

                INT iLoadOffset = iOffset;
                vmxRow1 = __vor (__lvlx( pDepth320x240, iLoadOffset ),
                    __lvrx( pDepth320x240,  iLoadOffset + 16 ) );
                iLoadOffset+=iBytePitch320x240;
                vmxRow2 = __vor (__lvlx( pDepth320x240,  iLoadOffset ),
                    __lvrx( pDepth320x240, iLoadOffset + 16 ) );
                iLoadOffset+=iBytePitch320x240;
                vmxRow3 = __vor (__lvlx( pDepth320x240, iLoadOffset ),
                    __lvrx( pDepth320x240,  iLoadOffset + 16 ) );
                iLoadOffset+=iBytePitch320x240;
                vmxRow4 = __vor (__lvlx( pDepth320x240,  iLoadOffset ),
                    __lvrx( pDepth320x240, iLoadOffset + 16 ) );

////////////////////////////////////////////////////////////////////////////////////////////////////
                // Compute squared distance for first 8 depth values.
                __vector4 vmxFinalRow = __vperm( vmxRow1, vmxRow2, vmxMergeTwoRows );
                
                __vector4 vmxFirst8ShiftedDepths = __vsrh( vmxFinalRow, v8ChannelShiftRight3 );
                vmxFirst8ShiftedDepths = __vaddshs(vmxFirst8ShiftedDepths, vmxOffsetCloseToBody );
                __vector4 vmxDepthDiff = __vsubuhs( __vmaxuh( vmxFirst8ShiftedDepths, vmxDepth8 ), __vminuh( vmxFirst8ShiftedDepths, vmxDepth8 ) );
                __vector4 vmxDepthDiffNormalized = __vaddshs( __vsrh( vmxDepthDiff, v8ChannelShiftRight3 ),
                                                   __vsrh( vmxDepthDiff, v8ChannelShiftRight4 ) );

                __vector4 vmxFirst4Shorts2Floats = __vupkhsh( vmxDepthDiffNormalized );
                __vector4 vmxFloatCompareDepths = __vcfsx( vmxFirst4Shorts2Floats, 0 );
                __vector4 vmxFirst4Squared = __vmulfp( vmxFloatCompareDepths, vmxFloatCompareDepths );
                __vector4 vmxFirst4SquaredShorts =  __vctsxs( vmxFirst4Squared, 0 );

                __vector4 vmxSecond4Shorts2Floats = __vupklsh( vmxDepthDiffNormalized );
                vmxFloatCompareDepths = __vcfsx( vmxSecond4Shorts2Floats, 0 );
                __vector4 vmxSecond4Squared = __vmulfp( vmxFloatCompareDepths, vmxFloatCompareDepths );
                __vector4 vmxSecond4SquaredShorts =  __vctsxs( vmxSecond4Squared, 0 );
            
                __vector4 vmxFirst8SquaredDepths;
                vmxFirst8SquaredDepths = __vpkuwus(vmxFirst4SquaredShorts, vmxSecond4SquaredShorts );
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
                // Compute distance squared for second 8 depth values.
                vmxFinalRow = __vperm( vmxRow3, vmxRow4, vmxMergeTwoRows );
                
                __vector4 vmxSecond8ShiftedDepths = __vsrh( vmxFinalRow, v8ChannelShiftRight3 );
                vmxSecond8ShiftedDepths = __vaddshs(vmxSecond8ShiftedDepths, vmxOffsetCloseToBody );
                vmxDepthDiff = __vsubuhs( __vmaxuh( vmxSecond8ShiftedDepths, vmxDepth8 ), __vminuh( vmxSecond8ShiftedDepths, vmxDepth8 ) );
                vmxDepthDiffNormalized = __vaddshs( __vsrh( vmxDepthDiff, v8ChannelShiftRight3 ),
                                                   __vsrh( vmxDepthDiff, v8ChannelShiftRight4 ) );

                vmxFirst4Shorts2Floats = __vupkhsh( vmxDepthDiffNormalized );
                vmxFloatCompareDepths = __vcfsx( vmxFirst4Shorts2Floats, 0 );
                vmxFirst4Squared = __vmulfp( vmxFloatCompareDepths, vmxFloatCompareDepths );
                vmxFirst4SquaredShorts =  __vctsxs( vmxFirst4Squared, 0 );

                vmxSecond4Shorts2Floats = __vupklsh( vmxDepthDiffNormalized );
                vmxFloatCompareDepths = __vcfsx( vmxSecond4Shorts2Floats, 0 );
                vmxSecond4Squared = __vmulfp( vmxFloatCompareDepths, vmxFloatCompareDepths );
                vmxSecond4SquaredShorts =  __vctsxs( vmxSecond4Squared, 0 );
            
                __vector4 vmxSecond8SquaredDepths;
                vmxSecond8SquaredDepths = __vpkuwus(vmxFirst4SquaredShorts, vmxSecond4SquaredShorts );
////////////////////////////////////////////////////////////////////////////////////////////////////

                // Compute the difference for X, Y Values
////////////////////////////////////////////////////////////////////////////////////////////////////
                __vector4 vmxOffsetsIn320x240XY;
                vmxOffsetsIn320x240XY = __vperm( vmxXYIterator, __vzero(), vmxReplicateIteratorFor4x4Population );

                __vector4 vmxXYDifferences;
                __vector4 vmxIndividualXYOffsets;
                vmxIndividualXYOffsets =  __vaddshs ( vmxOffsetsIn320x240XY, vmxSub80x60Offsets );
                vmxXYDifferences = __vaddshs( vmxIndividualXYOffsets, vmxCopiesOfSearchLocationXANDY  );


                __vector4 vmxSignedUnpackHigh = __vupkhsh( vmxXYDifferences );
                vmxFloatCompareDepths = __vcfsx( vmxSignedUnpackHigh, 0 );
                vmxFirst4Squared = __vmulfp( vmxFloatCompareDepths, vmxFloatCompareDepths );
                vmxFirst4SquaredShorts =  __vctsxs( vmxFirst4Squared, 0 );

                __vector4 vmxSignedUnpackLow =  __vupklsh( vmxXYDifferences );
                vmxFloatCompareDepths = __vcfsx( vmxSignedUnpackLow, 0 );
                vmxSecond4Squared = __vmulfp( vmxFloatCompareDepths, vmxFloatCompareDepths );
                vmxSecond4SquaredShorts =  __vctsxs( vmxSecond4Squared, 0 ); 
            
                __vector4 vmxSquaredXYDepths;
                vmxSquaredXYDepths = __vpkuwus(vmxFirst4SquaredShorts, vmxSecond4SquaredShorts );
////////////////////////////////////////////////////////////////////////////////////////////////////

                // Compute the X diffRows
////////////////////////////////////////////////////////////////////////////////////////////////////
                __vector4 vmxFirst8SquaredXValues;
                vmxFirst8SquaredXValues = __vperm( vmxSquaredXYDepths, __vzero(), vmxCreateXSquaredPerm );
                __vector4 vmxSecond8SquaredXValues;
                vmxSecond8SquaredXValues = __vperm( vmxSquaredXYDepths, __vzero(), vmxCreateXSquaredPerm );
////////////////////////////////////////////////////////////////////////////////////////////////////

                // Compute the X diffRows
////////////////////////////////////////////////////////////////////////////////////////////////////
                __vector4 vmxFirst8SquaredYValues;
                vmxFirst8SquaredYValues = __vperm( vmxSquaredXYDepths, __vzero(), vmxCreateYSquaredPermFirst8  );
                __vector4 vmxSecond8SquaredYValues;
                vmxSecond8SquaredYValues = __vperm( vmxSquaredXYDepths, __vzero(), vmxCreateYSquaredPermSecond8  );
////////////////////////////////////////////////////////////////////////////////////////////////////
                
                // Compute TotalDifference Per Depth
///////////////////////////////////////////////////////////////////////////////////////////////////////
                __vector4 vmxDifferenceForFirst8;
                vmxDifferenceForFirst8 = __vadduhs( __vadduhs( vmxFirst8SquaredDepths, vmxFirst8SquaredXValues ) ,
                    vmxFirst8SquaredYValues );
                __vector4 vmxDifferenceForSecond8;
                vmxDifferenceForSecond8 = __vadduhs( __vadduhs( vmxSecond8SquaredDepths, vmxSecond8SquaredXValues ) ,
                    vmxSecond8SquaredYValues );
///////////////////////////////////////////////////////////////////////////////////////////////////////

                // Calculate the sub pixels that pass the threshold
///////////////////////////////////////////////////////////////////////////////////////////////////////
                __vector4 vmxValidFirst8;
                vmxValidFirst8 = __vcmpgtuh( vmxMaxSquaredRadius, vmxDifferenceForFirst8 );
                __vector4 vmxValidSecond8;
                vmxValidSecond8 = __vcmpgtuh( vmxMaxSquaredRadius, vmxDifferenceForSecond8 );
///////////////////////////////////////////////////////////////////////////////////////////////////////
                

                // Calculate the actual X, Y values of the sub 80x60 blocks that will be averaged should 
                //they pass the test
///////////////////////////////////////////////////////////////////////////////////////////////////////
                __vector4 vmxXLocationsFirst8;
                __vector4 vmxXLocationsSecond8;
                __vector4 vmxYLocationsFirst8;
                __vector4 vmxYLocationsSecond8;
                vmxXLocationsFirst8 = __vperm( vmxIndividualXYOffsets, __vzero(), vmxPermuteForIndividualXLocations );
                vmxXLocationsSecond8 = __vperm( vmxIndividualXYOffsets, __vzero(), vmxPermuteForIndividualXLocations );
                vmxYLocationsFirst8 = __vperm( vmxIndividualXYOffsets, __vzero(), vmxPermuteForIndividualYLocationsFirst8 );
                vmxYLocationsSecond8 = __vperm( vmxIndividualXYOffsets, __vzero(), vmxPermuteForIndividualYLocationsSecond8 );

                vmxFirst8ShiftedDepths = __vand( vmxValidFirst8, vmxFirst8ShiftedDepths );
                vmxSecond8ShiftedDepths = __vand( vmxValidSecond8, vmxSecond8ShiftedDepths );
                __stvx( vmxFirst8ShiftedDepths, TemporaryMemoryFor320x240Copy, iCurrentCopyLocationByte );
                __stvx( vmxSecond8ShiftedDepths, TemporaryMemoryFor320x240Copy, iCurrentCopyLocationByte + 16 );

                vmxXLocationsFirst8 = __vand( vmxValidFirst8, vmxXLocationsFirst8 );
                vmxXLocationsSecond8 = __vand( vmxValidSecond8, vmxXLocationsSecond8 );
                vmxYLocationsFirst8 = __vand( vmxValidFirst8, vmxYLocationsFirst8 );
                vmxYLocationsSecond8 = __vand( vmxValidSecond8, vmxYLocationsSecond8 );
                __vector4 vmxCountFirst8 = __vand( vmxValidFirst8, vSingleBitPerHalf );
                __vector4 vmxCountSecond8 = __vand( vmxValidSecond8, vSingleBitPerHalf );

                __vector4 vmxAddin;
                vmxAddin= __vupkhsh( vmxFirst8ShiftedDepths );
                vmxFinal4WayDepth = __vadduws( vmxAddin, vmxFinal4WayDepth );
                vmxAddin= __vupklsh( vmxFirst8ShiftedDepths );
                vmxFinal4WayDepth = __vadduws( vmxAddin, vmxFinal4WayDepth );
                vmxAddin= __vupkhsh( vmxSecond8ShiftedDepths );
                vmxFinal4WayDepth = __vadduws( vmxAddin, vmxFinal4WayDepth );
                vmxAddin= __vupklsh( vmxSecond8ShiftedDepths );
                vmxFinal4WayDepth = __vadduws( vmxAddin, vmxFinal4WayDepth );
                
                vmxAddin= __vupkhsh( vmxXLocationsFirst8 );
                vmxFinal4WayX = __vadduws( vmxAddin, vmxFinal4WayX );
                vmxAddin= __vupklsh( vmxXLocationsFirst8 );
                vmxFinal4WayX = __vadduws( vmxAddin, vmxFinal4WayX );
                vmxAddin= __vupkhsh( vmxXLocationsSecond8 );
                vmxFinal4WayX = __vadduws( vmxAddin, vmxFinal4WayX );
                vmxAddin= __vupklsh( vmxXLocationsSecond8 );
                vmxFinal4WayX = __vadduws( vmxAddin, vmxFinal4WayX );

                vmxAddin= __vupkhsh( vmxYLocationsFirst8 );
                vmxFinal4WayY = __vadduws( vmxAddin, vmxFinal4WayY );
                vmxAddin= __vupklsh( vmxYLocationsFirst8 );
                vmxFinal4WayY = __vadduws( vmxAddin, vmxFinal4WayY );
                vmxAddin= __vupkhsh( vmxYLocationsSecond8 );
                vmxFinal4WayY = __vadduws( vmxAddin, vmxFinal4WayY );
                vmxAddin= __vupklsh( vmxYLocationsSecond8 );
                vmxFinal4WayY = __vadduws( vmxAddin, vmxFinal4WayY );

                vmxAddin= __vupkhsh( vmxCountFirst8 );
                vmxFinalConnt = __vadduws( vmxAddin, vmxFinalConnt );
                vmxAddin= __vupklsh( vmxCountFirst8 );
                vmxFinalConnt = __vadduws( vmxAddin, vmxFinalConnt );
                vmxAddin= __vupkhsh( vmxCountSecond8 );
                vmxFinalConnt = __vadduws( vmxAddin, vmxFinalConnt );
                vmxAddin= __vupklsh( vmxCountSecond8 );
                vmxFinalConnt = __vadduws( vmxAddin, vmxFinalConnt );
#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
                
                __declspec(align(16)) SHORT Depths[16];
                __stvx( vmxValidFirst8, Depths, 0 );
                __stvx( vmxValidSecond8, Depths, 16 );
                for ( INT iY4x4 = 0; iY4x4 < 4; ++iY4x4 )   
                {
                    for ( INT iX4x4 = 0; iX4x4 < 4; ++iX4x4 )
                    {
                        if ( Depths[iY4x4*4+iX4x4] != 0 )
                        {
                            pHandSpecificData->m_VisualizeHandFramesData.m_FrameData[iY12x12*4+iY4x4][iX12x12*4+iX4x4] = 1;    
                        }
                        else 
                        {
                            pHandSpecificData->m_VisualizeHandFramesData.m_FrameData[iY12x12*4+iY4x4][iX12x12*4+iX4x4] = 0;    
                        }
                    }                
                }
#endif
            }
            iCurrentCopyLocationByte += 32;
            vmxXYIterator = __vaddshs( vmxIncrementXOuterLoop, vmxXYIterator );
            iOffset += 8;
        }
        iCurrentCopyLocationByte += iEndXOffset;
        vmxXYIterator = __vaddshs( vmxIncrementYOuterLoop, vmxXYIterator );
        
        iOffset -= (8*(XRange.iMax - XRange.iMin ));
        iOffset += iBytePitch320x240 * 4; // we're doing 4 rows at a tim
    }


    if ( pOptionalRefinementData->m_dwFlags & ATG_REFINE_HANDS_FLAG_CALCULATE320x240MASK )
    {
        memset( pEHSP->m_uHandDepthValues320x240, 0, sizeof(pEHSP->m_uHandDepthValues320x240) );

        INT iIndexIntoDepthValues = YRange.iMin * 4 * g_iNuiRefinementKernelSize320x240;
        INT iCurrentCopyLocation = 0;
        for ( INT iY12x12 = YRange.iMin; iY12x12 < YRange.iMax; ++iY12x12 )
        {
            for ( INT iX12x12 = 0; iX12x12 < g_iNuiRefinementKernelSize80x60; ++iX12x12 )
            {
                
                for ( INT iCopy4x4TileIndex=0; iCopy4x4TileIndex < 4; ++iCopy4x4TileIndex )
                {
                   pEHSP->m_uHandDepthValues320x240[iIndexIntoDepthValues++] = 
                      TemporaryMemoryFor320x240Copy[iCurrentCopyLocation++]; 
                }
                iIndexIntoDepthValues -=4;
                iIndexIntoDepthValues +=g_iNuiRefinementKernelSize320x240;

                for ( INT iCopy4x4TileIndex=0; iCopy4x4TileIndex < 4; ++iCopy4x4TileIndex )
                {
                   pEHSP->m_uHandDepthValues320x240[iIndexIntoDepthValues++] = 
                      TemporaryMemoryFor320x240Copy[iCurrentCopyLocation++]; 
                }
                iIndexIntoDepthValues -=4;
                iIndexIntoDepthValues +=g_iNuiRefinementKernelSize320x240;

                for ( INT iCopy4x4TileIndex=0; iCopy4x4TileIndex < 4; ++iCopy4x4TileIndex )
                {
                   pEHSP->m_uHandDepthValues320x240[iIndexIntoDepthValues++] = 
                      TemporaryMemoryFor320x240Copy[iCurrentCopyLocation++]; 
                }
                iIndexIntoDepthValues -=4;
                iIndexIntoDepthValues +=g_iNuiRefinementKernelSize320x240;

                for ( INT iCopy4x4TileIndex=0; iCopy4x4TileIndex < 4; ++iCopy4x4TileIndex )
                {
                   pEHSP->m_uHandDepthValues320x240[iIndexIntoDepthValues++] = 
                      TemporaryMemoryFor320x240Copy[iCurrentCopyLocation++]; 
                }
                iIndexIntoDepthValues -= g_iNuiRefinementKernelSize320x240;
                iIndexIntoDepthValues -= g_iNuiRefinementKernelSize320x240;
                iIndexIntoDepthValues -= g_iNuiRefinementKernelSize320x240;
                
            }
            iIndexIntoDepthValues +=g_iNuiRefinementKernelSize320x240;
            iIndexIntoDepthValues +=g_iNuiRefinementKernelSize320x240;
            iIndexIntoDepthValues +=g_iNuiRefinementKernelSize320x240;

        }
    }
    PIXEndNamedEvent();


    __declspec(align(16)) INT Counts[4];
    __stvx( vmxFinalConnt, Counts, 0 );
    INT iTotalCountVMX;
    iTotalCountVMX = Counts[0] + Counts[1] + Counts[2] + Counts[3];


    if ( iTotalCountVMX == 0 )
    {
        *pRefinedHand = pHandSpecificData->m_vRawHand;
    }
    else 
    {
        __declspec(align(16)) INT Depths[4];
        __stvx( vmxFinal4WayDepth, Depths, 0 );
        INT iDepth;
        iDepth = ( Depths[0] + Depths[1] + Depths[2] + Depths[3] );
        FLOAT fDepth;
        fDepth = (FLOAT) iDepth / (FLOAT) iTotalCountVMX;
        __declspec(align(16)) INT Xs[4];
        __stvx( vmxFinal4WayX, Xs, 0 );
        INT iXValue;
        iXValue = ( Xs[0] + Xs[1] + Xs[2] + Xs[3] );
        FLOAT fXValue;
        fXValue = (FLOAT)iXValue / (FLOAT)iTotalCountVMX;
        __declspec(align(16)) INT Ys[4];
        __stvx( vmxFinal4WayY, Ys, 0 );
        INT iYValue;
        iYValue = ( Ys[0] + Ys[1] + Ys[2] + Ys[3] );
        FLOAT fYValue;
        fYValue = (FLOAT)iYValue / (FLOAT)iTotalCountVMX;

        TransformScreenToWorld320x240( fXValue, 
                                       fYValue, 
                                       fDepth / 1000.0f,
                                       pRefinedHand
                                     );
    }
    return iTotalCountVMX;
#else
    PIXBeginNamedEvent( 0xffffffff, "Expensive Average" );
    
    // 8 shorts per row
    // 2 rows per voxel
    __declspec(align(128)) USHORT TemporaryMemoryFor320x240Copy[ 8 * 2 * g_iNuiRefinementKernelSize80x60 * g_iNuiRefinementKernelSize80x60 ];
    XMemSet( TemporaryMemoryFor320x240Copy, 0, sizeof(TemporaryMemoryFor320x240Copy) );
    INT iCurrentCopyLocationByte = 0;
    iCurrentCopyLocationByte = ( 32 * g_iNuiRefinementKernelSize80x60);   
    INT iBeginXOffset = XRange.iMin * 32;
    INT iAverageX = 0;
    INT iAverageY = 0;
    INT iAverageZ = 0;
    INT iTotalCount = 0;
    if ( pOptionalRefinementData->m_dwFlags & ATG_REFINE_HANDS_FLAG_CALCULATE320x240MASK )
    {
        memset( pEHSP->m_uHandDepthValues320x240, 0, sizeof(pEHSP->m_uHandDepthValues320x240) );
    }

    for ( INT iY12x12 = YRange.iMin; iY12x12 < YRange.iMax; ++iY12x12 )
    {
        iCurrentCopyLocationByte += iBeginXOffset;
        for ( INT iX12x12 = XRange.iMin; iX12x12 < XRange.iMax; ++iX12x12 )
        {       
            if ( pEHSP->m_bValid80x60Voxels[iY12x12][iX12x12] )
            {

                INT iX80x60 = iX12x12 - iNumSearchPixels + iRefinedPosition80x60.iX;
                INT iY80x60 = iY12x12 - iNumSearchPixels + iRefinedPosition80x60.iY;

                INT iX320x240 = iX80x60 * 4;
                INT iY320x240 = iY80x60 * 4;
                
                //char temp[255];
                for ( INT iY4x4 = 0; iY4x4 < 4; ++iY4x4 )   
                {
                    INT iFinalY = iY320x240 + iY4x4;
                    for ( INT iX4x4 = 0; iX4x4 < 4; ++iX4x4 )
                    {
                        INT iFinalX = iX320x240 + iX4x4;  

                        assert( iFinalX < 320 );
                        assert( iFinalY < 240 );
                        USHORT uValue = pDepth320x240[ iFinalY * iShortPitch320x240 + iFinalX ];
                        {
                            USHORT uDepth = uValue >> 3;
                            INT iDepthDiff = ( (INT)uDepth - iSearchLocation320x240.iZ );
                            // Body pixels are sometimes acccidentally averaged in the hands 
                            // bias slight towars pixels in front of the hand.
                            
                            iDepthDiff = (iDepthDiff / 16) + (iDepthDiff / 8);
                            INT iDepthDiffSquared = iDepthDiff * iDepthDiff;

                            INT iDist = ( iFinalX - iSearchLocation320x240.iX ) * ( iFinalX - iSearchLocation320x240.iX ) + 
                                ( iFinalY - iSearchLocation320x240.iY ) * ( iFinalY - iSearchLocation320x240.iY ) +
                                iDepthDiffSquared ;
                            if ( iDist < pEHSP->m_iSquaredSizeOfHandTheshold )
                            {
                                iAverageX += iFinalX;
                                iAverageY += iFinalY;
                                ++iTotalCount;
                                iAverageZ += (INT)uDepth;
#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
                                pHandSpecificData->m_VisualizeHandFramesData.m_FrameData[iY12x12*4+iY4x4][iX12x12*4+iX4x4] = 1;    
#endif
                                if ( pOptionalRefinementData->m_dwFlags & ATG_REFINE_HANDS_FLAG_CALCULATE320x240MASK )
                                {
                                    pEHSP->m_uHandDepthValues320x240[
                                        (iY12x12 * 4 + iY4x4) * g_iNuiRefinementKernelSize320x240
                                        + (iX12x12 * 4 + iX4x4) 
                                    ] = uDepth;
                                }

                            }
#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
                            else 
                            {
                                pHandSpecificData->m_VisualizeHandFramesData.m_FrameData[iY12x12*4+iY4x4][iX12x12*4+iX4x4] = 0;    
                            }
#endif
                        }
                    }
                }
            }
        }
    }
    PIXEndNamedEvent();

    if ( iTotalCount == 0 )
    {
        *pRefinedHand = pHandSpecificData->m_vRawHand;
    }
    else 
    {        
        TransformScreenToWorld320x240( (FLOAT)iAverageX / (FLOAT)iTotalCount, 
                            (FLOAT)iAverageY / (FLOAT)iTotalCount, 
                           (FLOAT)iAverageZ / (FLOAT)(1000.0f*(FLOAT)iTotalCount ),
                            pRefinedHand
                            );
    }
    return iTotalCount;

#endif
}


//--------------------------------------------------------------------------------------
// Name: CalculateSearchDirections
// Desc: This function calculates the closest search direction to the hand elbow vector.
//--------------------------------------------------------------------------------------
__forceinline VOID CalculateSearchDirection( INT iHandX, INT iHandY, INT iElbowX, INT iElbowY, INT* pSearch )
{
    static CONST FLOAT fPIDIV8 =  XM_PI / 8.0f;

    FLOAT fAngle = 0.0f;
    FLOAT fDirection[] = { (FLOAT)( iHandX - iElbowX  ), 
        (FLOAT)( iHandY - iElbowY ) };

    fAngle = atan2f( fDirection[1], fDirection[0] );
    fAngle += XM_PI + fPIDIV8;
    fAngle /= XM_PI;
    fAngle *= 4.0f;
    fAngle = fAngle;
    INT iAngle = (INT)fAngle;
    iAngle %= 8;
    iAngle = iAngle;
    *pSearch = iAngle;

}

//--------------------------------------------------------------------------------------
// Name: RefineWith320x240
// Desc: Take the 80x60 coordinate to the 320x240 map for refinement
// It also takes care of interpolating betwee raw and refined data when necessary.
//--------------------------------------------------------------------------------------
BOOL RefineWith320x240( DepthMaps* __restrict pDMS,
                        RefinementData* __restrict pRefinementData,
                        HandSpecificData* __restrict pHandSpecificData,
                        OptionalRefinementData* __restrict pOptionalRefinementData,
                        OptionalHandSpecificData* __restrict pOHSP
                      )
{
    static CONST FLOAT fTetheringDistanceLimit = 0.1f;
    static CONST FLOAT fInterpolateBetweenRawAndRefinedDecayRate = 0.8f;
    XMVECTOR vRefinedHand;
    INT iPixelsReturned = 0;

    if ( !pHandSpecificData->m_bRevertedToRawData ) 
    {       
        PIXBeginNamedEvent( 0xffffffff, "FindAverageForHand320x240" );
        iPixelsReturned = FindAverageForHand320x240( pDMS,
            pRefinementData,
            pHandSpecificData,
            &vRefinedHand,
            pOptionalRefinementData,
            pOHSP
        );
        PIXEndNamedEvent();
    }
    else 
    {
        #ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
            iPixelsReturned = FindAverageForHand320x240( pDMS,
                pRefinementData,
                pHandSpecificData,
                &vRefinedHand,
                pOptionalRefinementData,
                pOHSP
            );
        #endif
        vRefinedHand = pHandSpecificData->m_vRawHand;
    }

    assert( pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX < 80 );
    assert( pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY < 60 );

    pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iZ = 
        pDMS->m_pData80x60Min[pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX + 
                            pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY * 
                            pDMS->m_iShortPitch80x60Min] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

    XMVECTOR vPreviousRefinedHand;
    if ( pRefinementData->m_bFirstTimeThrough )
    {
        vPreviousRefinedHand = vRefinedHand;
    }
    else
    {
        vPreviousRefinedHand = pHandSpecificData->m_vRefinedHand;
    }
    XMVECTOR tetherVector = vRefinedHand - vPreviousRefinedHand;
    FLOAT interpolationValue = min( 1.0f, XMVector3Length(tetherVector).x / fTetheringDistanceLimit );
    vRefinedHand = vPreviousRefinedHand + tetherVector * interpolationValue;
    
    if ( pHandSpecificData->m_bRevertedToRawDataPreviousFrame != pHandSpecificData->m_bRevertedToRawData )
    {
        pHandSpecificData->m_vOffsetRawToRefined =  pHandSpecificData->m_vRefinedHand - vRefinedHand;
    }

    pHandSpecificData->m_vOffsetRawToRefined *= fInterpolateBetweenRawAndRefinedDecayRate;
    pHandSpecificData->m_vRefinedHand = vRefinedHand + pHandSpecificData->m_vOffsetRawToRefined;
    pHandSpecificData->m_bRevertedToRawDataPreviousFrame = pHandSpecificData->m_bRevertedToRawData;

    return TRUE;

}

//--------------------------------------------------------------------------------------
// Name: ClimbOntoHand80x60
// Desc: Climb onto the highest close point.  This will help climb onto the arm if the 
// skeleton is off the depth map.
//--------------------------------------------------------------------------------------
VOID ClimbOntoHand80x60 (   DepthMaps* __restrict pDMS, 
                            RefinementData* __restrict pRefinementData, 
                            HandSpecificData* __restrict pHandSpecificData )
{

    static CONST INT iBiggestPossibleDifferenceForJump = -150; 
    XMFLOAT4 fHandElbowDiff;
    USHORT* pDepth80x60Min = pDMS->m_pData80x60Min;
    INT iShortPitch80x60Min = pDMS->m_iShortPitch80x60Min;

    assert( pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX < 80 );
    assert( pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY < 60 );

    USHORT u80x60Raw = pDepth80x60Min[ pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX + 
        pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY * iShortPitch80x60Min ];
    INT i80x60RawDepth = (INT)(u80x60Raw >> NUI_IMAGE_PLAYER_INDEX_SHIFT);

    // when ST puts the arm out in space it's not on the depth map. so we're looking for the arm to be closer than the raw depth map position.
    INT iDiff = i80x60RawDepth - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iZ;

    if ( iDiff > g_iDistanceOffDepthMaptoWarrantLargeSearch || i80x60RawDepth == 0 ) 
    {
        INT2Vector iHandJumpSeedLocation(pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX, 
            pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY);
        INT2Vector iHandToElbow( pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iX - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX,
                                 pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iY - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY );

        iHandToElbow.iX /=2;
        iHandToElbow.iY /=2;

        iHandJumpSeedLocation.iX += iHandToElbow.iX;
        iHandJumpSeedLocation.iY += iHandToElbow.iY;

        Clamp80x60( &iHandJumpSeedLocation.iX, &iHandJumpSeedLocation.iY );

        // search 7x7 grid
        INT2Range iXRange( iHandJumpSeedLocation.iX - 3, iHandJumpSeedLocation.iX + 4 );
        INT2Range iYRange( iHandJumpSeedLocation.iY - 3, iHandJumpSeedLocation.iY + 4 );

        iXRange.iMin = max( iXRange.iMin, 0 );
        iXRange.iMax = min( iXRange.iMax, 79 );
        iYRange.iMin = max( iYRange.iMin, 0 );
        iYRange.iMax = min( iYRange.iMax, 59 );
        INT iSmallestDiff = INT_MAX;
        INT3Vector iSmallestLocation = pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;
        for ( INT iY = iYRange.iMin; iY < iYRange.iMax; ++iY )
        {
            for ( INT iX = iXRange.iMin; iX < iXRange.iMax; ++iX )
            {
                USHORT uCurrentValue = pDepth80x60Min[ iX + iY * iShortPitch80x60Min ]; 
                USHORT uDepth = uCurrentValue >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                if ( uDepth != 0 )
                {
                    iDiff = ( (INT)uDepth - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iZ );

                    if ( iDiff < iSmallestDiff ) 
                    {
                        iSmallestDiff = iDiff;
                        iSmallestLocation.iX = iX;
                        iSmallestLocation.iY = iY;
                        iSmallestLocation.iZ = (INT)uDepth;
                    }
                }
            }
        }
        pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60 = iSmallestLocation;

    }        
    else if ( iDiff > g_iDistanceOffDepthMaptoWarrantSmallSearch )  
    {
        //do a small search

        // search 3x3 grid
        INT2Vector iHandJumpSeedLocation( pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX, 
            pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY);
        Clamp80x60( &iHandJumpSeedLocation.iX, &iHandJumpSeedLocation.iY );

        INT2Range iXRange( iHandJumpSeedLocation.iX - 1, iHandJumpSeedLocation.iX + 2 );
        INT2Range iYRange( iHandJumpSeedLocation.iY - 1, iHandJumpSeedLocation.iY + 2 );
        iXRange.iMin = max( iXRange.iMin, 0 );
        iXRange.iMax = min( iXRange.iMax, 79 );
        iYRange.iMin = max( iYRange.iMin, 0 );
        iYRange.iMax = min( iYRange.iMax, 59 );
        INT iSmallestDiff = INT_MAX;
        INT3Vector iSmallestLocation = pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;
        for ( INT iY = iYRange.iMin; iY < iYRange.iMax; ++iY )
        {
            for ( INT iX = iXRange.iMin; iX < iXRange.iMax; ++iX )
            {
                USHORT uCurrentValue = pDepth80x60Min[ iX + iY * iShortPitch80x60Min ]; 
                USHORT uDepth = uCurrentValue >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                if ( uDepth != 0 )
                {
                    iDiff = ( (INT)uDepth - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iZ );
                    if ( iDiff < iSmallestDiff && iDiff > iBiggestPossibleDifferenceForJump ) // we're only searching 1 pixel so we're looking for small differences  
                    {
                        iSmallestDiff = iDiff;
                        iSmallestLocation.iX = iX;
                        iSmallestLocation.iY = iY;
                        iSmallestLocation.iZ = (INT)uDepth;
                    }
                }
            }
        }
        pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60 = iSmallestLocation;
    }

#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
    pHandSpecificData->m_INT3ScreenSpaceHandJumpOnArm80x60 = pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;
#endif

}

//--------------------------------------------------------------------------------------
// Name: WalkToEndOfHand80x60SingleDirection
// Desc: Follows the 5 closest directions to the passed in direction until a large edge 
// is found.  Stores the new end position into the hand-specific data as the new refined 
// hand.
//--------------------------------------------------------------------------------------
INT3Vector WalkToEndOfHand80x60SingleDirection( INT iDirection,
                                               INT3Vector iStartLocation,
                                               INT iWalkToEndOfHandThreshold, 
                                               DepthMaps* __restrict pDMS, 
                                               RefinementData* __restrict pRefinementData, 
                                               HandSpecificData* __restrict pHandSpecificData,
                                               INT iDirectionCount )
{
    INT iDirections[SEARCHDIRECTION_COUNT];
    iDirections[SEARCHDIRECTION_FORWARD] = (iDirection );
    iDirections[SEARCHDIRECTION_DIAGONAL_LEFT] = (iDirection + 7) % 8;
    iDirections[SEARCHDIRECTION_DIAGONAL_RIGHT] = (iDirection + 1) % 8;
    iDirections[SEARCHDIRECTION_ORTHOGONAL_LEFT] = (iDirection + 6) % 8;
    iDirections[SEARCHDIRECTION_ORTHOGONAL_RIGHT] = (iDirection + 2) % 8;

    INT iShortPitch80x60Min = pDMS->m_iShortPitch80x60Min;
    USHORT* pDepth80x60Min = pDMS->m_pData80x60Min;
   
    INT iSmallestValue = g_iMaxImageDepth;
    INT iOffset[] = {0,0};

    INT3Vector WalkLocation = iStartLocation; 
    INT iCurrentMarchLocationDepth = 0;
    INT iLastSmallestIndex = -1;
    INT iSmallestTest = -1;

    assert( iStartLocation.iX < 80 );
    assert( iStartLocation.iY < 60 );

    for ( INT iWalkIndex = 0; iWalkIndex < g_nWalkToEdgeIterations; ++iWalkIndex )
    {   
        WalkLocation.iX = iStartLocation.iX + iOffset[0];
        WalkLocation.iY = iStartLocation.iY + iOffset[1];

        Clamp80x60( &WalkLocation.iX, &WalkLocation.iY );
        
        assert( WalkLocation.iX < 80 );
        assert( WalkLocation.iY < 60 );

        iCurrentMarchLocationDepth = pDepth80x60Min[ WalkLocation.iX + WalkLocation.iY * iShortPitch80x60Min ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

        INT3Vector EdgeWalkLocation = WalkLocation;
        EdgeWalkLocation.iZ = iCurrentMarchLocationDepth;
        // Find the average for the potential search directions

        iSmallestTest = -1;
        iSmallestValue = g_iMaxImageDepth;
        INT iOffLimitsDirection = -1;
        if ( iLastSmallestIndex == SEARCHDIRECTION_ORTHOGONAL_LEFT )
        {
            iOffLimitsDirection = SEARCHDIRECTION_ORTHOGONAL_RIGHT;
        }
        else if ( iLastSmallestIndex == SEARCHDIRECTION_ORTHOGONAL_RIGHT )
        {
            iOffLimitsDirection = SEARCHDIRECTION_ORTHOGONAL_LEFT;
        }

        for ( INT iIndex = 0; iIndex < iDirectionCount; ++iIndex )
        {
            EdgeWalkLocation.iX = iStartLocation.iX + 
                iOffset[0] + g_SpiralSearchOffsets[iDirections[iIndex]].iX;
            EdgeWalkLocation.iY = iStartLocation.iY + 
                iOffset[1] + g_SpiralSearchOffsets[iDirections[iIndex]].iY;

            Clamp80x60( &EdgeWalkLocation.iX, &EdgeWalkLocation.iY );

            assert( EdgeWalkLocation.iX < 80 );
            assert( EdgeWalkLocation.iY < 60 );

            USHORT uValue = pDepth80x60Min[ EdgeWalkLocation.iX + EdgeWalkLocation.iY * iShortPitch80x60Min ]; 
            INT iDepth = uValue >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
            INT iDiff = iDepth - iCurrentMarchLocationDepth;
            if ( ( iDiff ) < iWalkToEndOfHandThreshold )
            {
                iDiff = INTABSFAST( iDiff );
                INT Test = iDiff + g_iBiasFor5SearchDirections[iIndex];
                if ( iSmallestValue > Test && iIndex != iOffLimitsDirection && iDiff < 150 )
                {
                    iSmallestTest = iIndex;
                    iSmallestValue = Test;
                }
            }
        }
        
        iLastSmallestIndex = iSmallestTest;

        if ( iSmallestTest == -1 )
        {
            break;
        }
        iOffset[0] += g_SpiralSearchOffsets[iDirections[iSmallestTest]].iX; 
        iOffset[1] += g_SpiralSearchOffsets[iDirections[iSmallestTest]].iY; 


    }
    INT3Vector iWalkedLocation;
    iWalkedLocation.iX = iStartLocation.iX + iOffset[0];
    iWalkedLocation.iY = iStartLocation.iY + iOffset[1];
    Clamp80x60( &iWalkedLocation.iX, &iWalkedLocation.iY );
    iWalkedLocation.iZ = pDepth80x60Min[ iWalkedLocation.iX + iWalkedLocation.iY * iShortPitch80x60Min ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT; 

    return iWalkedLocation;

}

//--------------------------------------------------------------------------------------
// Name: WalkToEndofHand80x60
// Desc: Follow the current position away from the elbow towards the end of the hand
// 
//--------------------------------------------------------------------------------------
VOID WalkToEndofHand80x60 ( DepthMaps* __restrict pDMS, 
                            RefinementData* __restrict pRefinementData, 
                            HandSpecificData* __restrict pHandSpecificData, 
                            OptionalHandSpecificData* __restrict pExtraData )
{

    INT iDirection1;

    CalculateSearchDirection( 
        pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX, 
        pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY, 
        pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iX, 
        pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iY, &iDirection1 );
    INT iDirection2 = ( iDirection1  + 1 ) % 8;
    INT iDirection3 = ( iDirection1  + 7 ) % 8;

    INT3Vector iDirection1Vector;
    INT3Vector iDirection2Vector;
    INT3Vector iDirection3Vector;

    BOOL bWalkWasValid = FALSE;
    
    INT3Vector idiff;
    idiff.iX = pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iX - pRefinementData->m_INT3ScreenSpaceSpine80x60.iX;
    idiff.iY = pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iY - pRefinementData->m_INT3ScreenSpaceSpine80x60.iY;
    idiff.iX *= idiff.iX;
    idiff.iY *= idiff.iY;
    INT3Vector INT3WalkAwayFromPoint;

    INT3WalkAwayFromPoint = pHandSpecificData->m_INT3ScreenSpaceElbow80x60;


    static CONST INT iIterations = sizeof(g_iWalktoEndofHandThesholds) / sizeof(INT);
    for ( INT iIndex = 0; iIndex < iIterations && !bWalkWasValid; ++iIndex )
    {
        PIXBeginNamedEvent( 0, "Walk to End of Hand Direction 1" );
        iDirection1Vector = WalkToEndOfHand80x60SingleDirection( iDirection1, 
                pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60,
                g_iWalktoEndofHandThesholds[iIndex], pDMS, pRefinementData, pHandSpecificData, SEARCHDIRECTION_COUNT );
        PIXEndNamedEvent();
        PIXBeginNamedEvent( 0, "Walk to End of Hand Direction 2" );
            iDirection2Vector = WalkToEndOfHand80x60SingleDirection( iDirection2,
                pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60,
                g_iWalktoEndofHandThesholds[iIndex], pDMS, pRefinementData, pHandSpecificData, SEARCHDIRECTION_COUNT );
        PIXEndNamedEvent();
        PIXBeginNamedEvent( 0, "Walk to End of Hand Direction 3" );
            iDirection3Vector = WalkToEndOfHand80x60SingleDirection( iDirection3,
                pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60,
                g_iWalktoEndofHandThesholds[iIndex], pDMS, pRefinementData, pHandSpecificData, SEARCHDIRECTION_COUNT );
        PIXEndNamedEvent();

        idiff.iX = iDirection1Vector.iX - INT3WalkAwayFromPoint.iX;
        idiff.iY = iDirection1Vector.iY - INT3WalkAwayFromPoint.iY;
        idiff.iZ = ( iDirection1Vector.iZ - INT3WalkAwayFromPoint.iZ ) / 24;
        INT iDistance1 = idiff.iX * idiff.iX + idiff.iY * idiff.iY + idiff.iZ * idiff.iZ;

        idiff.iX = iDirection2Vector.iX - INT3WalkAwayFromPoint.iX;
        idiff.iY = iDirection2Vector.iY - INT3WalkAwayFromPoint.iY;
        idiff.iZ = ( iDirection2Vector.iZ - INT3WalkAwayFromPoint.iZ ) / 24;
        INT iDistance2 = idiff.iX * idiff.iX + idiff.iY * idiff.iY + idiff.iZ * idiff.iZ;

        idiff.iX = iDirection3Vector.iX - INT3WalkAwayFromPoint.iX;
        idiff.iY = iDirection3Vector.iY - INT3WalkAwayFromPoint.iY;
        idiff.iZ = ( iDirection3Vector.iZ - INT3WalkAwayFromPoint.iZ ) / 24;
        INT iDistance3 = idiff.iX * idiff.iX + idiff.iY * idiff.iY + idiff.iZ * idiff.iZ;

        INT3Vector iDirectionVector;
        INT iTestDifference;

        INT iBestDirection = 0;
        if ( iDistance1 >= iDistance2 && iDistance1 >= iDistance3 )
        {
            iBestDirection = iDirection1;
            iDirectionVector = iDirection1Vector;
            idiff = iDirectionVector;
            idiff.iX -= pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iX;
            idiff.iY -= pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iY;
        }
        else if ( iDistance2 >= iDistance3 )
        {
            iBestDirection = iDirection2;
            iDirectionVector = iDirection2Vector;
            idiff = iDirectionVector;
            idiff.iX -= pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iX;
            idiff.iY -= pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iY;
        }
        else 
        {
            iBestDirection = iDirection3;
            iDirectionVector = iDirection3Vector;
            idiff = iDirectionVector;
            idiff.iX -= pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iX;
            idiff.iY -= pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iY;
        }
        iTestDifference = idiff.iX * idiff.iX + idiff.iY * idiff.iY;
        INT iDirection = 0;
        CalculateSearchDirection( iDirectionVector.iX, iDirectionVector.iY, 
            pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX, 
            pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY, &iDirection );
        if ( iTestDifference < pRefinementData->m_iSpillDistanceSquared )
        {
                pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60 = iDirectionVector;
                bWalkWasValid = TRUE;
        }
    }
    if ( !bWalkWasValid )
    {
        pHandSpecificData->m_bRevertedToRawData = TRUE;
    }

#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
    pHandSpecificData->m_INT3ScreenSpaceHandWalkToEndofArm80x60 = pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;
#endif

}


//--------------------------------------------------------------------------------------
// Name: InitializeHand
// Desc: Resets all of the hand specific data. Calculates hand specific data 
// such as the search radius and limb lengths
//--------------------------------------------------------------------------------------
VOID InitializeHand( NUI_SKELETON_POSITION_INDEX eHand, 
                    NUI_SKELETON_POSITION_INDEX eElbow,
                    RefinementData* __restrict pRefinementData,
                    const XMVECTOR pSkeletonJoints[NUI_SKELETON_POSITION_COUNT], 
                    HandSpecificData* __restrict pHandData,
                    OptionalHandSpecificData* __restrict pEHSP )
{
    static CONST FLOAT fScaleToRadiusSize = 20.0f;
    

#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
    memset( &pHandData->m_INT3ScreenSpaceHandJumpOnArm80x60, 0, sizeof(INT3Vector) );    
    memset( &pHandData->m_INT3ScreenSpaceHandWalkToEndofArm80x60, 0, sizeof(INT3Vector) );    
    memset( &pHandData->m_INT3ScreenSpaceHandCenterAroundEndofArm80x60, 0, sizeof(INT3Vector) );    
#endif

    pHandData->m_iOffsetForHandCloseToBody = 0;
    FLOAT fSearchRadiusAtNearPlane =( pRefinementData->m_fHeadToSpine * fScaleToRadiusSize);
    
    FLOAT fRadius = fSearchRadiusAtNearPlane / XMVectorGetZ( pSkeletonJoints[eHand] );
    INT iSize =  sizeof( OptionalHandSpecificData );
    memset( pEHSP, 0, iSize );
    pEHSP->m_iVoxelSearchRadius = min( (INT)fRadius, g_iNuiRefinementKernelMaxSize80x60 );
    // scale from 80x60 to 320x240
    fRadius *= 4.0f;
    fRadius = min( fRadius, g_fNuiRefinementHalfKernelSize320x240 );

    pEHSP->m_iSquaredSizeOfHandTheshold = (INT) ( fRadius * fRadius );

    pHandData->m_vRawElbow = pSkeletonJoints[eElbow];
    pHandData->m_vRawHand = pSkeletonJoints[eHand];

    TransformWorldToScreen80x60( pHandData->m_vRawElbow, 
        &pHandData->m_INT3ScreenSpaceElbow80x60 );
    
    TransformWorldToScreen80x60( pSkeletonJoints[eHand], 
        &pHandData->m_INT3ScreenSpaceHandRaw80x60 );
    ATG::INT3Vector* ScreenHand = &pHandData->m_INT3ScreenSpaceHandRaw80x60;
    if ( ScreenHand->iX < 0 ||  ScreenHand->iX > 79 || ScreenHand->iY < 0|| ScreenHand->iY > 59 )
    {
        pHandData->m_bRevertedToRawData = TRUE;
    }
    else
    {
        pHandData->m_bRevertedToRawData = FALSE;
    }
    pHandData->m_INT3ScreenSpaceHandRefined80x60  = pHandData->m_INT3ScreenSpaceHandRaw80x60;
    Clamp80x60( &pHandData->m_INT3ScreenSpaceHandRefined80x60.iX, &pHandData->m_INT3ScreenSpaceHandRefined80x60.iY );
    pHandData->m_eHand = eHand;

}

//--------------------------------------------------------------------------------------
// Name: BuildValidMaskWithSearchSpiral80x60()
// Desc: Builds the mask that is used to threshold the hand from the body and optimize 
// the 320x240 search.  
//--------------------------------------------------------------------------------------
VOID BuildValidMaskWithSearchSpiral80x60( DepthMaps* __restrict pDMS,
                                          RefinementData* __restrict pRefinementData,
                                          INT3Vector Position80x60,
                                          BOOL* __restrict pValid80x60Voxels,
                                          INT iThresholdForNeighbor,
                                          INT iNegativeThresholdForNeighbor
                                         )
{

    PIXBeginNamedEvent( 0, "BuildValidVoxelMaskWithSearchSpiral80x60" );

    INT2Vector iKernelCenter(g_iNuiRefinementKernelCenter80x60, g_iNuiRefinementKernelCenter80x60);
    // start the center position of he mask to on
    pValid80x60Voxels[ iKernelCenter.iY * g_iNuiRefinementKernelSize80x60 + iKernelCenter.iX ] = TRUE;
    
    USHORT* pDepth80x60Min = pDMS->m_pData80x60Min;
    INT iShortPitch80x60Min = pDMS->m_iShortPitch80x60Min;
    
    static INT iSearchIterations = sizeof( g_UnrolledNeighborSearchSpiral ) / sizeof( SpiralSearchLocation ) ;
    INT2Vector CurrentRingLocation80x60;
    INT2Vector CurrentTestLocationVoxel;
    SpiralSearchLocation* pCurrentSpiral;
    INT iLastKnownDepth = Position80x60.iZ;
    INT iCurrentRing80x60Location = Position80x60.iY * iShortPitch80x60Min + Position80x60.iX;
    for ( INT iIter =0; iIter <  iSearchIterations; ++iIter )
    {
        pCurrentSpiral = &g_UnrolledNeighborSearchSpiral[iIter];
        CurrentRingLocation80x60.iX = Position80x60.iX + pCurrentSpiral->m_iSearchSpot.iX;
        CurrentRingLocation80x60.iY = Position80x60.iY + pCurrentSpiral->m_iSearchSpot.iY;
        iCurrentRing80x60Location += pCurrentSpiral->m_iOffsetIn80x60;
        if ( CurrentRingLocation80x60.iX < 80 && CurrentRingLocation80x60.iX > -1 && CurrentRingLocation80x60.iY < 60 && CurrentRingLocation80x60.iY > -1 )
        // this is the only value that needs to be tested to keeps us from reading outside of the buffer.
        //all other buffers will be calculated to point inwards from this buffer and therefore don't need to be calculated.
        {
            INT iCurrentDepth= pDepth80x60Min[ iCurrentRing80x60Location ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
            // 3 interations of unrolled loop
            {
                INT iCurrentLocation12x12 = ( iKernelCenter.iY + pCurrentSpiral->m_iSearchSpot.iY )  
                    * g_iNuiRefinementKernelSize80x60 + (iKernelCenter.iX + pCurrentSpiral->m_iSearchSpot.iX );
                INT iTestLocation12x12 = iCurrentLocation12x12 + 
                    g_SpiralSearchOffsets12x12[pCurrentSpiral->m_iSearchDirection];
                
                if ( pValid80x60Voxels[iTestLocation12x12] )
                {
                    INT iTestDepthLocation80x60 = 
                        (iCurrentRing80x60Location + g_SpiralSearchOffsets80x60[pCurrentSpiral->m_iSearchDirection]);

                    INT iDepthForTestLocation = pDepth80x60Min[ iTestDepthLocation80x60 ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

                    INT iDiff = iCurrentDepth - iDepthForTestLocation;
                    if ( ( iDiff ) < iThresholdForNeighbor && iDiff > iNegativeThresholdForNeighbor )
                    {
                        iLastKnownDepth = iCurrentDepth;
                        pValid80x60Voxels[ iCurrentLocation12x12 ] = TRUE;   
                    }
                }
            }
            {
                INT iDirection = (pCurrentSpiral->m_iSearchDirection+1)%8;
                INT iCurrentLocation12x12 = ( iKernelCenter.iY + pCurrentSpiral->m_iSearchSpot.iY )  * g_iNuiRefinementKernelSize80x60 + 
                             (iKernelCenter.iX + pCurrentSpiral->m_iSearchSpot.iX );
                INT iTestLocation12x12 = iCurrentLocation12x12 + g_SpiralSearchOffsets12x12[iDirection];
                
                if ( pValid80x60Voxels[iTestLocation12x12] )
                {
                    INT iTestDepthLocation80x60 = 
                        (iCurrentRing80x60Location + g_SpiralSearchOffsets80x60[iDirection]);

                    INT iDepthForTestLocation = pDepth80x60Min[ iTestDepthLocation80x60 ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

                    INT iDiff = iCurrentDepth - iDepthForTestLocation;
                    if ( ( iDiff ) < iThresholdForNeighbor && iDiff > iNegativeThresholdForNeighbor )
                    {
                        iLastKnownDepth = iCurrentDepth;
                        pValid80x60Voxels[ iCurrentLocation12x12 ] = TRUE;   
                    }
                }
            }
            {
                INT iDirection = (pCurrentSpiral->m_iSearchDirection+7)%8;
                INT iCurrentLocation12x12 = ( iKernelCenter.iY + pCurrentSpiral->m_iSearchSpot.iY )  * g_iNuiRefinementKernelSize80x60 + 
                             (iKernelCenter.iX + pCurrentSpiral->m_iSearchSpot.iX );
                INT iTestLocation12x12 = iCurrentLocation12x12 + g_SpiralSearchOffsets12x12[iDirection];
                
                if ( pValid80x60Voxels[iTestLocation12x12] )
                {
                    INT iTestDepthLocation80x60 = 
                        (iCurrentRing80x60Location + g_SpiralSearchOffsets80x60[iDirection]);

                    INT iDepthForTestLocation = pDepth80x60Min[ iTestDepthLocation80x60 ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

                    INT iDiff = iCurrentDepth - iDepthForTestLocation;
                    if ( ( iDiff ) < iThresholdForNeighbor && iDiff > iNegativeThresholdForNeighbor )
                    {
                        iLastKnownDepth = iCurrentDepth;
                        pValid80x60Voxels[ iCurrentLocation12x12 ] = TRUE;   
                    }
                }
            }
        }

        
    }
    PIXEndNamedEvent();

}

//--------------------------------------------------------------------------------------
// Name: MaskTooCloseToBodyToCenter()
// Desc: Determines if the mask is too close to the body for centering to be helpful.
//--------------------------------------------------------------------------------------
BOOL MaskTooCloseToBodyToCenter (   INT iDirection,
                                    DepthMaps* __restrict pDMS,
                                    RefinementData* __restrict pRefinementData,
                                    HandSpecificData* __restrict pHandSpecificData,
                                    BOOL* __restrict pValid80x60Voxels )
{
    INT diff, mask;
    INT2Vector iCurrentGridCell;
    USHORT* pDepth80x60Min = pDMS->m_pData80x60Min;
    INT iShortPitch80x60Min = pDMS->m_iShortPitch80x60Min;

    INT iMaxValueOfOuterVoxelMask = 0;
    INT2Vector iTest;

    INT iX,iY;
    for ( INT iOuter= 0; iOuter < 11; ++iOuter )
    {
        iX = iOuter;
        iY = 0;
        if ( pValid80x60Voxels[iY*g_iNuiRefinementKernelSize80x60+iX] )
        {
            iTest.iX = iX;
            iTest.iY = iY;
            iTest.iX += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX;
            iTest.iY += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY;
            if ( iTest.iX >=0 && iTest.iX < 80 && iTest.iY >=0 && iTest.iY < 60 )
            {
                INT iDepth = pDepth80x60Min[iTest.iX + iTest.iY * iShortPitch80x60Min] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                GETMAX( iMaxValueOfOuterVoxelMask, iMaxValueOfOuterVoxelMask, iDepth ); 
            }
        }
        iX = iOuter;
        iY = 10;
        if ( pValid80x60Voxels[iY*g_iNuiRefinementKernelSize80x60+iX] )
        {
            iTest.iX = iX;
            iTest.iY = iY;
            iTest.iX += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX;
            iTest.iY += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY;
            if ( iTest.iX >=0 && iTest.iX < 80 && iTest.iY >=0 && iTest.iY < 60 )
            {
                INT iDepth = pDepth80x60Min[iTest.iX + iTest.iY * iShortPitch80x60Min] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                GETMAX( iMaxValueOfOuterVoxelMask, iMaxValueOfOuterVoxelMask, iDepth ); 
            }
        }
        iX = 0;
        iY = iOuter;
        if ( pValid80x60Voxels[iY*g_iNuiRefinementKernelSize80x60+iX] )
        {
            iTest.iX = iX;
            iTest.iY = iY;
            iTest.iX += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX;
            iTest.iY += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY;
            if ( iTest.iX >=0 && iTest.iX < 80 && iTest.iY >=0 && iTest.iY < 60 )
            {
                INT iDepth = pDepth80x60Min[iTest.iX + iTest.iY * iShortPitch80x60Min] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                GETMAX( iMaxValueOfOuterVoxelMask, iMaxValueOfOuterVoxelMask, iDepth ); 
            }
        }
        iX = 10;
        iY = iOuter;
        if ( pValid80x60Voxels[iY*g_iNuiRefinementKernelSize80x60+iX] )
        {
            iTest.iX = iX;
            iTest.iY = iY;
            iTest.iX += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX;
            iTest.iY += -g_iNuiRefinementKernelCenter80x60 + pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY;
            if ( iTest.iX >=0 && iTest.iX < 80 && iTest.iY >=0 && iTest.iY < 60 )
            {
                INT iDepth = pDepth80x60Min[iTest.iX + iTest.iY * iShortPitch80x60Min] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                GETMAX( iMaxValueOfOuterVoxelMask, iMaxValueOfOuterVoxelMask, iDepth ); 
            }
        }
    }
    {
        INT iDirections[] = { iDirection, ( iDirection + 1 ) % 8, ( iDirection + 7 ) % 8 };
        INT iOutOfVoxelValue = g_iMaxImageDepth; 
        INT iZeros = 0;
        for ( INT iIndex = 0; iIndex < 3; ++iIndex )
        {
            
            iTest.iX = g_SpiralSearchOffsets[iDirections[iIndex]].iX * 2; 
            iTest.iY = g_SpiralSearchOffsets[iDirections[iIndex]].iY * 2; 
            if ( !pValid80x60Voxels[ ( iTest.iY + g_iNuiRefinementKernelCenter80x60 ) * 
                g_iNuiRefinementKernelSize80x60 + iTest.iX + g_iNuiRefinementKernelCenter80x60] )
            iTest.iX += pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX;
            iTest.iY += pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY;
            if ( iTest.iX >=0 && iTest.iX < 80 && iTest.iY >=0 && iTest.iY < 60 )
            {
                INT iDepth = pDepth80x60Min[iTest.iY * iShortPitch80x60Min + iTest.iX ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
                if ( iDepth == 0 )
                {
                    ++iZeros;
                }
                else
                {
                    GETMIN( iOutOfVoxelValue, iDepth, iOutOfVoxelValue );
                }
            }
        }
        if ( iZeros > 1 ) 
        {
            return TRUE;
        }
        if ( iOutOfVoxelValue != g_iMaxImageDepth )
        {
            if ( ( iOutOfVoxelValue - iMaxValueOfOuterVoxelMask ) > 200 )
            {
                return TRUE;
            }
            if ( ( iOutOfVoxelValue - iMaxValueOfOuterVoxelMask ) < -30 )
            {
                pHandSpecificData->m_iOffsetForHandCloseToBody = 50;
            }
            if ( ( iOutOfVoxelValue - iMaxValueOfOuterVoxelMask ) < 30 )
            {
                pHandSpecificData->m_iOffsetForHandCloseToBody = 30;
            }
        }
    }
    return FALSE;
}

//--------------------------------------------------------------------------------------
// Name: MaskSpill()
// Desc: Determines if the mask spilled off the hand and passed the end of the hand.
//--------------------------------------------------------------------------------------
BOOL MaskSpill( BOOL* __restrict pValid80x60Voxels, INT2Vector INT2SearchNormal )
{
    INT iEdgeCount = 0;
    INT2Vector iCurrentGridCell;  
    for ( INT iSidesIndex = 0; iSidesIndex < 10; ++iSidesIndex )
    {
       
        iCurrentGridCell.iX = iSidesIndex;
        iCurrentGridCell.iY = 0;
        if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
        {
            ++iEdgeCount;
        }
        iCurrentGridCell.iX = iSidesIndex;
        iCurrentGridCell.iY = 10;
        if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
        {
            ++iEdgeCount;
        }
        iCurrentGridCell.iX = 0;
        iCurrentGridCell.iY = iSidesIndex;
        if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
        {
            ++iEdgeCount;
        }
        iCurrentGridCell.iX = 10;
        iCurrentGridCell.iY = iSidesIndex;
        if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
        {
            ++iEdgeCount;
        }
    }
    if ( iEdgeCount > 3 )
    {

        INT2Vector iVectorToCell;
        BOOL iSpillDetected = FALSE;
        for ( INT iSidesIndex = 0; iSidesIndex < 10 && !iSpillDetected; ++iSidesIndex )
        {
            
            iCurrentGridCell.iX = iSidesIndex;
            iCurrentGridCell.iY = 0;
            if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
            {
                iVectorToCell.iX =  iCurrentGridCell.iX - g_iNuiRefinementKernelCenter80x60;
                iVectorToCell.iY =  iCurrentGridCell.iY - g_iNuiRefinementKernelCenter80x60;
                INT iTest = iVectorToCell.iX * INT2SearchNormal.iX + iVectorToCell.iY * INT2SearchNormal.iY;
                if ( iTest < 0 )
                {
                    return TRUE;
                }
            }
            iCurrentGridCell.iX = iSidesIndex;
            iCurrentGridCell.iY = 10;
            if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
            {
                iVectorToCell.iX =  iCurrentGridCell.iX - g_iNuiRefinementKernelCenter80x60;
                iVectorToCell.iY =  iCurrentGridCell.iY - g_iNuiRefinementKernelCenter80x60;
                INT iTest = iVectorToCell.iX * INT2SearchNormal.iX + iVectorToCell.iY * INT2SearchNormal.iY;
                if ( iTest < 0 )
                {
                    return TRUE;
                }
            }
            iCurrentGridCell.iX = 0;
            iCurrentGridCell.iY = iSidesIndex;
            if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
            {
                iVectorToCell.iX =  iCurrentGridCell.iX - g_iNuiRefinementKernelCenter80x60;
                iVectorToCell.iY =  iCurrentGridCell.iY - g_iNuiRefinementKernelCenter80x60;
                INT iTest = iVectorToCell.iX * INT2SearchNormal.iX + iVectorToCell.iY * INT2SearchNormal.iY;
                if ( iTest < 0 )
                {
                    return TRUE;
                }
            }
            iCurrentGridCell.iX = 10;
            iCurrentGridCell.iY = iSidesIndex;
            if ( pValid80x60Voxels[iCurrentGridCell.iY*g_iNuiRefinementKernelSize80x60+iCurrentGridCell.iX] )
            {
                iVectorToCell.iX =  iCurrentGridCell.iX - g_iNuiRefinementKernelCenter80x60;
                iVectorToCell.iY =  iCurrentGridCell.iY - g_iNuiRefinementKernelCenter80x60;
                INT iTest = iVectorToCell.iX * INT2SearchNormal.iX + iVectorToCell.iY * INT2SearchNormal.iY;
                if ( iTest < 0 )
                {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;       
}

//--------------------------------------------------------------------------------------
// Name: CenterMaskOnEndOfHand()
// Desc: Center the mask on the end of the hand.
//--------------------------------------------------------------------------------------
VOID CenterMaskOnEndOfHand( DepthMaps* __restrict pDMS,
                            RefinementData* __restrict pRefinementData,
                            HandSpecificData* __restrict pHandSpecificData,
                            OptionalHandSpecificData* __restrict pEHSP,
                            INT iFinalThreshold,
                            INT2Vector iNormal )
{
    INT diff, mask;
    INT2Vector iAverage(0,0 );
    INT iCount = 0;
    for ( INT iY = 0; iY < 11; ++iY )
    {
        for ( INT iX = 0; iX < 11; ++iX )
        {
            if ( pEHSP->m_bValid80x60Voxels[iY][iX] )
            {
                iAverage.iX += iX;
                iAverage.iY += iY;
                ++iCount;
            }
        }    
    }
    if ( iCount != 0 )
    {
        iAverage.iX -= ( g_iNuiRefinementKernelCenter80x60 * iCount );
        iAverage.iX *= 20;
        iAverage.iX /= 30;
        iAverage.iX /= iCount;
        iAverage.iX += g_iNuiRefinementKernelCenter80x60;

        iAverage.iY -= ( g_iNuiRefinementKernelCenter80x60 * iCount );
        iAverage.iY *= 20;
        iAverage.iY /= 30;
        iAverage.iY /= iCount;
        iAverage.iY += g_iNuiRefinementKernelCenter80x60;

        GETMIN( iAverage.iX, (g_iNuiRefinementKernelSize80x60-1), iAverage.iX );
        GETMIN( iAverage.iY, (g_iNuiRefinementKernelSize80x60-1), iAverage.iY );
        if ( pEHSP->m_bValid80x60Voxels[iAverage.iY][iAverage.iX] )
        {
            BOOL bValidVoxelsTemp[ g_iNuiRefinementKernelSize80x60 ][ g_iNuiRefinementKernelSize80x60 ];
            memset( bValidVoxelsTemp, 0, sizeof( bValidVoxelsTemp ) );
            INT3Vector iNewTestPosition =  pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60;
            iNewTestPosition.iX += iAverage.iX - g_iNuiRefinementKernelCenter80x60;
            iNewTestPosition.iY += iAverage.iY - g_iNuiRefinementKernelCenter80x60;
            Clamp80x60( &iNewTestPosition.iX, &iNewTestPosition.iY );
            iNewTestPosition.iZ = pDMS->m_pData80x60Min[ iNewTestPosition.iY * pDMS->m_iShortPitch80x60Min
                + iNewTestPosition.iX ] >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

            BuildValidMaskWithSearchSpiral80x60( pDMS, pRefinementData, iNewTestPosition,
                &bValidVoxelsTemp[0][0], iFinalThreshold, -200);

            BOOL bSpill = MaskSpill( &bValidVoxelsTemp[0][0], iNormal );

            if ( !bSpill )
            {
                pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60 = iNewTestPosition;
                memcpy( pEHSP->m_bValid80x60Voxels, bValidVoxelsTemp, sizeof( bValidVoxelsTemp ) );
            }
            //pRefinementData
        }
    }
}

//--------------------------------------------------------------------------------------
// Name: FillInCracksInMask()
// Desc: Those mask entries that are turned off but share more than one valid mask edge
// are turned on.
//--------------------------------------------------------------------------------------
VOID FillInCracksInMask( HandSpecificData* __restrict pHandSpecificData,
                         OptionalHandSpecificData* __restrict pEHSP )
{
    // we're going to fill in the voxel mask a touch.
    INT diff, mask;
    if ( !pHandSpecificData->m_bRevertedToRawData )
    {
        BOOL bValid80x60VoxelsCopy[g_iNuiRefinementKernelSize80x60][g_iNuiRefinementKernelSize80x60];
        memset( bValid80x60VoxelsCopy, 0, sizeof( bValid80x60VoxelsCopy ) );

        for ( INT iY = 0; iY < 11; ++iY )
        {
            for ( INT iX = 0; iX < 11; ++iX )
            {
                if ( pEHSP->m_bValid80x60Voxels[iY][iX] )
                {
                    bValid80x60VoxelsCopy[iY][iX] = TRUE;   
                }
                else 
                {
                    BOOL iNeighborCount = 0;
                    INT iXMinus1 = iX-1;
                    INT iYMinus1= iY -1;
                    GETMAX( iXMinus1, iXMinus1, 0 ); 
                    GETMAX( iYMinus1, iYMinus1, 0 ); 
                    
                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iYMinus1][iXMinus1];
                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iYMinus1][iX];
                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iYMinus1][iX+1];

                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iY][iXMinus1];
                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iY][iX+1];

                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iY+1][iXMinus1];
                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iY+1][iX];
                    iNeighborCount+= pEHSP->m_bValid80x60Voxels[iY+1][iX+1];
                    if ( iNeighborCount > 1 ) 
                    {
                        bValid80x60VoxelsCopy[iY][iX] = TRUE;   
                    }
                }
            }    
        }
        memcpy( pEHSP->m_bValid80x60Voxels, bValid80x60VoxelsCopy, sizeof( bValid80x60VoxelsCopy ) );
    }
}    

//--------------------------------------------------------------------------------------
// Name: BuildValidMaskAndCenter()
// Desc: Iterativly build the mask.  Attempt to center it.  Fill in the cracks in the 
// mask.
//--------------------------------------------------------------------------------------
VOID BuildValidMaskAndCenter ( DepthMaps* __restrict pDMS,
                                    RefinementData* __restrict pRefinementData,
                                    HandSpecificData* __restrict pHandSpecificData,
                                    OptionalHandSpecificData* __restrict pEHSP )
{
    if ( pHandSpecificData->m_bRevertedToRawData ) return;
    
    // Build the first mask, iterate over the potential thresholds untill we find one that doesn't spill
////////////////////////////////////
    INT iFinalThreshold = g_iBuildMaskThresholds[0];
    INT2Vector iNormal( pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iX - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iX,
        pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iY - pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60.iY );

    BOOL bSpillDetected = TRUE;
    // stay on point we settled on if we risk jumping into the weeds.
    static INT nMaskThresholds = sizeof(g_iBuildMaskThresholds) / sizeof( INT );
    for ( INT iThesholdIndex = 0; iThesholdIndex < nMaskThresholds && bSpillDetected; ++iThesholdIndex )
    {
        INT iSize = sizeof(BOOL) * g_iNuiRefinementKernelSize80x60 * g_iNuiRefinementKernelSize80x60;
        memset( pEHSP->m_bValid80x60Voxels, 0, iSize );

        BuildValidMaskWithSearchSpiral80x60( pDMS, pRefinementData, 
            pHandSpecificData->m_INT3ScreenSpaceHandRefined80x60,
            &pEHSP->m_bValid80x60Voxels[0][0], g_iBuildMaskThresholds[iThesholdIndex], -200 );//-iThresholds[iThesholdIndex] );

        bSpillDetected  = MaskSpill( &pEHSP->m_bValid80x60Voxels[0][0], iNormal );
        iFinalThreshold = g_iBuildMaskThresholds[iThesholdIndex];
    
    }
    if ( bSpillDetected ) pHandSpecificData->m_bRevertedToRawData = TRUE;
//////////////////////////

    INT iDirection;

    CalculateSearchDirection( pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iX, 
        pHandSpecificData->m_INT3ScreenSpaceHandRaw80x60.iY, 
        pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iX, 
        pHandSpecificData->m_INT3ScreenSpaceElbow80x60.iY, &iDirection );

    BOOL bMaskGoodEnoughToCenter;
    bMaskGoodEnoughToCenter = MaskTooCloseToBodyToCenter( iDirection, pDMS, pRefinementData, pHandSpecificData, 
            &pEHSP->m_bValid80x60Voxels[0][0] );

    // if the Mask is Suficient quality, center it on the end of the hand.
    if ( !pHandSpecificData->m_bRevertedToRawData && bMaskGoodEnoughToCenter && iFinalThreshold != 30 )
    {
        CenterMaskOnEndOfHand( pDMS, pRefinementData, pHandSpecificData, pEHSP, iFinalThreshold, iNormal );
    }

    FillInCracksInMask( pHandSpecificData, pEHSP );
}

//--------------------------------------------------------------------------------------
// Name: CenterPointOnHandAverageSearch
// Desc: Find the hand inthe 80x60 map
//--------------------------------------------------------------------------------------
VOID Search80x60HandForTheEndOfHand ( DepthMaps* __restrict pDMS,
                                      RefinementData* __restrict pRefinementData,
                                      HandSpecificData* __restrict pHandSpecificData,
                                      OptionalHandSpecificData* __restrict pEHSP )
{
    ClimbOntoHand80x60( pDMS, pRefinementData, pHandSpecificData );
    if ( pHandSpecificData->m_bRevertedToRawData )
    {
        return;
    }

    PIXBeginNamedEvent( 0, "Walk to End of Hand" );
    WalkToEndofHand80x60( pDMS, pRefinementData, pHandSpecificData, pEHSP );
    PIXEndNamedEvent();
    if ( pHandSpecificData->m_bRevertedToRawData )
    {
        return;
    }

    BuildValidMaskAndCenter( pDMS, pRefinementData, pHandSpecificData, pEHSP );
    
}

// We really need to test the tracking state of the joint, but we don't want to break the public API in
// approved libs, so simply check for (0,0,0,0)
__forceinline BOOL IsJointValid( XMVECTOR vJoint )
{
    return XMVector4NotEqual( vJoint, XMVectorZero() );
}

//--------------------------------------------------------------------------------------
// Name: RefineHandsCore()
// Desc: Internal function that refine hands calls to do hand refinement
//--------------------------------------------------------------------------------------
HRESULT RefineHandsCore(_In_     USHORT*                  __restrict pDepth80x60Min,
                        _In_     INT                                 iShortPitch80x60Min,
                        _In_     USHORT*                  __restrict pDepth320x240,
                        _In_     INT                                 iShortPitch320x240,
                        _In_     INT                                 iSegmentationIndex,
                        _In_     const XMVECTOR                      pSkeletonJoints[NUI_SKELETON_POSITION_COUNT],
                        _In_     DWORD                               dwFlags,
                        _Inout_  RefinementData*          __restrict pRefinementData, 
                        _Out_opt_ OptionalRefinementData* __restrict pOptionalRefinementData
                        )
{
    OptionalRefinementData StackAllocatedOptionalRefinmentData;
    // some validation of parameters

    if ( dwFlags & ATG_REFINE_HANDS_FLAG_CALCULATE320x240MASK && pOptionalRefinementData == NULL )
    {
        return E_INVALIDARG;
    }

    if ( pOptionalRefinementData == NULL )
    {
        pOptionalRefinementData = &StackAllocatedOptionalRefinmentData;
    }
    pOptionalRefinementData->m_dwFlags = dwFlags;
    DepthMaps dms;
    dms.m_pData320x240 = pDepth320x240;
    dms.m_iShortPitch320x240 = iShortPitch320x240;
    dms.m_pData80x60Min = pDepth80x60Min;
    dms.m_iShortPitch80x60Min = iShortPitch80x60Min;
    
    InitUnrolledNeighborSearchSpiral( iShortPitch80x60Min );
    pRefinementData->m_uSegmentationIndex = (USHORT) iSegmentationIndex; 

    __analysis_assume( sizeof(pSkeletonJoints) ==  NUI_SKELETON_POSITION_COUNT * 16 );

    XMVECTOR vSpine = pSkeletonJoints[NUI_SKELETON_POSITION_SPINE];
    XMVECTOR vHead  = pSkeletonJoints[NUI_SKELETON_POSITION_HEAD];
    TransformWorldToScreen80x60( vSpine, &pRefinementData->m_INT3ScreenSpaceSpine80x60 );
    TransformWorldToScreen80x60( vHead, &pRefinementData->m_INT3ScreenSpaceHead80x60 );
    INT3Vector iDist = pRefinementData->m_INT3ScreenSpaceSpine80x60;
    iDist.iX -= pRefinementData->m_INT3ScreenSpaceHead80x60.iX;
    iDist.iY -= pRefinementData->m_INT3ScreenSpaceHead80x60.iY;

    iDist.iX *= iDist.iX;
    iDist.iY *= iDist.iY;

    FLOAT fdist = sqrtf( (FLOAT)(iDist.iX + iDist.iY ) );
    pRefinementData->m_fHeadToSpine80x60 = fdist;


    if ( pRefinementData->m_fHeadToSpine80x60 <= 13.0f )
    {
        pRefinementData->m_iSpillDistanceSquared = 25;
    }
    else 
    {
        pRefinementData->m_iSpillDistanceSquared = 82;
    }

    if ( IsJointValid( vSpine ) && IsJointValid( vHead ) )
    {
        XMVECTOR vHeadToSpine = vHead - vSpine;
        FLOAT fLength = XMVectorGetX( XMVector3Length( vHeadToSpine ) );

        if ( pRefinementData->m_fHeadToSpine == 0.0f )
        {
            pRefinementData->m_fHeadToSpine = fLength;
        }
        else 
        {
            pRefinementData->m_fHeadToSpine = 0.8f * pRefinementData->m_fHeadToSpine + 0.2f * fLength;
        }
    }

    // Check if right hand is valid
    XMVECTOR vHand  = pSkeletonJoints[ NUI_SKELETON_POSITION_HAND_RIGHT ];
    XMVECTOR vElbow = pSkeletonJoints[ NUI_SKELETON_POSITION_ELBOW_RIGHT ];
    if ( IsJointValid( vHand ) &&
         IsJointValid( vElbow ) )
    {
        InitializeHand( NUI_SKELETON_POSITION_HAND_RIGHT, NUI_SKELETON_POSITION_ELBOW_RIGHT, pRefinementData,
                        pSkeletonJoints, &pRefinementData->m_RightHandData, &pOptionalRefinementData->m_RightHand );

        PIXBeginNamedEvent( 0, "Refine80x60search Right" );
            Search80x60HandForTheEndOfHand( &dms, pRefinementData, &pRefinementData->m_RightHandData, 
                &pOptionalRefinementData->m_RightHand );
        PIXEndNamedEvent( );

        PIXBeginNamedEvent( 0, "Refine320x240search Right" );
            RefineWith320x240( &dms,
                pRefinementData, &pRefinementData->m_RightHandData, pOptionalRefinementData,
                &pOptionalRefinementData->m_RightHand );
        PIXEndNamedEvent( );
    }
 
    // Check if left hand is valid
    vHand  = pSkeletonJoints[ NUI_SKELETON_POSITION_HAND_LEFT ];
    vElbow = pSkeletonJoints[ NUI_SKELETON_POSITION_ELBOW_LEFT ];
    if ( IsJointValid( vHand ) &&
         IsJointValid( vElbow ) )
    {
        InitializeHand( NUI_SKELETON_POSITION_HAND_LEFT, NUI_SKELETON_POSITION_ELBOW_LEFT, pRefinementData,
                        pSkeletonJoints, &pRefinementData->m_LeftHandData, &pOptionalRefinementData->m_LeftHand);

        PIXBeginNamedEvent( 0, "Refine80x60search Left" );
            Search80x60HandForTheEndOfHand( &dms, pRefinementData, &pRefinementData->m_LeftHandData, 
                &pOptionalRefinementData->m_LeftHand );
        PIXEndNamedEvent( );

        PIXBeginNamedEvent( 0, "Refine320x240search Left" );
            RefineWith320x240( &dms,
                pRefinementData, &pRefinementData->m_LeftHandData, pOptionalRefinementData,
                &pOptionalRefinementData->m_LeftHand );
        PIXEndNamedEvent( );
    }

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: RefineHands
// Desc: Uses the 80x60 map to find the hand and then refines with the 320x240 map.
//--------------------------------------------------------------------------------------
HRESULT RefineHands (   _In_        LPDIRECT3DTEXTURE9                      pDepthAndSegmentationTexture320x240,
                        _In_        LPDIRECT3DTEXTURE9                      pDepthAndSegmentationTexture80x60,
                        _In_        INT                                     iSkeletonDataSlot,
                        _In_        const NUI_SKELETON_FRAME*   __restrict pSkeletonFrame,
                        _In_        DWORD                                   dwFlags,
                        _Inout_     RefinementData*             __restrict pRefinementData,
                        _In_opt_    const D3DLOCKED_RECT*       __restrict pDepthRect320x240,
                        _In_opt_    const D3DLOCKED_RECT*       __restrict pDepthRect80x60,
                        _Out_opt_   OptionalRefinementData*        __restrict pOptionalRefinementData
                 )
{
    if ( pDepthAndSegmentationTexture320x240 == NULL ||
         pDepthAndSegmentationTexture80x60 == NULL ||
         iSkeletonDataSlot < 0 ||
         iSkeletonDataSlot >= NUI_SKELETON_COUNT )
    {
        return E_INVALIDARG;
    }


    D3DLOCKED_RECT Rect320x240;
    D3DLOCKED_RECT Rect80x60;

    if ( pDepthRect320x240 == NULL )
    {
        pDepthAndSegmentationTexture320x240->LockRect( 0, &Rect320x240, 0, D3DLOCK_READONLY );
        pDepthRect320x240 = &Rect320x240;
    }
    if ( pDepthRect80x60 == NULL )
    {
        pDepthAndSegmentationTexture80x60->LockRect( 0, &Rect80x60, 0, D3DLOCK_READONLY );
        pDepthRect80x60 = &Rect80x60;
    }
    if ( pDepthRect320x240->Pitch < (320 * 2) || ( pDepthRect80x60->Pitch < (80 * 2) ) )
    {
        return E_INVALIDARG;
    }

    USHORT *pDepth80x60 = (USHORT *)pDepthRect80x60->pBits;
    INT iShortPitch80x60 = pDepthRect80x60->Pitch / sizeof(USHORT);
    USHORT *pDepth320x240 = (USHORT *)pDepthRect320x240->pBits;
    INT iShortPitch320x240 = pDepthRect320x240->Pitch / sizeof(USHORT);

    if ( iShortPitch80x60 < 0 ||
         iShortPitch320x240 < 0 ||
         pDepth80x60 == NULL ||
         pDepth320x240 == NULL )
    {
        pDepthAndSegmentationTexture320x240->UnlockRect( 0 );
        pDepthAndSegmentationTexture80x60->UnlockRect( 0 );
        return E_FAIL;
    }

    // Reset when the tracking changed.
    DWORD dwCurrentTrackingID = pSkeletonFrame->SkeletonData[iSkeletonDataSlot].dwTrackingID;
    if ( pRefinementData->m_dwLastTrackingID != dwCurrentTrackingID )
    {
        pRefinementData->Reset();
        pRefinementData->m_dwLastTrackingID = dwCurrentTrackingID;
    }

    PIXBeginNamedEvent( 0, "Refine Both Hands Core" );
    HRESULT hr = RefineHandsCore( pDepth80x60,
        iShortPitch80x60,
        pDepth320x240,
        iShortPitch320x240,
        iSkeletonDataSlot + 1, // segmentation index is SkeletonData Slot + 1
        pSkeletonFrame->SkeletonData[iSkeletonDataSlot].SkeletonPositions,
        dwFlags,
        pRefinementData,
        pOptionalRefinementData 
    );
    PIXEndNamedEvent( );

    if ( pDepthRect320x240 != NULL )
    {
        pDepthAndSegmentationTexture320x240->UnlockRect( 0 );
    }
    if ( pDepthRect80x60 != NULL )
    {
        pDepthAndSegmentationTexture80x60->UnlockRect( 0 );
    }
    pRefinementData->m_bFirstTimeThrough = FALSE;
    return hr;
}

// ********************************************************************************************************* //
// ********************************************************************************************************* //

}//namespace NuiHandRefinement

