//=============================================================================
// Copyright (c) 2002 Radical Games Ltd.  All rights reserved.
//=============================================================================


#include <p3d/anim/multicontroller.hpp>
#include <constants/chunks.h>
#include <p3d/chunkfile.hpp>
#include <p3d/file.hpp>
#include <p3d/utility.hpp>


tMultiController::tMultiController(int t, float f)
{
    minFrame = 0.0f;
    maxFrame = f;
    numCycles = 0;
    nTracks = t;
    relativeSpeed = 1.0f;
    time = 0.0f;
    speed = 1;

    cycleMode = DEFAULT_CYCLE_MODE;

    trackInfo = new TrackInfo[nTracks];
    tracks = new tFrameController*[nTracks];

    for(unsigned i = 0; i < nTracks; i++)
        tracks[i] = NULL;

    deltaTime = 0.0f;
    
    mLastFrameUpdated = 0;
}

tMultiController::~tMultiController()
{
    for(unsigned i = 0; i < nTracks; i++)
        if(tracks[i])
            tracks[i]->Release();

    delete [] trackInfo;
    delete [] tracks;
}

tFrameController *tMultiController::Clone(void)
{
    P3DASSERT(0);
    return NULL;
}

void tMultiController::Advance(float d, bool update)
{
    deltaTime += d * relativeSpeed;
    time += deltaTime;
    if(update)
    {
        Update();
    }
}

void tMultiController::SetFrame(float f)
{
    static bool first = true;
    if((f!=0.0f)&&first)
    {
        p3d::print("WARNING : tMultiController::SetFrame was called, results are probably not what you expect!\n");
        first = false;
    }

    time = (f / speed) * 1000;
    deltaTime = 0;
    numCycles = 0;
    float lenMS = ((maxFrame-minFrame)/speed)*1000;
    float maxMS = ((maxFrame / speed) * 1000);

    switch (cycleMode)
    {
        case FORCE_CYCLIC:
        {
            while(time >= maxMS)
            {
                time -= lenMS;
            }
            break;
        }
        case FORCE_NON_CYCLIC:
        {
            if (time > maxMS)
            {
                time = maxMS;
            }
            break;
        }
        case DEFAULT_CYCLE_MODE:
        default:
        {
            break;
        }
    }

    for(unsigned i = 0; i < nTracks; i++)
    {
        if(tracks[i])
        {
            tracks[i]->SetFrame(f + trackInfo[i].offset);
        }
    }
}

float tMultiController::GetFrame()
{
    return float(time * speed) / 1000.0f;
}

void tMultiController::SetFrameRange(float minframe, float maxframe)
{
    static bool first = true;
    p3d::print("WARNING : tMultiController::SetFrameRange was called, results are probably not what you expect!\n");
    first = false;

//    float minFrame = (minframe / speed) * 1000;
//   float maxFrame = (maxframe / speed) * 1000;

    minFrame = minframe>0.0f ? minframe : 0.0f;
    maxFrame = maxframe>minFrame ? maxframe : minFrame;
    for(unsigned i = 0; i < nTracks; i++)
    {
        if(tracks[i])
        {
            tracks[i]->SetFrameRange(minframe,maxframe);
        }
    }
}

void tMultiController::GetFrameRange(float& MinFrame, float& MaxFrame)
{
    MinFrame = minFrame;
    MaxFrame = maxFrame;
}

tFrameController* tMultiController::GetTrack(unsigned i) 
{ 
    return (i < nTracks) ? tracks[i] : NULL;
}

void tMultiController::SetTrack(unsigned i, tFrameController* t) 
{ 
    if (i >= nTracks) 
        return;
    tRefCounted::Assign(tracks[i], t);
}

void tMultiController::AddTrack(tFrameController* t) 
{ 
    TrackInfo* newTrackInfo = new TrackInfo[nTracks+1];
    tFrameController** newTracks = new tFrameController*[nTracks+1];
    for (unsigned i = 0; i < nTracks; i++)
    {
        newTrackInfo[i] = trackInfo[i];
        newTracks[i] = tracks[i];
    }
    newTrackInfo[nTracks].startTime = minFrame;
    newTrackInfo[nTracks].endTime = maxFrame;
    newTrackInfo[nTracks].offset = t->GetFrame();
    newTrackInfo[nTracks].scale = 1.0f;
    newTracks[nTracks] = t; // TC: found bug!! changed 'tracks' -> 'newTracks'
    newTracks[nTracks]->AddRef();
    nTracks++;
    delete [] tracks;
    delete [] trackInfo;
    tracks = newTracks;
    trackInfo = newTrackInfo;
}

