#include <sound/soundrenderer/soundrenderingmanager.h>
#include <sound/avatar/carsoundparameters.h>
#include <sound/soundfx/reverbsettings.h>
#include <sound/soundfx/positionalsoundsettings.h>
#include <sound/tuning/globalsettings.h>

namespace Sound {
#pragma optimize("t", on)
void daSoundRenderingManager::RunTuningSoundScripts( void )
{
    SetCurrentNameSpace( GetTuningNamespace() );
    GetSoundManager()->GetSoundLoader()->SetCurrentCluster( SC_NEVER_LOADED );
    #include "car_tune.inl"
    #include "positionalsettings.inl"
    #include "global.inl"
}
#pragma optimize("", on)
}
