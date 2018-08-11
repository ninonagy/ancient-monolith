#include "engine_precompiled.h"

#include "engine.h"
#include "engine_random.cpp"
#include "engine_renderer.cpp"
#include "engine_opengl.cpp"
#include "engine_sound.cpp"
#include "engine_collision.cpp"
#include "engine_sim_region.cpp"
#include "engine_entity.cpp"
#include "engine_world.cpp"
#include "engine_debug.cpp"

struct pixel
{
    u8 R;
    u8 G;
    u8 B;
    u8 A;
};

internal uint8 *
CreatePixels(memory_block *Block, u32 Width, u32 Height)
{
    pixel *Pixels = PushArray(Block, Width*Height, pixel);

    real32 R = 1.0f;
    real32 G = 0.2f;
    real32 B = 0.2f;

    for(u32 PixelCount = 0;
        PixelCount < Width * Height;
        ++PixelCount)
    {
        pixel *Pixel = Pixels + PixelCount;
        Pixel->R = (u8)(Random * 255.0f);
        Pixel->G = (u8)(G * 255.0f);
        Pixel->B = (u8)(B * 255.0f);
        Pixel->A = (u8)(255.0f);
    }
    
    return (uint8 *)Pixels;
}

inline u32
GetRandomSoundIndex(u32 From, u32 To, u32 Size)
{
    return (u32)Clamp((r32)From, (r32)To, Random * (r32)Size);
}

#if ENGINE_DEBUG
game_memory *DebugGlobalMemory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    TIMED_BLOCK();

    Platform = Memory->PlatformAPI;

    RenderBuffer->FocusLength = 5.0f;
    //float PixelSizeInMeters = 0.000264583f;
    //real32 WidthOfMonitor = 0.635f;
    RenderBuffer->MetersToPixels = (r32)(0.5f*RenderBuffer->Height);

    vec2 MouseP = v2(Input->MouseX, Input->MouseY);
    vec3 WorldMouseP = Unproject(RenderBuffer, MouseP);

    real32 TimeAdvance = Input->dTimeForFrame;

#if ENGINE_DEBUG
    DEBUGRenderBuffer = RenderBuffer;
    DebugGlobalMemory = Memory;
    DEBUGWindowMouseP = MouseP;
    DEBUGWorldMouseP = WorldMouseP.xy;