void tMultiController::RemoveLastTrack()
{
    if( nTracks > 0 )
    {
        // decrement number of tracks
        nTracks--;

        // release last track
        tracks[ nTracks ]->Release();
        tracks[ nTracks ] = NULL;
    }
}

void tMultiController::Update()
{
    if(time < 0)
        time = 0;

    float maxMS = ((maxFrame / speed) * 1000);
    float minMS = ((minFrame / speed) * 1000);
    float lenMS = maxMS-minMS;

    switch (cycleMode)
    {
        case FORCE_CYCLIC:
        {
            bool loop = false;
            while(time >= maxMS)
            {
                loop = true;
                time -= lenMS;
                numCycles++;
            }

            for(unsigned i = 0; i < nTracks; i++)
            {
                if(tracks[i])
                {
                    if (loop)
                    {
                        tracks[i]->SetFrame(trackInfo[i].offset);
                        tracks[i]->Advance(time-minMS);
                    }
                    else
                    {
                        tracks[i]->Advance(deltaTime);
                    }
                }
            }   
            break;
        }
        case FORCE_NON_CYCLIC:
        {
            if (time <= maxMS)
            {
                for(unsigned i = 0; i < nTracks; i++)
                {
                    if(tracks[i])
                    {
                        tracks[i]->Advance(deltaTime);
                    }
                }   
            }
            else
            {
                numCycles = 1;
            }
            break;
        }
        case DEFAULT_CYCLE_MODE:
        default:
        {
            numCycles = 1;
            for(unsigned i = 0; i < nTracks; i++)
            {
                if(tracks[i])
                {
                    tracks[i]->Advance(deltaTime);
                    numCycles &= tracks[i]->LastFrameReached();
                }
            }   
            break;
        }
    }
    deltaTime = 0;
}


tMultiControllerLoader::tMultiControllerLoader() : tSimpleChunkHandler(P3D_MULTI_CONTROLLER)
{
}

tEntity* tMultiControllerLoader::LoadObject(tChunkFile* f, tEntityStore* store)
{
    char buf[256];
    f->GetPString(buf);  // name
    unsigned version = f->GetLong();
    P3DASSERT(version == 0);
    float length = f->GetFloat();
    float framerate = f->GetFloat();
    unsigned numtracks = f->GetLong();

    tMultiController* cont = new tMultiController(numtracks, length);
    cont->SetName(buf);
    cont->SetFramerate(framerate);

    unsigned track_index = 0;

    while(f->ChunksRemaining())
    {
        switch(f->BeginChunk())
        {
            case P3D_MULTI_CONTROLLER_TRACKS:
            {
                int n = f->GetLong();

                for(int i = 0; i < n; i++)
                {
                    char name[256];
                    tMultiController::TrackInfo track;

                    f->GetPString(name);
                    track.startTime = f->GetFloat();
                    track.endTime = f->GetFloat();
                    track.scale = f->GetFloat();

                    P3DASSERT(track_index < numtracks);
                    cont->SetTrack(track_index, p3d::find<tFrameController>(store, name));
                    tFrameController* controller = p3d::find<tFrameController>(store, name);
                    if (controller)
                    {
                        track.offset = controller->GetFrame();
                    }
                    else
                    {
                        p3d::printf("multicontroller (%s) has invalid track (%s)\n", buf, name);
                        track.offset = 0.0f;
                    }
                    cont->SetTrackInfo(track_index, &track);

                    ++track_index;
                }
                break;
            }

            case P3D_MULTI_CONTROLLER_TRACK:
            {
                char name[256];
                tMultiController::TrackInfo track;

                f->GetPString(name);
                track.startTime = f->GetFloat();
                track.endTime = f->GetFloat();
                track.scale = f->GetFloat();

                P3DASSERT(track_index < numtracks);
                cont->SetTrack(track_index, p3d::find<tFrameController>(store, name));
                tFrameController* controller = p3d::find<tFrameController>(store, name);
                if (controller)
                {
                    track.offset = controller->GetFrame();
                }
                else
                {
                    p3d::printf("multicontroller (%s) has invalid track (%s)\n", buf, name);
                    track.offset = 0.0f;
                }
                cont->SetTrackInfo(track_index, &track);

                ++track_index;
                break;
            }

            default:
                break;
        }
        f->EndChunk();
    }

    P3DASSERT(track_index == numtracks);
    return cont;
}

