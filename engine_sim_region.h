#pragma once

struct sim_region
{
    u32 MaxEntityCount;
    u32 EntityCount;
    entity **Entities;
    u32 IndexTable[256];
};

struct collision_rule
{
    u32 StorageIndexA;
    u32 StorageIndexB;
    bool32 CanCollide;
    collision_rule *NextInHash;
};