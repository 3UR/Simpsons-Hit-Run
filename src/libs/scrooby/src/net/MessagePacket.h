#ifndef __MESSAGE_PACKET_H
#define __MESSAGE_PACKET_H

#define    MESSAGE_LENGTH 76

#include <string.h>

enum CommandType
{
    e_ctConnect,
   e_ctAcknowledge,
   e_ctProject,
    e_ctPage,
    e_ctReload,
    e_ctQuit,
   e_ctWorkingDir
};

class MessagePacket
{
public:
    // constructors and destructors
   //MessagePacket() {};
   //~MessagePacket() {};

    CommandType GetCommand() { return m_eCommand; };
    char* GetMessage() { return m_strMessage; };

    void SetCommand( const CommandType c ) { m_eCommand = c; };
    void SetMessage( const char* s ) { strncpy( m_strMessage, s, MESSAGE_LENGTH); };

    unsigned int Size() { return sizeof( MessagePacket ); };
private:

    CommandType m_eCommand;
    char m_strMessage[MESSAGE_LENGTH];

};

#endif
