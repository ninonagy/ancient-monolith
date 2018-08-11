#include "engine_precompiled.h"

#include "engine.h"
#include "engine_win32.h"

#include "engine_sound.cpp"
#include "engine_renderer.cpp"
#include "engine_opengl.cpp"

#include <Shlwapi.h>

#define CINTERFACE
#include <initguid.h>

// TODO: Move this to GLState
global_variable bool GlobalSampleBuffers;
global_variable int32 GlobalMultiSampleCount;
global_variable float GlobalSamplingMultiplier;

global_variable u32 WindowWidth = WINDOW_WIDTH;
global_variable u32 WindowHeight = WINDOW_HEIGHT;

global_variable int64 GlobalPerfCountFrequency;
global_variable bool GlobalRunning;
global_variable bool GlobalPause;
global_variable HANDLE LogFileHandle;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

PLATFORM_LOG(Win32Log)
{
    if(LogFileHandle)
    {
        SetFilePointer(LogFileHandle, 0, 0, FILE_END);

        DWORD BytesWritten;
        WriteFile(LogFileHandle, String, (DWORD)StringLength(String), &BytesWritten, 0);
        if(BytesWritten != StringLength(String))
        {
            Assert("BytesWritten is not the size that should've been written");
        }

        BytesWritten = 0;
        WriteFile(LogFileHandle, "\r\n", 2, &BytesWritten, 0);
    }
}

#include "win32_sound.cpp"

global_variable win32_audio Win32Audio;

DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

#define LOG_FORMATTED_ERROR(size)                                                                                      \
    char _ErrorString[size];                                                                                           \
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, _ErrorString, sizeof(_ErrorString), 0);            \
    Win32Log(_ErrorString);

local void
Win32BuildEXEPathFileName(win32_state *State, char *FileName,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
               StringLength(FileName), FileName, DestCount, Dest);
}

local LARGE_INTEGER
Win32GetWallClock(void)
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

local float
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	float Result = ((float)(End.QuadPart - Start.QuadPart) /
				    (float)GlobalPerfCountFrequency);
	return(Result);
}

local void
Win32GetEXEFileName(win32_state *State)
{
    State->EXEFileName[MAX_PATH];
    DWORD SizeOfFileName = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for(char *Scan = State->EXEFileName;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

inline FILETIME
Win32GetLastWriteTime(char *FileName)
{
    FILETIME LastWriteTime = {};
    
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFileA(FileName, &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE)
    {
        LastWriteTime = FindData.ftLastWriteTime;
        FindClose(FindHandle);
    }
    
    return LastWriteTime;
}

PLATFORM_FREE_FILE_MEMORY(Win32PlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

PLATFORM_READ_ENTIRE_FILE(Win32PlatfromReadEntireFile)
{
    read_file_result Result = {0};

    HANDLE File = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(File != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(File, &FileSize))
        {
            u32 FileSize32  = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(File, Result.Contents, FileSize32, &BytesRead, 0) && BytesRead == FileSize32)
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    Win32Log(FileName);
                    Win32Log("BytesRead was not equal FileSize32");
                    Win32PlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
                Win32Log(FileName);
                Win32Log("Couldnt allocate memory for file read");
            }
        }
        else
        {
            Win32Log(FileName);
            Win32Log("Coudlnt get file size");
            LOG_FORMATTED_ERROR(4096);
        }

        CloseHandle(File);
    }
    else
    {
        Win32Log(FileName);
        Win32Log("Couldnt create file handle");
        LOG_FORMATTED_ERROR(4096);
    }

    return Result;
}

PLATFORM_WRITE_ENTIRE_FILE(Win32PlatformWriteEntireFile)
{
    bool Result = false;

    HANDLE File = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(File != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(File, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            Win32Log(FileName);
            Win32Log("Could not write memory into file");
        }


        CloseHandle(File);
    }
    else
    {
        Win32Log(FileName);
        Win32Log("Could not create file name");
        LOG_FORMATTED_ERROR(4096);
    }

    return Result;
}

