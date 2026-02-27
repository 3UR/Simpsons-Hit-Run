//===========================================================================
// Copyright (C) 2000 Radical Entertainment Ltd.  All rights reserved.
//
// Component:   FE Builder Data Structure
//
// Description: This contains the XMLParser class.
//
// Authors:     Eric Tong
//
//===========================================================================

//===========================================================================
// Includes
//===========================================================================
#include "xml/XMLParser.h"

#include <assert.h>
#include <string.h>
#ifdef SCROOBY_TOOL
#include "utility/StreamReader.h"
#else
#include "utility/BufferReader.h"
#endif
#include <utility/memory.h>

//===========================================================================
// Local Constants, Typedefs, and Macros
//===========================================================================

//===========================================================================
// Global Data, Local Data, Local Classes
//===========================================================================
PascalCString XMLAttribute::GetName()
{
    return m_name;
}

PascalCString XMLAttribute::GetValue()
{
    return m_value;
}

int XMLAttributeList::GetLength()
{
    return m_attributes.Size();
}

void XMLAttributeList::AddItem( XMLAttribute* attribute )
{
    m_attributes.Insert( attribute );
}

XMLAttribute* XMLAttributeList::GetItem( int index )
{
    return m_attributes[ index ];
}

int XMLNodeList::GetLength()
{
    return m_childNodes.Size();
}

void XMLNodeList::AddItem( XMLNode* node )
{
    m_childNodes.Insert( node );
}

XMLNode* XMLNodeList::GetItem( int index )
{
    return m_childNodes[ index ];
}

XMLNode::XMLNode()
{
    m_nodeType = NormalTag;
    m_tagName = "";
}

XMLNode::~XMLNode()
{
    p3d::AllocType type = p3d::GetCurrentAlloc();
    p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );
    // Delete all attributes
    int length = m_attributes.GetLength();
    int i;
    for( i = 0; i < length; i++ )
    {
        XMLAttribute* attribute = m_attributes.GetItem( i );
        if( attribute != NULL )
        {
            delete attribute;
            attribute = NULL;
        }
    }

    // Delete all child nodes
    length = m_childNodes.GetLength();
    for( i = 0; i < length; i++ )
    {
        XMLNode* childNode = m_childNodes.GetItem( i );
        if( childNode != NULL )
        {
            delete childNode;
            childNode = NULL;
        }
    }
    p3d::SetCurrentAlloc( type );
}

XMLNodeType XMLNode::GetType()
{
    return m_nodeType;
}

PascalCString XMLNode::GetName()
{
    return m_tagName;
}

XMLAttributeList* XMLNode::GetAttributes()
{
    return &m_attributes;
}

XMLNodeList* XMLNode::GetChildNodes()
{
    return &m_childNodes;
}

//===========================================================================
// Member Functions
//===========================================================================

//===========================================================================
// XMLParser::XMLParser
//===========================================================================
// Description: XMLParser's constructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
XMLParser::XMLParser()
{
}

//===========================================================================
// XMLParser::~XMLParser
//===========================================================================
// Description: XMLParser's destructor
//
// Constraints:    None
//
// Parameters:    None
//
// Return:      Nothing
//
//===========================================================================
XMLParser::~XMLParser()
{
}

//===========================================================================
// XMLParser::ParseFromFile
//===========================================================================
// Description: This parses the file and constructs the tree
//
// Constraints:    None
//
// Parameters:    filename - the file name
//
// Return:      XMLNode* the pointer to the root node
//
//===========================================================================
XMLNode* XMLParser::ParseFromFile( const char* filename )
{
    p3d::AllocType type = p3d::GetCurrentAlloc();
    p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );

    m_filename = filename;

