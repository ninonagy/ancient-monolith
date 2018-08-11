// engine_sim_region.cpp

internal void
AddSimEntity(sim_region *SimRegion, entity *Source)
{
    if(SimRegion->EntityCount < SimRegion->MaxEntityCount)
    {
        SimRegion->Entities[SimRegion->EntityCount++] = Source;
    }
    else
    {
        InvalidCodePath;
    }
}

internal sim_region *
BeginSim(temp_memory SimMemory, stream_state *StreamState, game_state *GameState, rect2 Bounds)
{
    TIMED_BLOCK();

    sim_region *SimRegion = PushStruct(SimMemory.Block, sim_region);
    SimRegion->MaxEntityCount = 256;
    SimRegion->EntityCount = 0;
    SimRegion->Entities = PushArray(SimMemory.Block, SimRegion->MaxEntityCount, entity*);
    ZeroSize(sizeof(entity*)*SimRegion->MaxEntityCount, (uint8 *)SimRegion->Entities);
    Assert(SimRegion->MaxEntityCount <= ArrayCount(SimRegion->IndexTable))
    ZeroStruct(SimRegion->IndexTable);

#if 1
    u32 IndexTableIndex = 0;
    for(u32 EntityIndex = 0;
        EntityIndex < GameState->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = GameState->Entities + EntityIndex;
        if(!Entity->Inactive)
        {
            // TODO: CameraSpace for floating point precision on larger worlds
            if(IsInRectangle(Bounds, Entity->P))
            {
                AddSimEntity(SimRegion, Entity);
                u32 *IndexValue = &SimRegion->IndexTable[IndexTableIndex++];
                *IndexValue = EntityIndex;
            }
        }
    }
#endif

#if 0
    // TODO: Linked list for chunks?
    for(u32 ChunkIndex = 0;
        ChunkIndex < StreamState->ChunkCount;
        ++ChunkIndex)
    {
        world_chunk *Chunk = StreamState->Chunks + ChunkIndex;
        if(Chunk)
        {
            for(u32 EntityIndex = 0;
                EntityIndex < Chunk->EntityCount;
                ++EntityIndex)
            {
                entity *Entity = Chunk->Entities[EntityIndex];
                if(Entity)
                {
                    if(IsInRectangle(Bounds, Entity->P))
                    {
                        AddSimEntity(SimRegion, Entity);
                    }
                }
            }
        }
    }
#endif

    return SimRegion;
}

internal void
ClearCollisionRulesFor(game_state *GameState, u32 StorageIndex)
{
    for(u32 HashSlot = 0;
        HashSlot < ArrayCount(GameState->CollisionRuleHash);
        ++HashSlot)
    {
        for(collision_rule **Rule = &GameState->CollisionRuleHash[HashSlot];
            *Rule;
            )
        {
            if(((*Rule)->StorageIndexA == StorageIndex) ||
               ((*Rule)->StorageIndexB == StorageIndex))
            {
                collision_rule *RemovedRule = *Rule;
                *Rule = (*Rule)->NextInHash;

                RemovedRule->NextInHash = GameState->FirstFreeCollisionRule;
                GameState->FirstFreeCollisionRule = RemovedRule;
            }
            else
            {
                Rule = &(*Rule)->NextInHash;
            }
        }
    }
}

internal void
AddCollisionRule(game_state *GameState, u32 StorageIndexA, u32 StorageIndexB,
                 b32 CanCollide)
{
    if(StorageIndexA > StorageIndexB)
    {
        Swap(StorageIndexA, StorageIndexB);
    }

    collision_rule *Found = 0;
    u32 HashSlot = StorageIndexA & (ArrayCount(GameState->CollisionRuleHash) - 1);
    for(collision_rule *Rule = GameState->CollisionRuleHash[HashSlot];
        Rule;
        Rule = Rule->NextInHash)
    {
        if(Rule->StorageIndexA == StorageIndexA &&
           Rule->StorageIndexB == StorageIndexB)
        {
            Found = Rule;
            break;
        }
    }
    
    if(!Found)
    {
        Found = GameState->FirstFreeCollisionRule;
        if(Found)
        {
            GameState->FirstFreeCollisionRule = Found->NextInHash;
        }
        else
        {
            Found = PushStruct(&GameState->WorldBlock, collision_rule);
        }

        Found->NextInHash = GameState->CollisionRuleHash[HashSlot];
        GameState->CollisionRuleHash[HashSlot] = Found;
    }

    if(Found)
    {
        Found->StorageIndexA = StorageIndexA;
        Found->StorageIndexB = StorageIndexB;
        Found->CanCollide = CanCollide;
    }
}