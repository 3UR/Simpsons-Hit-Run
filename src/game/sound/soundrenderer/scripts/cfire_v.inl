Create<daSoundResourceData>("cfirecar_v")
    .AddFilename ( "sound/carsound/firetruck.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.630000f );
Create<daSoundResourceData>("cfirecar_horn")
    .AddFilename ( "sound/carsound/fire_horn.rsd", 1.000000f )
    .SetTrim ( 0.800000f )
    .SetLooping ( false );
Create<daSoundResourceData>("cfirecar_overlay")
    .AddFilename ( "sound/carsound/fire_siren_02.rsd", 1.000000f )
    .SetTrim ( 0.800000f )
    .SetLooping ( true )
    .SetStreaming ( true );
