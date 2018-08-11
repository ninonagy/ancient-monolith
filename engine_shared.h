#pragma once

#include "engine_platform.h"

struct memory_block
{
    memory_index Used;
    memory_index Size;
    uint8 *Base;
};

struct temp_memory
{
    memory_index Used;
    memory_block *Block;
};

#define PushStruct(Memory, type) (type *)PushSize_(Memory, sizeof(type))
#define PushArray(Memory, Count, type) (type *)PushSize_(Memory, Count*sizeof(type))
#define PushSize(Memory, Size) PushSize_(Memory, Size)

inline void *
PushSize_(memory_block *Block, memory_index Size)
{
    Assert((Block->Used + Size) <= Block->Size);
    void *Result = Block->Base + Block->Used;
    Block->Used += Size;
    
    return Result;
}

#define ZeroStruct(Instance) (sizeof(Instance), (Instance))
inline void
ZeroSize(memory_index Size, void *Source)
{
    uint8 *Byte = (uint8 *)Source;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

#define TempPointer(TempMemory, type) (type *)(TempMemory.Block->Base + TempMemory.Used)
#define MemoryPointer(Memory, type) (type *)(Memory.Base + Memory.Used)
#define MemoryBase(Memory, type) (type *)(Memory.Base)

internal void
InitializeBlock(memory_block *Block, memory_index Size, void *Base)
{
    Block->Used = 0;
    Block->Size = Size;
    Block->Base = (uint8 *)Base;
}

inline temp_memory
BeginTempMemory(memory_block *Block)
{
    temp_memory Result = {};
    
    Result.Used = Block->Used;
    Result.Block = Block;
    
    return Result;
}

inline void
EndTempMemory(temp_memory Temp)
{
    memory_block *Block = Temp.Block;
    Assert(Block->Used <= Block->Size);
    // NOTE: Temp.Used is marking value that was used
    //       when BeginTempMemory has been called.
    Block->Used -= Block->Used - Temp.Used;
}