#pragma once

// TODO: Work when recompiling!
// Power of 2
#define TERRAIN_RESOLUTION 6
#define TERRAIN_SIZE 16
#define POINT_COUNT (TERRAIN_SIZE * TERRAIN_RESOLUTION + 1)

struct world_chunk
{
    int32 Index;

    vec2 Pos;
    vec2 *Vertices;

    u32 EntityCount;
    entity *Entities[16];
    //u32 LandEntityIndex;    
};