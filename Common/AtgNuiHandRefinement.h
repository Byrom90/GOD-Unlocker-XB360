// ********************************************************************************************************* //
// --------------------------------------------------------------------------------------------------------- //
//
// Copyright (c) Microsoft Corporation
//
// --------------------------------------------------------------------------------------------------------- //
// ********************************************************************************************************* //

#ifndef _ATG_NUI_HAND_REFINEMENT_H
#define _ATG_NUI_HAND_REFINEMENT_H

//#define DEBUG_COMPUTE_AND_SHOW_VISUALIZATION 1
// When this is defined hand refinement calculates expensive debug information.
// The HandRefinement sample uses this for visualziation.


// ********************************************************************************************************* //
//
//  Namespace:
//

#include <nuiapi.h>

namespace ATG
{

static CONST INT g_iNuiRefinementKernelSize320x240 = 48;  // The size of the mask that we use to find the average hand value
static CONST INT g_iNuiRefinementHalfKernelSize320x240 = 24; // Half the size of the above mask
static CONST FLOAT g_fNuiRefinementHalfKernelSize320x240 = 24.0f; // Half the size of the above mask
static CONST INT g_iNuiRefinementKernelSize80x60 = 12;  // Each 4x4 block in the 320x240 mask maps to one vlaue in the 80x60 mask
static CONST INT g_iNuiRefinementKernelCenter80x60 = 5; // The 5,5 location is the actual center of the kernel
static CONST INT g_iNuiRefinementKernelMaxSize80x60 = 6; // The biggest possible 80x60 radius


#define ATG_REFINE_HANDS_FLAG_CALCULATE320x240MASK                  0x00000001
// This flag tells us to fill in the user provided pointer with a mask of the 320x240 values used in hand refinement

//--------------------------------------------------------------------------------------
// Trivial struct to wrap up 3 INTs so I don't have to use arrays
//--------------------------------------------------------------------------------------
// TODO the February XDK has an XMINT3 that I could use instead of this class.  
// This should be used when the code is ready for this dependency.
struct INT3Vector
{
    INT3Vector() {};
    INT3Vector( INT iInX, INT iInY, INT iInZ )
    {
        iX = iInX;
        iY = iInY;
        iZ = iInZ;
    };
    INT iX, iY, iZ;
};

//--------------------------------------------------------------------------------------
// Trivial struct to wrap up 2 INTs so I don't have to use arrays
//--------------------------------------------------------------------------------------
struct INT2Vector
{
    INT2Vector() {};
    INT2Vector( INT iInX, INT iInY )
    {
        iX = iInX;
        iY = iInY;
    };
    INT iX, iY;
};

//--------------------------------------------------------------------------------------
// Trivial struct to wrap a range of ints so I don't need 2 variables for a min and max
//--------------------------------------------------------------------------------------
struct INT2Range
{
    INT2Range(){};
    INT2Range( INT iInMin, INT iInMax){
        iMin = iInMin;
        iMax = iInMax;
    };
    INT iMin, iMax;
};

#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION

//--------------------------------------------------------------------------------------
// Debug data used by apps to visualize the pixels that were used in the final hand 
// average.
//--------------------------------------------------------------------------------------
class VisualizeHandFramesData {
public:
    CHAR m_FrameData[ g_iNuiRefinementKernelSize320x240 ][ g_iNuiRefinementKernelSize320x240 ];
};
#endif

//--------------------------------------------------------------------------------------
// Keeps intermediate data for one hand.  
//--------------------------------------------------------------------------------------
struct HandSpecificData
{
    HandSpecificData() 
    {
        Reset();
    };
    ~HandSpecificData(){};