/*PLATFORM_OUTPUT_DEBUG_STRING(Win32PlatformOutputDebugString)
{
    OutputDebugString(String);
}*/

PLATFORM_GET_FILE_TIME(Win32PlatformGetFileTime)
{
    int64 FileWriteTime = 0;

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(FilePath, GetFileExInfoStandard, &Data))
    {
        FileWriteTime = (((int64)Data.ftLastWriteTime.dwHighDateTime << 32) + Data.ftLastWriteTime.dwLowDateTime);
    }

    return (FileWriteTime);
}

PLATFORM_ALLOCATE_MEMORY(Win32PlatformAllocateMemory)
{
    void *Result = VirtualAlloc(0, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    return (Result);
}

PLATFORM_DEALLOCATE_MEMORY(Win32PlatformDeallocateMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal win32_game_code
Win32LoadGameCode(char *FileName, char *TempDLLName)
{
    win32_game_code Result = {};

#if ENGINE_DEBUG
    Result.LastWriteTime = Win32GetLastWriteTime(FileName);
 
    CopyFile(FileName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);
    if(Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render *)
            GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
        Result.GenerateAudioSamples = (game_generate_audio_samples *)
            GetProcAddress(Result.GameCodeDLL, "GameGenerateAudioSamples");
        Result.DEBUGFrameEnd = (debug_game_frame_end *)
            GetProcAddress(Result.GameCodeDLL, "DEBUGGameFrameEnd");

        Result.IsValid = (Result.UpdateAndRender && Result.GenerateAudioSamples);
    }
    else
    {
        Win32Log("Could not load engine_core.dll");
    }
#else

    Result.GameCodeDLL = LoadLibraryA(FileName);
    if(Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render *)
            GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
        Result.GenerateAudioSamples = (game_generate_audio_samples *)
            GetProcAddress(Result.GameCodeDLL, "GameGenerateAudioSamples");
        Result.DEBUGFrameEnd = (debug_game_frame_end *)
            GetProcAddress(Result.GameCodeDLL, "DEBUGGameFrameEnd");
        
        Result.IsValid = (Result.UpdateAndRender && Result.GenerateAudioSamples);
    }
    else
    {
        // TODO: Message Box!
        Win32Log("Could not load engine_core.dll");
    }
#endif
    
    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GenerateAudioSamples = 0;
    }
    
    return Result;
}

local void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
    GameCode->GenerateAudioSamples = 0;
}

