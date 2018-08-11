#pragma once

/*
TODO(nino):

- Engine
    - vsync is not working, maybe is graphics driver outdated
    - Collision detection
        - tweak some code
    - Terrain Generation
        - different terrain generation system
    - Sim Region
        - multiple sim regions?
        - calculate entities physics in chunk space

- Renderer
    - entity triangle mesh
    - storing mesh data with same shader,
      material and texture? and sorting it
    - multithreaded texture downloads -
      shared context! (HMH ep.243)
    - fix draw width and height window sizes
    - framebuffers and render targets
    - supersampling antialising

- Memory
    - more flexible and robust memory management
    - allocating SubBlocks
    - Entities
        - low and high entity storage

- Sound
    - to many playing sounds is painfull to hear
    - handle stereo channels? Think I have that.
    - streaming large wav files

- Debug
    - seperate debug renderer
    - diagraming

*/

#include "engine_platform.h"
#include "engine_intrinsics.h"
#include "engine_math.h"
#include "engine_renderer.h"
#include "engine_opengl.h"
#include "engine_sound.h"
#include "engine_entity.h"
#include "engine_sim_region.h"
#include "engine_world.h"
#include "engine_shared.h"
#include "engine_debug.h"

struct enemy_stat
{
    u32 Index;
    u32 FireCount;
};

struct game_state
{
    b32 IsInitialized;

    b32 GameOver;
    b32 ShakeScreen;

    u32 EnemiesDestroyed;
    real32 SecondsAlive;

    u32 PlayerFireCount;
    u32 BallFireCount;

    real32 WaveMultiplier;
    real32 TimerValue;
    real32 TimerMultiplier;

    b32 StartWave;
    u32 EnemiesRemaining;
    enemy_stat Enemies[128];
    real32 WaveTimer;

    real32 MonolithHealth;
    real32 MonolithHealingRate;
    real32 MonolithHealthDamage;
    u32 MonolithEntityIndex;

    u32 CameraIndex;
    u32 PlayerIndex;
    u32 CameraFollowingEntityIndex;
    u32 ControllingEntityIndex;
    
    u32 EntityCount;
    entity Entities[512];
    u32 FreeEntityCount;
    u32 FirstFreeEntityIndex;

    u32 CollisionVolumeCount;
    collision_volume *FirstFreeCollisionVolume;
    collision_volume CollisionVolumes[512];

    collision_rule *CollisionRuleHash[512];
    collision_rule *FirstFreeCollisionRule;

    memory_block WorldBlock;

    audio_state AudioState;
};

struct stream_state
{
    bool IsInitialized;
    
    b32 FontsLoaded;

    u32 ChunkCount;
    // TODO: Add Chunks to GameState?
    world_chunk Chunks[64];
    world_chunk *CurrentChunk;

    u16 SoundCount;
    sound *Sounds;

    memory_block TranBlock;
};

#define GAME_UPDATE_AND_RENDER(name) void name(game_memory *Memory, game_input *Input, render_buffer *RenderBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GENERATE_AUDIO_SAMPLES(name) void name(game_memory *Memory, audio_sample_request *SampleRequest)
typedef GAME_GENERATE_AUDIO_SAMPLES(game_generate_audio_samples);

#define DEBUG_GAME_FRAME_END(name) void name(game_memory *Memory, debug_frame_end FrameEnd, debug_record *DebugRecords_Win)
typedef DEBUG_GAME_FRAME_END(debug_game_frame_end);