/*    //IAN IMPROVE: this is just till darwin checks in some of his stuff
    char* pBuffer ;
    struct _stat fileStats ;
    _stat( filename, &fileStats ) ;
    int total = fileStats.st_size  ;

    //read in the file
    pBuffer = new char[ total ] ;
    assert( pBuffer != NULL ) ;
    ifstream infile ;
    infile.open( filename, ios::in | ios::binary ) ;
    assert( infile.is_open() ) ;
    infile.read( pBuffer, total ) ;
*/

    StreamReader stream( filename );
    XMLNode* rootNode = new XMLNode;
    rootNode->m_tagName = "Root";
    rootNode->m_nodeType = NormalTag;

    XMLNode* childNode = Parse( &stream );
    while( childNode != NULL )
    {
        // Only add if valid tag
        if( ( childNode->m_nodeType == NormalTag ) || 
            ( childNode->m_nodeType == SingleTag ) ) 
        {
            rootNode->m_childNodes.AddItem( childNode );
        }
        else
        {
            delete childNode;
        }
        childNode = Parse( &stream );
    }
    p3d::SetCurrentAlloc( type );

    return rootNode;
}


//===========================================================================
// XMLParser::ParseFromBuffer
//===========================================================================
// Description: This parses the file and constructs the tree
//
// Constraints:    None
//
// Parameters:    filename - the file name
//
// Return:      XMLNode* the pointer to the root node
//
//===========================================================================
XMLNode* XMLParser::ParseFromBuffer
( 
    char* buffer,
    const unsigned int bufferSize
)
{
    p3d::AllocType type = p3d::GetCurrentAlloc();
    p3d::SetCurrentAlloc( p3d::ALLOC_TEMPORARY );

    m_filename = "";
    //StreamReader stream( buffer, bufferSize );

    XMLNode* rootNode = new XMLNode;
    rootNode->m_tagName = "Root";
    rootNode->m_nodeType = NormalTag;

    //XMLNode* childNode = Parse( &stream );
    XMLNode* childNode = Parse( buffer );
    while( childNode != NULL )
    {
        // Only add if valid tag
        if( ( childNode->m_nodeType == NormalTag ) || 
            ( childNode->m_nodeType == SingleTag ) ) 
        {
            rootNode->m_childNodes.AddItem( childNode );
        }
        else
        {
            delete childNode;
        }
        //childNode = Parse( &stream );
        childNode = Parse( buffer );
    }
    p3d::SetCurrentAlloc( type );

    return rootNode;
}


//===========================================================================
// XMLParser::Parse
//===========================================================================
// Description: This parses the file and constructs the tree
//
// Constraints:    None
//
// Parameters:    stream - the current opened file stream
//
// Return:      XMLNode* the pointer to the current root node
//
//===========================================================================
XMLNode* XMLParser::Parse( StreamReader* stream )
{
    PascalCString decodeMe = ReadNextTag( stream );
    if( decodeMe.Length() > 0 )
    {
        XMLNode* node = DecodeTag( decodeMe );
        if( node != NULL )
        {
            if( node->m_nodeType == NormalTag ) 
            {
                XMLNode* childNode = Parse( stream );
                while( childNode != NULL )
                {
                    // Only add if valid tag
                    if( ( childNode->m_nodeType == NormalTag ) || 
                        ( childNode->m_nodeType == SingleTag ) ) 
                    {
                        node->m_childNodes.AddItem( childNode );
                    }
                    else
                    {
                        delete childNode;
                    }
                    childNode = Parse( stream );
                }
                return node;
            }
            else if( node->m_nodeType == SingleTag )
            {
                return node;
            }
            else if( node->m_nodeType == EndTag )
            {
                return node;
            }
            else if( node->m_nodeType == UnknownTag )
            {
                return node;
            }
        }
    }
    
    return NULL;
}