    VOID Reset()
    {
        m_vRawHand = XMVectorZero();
        m_vRefinedHand = XMVectorZero();
        m_vRawElbow = XMVectorZero();
        m_vOffsetRawToRefined = XMVectorZero();
        m_INT3ScreenSpaceHandRaw80x60.iX = 0;
        m_INT3ScreenSpaceHandRaw80x60.iY = 0;
        m_INT3ScreenSpaceHandRaw80x60.iZ = 0;
        m_INT3ScreenSpaceHandRefined80x60.iX = 0;
        m_INT3ScreenSpaceHandRefined80x60.iY = 0;
        m_INT3ScreenSpaceHandRefined80x60.iZ = 0;
        m_INT3ScreenSpaceElbow80x60.iX = 0;
        m_INT3ScreenSpaceElbow80x60.iY = 0;
        m_INT3ScreenSpaceElbow80x60.iZ = 0;
        m_bRevertedToRawData = FALSE;
        m_bRevertedToRawDataPreviousFrame = FALSE;
        m_eHand = (NUI_SKELETON_POSITION_INDEX)0; 
        m_iOffsetForHandCloseToBody = 0;
#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
        memset( &m_VisualizeHandFramesData, 0, sizeof( VisualizeHandFramesData ) );
#endif
    };
    
    XMVECTOR m_vRawHand;                                            // The hand that was received from the Raw skeleton.
    XMVECTOR m_vRefinedHand;                                        // The refined hand that will be returned to the user.
    XMVECTOR m_vRawElbow;                                           // The Elbow that correlates to the hand. i.e. right hand uses right elbow
    XMVECTOR m_vOffsetRawToRefined;                                 // An Offset that is set to interpolate between raw and refined data when we must fall back to raw data

    INT3Vector m_INT3ScreenSpaceHandRaw80x60;                         // The Raw hand in screen space. 
    INT3Vector m_INT3ScreenSpaceElbow80x60;                           // The Raw elbow in screen space.
    INT3Vector m_INT3ScreenSpaceHandRefined80x60;                     // The refined hand position in 80x60 space.

    NUI_SKELETON_POSITION_INDEX m_eHand;                              // NUI_SKELETON_POSITION_INDEX
    SHORT m_iOffsetForHandCloseToBody;                              // This is calculated when we determine the voxel mask.  It's used in the 320x240 to position the collision ellipsoid.

    BOOL m_bRevertedToRawData;                                      // Set this to true when we need to fall back to raw data
    BOOL m_bRevertedToRawDataPreviousFrame;                         // True when we fell back to raw data on the previous frame

#ifdef DEBUG_COMPUTE_AND_SHOW_VISUALIZATION
    INT3Vector m_INT3ScreenSpaceHandJumpOnArm80x60;                   // The hand after jumping onto the arm.
    INT3Vector m_INT3ScreenSpaceHandWalkToEndofArm80x60;              // The hand after walking to the end of the arm.
    INT3Vector m_INT3ScreenSpaceHandCenterAroundEndofArm80x60;        // The hand after centering on the arm.
    VisualizeHandFramesData m_VisualizeHandFramesData;        
#endif

};

//--------------------------------------------------------------------------------------
// This struct can optionally be passed in by apps that want to leverage some of the
// calculations performed by hand refinement.  The Hand Control samples uses this data
// to calculate the orientation of the hand mask.
//--------------------------------------------------------------------------------------
struct OptionalHandSpecificData
{
    OptionalHandSpecificData()
    {
        m_iVoxelSearchRadius = 0;
        m_iSquaredSizeOfHandTheshold = 0;
        memset( m_bValid80x60Voxels, 0, sizeof( m_bValid80x60Voxels ) );
        memset( m_iSpillDirectionSquaredLength, 0, sizeof( m_iSpillDirectionSquaredLength ) );
        memset( &m_SearchSphereCenter320x240, 0, sizeof( m_SearchSphereCenter320x240 ) );
        memset( &m_uHandDepthValues320x240, 0, sizeof( m_uHandDepthValues320x240 ) );
    };

    ~OptionalHandSpecificData(){};

