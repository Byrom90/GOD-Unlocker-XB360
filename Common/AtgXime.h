//--------------------------------------------------------------------------------------
// AtgXime.h
//
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#ifndef ATGXIME_H
#define ATGXIME_H

#include <xui.h>
#include <xime.h>
#include <list>
#include <string>
#include "AtgXmlParser.h"

//#define DEBUG_USER_DIC_TEST               // For testing purpose to not delete the contents of user dictionray when ending Japanese
//#define DEBUG_FLUSH_LEARNING              // For testing purpose to flush all learning words at once
//#define DEBUG_DELETE_SINGLE_REG_WORD      // For testing purpose to delete 1 registered word 

namespace ATG
{

//--------------------------------------------------------------------------------------
// Definitions
//--------------------------------------------------------------------------------------

    const D3DCOLOR COLOR_NORMAL = D3DCOLOR_ARGB( 255, 255, 255, 255 );
    const D3DCOLOR COLOR_INIME  = D3DCOLOR_ARGB( 255, 128, 128, 128 );
    const D3DCOLOR COLOR_LIST   = D3DCOLOR_ARGB( 255, 128, 128, 128 );
    const D3DCOLOR COLOR_FOCUS  = D3DCOLOR_ARGB( 255, 255, 255, 0 );

    enum IMEMODE {
        MODE_INPUT      = 0x1,
        MODE_CONVERT    = 0x2,
        MODE_ALLCONVERT = 0x4,
        MODE_OFF        = 0x8, 
        MODE_JP         = 0x10,
        MODE_KR         = 0x20,
        MODE_TC         = 0x40, 
        MODE_SC         = 0x80, 

        MODE_JKT        = ( MODE_JP | MODE_KR | MODE_TC ),
        MODE_JKTS       = ( MODE_JP | MODE_KR | MODE_TC | MODE_SC)

    };

    class   Xime;
    struct  XIME_KEYMAPPING;
    typedef HRESULT (*IMECALLBACK)( Xime *, XIME_KEYMAPPING * pKey ) ;

    struct XIME_KEYMAPPING{
        DWORD   dwFlags;
        WORD    wVK;
        WORD    wXinputFlags;
        WORD    wIgnoredXinputFlags;
        union {
            DWORD dwUnicode;
            XIMEKEY eControlKey;
        };
        IMECALLBACK     function;
    };

    struct XIMEREQUEST{
        DWORD dwRequestId;
        XOVERLAPPED ov;
        XINPUT_KEYSTROKE key;
    };
    const DWORD NEED_TO_DISABLE_LIST = 1; // magic number; for JP's SetCandidate()
    const DWORD NEED_TO_COMPLETE = 2;     // magic number; for CHT's Complete()

typedef std::list< ATG::XIMEREQUEST > RequestList;

struct HID_LOOKUP_TABLE {
        WCHAR wHiragana;
        WCHAR wHiraganaShifted;
        WCHAR wKatakana;
        WCHAR wKatakanaShifted;
};

struct XIMEPOS_LOOKUP_TABLE{
    XIMEPOS value;
    LPCWSTR string;
};

    const UINT  XIME_DIC_ENTRY_WORD_LENGTH = 16;

class Xime
{
public:
    static BOOL g_bIsStaticXime;
    static XIME_KEYMAPPING m_InputKeymap[];
    static BOOL m_bLoadTitleDictionary;
    DWORD  m_dwUserIndex;