//===========================================================================
// XMLParser::Parse
//===========================================================================
// Description: This parses the file and constructs the tree
//
// Constraints:    None
//
// Parameters:    buffer - pointer to the text array that contains the input
//              data.  This pointer is advanced as reading occurs
//
// Return:      XMLNode* the pointer to the current root node
//
//===========================================================================
XMLNode* XMLParser::Parse( char*& buffer )
{
    PascalCString tagString = ReadNextTag( buffer );
    //char* tagString = ReadNextTag( buffer );
    if( tagString.Length() > 0 )
    {
        XMLNode* node = DecodeTag( tagString );
        if( node != NULL )
        {
            if( node->m_nodeType == NormalTag ) 
            {
                XMLNode* childNode = Parse( buffer );
                while( childNode != NULL )
                {
                    // Only add if valid tag
                    if( ( childNode->m_nodeType == NormalTag ) || 
                        ( childNode->m_nodeType == SingleTag ) ) 
                    {
                        node->m_childNodes.AddItem( childNode );
                    }
                    else
                    {
                        delete childNode;
                    }
                    childNode = Parse( buffer );
                }
                return node;
            }
            else if( node->m_nodeType == SingleTag )
            {
                return node;
            }
            else if( node->m_nodeType == EndTag )
            {
                return node;
            }
            else if( node->m_nodeType == UnknownTag )
            {
                return node;
            }
        }
    }
    return NULL;
}

//===========================================================================
// XMLParser::ReadNextTag
//===========================================================================
// Description: Reads the next tag from buffer and parses it
//
// Constraints:    None
//
// Parameters:    buffer - char pointer that points to the input data. This 
//              pointer is advanced.
//
// Return:      PascalCString - returns the tag as a string
//
//===========================================================================
PascalCString XMLParser::ReadNextTag( char*& buffer )
{
    PascalCString tag;

    // Reserve a nice big buffer so we don't have to keep growing after
    // every damn character.
    // 
    tag.Reserve( 128 );
    
    // First skip anything before the tag
    char c = buffer[ 0 ];
    while( ( c != '<' ) && ( c != '\0' ) )
    {
        buffer++;
        c = buffer[ 0 ];
    }

    buffer++;
    c = buffer[ 0 ];
   
    while( ( c != '>' ) && ( c != '\0' ) )
    {
        tag += c;
        buffer++;
        c = buffer[ 0 ];
    }

    return tag;
}

//===========================================================================
// XMLParser::ReadNextTag
//===========================================================================
// Description: Reads the next tag from buffer and parses it
//
// Constraints:    None
//
// Parameters:    stream - the current stream
//
// Return:      PascalCString - returns the tag as a string
//
//===========================================================================
PascalCString XMLParser::ReadNextTag( StreamReader* stream )
{
    PascalCString tag;

    // Reserve a nice big buffer so we don't have to keep growing after
    // every damn character.
    // 
    tag.Reserve( 128 );
    
    // First skip anything before the tag
    char c = stream->GetNextChar();
    while( ( c != '<' ) && ( c != '\0' ) )
    {
        c = stream->GetNextChar();
    }

    c = stream->GetNextChar();
    while( ( c != '>' ) && ( c != '\0' ) )
    {
        tag += c;
        c = stream->GetNextChar();
    }

    return tag;
}

