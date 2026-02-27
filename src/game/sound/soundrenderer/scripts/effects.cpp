#include <sound/soundrenderer/soundrenderingmanager.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunSoundEffectScripts( void )
{
    SetCurrentNameSpace( GetSoundNamespace() );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_ALWAYS_LOADED );
    #include "frontend.inl"
    #include "collide.inl"
    #include "carsound.inl"
    #include "world.inl"
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_INGAME );
    #include "positionalsounds.inl"
}
#pragma optimize("", on)
}