#endif

    Assert(sizeof(game_state) <= Memory->PermanentStorageSize)
    Assert(sizeof(stream_state) <= Memory->StreamingStorageSize)
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!GameState->IsInitialized)
    {
        InitializeBlock(&GameState->WorldBlock,
                        (memory_index)(Memory->PermanentStorageSize - sizeof(game_state)),
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));
        
	    InitRandom32(&RandomGenerator, (u64)&MouseP, 30041999);
	    for(int i = 0; i < ArrayCount(RandomSequence); i++)
	    {
            RandomSequence[i] = Random * 4 - 2;
	    }

        // TODO: Initialize world: entities, environment...

        AddCamera(GameState, v2(0, 8.0f));

        vec2 PlayerPos = v2(0, 6);
        AddPlayer(GameState, PlayerPos, v2(0.3f, 0.2f));
        GameState->CameraFollowingEntityIndex = GameState->PlayerIndex;

        u32 BallIndex = AddBall(GameState, v2(-0.5f, 8), v2(0.2f, 0.2f));

        vec2 BlockDim = v2(0.13f, 0.6f);
        vec2 BlockPos = v2(PlayerPos.x, PlayerPos.y + 0.5f*BlockDim.y);
        AddBlock(GameState, BlockPos - v2(2.1f, 0), BlockDim);
        AddBlock(GameState, BlockPos + v2(2.1f, 0), BlockDim);
 
        vec2 MonPos = v2(0, 6);
        vec2 MonDim = v2(1.3f, 4.0f);
        u32 MonolithIndex = AddEntity(GameState, MonPos, MonDim,
                                      EntityComponent_Monolith |
                                      EntityComponent_Position |
                                      EntityComponent_Health |
                                      EntityComponent_Render |
                                      EntityComponent_Collide);
        GameState->MonolithEntityIndex = MonolithIndex;
        GameState->MonolithHealth = 100;
        GameState->MonolithHealingRate = 0.2f;
        GameState->MonolithHealthDamage = 0.8f;

        AddCollisionRule(GameState, GameState->PlayerIndex, MonolithIndex, false);
        //AddCollisionRule(GameState, BallIndex, MonolithIndex, false);
        //AddCollisionRule(GameState, GameState->PlayerIndex, BallIndex, true);
        

        GameState->AudioState.Volume = 0.8f;

        GameState->IsInitialized = true;
    }

    stream_state *StreamState = (stream_state *)Memory->StreamingStorage;
    if(!StreamState->IsInitialized)
    {
        InitializeBlock(&StreamState->TranBlock,
                        (memory_index)(Memory->StreamingStorageSize - sizeof(stream_state)),
                        (uint8 *)Memory->StreamingStorage + sizeof(stream_state));
        
        char *SoundNames[] =
        {
            "sound/shoot01.wav",
            "sound/shoot02.wav",
            "sound/shoot03.wav",
            "sound/shoot04.wav",
            "sound/shoot05.wav",
        };

        sound *Sound = PushArray(&StreamState->TranBlock, ArrayCount(SoundNames), sound);
        StreamState->Sounds = Sound;

        for(u32 FileIndex = 0;
            FileIndex < ArrayCount(SoundNames);
            ++FileIndex)
        {    
            char *FileName = SoundNames[FileIndex];
            Sound[FileIndex] = LoadSoundFromWavFile(&StreamState->TranBlock, FileName);
            ++StreamState->SoundCount;
        }

        StreamState->IsInitialized = true;
    }

    audio_state *AudioState = &GameState->AudioState;
    sound *Sound = StreamState->Sounds;

    entity *Player = GetEntity(GameState->PlayerIndex);

    game_controller_input *Controller = &Input->Controller;
    vec2 Direction = {};
    if(Controller)
    {
        if(Controller->MoveLeft.EndedDown ||
           Controller->ActionLeft.EndedDown)
        {   
            Direction.x = -1.0f;
        }
        if(Controller->MoveRight.EndedDown ||
           Controller->ActionRight.EndedDown)
        {   
            Direction.x = 1.0f;
        }
        if(Controller->MoveUp.EndedDown)
        {   
            Direction.y = 1.0f;
        }
        if(Controller->MoveDown.EndedDown)
        {   
            Direction.y = -1.0f;
        }
        if(Direction.x != 0.0f &&
           Direction.y != 0.0f)
        {
            Direction *= 0.707106781f;
        }
        
        if(Controller->RightShoulder.EndedDown)
        {
            if(GameState->MonolithHealth <= 100)
            {
                GameState->MonolithHealth += GameState->MonolithHealingRate;
            }
        }

        if(Controller->Key_R.EndedDown)
        {
            if(GameState->MonolithHealth <= 0)
            {
                GameState->GameOver = false;
                GameState->MonolithHealth = 100;
                GameState->WaveMultiplier = 0;
                GameState->TimerValue = 0;
                GameState->TimerMultiplier = 0;
                
                GameState->EnemiesDestroyed = 0;
                GameState->SecondsAlive = 0;

                GameState->WaveTimer = 0;

                RenderBuffer->FrameRateValue = 1.0f;
            }
        }

        if(Controller->Space.EndedDown)
        {
            if(GameState->PlayerFireCount == 6)
            {
                vec2 BulletPos = Player->P + v2(0, Player->Dim.y);

                InstantiateBullet(GameState, BulletPos, WorldMouseP.xy, 500,
                                  GameState->PlayerIndex, EntityComponent_Player);

                u32 SoundIndex = GetRandomSoundIndex(0, 5, 5);
                u16 SoundID = PlaySound(AudioState, Sound + SoundIndex);

                GameState->PlayerFireCount = 0;
            }

            ++GameState->PlayerFireCount;
        }
/*
    #if ENGINE_DEBUG
        if(Controller->Key_R.EndedDown)
        {
            RespawnPlayer(GameState, GameState->PlayerIndex, v2(0, 6));
        }
    #endif
*/
    }

    if(!GameState->EnemiesRemaining && GameState->StartWave && !GameState->GameOver)
    {
        GameState->TimerValue = Clamp(4.0f, 8.0f, 8.0f - GameState->TimerMultiplier);
        //u32 EnemyCount = (u32)(Random * 5 + GameState->WaveMultiplier);
        u32 EnemyCount = (u32)(Random * 10 + GameState->WaveMultiplier);
        
        for(u32 EnemyIndex = 0;
            EnemyIndex < EnemyCount;
            ++EnemyIndex)
        {
            vec2 EnemySpawnPos = {};
            if(EnemyIndex > (u32)(EnemyCount / 2))
            {
                EnemySpawnPos = v2(10, 10) + Random * 4;
            }
            else
            {
                EnemySpawnPos = v2(-10, 10) + Random * 4;
            }

            u32 EntityIndex = AddEnemy(GameState, EnemySpawnPos, v2(0.2f, 0.2f));
            Assert(EnemyIndex < ArrayCount(GameState->Enemies));
            GameState->Enemies[EnemyIndex].Index = EntityIndex;
            GameState->Enemies[EnemyIndex].FireCount = 0;
        }

        GameState->EnemiesRemaining = EnemyCount;

        GameState->WaveMultiplier += 5.8f;  // 1.8f
        GameState->TimerMultiplier += 0.8f; // 0.2f

        GameState->StartWave = false;
    }
    else
    {
        for(u32 EnemyIndex = 0;
            EnemyIndex < ArrayCount(GameState->Enemies);
            ++EnemyIndex)
        {
            enemy_stat *Stat = &GameState->Enemies[EnemyIndex];
            if(Stat->Index)
            {
                entity *Enemy = GetEntity(Stat->Index);
                if(Enemy)
                {
                    if(Enemy->Inactive)
                    {
                        Stat->Index = 0;
                        Stat->FireCount = 0;
                        --GameState->EnemiesRemaining;
                    }
                }
            }
        }

        if(GameState->EnemiesRemaining == 0)
        {
            if(GameState->WaveTimer >= GameState->TimerValue)
            {
                GameState->EnemiesRemaining = 0;
                GameState->WaveTimer = 0;
                GameState->StartWave = true;
            }

            GameState->WaveTimer += TimeAdvance;
        }
    }

    int32 CurrentChunkPos;
    StreamState->CurrentChunk = GetChunkWithIndex(StreamState,
        GetChunkIndex(Player->P.x, &CurrentChunkPos));
    
    // TODO: Multiple area bounds
    // NOTE: Collision area bounds
    rect2 AreaBounds = RectCenterDim(Player->P, v2(34, 32));
    
    // TODO: Put all Entities in correspoding chunk area!
    for(float x = Player->P.x - 18;
        x <= Player->P.x + 18;
        x += TERRAIN_SIZE)
    {
        int32 ChunkPos = 0;
        int32 Index = GetChunkIndex(x, &ChunkPos);
        world_chunk *Chunk = GetChunkWithIndex(StreamState, Index);
        if(Chunk)
        {
            // TODO: First entity in chunk is collision entity!
            entity *Entity = Chunk->Entities[0];
            if(IsInRectangle(AreaBounds, Entity->P))
            {
                if(Entity->FirstCollisionVolume == 0)
                {
                    Entity->FirstCollisionVolume = GenerateTerrainCollisionVolumes(GameState, Chunk->Vertices, Entity->P);
                }
            }
            else
            {
                if(Entity->FirstCollisionVolume)
                {
                    FreeCollisionVolume(GameState, Entity->FirstCollisionVolume);
                    Entity->FirstCollisionVolume = 0;
                }
            }

            continue;
        }

        Chunk = AllocateChunk(StreamState, v2(ChunkPos, 0.0f), Index);
        GenerateTerrainVertices(Chunk->Vertices, (real32)ChunkPos, POINT_COUNT);
        
        u32 EntityIndex = AddLand(GameState, v2(ChunkPos, 8) + v2(0.5f*TERRAIN_SIZE, 0));
        entity *Entity = GetEntity(EntityIndex);
        Entity->Mesh = GenerateTerrainMesh(StreamState, Chunk->Vertices);
        Entity->FirstCollisionVolume = GenerateTerrainCollisionVolumes(GameState, Chunk->Vertices, Entity->P);

        AddCollisionRule(GameState, GameState->PlayerIndex, EntityIndex, true);

        StoreEntityInChunk(Chunk, Entity);
    }

    rect2 SimBounds = RectCenterDim(Player->P, v2(40, 16));
    temp_memory SimMemory = BeginTempMemory(&StreamState->TranBlock);
    sim_region *SimRegion = BeginSim(SimMemory, StreamState, GameState, SimBounds);

    SimulateEntities(SimRegion, GameState, &StreamState->TranBlock, Direction, Input->dTimeForFrame);

    // NOTE: shake screen from sim code
    vec2 Offset = {};
    if(GameState->ShakeScreen)
    {
        Offset = v2((Random * 2 - 1), (Random * 2 - 1)) * 0.08f;
        GameState->ShakeScreen = false;
    }

    // NOTE: Draw commands should be added below camera code!
    entity *Camera = &GameState->Entities[GameState->CameraIndex];
    if(Camera)
    {
        bool32 MoveCamera = 1;
        bool32 CameraFollowPlayer = 0;
        real32 CameraZ = 2.0f;
        vec3 TransformCameraP = v3(Camera->P, CameraZ);

        if(MoveCamera)
        {
            if(CameraFollowPlayer)
            {
                Assert(GameState->CameraFollowingEntityIndex)
                GameState->ControllingEntityIndex = GameState->CameraFollowingEntityIndex;
                real32 CameraOffset = TransformCameraP.y - Player->P.y;
                TransformCameraP = v3(v2(Player->P.x, Player->P.y + CameraOffset), CameraZ);
            }
    #if ENGINE_DEBUG
            else
            {
                GameState->ControllingEntityIndex = GameState->CameraIndex;
                TransformCameraP = v3(Camera->P, CameraZ);
            }
    #endif
        }
        else
        {
            GameState->ControllingEntityIndex = GameState->PlayerIndex;
        }

        RenderBuffer->CameraP = v3(-TransformCameraP.xy + Offset, TransformCameraP.z);
        mat4 CameraViewMatrix = Translate(RenderBuffer->CameraP.xy);
        PushSetCameraMatrix(RenderBuffer, CameraViewMatrix);
    }

    // Test
    
    for(int i = 0; i < 3; ++i)
    {
        PushDrawRect(RenderBuffer, v3((i - 1) * 2, 8, 5.0f), v2(1.0f, 1.0f), 0, v4(0, 0, 0, 1));
    }
    /*
    PushDrawRect(RenderBuffer, WorldMouseP, v2(0.5f, 0.5f));
    */