//===========================================================================
// XMLParser::DecodeTag
//===========================================================================
// Description: Given the string, decode the tag type
//
// Constraints:    None
//
// Parameters:    tagString - the string to decode
//
// Return:      Constructs a node
//
//===========================================================================
XMLNode* XMLParser::DecodeTag( char*& buffer )
{
    XMLNode* node = new XMLNode;

    // Default tag type to normal
    node->m_nodeType = NormalTag;
    
    // Decode tag name
    node->m_tagName = ::strtok( buffer, " " );
    //node->m_tagName = tagString.StreamFirstWord( ' ' );

    // Is it an end tag?
    if( node->m_tagName.Find( '/' ) )
    {
        delete node;
        return NULL;
    }

    // Is it a comment tag?
    if( node->m_tagName.Find( '?' ) || node->m_tagName.Find( '!' ) )
    {
        node->m_nodeType = UnknownTag;
        return node;
    }

    // Get the attributes
    bool notDone = true;
    while( notDone )
    {
        PascalCString attributeString = ::strtok( buffer, " " );
        //PascalCString attributeString = tagString.StreamFirstWord( ' ' );

        if( attributeString != "" )
        {
            XMLAttribute* attribute = new XMLAttribute;
            // Get the attribute name
            attribute->m_name = attributeString.StreamFirstWord( '=' );
            
            // Get the attribute value.  Does attribute start with "quote"?
            if( attributeString.Find( '"' ) )
            {
                // So we found the first quote
                attributeString.StreamFirstWord( '"' );

                if( !attributeString.Find( '"' ) )
                {
                    // So we didn't get the second quote, so this string contains spaces
                    // so complete the string from the original buffer
                    // Remember to put the double quote back so we can continue parsing
                    attributeString += ' ';
                    attributeString += ::strtok( buffer, " " );
                    attributeString += '"';
                    
                    // Remove the space right after the quote (if there's any)
                    if( ::strpbrk( buffer, " " ) != NULL )
                    {
                        ::strtok( buffer, " " );
                        //tagString.StreamFirstWord( ' ' );
                    }
                    else if( ::strpbrk( buffer, "/" ) != NULL )
                    {
                        // Get the end slash if there is one
                        attributeString += ::strtok( buffer, "/" );
                        attributeString += '/';
                    }
                }
                attribute->m_value = attributeString.StreamFirstWord( '"' );
            }
            else
            {
                // We didn't find it, get the rest of the string
                attribute->m_value = attributeString.StreamFirstWord( '"' );
            }
            
            node->m_attributes.AddItem( attribute );

            // Determine the node type
            if( attributeString.Find( '/' ) )
            {
                node->m_nodeType = SingleTag;
            }
        }
        else
        {
            notDone = false; 
        }
    }

    return node;
}

//===========================================================================
// XMLParser::DecodeTag
//===========================================================================
// Description: Given the string, decode the tag type
//
// Constraints:    None
//
// Parameters:    tagString - the string to decode
//
// Return:      Constructs a node
//
//===========================================================================
XMLNode* XMLParser::DecodeTag( PascalCString& tagString )
{
    XMLNode* node = new XMLNode;

    // Default tag type to normal
    node->m_nodeType = NormalTag;
    
    // Decode tag name
    node->m_tagName = tagString.StreamFirstWord( ' ' );

    // Is it an end tag?
    if( node->m_tagName.Find( '/' ) )
    {
        delete node;
        return NULL;
    }

    // Is it a comment tag?
    if( node->m_tagName.Find( '?' ) || node->m_tagName.Find( '!' ) )
    {
        node->m_nodeType = UnknownTag;
        return node;
    }

    // Get the attributes
    bool notDone = true;
    while( notDone )
    {
        PascalCString attributeString = tagString.StreamFirstWord( ' ' );

        if( attributeString != "" )
        {
            XMLAttribute* attribute = new XMLAttribute;
            // Get the attribute name
            attribute->m_name = attributeString.StreamFirstWord( '=' );
            
            // Get the attribute value.  Does attribute start with "quote"?
            if( attributeString.Find( '"' ) )
            {
                // So we found the first quote
                attributeString.StreamFirstWord( '"' );

                if( !attributeString.Find( '"' ) )
                {
                    // So we didn't get the second quote, so this string contains spaces
                    // so complete the string from the original buffer
                    // Remember to put the double quote back so we can continue parsing
                    attributeString += ' ';
                    attributeString += tagString.StreamFirstWord( '"' );
                    attributeString += '"';
                    
                    // Remove the space right after the quote (if there's any)
                    if( tagString.Find( ' ' ) )
                    {
                        tagString.StreamFirstWord( ' ' );
                    }
                    else if( tagString.Find( '/' ) )
                    {
                        // Get the end slash if there is one
                        attributeString += tagString.StreamFirstWord( '/' );
                        attributeString += '/';
                    }
                }
                attribute->m_value = attributeString.StreamFirstWord( '"' );
            }
            else
            {
                // We didn't find it, get the rest of the string
                attribute->m_value = attributeString.StreamFirstWord( '"' );
            }
            
            node->m_attributes.AddItem( attribute );

            // Determine the node type
            if( attributeString.Find( '/' ) )
            {
                node->m_nodeType = SingleTag;
            }
        }
        else
        {
            notDone = false; 
        }
    }

    return node;
}
