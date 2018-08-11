// engine_renderer.cpp

#include "engine_renderer.h"

internal render_buffer
AllocateRenderBuffer(uint32 Size)
{
    render_buffer Result = {};

    Result.Size = Size;
    Result.Base = (uint8*)Platform.AllocateMemory(Size);
    //(uint8*)VirtualAlloc(0, RenderBuffer.Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Result.FrameRateValue = 1.0f;

    return Result;
}

internal texture_slot *
AllocateTexture(render_buffer *RenderBuffer)
{
    texture_slot *Result = 0;

    if(RenderBuffer->FreeTextureSlot)
    {
        Result = RenderBuffer->FreeTextureSlot;
        RenderBuffer->FreeTextureSlot = Result->NextFree;
        Result->NextFree = 0;
    }
    else
    {
        if(RenderBuffer->TextureCount < RENDERER_TEXTURE_SLOTS - 1)
        {
            ++RenderBuffer->TextureCount;
            Result = &RenderBuffer->TextureSlots[RenderBuffer->TextureCount];
        }
        else
        {
            Assert(!"No more available texture slots");
        }
    }

    return Result;
}

internal void
FreeTexture(render_buffer *RenderBuffer, texture_slot *Slot)
{
    Slot->NextFree = RenderBuffer->FreeTextureSlot;
    RenderBuffer->FreeTextureSlot = Slot;
}

#define CommandPushCase(Type, Command)                                      \
    case Type:                                                              \
    {                                                                       \
        RenderBuffer->Current += sizeof(Command);                           \
    } break;

#define PushCommand(Buffer, Type) PushCommand_(Buffer, RenderCommand_##Type)
internal void *
PushCommand_(render_buffer *RenderBuffer, render_command_type Type)
{
    TIMED_BLOCK();

    render_command_header *Header = (render_command_header *)RenderBuffer->Current;
    RenderBuffer->Current += sizeof(render_command_header);
    void *Result = RenderBuffer->Current;

    Header->Type = Type;
    
    switch(Type)
    {
        ForEachType(CommandPushCase);

        case RenderCommand_DrawString:
        {
            RenderBuffer->Current += sizeof(render_command_draw_string);
        } break;

        default: InvalidCodePath;
    }

    return Result;
}

internal void
PushDrawMesh(render_buffer *RenderBuffer, triangle_mesh Mesh, vec4 Color = v4(1, 0, 1, 1))
{
    render_command_draw_mesh *Command = (render_command_draw_mesh *)
        PushCommand(RenderBuffer, DrawMesh);

    Command->Mesh = Mesh;
    Command->Color = Color;
    Command->TextureSlot = 0;
    //Command->TextureSlot = TextureSlot;
    // TODO: Add UVs with texture slot.

    RenderBuffer->MeshVerticesCount += Mesh.NumberOfVertices;
}

internal render_command_draw_rect *
PushDrawRect(render_buffer *RenderBuffer, vec3 P, vec2 Dim, real32 Angle = 0, 
             vec4 Color = v4(1, 0, 1, 1), texture_slot *TextureSlot = 0)
{
    render_command_draw_rect *Command = (render_command_draw_rect *)
        PushCommand(RenderBuffer, DrawRect);

    Command->P = v3(P.xy, P.z + RenderBuffer->CameraP.z);
    Command->Dim = Dim;
    Command->Angle = Angle;
    Command->Color = Color;
    Command->TextureSlot = TextureSlot;
    // TODO: Add UVs with texture slot.
    Command->MinUV = v2(0, 0);
    Command->MaxUV = v2(1, 1);

    return Command;
}

internal void
PushDrawRectOutline(render_buffer *RenderBuffer, vec2 P, vec2 Dim, real32 Angle = 0, 
                    vec4 Color = v4(1, 0, 1, 1), texture_slot *TextureSlot = 0)
{
    render_command_draw_rect_outline *Command = (render_command_draw_rect_outline *)
        PushCommand(RenderBuffer, DrawRectOutline);

    Command->P = v3(P, RenderBuffer->CameraP.z);
    Command->Dim = Dim;
    Command->Angle = Angle;
    Command->Color = Color;
    Command->TextureSlot = TextureSlot;
}

internal void
PushDrawLine(render_buffer *RenderBuffer, vec2 Start, vec2 End, vec4 Color = v4(1, 0, 1, 1))
{
    render_command_draw_line *Command = (render_command_draw_line *)
        PushCommand(RenderBuffer, DrawLine);

    Command->Start = Start;
    Command->End = End;
    Command->Color = Color;
}

internal void
PushLoadTexture(render_buffer* RenderBuffer, texture_slot* TextureSlot, bool32 Interpolate, char* Filename)
{
    render_command_load_texture *Command = (render_command_load_texture *)
        PushCommand(RenderBuffer, LoadTexture);

    Command->TextureSlot = TextureSlot;
    Command->Interpolate = Interpolate;
    Command->Filename = Filename;
}

