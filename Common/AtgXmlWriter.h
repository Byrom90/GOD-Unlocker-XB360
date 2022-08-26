//-------------------------------------------------------------------------------------
//  AtgXmlWriter.h
//  
//  A simple XML writer.
//  
//  Xbox Advanced Technology Group
//  Copyright (C) Microsoft Corporation. All rights reserved.
//-------------------------------------------------------------------------------------

#pragma once
#ifndef ATGXMLWRITER_H
#define ATGXMLWRITER_H

#include <vector>
#include <xtl.h>

namespace ATG
{

#define XMLWRITER_NAME_STACK_SIZE 255

class XMLWriter
{
public:
                XMLWriter();
                XMLWriter( CHAR* strBuffer, UINT uBufferSize );
                XMLWriter( const CHAR* strFileName );
                ~XMLWriter();

    VOID        Initialize( CHAR* strBuffer, UINT uBufferSize );
    VOID        Initialize( const CHAR* strFileName );
    VOID        Close();

    VOID        SetIndentCount( UINT uSpaces );
    VOID        EnableNewlines( BOOL bWriteNewLines )
    {
        m_bWriteNewlines = bWriteNewLines;
    }

    BOOL        StartElement( const CHAR* strName );
    BOOL        EndElement();
    BOOL        WriteElement( const CHAR* strName, const CHAR* strBody );
    BOOL        WriteElement( const CHAR* strName, INT iBody );
    BOOL        WriteElement( const CHAR* strName, FLOAT fBody );
    BOOL        WriteElementFormat( const CHAR* strName, _In_z_ _Printf_format_string_ const CHAR* strFormat, ... );

    BOOL        StartCDATA();
    BOOL        EndCDATA();
    BOOL        WriteCDATA( const CHAR* strData, DWORD dwDataLength );

    BOOL        StartComment( BOOL bInline = FALSE );
    BOOL        EndComment();
    BOOL        WriteComment( const CHAR* strComment, BOOL bInline = FALSE );

    BOOL        AddAttributeFormat( const CHAR* strName, _In_z_ _Printf_format_string_ const CHAR* strFormat, ... );
    BOOL        AddAttribute( const CHAR* strName, const CHAR* strValue );
    BOOL        AddAttribute( const CHAR* strName, const WCHAR* wstrValue );
    BOOL        AddAttribute( const CHAR* strName, INT iValue );
    BOOL        AddAttribute( const CHAR* strName, FLOAT fValue );

    BOOL        WriteString( const CHAR* strText );
    BOOL        WriteStringFormat( _In_z_ _Printf_format_string_ const CHAR* strFormat, ... );

private:

    VOID        PushName( const CHAR* strName );
    const CHAR* PopName();

    inline BOOL EndOpenTag();
    inline BOOL WriteNewline();
    inline BOOL WriteIndent();

    inline BOOL OutputString( const CHAR* strText );
    inline BOOL OutputStringFast( const CHAR* strText, UINT uLength );
    VOID FlushBufferToFile();

    HANDLE          m_hFile;
    CHAR*           m_strBuffer;
    CHAR*           m_strBufferStart;
    UINT            m_uBufferSizeRemaining;

    CHAR            m_strNameStack[XMLWRITER_NAME_STACK_SIZE];
    CHAR*           m_strNameStackTop;
    UINT            m_uNameStackSize;
    std::vector<UINT>    m_NameStackPositions;
    UINT            m_uIndentCount;
    CHAR            m_strIndent[9];
    BOOL            m_bWriteNewlines;

    BOOL            m_bOpenTagFinished;
    BOOL            m_bWriteCloseTagIndent;
    BOOL            m_bInlineComment;
};

} // namespace ATG

#endif