internal void
Win32GetSystemInfo()
{
    int32 WindowsMajorVersion = 0;
    int32 WindowsMinorVersion = 0;
    int32 WindowsBuildNumber = 0;

    char KernelPath[4096];
    GetSystemDirectory(KernelPath, sizeof(KernelPath));
    PathAppend(KernelPath, "kernel32.dll");

    DWORD KernelFileVersionInfoSize = GetFileVersionInfoSize(KernelPath, 0);

    DWORD Error = GetLastError();

    if(KernelFileVersionInfoSize != 0)
    {
        void* KernelVersionInfo = Win32PlatformAllocateMemory(KernelFileVersionInfoSize);

        if(GetFileVersionInfo(KernelPath, 0, KernelFileVersionInfoSize, KernelVersionInfo))
        {
            VS_FIXEDFILEINFO *KernelVersionFileInfo = 0;
            UINT KernelVersionFileInfoLength = 0;
            if(VerQueryValue(KernelVersionInfo, "\\", (LPVOID*)&KernelVersionFileInfo, &KernelVersionFileInfoLength))
            {
                WindowsMajorVersion = HIWORD(KernelVersionFileInfo->dwProductVersionMS);
                WindowsMinorVersion = LOWORD(KernelVersionFileInfo->dwProductVersionMS);

                if(WindowsMinorVersion == 5)
                {
                    if(WindowsMinorVersion == 0)
                    {
                        MessageBoxA(0,
                                    "This application requires at least Windows Vista to run",
                                    "Windows Version Error",
                                    MB_OK);
                        ExitProcess(1);
                    }
                    else if(WindowsMinorVersion == 1)
                    {
                        MessageBoxA(0,
                                    "This application requires at least Windows Vista to run",
                                    "Windows Version Error",
                                    MB_OK);
                        ExitProcess(1);
                    }
                }

                // TODO(zak): When we start supporting 32-bit add a check if the user is running x86 or x64
                if(WindowsMinorVersion == 6)
                {
                    if(WindowsMinorVersion == 0)
                    {
                        Win32Log("Operating System: Windows Vista");
                    }
                    else if(WindowsMinorVersion == 1)
                    {
                        Win32Log("Operating System: Windows 7");
                    }

                    else if(WindowsMinorVersion == 2)
                    {
                        Win32Log("Operating System: Windows 8");
                    }
                    else if(WindowsMinorVersion == 3)
                    {
                        Win32Log("Operating System: Windows 8.1");
                    }
                }

                if(WindowsMajorVersion == 10)
                {
                    Win32Log("Operating System: Windows 10");
                }
            }
            else
            {
                Win32Log("Version info could not be found");
            }
        }
        else
        {
            Win32Log("Could not successfully GetFileVersionInfo on KernelVersionInfo");
        }
    }
    else
    {
        Win32Log("Could not query KernelFileVersionInfoSize");
    }

    Win32Log("===  CPU  ====");
    SYSTEM_INFO SystemInfo = {0};
    GetSystemInfo(&SystemInfo);

    if(SystemInfo.dwProcessorType == 386)
    {
        Win32Log("Processor Type: Intel 386");
    }
    else if(SystemInfo.dwProcessorType == 486)
    {
        Win32Log("Processor Type: Intel 486");
    }
    else if(SystemInfo.dwProcessorType == 586)
    {
        Win32Log("Processor Type: Intel 586 (Pentium)");
    }
    else if(SystemInfo.dwProcessorType == 2200)
    {
        Win32Log("Processor Type: Intel IA64");
    }
    else if(SystemInfo.dwProcessorType == 8664)
    {
        Win32Log("Processor Type: AMD x8664");
    }
    else
    {
        Win32Log("ARM or Undefined");
    }

    if(SystemInfo.wProcessorArchitecture == 9)
    {
        Win32Log("Processor Architecture: x64");
    }
    else if(SystemInfo.wProcessorArchitecture == 5)
    {
        Win32Log("Processor Architecture: ARM");
    }
    else if(SystemInfo.wProcessorArchitecture == 6)
    {
        Win32Log("Processor Architecture: Intel Itanium-based");
    }
    else if(SystemInfo.wProcessorArchitecture == 0)
    {
        Win32Log("Processor Architecture: x86");
    }
    else if(SystemInfo.wProcessorArchitecture == 0xffff)
    {
        Win32Log("Processor Architecture: Unknown architecture");
    }

    // TODO(Hoej): Investigate if __CPUID is reliable
    char CoreBuffer[60];
    sprintf_s(CoreBuffer, "Number of Logical Cores: %d", SystemInfo.dwNumberOfProcessors);
    Win32Log(CoreBuffer);

    MEMORYSTATUSEX MemoryStatus = {0};
    MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&MemoryStatus);

    Win32Log("--- Memory ------");
    char PhysicalMemoryBuffer[256];
    sprintf_s(PhysicalMemoryBuffer,
              "Physical Memory: %llu bytes \nIn Use: %llu bytes (%lu%%) \nFree: %llu bytes",
              MemoryStatus.ullTotalPhys,
              MemoryStatus.ullTotalPhys - MemoryStatus.ullAvailPhys,
              MemoryStatus.dwMemoryLoad,
              MemoryStatus.ullAvailPhys);
    Win32Log(PhysicalMemoryBuffer);
}