    // XIME methods
    BOOL    IsIMEOn( void ) { return m_bIMEOn || m_bCompleted || m_dwCharsInXime != 0; };
    HRESULT SetInputLanguage(IMEMODE eLanguage);
    HRESULT Init( IMEMODE eLanguage, DWORD dwCharLength, D3DPRESENT_PARAMETERS * pD3DParam );
    HRESULT Update();
    HRESULT Render( HXUIDC hDC, HXUIFONT hFont, float fPosX, float fPofY );
    BOOL    IsStringReady( void );
    HRESULT GetString( std::wstring & stringOut );
    // Get IME modes
    INT     GetInputMode( void ) { return m_iInputMode; };
    INT     GetKeyboardLayout( void ) { return m_iKeyboardLayout; };
    IMEMODE GetCurrentLanguage( void ) { return m_CurrentLanguage; };
    HRESULT GetLastKey( XINPUT_KEYSTROKE * pKey ) { XINPUT_KEYSTROKE *pSrcKey;
                                                    if( IsIMEOn() )
                                                    {
                                                        if( m_bCompleted )
                                                        {
                                                            memset( pKey, 0, sizeof(XINPUT_KEYSTROKE) );
                                                            return S_OK;
                                                        }
                                                        pSrcKey = &m_LastKey;
                                                    }
                                                    else
                                                        pSrcKey = &m_Key;
                                                    memcpy( pKey, pSrcKey, sizeof(XINPUT_KEYSTROKE) );
                                                    memset( pSrcKey, 0, sizeof(XINPUT_KEYSTROKE) );
                                                    return S_OK;
                                                  };
    DWORD   GetInputCharacterLength( void ) { return m_dwInputCharacterLength; };
    VOID    SetInputCharacterLength( DWORD dwLength ) { m_dwInputCharacterLength = dwLength; m_bUpdateInputLength = TRUE; };
    VOID    EnumerateDictionarySummary();
    static  HRESULT SwitchLanguage(Xime *pXime);
    static  HRESULT ToggleIME( Xime * pXime );

    HRESULT LoadJPUserWordFile(const CHAR* strUserWordFile);
    static  HRESULT RegisterJPUserWord(LPCWSTR pReading, LPCWSTR pDisplay, LPCWSTR pImePos);

    HRESULT DeleteJPUserWordAll();
    UINT    GetJPUserWordNumber(){ return m_JPUserWord; };

private:
    // Dictionaries
    static const int   iNumDicsJP;
    static const int   iNumDicsTC;
    static const char  szMainDicPath[];
    static const char  szSubDicPath[];
    static const char  szTitleDicPath[];
    static const char  szXEXTitleDicPathFileLocator[];
    static const char  szXEXTitleDicPath[];
    static const char  szBopomofoDicPath[];
    
    static const INT NUM_STRINGBUFFER =512;
    static const INT NUM_CANDIATEINPAGE = 5;
    
    static const DWORD DEFAULT_CHARACTERLENGTH = 32;

    static XOVERLAPPED s_imeOverLapped;

    static WCHAR  m_StringBuffer[ NUM_STRINGBUFFER ];
    static WCHAR  m_CandidateListBuffer[ NUM_STRINGBUFFER ];
    
    static FILE   *fpUserDic;
    static char   m_UserDicMemory[ XEIME_JP_SIZE_OF_USERDIC ];