#if 0
    if(!RenderBuffer->GradientTexture)
    {
        RenderBuffer->GradientTexture = AllocateTexture(RenderBuffer);
        PushLoadTexture(RenderBuffer, RenderBuffer->GradientTexture, false, 
                        "textures/gradient0.png");
    }               
    PushDrawRect(RenderBuffer, v3(Camera->P + v2(0, 1), 0), v2(20, 10), 0,
                 v4(0.95f, 0.819f, 0.466f, 1.0f), RenderBuffer->GradientTexture);
#endif

    entity *Monolith = GetEntity(GameState->MonolithEntityIndex);
    if(!Monolith->TextureSlot)
    {
        Monolith->TextureSlot = AllocateTexture(RenderBuffer);
        PushLoadTexture(RenderBuffer, Monolith->TextureSlot, false, "textures/gradient0.png");
    }
    draw_rect *Command = PushDrawRect(RenderBuffer, v3(Monolith->P, 0), Monolith->Dim, Monolith->Angle, 
        v4(0.12f, 0.12f, 0.12f, 1.0f), Monolith->TextureSlot);
    Command->MinUV = v2(1, 1);
    Command->MaxUV = v2(0, 0);

    for(collision_volume *CollisionVolume = Monolith->FirstCollisionVolume;
        CollisionVolume;
        CollisionVolume = CollisionVolume->Next)
    {
        vec2 P = Monolith->P + CollisionVolume->OffsetP;
        real32 Angle = Monolith->Angle + CollisionVolume->Angle;
        rect2 CollisionVolumeRect = RectCenterDim(P, CollisionVolume->Dim);
        
        {
            if((int32)GameState->WaveTimer == 0)
            {
                PushDrawRectOutline(RenderBuffer, P, CollisionVolume->Dim, Angle,
                                    v4(0, 1, 1, 1));
            }
            
            vec2 HealthBarPos = Monolith->P + v2(0, 0.5f*Monolith->Dim.y + 0.3f);

            real32 BarLength = Map(GameState->MonolithHealth, 0, 100, 0, 1) * 1.5f;
            vec2 HealthBarDim = v2(BarLength, 0.06f);
            PushDrawRect(RenderBuffer, v3(HealthBarPos, 0), HealthBarDim, 0,
                         v4(0.2f, 1.0f, 0.2f, 1));
        }
    }

    for(uint32 EntityIndex = 0;
        EntityIndex < GameState->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = GetEntity(EntityIndex);
        Assert(Entity);
        
        if(!Entity->Inactive && EntityIndex != GameState->MonolithEntityIndex)
        {
            if(!IsInRectangle(SimBounds, Entity->P))
            {
                if(HasComponent(Entity, EntityComponent_Bullet))
                {
                    FreeEntity(GameState, EntityIndex);
                }
            }
        
            // TODO: Make update and render system seperate?
            if(HasComponent(Entity, EntityComponent_Render))
            {
                if(HasComponent(Entity, EntityComponent_Player))
                {
                    /*
                    if(!Player->TextureSlot)
                    {
                        Player->TextureSlot = AllocateTexture(RenderBuffer);
                    }

                    uint8 *TextureData = CreatePixels(&StreamState->TranBlock, 16, 16);
                    PushLoadTexture(RenderBuffer, Player->TextureSlot, false, TextureData, 
                                    16, 16);
                    */
                    PushDrawRect(RenderBuffer, v3(Entity->P, 0), Entity->Dim, Entity->Angle, 
                                    v4(1, 1, 1, 1));
                }

                else if(HasComponent(Entity, EntityComponent_Ball))
                {
                    entity *Enemy = FindEntity(GameState, EntityComponent_Enemy);
                    if(Enemy)
                    {
                        if(GameState->BallFireCount == 110)
                        {
                            vec2 BulletPos = Entity->P;
                            InstantiateBullet(GameState, BulletPos, Enemy->P, 160, 
                                              EntityIndex, EntityComponent_Ball);

                            u32 SoundIndex = GetRandomSoundIndex(0, 5, 5);
                            u16 SoundID = PlaySound(AudioState, Sound + SoundIndex);
                            
                            GameState->BallFireCount = 0;
                        }

                        ++GameState->BallFireCount;
                    }

                    PushDrawRect(RenderBuffer, v3(Entity->P, 0), Entity->Dim, Entity->Angle, v4(0.6f, 0.6f, 0.7f, 1));
                }

                else if(HasComponent(Entity, EntityComponent_Block))
                {
                    PushDrawRect(RenderBuffer, v3(Entity->P, 0), Entity->Dim, Entity->Angle, v4(0.2f, 0.2f, 0.2f, 1));
                }

                else if(HasComponent(Entity, EntityComponent_Enemy))
                {
                    if(!GameState->GameOver)
                    {
                        enemy_stat *Stat = FindEnemyStat(GameState, EntityIndex);
                        if(Stat)
                        {
                            rect2 MonolithZone = RectCenterDim(Monolith->P, v2(10, 10));
                            if(IsInRectangle(MonolithZone, Entity->P))
                            {
                                if(Stat->FireCount == 100)
                                {
                                    vec2 BulletPos = Entity->P;
                                    InstantiateBullet(GameState, BulletPos, Monolith->P, 200, 
                                                      EntityIndex, EntityComponent_Enemy);

                                    u32 SoundIndex = GetRandomSoundIndex(0, 5, 5);
                                    u16 SoundID = PlaySound(AudioState, Sound + SoundIndex);

                                    Stat->FireCount = 0;
                                }

                                ++Stat->FireCount;
                            }
                        }

                        PushDrawRect(RenderBuffer, v3(Entity->P, 0), Entity->Dim, Entity->Angle, v4(0.3f, 0.3f, 0.3f, 1));
                    }
                }

                else if(HasComponent(Entity, EntityComponent_Bullet))
                {
                    PushDrawRect(RenderBuffer, v3(Entity->P, 0), Entity->Dim, Entity->Angle, v4(0.8f, 0.2f, 0.2f, 1));
                }

                else if(HasComponent(Entity, EntityComponent_Land))
                {
                    rect2 RenderBounds = RectCenterDim(Player->P, v2(32, 64));
                    if(IsInRectangle(RenderBounds, Entity->P))
                    {
                        PushDrawMesh(RenderBuffer, Entity->Mesh, v4(0.128f, 0.124f, 0.144f, 1.0f));
                    }
                }
                
                else
                {
                    if(HasComponent(Entity, EntityComponent_Camera))
                        continue;
                    Assert(!"No entity case")
                }
        #if 1
                for(collision_volume *CollisionVolume = Entity->FirstCollisionVolume;
                    CollisionVolume;
                    CollisionVolume = CollisionVolume->Next)
                {
                    vec2 P = Entity->P + CollisionVolume->OffsetP;
                    real32 Angle = Entity->Angle + CollisionVolume->Angle;
                    rect2 CollisionVolumeRect = RectCenterDim(P, CollisionVolume->Dim);
                    if(IsInRectangle(CollisionVolumeRect, WorldMouseP.xy))
                    {
                        PushDrawRectOutline(RenderBuffer, P, CollisionVolume->Dim, Angle,
                                            v4(0, 1, 1, 1));
                    }
                }
        #endif
            }
        }
    }

    EndTempMemory(SimMemory);

    font_slot *GameFontSlot = &RenderBuffer->FontSlots[0];
    font_slot *TitleFontSlot = &RenderBuffer->FontSlots[1];
    if(!StreamState->FontsLoaded)
    {
        char *FontSource = "fonts/arial.ttf";
        PushLoadFont(RenderBuffer, GameFontSlot, FontSource, 20.0f);
        PushLoadFont(RenderBuffer, TitleFontSlot, FontSource, 28.0f);

#if ENGINE_DEBUG
        font_slot *DebugFontSlot = &RenderBuffer->FontSlots[2];
        char *DebugFontSource = "c:/Windows/Fonts/LiberationMono-Regular.ttf";
        PushLoadFont(RenderBuffer, DebugFontSlot, DebugFontSource, 16.0f);
#endif
        StreamState->FontsLoaded = true;
    }

    mat4 HUDMatrix = Translate(v3(0, 0, RenderBuffer->MetersToPixels));
    PushSetCameraMatrix(RenderBuffer, HUDMatrix);

    if(!RenderBuffer->OverlayTexture)
    {
        RenderBuffer->OverlayTexture = AllocateTexture(RenderBuffer);
        PushLoadTexture(RenderBuffer, RenderBuffer->OverlayTexture, false, 
                        "textures/texture0.png");
    }
    r32 HalfWidth = (r32)RenderBuffer->Width;
    r32 HalfHeight = (r32)RenderBuffer->Height;    
    PushDrawRect(RenderBuffer, v3(v2(0, 0) + Offset * 100, 0), v2(HalfWidth, HalfHeight), 0, 
                 v4(1.0f, 1.0f, 1.0f, 0.5f), RenderBuffer->OverlayTexture);


    if(GameState->MonolithHealth <= 0)
    {
        GameState->EnemiesRemaining = 0;

        for(u32 EnemyIndex = 0;
            EnemyIndex < ArrayCount(GameState->Enemies);
            ++EnemyIndex)
        {
            enemy_stat *Stat = &GameState->Enemies[EnemyIndex];
            if(Stat->Index)
            {
                entity *Enemy = GetEntity(Stat->Index);
                if(Enemy)
                {
                    if(Enemy->Inactive == false)
                    {
                        FreeEntity(GameState, Stat->Index);
                    }
                }
            }
        }

        //RenderBuffer->FrameRateValue = 0.0f;
        GameState->GameOver = true;

        real32 xPos = -100;
        real32 yPos = 110;
        real32 LineHeight = 18 + 2;

        char *GameOverString = "GAME OVER";
        PushDrawString(RenderBuffer, GameOverString, v2(-80, yPos), 1.0f, TitleFontSlot,
            v4(0.8f, 0.9f, 0.8f, 1));
        yPos -= LineHeight + 8;

        char *Strings[] = {
            "enemies destroyed: ", "%5u\n",
            "seconds alive: ",     "%5u\n",
        };

        char Stat0[16];
        PushDrawString(RenderBuffer, Strings[0], v2(xPos, yPos), 1.0f, GameFontSlot, v4(0.8f, 0.9f, 0.8f, 1));
        _snprintf_s(Stat0, sizeof(Stat0), Strings[1], GameState->EnemiesDestroyed);
        PushDrawString(RenderBuffer, Stat0, v2(xPos + 170, yPos), 1.0f, GameFontSlot, v4(0.8f, 0.9f, 0.8f, 1));
        yPos -= LineHeight;

        char Stat1[16];
        PushDrawString(RenderBuffer, Strings[2], v2(xPos, yPos), 1.0f, GameFontSlot, v4(0.8f, 0.9f, 0.8f, 1));
        _snprintf_s(Stat1, sizeof(Stat1), Strings[3], (u32)GameState->SecondsAlive);
        PushDrawString(RenderBuffer, Stat1, v2(xPos + 170, yPos), 1.0f, GameFontSlot, v4(0.8f, 0.9f, 0.8f, 1));
        yPos -= LineHeight + 10;

        char *StartAgainString = "press R to try again";
        PushDrawString(RenderBuffer, StartAgainString, v2(-80, yPos), 1.0f, GameFontSlot,
                       v4(0.8f, 0.9f, 0.8f, 1));
        yPos -= LineHeight + 8;
    }
    else
    {
        GameState->SecondsAlive += TimeAdvance;
    }

    if(!GameState->EnemiesRemaining && !GameState->GameOver)
    {
        real32 Value = GameState->TimerValue - GameState->WaveTimer;

        char TimerString[48];
        _snprintf_s(TimerString, sizeof(TimerString),
                    "next attack in: %u seconds\n",//%0.2f\n",
                    (u32)Value);

        PushDrawString(RenderBuffer, TimerString, v2(-100, 0), 1.0f, GameFontSlot,
                       v4(0.8f, 0.9f, 0.8f, 1));
    }

    RenderBuffer->ClearColor = v4(1, 1, 1, 1);

    OverlayDebugInfo(Memory);
}

extern "C" GAME_GENERATE_AUDIO_SAMPLES(GameGenerateAudioSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    PushSoundSamples(&GameState->AudioState, SampleRequest);
}

debug_record DebugRecordArray[__COUNTER__];

extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    if(DebugState)
    {
        DebugState->FrameCyclesElapsed = FrameEnd.CyclesElapsed;
        DebugState->FrameSecondsElapsed = FrameEnd.SecondsElapsed;

        // TODO: Super hacky count!
        DebugState->CounterCount = 0;
        u32 Count = 0;
        for(;;)
        {
            if((DebugRecords_Win + Count++)->Reserved != 0)
            {
                break;
            }
        }
        
        UpdateDebugRecords(DebugState, Count - 1, DebugRecords_Win);
        UpdateDebugRecords(DebugState, ArrayCount(DebugRecords_Main), DebugRecords_Main);
    }
}