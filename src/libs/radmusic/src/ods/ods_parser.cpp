#include <pch.hpp>
#include <ods/parser.hpp>


bool Parser::IsLetter( char c )
{
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) )
	{
		return true;
	}

	return false;
}

bool Parser::IsStartOfVarName( char c )
{
	if ( IsLetter( c ) || ( c == '_' ) )
	{
		return true;
	}

	return false;
}

bool Parser::IsMiddleOfVarName( char c )
{
	if ( IsStartOfVarName( c ) || IsNumber( c ) )
	{
		return true;
	}

	return false;
}

bool Parser::IsWhite( char c )
{
	if ( c == '\t' || c ==' '  || c == 10 || c == 13 )
	{
		return true;
	}

	return false;
}

bool Parser::IsNumber( char c )
{
	if ( c >= '0' && c <= '9' )
	{
		return true;
	}

	return false;
}

void Parser::SkipWhite( void )
{
	while ( m_pCurrent < m_pEnd )
	{
		if ( IsWhite( *m_pCurrent ) )
		{
			NextChar( );
		}
		else
		{
			break;
		}
	}
}

bool Parser::IsToken( const char * pToken )
{
    SkipWhite( );

    return IsTokenNsw( pToken );
}

bool Parser::IsTokenNsw( const char * pToken )
{
	const char * p = m_pCurrent;

    int tokenLength = strlen( pToken );


	while( ( p - m_pCurrent) < tokenLength )
	{
	    if ( p >= m_pEnd )
	    {
	        return false;
	    }
		
		if ( *p != *pToken )
		{
            return false;
		}

		p++;
		pToken++;
	}

	return true;
}

void Parser::SkipToken( void )
{
    SkipWhite( );

	while ( m_pCurrent < m_pEnd )
	{
		if ( ! IsWhite( *m_pCurrent ) )
		{
			NextChar( );
		}
		else
		{
			break;
		}
	}
}

bool Parser::SkipToken( const char * pToken )
{
    SkipWhite( );

    return SkipTokenNsw( pToken );
}

bool Parser::SkipTokenNsw( const char * pToken )
{
	if ( IsTokenNsw( pToken ) )
	{
		m_pCurrent += strlen( pToken );

		return true;
	}
	
	return false;
}

bool Parser::SkipChar( char c )
{
    SkipWhite( );

	if ( *m_pCurrent == c )
	{
		NextChar( );
		return true;
	}

	return false;
}

void Parser::SkipLine( void )
{
	while( m_pCurrent < m_pEnd )
	{
		if ( *m_pCurrent == 0x0A )
		{
			NextChar( );
			break;
		}

		NextChar( );
	}
}

bool Parser::GetChar( char * pChar )
{
    SkipWhite( );

    if ( m_pCurrent < m_pEnd )
    {
        *pChar = *m_pCurrent;
        NextChar( );
        return true;
    }

    return false;        
}

bool Parser::GetToken( char * pBuffer, int size )
{
    SkipWhite( );

	const char *pOldBuf = pBuffer;

	while( m_pCurrent < m_pEnd )
	{
		if ( !IsWhite( *m_pCurrent ) )
		{
			*pBuffer = *m_pCurrent;
			pBuffer++;
			NextChar( );
		}
		else
		{
			break;
		}
	}

	*pBuffer = '\0';

	if ( pOldBuf == pBuffer )
	{
		return false;
	}

	return true;
}

bool Parser::GetQuotedString( char * pBuffer, int size )
{
    SkipWhite( );
    
    char * pBufferEnd = pBuffer + size;

    if ( m_pCurrent >= m_pEnd )
    {
        return false;
    }

    if (*m_pCurrent != '\"' )
    {
        return false;
    }

    NextChar( );

    while ( 1 )
    {
        if ( m_pCurrent >= m_pEnd ||pBuffer >= pBufferEnd )
        {
            return false;
        }

        if ( *m_pCurrent == '\"' )
        {
            *pBuffer = 0;
            NextChar( );
            return true;
        }

        *pBuffer = *m_pCurrent;
        pBuffer++;

        NextChar( );
    }
}

int Parser::GetInteger( void )
{
    SkipWhite( );

	char intBuffer[ 128 ];
	
	char * pP = intBuffer;

	while( *m_pCurrent >= '0' && *m_pCurrent <= '9' )
	{
		*pP = *m_pCurrent;
		NextChar( );
		pP++;
	}

	*pP = '\0';

	return atoi( intBuffer);
}

float Parser::GetFloat( void )
{
    SkipWhite( );
    	
	char floatBuffer[ 128 ];
	
	char * pP = floatBuffer;

	while( (*m_pCurrent >= '0' && *m_pCurrent <= '9') || *m_pCurrent == '.' )
	{
		*pP = *m_pCurrent;
		NextChar( );
		pP++;
	}

    *pP = '\0';

    float result = (float) atof( floatBuffer );

	return result;
}

bool Parser::IsNumber( void )
{
    SkipWhite( );

	return IsNumber( *m_pCurrent );
}

bool Parser::GetVariableName( char * pBuffer, int size )
{
    SkipWhite( );

	char * pP = pBuffer;

	if ( ! IsStartOfVarName( *m_pCurrent ) )
	{
		return false;
	}

	*pP = *m_pCurrent;

	NextChar( );
	pP++;

	while ( IsMiddleOfVarName( *m_pCurrent ) )
	{
		*pP = *m_pCurrent;
		NextChar( );
		pP++;
	}

	*pP = '\0';

	return true;

}

bool Parser::IsStartOfVarName( void )
{
    SkipWhite( );

	return IsStartOfVarName( *m_pCurrent );
}

bool Parser::IsEof( void )
{
    SkipWhite( );

	return m_pCurrent >= m_pEnd;
}

unsigned int Parser::GetLineCount( void )
{
    return m_LineCount;
}

void Parser::NextChar( void )
{
    if ( *m_pCurrent == 0x0a )
    {
        m_LineCount++;
    }

    m_pCurrent++;
}