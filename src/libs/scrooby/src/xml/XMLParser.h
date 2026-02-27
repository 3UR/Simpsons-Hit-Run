//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FE Builder Data Structure
//
// Description: This contains the XMLParser class which parses an
//              XML file and contructs a tree
//
// Authors:     Eric Tong
//
//===========================================================================

// Recompilation protection flag.
#ifndef _XMLPARSER_H_
#define _XMLPARSER_H_

//===========================================================================
// Nested Includes
//===========================================================================
#include "utility/rVector.hpp"
#include "strings/pcstring.h"

//===========================================================================
// Forward References
//===========================================================================
class StreamReader;

//===========================================================================
// Constants, Typedefs, and Macro Definitions (needed by external clients)
//===========================================================================

enum XMLNodeType
{
    NormalTag = 0,
    EndTag,
    SingleTag,
    UnknownTag
};

class XMLAttribute
{
    public:
        PascalCString GetName();
        PascalCString GetValue();

    public:
        PascalCString m_name;
        PascalCString m_value;
};

class XMLAttributeList
{
    public:
        int GetLength();
        void AddItem( XMLAttribute* attribute );
        XMLAttribute* GetItem( int index );

    private:
        rVector<XMLAttribute*> m_attributes;
};

class XMLNode;

class XMLNodeList
{
    public:
        int GetLength();
        void AddItem( XMLNode* node );
        XMLNode* GetItem( int index );

    private:
        rVector<XMLNode*> m_childNodes;
};

class XMLNode
{
    public:
        XMLNode();
        virtual ~XMLNode();

        XMLNodeType GetType();
        PascalCString GetName();
        XMLAttributeList* GetAttributes();
        XMLNodeList* GetChildNodes();

    public:
        XMLNodeType m_nodeType;
        PascalCString m_tagName;
        XMLAttributeList m_attributes;
        XMLNodeList m_childNodes;
};

//===========================================================================
// Interface Definitions
//===========================================================================

//===========================================================================
//
// Description: This class is used to parse a xml file and constructs a tree
//
// Constraints: None
//
//===========================================================================

class XMLParser  
{
    public:
        XMLParser();
        virtual ~XMLParser();
        
        XMLNode* ParseFromFile( const char* filename );
        XMLNode* ParseFromBuffer( char* buffer, 
                                  const unsigned int bufferSize );

    private:
        XMLNode* Parse( StreamReader* stream );
        XMLNode* Parse( char*& buffer );
        PascalCString ReadNextTag( StreamReader* stream );
        PascalCString ReadNextTag( char*& buffer );
        XMLNode* DecodeTag( PascalCString& tagString );
        XMLNode* DecodeTag( char*& buffer );

        PascalCString m_filename;
        XMLNode m_rootNode;
};

//===========================================================================
// Inlines
//===========================================================================

#endif                                  // End conditional inclusion
