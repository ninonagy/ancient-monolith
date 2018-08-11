// engine_debug.cpp

global_variable real32 xPos;
global_variable real32 yPos;

inline void
DEBUGRect(render_buffer *RenderBuffer, vec2 P, vec2 Dim, r32 Angle, vec4 Color)
{
    PushDrawRect(RenderBuffer, v3(P, 0), Dim, Angle, Color);
}

inline void
DEBUGTextLine(char *String)
{
    yPos -= 16.0f;
    rect2 TextRect = RectMinDim(v2(xPos, yPos + 1), v2(1000, 15));
    if(IsInRectangle(TextRect, DEBUGWindowMouseP))
    {
        PushDrawString(DEBUGRenderBuffer, String, v2(xPos, yPos), 1.0f,
                       0, v4(0.9f, 0.5f, 0.5f, 1));
    }
    else
    {
        PushDrawString(DEBUGRenderBuffer, String, v2(xPos, yPos), 1.0f,
                       0, v4(0.3f, 0.3f, 0.3f, 1));
    }
}

internal void
OverlayDebugInfo(game_memory *Memory)
{
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    if(DebugState)
    {
        // NOTE: Origin is at top left corner of a window!
        r32 HalfWidth = 0.5f*(r32)DEBUGRenderBuffer->Width;
        r32 HalfHeight = 0.5f*(r32)DEBUGRenderBuffer->Height;
        mat4 TextMatrix = Translate(v3(-HalfWidth, -HalfHeight,
                                    DEBUGRenderBuffer->MetersToPixels));
        PushSetCameraMatrix(DEBUGRenderBuffer, TextMatrix);
        
        xPos = 0.0f;
        yPos = DEBUGRenderBuffer->Height;

        //DEBUGRect(DEBUGRenderBuffer, v2(0, 0), v2(1, 1), 0, v4(1, 0, 0, 1));
              
        char FPSBuffer[256];
        _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                    "Last Frame: %.02fms/f, %.02ff/s, %ucy\n",
                    1000.0f*DebugState->FrameSecondsElapsed,
                    1.0f/DebugState->FrameSecondsElapsed,
                    (u32)DebugState->FrameCyclesElapsed);
        DEBUGTextLine(FPSBuffer);

        game_state *GameState = (game_state *)Memory->PermanentStorage;
        stream_state *StreamState = (stream_state *)Memory->StreamingStorage;

        char MemoryBuffer[256];
        _snprintf_s(MemoryBuffer, sizeof(MemoryBuffer),
                    "Tran Memory Used: %u\n",
                     (u32)StreamState->TranBlock.Used);
        DEBUGTextLine(MemoryBuffer);

#if 1
        world_chunk *Chunk = StreamState->CurrentChunk;
        entity *Player = &GameState->Entities[GameState->ControllingEntityIndex];
        if(Chunk)
        {
            char ChunkBuffer[256];
            ZeroStruct(ChunkBuffer);
            _snprintf_s(ChunkBuffer, sizeof(ChunkBuffer),
                        "ChunkIndex: %i, ChunkPos: %i, PlayerPos: %i, EntityCount: %u, FreeEntityCount: %u\n",
                         (int32)Chunk->Index,
                         (int32)Chunk->Pos.x,
                         (int32)Player->P.x,
                         GameState->EntityCount,
                         GameState->FreeEntityCount);
            DEBUGTextLine(ChunkBuffer);
        }

        char MouseBuffer[256];
        ZeroStruct(MouseBuffer);
        _snprintf_s(MouseBuffer, sizeof(MouseBuffer),
                    "WindowMouseP: %i, %i, WorldMouseP: %0.2f, %0.2f\n",
                    (int32)DEBUGWindowMouseP.x,
                    (int32)DEBUGWindowMouseP.y,
                    DEBUGWorldMouseP.x,
                    DEBUGWorldMouseP.y);
        DEBUGTextLine(MouseBuffer);
#endif

#if 0
        DEBUGTextLine("DEBUG CYCLE COUNTERS: ");
        for(uint32 CounterIndex = 0;
            CounterIndex < DebugState->CounterCount;
            ++CounterIndex)
        {
            debug_counter_state *Counter = DebugState->CounterStates + CounterIndex;
            
            if(Counter->CallCount)
            {
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "  %24s: %9ucy %5ucl %10ucy/cl\n",
                            Counter->FunctionName,
                            Counter->CycleCount,
                            Counter->CallCount,
                            Counter->CycleCount / Counter->CallCount);
                DEBUGTextLine(TextBuffer);
            }
        }
#endif
    }
}

internal void
UpdateDebugRecords(debug_state *DebugState, u32 CounterCount, debug_record *Counters)
{
    for(uint32 CounterIndex = 0;
        CounterIndex < CounterCount;
        ++CounterIndex)
    {
        debug_record *Source = Counters + CounterIndex;
        if(Source->FunctionName)
        {
            debug_counter_state *Dest = DebugState->CounterStates + DebugState->CounterCount++;

            u64 CallCount_CycleCount = AtomicExchangeU64(&Source->CallCount_CycleCount, 0);
            Dest->CallCount = (u32)(CallCount_CycleCount >> 32);
            Dest->CycleCount = (u32)(CallCount_CycleCount & 0xFFFFFFFF);
            Dest->FileName = Source->FileName;
            Dest->FunctionName = Source->FunctionName;
            Dest->LineNumber = Source->LineNumber;
        }
    }   
}