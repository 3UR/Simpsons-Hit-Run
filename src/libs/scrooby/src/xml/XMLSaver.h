//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   XML Saver
//
// Description: This contains the XMLSaver class which allows xml type output
//
// Authors:     Eric Tong
//
//===========================================================================

// Recompilation protection flag.
#ifndef __XMLSAVER_H__
#define __XMLSAVER_H__

// The debugger can't handle symbols more than 255 characters long.
// STL often creates symbols longer than that.
// When symbols are longer than 255 characters, the warning is disabled.
#pragma warning(disable:4786)

//===========================================================================
// Nested Includes
//===========================================================================

#include <deque>
#include "Strings\PCString.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

//===========================================================================
// Forward References
//===========================================================================

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

struct AttributeStruct 
{
    PascalCString m_name;
    PascalCString m_value;
};

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to save xml compatible output
//
// Constraints: None
//
//===========================================================================

class XMLSaver
{
    public:
        XMLSaver();
        virtual ~XMLSaver();

        bool OpenFile( const char* filename );
        bool CloseFile();

        bool PushElement( const char* name );
        bool PopElement();

        bool AddAttribute( const char* name, const char* value );
        bool AddAttribute( const char* name, int value );
        bool AddAttribute( const char* name, double value );
        bool AddAttribute( const char* name, bool value );
        bool FlushAttributes();

    protected:

    private:
        bool PrintTabs();
        bool PrintNewLine();
        bool Print( const char* string );

        // Create this methods as private so they can not be used
        XMLSaver(const XMLSaver &right);
        const XMLSaver & operator = (const XMLSaver &right);

        // Constants
        static const int m_tabSize;
        static const int m_maxValueLength;

        // Data members
        std::deque< AttributeStruct* > m_attributes;
        std::deque< PascalCString* > m_elementNames;

        HANDLE m_hFile;
        int m_numTabs;
        bool m_justPushed;
        bool m_justPopped;
};

//===========================================================================
// Inlines
//===========================================================================

#endif                                  // End conditional inclusion
