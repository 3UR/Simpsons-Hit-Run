Create<daSoundResourceData>("ship_v")
    .AddFilename ( "sound/carsound/ghostboat1.rsd", 1.000000f )
    .SetLooping ( true )
    .SetTrim ( 0.830000f );
Create<daSoundResourceData>("ship_v_horn")
    .AddFilename ( "sound/carsound/foghorn.rsd", 1.000000f )
    .SetTrim ( 0.810000f )
    .SetStreaming ( true );
Create<daSoundResourceData>("ship_overlay")
    .AddFilename ( "sound/carsound/ghost_ship.rsd", 1.000000f )
    .SetTrim ( 0.840000f )
    .SetStreaming ( true )
    .SetLooping ( true );
Create<daSoundResourceData>("ship_skid")
    .AddFilename ( "sound/carsound/swoosh3.rsd", 1.000000f )
    .SetTrim ( 1.000000f )
    .SetLooping ( true );
