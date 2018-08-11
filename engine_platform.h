#pragma once

#define WINDOW_NAME "ancient monolith"
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

#if PROC_EXPORT
    #define PROC_DEC __declspec(dllexport)
#endif
#if PROC_IMPORT
    #define PROC_DEC __declspec(dllimport)
#endif

#if ENGINE_DEBUG
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#define InvalidCodePath Assert(0);
#define InvalidCase default: Assert(0)

#define internal static
#define local static // not useful
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t memory_index;
//typedef uintptr_t memory_index;

typedef bool32 b32;
typedef float r32;
typedef double r64;
typedef float real32;
typedef double real64;

template <class T> void Swap(T& A, T& B)
{
    T C(A); A=B; B=C;
}

#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define FLT_MAX 3.402823466e+38F

#include "engine_string.cpp"

inline bool
IsEven(u32 Value)
{
	if((Value % 2) == 0) return true;
	return false;
}

inline u32
SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return Result;
}


#if ENGINE_DEBUG

extern struct game_memory *DebugGlobalMemory;
#define BEGIN_TIMED_BLOCK_(StartCycleCount) StartCycleCount = __rdtsc();
#define BEGIN_TIMED_BLOCK(ID) uint64 BEGIN_TIMED_BLOCK_(StartCycleCount##ID)
#define END_TIMED_BLOCK_(StartCycleCount, ID) DebugGlobalMemory->Counters[ID].CycleCount += __rdtsc() - StartCycleCount; ++DebugGlobalMemory->Counters[ID].CallCount;
#define END_TIMED_BLOCK(ID) END_TIMED_BLOCK_(DebugCycleCounter_##ID)

// TODO: Add platform api here?
#endif

typedef struct read_file_result
{
    u32 ContentsSize;
    void *Contents;
} read_file_result;

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(char *FileName)
typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#define PLATFORM_WRITE_ENTIRE_FILE(name) bool name(char *FileName, u32 MemorySize, void *Memory)
typedef PLATFORM_WRITE_ENTIRE_FILE(platform_write_entire_file);

#define PLATFORM_ALLOCATE_MEMORY(name) void *name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

#define PLATFORM_LOG(name) void name(char *String)
typedef PLATFORM_LOG(platform_log);

//#define PLATFORM_OUTPUT_DEBUG_STRING(name) void name(char* String)
//typedef PLATFORM_OUTPUT_DEBUG_STRING(platform_output_debug_string);

#define PLATFORM_GET_FILE_TIME(name) int64 name(char* FilePath)
typedef PLATFORM_GET_FILE_TIME(platform_get_file_time);

typedef struct platform_api
{
	platform_allocate_memory *AllocateMemory;
	platform_deallocate_memory *DeallocateMemory;
	platform_free_file_memory *FreeFileMemory;
	platform_read_entire_file *ReadEntireFile;
	platform_write_entire_file *WriteEntireFile;
	platform_get_file_time *GetFileTime;
	platform_log *Log;
} platform_api;

struct game_button_state
{
	int HalfTransitionCount;
	bool EndedDown;
};

struct game_controller_input
{
	bool IsConnected;
	bool IsAnalog;
	
	union
	{
		game_button_state Buttons[14];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;
	
			game_button_state Back;
			game_button_state Start;

			game_button_state Space;
		
			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Key_R;

			// NOTE: All buttons must be added above this line
			game_button_state Terminator;
		};
	};
};

enum game_input_mouse_button
{
	MouseButton_Left,
	MouseButton_Middle,
	MouseButton_Right,
	MouseButton_Extended0,
    MouseButton_Extended1,
	
	MouseButton_Count
};

struct game_input
{
	game_controller_input Controller;

	game_button_state MouseButtons[MouseButton_Count];
    r32 MouseX, MouseY;

	r32 dTimeForFrame;
};
/*
inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	
	game_controller_input *Result = &Input->Controllers[ControllerIndex];
	return(Result);
}
*/

inline b32
WasPressed(game_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) || ((State.HalfTransitionCount == 1) && (State.EndedDown)));

    return Result;
}

struct game_memory
{
    uint64 PermanentStorageSize;
    void *PermanentStorage;

    uint64 StreamingStorageSize;
    void *StreamingStorage;

	uint64 DebugStorageSize;
	void *DebugStorage;

	platform_api PlatformAPI;
};

typedef struct audio_sample_request
{
    s32 SampleRate;
    s32 ChannelCount;
    s32 SampleCount;

    float *SampleBuffer;
} audio_sample_request;

global_variable platform_api Platform;