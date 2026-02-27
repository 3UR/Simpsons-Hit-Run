#ifndef PARSER_HPP
#define PARSER_HPP

#include <stdlib.h>
#include <string.h>

class Parser
{
	public:

		Parser( const char * pBytes, unsigned int size )
			:
			m_pStart( pBytes ),
			m_pEnd( pBytes + size ),
			m_pCurrent( pBytes ),
            m_LineCount( 0 )
		{
		}

		static bool IsWhite( char c );
		static bool IsNumber( char c );
		static bool IsStartOfVarName( char c );
		static bool IsLetter( char c );
		static bool IsMiddleOfVarName( char c );

		void SkipWhite( void );

		bool IsToken( const char * pToken );
        bool IsTokenNsw( const char * pToken );

		bool IsEndOfLine( void );
		bool IsNumber( void );
		bool IsStartOfVarName( void );

		void SkipToken( void );
		bool SkipToken( const char * pToken );
        bool SkipTokenNsw( const char * pToken );
		bool SkipChar( char c );
		void SkipLine( void );

		bool GetToken( char * pBuffer, int size );
        bool GetChar( char * pChar );
		int  GetInteger( void );
        float GetFloat( void );
        bool GetQuotedString( char * pBuffer, int size );

		bool GetVariableName( char * pBuffer, int size );
		bool IsEof( void );

        unsigned int GetLineCount( void );


	private:

        void NextChar( );

		const char * m_pStart;
		const char * m_pEnd;
		const char * m_pCurrent;

        unsigned int m_LineCount;

		char m_TempBuffer[ 1024 ];
};

#endif // PARSER_HPP
