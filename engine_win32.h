#pragma once

struct win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME LastWriteTime;
    game_update_and_render *UpdateAndRender;
    game_generate_audio_samples *GenerateAudioSamples;
    debug_game_frame_end *DEBUGFrameEnd;
    
    bool IsValid;
};

struct win32_state
{
    HANDLE RecordingHandle;
    int InputRecordingIndex;
    
    HANDLE PlayBackHandle;
    int InputPlayingIndex;
    
    uint64 TotalSize;
    void *GameMemoryBlock;

    char EXEFileName[MAX_PATH];
    char *OnePastLastEXEFileNameSlash;
};