    //Callbacks
    static HRESULT Convert( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT NonConvert( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT ChangeFocus( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT ChangeClauseLength( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT Revert( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT BuildCandidateList( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT SetCandidate( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT Complete( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT HandleNumpadInput( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT HandleNumpadConversion( Xime * pXime, XIME_KEYMAPPING * pKey );

    static HRESULT AllConvert( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT AllRevert( Xime * pXime, XIME_KEYMAPPING * pKey );
    static HRESULT ChangeInputMode( Xime * pXime, XIME_KEYMAPPING * pKey );

    //Japanese specific routines
    static HRESULT ToggleKeyboardLayout( Xime * pXime, XIME_KEYMAPPING * pKey );

    HRESULT LayoutConversion( XINPUT_KEYSTROKE* pKey, INT* pIntputMode );

    // Bopomofo specific routines
    static HRESULT NothingToDo( Xime * pXime, XIME_KEYMAPPING * pKey ) { return ERROR_SUCCESS; }

    void    DrawText( HXUIDC hdc, HXUIFONT hFont, D3DCOLOR color,
                      float x, float y, LPCWSTR text,
                      float * fX = NULL, float * fY = NULL );
    void    SetCurrentMode( IMEMODE );

    // Language specific routine for non ALPHANUMERIC layout
    VOID    HidUnicodeLookUp( XINPUT_KEYSTROKE* pKey, BOOL fImeOn, BOOL fKana,
                              HID_LOOKUP_TABLE * pTable, HID_LOOKUP_TABLE * pTable2 );
    
    static  XIMEPOS XimeposLookup(LPCWSTR pStr);
    // block function, shouldn't be called every frame
    void    UpdateJPUserWordNumber(); 

    RequestList         m_RequestList;
    IMEMODE             m_CurrentImeMode;
    IMEMODE             m_CurrentLanguage;
    BOOL                m_bShowCandidateList;
    BOOL                m_bNeedBlocking;
    
    float               m_BackBufferWidth;
    float               m_BackBufferHeight;
    BOOL                m_bCompleted;
    DWORD               m_dwCharsInXime;
    INT                 m_iInputMode;
    INT                 m_iKeyboardLayout;
    BOOL                m_bIMEOn;
    DWORD               m_dwInputCharacterLength;
    BOOL                m_bUpdateInputLength;
    BOOL                m_bGotClause;

    XINPUT_KEYSTROKE    m_Key;
    XINPUT_KEYSTROKE    m_LastKey;

    std::wstring        m_CurrentString;
    std::wstring        m_CompletedString;
    UINT                m_JPUserWord;
};



class UserWordFileCallback : public ISAXCallback
{
public:
    virtual HRESULT  StartDocument() { return S_OK; };
    virtual HRESULT  EndDocument() { return S_OK; };

    virtual HRESULT  ElementBegin( CONST WCHAR* strName, UINT NameLen, CONST XMLAttribute *pAttributes, UINT NumAttributes )
    {
        WCHAR wAttName[32] = L"";
        WCHAR wReading[32] = L"";
        WCHAR wDisplay[32] = L"";;
        WCHAR wXIMEPOS[32] = L"";;

        if (NameLen >31)
            return S_FALSE;
        else
            wcsncpy_s( wAttName, strName, NameLen);
        
        if  ( _wcsicmp(wAttName,L"USERWORD") == 0)
        {
            return S_OK;
        }
        else if ( _wcsicmp(wAttName,L"ENTRY") == 0)
        {
            for(UINT i = 0; i < NumAttributes; i++)
            {
                wcsncpy_s( wAttName, pAttributes[i].strName, pAttributes[i].NameLen);
                if (_wcsicmp(wAttName,L"READING")==0)
                {
                    if (pAttributes[i].ValueLen <= XIME_DIC_ENTRY_WORD_LENGTH)
                        wcsncpy_s( wReading, pAttributes[i].strValue, pAttributes[i].ValueLen);
                }
                else if (_wcsicmp(wAttName,L"DISPLAY")==0)
                {
                    if (pAttributes[i].ValueLen <= XIME_DIC_ENTRY_WORD_LENGTH)
                        wcsncpy_s( wDisplay, pAttributes[i].strValue, pAttributes[i].ValueLen);
                }
                else if (_wcsicmp(wAttName,L"XIMEPOS")==0)
                {
                    if (pAttributes[i].ValueLen <= 31)
                        wcsncpy_s( wXIMEPOS, pAttributes[i].strValue, pAttributes[i].ValueLen);
                }
            }
            return Xime::RegisterJPUserWord(wReading , wDisplay , wXIMEPOS);
        }
        else
        {
            return S_FALSE;
        }
    };
    
    virtual HRESULT  ElementContent( CONST WCHAR *strData, UINT DataLen, BOOL More ) {    return S_OK;   };

    virtual HRESULT  ElementEnd( CONST WCHAR *strName, UINT NameLen ){       return S_OK;    };

    virtual HRESULT  CDATABegin( )  { return S_OK; };

    virtual HRESULT  CDATAData( CONST WCHAR *strCDATA, UINT CDATALen, BOOL bMore ){ return S_OK; };

    virtual HRESULT  CDATAEnd( ){ return S_OK; };

    virtual VOID     Error( HRESULT hError, CONST CHAR *strMessage )    {     OutputDebugString("Error when Parsing user word XML\n");    };

};



}; // namespace ATG

#endif