local void
Win32GetInputFileLocation(win32_state *State, int SlotIndex, int DestCount, char *Dest)
{
    Assert(SlotIndex == 1);
    Win32BuildEXEPathFileName(State, "engine_input.erc", DestCount, Dest);
}

local void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
    State->InputRecordingIndex = InputRecordingIndex;
    
    char FileName[MAX_PATH];
    Win32GetInputFileLocation(State, InputRecordingIndex, sizeof(FileName), FileName);
    
    State->RecordingHandle =
        CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
}

local void
Win32EndRecordingInput(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

local void
Win32BeginInputPlayBack(win32_state *State, int InputPlayingIndex)
{
    State->InputPlayingIndex = InputPlayingIndex;
    
    char FileName[MAX_PATH];
    Win32GetInputFileLocation(State, InputPlayingIndex, sizeof(FileName), FileName);
    
    State->PlayBackHandle =
        CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
}

local void
Win32EndInputPlayBack(win32_state *State)
{
    CloseHandle(State->PlayBackHandle);
    State->InputPlayingIndex = 0;
}

local void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

local void
Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesRead = 0;
    if(ReadFile(State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
    {
        if(BytesRead == 0)
        {
            int PlayingIndex = State->InputPlayingIndex;
            Win32EndInputPlayBack(State);
            Win32BeginInputPlayBack(State, PlayingIndex);
            ReadFile(State->PlayBackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
        }
    }
}

local void
Win32SetPixelFormat(HDC WindowDC)
{
    int SuggestedPixelFormatIndex = 0;
    uint32 ExtendedPick = 0;
            
    if(wglChoosePixelFormatARB)
    {        
        int AttribList[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
            WGL_SAMPLES_ARB, 4,
            0,
        };
        
        wglChoosePixelFormatARB(WindowDC, AttribList, 0, 1,
            &SuggestedPixelFormatIndex, &ExtendedPick);
    }
    
    if(!ExtendedPick)
    {
        PIXELFORMATDESCRIPTOR PixelFormat = {};
        PixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        PixelFormat.nVersion = 1;
        PixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        PixelFormat.iPixelType = PFD_TYPE_RGBA;
        PixelFormat.cColorBits = 32;
        PixelFormat.cAlphaBits = 8;
        PixelFormat.iLayerType = PFD_MAIN_PLANE;
                
        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &PixelFormat);
    }
    
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat); 
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

local void
Win32LoadWGLExtensions()
{
    WNDCLASSA WindowClass = {};
	WindowClass.lpfnWndProc = DefWindowProcA;
	WindowClass.hInstance = GetModuleHandle(0);
	WindowClass.lpszClassName = "EngineWGLLoader";
    
	if (RegisterClassA(&WindowClass))
	{
		HWND Window = CreateWindowExA(
            0,
			WindowClass.lpszClassName,
			"Engine WGL Loader",
			0,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			WindowClass.hInstance,
			0);
        
        if(Window)
        {
            HDC WindowDC = GetDC(Window);
            Win32SetPixelFormat(WindowDC);
            HGLRC OpenGLRC = wglCreateContext(WindowDC);
            
            if(wglMakeCurrent(WindowDC, OpenGLRC))
            {
                wglSwapInterval = (wgl_swap_interval_ext *)
                    wglGetProcAddress("wglSwapIntervalEXT");
                if(!wglSwapInterval)
                {
                    Win32Log("Couldnt get wglSwapInternalEXT extension");
                }

                wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb *)
                    wglGetProcAddress("wglChoosePixelFormatARB");
                if(!wglChoosePixelFormatARB)
                {
                    Win32Log("Couldnt get wglChoosePixelFormatARB extension");
                }
                
                wglCreateContextAttribsARB = (wgl_create_context_attribts_arb *)
                    wglGetProcAddress("wglCreateContextAttribsARB");
                if(!wglCreateContextAttribsARB)
                {
                    Win32Log("Couldnt get wglCreateContextAttribsARB extension");
                }
                
                /*
                wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)
                    wglGetProcAddress("wglGetExtensionsStringEXT");
                char *Extensions = (char *)wglGetExtensionsStringEXT();
                */

                wglMakeCurrent(0, 0);
            }

            wglDeleteContext(OpenGLRC);
            ReleaseDC(Window, WindowDC);
            DestroyWindow(Window);
        }
        else
        {
            Win32Log("Couldnt create WGL Loader Window");
            LOG_FORMATTED_ERROR(4096);
        }
    }
    else
    {
        Win32Log("Couldnt register class for WGL Loader Window");
        LOG_FORMATTED_ERROR(4096);
    }
}

internal void
Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();
    
    HGLRC OpenGLRC = 0;
    bool32 ModernContext = true;

    if(wglCreateContextAttribsARB)
    {
        // NOTE(nino): This is modern version of OpenGL
        int Win32OpenGLAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 1,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if ENGINE_DEBUG
                |
                WGL_CONTEXT_DEBUG_BIT_ARB
#endif
            ,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        
        Win32SetPixelFormat(WindowDC);
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }
    
    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC = wglCreateContext(WindowDC);
    }
    
    if(wglMakeCurrent(WindowDC, OpenGLRC))
    {
        InitOpenGL(ModernContext);
        
        if(wglSwapInterval)
        {
            // 0 = DisableVSync
            // 1 = EnableVSync
            //-1 = EnableAdaptiveVSync; EXT_swap_control_tear
            wglSwapInterval(1);
        }
    }
    else
    {
        Win32Log("Couldnt set OpenGL context");
        LOG_FORMATTED_ERROR(4096);
    }
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool IsDown)
{
	if(NewState->EndedDown != IsDown)
    {
	   NewState->EndedDown = IsDown;
       ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input *KeyboardController)
{
	MSG Message;
	while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		switch(Message.message)
		{
			case WM_QUIT:
			{
                GlobalRunning = false;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				u32 VKCode = (u32)Message.wParam;
				bool WasDown = ((Message.lParam & (1 << 30)) != 0);
				bool IsDown = ((Message.lParam & (1 << 31)) == 0);
				if (WasDown != IsDown)
				{
					if(VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
					}
					else if(VKCode == 'A')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
					}
					else if(VKCode == 'S')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
					}
					else if(VKCode == 'D')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
					}
					else if(VKCode == VK_UP)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
					}
					else if(VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
					}
					else if(VKCode == VK_DOWN)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
					}
					else if(VKCode == VK_RIGHT)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
					}
					else if(VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
					}
                    else if(VKCode == VK_RETURN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
					else if(VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Space, IsDown);
					}
                    else if(VKCode == 'Q')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
					}
					else if(VKCode == 'E')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
					}
                    else if(VKCode == 'R')
					{
						Win32ProcessKeyboardMessage(&KeyboardController->Key_R, IsDown);
					}
            #if ENGINE_DEBUG
                    else if(VKCode == 'L')
                    {
                        if(IsDown)
                        {
                            if(State->InputRecordingIndex == 0)
                            {
                                Win32BeginRecordingInput(State, 1);
                            }
                            else
                            {
                                Win32EndRecordingInput(State);
                                Win32BeginInputPlayBack(State, 1);
                            }
                        }
                    }
            #endif

                    if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }

                    if(VKCode == VK_ESCAPE)
                    {
                        if(IsDown)
                        {
                            GlobalRunning = false;
                        }
                    }

                    if(IsDown)
                    {
                        bool AltKeyWasDown = (Message.lParam & (1 << 29));
				        if((VKCode == VK_F4) && AltKeyWasDown)
				        {
				        	GlobalRunning = false;
				        }
                        if((VKCode == VK_RETURN) && AltKeyWasDown)
                        {
                            if(Message.hwnd)
                            {
                                Win32ToggleFullscreen(Message.hwnd);
                            }
                        }
                    }
				}
			} break;

			default:
			{
                TranslateMessage(&Message);
				DispatchMessageA(&Message);
			} break;
		}
	}
}

