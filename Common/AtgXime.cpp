//--------------------------------------------------------------------------------------
// AtgXime.cpp
//
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//
// ** This sample has been modified to run with the XDK Xime Stub library by stevdai **
//
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include <malloc.h>
#include "AtgXime.h"
#include "AtgUtil.h"
#include "AtgXmlParser.h"


namespace ATG
{
//#define DEBUG_CLEAR_USER_DIC
//#define XEX_TITLE_DIC_FILE_LOCATOR
//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

FILE* Xime::fpUserDic = NULL;

BOOL   Xime::g_bIsStaticXime = FALSE;
BOOL   Xime::m_bLoadTitleDictionary = TRUE; 
WCHAR  Xime::m_StringBuffer[ NUM_STRINGBUFFER ];
WCHAR  Xime::m_CandidateListBuffer[ NUM_STRINGBUFFER ];
char   Xime::m_UserDicMemory[ XEIME_JP_SIZE_OF_USERDIC ];

BOOL   bGetClauseInfo = FALSE;
byte   *g_pvMemoryBlock = NULL;
byte   *g_pvCandidateFilterTable = NULL;

// Japanese Dictionaries
const int   Xime::iNumDicsJP                        = 6;
const char  *szFontFile                             = "game:\\media\\ximecandidatefilter.bin";
const char  Xime::szXEXTitleDicPath[]               = "game:\\media\\XimeJPTitleDictionary.xex";
const char  Xime::szXEXTitleDicPathFileLocator[]    = "game:\\media\\XimeJPTitleDictionary.xex,titledic";
const char  *szLocatorTemplate1                     = "section://%4x,%s#%s";   // For XUI package
const char  *szLocatorTemplate2                     = "section://%4x,%s";      // For Single Section

XOVERLAPPED Xime::s_imeOverLapped = { 0 };
// Chinese Dictionaries
const int   Xime::iNumDicsTC = 1;
const char  Xime::szBopomofoDicPath[]="game:\\media\\ximechtbopomofo.dic";

XIME_KEYMAPPING Xime::m_InputKeymap[] = {
    { MODE_INPUT | MODE_JKT, VK_SHIFT,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT | XINPUT_KEYSTROKE_SHIFT, 0, 0, (IMECALLBACK)Xime::SwitchLanguage },  //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_RETURN,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE, (IMECALLBACK)Xime::Complete },         //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_SEPARATOR,XINPUT_KEYSTROKE_KEYDOWN,0, XIME_KEY_COMPLETE, (IMECALLBACK)Xime::Complete },         //for Japanese, Hangul, Bopomofo, special conversion
    { MODE_INPUT | MODE_JKT, VK_BACK,    XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_BACKSPACE, NULL },                               //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_DELETE,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_DELETE,    NULL },                               //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_HOME,    XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_HOME,      NULL },                               //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_END,     XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_END,       NULL },                               //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_RIGHT,   XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_RIGHT,     NULL },                               //for Japanese, Hangul, Bopomofo
    { MODE_INPUT | MODE_JKT, VK_LEFT,    XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_LEFT,      NULL },                               //for Japanese, Hangul, Bopomofo
    
    { MODE_INPUT | MODE_KR,  VK_PRIOR,   XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE,  NULL },                               // for Hangul
    { MODE_INPUT | MODE_KR,  VK_NEXT,    XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE,  NULL },                               // for Hangul
    { MODE_INPUT | MODE_KR,  VK_INSERT,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE,  NULL },                               // for Hangul

    { MODE_INPUT | MODE_JP,  VK_SPACE,   XINPUT_KEYSTROKE_KEYDOWN, XINPUT_KEYSTROKE_SHIFT | XINPUT_KEYSTROKE_CTRL | XINPUT_KEYSTROKE_ALT,   XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },         //for Japanese
    { MODE_INPUT | MODE_TC,  VK_SPACE,   XINPUT_KEYSTROKE_KEYDOWN,  XINPUT_KEYSTROKE_ALT,   XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },      //for , Bopomofo

    { MODE_INPUT | MODE_JP,  VK_CONVERT, XINPUT_KEYSTROKE_KEYDOWN,  XINPUT_KEYSTROKE_CTRL | XINPUT_KEYSTROKE_ALT,   XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },         //for Japanese, Bopomofo
    { MODE_INPUT | MODE_JP,  VK_CONVERT, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT , XINPUT_KEYSTROKE_CTRL | XINPUT_KEYSTROKE_ALT,   XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },         //for Japanese, Bopomofo
    { MODE_INPUT | MODE_JP,  VK_NONCONVERT, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_KATAKANA_CANDIDATE , (IMECALLBACK)Xime::NonConvert },            //for Japanese

    { MODE_INPUT | MODE_JKT, VK_ESCAPE,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_FLUSH, NULL },                                                   //for Japanese, Bopomofo
    { MODE_INPUT | MODE_JP,  VK_F6,      XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_HIRAGANA, (IMECALLBACK)Xime::AllConvert },                   //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  VK_F7,      XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_KATAKANA, (IMECALLBACK)Xime::AllConvert },                   //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  VK_F9,      XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_ALPHANUMERIC_FULL_WIDTH, (IMECALLBACK)Xime::AllConvert },    //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  VK_F10,     XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_ALPHANUMERIC_HALF_WIDTH, (IMECALLBACK)Xime::AllConvert },    //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  L'U',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_HIRAGANA, (IMECALLBACK)Xime::AllConvert },   //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  L'I',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_KATAKANA, (IMECALLBACK)Xime::AllConvert },   //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  L'P',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_ALPHANUMERIC_FULL_WIDTH, (IMECALLBACK)Xime::AllConvert },          //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  L'O',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_ALPHANUMERIC_HALF_WIDTH, (IMECALLBACK)Xime::AllConvert },          //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  VK_KANA,    XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_MODE_JP_ROMAJI_HIRAGANA_WITH_FULLWIDTH_ALPHANUMERIC, (IMECALLBACK)Xime::ChangeInputMode },          //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  VK_KANA,    XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT, 0, XIME_MODE_JP_ROMAJI_KATAKANA_WITH_FULLWIDTH_ALPHANUMERIC, (IMECALLBACK)Xime::ChangeInputMode },          //for Japanese, special conversion
    { MODE_INPUT | MODE_JP,  VK_KANA,    XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, XIME_MODE_JP_KANAINPUT_WITH_KEYSTROKE_SHIFTFLAG_OFF, (IMECALLBACK)Xime::ToggleKeyboardLayout },          //for Japanese, special conversion

    { MODE_INPUT | MODE_JP,  VK_CAPITAL, XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_MODE_JP_HALFWIDTH_ALPHANUMERIC, (IMECALLBACK)Xime::ChangeInputMode },                            //for Japanese
    { MODE_INPUT | MODE_JP,  VK_KANJI,   XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                            //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_INPUT | MODE_JP,  VK_KANJI,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },     //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_INPUT | MODE_JP,  VK_OEM_3,   XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                            //for Japanese, special conversion, when dashboard is set to non-Japanese
    { MODE_INPUT | MODE_JP,  VK_OEM_3,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },     //for Japanese, special conversion, when dashboard is set to non-Japanese
    { MODE_INPUT | MODE_KR,  VK_HANGUL,  XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },
    { MODE_INPUT | MODE_TC,  VK_SPACE,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0, 0, (IMECALLBACK)Xime::ToggleIME },    //for Bopomofo

    // Convert mode
    { MODE_CONVERT | MODE_JKT, VK_SHIFT,  XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT | XINPUT_KEYSTROKE_SHIFT, 0, 0, (IMECALLBACK)Xime::SwitchLanguage },  //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_RETURN,XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE, (IMECALLBACK)Xime::Complete },//for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_SEPARATOR, XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE, (IMECALLBACK)Xime::Complete },     //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_RIGHT, XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_RIGHT, (IMECALLBACK)Xime::ChangeFocus },         //for Japanese, Hangul
    { MODE_CONVERT | MODE_JP,  VK_LEFT,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_LEFT, (IMECALLBACK)Xime::ChangeFocus },          //for Japanese, Hangul
    { MODE_CONVERT | MODE_JP,  VK_RIGHT, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT, 0,   XIME_KEY_RIGHT, (IMECALLBACK)Xime::ChangeClauseLength },          //for Japanese, Hangul
    { MODE_CONVERT | MODE_JP,  VK_LEFT,  XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT, 0,   XIME_KEY_LEFT, (IMECALLBACK)Xime::ChangeClauseLength },           //for Japanese, Hangul
    { MODE_CONVERT | MODE_JP,  VK_ESCAPE,XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_FLUSH, (IMECALLBACK)Xime::Revert },              //for Japanese
    { MODE_CONVERT | MODE_TC,  VK_ESCAPE,XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_FLUSH, NULL },                                   //for Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_BACK,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_FLUSH, (IMECALLBACK)Xime::Revert },              //for Japanese
    { MODE_CONVERT | MODE_TC,  VK_BACK,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_UNICODE, (IMECALLBACK)Xime::NothingToDo },       //for Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_SPACE, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_NEXT, (IMECALLBACK)Xime::SetCandidate },                      //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_TC,  VK_SPACE, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_TOP_OF_NEXT_PAGE, (IMECALLBACK)Xime::SetCandidate },          //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_CONVERT, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_NEXT, (IMECALLBACK)Xime::SetCandidate },                    //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_CONVERT, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT, 0, (XIMEKEY)XIME_INDEX_PREV, (IMECALLBACK)Xime::SetCandidate },           //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_UP,    XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_PREV, (IMECALLBACK)Xime::SetCandidate },             //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_SPACE, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT, 0,   (XIMEKEY)XIME_INDEX_PREV, (IMECALLBACK)Xime::SetCandidate },           //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_DOWN,  XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_NEXT, (IMECALLBACK)Xime::SetCandidate },             //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_PRIOR, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_TOP_OF_PREV_PAGE, (IMECALLBACK)Xime::SetCandidate }, //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NEXT,  XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_TOP_OF_NEXT_PAGE, (IMECALLBACK)Xime::SetCandidate }, //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_HOME,  XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_TOP, (IMECALLBACK)Xime::SetCandidate },              //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_END,   XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_END, (IMECALLBACK)Xime::SetCandidate },              //for Japanese, Bopomofo
    { MODE_CONVERT | MODE_JP,  VK_NONCONVERT, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_HIRAGANA_KATAKANA_TOGGLE, (IMECALLBACK)Xime::SetCandidate },            //for Japanese
    { MODE_CONVERT | MODE_JP,  VK_F6,    XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_HIRAGANA_CANDIDATE, (IMECALLBACK)Xime::SetCandidate },        //for Japanese
    { MODE_CONVERT | MODE_JP,  VK_F7,    XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_KATAKANA_CANDIDATE, (IMECALLBACK)Xime::SetCandidate },        //for Japanese
    { MODE_CONVERT | MODE_JP,  L'U',     XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   (XIMEKEY)XIME_INDEX_HIRAGANA_CANDIDATE, (IMECALLBACK)Xime::SetCandidate },      //for Japanese
    { MODE_CONVERT | MODE_JP,  L'I',     XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   (XIMEKEY)XIME_INDEX_KATAKANA_CANDIDATE, (IMECALLBACK)Xime::SetCandidate },      //for Japanese
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD0, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD1, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD2, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD3, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD4, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD5, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD6, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD7, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD8, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_NUMPAD9, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP,  VK_KANJI, XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                                //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_CONVERT | MODE_JP,  VK_KANJI, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },         //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_CONVERT | MODE_JP,  VK_OEM_3, XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                                //for Japanese, special conversion, when dashboard is set to English
    { MODE_CONVERT | MODE_JP,  VK_OEM_3, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },         //for Japanese, special conversion, when dashboard is set to English
    { MODE_CONVERT | MODE_TC,  VK_SPACE, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0, 0, (IMECALLBACK)Xime::ToggleIME },

    { MODE_CONVERT | MODE_JP | MODE_TC, VK_0, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_1, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_2, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_3, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_4, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_5, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_6, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_7, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_8, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion
    { MODE_CONVERT | MODE_JP | MODE_TC, VK_9, XINPUT_KEYSTROKE_KEYDOWN, 0, (XIMEKEY)XIME_INDEX_CURRENT_PAGE_OFFSET, (IMECALLBACK)Xime::HandleNumpadConversion },          //for Japanese, special conversion

    // All conversion mode
    { MODE_ALLCONVERT | MODE_JKT, VK_SHIFT,  XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT | XINPUT_KEYSTROKE_SHIFT, 0, 0, (IMECALLBACK)Xime::SwitchLanguage },  //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_RETURN,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE, (IMECALLBACK)Xime::Complete },        //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_SEPARATOR,XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_COMPLETE, (IMECALLBACK)Xime::Complete },       //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_ESCAPE,  XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_FLUSH, (IMECALLBACK)Xime::AllRevert },          //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_BACK,    XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_FLUSH, (IMECALLBACK)Xime::AllRevert },          //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_F6,      XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_HIRAGANA, (IMECALLBACK)Xime::AllConvert },          //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  VK_F7,      XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_KATAKANA, (IMECALLBACK)Xime::AllConvert },          //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  VK_F9,      XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_ALPHANUMERIC_FULL_WIDTH, (IMECALLBACK)Xime::AllConvert },           //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  VK_F10,     XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_ALL_ALPHANUMERIC_HALF_WIDTH, (IMECALLBACK)Xime::AllConvert },           //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  L'U',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_HIRAGANA, (IMECALLBACK)Xime::AllConvert },   //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  L'I',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_KATAKANA, (IMECALLBACK)Xime::AllConvert },   //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  L'P',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_ALPHANUMERIC_FULL_WIDTH, (IMECALLBACK)Xime::AllConvert },          //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  L'O',       XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0,   XIME_KEY_ALL_ALPHANUMERIC_HALF_WIDTH, (IMECALLBACK)Xime::AllConvert },          //for Japanese, special conversion
    { MODE_ALLCONVERT | MODE_JP,  VK_SPACE,   XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },         //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_CONVERT, XINPUT_KEYSTROKE_KEYDOWN, 0, XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },         //for Japanese, Bopomofo
    { MODE_ALLCONVERT | MODE_JP,  VK_CONVERT, XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_SHIFT, 0, XIME_KEY_CONVERT, (IMECALLBACK)Xime::Convert },         //for Japanese, Bopomofo
    
    { MODE_ALLCONVERT | MODE_JP,  VK_KANJI,   XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                         //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_ALLCONVERT | MODE_JP,  VK_KANJI,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },  //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_ALLCONVERT | MODE_JP,  VK_OEM_3,   XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                         //for Japanese, special conversion, when dashboard is set to non-Japanese
    { MODE_ALLCONVERT | MODE_JP,  VK_OEM_3,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },  //for Japanese, special conversion, when dashboard is set to non-Japanese

    { MODE_OFF | MODE_JKT, VK_SHIFT,  XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT | XINPUT_KEYSTROKE_SHIFT, 0, 0, (IMECALLBACK)Xime::SwitchLanguage },  //for Japanese, Hangul, Bopomofo
    { MODE_OFF | MODE_JP, VK_KANJI,   XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                                 //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_OFF | MODE_JP, VK_KANJI,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },          //for Japanese, special conversion, when dashboard is set to Japanese
    { MODE_OFF | MODE_JP, VK_OEM_3,   XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },                                 //for Japanese, special conversion, when dashboard is set to non_Japanese
    { MODE_OFF | MODE_JP, VK_OEM_3,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_ALT, 0, 0, (IMECALLBACK)Xime::ToggleIME },          //for Japanese, special conversion, when dashboard is set to non_Japanese
    { MODE_OFF | MODE_KR, VK_HANGUL,  XINPUT_KEYSTROKE_KEYDOWN, 0, 0, (IMECALLBACK)Xime::ToggleIME },
    { MODE_OFF | MODE_TC, VK_SPACE,   XINPUT_KEYSTROKE_KEYDOWN | XINPUT_KEYSTROKE_CTRL, 0, 0, (IMECALLBACK)Xime::ToggleIME },

};
static const DWORD NUM_INPUT_KEYMAP = sizeof(Xime::m_InputKeymap)/sizeof(Xime::m_InputKeymap[0]);

//---------------------------------------------------------------
//   Lookup Table For Japanese (Hiragana character) Non-alphabet characters
//---------------------------------------------------------------
HID_LOOKUP_TABLE JapaneseHiraganaNonAlphabetic[] = {
                                          { 0x1e,   0x38,   0,      0 },
/*HID_USAGE_INDEX_KEYBOARD_ONE*/          { 0x306C, 0x306C, 0x30CC, 0x30CC }, // nu, nu
/*HID_USAGE_INDEX_KEYBOARD_TWO*/          { 0x3075, 0x3075, 0x30D5, 0x30D5 }, // fu, fu
/*HID_USAGE_INDEX_KEYBOARD_THREE*/        { 0x3042, 0x3041, 0x30A2, 0x30A1 }, // a, a (small)
/*HID_USAGE_INDEX_KEYBOARD_FOUR*/         { 0x3046, 0x3045, 0x30A6, 0x30A5 }, // u, u (small)
/*HID_USAGE_INDEX_KEYBOARD_FIVE*/         { 0x3048, 0x3047, 0x30A8, 0x30A7 }, // e, e (small)
/*HID_USAGE_INDEX_KEYBOARD_SIX*/          { 0x304A, 0x3049, 0x30AA, 0x30A9 }, // o, o (small)
/*HID_USAGE_INDEX_KEYBOARD_SEVEN*/        { 0x3084, 0x3083, 0x30E4, 0x30E3 }, // ya, ya (small)
/*HID_USAGE_INDEX_KEYBOARD_EIGHT*/        { 0x3086, 0x3085, 0x30E6, 0x30E5 }, // yu, yu (small)
/*HID_USAGE_INDEX_KEYBOARD_NINE*/         { 0x3088, 0x3087, 0x30E8, 0x30E7 }, // yo, yo (small)
/*HID_USAGE_INDEX_KEYBOARD_ZERO*/         { 0x308F, 0x3092, 0x30EF, 0x30F2 }, // wa, o
/*HID_USAGE_INDEX_KEYBOARD_RETURN*/       { 0x000A, 0x000A, 0x000A, 0x000A }, //LF,LF
/*HID_USAGE_INDEX_KEYBOARD_ESCAPE*/       { 0x001B, 0x001B, 0x001B, 0x001B }, //ESC,ESC
/*HID_USAGE_INDEX_KEYBOARD_BACKSPACE*/    { 0x0008, 0x0008, 0x0008, 0x0008 }, //BS
/*HID_USAGE_INDEX_KEYBOARD_TAB*/          { 0x0000, 0x0000, 0x0000, 0x0000 }, //TAB is not supported
/*HID_USAGE_INDEX_KEYBOARD_SPACEBAR*/     { 0x0020, 0x0020, 0x0020, 0x0020 }, //SPACE,SPACE
/*HID_USAGE_INDEX_KEYBOARD_MINUS*/        { 0x307B, 0x307B, 0x30DB, 0x30DB }, // ho, ho
/*HID_USAGE_INDEX_KEYBOARD_EQUALS*/       { 0x3078, 0x3078, 0x30D8, 0x30D8 }, // he, he
/*HID_USAGE_INDEX_KEYBOARD_OPEN_BRACE*/   { 0x309B, 0x309B, 0x309B, 0x309B }, // voiced sound mark, voiced sound mark
/*HID_USAGE_INDEX_KEYBOARD_CLOSE_BRACE*/  { 0x309C, 0x300C, 0x309C, 0x300C }, // semi-voiced sound mark, left corner bracket
/*HID_USAGE_INDEX_KEYBOARD_BACKSLASH*/    { 0x0000, 0x0000, 0x0000, 0x0000 }, // no character, no character
/*HID_USAGE_INDEX_KEYBOARD_NON_US_TILDE*/ { 0x3080, 0x300D, 0x30E0, 0x300D }, // mu, right corner bracket
/*HID_USAGE_INDEX_KEYBOARD_COLON*/        { 0x308C, 0x308C, 0x30EC, 0x30EC }, // re, re
/*HID_USAGE_INDEX_KEYBOARD_QUOTE*/        { 0x3051, 0x3051, 0x30B1, 0x30B1 }, // ke, ke
/*HID_USAGE_INDEX_KEYBOARD_TILDE*/        { 0x0000, 0x0000, 0x0000, 0x0000 }, // no character, no character (Japanese/English toggle)
/*HID_USAGE_INDEX_KEYBOARD_COMMA*/        { 0x306D, 0x3001, 0x30CD, 0x3001 }, // ne, ideographic comma
/*HID_USAGE_INDEX_KEYBOARD_PERIOD*/       { 0x308B, 0x3002, 0x30EB, 0x3002 }, // ru,ideographic full stop
/*HID_USAGE_INDEX_KEYBOARD_QUESTION*/     { 0x3081, 0x30FB, 0x30E1, 0x30FB }  // me, middle dot
};

//---------------------------------------------------------------
//   Lookup Table For Japanese (Hiragana character) alphabet characters
//---------------------------------------------------------------
HID_LOOKUP_TABLE JapaneseHiraganaAlphabetic[] = {
                                          { 0x04,   0x1d,   0,      0 },
/*HID_USAGE_INDEX_KEYBOARD_aA*/           { 0x3061, 0x3061, 0x30C1, 0x30C1 },  // chi, chi
/*HID_USAGE_INDEX_KEYBOARD_bB*/           { 0x3053, 0x3053, 0x30B3, 0x30B3 },  // ko, ko
/*HID_USAGE_INDEX_KEYBOARD_cC*/           { 0x305D, 0x305D, 0x30BD, 0x30BD },  // so, so
/*HID_USAGE_INDEX_KEYBOARD_dD*/           { 0x3057, 0x3057, 0x30B7, 0x30B7 },  // shi, shi
/*HID_USAGE_INDEX_KEYBOARD_eE*/           { 0x3044, 0x3043, 0x30A4, 0x30A3 },  // i, i (small)
/*HID_USAGE_INDEX_KEYBOARD_fF*/           { 0x306F, 0x306F, 0x30CF, 0x30CF },  // ha, ha
/*HID_USAGE_INDEX_KEYBOARD_gG*/           { 0x304D, 0x304D, 0x30AD, 0x30AD },  // ki, ki
/*HID_USAGE_INDEX_KEYBOARD_hH*/           { 0x304F, 0x304F, 0x30AF, 0x30AF },  // ku, ku
/*HID_USAGE_INDEX_KEYBOARD_iI*/           { 0x306B, 0x306B, 0x30CB, 0x30CB },  // ni, ni
/*HID_USAGE_INDEX_KEYBOARD_jJ*/           { 0x307E, 0x307E, 0x30DE, 0x30DE },  // ma, ma
/*HID_USAGE_INDEX_KEYBOARD_kK*/           { 0x306E, 0x306E, 0x30CE, 0x30CE }, // no, no
/*HID_USAGE_INDEX_KEYBOARD_lL*/           { 0x308A, 0x308A, 0x30EA, 0x30EA }, // ri, ri
/*HID_USAGE_INDEX_KEYBOARD_mM*/           { 0x3082, 0x3082, 0x30E2, 0x30E2 }, // mo, mo
/*HID_USAGE_INDEX_KEYBOARD_nN*/           { 0x307F, 0x307F, 0x30DF, 0x30DF }, // mi, mi
/*HID_USAGE_INDEX_KEYBOARD_oO*/           { 0x3089, 0x3089, 0x30E9, 0x30E9 }, // ra,ra
/*HID_USAGE_INDEX_KEYBOARD_pP*/           { 0x305B, 0x305B, 0x30BB, 0x30BB }, // se, se
/*HID_USAGE_INDEX_KEYBOARD_qQ*/           { 0x305F, 0x305F, 0x30BF, 0x30BF }, // ta, ta
/*HID_USAGE_INDEX_KEYBOARD_rR*/           { 0x3059, 0x3059, 0x30B9, 0x30B9 }, // su, su
/*HID_USAGE_INDEX_KEYBOARD_sS*/           { 0x3068, 0x3068, 0x30C8, 0x30C8 }, // to, to
/*HID_USAGE_INDEX_KEYBOARD_tT*/           { 0x304B, 0x304B, 0x30AB, 0x30AB }, // ka, ka
/*HID_USAGE_INDEX_KEYBOARD_uU*/           { 0x306A, 0x306A, 0x30CA, 0x30CA }, // na, na
/*HID_USAGE_INDEX_KEYBOARD_vV*/           { 0x3072, 0x3072, 0x30D2, 0x30D2 }, // hi, hi
/*HID_USAGE_INDEX_KEYBOARD_wW*/           { 0x3066, 0x3066, 0x30C6, 0x30C6 }, // te, te
/*HID_USAGE_INDEX_KEYBOARD_xX*/           { 0x3055, 0x3055, 0x30B5, 0x30B5 }, // sa, sa
/*HID_USAGE_INDEX_KEYBOARD_yY*/           { 0x3093, 0x3093, 0x30F3, 0x30F3 }, // n, n
/*HID_USAGE_INDEX_KEYBOARD_zZ*/           { 0x3064, 0x3063, 0x30C4, 0x30C3 }  // tsu, tsu (small)
};


//---------------------------------------------------------------
//   Lookup Table For Korean Non-alphabet characters
//---------------------------------------------------------------
HID_LOOKUP_TABLE KoreanNonAlphabetic[] = {
                                          { 0x1e,   0x38,   0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_ONE*/          { 0x0031, 0x0021, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_TWO*/          { 0x0032, 0x0040, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_THREE*/        { 0x0033, 0x0023, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_FOUR*/         { 0x0034, 0x0024, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_FIVE*/         { 0x0035, 0x0025, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_SIX*/          { 0x0036, 0x005E, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_SEVEN*/        { 0x0037, 0x0026, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_EIGHT*/        { 0x0038, 0x002A, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_NINE*/         { 0x0039, 0x0028, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_ZERO*/         { 0x0030, 0x0029, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_RETURN*/       { 0x000A, 0x000A, 0, 0 }, // LF,LF
/*HID_USAGE_INDEX_KEYBOARD_ESCAPE*/       { 0x001B, 0x001B, 0, 0 }, // ESC,ESC
/*HID_USAGE_INDEX_KEYBOARD_BACKSPACE*/    { 0x0008, 0x0008, 0, 0 }, // BS
/*HID_USAGE_INDEX_KEYBOARD_TAB*/          { 0x0000, 0x0000, 0, 0 }, // TAB is not supported
/*HID_USAGE_INDEX_KEYBOARD_SPACEBAR*/     { 0x0020, 0x0020, 0, 0 }, // SPACE,SPACE
/*HID_USAGE_INDEX_KEYBOARD_MINUS*/        { 0x002D, 0x005F, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_EQUALS*/       { 0x003D, 0x002B, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_OPEN_BRACE*/   { 0x005B, 0x007B, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_CLOSE_BRACE*/  { 0x005D, 0x007D, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_BACKSLASH*/    { 0x005C, 0x007C, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_NON_US_TILDE*/ { 0x0000, 0x0000, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_COLON*/        { 0x003B, 0x003A, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_QUOTE*/        { 0x0027, 0x0022, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_TILDE*/        { 0x0060, 0x007E, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_COMMA*/        { 0x002C, 0x003C, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_PERIOD*/       { 0x002E, 0x003E, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_QUESTION*/     { 0x002F, 0x003F, 0, 0 }, // 
};

//---------------------------------------------------------------
//   Lookup Table For Korean alphabet characters
//---------------------------------------------------------------
HID_LOOKUP_TABLE KoreanAlphabetic[] = {
                                          { 0x04,   0x1d,   0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_aA*/           { 0x3141, 0x3141, 0, 0 }, // Mieum, Mieum
/*HID_USAGE_INDEX_KEYBOARD_bB*/           { 0x3160, 0x3160, 0, 0 }, // Yu, Yu
/*HID_USAGE_INDEX_KEYBOARD_cC*/           { 0x314A, 0x314A, 0, 0 }, // Chieuch, Chieuch
/*HID_USAGE_INDEX_KEYBOARD_dD*/           { 0x3147, 0x3147, 0, 0 }, // leung, leung
/*HID_USAGE_INDEX_KEYBOARD_eE*/           { 0x3137, 0x3138, 0, 0 }, // Tikeut, Ssangtikeut
/*HID_USAGE_INDEX_KEYBOARD_fF*/           { 0x3139, 0x3139, 0, 0 }, // Rieul, Rieul
/*HID_USAGE_INDEX_KEYBOARD_gG*/           { 0x314E, 0x314E, 0, 0 }, // Hieuh, Hieuh
/*HID_USAGE_INDEX_KEYBOARD_hH*/           { 0x3157, 0x3157, 0, 0 }, // O, O
/*HID_USAGE_INDEX_KEYBOARD_iI*/           { 0x3151, 0x3151, 0, 0 }, // Ya, Ya
/*HID_USAGE_INDEX_KEYBOARD_jJ*/           { 0x3153, 0x3153, 0, 0 }, // Eo, Eo
/*HID_USAGE_INDEX_KEYBOARD_kK*/           { 0x314F, 0x314F, 0, 0 }, // A, A
/*HID_USAGE_INDEX_KEYBOARD_lL*/           { 0x3163, 0x3163, 0, 0 }, // I, I
/*HID_USAGE_INDEX_KEYBOARD_mM*/           { 0x3161, 0x3161, 0, 0 }, // Eu, Eu
/*HID_USAGE_INDEX_KEYBOARD_nN*/           { 0x315C, 0x315C, 0, 0 }, // U, U
/*HID_USAGE_INDEX_KEYBOARD_oO*/           { 0x3150, 0x3152, 0, 0 }, // Ae, Yae
/*HID_USAGE_INDEX_KEYBOARD_pP*/           { 0x3154, 0x3156, 0, 0 }, // E, Ye
/*HID_USAGE_INDEX_KEYBOARD_qQ*/           { 0x3142, 0x3143, 0, 0 }, // Pieup, Ssangpieup
/*HID_USAGE_INDEX_KEYBOARD_rR*/           { 0x3131, 0x3132, 0, 0 }, // Kiyeok, Ssangkiyeok
/*HID_USAGE_INDEX_KEYBOARD_sS*/           { 0x3134, 0x3134, 0, 0 }, // Nieun, Nieun
/*HID_USAGE_INDEX_KEYBOARD_tT*/           { 0x3145, 0x3146, 0, 0 }, // Sios, Ssangsios
/*HID_USAGE_INDEX_KEYBOARD_uU*/           { 0x3155, 0x3155, 0, 0 }, // Yeo, Yeo
/*HID_USAGE_INDEX_KEYBOARD_vV*/           { 0x314D, 0x314D, 0, 0 }, // Phieuph, Phieuph
/*HID_USAGE_INDEX_KEYBOARD_wW*/           { 0x3148, 0x3149, 0, 0 }, // Cieuc, Ssangcieuc
/*HID_USAGE_INDEX_KEYBOARD_xX*/           { 0x314C, 0x314C, 0, 0 }, // Thieuth, Thieuth
/*HID_USAGE_INDEX_KEYBOARD_yY*/           { 0x315B, 0x315B, 0, 0 }, // Yo, Yo
/*HID_USAGE_INDEX_KEYBOARD_zZ*/           { 0x314B, 0x314B, 0, 0 }  // Khieukh, Khieukh
};

//---------------------------------------------------------------
//   Lookup Table For 10 key
//---------------------------------------------------------------
WCHAR TenKeyTable[] = {
// From 0x54 to 0x63
/*HID_USAGE_INDEX_TENKEY_SEPARATOR*/      L'/',
/*HID_USAGE_INDEX_TENKEY_**/              L'*',
/*HID_USAGE_INDEX_TENKEY_-*/              L'-',
/*HID_USAGE_INDEX_TENKEY_+*/              L'+',
/*HID_USAGE_INDEX_TENKEY_ENTER*/          0x0013,
/*HID_USAGE_INDEX_TENKEY_1*/              L'1',
/*HID_USAGE_INDEX_TENKEY_2*/              L'2',
/*HID_USAGE_INDEX_TENKEY_3*/              L'3',
/*HID_USAGE_INDEX_TENKEY_4*/              L'4',
/*HID_USAGE_INDEX_TENKEY_5*/              L'5',
/*HID_USAGE_INDEX_TENKEY_6*/              L'6',
/*HID_USAGE_INDEX_TENKEY_7*/              L'7',
/*HID_USAGE_INDEX_TENKEY_8*/              L'8',
/*HID_USAGE_INDEX_TENKEY_9*/              L'9',
/*HID_USAGE_INDEX_TENKEY_0*/              L'0',
/*HID_USAGE_INDEX_TENKEY_.*/              L'.'
};


//---------------------------------------------------------------
//   Lookup Table For ChineseTraditional (Bopomofo) Non-alphabet characters
//---------------------------------------------------------------
HID_LOOKUP_TABLE ChineseTraditionalBopomofoNonAlphabetic[] = {
                                          { 0x1e,  0x38,   0,      0 },
/*HID_USAGE_INDEX_KEYBOARD_ONE*/          { 0x3105, 0x3105, 0, 0 }, // Half pinyin B
/*HID_USAGE_INDEX_KEYBOARD_TWO*/          { 0x3109, 0x3109, 0, 0 }, // Half pinyin D
/*HID_USAGE_INDEX_KEYBOARD_THREE*/        { 0x02C7, 0x02C7, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_FOUR*/         { 0x02CB, 0x02CB, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_FIVE*/         { 0x3113, 0x3113, 0, 0 }, //
/*HID_USAGE_INDEX_KEYBOARD_SIX*/          { 0x02CA, 0x02CA, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_SEVEN*/        { 0x02D9, 0x02D9, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_EIGHT*/        { 0x311A, 0x311A, 0, 0 }, //
/*HID_USAGE_INDEX_KEYBOARD_NINE*/         { 0x311E, 0x311E, 0, 0 }, //
/*HID_USAGE_INDEX_KEYBOARD_ZERO*/         { 0x3122, 0x3122, 0, 0 }, //
/*HID_USAGE_INDEX_KEYBOARD_RETURN*/       { 0x000A, 0x000A, 0, 0 }, //LF,LF
/*HID_USAGE_INDEX_KEYBOARD_ESCAPE*/       { 0x001B, 0x001B, 0, 0 }, //ESC,ESC
/*HID_USAGE_INDEX_KEYBOARD_BACKSPACE*/    { 0x0008, 0x0008, 0, 0 }, //BS
/*HID_USAGE_INDEX_KEYBOARD_TAB*/          { 0x0000, 0x0000, 0, 0 }, //TAB is not supported
/*HID_USAGE_INDEX_KEYBOARD_SPACEBAR*/     { 0x0020, 0x0020, 0, 0 }, //SPACE,SPACE
/*HID_USAGE_INDEX_KEYBOARD_MINUS*/        { 0x3126, 0x3126, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_EQUALS*/       { 0x003D, 0x002B, 0, 0 }, // '=', '+'
/*HID_USAGE_INDEX_KEYBOARD_OPEN_BRACE*/   { 0x005B, 0x007B, 0, 0 }, // '[', '{'
/*HID_USAGE_INDEX_KEYBOARD_CLOSE_BRACE*/  { 0x005D, 0x007D, 0, 0 }, // ']', '}'
/*HID_USAGE_INDEX_KEYBOARD_BACKSLASH*/    { 0x005C, 0x007C, 0, 0 }, // Backslash, Vertical Line
/*HID_USAGE_INDEX_KEYBOARD_NON_US_TILDE*/ { 0x0000, 0x0000, 0, 0 }, // Not on US keyboard
/*HID_USAGE_INDEX_KEYBOARD_COLON*/        { 0x3124, 0x3124, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_QUOTE*/        { 0x0027, 0x0022, 0, 0 }, // ''', '"'
/*HID_USAGE_INDEX_KEYBOARD_TILDE*/        { 0x0060, 0x007E, 0, 0 }, // '`', '~'
/*HID_USAGE_INDEX_KEYBOARD_COMMA*/        { 0x311D, 0x311D, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_PERIOD*/       { 0x3121, 0x3121, 0, 0 }, // 
/*HID_USAGE_INDEX_KEYBOARD_QUESTION*/     { 0x3125, 0x3125, 0, 0 }  // 
};

//---------------------------------------------------------------
//   Lookup Table For Chinese (Bopomofo) alphabet characters
//---------------------------------------------------------------
HID_LOOKUP_TABLE ChineseTraditionalBopomofoAlphabetic[] = {
                                          { 0x04,   0x1d,  0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_aA*/           { 0x3107, 0x3107, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_bB*/           { 0x3116, 0x3116, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_cC*/           { 0x310F, 0x310F, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_dD*/           { 0x310E, 0x310E, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_eE*/           { 0x310D, 0x310D, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_fF*/           { 0x3111, 0x3111, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_gG*/           { 0x3115, 0x3115, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_hH*/           { 0x3118, 0x3118, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_iI*/           { 0x311B, 0x311B, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_jJ*/           { 0x3128, 0x3128, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_kK*/           { 0x311C, 0x311C, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_lL*/           { 0x3120, 0x3120, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_mM*/           { 0x3129, 0x3129, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_nN*/           { 0x3119, 0x3119, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_oO*/           { 0x311F, 0x311F, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_pP*/           { 0x3123, 0x3123, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_qQ*/           { 0x3106, 0x3106, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_rR*/           { 0x3110, 0x3110, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_sS*/           { 0x310B, 0x310B, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_tT*/           { 0x3114, 0x3114, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_uU*/           { 0x3127, 0x3127, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_vV*/           { 0x3112, 0x3112, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_wW*/           { 0x310A, 0x310A, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_xX*/           { 0x310C, 0x310C, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_yY*/           { 0x3117, 0x3117, 0, 0 },
/*HID_USAGE_INDEX_KEYBOARD_zZ*/           { 0x3108, 0x3108, 0, 0 },
};

XIMEPOS_LOOKUP_TABLE XimeposTable[] = 
{
    XIMEPOS_NOUN,                           L"XIMEPOS_NOUN",
    XIMEPOS_PSEUDO_NOUN_SURU,               L"XIMEPOS_PSEUDO_NOUN_SURU",
    XIMEPOS_PSEUDO_NOUN_ZURU,               L"XIMEPOS_PSEUDO_NOUN_ZURU",
    XIMEPOS_ADJECTIVAL_NOUN,                L"XIMEPOS_ADJECTIVAL_NOUN",
    XIMEPOS_ADVERBIAL_NOUN,                 L"XIMEPOS_ADVERBIAL_NOUN",
    XIMEPOS_PSEUDO_ADJECTIVAL_NOUN,         L"XIMEPOS_PSEUDO_ADJECTIVAL_NOUN",
    XIMEPOS_HUMAN_NAME,                     L"XIMEPOS_HUMAN_NAME",
    XIMEPOS_FAMILY_NAME,                    L"XIMEPOS_FAMILY_NAME",
    XIMEPOS_GIVEN_NAME,                     L"XIMEPOS_GIVEN_NAME",
    XIMEPOS_PLACE_NAME,                     L"XIMEPOS_PLACE_NAME",
    XIMEPOS_PROPER_NOUN,                    L"XIMEPOS_PROPER_NOUN",
    XIMEPOS_VERB_5THSTEP_A_WA,              L"XIMEPOS_VERB_5THSTEP_A_WA",
    XIMEPOS_VERB_5THSTEP_KA,                L"XIMEPOS_VERB_5THSTEP_KA",
    XIMEPOS_VERB_5THSTEP_GA,                L"XIMEPOS_VERB_5THSTEP_GA",
    XIMEPOS_VERB_5THSTEP_SA,                L"XIMEPOS_VERB_5THSTEP_SA",
    XIMEPOS_VERB_5THSTEP_TA,                L"XIMEPOS_VERB_5THSTEP_TA",
    XIMEPOS_VERB_5THSTEP_NA,                L"XIMEPOS_VERB_5THSTEP_NA",
    XIMEPOS_VERB_5THSTEP_BA,                L"XIMEPOS_VERB_5THSTEP_BA",
    XIMEPOS_VERB_5THSTEP_MA,                L"XIMEPOS_VERB_5THSTEP_MA",
    XIMEPOS_VERB_5THSTEP_RA,                L"XIMEPOS_VERB_5THSTEP_RA",
    XIMEPOS_VERB_1STSTEP,                   L"XIMEPOS_VERB_1STSTEP",
    XIMEPOS_ADJECTIVE,                      L"XIMEPOS_ADJECTIVE",
    XIMEPOS_NOMINAL_ADJECTIVE,              L"XIMEPOS_NOMINAL_ADJECTIVE",
    XIMEPOS_ADVERB,                         L"XIMEPOS_ADVERB",
    XIMEPOS_ADNOUN,                         L"XIMEPOS_ADNOUN",
    XIMEPOS_CONJUNCTION,                    L"XIMEPOS_CONJUNCTION",
    XIMEPOS_INTERJECTION,                   L"XIMEPOS_INTERJECTION",
    XIMEPOS_PREFIX,                         L"XIMEPOS_PREFIX",
    XIMEPOS_SUFFIX,                         L"XIMEPOS_SUFFIX",
    XIMEPOS_SUFFIX_FOR_NUMERALS,            L"XIMEPOS_SUFFIX_FOR_NUMERALS",
    XIMEPOS_SINGLE_KANJI,                   L"XIMEPOS_SINGLE_KANJI",
    XIMEPOS_FACE_MARK,                      L"XIMEPOS_FACE_MARK",
    XIMEPOS_IDIOM,                          L"XIMEPOS_IDIOM",
};

#ifdef DEBUG_USER_DIC_TEST
void SaveUserDic(void* mem, int size){

    FILE *fp;
    fopen_s(&fp, "game:\\media\\user.dic", "wb");
    if( fp )
    {
        fwrite(mem, 1, size, fp);
        fclose(fp);
    }
}
#endif  //DEBUG_USER_DIC_TEST

//--------------------------------------------------------------------------------------
// Name: EnumerateDictionarySummary
// Desc: Enumerate XEX based title dictionary
//--------------------------------------------------------------------------------------
void Xime::EnumerateDictionarySummary()
{
    HMODULE hModule = LoadLibrary( szXEXTitleDicPath );
    if( !hModule )
    {
        return;
    }

    // Make Locator
    char szLocator[ MAX_PATH ];
    sprintf_s( szLocator, szLocatorTemplate2, hModule, "titledic" );

    //
    // Example of using XIMEEnumerateDictionarySummaryEx
    //
    DWORD dwNumOfDic = 2;
    WCHAR buff1[ 256 ], buff2[ 256 ];
    XIME_ENUM_TITLE_DICTIONARY_EX XimeEnumTitleDic[ 2 ];
    
    XimeEnumTitleDic[ 0 ].pcszDicFile               = szXEXTitleDicPathFileLocator; // File locator
    XimeEnumTitleDic[ 0 ].pwszDictionaryTitleString = buff1;
    XimeEnumTitleDic[ 0 ].cwchDictionaryTitleString = ARRAYSIZE( buff1 );

    XimeEnumTitleDic[ 1 ].pcszDicFile               = szLocator;                    // Resource locator
    XimeEnumTitleDic[ 1 ].pwszDictionaryTitleString = buff2;
    XimeEnumTitleDic[ 1 ].cwchDictionaryTitleString = ARRAYSIZE( buff2 );

    XIMEEnumerateDictionarySummaryEx( &dwNumOfDic, XimeEnumTitleDic, NULL );

    FreeLibrary( hModule );
}

//--------------------------------------------------------------------------------------
// Name: SetInputLanguage
// Desc: Set the Input Language for IME
//--------------------------------------------------------------------------------------
HRESULT Xime::SetInputLanguage( IMEMODE eLanguage )
{
    XIMEClose();

    // Free any previous allocated memory
    if( g_pvMemoryBlock )
    {
        delete [] g_pvMemoryBlock;
        g_pvMemoryBlock = NULL;
    }
    if( g_pvCandidateFilterTable )
    {
        delete [] g_pvCandidateFilterTable;
        g_pvCandidateFilterTable = NULL;
    }

    //
    // Example of using user index
    //
    m_dwUserIndex = XUSER_INDEX_ANY;
    XUID    xuid;
    char    GamerName[ 256 ];

    for( int i = 0; i < XUSER_MAX_COUNT; i++ )
    {
        // check for signed in user
        if( XUserGetXUID( i, &xuid ) == ERROR_SUCCESS)
        {
            // Found an active user index
            if( XUserGetName( i, GamerName, ARRAYSIZE(GamerName)) == ERROR_SUCCESS )
            {
                m_dwUserIndex = i;
                break;
            }
        }
    }

    HMODULE hModule = NULL;

    switch ( eLanguage )
    {
    case MODE_JP:
        {
            EnumerateDictionarySummary();

            m_CurrentLanguage = MODE_JP;

            //Initialize parameters
            int nNumOfDic = 0;
            m_JPUserWord = 0;
            XIME_CREATE_EX      XimeJpCreate = { 0 };
            LPCSTR              XimeDic[ iNumDicsJP ];
            ZeroMemory( XimeDic, sizeof( XimeDic ) );

            //
            // Load XEX title dic (resource locator)
            //
            hModule = LoadLibrary( szXEXTitleDicPath );
            if( hModule )
            {
                // Set title dic
                //
                // Load XEX title dic (file locator)
                //
                XimeDic[ nNumOfDic++ ] = szXEXTitleDicPathFileLocator;     // File locator

                char szLocator[ MAX_PATH ];
                sprintf_s( szLocator, szLocatorTemplate2, hModule, "titledic" );                
                XimeDic[ nNumOfDic++ ] = szLocator;    // resource locator
            }
                        
            //
            // Example of using fixed memory when creating XIME
            //
            int MemorySize = 2 * 1024 * 1024;   // 2MB
            g_pvMemoryBlock = new byte[ MemorySize ];  
            if( g_pvMemoryBlock )
            {
                XimeJpCreate.pvMemoryBlock = (void*)g_pvMemoryBlock;
                XimeJpCreate.dwSizeOfMemory = MemorySize;
                DWORD dwRequiredSizeofMemory;
                XimeJpCreate.pdwRequiredSizeOfMemory = &dwRequiredSizeofMemory;
            }
            else
            {
                XimeJpCreate.pvMemoryBlock           = NULL;
                XimeJpCreate.dwSizeOfMemory          = 0;
                XimeJpCreate.pdwRequiredSizeOfMemory = NULL;
            }
            
            //
            // Open bit table file
            //
            BOOL    fSetFontFile = FALSE;
            DWORD   dwRead;
            HANDLE  handleIn = CreateFileA( szFontFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL );
            if( handleIn != INVALID_HANDLE_VALUE )
            {
                DWORD dwSize = GetFileSize( handleIn, NULL );
                if( dwSize != -1 )
                {
                    g_pvCandidateFilterTable = (LPBYTE)new BYTE[ dwSize ];
                    if( g_pvCandidateFilterTable )
                    {
                        if( ReadFile( handleIn, g_pvCandidateFilterTable, dwSize, &dwRead, NULL ) )
                        {
                            if( dwSize == dwRead )
                            {
                                fSetFontFile = TRUE;    
                            }
                        }
                    }
                }
            }

            //Initialzie IME
            XimeJpCreate.dwNumberOfDictionaries         = nNumOfDic;
            XimeJpCreate.pcszDicFile                    = ( nNumOfDic == 0 ) ? NULL : &XimeDic[0];
            XimeJpCreate.dwProcessor                    = 0;
            XimeJpCreate.pvCandidateFilterTable         = ( fSetFontFile ) ? g_pvCandidateFilterTable : NULL;
            XimeJpCreate.dwNumberOfCandidateListInPage  = NUM_CANDIATEINPAGE;
            XimeJpCreate.eXimeVocabularyOption          = XIME_VOCABULARY_STANDARD;

            // Create XIME
            DWORD dw = XIMECreateEx( m_dwUserIndex, XIMEJPInit, &XimeJpCreate );
            if( dw != ERROR_SUCCESS )
            {
                return E_FAIL;
            }
            
            // Count curent user register words
            UpdateJPUserWordNumber();

            m_iInputMode = XIME_MODE_JP_ROMAJI_HIRAGANA_WITH_FULLWIDTH_ALPHANUMERIC;
            m_iKeyboardLayout = XIME_LAYOUT_ALPHANUMERIC;

            if( handleIn != INVALID_HANDLE_VALUE )
            {
                CloseHandle( handleIn );
            }

            if( hModule )
            {
                FreeLibrary( hModule );
            }
                            
         }
        break;

    case MODE_KR:
        m_CurrentLanguage = MODE_KR;
        if( XIMECreateEx( XUSER_INDEX_ANY, XIMEKRInit, NULL ) != ERROR_SUCCESS )
            return E_FAIL;
        m_iInputMode = XIME_MODE_KR_HANGUL;
        break;

    case MODE_TC:
        {
            //Initialize parameters
            XIME_CREATE_EX    XimeChtCreate = { 0 };
            XimeChtCreate.dwNumberOfCandidateListInPage = NUM_CANDIATEINPAGE;
            XimeChtCreate.eXimeVocabularyOption         = XIME_VOCABULARY_STANDARD;    

            m_CurrentLanguage = MODE_TC;
            if( XIMECreateEx( XUSER_INDEX_ANY, XIMECHBopomofoInit, &XimeChtCreate ) != ERROR_SUCCESS )
                return E_FAIL;
            m_iInputMode = XIME_MODE_CHT_BOPOMOFO;
        }
        break;

    default:
        return E_FAIL;
    }

    // Reset the input character limit after each time we toggle the language
    XIMESetCharacterLimit( m_dwInputCharacterLength );
    
    SetCurrentMode( MODE_INPUT );
    m_bNeedBlocking = FALSE;
    m_dwCharsInXime = 0;

    m_bIMEOn = TRUE;
    m_bCompleted = FALSE;

    ZeroMemory( &m_Key, sizeof( m_Key ) );
    ZeroMemory( &m_LastKey, sizeof( m_LastKey ) );
    m_CompletedString.clear();
    m_bGotClause = FALSE;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Init
// Desc: Initialize the XIME
//--------------------------------------------------------------------------------------
HRESULT Xime::Init( IMEMODE eLanguage, DWORD dwCharLength, D3DPRESENT_PARAMETERS * pD3DParam )
{
    assert( pD3DParam != NULL );

#ifdef DEBUG_USER_DIC_TEST
    ZeroMemory(m_UserDicMemory, XEIME_JP_SIZE_OF_USERDIC);
#endif // DEBUG_USER_DIC_TEST

    if( SetInputLanguage( eLanguage ) != S_OK  )
        return E_FAIL;

    m_BackBufferWidth = (float)pD3DParam->BackBufferWidth;
    m_BackBufferHeight = (float)pD3DParam->BackBufferHeight;

    m_dwInputCharacterLength = ( dwCharLength == 0 ) ? DEFAULT_CHARACTERLENGTH : dwCharLength;
    XIMESetCharacterLimit( m_dwInputCharacterLength );

    m_bUpdateInputLength = FALSE;

    ZeroMemory( &s_imeOverLapped, sizeof( s_imeOverLapped ) );
    s_imeOverLapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if( s_imeOverLapped.hEvent == NULL ){
        ATG::DebugSpew( "Couldn't create event.\n" );
        return E_FAIL;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Update
// Desc: Update Xime
//--------------------------------------------------------------------------------------
HRESULT Xime::Update()
{
    HRESULT hr;

    //Check request queue status
    if( !m_RequestList.empty() )
    {
        XIMEREQUEST  &xr = m_RequestList.front();
        if( XHasOverlappedIoCompleted( &xr.ov ) )
        {
            // Check error code.
            DWORD dwResult = XGetOverlappedResult( &xr.ov, NULL, false );
            if( dwResult == ERROR_SUCCESS )
            {
                if( xr.dwRequestId == NEED_TO_DISABLE_LIST )
                    m_bShowCandidateList = FALSE;
                else if( xr.dwRequestId == NEED_TO_COMPLETE )
                    Complete(this, NULL);
            }
            else if( dwResult == ERROR_NO_DATA )
            {
                m_bCompleted = TRUE;
                m_bGotClause = FALSE;
                m_LastKey = xr.key;
            }
            m_RequestList.pop_front();
        }
        if( m_bNeedBlocking )
            return S_OK;
    }
    else
        m_bNeedBlocking = FALSE;

    hr = XInputGetKeystroke( XUSER_INDEX_ANY, XINPUT_FLAG_KEYBOARD, &m_Key );
    if( hr != ERROR_SUCCESS )
        return hr;
    
    //Just ingnore released key
    if( m_Key.Flags & XINPUT_KEYSTROKE_KEYUP ) return S_OK;

    // Ignore some flags
    m_Key.Flags &= ~( XINPUT_KEYSTROKE_VALIDUNICODE |
                      XINPUT_KEYSTROKE_CAPSLOCK |
                      XINPUT_KEYSTROKE_NUMLOCK |
                      XINPUT_KEYSTROKE_ROMAJI |
                      XINPUT_KEYSTROKE_HIRAGANA |
                      XINPUT_KEYSTROKE_HANGUL |
                      XINPUT_KEYSTROKE_BOPOMOFO |
                      XINPUT_KEYSTROKE_CHAJEI |
                      XINPUT_KEYSTROKE_REMOTE |
                      XINPUT_KEYSTROKE_REPEAT );

    DWORD   dwFlags = m_CurrentImeMode | m_CurrentLanguage;
    XIMEKEY eControlKey = XIME_KEY_UNICODE;
    ATG::IMECALLBACK  CallBack = NULL;
    int i;

    // Look up Unicode from HID before looking up a keymap table
    // due to I wanted to convert VirtualKey, too. (for KR)
    INT iInputMode = m_iInputMode;
    LayoutConversion( &m_Key, &iInputMode );

    if( m_bUpdateInputLength )
    {
        // Reset IME string length
        XIMESetCharacterLimit( m_dwInputCharacterLength );
        
        m_bUpdateInputLength = FALSE;
    }

    // Look up a table...
    for( i = 0; i < NUM_INPUT_KEYMAP; i++ )
    {
        if( (dwFlags & m_InputKeymap[ i ].dwFlags) == dwFlags
            && m_Key.VirtualKey == m_InputKeymap[ i ].wVK
            && ((m_Key.Flags & ~m_InputKeymap[ i ].wIgnoredXinputFlags) == m_InputKeymap[ i ].wXinputFlags) )
        {
            eControlKey = m_InputKeymap[ i ].eControlKey;
            CallBack = m_InputKeymap[ i ].function;
            break;
        }
    }

    if( CallBack )
    {
        // Then, invoke callback
        hr = CallBack( this, &m_InputKeymap[ i ] );

        // in order to insert a unicode character if hr is S_FALSE.
        eControlKey = XIME_KEY_UNICODE;
    }
    else if( !m_bIMEOn )
    {
        hr = ERROR_SERVICE_DISABLED;
    }
    else
    {
        hr = S_FALSE;
    }

    if( hr == S_FALSE && (m_Key.Unicode || eControlKey != XIME_KEY_UNICODE) )
    {
        hr = S_OK;

        if( eControlKey == XIME_KEY_UNICODE &&
            (m_CurrentImeMode == MODE_CONVERT || m_CurrentImeMode == MODE_ALLCONVERT) )
        {
            //
            // #142426, In CHT
            // Flush candidate list for ASCI charachters.
            // Determine candidate list for Bopomofo characters.
            //
            if( m_CurrentLanguage == MODE_TC && ( m_Key.Unicode >= 0x21 && m_Key.Unicode <= 0x7E ) )
            {
                XIMEInsertCharacter( '\0', XIME_KEY_FLUSH, iInputMode, NULL );
            }
            else
            {
                Complete( this, NULL );
            }
            //this->m_bNeedBlocking = TRUE;
            // Need to block since input mode has been changed
            SetCurrentMode( MODE_INPUT );
        }

        //Input some letters
        XIMEREQUEST request = { 0 };
        m_RequestList.push_back( request );

        if( eControlKey != XIME_KEY_UNICODE )
        {
            // API does not accept Unicode value except when using XIME_KEY_UNICODE
            m_Key.Unicode = 0x0;
        }
        // save the keystroke for pop back when this call is not taken the key.
        m_RequestList.back().key = m_Key;

        XIMEInsertCharacter( m_Key.Unicode,
                             eControlKey,
                             iInputMode,
                             &m_RequestList.back().ov );

        m_dwCharsInXime++;
    }
    return hr;
}


//--------------------------------------------------------------------------------------
// Name: GetString
// Desc: RetrieveString
//--------------------------------------------------------------------------------------
HRESULT Xime::GetString( std::wstring & stringOut )
{
    if( !IsStringReady() ) 
        return ERROR_NOT_READY;
    stringOut = m_CompletedString;
    m_bCompleted = FALSE;
    m_CompletedString.clear();

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: IsStringReady
// Desc:
//--------------------------------------------------------------------------------------
BOOL Xime::IsStringReady( void )
{
    if( !m_RequestList.empty() ) return FALSE;
    if( m_CompletedString.empty() )
    {
        if( m_bGotClause )
            m_bCompleted = FALSE;
        return FALSE;
    }
    return TRUE;
}


//--------------------------------------------------------------------------------------
// Name: Render
// Desc: Render Xime
//--------------------------------------------------------------------------------------
HRESULT Xime::Render( HXUIDC hDC, HXUIFONT hFont, float fPosX, float fPosY )
{
    assert( hDC != NULL );
    assert( hFont != NULL );

    DWORD   dwRet = ERROR_SUCCESS;
    DWORD   dwFocus = 0;
    DWORD   dwCursor = 0;
    DWORD   dwClauses = 0;
    DWORD   dwDeterminedClause = 0;

    DWORD   dwNumCandidate = 0;
    DWORD   dwCandidateIndex = 0;
    DWORD   dwHighLightIndexInlist;
    DWORD   dwNumberOfCandidateStringsInList;
    bool    bListAvailable = false;

    // we need to get the clause info until XIME will complete
    // even if the ime state is off.
    if( !IsIMEOn() )
    {
        return ERROR_SERVICE_DISABLED;
    }

    // clear for XIMEGetClauseInfo behavior
    dwNumberOfCandidateStringsInList = 0; 

    // Reading string buffer
    WCHAR wszReadingString[64];
    wszReadingString[ 0 ] = 0;
   
    // Retrieve current clause information of IME
    dwRet = XIMEGetClauseInfo( &dwFocus,
                               &dwClauses,
                               &dwCursor,
                               &dwDeterminedClause,
                               m_StringBuffer,
                               sizeof( m_StringBuffer ) / sizeof( WCHAR ),
                               NULL,
                               &dwNumberOfCandidateStringsInList,
                               wszReadingString,
                               ARRAYSIZE( wszReadingString ) );
    if( dwRet != ERROR_SUCCESS ) return E_FAIL;
    
    // Chinese XIME has the auto-conversion behavior.
    // so, the current mode has to be controlled by Chinese XIME
    if( m_CurrentLanguage == MODE_TC )
    {
        if( dwNumberOfCandidateStringsInList > 0 )
        {
            SetCurrentMode( MODE_CONVERT );
            m_bShowCandidateList = TRUE;
        }
        else
        {
            SetCurrentMode( MODE_INPUT );
        }
    }

    // Retrieve candidate list if IME is in conversion mode
    if( m_CurrentImeMode == MODE_CONVERT )
    {
        DWORD dwRequiredCandidate;
        dwRet = XIMEGetCandidateListInfo( m_CandidateListBuffer,
                                          sizeof( m_CandidateListBuffer ) / sizeof( WCHAR ),
                                          &dwRequiredCandidate,
                                          &dwCandidateIndex,
                                          &dwNumCandidate,
                                          &dwHighLightIndexInlist,
                                          &dwNumberOfCandidateStringsInList );
        if( dwRet == ERROR_SUCCESS ) bListAvailable = true;
    }

    // Begin Xui rendering
    XuiRenderBegin( hDC, D3DCOLOR_ARGB( 255, 0, 0, 0 ) );

    // Set the view
    D3DXMATRIX matView;
    D3DXMatrixIdentity( &matView );
    XuiRenderSetViewTransform( hDC, &matView );

    WCHAR * pString = m_StringBuffer;
    D3DCOLOR color;

    // we get the determined string exactly
    // instead of getting it after completed signal of XIME_KEY_COMPLETE
    m_bGotClause = TRUE;

    DWORD  dwDeterminedLength;
    if( dwDeterminedClause == 0 )
    {
        m_CompletedString += pString;
        dwDeterminedLength = wcslen( pString );
        m_bCompleted = TRUE;
    }
    else
    {
        assert( dwDeterminedClause == dwDeterminedClause );
        dwDeterminedLength = 0;
    }

    m_CurrentString = L"";

    //Show typed string
    color = COLOR_NORMAL;

    float fX = fPosX;

    // Render clause, candidate list
    for( DWORD i = 0; i < dwClauses; i++ )
    {
        if( i == dwFocus )
        {
            // Draw focus string
            color = COLOR_FOCUS;
            float fWidth, fHeight;
            DrawText( hDC, hFont, color, fX, fPosY, pString, &fWidth, &fHeight );
            if( bListAvailable && m_bShowCandidateList )
            {
                float fY = fPosY + fHeight;
                WCHAR* pCandidateString = m_CandidateListBuffer;

                // draw candidate list
                for( DWORD j = 0; j < dwNumCandidate; j++ )
                {
                    if( j == dwCandidateIndex )
                        color = COLOR_FOCUS;
                    else
                        color = COLOR_LIST;

                    DrawText( hDC, hFont, color, fX, fY, pCandidateString, NULL, &fHeight );
                    INT iStrLen = wcslen( pCandidateString );
                    pCandidateString += iStrLen + 1;
                    fY += fHeight;
                }
            }
            fX += fWidth;
        }
        else
        {
            // Draw other clause strings in IME
            color = ( i == dwDeterminedClause ) ? COLOR_NORMAL : COLOR_INIME;
            float fWidth;
            DrawText( hDC, hFont, color, fX, fPosY, pString, &fWidth );
            fX += fWidth;
        }
        m_CurrentString += pString;

        INT iStrLen = wcslen( pString );
        pString += iStrLen + 1;
    }

    // We don't count m_dwCharsInXime correctly in Backspace, ESC and etc...
    // So we refresh m_dwCharsInXime every time.
    m_dwCharsInXime = m_CurrentString.length() - dwDeterminedLength;

    //Show cursor
    if( (DWORD)color == COLOR_FOCUS && m_CurrentLanguage == MODE_KR )
    {
        // drew box cursor in DrawText
    }
    else if( dwCursor >= 0 && dwCursor <= (DWORD)m_CurrentString.length() )
    {
        // Measure the cursor position
        pString = (WCHAR* )m_CurrentString.c_str();
        XUIRect clipRect( 0, 0, m_BackBufferWidth, m_BackBufferHeight );
        if( dwCursor )
            XuiMeasureText( hFont, pString, dwCursor,
                            XUI_FONT_STYLE_NORMAL | XUI_FONT_STYLE_SINGLE_LINE,
                            0, &clipRect );
        else
            clipRect.right = 0; //Work around for XuiMeasureText behaivor

        // Draw the cursor if it's inside a screen
        color = COLOR_NORMAL;
        if( fPosX + clipRect.right < m_BackBufferWidth )
            DrawText( hDC, hFont, color, fPosX + clipRect.right, fPosY, L"|" );
    }

    // Complete Xui rendering
    XuiRenderEnd( hDC );
    XuiRenderPresent( hDC, NULL, NULL, NULL );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: DrawText
// Desc: Draw text at the given coordinates with the given color.
//--------------------------------------------------------------------------------------
void Xime::DrawText( HXUIDC hdc, HXUIFONT hFont, D3DCOLOR color, float x, float y,
                     LPCWSTR text, float * pfX, float * pfY )
{
    assert( hdc != NULL );
    assert( hFont != NULL );
    assert( text );

    // Measure the text
    XUIRect clipRect( 0, 0, m_BackBufferWidth - x, m_BackBufferHeight - y );
    XuiMeasureText( hFont, text, -1, XUI_FONT_STYLE_NORMAL | XUI_FONT_STYLE_SINGLE_LINE,
                    0, &clipRect );

    // Set the text position in the device context
    D3DXMATRIX matXForm;
    D3DXMatrixIdentity( &matXForm );
    matXForm._41 = x;
    matXForm._42 = y;
    XuiRenderSetTransform( hdc, &matXForm );

    // Select the font and color into the device context
    XuiSelectFont( hdc, hFont );
    XuiSetColorFactor( hdc, (DWORD)color );

    if( (DWORD)color == COLOR_FOCUS && m_CurrentLanguage == MODE_KR )
    {
        HXUIBRUSH hBrush;
        XuiCreateSolidBrush( (DWORD)color, &hBrush );
        XuiSelectBrush( hdc, hBrush );
        XuiFillRect( hdc, &clipRect );
        XuiDestroyBrush( hBrush );
        XuiSetColorFactor( hdc, 0xFF000000 );
    }

    // Draw the text
    XuiDrawText( hdc, text, XUI_FONT_STYLE_NORMAL, 0, &clipRect );

    if( pfX != NULL ) *pfX = clipRect.GetWidth();
    if( pfY != NULL ) *pfY = clipRect.GetHeight();
    return;
}


//--------------------------------------------------------------------------------------
// Callback functions
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Name: Convert
// Desc: Start a conversion process
//--------------------------------------------------------------------------------------
HRESULT Xime::Convert( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );

    //Input some letters
    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    // save the keystroke for pop back when this call is not taken the key.
    pXIME->m_RequestList.back().key = pXIME->m_Key;

    if( !pXIME->m_dwCharsInXime )
    {
        // Just input white space
        XIMEInsertCharacter( L' ',
                             XIME_KEY_UNICODE,
                             pXIME->m_iInputMode,
                             &pXIME->m_RequestList.back().ov );

    }
    else
    {
        pXIME->SetCurrentMode( MODE_CONVERT );

        // do convert
        XIMEInsertCharacter( 0,
                             XIME_KEY_CONVERT,
                             pXIME->m_iInputMode,
                             &pXIME->m_RequestList.back().ov );

        // Build candidate list
        BuildCandidateList( pXIME, pKey );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: NonConvert
// Desc: Process the [No convert] key 
//--------------------------------------------------------------------------------------
HRESULT Xime::NonConvert( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    // #143421, when there are no reading strings, do nothing!
    if( !pXIME->m_dwCharsInXime )
    {   
        return S_OK;
    }

    // start the convert 
    Convert( pXIME, pKey );
    pXIME->SetCurrentMode( MODE_CONVERT );

    // and set the XIMEKEY
    SetCandidate( pXIME, pKey );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: ChangeFocus
// Desc: Chang clause focus
//--------------------------------------------------------------------------------------
HRESULT Xime::ChangeFocus( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );
    assert( pXIME->m_CurrentImeMode == MODE_CONVERT );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    BOOL bMoveToRight = ( pKey->wVK == VK_RIGHT ? TRUE : FALSE );
    XIMEMoveClauseFocus( bMoveToRight, &pXIME->m_RequestList.back().ov );

    BuildCandidateList( pXIME, pKey );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: ChangeClauseLength
// Desc: Change clause length in focus
//--------------------------------------------------------------------------------------
HRESULT Xime::ChangeClauseLength( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );
    assert( pXIME->m_CurrentImeMode == MODE_CONVERT );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    BOOL bExpand = ( pKey->wVK == VK_RIGHT ? TRUE : FALSE );
    XIMEChangeClauseLength( bExpand, &pXIME->m_RequestList.back().ov );

    BuildCandidateList( pXIME, pKey );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: Revert
// Desc: Revert conversion
//--------------------------------------------------------------------------------------
HRESULT Xime::Revert( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );
    assert( pXIME->m_CurrentImeMode == MODE_CONVERT );

    pXIME->SetCurrentMode( MODE_INPUT );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    XIMERevertString( &pXIME->m_RequestList.back().ov );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: SetCandidate
// Desc: Notify a candidate to IME
//--------------------------------------------------------------------------------------
HRESULT Xime::SetCandidate( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );
    assert( pXIME->m_CurrentImeMode == MODE_CONVERT );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    XIMESetCandidateIndex( (XIMEINDEX)pKey->eControlKey,
                           0,
                           pXIME->m_CurrentLanguage == MODE_JP,
                           &pXIME->m_RequestList.back().ov );
   
    pXIME->m_bShowCandidateList = TRUE;
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: BuildCandidateList
// Desc: Request candidate list to IME
//--------------------------------------------------------------------------------------
HRESULT Xime::BuildCandidateList( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );
    XIMEBuildCandidateList( &pXIME->m_RequestList.back().ov );

    pXIME->m_bShowCandidateList = (pXIME->m_CurrentLanguage == MODE_TC);

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: SwitchLanguage
// Desc: Switch Language for IME, JP->KR->CH
//--------------------------------------------------------------------------------------
HRESULT Xime::SwitchLanguage(Xime * pXIME)
{
    switch( pXIME->m_CurrentLanguage )
    {
    case MODE_JP:

#ifdef DEBUG_USER_DIC_TEST
        if ( XIMEUpdateUserDicMemory(&s_imeOverLapped) == ERROR_IO_INCOMPLETE )
        {
           // Wait for overlapped function to complete
           while (!XHasOverlappedIoCompleted(&s_imeOverLapped))
               Sleep(50);
           HRESULT result = XGetOverlappedResult( &s_imeOverLapped, NULL, TRUE );
           if (result == ERROR_SUCCESS)
           {
               OutputDebugStringA( "update user dic successfully!!\n" );
           }
           else
           {
               OutputDebugStringA( "update user dic error!!\n" );
           }
        }
  
        SaveUserDic( m_UserDicMemory, sizeof(m_UserDicMemory) );

#endif  // DEBUG_USER_DIC_TEST

        pXIME->SetInputLanguage( MODE_KR );
        break;
    case MODE_KR:
        pXIME->SetInputLanguage( MODE_TC );
        break;
    case MODE_TC:
        pXIME->SetInputLanguage( MODE_JP );
        break;
    default:
        return E_FAIL;
    }

    return S_OK;
}
 
//--------------------------------------------------------------------------------------
// Name: Complete
// Desc: Complete IME conversion
//--------------------------------------------------------------------------------------
HRESULT Xime::Complete( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    static int c = 0;

    assert( pXIME != NULL );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    if( pXIME->m_CurrentLanguage == MODE_TC &&
        pXIME->m_CurrentImeMode == MODE_CONVERT &&
        pKey != NULL )
    {
        // if it's not auto-completion before XIME_KEY_UNICODE,
        // CHT Bopomofo has to complete after the candidate index is updated.
        pXIME->m_RequestList.back().dwRequestId = NEED_TO_COMPLETE;
        pXIME->m_bNeedBlocking = TRUE;

        //XIMESetCandidateIndex( XIME_INDEX_CURRENT, 0, TRUE, &pXIME->m_RequestList.back().ov );
    }
    else
    {
        // save the keystroke for pop back when this call is not taken the key.
        pXIME->m_RequestList.back().key = (pKey != NULL)? pXIME->m_Key : XINPUT_KEYSTROKE();
        
        XIMEInsertCharacter( 0,
                             XIME_KEY_COMPLETE,
                             pXIME->m_iInputMode,
                             &pXIME->m_RequestList.back().ov );

        
        // We always change the mode to MODE_INPUT, and
        // we will get the completed string after XIME_KEY_COMPLETE is done.
        pXIME->SetCurrentMode( MODE_INPUT );
        pXIME->m_bCompleted = TRUE;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: HandleNumpadConversion
// Desc: Handle numpad inputs
//--------------------------------------------------------------------------------------
HRESULT Xime::HandleNumpadConversion( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );

    if( !pXIME->m_bShowCandidateList )
        return S_FALSE;

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    DWORD dwIndex = (pKey->wVK >= VK_NUMPAD0)? pKey->wVK-VK_NUMPAD1 : pKey->wVK-VK_1;
    if( dwIndex > 9)
        dwIndex = 10;

    DWORD dwRet = XIMESetCandidateIndex( XIME_INDEX_CURRENT_PAGE_OFFSET,
                                         dwIndex, true,
                                         &pXIME->m_RequestList.back().ov );

    //
    // we can't process the next command until XIMESetCandidateIndex completes successfully
    //
    if( dwRet == ERROR_SUCCESS || dwRet == ERROR_IO_INCOMPLETE )
    {
        if( pXIME->m_CurrentLanguage == MODE_JP )
        {
            pXIME->m_RequestList.back().dwRequestId = NEED_TO_DISABLE_LIST;
        }
        else if( pXIME->m_CurrentLanguage == MODE_TC )
        {
            // CHT Bopomofo needs to complete after the candidate index is updated.
            pXIME->m_RequestList.back().dwRequestId = NEED_TO_COMPLETE;
            pXIME->m_bNeedBlocking = TRUE;
        }
    }
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: AllConvert
// Desc: Start a conversion process
//--------------------------------------------------------------------------------------
HRESULT Xime::AllConvert( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );

    pXIME->SetCurrentMode( MODE_ALLCONVERT );
    //Input some letters
    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    // save the keystroke for pop back when this call is not taken the key.
    pXIME->m_RequestList.back().key = pXIME->m_Key;
    XIMEInsertCharacter( 0,
                         pKey->eControlKey,
                         pXIME->m_iInputMode,
                         &pXIME->m_RequestList.back().ov );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: AllRevert
// Desc: Revert all conversion
//--------------------------------------------------------------------------------------
HRESULT Xime::AllRevert( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );
    assert( pXIME->m_CurrentImeMode == MODE_ALLCONVERT );

    pXIME->SetCurrentMode( MODE_INPUT );

    XIMEREQUEST request = { 0 };
    pXIME->m_RequestList.push_back( request );

    // save the keystroke for pop back when this call is not taken the key.
    pXIME->m_RequestList.back().key = pXIME->m_Key;

    XIMEInsertCharacter(  0,
                          XIME_KEY_REVERT,
                          pXIME->m_iInputMode,
                          &pXIME->m_RequestList.back().ov );

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Name: ChangeInputMode
// Desc: Change IME input mode
//--------------------------------------------------------------------------------------
HRESULT Xime::ChangeInputMode( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );
    assert( pXIME->m_CurrentImeMode == MODE_INPUT );

    pXIME->m_iInputMode = pKey->eControlKey;

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: SetCurrentMode
// Desc: Switch current IME mode
//--------------------------------------------------------------------------------------
void    Xime::SetCurrentMode( IMEMODE mode )
{
    assert( mode == MODE_OFF || mode == MODE_CONVERT || mode == MODE_INPUT || mode == MODE_ALLCONVERT );

    if( m_CurrentImeMode != mode )
    {
        m_bNeedBlocking = TRUE;
    }
    m_CurrentImeMode = mode;
}


//--------------------------------------------------------------------------------------
// Name: ToggleIME
// Desc: Toggle IME
//--------------------------------------------------------------------------------------
HRESULT Xime::ToggleIME( Xime * pXIME )
{
    assert( pXIME != NULL );

    if ( ( pXIME->m_CurrentLanguage == MODE_JP ) && pXIME->m_Key.VirtualKey )   // if it's not from Gamepad
    {
        if ( XGetLanguage() == XC_LANGUAGE_JAPANESE )
        {
            if ( pXIME->m_Key.Unicode )                                         // no change if it's VK_OEM_3 when dash is Japanese
                return pXIME->m_bIMEOn ? S_FALSE : S_OK;
        }
        else
        {
            if ( pXIME->m_Key.Unicode != L'`' )                                 // only toggle IME if it's [~/`] key when dash is non-Japanese
                return pXIME->m_bIMEOn ? S_FALSE : S_OK;
        }
    }

    if( pXIME->m_bIMEOn )
    {
        // We like Flush as IME toggle behavior.
        XIMEREQUEST request = { 0 };
        pXIME->m_RequestList.push_back( request );

        // #142425, need to send XIME_KEY_FLUSH instead of XIME_KEY_COMPLETE For chinese traditional(MODE_TC)
        if( pXIME->m_CurrentImeMode == MODE_INPUT )
        {
            XIMEInsertCharacter( 0,
                             pXIME->m_CurrentLanguage == MODE_TC ? XIME_KEY_FLUSH : XIME_KEY_COMPLETE,
                             pXIME->m_iInputMode,
                             &pXIME->m_RequestList.back().ov );
        }
        else
        {
            XIMEInsertCharacter( 0,
                                 XIME_KEY_COMPLETE,
                                 pXIME->m_iInputMode,
                                 &pXIME->m_RequestList.back().ov );
        }
        

        pXIME->m_bCompleted = TRUE;
        // Need to block since input mode has been changed
        pXIME->SetCurrentMode( MODE_OFF );
        pXIME->m_bIMEOn = FALSE;

        // drop this keystroke.
        ZeroMemory( &pXIME->m_Key, sizeof( XINPUT_KEYSTROKE ) );
    }
    else
    {
        pXIME->SetCurrentMode( MODE_INPUT );
        pXIME->m_bIMEOn = TRUE;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: ToggleKeyboardLayout
// Desc: Toggle Keyboard layout
//--------------------------------------------------------------------------------------
HRESULT Xime::ToggleKeyboardLayout( Xime * pXIME, XIME_KEYMAPPING * pKey )
{
    assert( pXIME != NULL );
    assert( pKey != NULL );

    if( pXIME->m_CurrentLanguage == MODE_JP )
    {
        if( pXIME->m_iKeyboardLayout == XIME_LAYOUT_ALPHANUMERIC )
        {
            pXIME->m_iKeyboardLayout = XIME_LAYOUT_KANA;
        }
        else
        {
            pXIME->m_iKeyboardLayout = XIME_LAYOUT_ALPHANUMERIC;
        }
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: LayoutConversion
// Desc: Keyboard layout conversion routine. Language specific
//--------------------------------------------------------------------------------------
HRESULT Xime::LayoutConversion( XINPUT_KEYSTROKE* pKey, INT* pInputMode )
{
    if( m_CurrentLanguage == MODE_JP )
    {
        if( m_iKeyboardLayout == XIME_LAYOUT_KANA && 
            ( ( *pInputMode & XIME_LAYOUT_MASK ) == XIME_LAYOUT_KANA ||
            ( *pInputMode & XIME_COMBINE_MASK ) == XIME_COMBINE_ROMAJI_HIRAGANA ||
            ( *pInputMode & XIME_COMBINE_MASK ) == XIME_COMBINE_ROMAJI_KATAKANA ) )
        {
            HidUnicodeLookUp( pKey,
                              m_bIMEOn,
                              m_iInputMode == XIME_MODE_JP_ROMAJI_KATAKANA_WITH_FULLWIDTH_ALPHANUMERIC,
                              JapaneseHiraganaNonAlphabetic,
                              JapaneseHiraganaAlphabetic );
            *pInputMode = XIME_LANGUAGE_JAPANESE | XIME_LAYOUT_KANA |
                          ( pKey->Flags & XINPUT_KEYSTROKE_SHIFT ? XIME_COMBINE_KANA_SHIFT : 0 );
        }
        else
        {
            // for ten-key input
            HidUnicodeLookUp( pKey, FALSE, FALSE, NULL, NULL );
        }
    }
    else if( m_CurrentLanguage == MODE_KR )
    {
        if( XGetLanguage() != XC_LANGUAGE_KOREAN && pKey->HidCode == 0xe6 ) // Right-ALT key
        {
            pKey->VirtualKey = VK_HANGUL;
            pKey->Flags &= ~( XINPUT_KEYSTROKE_ALT | XINPUT_KEYSTROKE_ALTGR );
        }
        else
        {
            HidUnicodeLookUp( pKey,
                              m_bIMEOn,
                              FALSE,
                              KoreanNonAlphabetic,
                              KoreanAlphabetic );
        }
        *pInputMode = XIME_LANGUAGE_HANGUL;// | XIME_LAYOUT_HANGUL;
    }
    else if( m_CurrentLanguage == MODE_TC )
    {
        HidUnicodeLookUp( pKey,
                          m_bIMEOn,
                          FALSE,
                          ChineseTraditionalBopomofoNonAlphabetic,
                          ChineseTraditionalBopomofoAlphabetic );
        *pInputMode = XIME_LANGUAGE_BOPOMOFO; // | XIME_LAYOUT_BOPOMOFO;
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Name: HidUnicodeLookUp
// Desc: Look up Non-alphabet characters (Kana, Hangul, bopomofo)
//--------------------------------------------------------------------------------------
VOID Xime::HidUnicodeLookUp( XINPUT_KEYSTROKE* pKey, BOOL fImeOn, BOOL fKana,
                              HID_LOOKUP_TABLE * pTable, HID_LOOKUP_TABLE * pTable2 )
{
    HID_LOOKUP_TABLE *pLookupTable = NULL;
    DWORD  dwHidCode = pKey->HidCode;

    // always look up even if IME is off
    if( ( dwHidCode >= 0x54 ) && ( dwHidCode <= 0x63 ) )
    {
        // convert ten-key
        pKey->Unicode = TenKeyTable[ dwHidCode - 0x54 ];
    }

    else if( !fImeOn )
    {
        // IME is off
    }

    // look up when IME is on
    else if( pTable && dwHidCode >= pTable[ 0 ].wHiragana && dwHidCode <= pTable[ 0 ].wHiraganaShifted )
    {
        pLookupTable = pTable;
    }
    else if( pTable2 &&  dwHidCode >= pTable2[ 0 ].wHiragana && dwHidCode <= pTable2[ 0 ].wHiraganaShifted )
    {
        pLookupTable = pTable2;
    }
    else if( dwHidCode == 0x87 )
    {
        //
        // Handle the special Japanese international keycode for the prolonged sound mark
        //
        pKey->Unicode = fKana ? 0x30ED : 0x308D; // ro character
    }
    else if( dwHidCode == 0x89 )
    {
        pKey->Unicode = 0x30FC; // Prolonged sound mark
    }

    if( pLookupTable )
    {
        dwHidCode -= pLookupTable[ 0 ].wHiragana-1;
        if( fKana )
        {
            if(pKey->Flags & XINPUT_KEYSTROKE_SHIFT)
                pKey->Unicode = pLookupTable[ dwHidCode ].wKatakanaShifted;
            else
                pKey->Unicode = pLookupTable[ dwHidCode ].wKatakana;
        }
        else
        {
            if(pKey->Flags & XINPUT_KEYSTROKE_SHIFT)
                pKey->Unicode = pLookupTable[ dwHidCode ].wHiraganaShifted;
            else
                pKey->Unicode = pLookupTable[ dwHidCode ].wHiragana;
        }
    }

    return;
}

//--------------------------------------------------------------------------------------
// Name: XimeposLookup
// Desc: Ximepos STRING->Value lookup, JP mode only
//--------------------------------------------------------------------------------------
XIMEPOS Xime::XimeposLookup(LPCWSTR pStr)
{
    XIMEPOS_LOOKUP_TABLE *pTable = XimeposTable;
    for( UINT i = 0; i < sizeof( XimeposTable ) / sizeof( XIMEPOS_LOOKUP_TABLE ) ; i++, pTable++ )
    {
        if ( _wcsicmp( pStr, pTable->string ) == 0 )
            return pTable->value;
    }

    return (XIMEPOS)0;
}

//--------------------------------------------------------------------------------------
// Name: LoadJPUserWordFile
// Desc: Load and Register Word to from XML file,JP mode only
//--------------------------------------------------------------------------------------
HRESULT Xime::LoadJPUserWordFile(const CHAR* strUserWordFile)
{
    if( m_CurrentLanguage != MODE_JP )
        return S_FALSE;

    XMLParser wordParser;
    UserWordFileCallback wordCallback;

    wordParser.RegisterSAXCallbackInterface( &wordCallback );
    HRESULT hr = wordParser.ParseXMLFile( strUserWordFile );
    
    UpdateJPUserWordNumber();

    return hr;
}

//--------------------------------------------------------------------------------------
// Name: RegisterJPUserWord
// Desc: Register one word, JP mode only
//--------------------------------------------------------------------------------------
HRESULT Xime::RegisterJPUserWord(LPCWSTR pReading, LPCWSTR pDisplay, LPCWSTR pImePos)
{
    DWORD addtionResult;
    BOOL fComplete;

    XOVERLAPPED myXOverLapped = { 0 };

    if ( pReading[ 0 ] && pDisplay[ 0 ] && pImePos[ 0 ] )
    {
        XIMEPOS pos = Xime::XimeposLookup( pImePos );
        if ( pos )
        {
            //if ( XIMERegisterUserWord( pReading, pDisplay, pos, &s_imeOverLapped ) == ERROR_IO_INCOMPLETE )
            if ( XIMERegisterUserWord( pReading, pDisplay, pos, &myXOverLapped ) == ERROR_IO_INCOMPLETE )
            {
                // Wait for overlapped function to complete
                while ( ( fComplete = XHasOverlappedIoCompleted( &myXOverLapped ) ) == FALSE ) 
                    SwitchToThread();

                DWORD result = XGetOverlappedResult( &myXOverLapped, &addtionResult, TRUE );
                
                if ( result == ERROR_SUCCESS )
                {
                    return S_OK;
                    // registered successfully
                }
                else if ( result == ERROR_ALREADY_EXISTS )
                {
                    return S_OK;
                    // already registered
                }

                return S_OK;
            }
        }
    }

    return S_FALSE;
}

HRESULT Xime::DeleteJPUserWordAll()
{
    if( m_CurrentLanguage != MODE_JP )
        return S_FALSE;

    XIMEDeleteUserWord( XIME_DELETE_ALL_USER_WORDS, &s_imeOverLapped );
    while ( !XHasOverlappedIoCompleted( &s_imeOverLapped ) )
        Sleep( 50 );

    HRESULT result = XGetOverlappedResult( &s_imeOverLapped, NULL, TRUE );
    
    UpdateJPUserWordNumber();
    if ( result == ERROR_SUCCESS )
        return S_OK;
    else
        return S_FALSE;
}

//--------------------------------------------------------------------------------------bb
// Name: UpdateJPUserWordNumber
// Desc: Get the number of words that has been registered, JP mode only
//--------------------------------------------------------------------------------------
void Xime::UpdateJPUserWordNumber()
{
    if( m_CurrentLanguage != MODE_JP )
        return;

    // Max number of user registered words is 100
    DWORD number = 100;
    
    //XIME_USER_WORD XimeUserWords[100];
    //XIMEEnumerateUserWords( 0, &number, XimeUserWords, &s_imeOverLapped );
    DWORD dwRet = XIMEEnumerateUserWords( 0, &number, NULL, &s_imeOverLapped );
    if( dwRet != ERROR_IO_INCOMPLETE )
    {   
        m_JPUserWord = 0;
        return;
    }

    while ( !XHasOverlappedIoCompleted( &s_imeOverLapped ) )
        Sleep( 50 );

    m_JPUserWord = number;
}

};// namespace ATG