    USHORT m_uHandDepthValues320x240[g_iNuiRefinementKernelSize320x240*g_iNuiRefinementKernelSize320x240];
    BOOL m_bValid80x60Voxels[g_iNuiRefinementKernelSize80x60][g_iNuiRefinementKernelSize80x60];
    INT m_iVoxelSearchRadius;
    INT m_iSquaredSizeOfHandTheshold;
    INT3Vector m_SearchSphereCenter320x240;
    INT m_iSpillDirectionSquaredLength[8];

};

//--------------------------------------------------------------------------------------
// This struct can optionally be passed in by apps that want to leverage some of the
// calculations performed by hand refinement.  The Hand Control samples uses this data
// to calculate the orientation of hte hand mask.
//--------------------------------------------------------------------------------------
struct OptionalRefinementData
{
public:
    OptionalRefinementData(): m_dwFlags( 0 ) {};
    OptionalHandSpecificData m_RightHand;
    OptionalHandSpecificData m_LeftHand;
    DWORD m_dwFlags;
};

//--------------------------------------------------------------------------------------
// This is the main hand refinement structure.  It stores some intermediate data as well 
// as the final refined hands.
//--------------------------------------------------------------------------------------
class RefinementData
{
public:
    RefinementData ()
    {
        Reset();
    };
    VOID Reset()
    {
        m_dwLastTrackingID = 0;
        m_LeftHandData.Reset();
        m_RightHandData.Reset();
        m_uSegmentationIndex = 0;
        m_fHeadToSpine = 0.0f;
        m_fHeadToSpine80x60 = 0.0f;
        m_iSpillDistanceSquared = 0; 
        m_INT3ScreenSpaceSpine80x60.iX = 0;
        m_INT3ScreenSpaceSpine80x60.iY = 0;
        m_INT3ScreenSpaceSpine80x60.iZ = 0;
        m_INT3ScreenSpaceHead80x60.iX = 0;
        m_INT3ScreenSpaceHead80x60.iY = 0;
        m_INT3ScreenSpaceHead80x60.iZ = 0;
        m_bFirstTimeThrough = TRUE;
    }
    XMVECTOR GetRefinedRight()
    {
        return m_RightHandData.m_vRefinedHand;
    }
    XMVECTOR GetRefinedLeft()
    {
        return m_LeftHandData.m_vRefinedHand;
    }
    ~RefinementData(){};

    DWORD m_dwLastTrackingID;
    HandSpecificData m_LeftHandData;
    HandSpecificData m_RightHandData;
    FLOAT m_fHeadToSpine80x60;
    FLOAT m_fHeadToSpine;
    INT3Vector m_INT3ScreenSpaceSpine80x60;                     
    INT3Vector m_INT3ScreenSpaceHead80x60;                     
    INT m_iSpillDistanceSquared; // The number of voxels we can walk and not spill.  
                                 // This value is squared to run faster.
    USHORT m_uSegmentationIndex;
    BOOL m_bFirstTimeThrough;

};

//--------------------------------------------------------------------------------------
// Name: RefineHands
// Desc: Uses the 80x60 map to find the hand and then refines with the 320x240 map.
//--------------------------------------------------------------------------------------
HRESULT RefineHands (  
                    _In_        LPDIRECT3DTEXTURE9                      pDepthAndSegmentationTexture320x240,
                    _In_        LPDIRECT3DTEXTURE9                      pDepthAndSegmentationTexture80x60,
                    _In_        INT                                     iSkeletonDataSlot,
                    _In_        const NUI_SKELETON_FRAME*    __restrict pSkeletonFrame,
                    _In_        DWORD                                   dwFlags,
                    _Inout_     RefinementData*              __restrict pRefinementData,
                    _In_opt_    const D3DLOCKED_RECT*        __restrict pDepthRect320x240,
                    _In_opt_    const D3DLOCKED_RECT*        __restrict pDepthRect80x60,
                    _Out_opt_   OptionalRefinementData*      __restrict pOptionalRefinementData
                 );


// ********************************************************************************************************* //
// ********************************************************************************************************* //

}//namespace NuiHandRefinement

// --------------------------------------------------------------------------------------------------------- //

#endif//__HANDREFINEMENTINTERNAL_H__

