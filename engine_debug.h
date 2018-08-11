#pragma once

#define TIMED_BLOCK__(Number, ...) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)

struct debug_record
{
    char *FileName;
    char *FunctionName;

    u32 LineNumber;
    u32 Reserved;

    u64 CallCount_CycleCount;
};

global_variable vec2 DEBUGWorldMouseP;
global_variable vec2 DEBUGWindowMouseP;
render_buffer *DEBUGRenderBuffer;

debug_record DebugRecordArray[];

struct timed_block
{
    debug_record *Record;
    u64 StartCycles;
    u32 CallCount;

    timed_block(int Counter, char *FileName, u32 LineNumber, char *FunctionName, int CallCountInit = 1)
    {
        CallCount = CallCountInit;
        Record = DebugRecordArray + Counter;
        Record->FileName = FileName;
        Record->LineNumber = LineNumber;
        Record->FunctionName = FunctionName;

        StartCycles = __rdtsc();
    }

    ~timed_block()
    {
        u64 Delta = (__rdtsc() - StartCycles) | ((u64)CallCount << 32);
        AtomicAddU64(&Record->CallCount_CycleCount, Delta);
    }
};

struct debug_frame_end
{
    u64 CyclesElapsed;
    float SecondsElapsed;
};

struct debug_counter_state
{
    char *FileName;
    char *FunctionName;

    u32 LineNumber;

    u32 CallCount;
    u32 CycleCount;
};

struct debug_state
{
    u32 CounterCount;
    debug_counter_state CounterStates[50];
    u64 FrameCyclesElapsed;
    float FrameSecondsElapsed;
};