internal void
PushLoadTextureRaw(render_buffer *RenderBuffer, texture_slot *TextureSlot, bool Interpolate,
                   uint8 *Data, uint32 Width, uint32 Height)
{
    render_command_load_texture_raw *Command = (render_command_load_texture_raw *)
        PushCommand(RenderBuffer, LoadTextureRaw);

    Command->TextureSlot = TextureSlot;
    Command->Interpolate = Interpolate;
    Command->Data = Data;
    Command->Width = Width;
    Command->Height = Height;
}

internal void
PushLoadFont(render_buffer *RenderBuffer, font_slot *FontSlot, char *FileName, float Height)
{
    render_command_load_font *Command = (render_command_load_font *)
        PushCommand(RenderBuffer, LoadFont);

    Command->FontSlot = FontSlot;
    Command->FileName = FileName;
    Command->Height = Height;
    Command->OversampleX = 2;
    Command->OversampleY = 2;
}

internal void
PushDrawString(render_buffer *RenderBuffer, char *String, vec2 P, float Scale,
               font_slot *FontSlot = 0, vec4 Color = v4(1.0f, 1.0f, 1.0f, 1.0f))
{
    render_command_draw_string *Command = (render_command_draw_string *)
        PushCommand(RenderBuffer, DrawString);

    // TODO: Add debug font in system!
    if(FontSlot) Command->FontSlot = FontSlot;
    else Command->FontSlot = &RenderBuffer->FontSlots[2];
    Command->P = P;
    Command->Scale = Scale;
    Command->Color = Color;
    Command->CharCount = 0;

    while(*String)
    {
        *RenderBuffer->Current = (uint8)*String++;
        ++RenderBuffer->Current;
        ++Command->CharCount;
    }
}

internal void
PushSetCameraMatrix(render_buffer *RenderBuffer, mat4 Matrix)
{
    render_command_set_camera_matrix *Command = (render_command_set_camera_matrix *)
    PushCommand(RenderBuffer, SetCameraMatrix);

    Command->Matrix = Matrix;
}

inline render_command_type
GetNextType(render_buffer *RenderBuffer)
{
    render_command_header *Header = (render_command_header *)RenderBuffer->Current;
    RenderBuffer->Current += sizeof(render_command_header);
    return Header->Type;
}

#define CommandGetCase(Type, Command)                                       \
    case Type:                                                              \
    {                                                                       \
        RenderBuffer->Current += sizeof(Command);                           \
    } break;

internal void *
GetNextCommand(render_buffer* RenderBuffer, render_command_type Type)
{
    void *Result = RenderBuffer->Current;

    switch(Type)
    {
        ForEachType(CommandGetCase);

        case RenderCommand_DrawString:
        {
            render_command_draw_string *Command = (render_command_draw_string *)RenderBuffer->Current;
            RenderBuffer->Current += sizeof(render_command_draw_string) + Command->CharCount;
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    };

    return Result;
}

internal vec3
Unproject(render_buffer *RenderBuffer, vec2 ScreenCoords)
{
    vec2 ScreenCenter = 0.5f*v2(RenderBuffer->Width, RenderBuffer->Height);
   
    vec2 UnprojectedXY = (ScreenCoords - ScreenCenter) * (1.0f/RenderBuffer->MetersToPixels);
    vec3 Result = v3(UnprojectedXY * (RenderBuffer->CameraP.z + 1) - RenderBuffer->CameraP.xy, 0);

    return Result;
}

/*
m00, m01, m02, m03;
m10, m11, m12, m13;
m20, m21, m22, m23;
m30, m31, m32, m33;
*/

inline mat4
Orthographic(float Left, float Right, float Bottom, float Top)
{
    mat4 Result = Identity();
    
    Result.m00 = 2.0f / (Right - Left);
    Result.m11 = 2.0f / (Top - Bottom);
    Result.m22 = 1.0f;
    Result.m32 = 1.0f;
    
    return Result;
}

// TODO: Will we need perspective projection like this?
inline mat4
Perspective(real32 AspectRatio, float FOV)
{
    float zNear = -5.0f;
    float zFar = 1.0f;
    float zRange = zNear - zFar;
    float TanHalfFOV = tanf(ToRadians(FOV/2.0f));

    mat4 Result = Identity();

    Result.m00 = 1.0f/(TanHalfFOV*AspectRatio);
    Result.m11 = 1.0f/TanHalfFOV; 
    Result.m22 = (-zNear - zFar)/zRange; 
    Result.m23 = 2.0f*zFar * zNear/zRange;
    Result.m32 = 1.0f;

    return Result;
}