LRESULT CALLBACK 
Win32MainWindowCallback(HWND Window, 
						UINT Message,
						WPARAM WParam, 
						LPARAM LParam)
{
	LRESULT Result = 0;

	switch(Message)
	{	
        case WM_ACTIVATEAPP:
		{
		} break;

        case WM_SIZE:
        {
            WindowWidth = LOWORD(LParam);
            WindowHeight = HIWORD(LParam);
            
            //OpenGLUpdateView(WindowWidth, WindowHeight);
        } break;

		case WM_CLOSE:
		{
			GlobalRunning = false;
            DestroyWindow(Window);
		} break;
	
		case WM_DESTROY:
		{   
            GlobalRunning = false;
            PostQuitMessage(0);
		} break;

		default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

struct win32_thread_info
{
    int LogicalThreadIndex;
};
/*
DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{    
    for(;;)
    {
        
    }
}
*/
int CALLBACK
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR CommandLine,
		int ShowCode)
{	
    SetCurrentDirectoryA("data");
    LogFileHandle = CreateFileA("log.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    Win32Log("Log File");

    //Win32GetSystemInfo();

    win32_state Win32State = {};
    /*
    char *Param = "Thread started!\n";
    
    win32_thread_info ThreadInfo;
    
    uint32 ThreadCount = 0;
    for(uint32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ++ThreadIndex)
    {
        ThreadInfo.LogicalThreadIndex = ThreadIndex;
        
        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, &ThreadInfo, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
    */
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    Win32GetEXEFileName(&Win32State);
    
    char SourceGameCodeDLLFullPath[MAX_PATH];
    Win32BuildEXEPathFileName(&Win32State, "engine_core.dll",
                              sizeof(SourceGameCodeDLLFullPath),
                              SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[MAX_PATH];
    Win32BuildEXEPathFileName(&Win32State, "engine_temp.dll",
                              sizeof(TempGameCodeDLLFullPath),
                              TempGameCodeDLLFullPath);
    
    win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                             TempGameCodeDLLFullPath);
		
	UINT DesiredSchedulerMS = 1;
	bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	WNDCLASSA WindowClass = {};
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    //WindowClass.hIcon;
	WindowClass.lpszClassName = "EngineWindowClass";
    
    // TODO: Remove these!
    GlobalSampleBuffers = true;
    GlobalMultiSampleCount = 8;
    GlobalSamplingMultiplier = 2;
    
    DWORD WindowStyles = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
                         WS_VISIBLE | WS_OVERLAPPEDWINDOW;

    RECT WindowRect = {};
    WindowRect.left = 0;
    WindowRect.top = 0;
    WindowRect.right = WINDOW_WIDTH;
    WindowRect.bottom = WINDOW_HEIGHT;
    AdjustWindowRect(&WindowRect, WindowStyles, 0);

	if(RegisterClassA(&WindowClass))
	{
		HWND Window =
			CreateWindowExA(
				0,
				WindowClass.lpszClassName,
				WINDOW_NAME,
				WindowStyles,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				WindowRect.right - WindowRect.left,
				WindowRect.bottom - WindowRect.top,
				0,
				0,
				Instance,
				0);
		if(Window)
		{
            // Win32ToggleFullscreen(Window);

            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }           
            float GameUpdateHz = MonitorRefreshHz;
	        float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;
            
#if ENGINE_DEBUG
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.StreamingStorageSize = Megabytes(256);
            GameMemory.DebugStorageSize = Megabytes(8);
            GameMemory.PlatformAPI.AllocateMemory = Win32PlatformAllocateMemory;
            GameMemory.PlatformAPI.DeallocateMemory = Win32PlatformDeallocateMemory;
            GameMemory.PlatformAPI.FreeFileMemory = Win32PlatformFreeFileMemory;
            GameMemory.PlatformAPI.ReadEntireFile = Win32PlatfromReadEntireFile;
            GameMemory.PlatformAPI.WriteEntireFile = Win32PlatformWriteEntireFile;
            GameMemory.PlatformAPI.GetFileTime = Win32PlatformGetFileTime;
            //GameMemory.PlatformAPI.OutputDebugString = Win32PlatformOutputDebugString;
            GameMemory.PlatformAPI.Log = Win32Log;

            Platform = GameMemory.PlatformAPI;
            
        #if ENGINE_DEBUG
            Win32State.TotalSize = GameMemory.PermanentStorageSize +
                                   GameMemory.StreamingStorageSize + 
                                   GameMemory.DebugStorageSize;
        #else
            Win32State.TotalSize = GameMemory.PermanentStorageSize +
                                   GameMemory.StreamingStorageSize;
        #endif

            Win32State.GameMemoryBlock = 
                VirtualAlloc(BaseAddress, (memory_index)Win32State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.StreamingStorage = (uint8*)GameMemory.PermanentStorage + 
                                          GameMemory.PermanentStorageSize;
        #if ENGINE_DEBUG                        
            GameMemory.DebugStorage = (uint8*)GameMemory.StreamingStorage + 
                                      GameMemory.StreamingStorageSize;
        #endif

            render_buffer RenderBuffer = AllocateRenderBuffer(Megabytes(4));

            if(GameMemory.PermanentStorage && GameMemory.StreamingStorage && RenderBuffer.Base)
            {
                HDC WindowDC = GetDC(Window);
                Win32InitOpenGL(WindowDC);
                ReleaseDC(Window, WindowDC);

                HRESULT Hr = CoInitializeEx(0, COINIT_MULTITHREADED);
                if(Hr == 0)
                {
                    Hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
                                          0,
                                          CLSCTX_ALL,
                                          IID_IMMDeviceEnumerator,
                                          (void**)&Win32Audio.DeviceEnumerator);
                    
                    if(Hr == 0 && Win32Audio.DeviceEnumerator)
                    {
                        Win32StartDefaultAudioOutput(&Win32Audio);
                    }
                    else
                    {
                        Win32Log("Could not create audio device enumerator");
                    }
                }
                else
                {
                    Win32Log("Could not initialize COM on main thread");
                }

                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                GlobalRunning = true;
                
                LARGE_INTEGER LastCounter = Win32GetWallClock();
                
                uint64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    debug_frame_end FrameEnd = {};

                    NewInput->dTimeForFrame = TargetSecondsPerFrame * RenderBuffer.FrameRateValue;

            #if ENGINE_DEBUG
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.LastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath);
                    }
            #endif
                    game_controller_input *OldKeyboardController = &OldInput->Controller;
                    game_controller_input *NewKeyboardController = &NewInput->Controller;
                    game_controller_input ZeroController = {};
                    *NewKeyboardController = ZeroController;
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }
                    
                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

                    if(!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = (r32)MouseP.x;
                        NewInput->MouseY = (r32)((WindowHeight - 1) - MouseP.y);

                        DWORD WinButtonID[MouseButton_Count] = {
                            VK_LBUTTON, VK_MBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2,
                        };

                        for(u32 ButtonIndex = 0;
                            ButtonIndex < MouseButton_Count;
                            ButtonIndex++)
                        {
                            NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
                            NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[ButtonIndex],
                                                        GetKeyState(WinButtonID[ButtonIndex]) & (1 << 15));
                        }

                #if ENGINE_DEBUG
                        if(Win32State.InputRecordingIndex)
                        {
                            Win32RecordInput(&Win32State, NewInput);
                        }
                        if(Win32State.InputPlayingIndex)
                        {
                            Win32PlayBackInput(&Win32State, NewInput);
                        }
                #endif
                        
                        BeginRenderBuffer(&RenderBuffer);
                        if(Game.UpdateAndRender)
                        {
                            RenderBuffer.Width = WindowWidth;
                            RenderBuffer.Height = WindowHeight;
                            OpenGLUpdateView(&RenderBuffer, 
                                             RenderBuffer.Width, 
                                             RenderBuffer.Height);
                            Game.UpdateAndRender(&GameMemory, NewInput, &RenderBuffer);
                        }
                        EndRenderBuffer(&RenderBuffer);
                        
                        if(Win32Audio.DeviceLost)
                        {
                            Win32ReleaseAudioWrite(&Win32Audio);
                            Win32StartDefaultAudioOutput(&Win32Audio);
                            Win32Audio.DeviceLost = false;
                        }
                        if(Win32Audio.Enabled)
                        {
                            u32 Padding;
                            b32 Success = false;
                            Hr = Win32Audio.WriteClient->lpVtbl->GetCurrentPadding(Win32Audio.WriteClient, &Padding);
                            if(Hr == 0)
                            {
                                s32 WriteAmmount = Win32Audio.BufferSize - Padding;
                                BYTE* Buffer;
                                if(WriteAmmount <= Win32Audio.BufferSize && WriteAmmount > 0)
                                {
                                    Hr = Win32Audio.RenderClient->lpVtbl->GetBuffer(
                                        Win32Audio.RenderClient, WriteAmmount, &Buffer);
                                    if(Hr == 0)
                                    {
                                        ZeroMemory(Buffer, WriteAmmount * sizeof(float) * Win32Audio.ChannelCount);
                                        audio_sample_request SampleRequest = {0};
                                        SampleRequest.SampleRate           = Win32Audio.SampleRate;
                                        SampleRequest.ChannelCount         = Win32Audio.ChannelCount;
                                        SampleRequest.SampleCount          = WriteAmmount;
                                        SampleRequest.SampleBuffer         = (float*)Buffer;
                                        if(Game.GenerateAudioSamples)
                                        {
                                            Game.GenerateAudioSamples(&GameMemory, &SampleRequest);
                                        }
                                        Hr = Win32Audio.RenderClient->lpVtbl->ReleaseBuffer(
                                            Win32Audio.RenderClient, WriteAmmount, 0);
                                        if(Hr == 0)
                                        {
                                            Success = true;
                                        }
                                    }
                                }
                                else
                                {
                                    Success = true;
                                }
                            }
                            if(!Success)
                            {
                                Win32Audio.DeviceLost = true;
                                Win32Log("Device lost, trying to refresh");
                            }                        
                        }

                        OpenGLRenderScene(&RenderBuffer);
                        
                #if ENGINE_DEBUG
                        CheckGLError();
                #endif

                        HDC SwapBufferDC = GetDC(Window);
                        SwapBuffers(SwapBufferDC);
                        ReleaseDC(Window, SwapBufferDC);

                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                        if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if(SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - 
                                    SecondsElapsedForFrame));

                                if(SleepMS > DesiredSchedulerMS)
                                {
                                    Sleep(SleepMS - DesiredSchedulerMS);
                                }
                            }
                            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
                                                                                Win32GetWallClock());
                            }
                        }
                        else
                        {
                            // TODO(nino): Missed frame!
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        FrameEnd.SecondsElapsed = Win32GetSecondsElapsed(LastCounter, EndCounter);
                        //float MSPerFrame = 1000.0f*CounterElapsed;
                        LastCounter = EndCounter;

                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

                        uint64 EndCycleCount = __rdtsc();
                        FrameEnd.CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        if(Game.DEBUGFrameEnd)
                        {
                            Game.DEBUGFrameEnd(&GameMemory, FrameEnd, DebugRecords_Win);
                        }
                    }
                }

                CloseHandle(LogFileHandle);
            }
            else
            {
                Win32Log("Couldnt allocate memory for the game");
            }
        }
		else
    	{
        	Win32Log("Couldnt create main window");
		}
    }
    else
	{
    	Win32Log("Couldnt register class for the main window");
    }
       
    return(0); 
}

debug_record DebugRecords_Win[__COUNTER__];