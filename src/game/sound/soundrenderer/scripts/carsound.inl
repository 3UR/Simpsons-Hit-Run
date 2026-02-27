Create<daSoundResourceData>("fire")
    .AddFilename ( "sound/carsound/damagedcar.rsd", 1.000000f )
    .SetTrim ( 0.800000f )
    .SetLooping ( true );
Create<daSoundResourceData>("ferrari")
    .AddFilename ( "sound/carsound/cellcar.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.490000f )
#ifdef RAD_PS2
    .SetTrim ( 0.680000f )
#endif
#ifdef RAD_GAMECUBE
    .SetTrim ( 0.660000f )
#endif
    ;
Create<daSoundResourceData>("gearshift")
    .AddFilename ( "sound/carsound/common/gearshft.rsd", 1.000000f );
Create<daSoundResourceData>("skid")
    .AddFilename ( "sound/carsound/common/skid.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.600000f );
Create<daSoundResourceData>("horn")
    .AddFilename ( "sound/carsound/common/horn.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.790000f );
Create<daSoundResourceData>("siren")
    .AddFilename ( "sound/carsound/siren.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.830000f );
Create<daSoundResourceData>("rocket")
    .AddFilename ( "sound/carsound/rocket_car_04.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.870000f );
Create<daSoundResourceData>("car_jump")
    .AddFilename ( "sound/carsound/car_jump_02.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 1.000000f );
Create<daSoundResourceData>("vote_quimby")
    .AddFilename ( "sound/carsound/vote_quimby_02.rsd", 1.000000f )
    .AddFilename ( "sound/carsound/silent.rsd", 1.000000f )
    .AddFilename ( "sound/carsound/silent.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.760000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("nuke_truck")
    .AddFilename ( "sound/carsound/nuke_truck_02.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 1.000000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("i_and_s_truck")
    .AddFilename ( "sound/carsound/i_and_s_loop.rsd", 1.000000f )
    .AddFilename ( "sound/carsound/silent.rsd", 1.000000f )
    .AddFilename ( "sound/carsound/silent.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.780000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("ice_cream_truck")
    .AddFilename ( "sound/carsound/ice_cream_truck.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 1.000000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("amb_siren")
    .AddFilename ( "sound/carsound/amb_siren.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 1.000000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("fire_siren")
    .AddFilename ( "sound/carsound/fire_siren.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 1.000000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("klaxon")
    .AddFilename ( "sound/carsound/fire_siren.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 1.000000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("nuctruck_glow")
    .AddFilename ( "sound/carsound/nuctruck_glow2.rsd", 1.000000f )
    .SetTrim ( 0.670000f )
    .SetLooping ( true )
    .SetStreaming ( true );
Create<daSoundResourceData>("blank")
    .AddFilename ( "sound/carsound/blank.rsd", 1.000000f )
    .SetLooping ( true );
Create<daSoundResourceData>("dirt_skid")
    .AddFilename ( "sound/carsound/common/dirt_skid.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.680000f );
Create<daSoundResourceData>("mission_ai_vehicle")
    .AddFilename ( "sound/carsound/sportscarB.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.610000f )
#ifdef RAD_PS2
    .SetTrim ( 0.850000f )
#endif
#ifdef RAD_GAMECUBE
    .SetTrim ( 0.780000f )
#endif
    ;
