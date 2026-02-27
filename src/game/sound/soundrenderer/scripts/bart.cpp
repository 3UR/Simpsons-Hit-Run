#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunBartSoundScripts( void )
{
    SetCurrentNameSpace( GetCharacterNamespace( SC_CHAR_BART ) );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_CHAR_BART );
    #include "bart.inl"
}
#pragma optimize("", on)
}
